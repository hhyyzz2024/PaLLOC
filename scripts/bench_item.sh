#!/bin/bash

#read -s -p "Enter Password for sudo: " sudoPW
set -e

result_dir=$1
workload_define_file=$2
mix_num=$3
period=$4
interval=$5
sudoPW=$6
source $workload_define_file

current_dir=$(pwd)

core_num="${#cores[@]}"

PROJ_DIR=${current_dir}/..

IPC_RESULTS_DIR=${PROJ_DIR}/results/${result_dir}/${core_num}/ipc/${conf_name}
OVERHEAD_RESULTS_DIR=${PROJ_DIR}/results/${result_dir}/${core_num}/overhead/${conf_name}

process_ids=()

if [[ ! -d ${IPC_RESULTS_DIR} ]];then
    mkdir -p ${IPC_RESULTS_DIR}
fi

if [[ ! -d ${OVERHEAD_RESULTS_DIR} ]];then
    mkdir -p ${OVERHEAD_RESULTS_DIR}
fi

#cache_conf=0x1
#clos_id=1
#mb_throttle=20
#echo $sudoPW | sudo -S pqos -R
# configuration cache and mb
#for core in ${cores[@]}
#do
#    echo $sudoPW | sudo -S pqos -a "llc:${clos_id}=${core}"
#    echo $sudoPW | sudo -S pqos -e "llc:${clos_id}=${cache_conf}"
#    echo $sudoPW | sudo -S pqos -e "mba:${clos_id}=${mb_throttle}"
#    ((clos_id++))
#done

# workload mix0
if [[ $mix_num -eq 0 ]];then
    mix_name="workload_mix0"
    for core in ${cores[@]}
    do
        workload_queue=${workload_mix0[$core]}
        cd ${PROJ_DIR}/scripts
        time ./core_exec.sh $core ${workload_queue[@]} &
        process_ids+=("$!")
    done

    for core in ${background_cores[@]}
    do
        cd ${PROJ_DIR}/scripts
        time ./core_exec.sh $core ${background_workload_queue} &
    done

    cd ${PROJ_DIR}
    echo $sudoPW | sudo -S ./PaLLOC -P $period -i $interval -m 0 -c $cores_arg -a 1
    mv experimenta_ipc_data.csv ${IPC_RESULTS_DIR}/${mix_name}_experimenta_ipc_data.csv
    mv experimenta_ipc_data.data ${IPC_RESULTS_DIR}/${mix_name}_experimenta_ipc_data.data

    mv experimenta_overhead_data.csv ${OVERHEAD_RESULTS_DIR}/${mix_name}_experimenta_overhead_data.csv
    mv experimenta_overhead_data.data ${OVERHEAD_RESULTS_DIR}/${mix_name}_experimenta_overhead_data.data
    echo "Move data success!"

    kill -- -$$
    wait

elif [[ $mix_num -eq 1 ]];then
    # workload mix1
    mix_name="workload_mix1"
    for core in ${cores[@]}
    do
        workload_queue=${workload_mix1[$core]}
        cd ${PROJ_DIR}/scripts
        time ./core_exec.sh $core ${workload_queue[@]} &
    done
    cd ${PROJ_DIR}
    echo $sudoPW | sudo -S ./PaLLOC -P $period -i $interval -m 0 -c $cores_arg -a 1
    mv experimenta_ipc_data.csv ${IPC_RESULTS_DIR}/${mix_name}_experimenta_ipc_data.csv
    mv experimenta_ipc_data.data ${IPC_RESULTS_DIR}/${mix_name}_experimenta_ipc_data.data

    mv experimenta_overhead_data.csv ${OVERHEAD_RESULTS_DIR}/${mix_name}_experimenta_overhead_data.csv
    mv experimenta_overhead_data.data ${OVERHEAD_RESULTS_DIR}/${mix_name}_experimenta_overhead_data.data
    echo "Move data success!"

    kill -- -$$
#    kill -TERM -$$
    wait

