#include <Arduino.h>

void setup() {
  Serial.begin(115200);

  uint8_t table[] = { 2, 4, 8, 16, 32, 64, 128, 3, 129};
  uint8_t scalor, clz, cnt;
  for (size_t i=0; i<sizeof(table); i++) {
    scalor = table[i];
    Serial.print(scalor);
    clz = __builtin_clz(scalor);
    Serial.print(" : clz=");
    Serial.print(clz);
    cnt = __builtin_popcount(scalor);
    Serial.print(" cnt=");
    Serial.print(cnt);
    Serial.println();
  }
}

void loop()
{
  uint16_t seed = analogRead(2);
  float sum = 0.0;
  Serial.print("logn (us per cyle): ");
  uint32_t t0 = micros();
  for (size_t i=0; i<1000; i++) {
    sum += logf((i+1)*seed);
  }
  Serial.print((micros()-t0)/1000);
  Serial.print(" | sum=");
  Serial.println(sum);

  sum = 0.0;
  Serial.print("log10 (us per cyle): ");
  t0 = micros();
  for (size_t i=0; i<1000; i++) {
    sum += log10f((i+1)*seed);
  }
  Serial.print((micros()-t0)/1000);
  Serial.print(" | sum=");
  Serial.println(sum);

  delay(2000);
}
