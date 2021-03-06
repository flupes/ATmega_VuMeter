#include "audio.h"

#define USE_SERIAL 1

#ifdef USE_SERIAL
#include <Arduino.h>
#endif

uint16_t gSamples[2][kSamplesPerMeasurement];
uint16_t volatile gMeasurementsCount = 0;
uint8_t volatile gCurrentBuffer = 0;

void audio_error(uint8_t code) {
// Not sure what to do if we do not have serial line, so just stop everything
#ifdef USE_SERIAL
  Serial.print("Audio Error: ");
  Serial.println(code);
#endif
  while (1)
    ;
}

void configure_audio(uint8_t adcChannel, uint8_t clockPrescaler) {
  cli();  // Disable interupts

  ADCSRA = 0;  // clear ADCSRA register
  ADCSRB = 0;  // clear ADCSRB register

  if (adcChannel > 7) {
    audio_error(1);
  }
  ADMUX = adcChannel & 0x07;  // set analog input pin
  ADMUX |= (1 << REFS0);      // set reference voltage

  uint32_t clock = F_CPU / clockPrescaler;
  if (clock > 200000) {
    // Future work: use 8 bits only for high sampling frequencies
    ADMUX |= (1 << ADLAR);  // left align ADC value to 8 bits from ADCH register
    audio_error(2);
  }

  switch (clockPrescaler) {
    case 2:
      ADCSRA |= (1 << ADPS0);
      break;
    case 4:
      ADCSRA |= (1 << ADPS1);
      break;
    case 8:
      ADCSRA |= (1 << ADPS1) | (1 << ADPS0);
      break;
    case 16:
      ADCSRA |= (1 << ADPS2);
      break;
    case 32:
      ADCSRA |= (1 << ADPS2) | (1 << ADPS0);
      break;
    case 64:
      ADCSRA |= (1 << ADPS2) | (1 << ADPS1);
      break;
    case 128:
      ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
      break;
    default:
      audio_error(3);
  }

  ADCSRA |= (1 << ADATE);  // enable auto trigger
  ADCSRA |= (1 << ADIE);   // enable interrupts when measurement complete
  ADCSRA |= (1 << ADEN);   // enable ADC
  ADCSRA |= (1 << ADSC);   // start ADC measurements

  sei();  // Enable interrupts
}

float compute_rms(const uint8_t buffer) {
  uint16_t *ptr = kBufferHead[buffer];
  uint32_t sqsum = 0;
  while (ptr < kBufferEnd[buffer]) {
    int16_t val = *(ptr++) - 512;
    sqsum += val * val;
  }

  return sqrt((float)sqsum / (float)kSamplesPerMeasurement);
}

ISR(ADC_vect) {
  static uint8_t *byteSlot = kSamplesBuffer1;
  *(byteSlot++) = ADCL;
  *(byteSlot++) = ADCH;
  
  if (byteSlot < kSamplesBuffer2) {
    gCurrentBuffer = 0;
  } else {
    gCurrentBuffer = 1;
  }
  if (byteSlot >= kSamplesEnd) {
    byteSlot = kSamplesBuffer1;
  }
  gMeasurementsCount++;
}

/*
avr-g++ -E -c -fno-exceptions -fno-threadsafe-statics -fpermissive -std=gnu++11
-Os -Wall -ffunction-sections -fdata-sections -flto -mmcu=atmega328p
-DPLATFORMIO=50100 -DARDUINO_AVR_PROTRINKET3FTDI -DF_CPU=12000000L
-DARDUINO_ARCH_AVR -DARDUINO=10808 -DUNIT_TEST -DUNITY_INCLUDE_CONFIG_H
-Ilib/audio
-I/home/itdad/.platformio/packages/framework-arduino-avr/cores/arduino
-I/home/itdad/.platformio/packages/framework-arduino-avr/variants/eightanaloginputs
-I.pio/build/pt3/UnityTestLib -I/home/itdad/.platformio/packages/tool-unity
lib/audio/audio.cpp > out.cpp

extern "C" void __vector_21 (void) __attribute__ ((signal,used,
externally_visible)) ; void __vector_21 (void) { ... }
*/
