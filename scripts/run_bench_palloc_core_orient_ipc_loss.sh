#!/bin/bash

#workload_define_files=(workload_define0.conf workload_define1.conf workload_define2.conf workload_define3.conf workload_define4.conf
#        workload_define5.conf workload_define6.conf workload_define7.conf workload_define8.conf workload_define9.conf
#        workload_define10.conf workload_define11.conf workload_define12.conf workload_define13.conf workload_define14.conf
#        workload_define15.conf workload_define16.conf workload_define17.conf workload_define18.conf workload_define19.conf
#        workload_define20.conf workload_define21.conf workload_define22.conf workload_define23.conf workload_define24.conf)

#workload_define_files=(workload_define1.conf)

set -e

ROOT_DIR=/home/huangyizhi/workspace/PaLLOC

mixs=(0)
workload_define_files=(workload_define24.conf)
bench_name=bench_item.sh
result_dir="palloc_ipc_loss"
strategy=6
period=100

if [ $# -eq 1 ];
then
	bench_name=$1
fi

read -s -p "Enter Password for sudo: " sudoPW

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

for workload_define_file in ${workload_define_files[@]}
do
    for mix in ${mixs[@]}
    do
        echo ${workload_define_file}
        cd ${ROOT_DIR}/scripts
        setsid ./${bench_name} ${result_dir} ${workload_define_file} ${mix} ${period} ${strategy} ${sudoPW} &
        pid=$(pgrep -f ${bench_name})
	    tail --pid=${pid} -f /dev/null
    done
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
