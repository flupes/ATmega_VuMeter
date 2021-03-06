#include <Arduino.h>

void setup()
{
  Serial.begin(115200);
}

void loop()
{
  uint16_t adc = analogRead(2);
  Serial.println(adc);
  delay(1);
}
