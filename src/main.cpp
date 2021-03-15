#include <Arduino.h>
#include <FastLED.h>

#include "audio.h"

#define DATA_PIN 6
#define LED_TYPE WS2812
#define COLOR_ORDER GRB

// #define PRINT_STATS

const size_t kNumberOfLeds = 60;
CRGB gLeds[kNumberOfLeds];
uint8_t gLevelsRingBuffer[kNumberOfLeds];
uint8_t gHuesTable[255];

// Smaller RMS computed when mic is very quiet
const float kRmsRef = 3.0f;

// Value under which it is considered quiet enough to turn on the breathing
// pattern
const float kRmsThreshold = 6.0f;

// Max RMS acceptable at night
const float kNightMaxRms = 60.0f;

// Max RMS acceptable during daytime
const float kDayMaxRms = 120.0f;

const uint8_t kButtonLedPin = 11;
const uint8_t kButtonInputPin = 12;

const uint8_t kAudioAnalogPin = 2;

const uint8_t kLightLevelPin = 8;
const uint8_t kReedRelayPin = 4;

const uint8_t kSensitivityPotPin = A4;

// Number of seconds before declaring quiet time
const uint16_t kQuietSeconds = 20;

const uint16_t kNightBrightness = 90;
const uint16_t kDayBrightness = 140;

float gSilenceFactor = 1.0f;
uint8_t gNightTime = 1;
uint8_t gDoorClosed = 1;

uint16_t gAnalogOffset = 0;
float gRmsOffset = 0.0f;

void setup() {
  Serial.begin(115200);
  for (size_t i = 0; i < kNumberOfLeds; i++) {
    gLevelsRingBuffer[i] = 0;
  }

  pinMode(kButtonLedPin, OUTPUT);
  digitalWrite(kButtonLedPin, gNightTime);
  pinMode(kButtonInputPin, INPUT);

  pinMode(kLightLevelPin, INPUT);
  pinMode(kReedRelayPin, INPUT);

  delay(3000);  // 3 second delay for recovery

  Serial.print(F("Sampling Period :             "));
  Serial.print(kMeasurementPeriodMs);
  Serial.println(F(" ms"));

  Serial.print(F("Sampling Frequency :          "));
  Serial.print(kSampleFrequency);
  Serial.println((" Hz"));

  Serial.print(F("Samples per RMS measurement : "));
  Serial.print(kSamplesPerMeasurement);
  Serial.println(F(" count"));

  // Read audio sensitivity from potentiometer (only at startup since after that
  // we use a custom free-run reading with interrupts)
  gAnalogOffset = analogRead(kSensitivityPotPin);

  // 5K pot is creating a voltage divisor with a ~5K resistor
  int16_t offset = map(gAnalogOffset, 520, 1023, -100, 100);
  gRmsOffset = 0.5f * offset;

  Serial.print(F("RMS Offset = "));
  Serial.println(gRmsOffset);

  // Configure the special anolog read for audio
  configure_audio(kAudioAnalogPin);

  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(gLeds, kNumberOfLeds);
  FastLED.setCorrection(TypicalLEDStrip);
}

// Max RMS when the 10 bit ADC is saturated -512 --> +512 (for reference only)
// Integral( (peak*sin(x))^2 = 1/2 * peak^2
// const float kRms10Max = 512.0 * 512.0 / 2.0;

// RMS we consider is very high volume and should saturate the 8 bit level
// const float kSaturationRms = 4000.0;

// Saturation gain that will make the pseudoDb the max value of a 8 bit int
// pseudoDb = kSaturationGain * log (rms / kRmsRef)
// const float kSaturationGain = 254.9 / log(kSaturationRms / kRmsRef);

const uint8_t kFlashingCyclesCount = 250 / kMeasurementPeriodMs;
const uint8_t kFlashingLoops = 15;

