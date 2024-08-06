#!/bin/bash

# sudo python3 scripts/accel_conf.py --load=configs/iaa-1n1d8e1w128q-s-n2.conf

source configs/phys_core.sh
source configs/devid.sh
source configs/query_size.sh

[ -z "$CORE" ] && echo "CORE is not set" && exit 1
[ -z "$query_size" ] && echo "query_size is not set" && exit 1

iters=5
reqs=10

make -j CXXFLAGS="-DPERF"
mkdir -p three_phase_composable_logs

taskset -c $CORE sudo LD_LIBRARY_PATH=/opt/intel/oneapi/ippcp/2021.11/lib \
  ./three_phase_composable \
  -t $reqs -i $iters \
  -s ${query_size} \
  -m $DSA_DEV_ID -n $IAA_DEV_ID \
  -k 3 \
  > three_phase_composable_logs/axcore_inp_querysize_${query_size}.log