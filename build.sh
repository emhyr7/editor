#!/bin/env bash

CF='-O0 -g'
LF=-lm

clang $CF -o editor code/editor.c $LF