elif [[ $mix_num -eq 2 ]];then
# workload mix2
    mix_name="workload_mix2"
    for core in ${cores[@]}
    do
        workload_queue=${workload_mix2[$core]}
        cd ${PROJ_DIR}/scripts
        time ./core_exec.sh $core ${workload_queue[@]} &
    done
    cd ${PROJ_DIR}
    echo $sudoPW | sudo -S ./PaLLOC -P $period -i $interval -m 0 -c $cores_arg -a 1
    mv experimenta_ipc_data.csv ${IPC_RESULTS_DIR}/${mix_name}_experimenta_ipc_data.csv
    mv experimenta_ipc_data.data ${IPC_RESULTS_DIR}/${mix_name}_experimenta_ipc_data.data

    mv experimenta_overhead_data.csv ${OVERHEAD_RESULTS_DIR}/${mix_name}_experimenta_overhead_data.csv
    mv experimenta_overhead_data.data ${OVERHEAD_RESULTS_DIR}/${mix_name}_experimenta_overhead_data.data
    echo "Move data success!"

    kill -- -$$
#    kill -TERM -$$
    wait

elif [[ $mix_num -eq 3 ]];then
    # workload mix3
    mix_name="workload_mix3"
    for core in ${cores[@]}
    do
        workload_queue=${workload_mix3[$core]}
        cd ${PROJ_DIR}/scripts
        time ./core_exec.sh $core ${workload_queue[@]} &
    done
    cd ${PROJ_DIR}
    echo $sudoPW | sudo -S ./PaLLOC -P $period -i $interval -m 0 -c $cores_arg -a 1
    mv experimenta_ipc_data.csv ${IPC_RESULTS_DIR}/${mix_name}_experimenta_ipc_data.csv
    mv experimenta_ipc_data.data ${IPC_RESULTS_DIR}/${mix_name}_experimenta_ipc_data.data

    mv experimenta_overhead_data.csv ${OVERHEAD_RESULTS_DIR}/${mix_name}_experimenta_overhead_data.csv
    mv experimenta_overhead_data.data ${OVERHEAD_RESULTS_DIR}/${mix_name}_experimenta_overhead_data.data
    echo "Move data success!"

    kill -- -$$
#    kill -TERM -$$
    wait

elif [[ $mix_num -eq 4 ]];then
    # workload mix4
    mix_name="workload_mix4"
    for core in ${cores[@]}
    do
        workload_queue=${workload_mix4[$core]}
        cd ${PROJ_DIR}/scripts
        time ./core_exec.sh $core ${workload_queue[@]} &
    done
    cd ${PROJ_DIR}
    echo $sudoPW | sudo -S ./PaLLOC -P $period -i $interval -m 0 -c $cores_arg -a 1
    mv experimenta_ipc_data.csv ${IPC_RESULTS_DIR}/${mix_name}_experimenta_ipc_data.csv
    mv experimenta_ipc_data.data ${IPC_RESULTS_DIR}/${mix_name}_experimenta_ipc_data.data

    mv experimenta_overhead_data.csv ${OVERHEAD_RESULTS_DIR}/${mix_name}_experimenta_overhead_data.csv
    mv experimenta_overhead_data.data ${OVERHEAD_RESULTS_DIR}/${mix_name}_experimenta_overhead_data.data
    echo "Move data success!"

    kill -- -$$
#    kill -TERM -$$
    wait

elif [[ $mix_num -eq 5 ]];then
    # workload mix5
    mix_name="workload_mix5"
    for core in ${cores[@]}
    do
        workload_queue=${workload_mix5[$core]}
        cd ${PROJ_DIR}/scripts
        time ./core_exec.sh $core ${workload_queue[@]} &
    done
    cd ${PROJ_DIR}
    echo $sudoPW | sudo -S ./PaLLOC -P $period -i $interval -m 0 -c $cores_arg -a 1
    mv experimenta_ipc_data.csv ${IPC_RESULTS_DIR}/${mix_name}_experimenta_ipc_data.csv
    mv experimenta_ipc_data.data ${IPC_RESULTS_DIR}/${mix_name}_experimenta_ipc_data.data

    mv experimenta_overhead_data.csv ${OVERHEAD_RESULTS_DIR}/${mix_name}_experimenta_overhead_data.csv
    mv experimenta_overhead_data.data ${OVERHEAD_RESULTS_DIR}/${mix_name}_experimenta_overhead_data.data
    echo "Move data success!"

    kill -- -$$
#    kill -TERM -$$
    wait

