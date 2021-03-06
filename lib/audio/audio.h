#ifndef SOUNDLEVEL_AUDIO_H
#define SOUNDLEVEL_AUDIO_H

#include <Arduino.h>
#include <stdint.h>

// The ATmega238P conversion takes 13 clock cycles
// The Trinket Pro 3.3V runs at 12MHz
// with prescaler = 128 --> sampling rate = 12E6 / 128 / 13 ~= 7.2 kHz
const uint32_t kAdcClockPrescaler = 128;
const uint32_t kAdcAcquisitionCycles = 13;
const uint32_t kSampleFrequency =
    F_CPU / kAdcClockPrescaler / kAdcAcquisitionCycles;

// Collection of sound envelope over 40ms (25Hz)
const uint16_t kMeasurementPeriodMs = 40;

// Rounded samples required for one measurement
// (watch order the order of operations to make sure there is no rounding error
// (32 -> 16 bits). With the settings above, it should result to 288 samples.
const uint16_t kSamplesPerMeasurement =
    kSampleFrequency * kMeasurementPeriodMs / 1000;

extern uint16_t gSamples[2][kSamplesPerMeasurement];
extern volatile uint16_t gMeasurementsCount;
extern volatile uint8_t gCurrentBuffer;

uint8_t* const kSamplesBuffer1{reinterpret_cast<uint8_t*>(gSamples[0])};
uint8_t* const kSamplesBuffer2{
    reinterpret_cast<uint8_t*>(gSamples[0] + kSamplesPerMeasurement)};
uint8_t* const kSamplesEnd{
    reinterpret_cast<uint8_t*>(gSamples[0] + 2 * kSamplesPerMeasurement)};
uint16_t* const kBufferHead[2] = {gSamples[0], gSamples[1]};
uint16_t* const kBufferEnd[2] = {gSamples[1],
                                 gSamples[0] + 2 * kSamplesPerMeasurement};

void configure_audio(uint8_t adcChannel, uint8_t clockPrescaler = kAdcClockPrescaler);

float compute_rms(const uint8_t buffer);

#endif
