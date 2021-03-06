#include "audio.h"

#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  configure_audio(2);
}

uint32_t t, t0;

void loop() {
  static uint8_t processedBuffer = 1;
  static uint16_t errors = 0;

  static float avg_rms = 0.0;
  
  if ( processedBuffer == gCurrentBuffer ) {
    uint16_t startCount = gMeasurementsCount;
    uint8_t bufferToProcess = (processedBuffer == 0)? 1 : 0;
    float rms = compute_rms(bufferToProcess);
    avg_rms += rms;
    uint16_t elapsedCount = gMeasurementsCount - startCount;
    if (elapsedCount > kSamplesPerMeasurement) {
      errors++;
    }
    processedBuffer = bufferToProcess;
  }

  if (gMeasurementsCount >= 10 * kSamplesPerMeasurement) {
    t = micros() - t0;  // calculate elapsed time

    Serial.print("Sampling frequency: ");
    Serial.print(1000.0 * 10 * kSamplesPerMeasurement / t);
    Serial.print(" KHz ");
    Serial.print(kSampleFrequency);
    Serial.print(" ");
    Serial.print(kSamplesPerMeasurement);

    Serial.print(" --> RMS = ");
    Serial.print(avg_rms/10.0);
    avg_rms = 0.0;

    Serial.print(" | errors=");
    Serial.println(errors);

    t0 = micros(); // restart
    gMeasurementsCount = 0;
  }
}