elif [[ $mix_num -eq 6 ]];then
    # workload mix6
    mix_name="workload_mix6"
    for core in ${cores[@]}
    do
        workload_queue=${workload_mix6[$core]}
        cd ${PROJ_DIR}/scripts
        time ./core_exec.sh $core ${workload_queue[@]} &
    done
    cd ${PROJ_DIR}
    echo $sudoPW | sudo -S ./PaLLOC -P $period -i $interval -m 0 -c $cores_arg -a 1
    mv experimenta_ipc_data.csv ${IPC_RESULTS_DIR}/${mix_name}_experimenta_ipc_data.csv
    mv experimenta_ipc_data.data ${IPC_RESULTS_DIR}/${mix_name}_experimenta_ipc_data.data

    mv experimenta_overhead_data.csv ${OVERHEAD_RESULTS_DIR}/${mix_name}_experimenta_overhead_data.csv
    mv experimenta_overhead_data.data ${OVERHEAD_RESULTS_DIR}/${mix_name}_experimenta_overhead_data.data
    echo "Move data success!"

    kill -- -$$
    wait

elif [[ $mix_num -eq 7 ]];then
    # workload mix7
    mix_name="workload_mix7"
    for core in ${cores[@]}
    do
        workload_queue=${workload_mix7[$core]}
        cd ${PROJ_DIR}/scripts
        time ./core_exec.sh $core ${workload_queue[@]} &
    done
    cd ${PROJ_DIR}
    echo $sudoPW | sudo -S ./PaLLOC -P $period -i $interval -m 0 -c $cores_arg -a 1
    mv experimenta_ipc_data.csv ${IPC_RESULTS_DIR}/${mix_name}_experimenta_ipc_data.csv
    mv experimenta_ipc_data.data ${IPC_RESULTS_DIR}/${mix_name}_experimenta_ipc_data.data

    mv experimenta_overhead_data.csv ${OVERHEAD_RESULTS_DIR}/${mix_name}_experimenta_overhead_data.csv
    mv experimenta_overhead_data.data ${OVERHEAD_RESULTS_DIR}/${mix_name}_experimenta_overhead_data.data
    echo "Move data success!"

    kill -- -$$
#    kill -TERM -$$
    wait

elif [[ $mix_num -eq 8 ]];then
    # workload mix8
    mix_name="workload_mix8"
    for core in ${cores[@]}
    do
        workload_queue=${workload_mix8[$core]}
        cd ${PROJ_DIR}/scripts
        time ./core_exec.sh $core ${workload_queue[@]} &
    done
    cd ${PROJ_DIR}
    echo $sudoPW | sudo -S ./PaLLOC -P $period -i $interval -m 0 -c $cores_arg -a 1
    mv experimenta_ipc_data.csv ${IPC_RESULTS_DIR}/${mix_name}_experimenta_ipc_data.csv
    mv experimenta_ipc_data.data ${IPC_RESULTS_DIR}/${mix_name}_experimenta_ipc_data.data

    mv experimenta_overhead_data.csv ${OVERHEAD_RESULTS_DIR}/${mix_name}_experimenta_overhead_data.csv
    mv experimenta_overhead_data.data ${OVERHEAD_RESULTS_DIR}/${mix_name}_experimenta_overhead_data.data
    echo "Move data success!"

    kill -- -$$
#    kill -TERM -$$
    wait

else
    # workload mix9
    mix_name="workload_mix9"
    for core in ${cores[@]}
    do
        workload_queue=${workload_mix9[$core]}
        cd ${PROJ_DIR}/scripts
        time ./core_exec.sh $core ${workload_queue[@]} &
        process_ids+=("$!")
    done
    cd ${PROJ_DIR}
    echo $sudoPW | sudo -S ./PaLLOC -P $period -i $interval -m 0 -c $cores_arg -a 1
    mv experimenta_ipc_data.csv ${IPC_RESULTS_DIR}/${mix_name}_experimenta_ipc_data.csv
    mv experimenta_ipc_data.data ${IPC_RESULTS_DIR}/${mix_name}_experimenta_ipc_data.data

    mv experimenta_overhead_data.csv ${OVERHEAD_RESULTS_DIR}/${mix_name}_experimenta_overhead_data.csv
    mv experimenta_overhead_data.data ${OVERHEAD_RESULTS_DIR}/${mix_name}_experimenta_overhead_data.data
    echo "Move data success!"

    kill -- -$$
#    kill -TERM -$$
    wait
fi
