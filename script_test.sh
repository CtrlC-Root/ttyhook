#!/bin/bash

# TTYHOOK_SCRIPT=./script_test.sh LD_PRELOAD=./ttyhook.so ttyhook_test /dev/ttyUSB0 rts

TIMESTAMP=$(date --utc --iso-8601='seconds')
SIGNAL=$1; shift 1;

printf "[${TIMESTAMP}]: ${SIGNAL}\n"
