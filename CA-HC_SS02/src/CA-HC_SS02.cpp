#include <Arduino.h>
#include <RF24.h>
#include <SPI.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Ticker.h>

Ticker ticker;
RF24 radio(2, 15);                     //nRF24L01 (CE,CSN) connections PIN
const uint64_t address = 401584261612; //CA-SS02: 40 + Timestamp
boolean smartConfigStart = false;

const char *ssid = "username wifi";
const char *password = "password wifi";

uint32_t data[3]; //data[0]: ON/OFF - data[1]: new_delayTime - data[2]: state_Device
const int smartConfig_LED = 16;

//Topic: product_id/button_id     char[10] = "l" / "O"
const char *CA_SS02_delayTime = "CA-SS02-delayTime";
const char *CA_SS02_deviceState = "CA-SS02-deviceState";
const char *CA_SS02_ONOFF = "CA-SS02-ONOFF";

//Config MQTT broker information:
const char *mqtt_server = "chika.gq";
const int mqtt_port = 2502;
const char *mqtt_user = "chika";
const char *mqtt_pass = "2502";

//Setup MQTT - Wifi ESP12F:
WiFiClient esp_12F;
PubSubClient client(esp_12F);

void setup_Wifi()
{
  delay(100);
  Serial.println();
  Serial.print("Connecting to ... ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected!");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

/*--------------NEEDED FUNCTIONS--------------*/
void blinking()
{
  bool state = digitalRead(smartConfig_LED);
  digitalWrite(smartConfig_LED, !state);
}

void exitSmartConfig()
{
  WiFi.stopSmartConfig();
  ticker.detach();
}

boolean startSmartConfig()
{
  int t = 0;
  Serial.println("Smart Config Start");
  WiFi.beginSmartConfig();
  delay(500);
  ticker.attach(0.1, blinking);
  while (WiFi.status() != WL_CONNECTED)
  {
    t++;
    Serial.print(".");
    delay(500);
    if (t > 120)
    {
      Serial.println("Smart Config Fail");
      smartConfigStart = false;
      ticker.attach(0.5, blinking);
      delay(3000);
      exitSmartConfig();
      return false;
    }
  }
  smartConfigStart = true;
  Serial.println("WIFI CONNECTED");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.SSID());
  exitSmartConfig();
  return true;
}

void reconnect_mqtt()
{
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    String clientId = "CA-SS02 - ";
    clientId += String(random(0xffff), HEX);
    Serial.println(clientId);

    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass))
    {
      Serial.println("Connected");
      client.subscribe(CA_SS02_delayTime);
      client.subscribe(CA_SS02_ONOFF);
    }
    else
    {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println("Try again in 1 second");
      delay(1000);
    }
  }
}

void callback(char *topic, byte *payload, unsigned int length)
{
  //Topic list test is the value of variables: CA_HC_SS02
  Serial.print("Topic [");
  Serial.print(topic);
  Serial.print("]: ");

  uint16_t calculate_delayTime = 0;
  for (unsigned int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
    calculate_delayTime += ((uint16_t)payload[i] - 48) * (uint16_t)(pow(10, length - 1));
  }

  if ((char)topic[10] == 'l')
  {
    data[1] = calculate_delayTime * 1000;
    Serial.print("\n\nNew delay time send to CA-SS02: ");
    Serial.println(data[1]);
    radio.stopListening();
    radio.openWritingPipe(address);
    radio.write(&data, sizeof(data));
  }

  if ((char)topic[10] == 'O')
  {
    if ((char)payload[0])
    {
      data[0] = true;
    }
    else
    {
      data[0] = false;
    }
    
    radio.stopListening();
    radio.openWritingPipe(address);
    radio.write(&data, sizeof(data));
  }
}

void setup()
{
  SPI.begin();
  Serial.begin(115200);
  pinMode(smartConfig_LED, OUTPUT);

  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);
  WiFi.mode(WIFI_STA);

  //      setup_Wifi();
  delay(6000);

  if (WiFi.status() != WL_CONNECTED)
  {
    startSmartConfig();
  }

  radio.begin();
  radio.setRetries(15, 15);
  radio.setPALevel(RF24_PA_MAX);

  Serial.println("WIFI CONNECTED");
  Serial.println(WiFi.SSID());
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  Serial.println("\nCA-HC-SS02 say hello to your home <3 !");

  Serial.println("Trying connect MQTT ...");
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  data[1] = 0;
}

void loop()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    if (!client.connected())
    {
      reconnect_mqtt();
    }
    else
      client.loop();
  }

  radio.openReadingPipe(1, address);
  radio.startListening();
  if (radio.available())
  {
    memset(&data, ' ', sizeof(data));
    radio.read(&data, sizeof(data));

    if (data[2])
      client.publish(CA_SS02_deviceState, "1", true);
    else
      client.publish(CA_SS02_deviceState, "0", true);
  }
  delay(100);
}