void processDigitalInputs() {
  static uint8_t lastLight = gNightTime;
  static uint8_t lastState = gDoorClosed;

  static bool buttonDown = false;
  static bool buttonPressed = false;
  static bool buttonActedOn = false;

  static uint16_t bounceCounts[2] = {0, 0};
  const uint8_t kDebounceCounter = 20;

  uint8_t buttonState = digitalRead(kButtonInputPin);
  if (buttonState == 0) {
    if (buttonDown) {
      buttonPressed = true;
    }
    buttonDown = true;
  } else {
    buttonDown = false;
    buttonPressed = false;
    buttonActedOn = false;
  }

  if (buttonPressed) {
    if (!buttonActedOn) {
      Serial.println(F("Button Pressed!"));
      buttonActedOn = true;
    }
  }

  uint8_t dark = digitalRead(kLightLevelPin);
  if (dark) {
    if (bounceCounts[0] < kDebounceCounter) {
      bounceCounts[0]++;
    } else {
      gNightTime = 1;
    }
  } else {
    if (bounceCounts[0] > 0) {
      bounceCounts[0]--;
    } else {
      gNightTime = 0;
    }
  }

  if (lastLight != gNightTime) {
    digitalWrite(kButtonLedPin, gNightTime);
    lastLight = gNightTime;
  }

  uint8_t shut = digitalRead(kReedRelayPin);
  if (shut) {
    if (bounceCounts[1] < kDebounceCounter) {
      bounceCounts[1]++;
    } else {
      gDoorClosed = 1;
    }
  } else {
    if (bounceCounts[1] > 0) {
      bounceCounts[1]--;
    } else {
      gDoorClosed = 0;
    }
  }

  if (lastState != gDoorClosed) {
    gSilenceFactor = gDoorClosed ? 1.0f : 0.75f;
    Serial.print(F("Door State: "));
    Serial.println(gDoorClosed);
    lastState = gDoorClosed;
  }
}

