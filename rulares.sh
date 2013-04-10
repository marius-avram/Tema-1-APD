#!/bin/bash

# Script de rulare pentru versiunea seriala a programului
iteratii=$1
fisin=$2
fisout=$3

time ./serial $iteratii $fisin $fisout
