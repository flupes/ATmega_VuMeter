#include <Arduino.h>

const uint8_t kSensitivityPotPin = A4;

const float kNightMaxRms = 70.0;

void setup()
{
  Serial.begin(115200);
}

void loop()
{
  delay(500);
  uint16_t v = analogRead(kSensitivityPotPin);
  int16_t s = map(v, 520, 1023, -100, 100);
  float rms = kNightMaxRms + 0.5f * s;
  Serial.print("v=");
  Serial.print(v);
  Serial.print(" --> s=");
  Serial.print(s);
  Serial.print(" ==> rms=");
  Serial.println(rms);
}
