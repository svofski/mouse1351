#!/bin/sh
echo $[`cat buildnum` + 1]> buildnum
cat buildnum
