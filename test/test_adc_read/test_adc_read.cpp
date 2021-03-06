#include <Arduino.h>

#include "audio.h"

void setup() {
  Serial.begin(115200);
  configure_audio(2);
}

const size_t kReadings = 4;

float gRms[kReadings];
uint16_t gMin[kReadings];
uint16_t gMax[kReadings];

void loop() {
  static uint8_t processedBuffer = 1;
  static uint16_t errors = 0;
  static size_t index = 0;
  uint16_t min, max;

  if (processedBuffer == gCurrentBuffer) {
    uint16_t startCount = gMeasurementsCount;
    uint8_t bufferToProcess = (processedBuffer == 0) ? 1 : 0;
    window_min_max(bufferToProcess, min, max);
    gMin[index] = min;
    gMax[index] = max;
    gRms[index++] = compute_rms(bufferToProcess);
    uint16_t elapsedCount = gMeasurementsCount - startCount;
    if (elapsedCount >= kSamplesPerMeasurement) {
      errors++;
    }
    processedBuffer = bufferToProcess;
  }

  if (index >= kReadings) {
    Serial.print("(rms, min max) : ");
    for (size_t i = 0; i < kReadings; i++) {
      Serial.print("(");
      Serial.print(gRms[i]);
      Serial.print(", ");
      Serial.print(gMin[i]);
      Serial.print(", ");
      Serial.print(gMax[i]);
      Serial.print(") ");
    }
    Serial.print(" | errors=");
    Serial.println(errors);

    index = 0;
    gMeasurementsCount = 0;
  }
}
