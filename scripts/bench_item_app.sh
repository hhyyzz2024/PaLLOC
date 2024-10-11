#!/bin/bash

#read -s -p "Enter Password for sudo: " sudoPW
set -e

result_dir=$1
app_define_file=$2
mix_num=$3
num_threads=$4
period=$5
sudoPW=$6
source $app_define_file
source benchmark_multithreads.conf

PROJ_DIR=/home/huangyizhi/workspace/PaLLOC

IPC_RESULTS_DIR=${PROJ_DIR}/results/${result_dir}/ipc/${conf_name}
OVERHEAD_RESULTS_DIR=${PROJ_DIR}/results/${result_dir}/overhead/${conf_name}

process_ids=""

if [[ ! -d ${IPC_RESULTS_DIR} ]];then
    mkdir -p ${IPC_RESULTS_DIR}
fi

if [[ ! -d ${OVERHEAD_RESULTS_DIR} ]];then
    mkdir -p ${OVERHEAD_RESULTS_DIR}
fi

if [[ $mix_num -eq 0 ]];then
    for app in ${apps_queue_0[@]}
    do
        cd ${PROJ_DIR}/scripts
        time ./process_exec.sh ${app} ${num_threads} &
        sleep 0.2
    done

    for app in ${apps_queue_0[@]}
    do
        pid_list=()
        while [ ${#pid_list[@]} -eq 0 ]; do
            pid_list=($(pgrep -f ${bench_process_name[${app}]}))
            #echo -e "\033[31m${app} ${pid_list}\033[0m"
        done
        pid=${pid_list[0]}
        if [ -z "$process_ids" ]; then
            process_ids="${pid}"
        else
            process_ids="${process_ids},${pid}"
        fi
    done

    #echo -e "\033[31m${process_ids}\033[0m"

    cd ${PROJ_DIR}
        echo $sudoPW | sudo -S ./PaLLOC -P $period -m 1 -p ${process_ids} -a 1
        mv experimenta_ipc_data.csv ${IPC_RESULTS_DIR}/experimenta_ipc_data_0.csv
        mv experimenta_ipc_data.data ${IPC_RESULTS_DIR}/experimenta_ipc_data_0.data

        mv experimenta_overhead_data.csv ${OVERHEAD_RESULTS_DIR}/experimenta_overhead_data_0.csv
        mv experimenta_overhead_data.data ${OVERHEAD_RESULTS_DIR}/experimenta_overhead_data_0.data
        echo "Move data success!"

    kill -- -$$
    wait

elif [[ $mix_num -eq 1 ]];then

    for app in ${apps_queue_1[@]}
    do
        cd ${PROJ_DIR}/scripts
        time ./process_exec.sh ${app} ${num_threads} &
        sleep 0.2
    done

    for app in ${apps_queue_1[@]}
    do
        pid_list=()
        while [ ${#pid_list[@]} -eq 0 ]; do
            pid_list=($(pgrep -f ${bench_process_name[${app}]}))
        done
        pid=${pid_list[0]}
        if [ -z "$process_ids" ]; then
            process_ids="${pid}"
        else
            process_ids="${process_ids},${pid}"
        fi
    done

    cd ${PROJ_DIR}
        echo $sudoPW | sudo -S ./PaLLOC -P $period -m 1 -p ${process_ids} -a 1
        mv experimenta_ipc_data.csv ${IPC_RESULTS_DIR}/experimenta_ipc_data_1.csv
        mv experimenta_ipc_data.data ${IPC_RESULTS_DIR}/experimenta_ipc_data_1.data

        mv experimenta_overhead_data.csv ${OVERHEAD_RESULTS_DIR}/experimenta_overhead_data_1.csv
        mv experimenta_overhead_data.data ${OVERHEAD_RESULTS_DIR}/experimenta_overhead_data_1.data
        echo "Move data success!"

    kill -- -$$
    wait

elif [[ $mix_num -eq 2 ]];then

    for app in ${apps_queue_2[@]}
    do
        cd ${PROJ_DIR}/scripts
        time ./process_exec.sh ${app} ${num_threads} &
        sleep 0.2
    done

    for app in ${apps_queue_2[@]}
    do
        pid_list=()
        while [ ${#pid_list[@]} -eq 0 ]; do
            pid_list=($(pgrep -f ${bench_process_name[${app}]}))
        done
        pid=${pid_list[0]}
        if [ -z "$process_ids" ]; then
            process_ids="${pid}"
        else
            process_ids="${process_ids},${pid}"
        fi
    done

    cd ${PROJ_DIR}
        echo $sudoPW | sudo -S ./PaLLOC -P $period -m 1 -p ${process_ids} -a 1
        mv experimenta_ipc_data.csv ${IPC_RESULTS_DIR}/experimenta_ipc_data_2.csv
        mv experimenta_ipc_data.data ${IPC_RESULTS_DIR}/experimenta_ipc_data_2.data

        mv experimenta_overhead_data.csv ${OVERHEAD_RESULTS_DIR}/experimenta_overhead_data_2.csv
        mv experimenta_overhead_data.data ${OVERHEAD_RESULTS_DIR}/experimenta_overhead_data_2.data
        echo "Move data success!"

    kill -- -$$
    wait

elif [[ $mix_num -eq 3 ]];then
    for app in ${apps_queue_3[@]}
    do
        cd ${PROJ_DIR}/scripts
        time ./process_exec.sh ${app} ${num_threads} &
        sleep 0.2
    done

    for app in ${apps_queue_3[@]}
    do
        pid_list=()
        while [ ${#pid_list[@]} -eq 0 ]; do
            pid_list=($(pgrep -f ${bench_process_name[${app}]}))
        done
        pid=${pid_list[0]}
        if [ -z "$process_ids" ]; then
            process_ids="${pid}"
        else
            process_ids="${process_ids},${pid}"
        fi
    done

    cd ${PROJ_DIR}
        echo $sudoPW | sudo -S ./PaLLOC -P $period -m 1 -p ${process_ids} -a 1
        mv experimenta_ipc_data.csv ${IPC_RESULTS_DIR}/experimenta_ipc_data_3.csv
        mv experimenta_ipc_data.data ${IPC_RESULTS_DIR}/experimenta_ipc_data_3.data

        mv experimenta_overhead_data.csv ${OVERHEAD_RESULTS_DIR}/experimenta_overhead_data_3.csv
        mv experimenta_overhead_data.data ${OVERHEAD_RESULTS_DIR}/experimenta_overhead_data_3.data
        echo "Move data success!"

    kill -- -$$
    wait

elif [[ $mix_num -eq 4 ]];then

    for app in ${apps_queue_4[@]}
    do
        cd ${PROJ_DIR}/scripts
        time ./process_exec.sh ${app} ${num_threads} &
        sleep 0.2
    done

    for app in ${apps_queue_4[@]}
    do
        pid_list=()
        while [ ${#pid_list[@]} -eq 0 ]; do
            pid_list=($(pgrep -f ${bench_process_name[${app}]}))
        done
        pid=${pid_list[0]}
        if [ -z "$process_ids" ]; then
            process_ids="${pid}"
        else
            process_ids="${process_ids},${pid}"
        fi
    done

    cd ${PROJ_DIR}
        echo $sudoPW | sudo -S ./PaLLOC -P $period -m 1 -p ${process_ids} -a 1
        mv experimenta_ipc_data.csv ${IPC_RESULTS_DIR}/experimenta_ipc_data_4.csv
        mv experimenta_ipc_data.data ${IPC_RESULTS_DIR}/experimenta_ipc_data_4.data

        mv experimenta_overhead_data.csv ${OVERHEAD_RESULTS_DIR}/experimenta_overhead_data_4.csv
        mv experimenta_overhead_data.data ${OVERHEAD_RESULTS_DIR}/experimenta_overhead_data_4.data
        echo "Move data success!"

    kill -- -$$
    wait

elif [[ $mix_num -eq 5 ]];then

    for app in ${apps_queue_5[@]}
    do
        cd ${PROJ_DIR}/scripts
        time ./process_exec.sh ${app} ${num_threads} &
        sleep 0.2
    done

    for app in ${apps_queue_5[@]}
    do
        pid_list=()
        while [ ${#pid_list[@]} -eq 0 ]; do
            pid_list=($(pgrep -f ${bench_process_name[${app}]}))
        done
        pid=${pid_list[0]}
        if [ -z "$process_ids" ]; then
            process_ids="${pid}"
        else
            process_ids="${process_ids},${pid}"
        fi
    done

    cd ${PROJ_DIR}
        echo $sudoPW | sudo -S ./PaLLOC -P $period -m 1 -p ${process_ids} -a 1
        mv experimenta_ipc_data.csv ${IPC_RESULTS_DIR}/experimenta_ipc_data_5.csv
        mv experimenta_ipc_data.data ${IPC_RESULTS_DIR}/experimenta_ipc_data_5.data

        mv experimenta_overhead_data.csv ${OVERHEAD_RESULTS_DIR}/experimenta_overhead_data_5.csv
        mv experimenta_overhead_data.data ${OVERHEAD_RESULTS_DIR}/experimenta_overhead_data_5.data
        echo "Move data success!"

    kill -- -$$
    wait

elif [[ $mix_num -eq 6 ]];then

    for app in ${apps_queue_6[@]}
    do
        cd ${PROJ_DIR}/scripts
        time ./process_exec.sh ${app} ${num_threads} &
        sleep 0.2
    done

    for app in ${apps_queue_6[@]}
    do
        pid_list=()
        while [ ${#pid_list[@]} -eq 0 ]; do
            pid_list=($(pgrep -f ${bench_process_name[${app}]}))
        done
        pid=${pid_list[0]}
        if [ -z "$process_ids" ]; then
            process_ids="${pid}"
        else
            process_ids="${process_ids},${pid}"
        fi
    done

    cd ${PROJ_DIR}
        echo $sudoPW | sudo -S ./PaLLOC -P $period -m 1 -p ${process_ids} -a 1
        mv experimenta_ipc_data.csv ${IPC_RESULTS_DIR}/experimenta_ipc_data_6.csv
        mv experimenta_ipc_data.data ${IPC_RESULTS_DIR}/experimenta_ipc_data_6.data

        mv experimenta_overhead_data.csv ${OVERHEAD_RESULTS_DIR}/experimenta_overhead_data_6.csv
        mv experimenta_overhead_data.data ${OVERHEAD_RESULTS_DIR}/experimenta_overhead_data_6.data
        echo "Move data success!"

    kill -- -$$
    wait

elif [[ $mix_num -eq 7 ]];then
    for app in ${apps_queue_7[@]}
    do
        cd ${PROJ_DIR}/scripts
        time ./process_exec.sh ${app} ${num_threads} &
        sleep 0.2
    done

    for app in ${apps_queue_7[@]}
    do
        pid_list=()
        while [ ${#pid_list[@]} -eq 0 ]; do
            pid_list=($(pgrep -f ${bench_process_name[${app}]}))
        done
        pid=${pid_list[0]}
        if [ -z "$process_ids" ]; then
            process_ids="${pid}"
        else
            process_ids="${process_ids},${pid}"
        fi
    done

    cd ${PROJ_DIR}
        echo $sudoPW | sudo -S ./PaLLOC -P $period -m 1 -p ${process_ids} -a 1
        mv experimenta_ipc_data.csv ${IPC_RESULTS_DIR}/experimenta_ipc_data_7.csv
        mv experimenta_ipc_data.data ${IPC_RESULTS_DIR}/experimenta_ipc_data_7.data

        mv experimenta_overhead_data.csv ${OVERHEAD_RESULTS_DIR}/experimenta_overhead_data_7.csv
        mv experimenta_overhead_data.data ${OVERHEAD_RESULTS_DIR}/experimenta_overhead_data_7.data
        echo "Move data success!"

    kill -- -$$
    wait

elif [[ $mix_num -eq 8 ]];then
    for app in ${apps_queue_8[@]}
    do
        cd ${PROJ_DIR}/scripts
        time ./process_exec.sh ${app} ${num_threads} &
        sleep 0.2
    done

    for app in ${apps_queue_8[@]}
    do
        pid_list=()
        while [ ${#pid_list[@]} -eq 0 ]; do
            pid_list=($(pgrep -f ${bench_process_name[${app}]}))
        done
        pid=${pid_list[0]}
        if [ -z "$process_ids" ]; then
            process_ids="${pid}"
        else
            process_ids="${process_ids},${pid}"
        fi
    done

    cd ${PROJ_DIR}
        echo $sudoPW | sudo -S ./PaLLOC -P $period -m 1 -p ${process_ids} -a 1
        mv experimenta_ipc_data.csv ${IPC_RESULTS_DIR}/experimenta_ipc_data_8.csv
        mv experimenta_ipc_data.data ${IPC_RESULTS_DIR}/experimenta_ipc_data_8.data

        mv experimenta_overhead_data.csv ${OVERHEAD_RESULTS_DIR}/experimenta_overhead_data_8.csv
        mv experimenta_overhead_data.data ${OVERHEAD_RESULTS_DIR}/experimenta_overhead_data_8.data
        echo "Move data success!"

    kill -- -$$
    wait

else
    for app in ${apps_queue_9[@]}
    do
        cd ${PROJ_DIR}/scripts
        time ./process_exec.sh ${app} ${num_threads} &
        sleep 0.2
    done

    for app in ${apps_queue_9[@]}
    do
        pid_list=()
        while [ ${#pid_list[@]} -eq 0 ]; do
            pid_list=($(pgrep -f ${bench_process_name[${app}]}))
        done
        pid=${pid_list[0]}
        if [ -z "$process_ids" ]; then
            process_ids="${pid}"
        else
            process_ids="${process_ids},${pid}"
        fi
    done

    cd ${PROJ_DIR}
        echo $sudoPW | sudo -S ./PaLLOC -P $period -m 1 -p ${process_ids} -a 1
        mv experimenta_ipc_data.csv ${IPC_RESULTS_DIR}/experimenta_ipc_data_9.csv
        mv experimenta_ipc_data.data ${IPC_RESULTS_DIR}/experimenta_ipc_data_9.data

        mv experimenta_overhead_data.csv ${OVERHEAD_RESULTS_DIR}/experimenta_overhead_data_9.csv
        mv experimenta_overhead_data.data ${OVERHEAD_RESULTS_DIR}/experimenta_overhead_data_9.data
        echo "Move data success!"

    kill -- -$$
    wait
fi    