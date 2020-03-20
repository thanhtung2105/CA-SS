#include <Arduino.h>
#include <SPI.h>
#include <RF24.h>

#define pinFlame A0
#define pinGas  A1
#define CE 9
#define CSN 10

RF24 radio(CE, CSN);

const uint64_t address = 1002502019006;

float sensorValue[2];
float flame, gas;

uint16_t timer = 0;

void setup() {
  Serial.begin(9600);
  //=================RF=====================
  SPI.begin();
  radio.begin();
  radio.setRetries(15, 15);
  radio.setPALevel(RF24_PA_MAX);
  radio.openWritingPipe(address);
  //========================================
  
}

void loop() {
  flame = analogRead(A0);
  gas = analogRead(A1);

  String output;
  output += F("Flame : ");
  output += flame;
  output += F("\t");
  output += F("gas :");
  output += gas;

  Serial.println(output);

  sensorValue[0] = flame;
  sensorValue[1] = gas;

  timer++;
  if(timer > 50){
    Serial.println("send ...");
    radio.write(sensorValue, sizeof(sensorValue));
    timer = 0;
  }
  delay(100);
}