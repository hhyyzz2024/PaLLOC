#!/bin/bash

set -e

PaLLOC_DIR=/home/huangyizhi/workspace/PaLLOC

mixs=(0)
workload_define_file=workload_define12.conf
bench_name=bench_item_core.sh
result_dir="palloc_cpu_usage"
period=100

sudoPW=$1

# disable turbo boost
for core in ${cores[@]}
do
    echo $sudoPW | sudo -S wrmsr -p${core} 0x1a0 0x4000850089
    state=$(sudo rdmsr -p${core} 0x1a0 -f 38:38)
    if [[ $state -eq 1 ]]; then
        echo "core ${core} disabled success!"
    else
        echo "core ${core} disabled fail!"
        exit 1
    fi
done

for mix in ${mixs[@]}
do
    cd ${PaLLOC_DIR}/scripts
    setsid ./${bench_name} ${result_dir} ${workload_define_file} ${mix} ${period} 1 ${sudoPW} &
    pid=$(pgrep -f ${bench_name})
    tail --pid=${pid} -f /dev/null
done

# enable turbo boost
for core in ${cores[@]}
do
    echo $sudoPW | sudo -S wrmsr -p${core} 0x1a0 0x850089
    state=$(sudo rdmsr -p${core} 0x1a0 -f 38:38)
    if [[ $state -eq 1 ]]; then
        echo "core ${core} enabled failed!"
    else
        echo "core ${core} enabled success!"
    fi
done