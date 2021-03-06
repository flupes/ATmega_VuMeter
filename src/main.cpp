#include <Arduino.h>
#include <FastLED.h>

#include "audio.h"

#define DATA_PIN 6
#define LED_TYPE WS2812
#define COLOR_ORDER GRB

const size_t kNumberOfLeds = 60;
CRGB gLeds[kNumberOfLeds];
uint8_t gLevelsRingBuffer[kNumberOfLeds];
uint8_t gHuesTable[255];

void setup() {
  Serial.begin(115200);
  for (size_t i = 0; i < kNumberOfLeds; i++) {
    gLevelsRingBuffer[i] = 0;
  }

  configure_audio(2);

  delay(3000);  // 3 second delay for recovery

  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(gLeds, kNumberOfLeds);
  FastLED.setCorrection(TypicalLEDStrip);
}

// Max RMS when the 10 bit ADC is saturated -512 --> +512
// Integral( (peak*sin(x))^2 = 1/2 * peak^2
const float kRms10Max = 512.0 * 512.0 / 2.0;

// RMS we consider is very high volume and should saturate the 8 bit level
const float kSaturationRms = 4000.0;

// Smaller RMS computed when mic is very quiet
const float kRmsRef = 2.0;

// Saturation gain that will make the pseudoDb the max value of a 8 bit int
// pseudoDb = kSaturationGain * log (rms / kRmsRef)
const float kSaturationGain = 254.9 / log(kSaturationRms / kRmsRef);

const uint8_t kFlashingCyclesCount = 250 / kMeasurementPeriodMs;
const uint8_t kFlashingLoops = 15;

void loop() {
  static float topRms = 100.0;
  static float gain = 249.0 / log(topRms / kRmsRef);
  static float loudRms = 0.8 * topRms;

  static size_t levelsHead = 0;
  static uint8_t processedBuffer = 1;
  static uint8_t flashingLoops = 0;
  static uint8_t cycleCount = 0;

  if (processedBuffer == gCurrentBuffer) {
    uint16_t countStart = gMeasurementsCount;
    uint8_t bufferToProcess = 1 - processedBuffer;

    // Store the last sound level
    float rms = compute_rms(bufferToProcess);
    if (rms > topRms) {
      rms = topRms;
    }

    gLevelsRingBuffer[levelsHead] =
        static_cast<uint8_t>(round(gain * logf(rms / kRmsRef)));

    // Detect above desirable level
    if (rms > loudRms) {
      flashingLoops = kFlashingLoops;
      cycleCount = kFlashingCyclesCount;
    }

    uint8_t val = 160;
    // Start flashing when loud
    if (flashingLoops > 0) {
      val = (flashingLoops % 2 == 0) ? 31 : 255;
      cycleCount--;
      if (cycleCount == 0) {
        flashingLoops--;
        if (flashingLoops > 0) {
          cycleCount = kFlashingCyclesCount;
        }
      }
    }

    // Fill the strip
    size_t index = (levelsHead + 1) % kNumberOfLeds;  // oldest reading first
    for (size_t i = 0; i < kNumberOfLeds; i++) {
      uint8_t hue = map(gLevelsRingBuffer[index++], 16, 255, 96, -16);
      gLeds[i].setHSV(hue, 255, val);
      if (index >= kNumberOfLeds) {
        index = 0;
      };
    }

    // Display new pattern
    FastLED.show();

    levelsHead++;
    if (levelsHead >= kNumberOfLeds) {
      levelsHead = 0;
    }

    processedBuffer = bufferToProcess;

    if ((gMeasurementsCount - countStart) > kSamplesPerMeasurement / 2) {
      audio_error(11);
    }
  }
}