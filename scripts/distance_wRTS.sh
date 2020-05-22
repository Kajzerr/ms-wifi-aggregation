#!/bin/sh

for distance in 1 2 4 6 8 10 15 20 30
do
  for rng in 1 2 3 4
  do
      ./waf --run "scratch/wifi-aggregation --RngRun=$rng --distance=$distance --outputCsv=distance_wRTS.csv"
  done
done