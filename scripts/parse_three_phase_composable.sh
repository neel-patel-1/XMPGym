#!/bin/bash

source configs/phys_core.sh

[ -z "$CORE" ] && echo "CORE is not set" && exit 1

query_size=$((42 * 1024))

grep -v main three_phase_composable_logs/querysize_${query_size}.log \
  | grep -v info \
  | grep -v RPS \
  | awk "\
    /PreProcFunc Mean/{printf(\"%s \", \$5 );} \
    /OffloadTax Mean/{printf(\"%s \", \$5);  } \
    /AxFunc Mean/{printf(\"%s \", \$5);} \
    /PostProcFunc Mean/{printf(\"%s\n\", \$5);} \
    "

# [ info] alloc wq 0 shared size 128 addr 0x7f46d3d28000 batch sz 0x400 xfer sz 0x80000000
# PreProcFunc Mean: 2559 Median: 2375 Stddev: 593.278181
# OffloadTax Mean: 63 Median: 63 Stddev: 2.236068
# AxFunc Mean: 51514 Median: 51715 Stddev: 447.644949
# PostProcFunc Mean: 50803 Median: 49251 Stddev: 3118.376501
# RPS Mean: 20426.598419 Median: 20424.425793 Stddev: 9857526.115615
# PreProcFunc Mean: 2255 Median: 2248 Stddev: 37.523326
# OffloadTax Mean: 616 Median: 616 Stddev: 2.645751
# AxFunc Mean: 14044 Median: 14026 Stddev: 49.295030
# PostProcFunc Mean: 49511 Median: 49516 Stddev: 53.469618
# RPS Mean: 31324.615047 Median: 31432.821146 Stddev: 2641916.884308
# PreProcFunc Mean: 2313 Median: 2311 Stddev: 39.408121
# OffloadTax Mean: 571 Median: 571 Stddev: 2.449490
# AxFunc Mean: 122604 Median: 122587 Stddev: 1896.719800
# PostProcFunc Mean: 49812 Median: 49838 Stddev: 119.256027
# RPS Mean: 39541.822501 Median: 39537.248513 Stddev: 31120437.404001