#!/bin/bash

if [ ! -d $HOME/.hewg/bootstrap/crow.scl ]; then
  mkdir -p $HOME/.hewg/bootstrap/crow.scl
fi


g++ -I $HOME/.hewg/bootstrap -I./include-std=c++23 -Wall -Wextra -O2 src/scl.cc -c -o scl.o
ar rcs libscl.a scl.o

cp include/scl.hh $HOME/.hewg/bootstrap/crow.scl/
cp libscl.a $HOME/.hewg/bootstrap/

