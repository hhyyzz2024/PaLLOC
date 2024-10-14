#!/bin/bash

read -s -p "Enter Password for sudo: " sudoPW

./run_bench_palloc_core_orient_cpu_usage.sh ${sudoPW} &

result_file="palloc_cpu_usage.txt"
if [ $# -eq 1 ];
then
	result_file=$1
fi

app_name=PaLLOC
until pgrep ${app_name} >/dev/null; do
  echo "Waiting for ${app_name} to start..."
done

pid_list=($(pgrep -f ${app_name}))
pid=${pid_list[1]}

pidstat -p ${pid} 5 > ${result_file}

#while sudo kill -0 $pid > /dev/null 2>&1;
#do
#    cpu_usage=$(top -b -n 1 -p $pid | awk '/^%Cpu/ {print $2}')
#    echo $cpu_usage
#    sleep 2
#done