void loop() {
  static size_t levelsHead = 0;
  static uint8_t processedBuffer = 1;
  static uint8_t flashingLoops = 0;
  static uint8_t cycleCount = 0;

  static uint32_t elapsed[4] = {0, 0, 0, 0};
  static uint32_t timing = 0;
  static uint16_t samplesCounter = 0;
  static uint16_t quietCounter = 25 * kQuietSeconds;
  static uint8_t breathingCounter = 0;
  static uint8_t statCounter = 0;

  if (processedBuffer == gCurrentBuffer) {
    processDigitalInputs();

    uint32_t t0 = micros();
    uint16_t countStart = gMeasurementsCount;
    uint8_t bufferToProcess = 1 - processedBuffer;
    float rms = compute_rms(bufferToProcess);

    // Store the last sound level
    float topRms = kDayMaxRms;
    if (gNightTime) {
      topRms = gSilenceFactor * (kNightMaxRms + gRmsOffset);
    }
    if (rms > topRms) {
      rms = topRms;
    }

    float gain = 249.9 / log(topRms / kRmsRef);
    float loudRms = 0.8 * topRms;

    gLevelsRingBuffer[levelsHead] =
        static_cast<uint8_t>(round(gain * logf(rms / kRmsRef)));

    // Serial.print(" gain=");
    // Serial.print(gain);
    // Serial.print(" rms=");
    // Serial.print(rms);
    // Serial.print(" pdb=");
    // Serial.println(gLevelsRingBuffer[levelsHead]);

    // Detect quiet time
    if (rms < kRmsThreshold) {
      if (quietCounter > 0) {
        quietCounter--;
      }
    } else {
      quietCounter = 25 * kQuietSeconds;
    }

    // Detect above desirable level
    if (rms > loudRms) {
      flashingLoops = kFlashingLoops - flashingLoops % 2;
      cycleCount = kFlashingCyclesCount;
    }

    uint32_t t1 = micros();
    elapsed[0] += t1 - t0;

    const uint8_t low = 6;
    const uint8_t high = 24;
    if (quietCounter == 0) {
      uint8_t blue = gLeds[0].blue;
      uint8_t green = gLeds[0].green;
      if (blue != green) {
        if (green > low) {
          if (blue < low) {
            blue += 1;
          }
          green -= 1;
          fill_solid(gLeds, kNumberOfLeds, CRGB(0, green, blue));
          breathingCounter = 0;
        }
      } else {
        uint8_t level = quadwave8(breathingCounter);
        level = map8(level, low, high);
        fill_solid(gLeds, kNumberOfLeds, CRGB(0, level, level));
        breathingCounter += 2;
      }
    } else {
      uint8_t val = gNightTime ? kNightBrightness : kDayBrightness;
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
        uint8_t hue = map(gLevelsRingBuffer[index++], 4, 255, 96, -16);
        gLeds[i].setHSV(hue, 255, val);
        if (index >= kNumberOfLeds) {
          index = 0;
        };
      }
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

    if ((gMeasurementsCount - countStart) >= kSamplesPerMeasurement) {
#if defined(PRINT_STATS)
      Serial.print(F("ERROR: window exceeded --> "));
      Serial.print(F("loop time: rms="));
      Serial.print(1E-3f * elapsed[0]);
      Serial.print(F("ms, set="));
      Serial.print(1E-3f * elapsed[1]);
      Serial.print(F("ms, show="));
      Serial.print(1E-3f * elapsed[2]);
      Serial.print(F(" | measurement cycles count="));
      Serial.println(gMeasurementsCount - countStart);
#endif
      // audio_error(11);
      // Output:
      // average active loop time: rms=1.37ms, set=4.12ms, show=1.43ms -->
      // total=6.92 | sampling frequency (kHz) : 7.47 ERROR: window exceeded -->
      // loop time: rms=4.13ms, set=12.36ms, show=4.12 | measurement cycles
      // count=65347 Something not-ideal has happened, but no need to panic: The
      // we may have skipped a measurement but the update will resume correctly
      // Actually, the message above looks like a wrap-around issue, however it
      // should not happen from what I understand of my code.
    }

    statCounter++;
    uint32_t t3 = micros();
    elapsed[2] += t3 - t2;
    elapsed[3] += t3 - t0;

    if (statCounter >= 100) {
#if defined(PRINT_STATS)
      uint32_t period = t3 - timing;
      Serial.print(F("avg. loop: rms="));
      Serial.print(1E-3f * elapsed[0] / 100);
      Serial.print(F("ms, set="));
      Serial.print(1E-3f * elapsed[1] / 100);
      Serial.print(F("ms, show="));
      Serial.print(1E-3f * elapsed[2] / 100);
      Serial.print(F("ms --> total="));
      Serial.print(1E-3f * elapsed[3] / 100);

      Serial.print(F(" | Sampling Freq (kHz) : "));
      Serial.print(1E3f * (gMeasurementsCount - samplesCounter) / period);
      Serial.println();

#endif
      for (size_t i = 0; i < 4; i++) {
        elapsed[i] = 0;
      }
      samplesCounter = gMeasurementsCount;
      statCounter = 0;
      timing = micros();
    }
  } else {
    // Let the processor sleep a little bit ;-)
    // This is useless, but an interesting concept: the loop should still cycle
    // 6 times before a sample buffer is ready... So the gain all depends of the
    // Arduino delay implementation :-/
    delay(6);
  }
}

// Memory footprint:
// RAM:   [========= ]  85.8% (used 1757 bytes from 2048 bytes)
// Flash: [===       ]  34.2% (used 9792 bytes from 28672 bytes)

// Output:
// avg. loop: rms=1.35ms, set=4.10ms, show=1.41ms --> total=6.87 | Sampling Freq (kHz) : 7.47
// avg. loop: rms=1.35ms, set=4.10ms, show=1.37ms --> total=6.83 | Sampling Freq (kHz) : 7.48
// avg. loop: rms=1.35ms, set=4.11ms, show=1.41ms --> total=6.87 | Sampling Freq (kHz) : 7.47
// avg. loop: rms=1.35ms, set=4.06ms, show=1.45ms --> total=6.87 | Sampling Freq (kHz) : 7.46
// avg. loop: rms=1.35ms, set=0.07ms, show=1.42ms --> total=2.85 | Sampling Freq (kHz) : 7.47
// avg. loop: rms=1.35ms, set=2.33ms, show=1.42ms --> total=5.09 | Sampling Freq (kHz) : 7.47
