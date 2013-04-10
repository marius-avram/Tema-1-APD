#!/bin/bash

# Script de rulare pentru versiunea paralela a programului
threads=$1 
iteratii=$2
fisin=$3
fisout=$4
# Se va modifica tipul de schedule aici dupa nevoie
export OMP_SCHEDULE="dynamic"
export OMP_NUM_THREADS=$threads

time ./paralel $iteratii $fisin $fisout
