#include <Arduino.h>
#include <RF24.h>
#include <SPI.h>

#define PIR_sensor 3
#define control_Device 4
#define default_delayTime 3000
RF24 radio(9, 10);                     //nRF24L01 (CE,CSN) connections PIN
const uint64_t address = 401584261612; //CA-SS02: 40 + Timestamp

uint16_t delayTime = default_delayTime;
uint32_t data[3]; //data[0]: ON/OFF - data[1]: new_delayTime - data[2]: state_Device

void setup()
{
  SPI.begin();
  Serial.println("\nCA-SS02 say hello to your home <3 ! ");
  Serial.begin(9600);

  pinMode(PIR_sensor, INPUT);
  pinMode(control_Device, OUTPUT);

  radio.begin();
  radio.setRetries(15, 15);
  radio.setPALevel(RF24_PA_MAX);
  data[0] = 0;
}

void loop()
{
  radio.openReadingPipe(1, address);
  radio.startListening();
  if (radio.available())
  {
    memset(&data, ' ', sizeof(data));
    radio.read(&data, sizeof(data));

    delayTime = data[1];
    Serial.println("__________________");
    Serial.print("Mode: ");
    Serial.println(data[0]);
    Serial.print("New delay time: ");
    Serial.println(data[1]);
    Serial.print("State device: ");
    Serial.println(data[2]);
  }

  if (data[0])
  {
    boolean check_PIRSensor = digitalRead(PIR_sensor);
    if (check_PIRSensor)
    {
      digitalWrite(control_Device, HIGH);
      data[2] = true;
      radio.stopListening();
      radio.openWritingPipe(address);
      radio.write(&data, sizeof(data));
      while (check_PIRSensor)
      {
        check_PIRSensor = digitalRead(PIR_sensor);
        delay(100);
      }
      delay(delayTime);
    }
    else
    {
      digitalWrite(control_Device, LOW);
      data[2] = false;
      radio.stopListening();
      radio.openWritingPipe(address);
      radio.write(&data, sizeof(data));
    }
  }
  delay(100);
}
