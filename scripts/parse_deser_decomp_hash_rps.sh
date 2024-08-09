#!/bin/bash

source configs/query_size.sh

for i in ${QUERY_SIZE_LG[@]}; do
    gp_rps=$(grep RPS three_phase_composable_logs/deser_decomp_hash_querysize_$(( 2 ** i )).log | \
      head -n 1 | awk '{print $5}')
    blocking_rps=$(grep RPS three_phase_composable_logs/deser_decomp_hash_querysize_$(( 2 ** i )).log | \
      head -n 2 | tail -n 1 | awk '{print $5}')
    sw_to_fill_rps=$(grep RPS three_phase_composable_logs/deser_decomp_hash_querysize_$(( 2 ** i )).log | \
      tail -n 1 | awk '{print $5}')
    echo $((2 ** i)) $gp_rps $blocking_rps $sw_to_fill_rps
done
