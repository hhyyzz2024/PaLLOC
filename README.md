# PaLLOC: Pairwise-based Low-Latency Online Coordinated Resources Management of Last-Level Cache and Memory Bandwidth on Multicore Systems

## Contents

- Setup PaLLOC
- Experiments


## Setup PaLLOC

### Prepare third-party libraries

#### Download third-party libraries
```
cd $PaLLOC Directory$/third_party 
git submodule update --init --recursive
```

#### Compile Intel(R) RDT Software Package
```
cd intel-cat-cmt
make
sudo make install
ldconfig -v
```

#### Compile Intel PCM
```
cd pcm
mkdir build
cd build
cmake .. && make
sudo make install
ldconfig -v
```

### Compile PaLLOC
```
cd $PaLLOC Directory$
make test=1
```

### How to use PaLLOC
Usage:
```
sudo ./PaLLOC [-a bool] [-P int] [-m int] [-c string] [-p string] [-h]
Options:
    -a, --allocator=bool			Whether to turn on the resource allocator (default true means open).
    -P, --period=int				The sampling period (ms, default 100ms) of monitor.
    -m, --mode=int					The mode of system (cores mode 0, processes mode 1, default 0).
    -c, --cores=string				The cores of system, use commas separated string of core numbers ("0,1,2,3,...", need -m 0).
    -p, --pids=string				The pids of system, use commas separated string of pids ("pid0,pid1,pid2,pid3,...", need -m 1").
    -h, --help						Print this help message and exit.
```
If -a is set to 0, PaLLC supports monitoring-only mode.
At this time, PaLLOC will only periodically monitor the resource usage of the target cores or process.
For example, monitor the resource usage of cores 0, 1, 2, and 3 with a period of 100 ms: 
```
sudo ./PaLLOC -a 0 -P 100 -m 0 -c "0,1,2,3"
```
If -a is set to 1, PaLLC will allocates resources to the target core or process.
For example, dynamically allocate resources to cores 0, 1, 2, and 3:
```
sudo ./PaLLOC -a 1 -P 100 -m 0 -c "0,1,2,3"
```
It must be noted that the current version does not support resource allocation to processes, and this feature will be added in subsequent versions.

## Experiments

### Prepare Benchmarks
Please download these three benchmarks, [PARSEC-3.0](https://acs.ict.ac.cn/baoyg/projects/202203/t20220317_20933.html), [SPEC CPU® 2006](https://www.spec.org/cpu2006/) and [SPEC CPU® 2017](https://www.spec.org/cpu2017/).
Then compile and run the benchs in these benchmark suites once.
Since the compiler has been upgraded, you may need to do some compilation fixes for the older benchmarks, PARSEC 3.0 and SPEC 2006.

After compiling, please modify the benchmark configuration file:
```
cd scripts
vim benchmark.conf
```
Modify three variables related to the benchmark path:
```
SPEC2017_DIR=$Your SPEC2017 PATH$
SPEC2006_DIR=$Your SPEC2006 PATH$
PARSEC_DIR=$Your PARSEC PATH$
```

It must be noted that the experiment uses the SPEC rate benchmark by default.
Please check some information of benchmarks in benchmark.conf, such as process information, benchmark execution command, benchmark directory.
Tip: You can get the single-step execution command of SPEC benchmark by executing the following command:
```
cd $Your SPEC2017 PATH$/benchspec/CPU/502.gcc_r/run/run_base_refrate_mytest-m64.0000/
specinvoke -n
```

### Prepare Cores
Please modify the file core_define.conf to configure the set of CPU cores that the workload queue executes on.

```
# Confirm which CPU cores participate in the experiment. These CPU cores must be on the same CPU.
lscpu  
vim core_define.conf
cores=(x,x,x,x)
cores_arg="x,x,x,x"
```

### Prepare workloads
Modify workload_defineX.conf to complete the configuration of the workload. Then, the file name must be written into the workload_define_files array of run_bench_palloc.sh.
If there is already a file workload_define0.conf in the original array, now add a workload_define1.conf file, run_bench_palloc.sh is modified to:

```
workload_define_files=(workload_define0.conf, workload_define1.conf)
```
Note: Typically, each workload_define.conf should be a resource requirement configuration of 𝑅𝑒𝑠𝐷𝑒𝑚𝑎𝑛𝑑𝑠𝑅𝑎𝑡𝑖𝑜 in paper.

The format of workload_define.conf is as follows:

1. Define workload_queues list, the number of which is the same as the number of CPU cores in core_define.conf. The content is a benchmark, which is randomly generated according to the workload generation method in the paper. The name of the benchmark must be the same as defined in benchmark.conf.
```
core0_workload_queue=()
core1_workload_queue=()
...
coreN_workload_queue=()
```

2. Defining workload_mix list. The format:
```
declare -A workload_mixX
workload_mixX=(
    [${cores[0]}]=${core0_workload_queue[@]}
    [${cores[1]}]=${core1_workload_queue[@]}
    ...
    [${cores[N]}]=${coreN_workload_queue[@]}
)
```

3. workload_queues and workload_mix form a workload mix. You can define as many mixes as you want, in this experiment it is 10. Note that you must modify the variables in run_bench_palloc.sh to determine the mix you need to run. For example
```
mixs=(0 1 2 3 4 5 6 7 8 9)
```

### Run the experiment
After the modification is complete, you can run the script and enter the sudo password
```
./run_bench_palloc.sh
```
You can get the result data of the relevant experiments in the PaLLOCDirectory/results directory