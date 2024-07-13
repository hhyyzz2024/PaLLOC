#!/bin/bash

source ./benchmark.conf

set -e

core=$1
shift
workload_queue=$@

cd ${PARSEC_DIR}
source env.sh

for workload in ${workload_queue[@]}
do
    cd ${bench_dir[${workload}]}
    eval taskset -c ${core} ${bench_cmd[${workload}]} &
    wait
done