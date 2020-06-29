# Abysse

## DMX512 multiple low frequency pseudo-random signal generators.

A single [Fraise](https://github.com/MetaluNet/Fraise) Versa2.0 board, equipped with a DMX512 driver, generates DMX512 frames to a multi-channels dimmer and 12VDC multiple motors driver.

The Pd patch `0Abysse.pd` allows to configure the 12 generators, for 6 lamps and 6 motors.

A low-freq random generator here is a random value with a 2nd order low-pass filter, which cutoff frequency is configurable.

Each lamps signal is the sum of 2 independent generators, each multiplied by a separate amplitude parameter.

For the motors, only one generator is used, and all motors share a common amplitude parameter and filter frequency setting.

The USB-DMX512 converter is a set of [Fraise](https://github.com/MetaluNet/Fraise) boards (one Pied and one Versa2.0 with a DMX512 driver).

(c) GPL 2019 metalu.net
