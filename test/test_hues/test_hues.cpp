#include <Arduino.h>
#include <FastLED.h>

#define DATA_PIN 6
#define LED_TYPE WS2812
#define COLOR_ORDER GRB

const size_t kNumberOfLeds = 60;
CRGB gLeds[kNumberOfLeds];

void setup() {
  Serial.begin(115200);

  delay(3000);  // 3 second delay for recovery

  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(gLeds, kNumberOfLeds)
      .setCorrection(TypicalLEDStrip);

  for (size_t i = 0; i < kNumberOfLeds; i++) {
    uint8_t hue = map(i * 4, 0, 240, 96, -16);
    Serial.print(i * 4);
    Serial.print(" --> ");
    Serial.println(hue);
    gLeds[i].setHue(hue);
  }
  Serial.print("RED=");
  Serial.println(HUE_RED);
  Serial.print("GREEN=");
  Serial.println(HUE_GREEN);
  Serial.print("BLUE=");
  Serial.println(HUE_BLUE);
}

void loop() {
  FastLED.show();
  FastLED.delay(1000 / 120);
}
