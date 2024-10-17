#!/bin/env bash

mkdir -p build

CF='-O0 -g'
LF=-lm

clang $CF -o build/editor code/editor.c $LF
