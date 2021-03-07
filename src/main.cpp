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

const float kNightMaxRms = 80;
const float kDayMaxRms = 130;

const uint8_t kFlashingCyclesCount = 250 / kMeasurementPeriodMs;
const uint8_t kFlashingLoops = 15;

void loop() {
  static float topRms = kDayMaxRms;
  // static float topRms = kNightMaxRms;
  static float gain = 249.9 / log(topRms / kRmsRef);
  static float loudRms = 0.8 * topRms;

  static size_t levelsHead = 0;
  static uint8_t processedBuffer = 1;
  static uint8_t flashingLoops = 0;
  static uint8_t cycleCount = 0;

  static uint32_t elapsed[4] = {0, 0, 0, 0};
  static uint32_t timing;
  static uint16_t samplesCounter = 0;
  static uint8_t statCounter = 0;

  if (processedBuffer == gCurrentBuffer) {
    uint32_t t0 = micros();
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

    uint32_t t1 = micros();
    elapsed[0] += t1 - t0;

    // Fill the strip
    size_t index = (levelsHead + 1) % kNumberOfLeds;  // oldest reading first
    for (size_t i = 0; i < kNumberOfLeds; i++) {
      uint8_t hue = map(gLevelsRingBuffer[index++], 16, 255, 96, -16);
      gLeds[i].setHSV(hue, 255, val);
      if (index >= kNumberOfLeds) {
        index = 0;
      };
    }

    uint32_t t2 = micros();
    elapsed[1] += t2 - t1;

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

    statCounter++;
    uint32_t t3 = micros();
    elapsed[2] += t3 - t2;
    elapsed[3] += t3 - t0;

    if (statCounter >= 100) {
      uint32_t period = t3 - timing;
      Serial.print("average active loop time: rms=");
      Serial.print(1E-3f * elapsed[0] / 100);
      Serial.print("ms, set=");
      Serial.print(1E-3f * elapsed[1] / 100);
      Serial.print("ms, show=");
      Serial.print(1E-3f * elapsed[2] / 100);
      Serial.print("ms --> total=");
      Serial.print(1E-3f * elapsed[3] / 100);

      Serial.print(" | sampling frequency (kHz) : ");
      Serial.print(1E3f * (gMeasurementsCount - samplesCounter) / period);
      Serial.println();

      for (size_t i = 0; i < 4; i++) {
        elapsed[i] = 0;
      }
      samplesCounter = gMeasurementsCount;
      statCounter = 0;
      timing = micros();
    }
  }
}
