#include <Arduino.h>

const size_t kMaxSamples = 288;

uint16_t gSamples[kMaxSamples];

void setup() {
  Serial.begin(115200);
}

void loop()
{
  for (size_t i=0; i<kMaxSamples; i++) {
    gSamples[i] = random(1024);
  }
  uint32_t start = micros();
  uint32_t sum = 0;
  for (size_t i=0; i<kMaxSamples; i++) {
    sum += gSamples[i]*gSamples[i];
  }
  uint32_t elapsed = micros() - start;
  float rms = sqrt((float)sum / (float)kMaxSamples);
  Serial.print("rms = ");
  Serial.print(rms);
  Serial.print(" / elapsed (us) = ");
  Serial.println(elapsed);
  delay(1000);
}
