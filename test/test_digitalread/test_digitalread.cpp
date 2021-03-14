#include "Arduino.h"

const uint8_t kLightLevelPin = 8;
const uint8_t kReedRelayPin = 4;

void setup() {
  Serial.begin(115200);

  pinMode(kLightLevelPin, INPUT);
  pinMode(kReedRelayPin, INPUT);

  delay(1000);
}

void loop() {
  static uint32_t accumulator = 0;
  static uint16_t counter = 0;
  static uint16_t changes[2];
  static uint8_t last[2];

  uint8_t d[2];
  uint32_t t0 = micros();

  d[0] = digitalRead(kLightLevelPin);
  d[1] = digitalRead(kReedRelayPin);

  accumulator += micros() - t0;
  counter++;

  for (size_t i = 0; i < 2; i++) {
    changes[i] += (last[i] == d[i]) ? 0 : 1;
    last[i] = d[i];
  }

  if (counter >= 10000) {
    Serial.print("Avg. for 2 x DigitalRead over 10000 cycles: ");
    Serial.print(accumulator / 10000);
    Serial.print("us | # changes=[");
    Serial.print(changes[0]);
    Serial.print(", ");
    Serial.print(changes[1]);
    Serial.println("]");
    counter = 0;
    accumulator = 0;
    for (size_t i = 0; i < 2; i++) {
      changes[0] = 0;
    }
  }
}

// Sample output:
// Avg. for 2 x DigitalRead over 10000 cycles: 14us | # changes=[0, 0]
// Avg. for 2 x DigitalRead over 10000 cycles: 14us | # changes=[0, 0]
// Avg. for 2 x DigitalRead over 10000 cycles: 14us | # changes=[0, 0]
