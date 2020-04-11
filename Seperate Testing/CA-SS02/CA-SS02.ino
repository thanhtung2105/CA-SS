#include <Arduino.h>
#include <RF24.h>
#include <SPI.h>
#include <EEPROM.h>

#define PIR_sensor 3
#define control_Device 4
#define signal_Led 5
#define default_delayTime 3000

RF24 radio(9, 10);                      //nRF24L01 (CE,CSN) connections PIN
const uint64_t address = 1002502019004; //CA-SS02: 40 + Timestamp

uint16_t delayTime = default_delayTime;
float data[3]; //data[0]: ON/OFF - data[1]: new_delayTime - data[2]: state_Device

void checkMode()
{
  data[0] = EEPROM.read(0);
  Serial.print("Mode in EEPROM: ");
  Serial.println(data[0]);
}

void setup()
{
  SPI.begin();
  Serial.println("\nCA-SS02 say hello to your home <3 ! ");
  Serial.begin(9600);
  checkMode();
  pinMode(PIR_sensor, INPUT);
  pinMode(control_Device, OUTPUT);
  pinMode(signal_Led, OUTPUT);

  radio.begin();
  radio.setRetries(15, 15);
  radio.setPALevel(RF24_PA_MAX);
}

void checkCommand()
{
  radio.openReadingPipe(1, address);
  radio.startListening();
  if (radio.available())
  {
    boolean state = digitalRead(signal_Led);
    memset(&data, ' ', sizeof(data));
    radio.read(&data, sizeof(data));
    
    delayTime = (int)data[1];
    Serial.println("__________________");
    Serial.print("Mode: ");
    Serial.println(data[0]);
    Serial.print("New delay time: ");
    Serial.println(data[1]);
    Serial.print("State device: ");
    Serial.println(data[2]);
    digitalWrite(signal_Led, !state);
    delay(400);
    digitalWrite(signal_Led, state);
  }
}

void sendData()
{
  radio.stopListening();
  radio.openWritingPipe(address);
  radio.write(&data, sizeof(data));
  Serial.print("Data sent - State of device: ");
  Serial.println(data[2]);
}

void loop()
{
  checkCommand();
  if ((boolean)data[0])
  {
    EEPROM.update(0, 1);
    digitalWrite(signal_Led, HIGH);
    checkCommand();
    boolean check_PIRSensor = digitalRead(PIR_sensor);
    if (check_PIRSensor)
    {
      digitalWrite(control_Device, HIGH);
      data[2] = (float)1;
      sendData();
      while (check_PIRSensor)
      {
        check_PIRSensor = digitalRead(PIR_sensor);
        checkCommand();
        delay(5);
      }
      delay(delayTime);
    }
    else
    {
      digitalWrite(control_Device, LOW);
      if (digitalRead(control_Device) != data[2])
      {
        data[2] = (float)0;
        sendData();
      }
    }
    checkCommand();
  }
  else
  {
    EEPROM.update(0, 0);
    digitalWrite(control_Device, LOW);
    digitalWrite(signal_Led, LOW);
    if (digitalRead(control_Device) != data[2])
    {
      data[2] = (float)0;
      sendData();
    }
  }
}
