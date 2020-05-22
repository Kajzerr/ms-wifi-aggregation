#!/bin/sh

for payload in 100 200 400 600 800 1000 1200 1472
do
  for rng in 1 2 3 4
  do
      ./waf --run "scratch/wifi-aggregation --RngRun=$rng --payload=$payload --enableRts=0 --outputCsv=payload_woRTS.csv"
  done
done