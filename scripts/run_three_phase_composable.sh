#!/bin/bash

# sudo python3 scripts/accel_conf.py --load=configs/iaa-1n1d8e1w128q-s-n2.conf

source configs/phys_core.sh
source configs/devid.sh

query_size=$((42 * 1024))

make -j CXXFLAGS="-DPERF"
mkdir -p three_phase_composable_logs

taskset -c 1 sudo LD_LIBRARY_PATH=/opt/intel/oneapi/ippcp/2021.11/lib \
  ./three_phase_composable \
  -t 1000 -i 10 \
  -s ${query_size} \
  -m $DSA_DEV_ID -n $IAA_DEV_ID \
  > three_phase_composable_logs/querysize_${query_size}.log