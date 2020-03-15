#include <Arduino.h>
#include <RF24.h>
#include <SPI.h>

#define PIR_sensor 3
#define control_Device 4
#define default_delayTime 3000

RF24 radio(9, 10);                     //nRF24L01 (CE,CSN) connections PIN
const uint64_t address = 401584261612; //CA-SS02: 40 + Timestamp

uint16_t delayTime = default_delayTime;
uint16_t new_delayTime;
boolean state_Device;

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
}

void loop()
{
  radio.openReadingPipe(1, address);
  radio.startListening();
  if (radio.available())
  {
    memset(&new_delayTime, ' ', sizeof(new_delayTime));
    radio.read(&new_delayTime, sizeof(new_delayTime));
    Serial.println("New delay time: ");
    Serial.println(new_delayTime);
    delayTime = new_delayTime;
  }

  boolean check_PIRSensor = digitalRead(PIR_sensor);
  if (check_PIRSensor)
  {
    digitalWrite(control_Device, HIGH);
    state_Device = true;
    radio.stopListening();
    radio.openWritingPipe(address);
    radio.write(&state_Device, sizeof(state_Device));
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
    state_Device = false;
    radio.stopListening();
    radio.openWritingPipe(address);
    radio.write(&state_Device, sizeof(state_Device));
  }
}