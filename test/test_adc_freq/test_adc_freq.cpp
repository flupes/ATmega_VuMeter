#include "audio.h"

#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  configure_audio(2);
}

uint32_t t, t0;

size_t kSamples = 4;

void loop() {
  if (gMeasurementsCount >= 10 * kSamplesPerMeasurement) {
    t = micros() - t0;  // calculate elapsed time

    Serial.print("Sampling frequency: ");
    Serial.print(1000.0 * 10 * kSamplesPerMeasurement / t);
    Serial.print(" KHz ");
    Serial.print(kSampleFrequency);
    Serial.print(" ");
    Serial.println(kSamplesPerMeasurement);

    t0 = micros(); // restart
    gMeasurementsCount = 0;
  }
}

// Output:
// Sampling frequency: 7.69 KHz 7211 288
// Sampling frequency: 7.69 KHz 7211 288
// Sampling frequency: 7.69 KHz 7211 288
