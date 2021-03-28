#!/bin/bash

gcc -c -pthread -lm program.c

gcc  -O3 -pthread  -o producerConsumer program.o -lm