# ATmega_VuMeter

VuMeter based on a TrinketPro 3V (ATmega328P)

## Intro

## Design notes for Trinket Pro 3V

### Relevant Specs

  - CPU Frequency: 12MHz
  - RAM: 2048 bytes
  - Memory footprint of FASTLed Demo with 60 LEDS
    RAM:   [==        ]  17.6% (used 361 bytes from 2048 bytes)
    Flash: [==        ]  20.5% (used 5864 bytes from 28672 bytes)

### Sound Envelope 

#### Sampling and memory requirement

For this application, there is no need to sample higher than ~8KHz.

In addition, we would like to capture sound amplitude for fundamental frequencies from ~100Hz (male)
to ~300Hz (child), including ~200Hz (female).

If we set the ADC pre-scaler to 128 on the 12MHz Trinket, knowling 13 cycles are required for
sampling, we get a sampling frequency of:

    fs = 12E6 / 128 / 13 ~= 7.2KHz (7211Hz)

Also, the clock for the ADC with this pre-scaler would be 93.75KHz, allowing potentially full 
10 bits resolution acquisition.

Collecting 144 samples for the filtering would fully cover the fundamental frequencies desired:

    7211 / 144 = 50Hz

Collecting 10bits reading (coded on a 16 bit integer), with a double buffer, would require:

    144 * 2 * 2 = 576 bytes

Even double this size would still fit on our 2K RAM minus the LED library! 

#### Implementation

Implement a double buffer of 288 x 10 bits samples.

## FastLED and audio interactions

The [FastLED doc](https://github.com/FastLED/FastLED/wiki/Interrupt-problems)
indicates that it takes 30us to update one WS2812 pixel , and that FastLED
disable interrupts duing the data transmission. With 60 LEDs, this amounts to
~1.8ms, which seems to be confirmed by the timing measurements (~1.4ms, not sure
how it could be shorter unless micros is not accurate enough).

If the FastLED.show() was totatlly disabling interrupt during the full update,
it means that we could loose ~13 samples for each show (every 40ms = 25Hz).

This is probably why the measured sampling frequency is 7.69kHz without FastLED and only 7.47kHz
for the full application.
Naive math: 7.69 * (288-13)/288 = 7.34

Updating the strip by segments is not really an option since this would simply introduce a lag.
There is likely no good solution beside upgrading the LED strip and/or micro-processor.

Note: visually, this not detrimental. This is equivalent to loose sampling for <2ms every 40ms.

## References

[1] Trinket User Guide: https://learn.adafruit.com/introducing-pro-trinket/overview

[2] ATmega328P Datasheet: https://ww1.microchip.com/downloads/en/DeviceDoc/ATmega48A-PA-88A-PA-168A-PA-328-P-DS-DS40002061B.pdf

[3] Human voice frequency range: https://seaindia.in/sea-online-journal/human-voice-frequency-range/

[4] Telephony sampling frequency: http://www.dspguide.com/ch22/3.htm
