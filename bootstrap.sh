#!/bin/bash

g++ -std=c++23 -Wall -Wextra -O2 src/scl.cc -c -o scl.o
ar rcs libscl.a scl.o

sudo cp include/scl.hh /usr/local/include/
sudo cp libscl.a /usr/local/lib/

