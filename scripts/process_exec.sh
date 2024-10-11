#!/bin/bash

source ./benchmark_multithreads.conf

set -e

cores="0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34,36,38,40,42,44,46"
workload=$1
num_threads=$2

echo -e "\033[31m${workload} ${num_threads}\033[0m"

cd ${SPEC2017_DIR}
source shrc

cd ${PARSEC_DIR}
source env.sh

ulimit -s unlimited

if [ "$workload" = "628.pop2_s" ];then
    if [[ ${num_threads} -eq 4 ]];then
        cd ${bench_dir[${workload}]}
    elif [[ ${num_threads} -eq 2 ]];then
        cd "${SPEC2017_DIR}/benchspec/CPU/628.pop2_s/run/run_base_refspeed_mytest-m64.0002"
    elif [[ ${num_threads} -eq 3 ]];then
        cd "${SPEC2017_DIR}/benchspec/CPU/628.pop2_s/run/run_base_refspeed_mytest-m64.0003"
    elif [[ ${num_threads} -eq 1 ]];then
        cd "${SPEC2017_DIR}/benchspec/CPU/628.pop2_s/run/run_base_refspeed_mytest-m64.0004"
    fi
elif [ "$workload" = "654.roms_s" ];then
    if [[ ${num_threads} -eq 1 ]] || [[ ${num_threads} -eq 3 ]]; then
        cd "${SPEC2017_DIR}/benchspec/CPU/654.roms_s/run/run_base_refspeed_mytest-m64.0001"
    else
        cd "${SPEC2017_DIR}/benchspec/CPU/654.roms_s/run/run_base_refspeed_mytest-m64.0000"
    fi
else
    cd ${bench_dir[${workload}]}
fi
eval OMP_NUM_THREADS=${num_threads} OMP_STACKSIZE=120M taskset -c ${cores} ${bench_cmd[${workload}]} &
wait