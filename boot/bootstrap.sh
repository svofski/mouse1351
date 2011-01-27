#!/bin/bash
# this programs atmega8 for internal RC oscillator @ 8MHz -- calibration may be required
set -x
avrdude -p m8 -c pickit2 -U hfuse:w:0xdc:m -U lfuse:w:0xe4:m
#avrdude -p m8 -c pickit2 -U flash:w:"boot_atmega8.hex":i -v -V -u
avrdude -p m8 -c pickit2 -U flash:w:"mouse+boot.hex":i -v -V -u
