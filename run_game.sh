#!/usr/bin/env bash

set -e

cmake .
make
./halite --replay-directory replays/ -vvv --width 32 --height 32 --results-as-json './MyBot' 'python3 ../Halite3_Python3_MacOS/MyBot.py'
