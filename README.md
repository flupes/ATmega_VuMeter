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

Implement a double buffer of 244 x 10 bits samples.


## References

[1] Trinket User Guide: https://learn.adafruit.com/introducing-pro-trinket/overview

[2] ATmega328P Datasheet: https://ww1.microchip.com/downloads/en/DeviceDoc/ATmega48A-PA-88A-PA-168A-PA-328-P-DS-DS40002061B.pdf

[3] Human voice frequency range: https://seaindia.in/sea-online-journal/human-voice-frequency-range/

[4] Telephony sampling frequency: http://www.dspguide.com/ch22/3.htm
