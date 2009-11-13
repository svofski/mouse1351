#!/bin/sh
kill -HUP `ps ax | grep -e "[0-9] screen.*usbserial" | cut -f 1 -d ' '`
./mega8_bootload mouse.bin /dev/tty.usbserial-A6007DJZ 19200
