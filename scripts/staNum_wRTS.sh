#!/bin/sh

for staNuM in 1 2 4 6 8 10 15 20 30 40 50 70 90 120
do
  for rng in 1 2 3 4
  do
      ./waf --run "scratch/wifi-aggregation --RngRun=$rng --staNum=$staNuM --outputCsv=staNum_wRTS.csv"
  done
done