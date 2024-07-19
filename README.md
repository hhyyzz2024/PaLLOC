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

### Prepare workloads

