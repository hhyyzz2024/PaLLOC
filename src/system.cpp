#include "system.h"
#include "pairwise_allocator.h"
#include "test_allocator.h"
#include <mutex>
#include <time.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <thread>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <getopt.h>

#ifdef TEST
static long long end_duration = 600000;
//static long long end_duration = 5000;
//static long long end_duration = 300000;
//static long long end_duration = 60000;
//static long long end_duration = 12000;
#endif

namespace PaLLOC {

system *system::instance = nullptr;

static int disable_prefetch(int cpu, uint64_t *flag)
{
    int ret, fd;
    char fname[100];
    uint64_t msr;

    sprintf(fname, "/dev/cpu/%d/msr", cpu);
    ret = fd = open(fname, O_RDWR);
    if (ret < 0) {
        log_error("Couldn't open msr dev. Please run 'modprobe msr' and run this program with root permissions\n");
        return -1;
    }

    ret = pread(fd, &msr, sizeof(msr), IA32_MISC_ENABLE_OFFSET);
    if (ret != sizeof(msr)) {
        log_error("Couldn't read msr dev\n");
        return -1;
    }

    *flag = msr;
    log_debug("Old MSR:0x%lx\n", msr);

    DISBALE_PREFETCH(msr);

    ret = pwrite(fd, &msr, sizeof(msr), IA32_MISC_ENABLE_OFFSET);
    if (ret != sizeof(msr)) {
        log_error("Couldn't write msr dev: %s\n", strerror(errno));
        return -1;
    }

    log_debug("New MSR:0x%lx\n", msr);

    return 0;
}

static int enable_prefetch(int cpu, uint64_t flag)
{
    int ret, fd;
    char fname[100];
    uint64_t msr;

    sprintf(fname, "/dev/cpu/%d/msr", cpu);
    ret = fd = open(fname, O_RDWR);
    if (ret < 0) {
        log_error("Couldn't open msr dev. Please run 'modprobe msr'\n");
        return -1;
    }

    ret = pread(fd, &msr, sizeof(msr), IA32_MISC_ENABLE_OFFSET);
    if (ret != sizeof(msr)) {
        log_error("Couldn't read msr dev\n");
        return -1;
    }

    log_debug("New MSR:0x%lx\n", msr);

    ret = pwrite(fd, &flag, sizeof(flag), IA32_MISC_ENABLE_OFFSET);
    if (ret != sizeof(msr)) {
        log_error("Couldn't write msr dev\n");
        return -1;
    }

    log_debug("Set MSR:0x%lx\n", flag);

    return 0;
}

system *system::get_instance(int argc, char **argv)
{
    if(!instance) {
        std::once_flag flag;
        std::call_once(flag, [&]{ instance = new (std::nothrow) system(argc, argv); });
    }

    return instance;
}

system *system::get_instance()
{
	return instance;
}


void system::clean_up()
{
#ifdef TEST
	std::ofstream fout1("experimenta_ipc_data.csv", std::ios_base::out);
	std::ofstream fout2("experimenta_ipc_data.data", std::ios_base::out);
	if(m_mode == mode::CORES) {
		fout1 << "Core,IPC" << std::endl;
		fout2 << "#Core IPC" << std::endl;
	} else {
		fout1 << "Pid,IPC" << std::endl;
		fout2 << "#Pid IPC" << std::endl;
	}

	double average_allocation_delay = 0.0;
	double average_allocation_num = 0.0;
	double average_find_roofline_num = 0.0;
	double average_allocating_num = 0.0;
	double system_algorithm_overhead = 0.0;
	double average_algorithm_overhead;

	for(const auto& attempt_allocating_exp_data : exp_data.attempt_allocating_exp_data_vec) {
		average_allocation_delay += attempt_allocating_exp_data.attempt_allocating_elapse;
		average_allocation_num += attempt_allocating_exp_data.attempt_allocating_num;
		average_find_roofline_num += attempt_allocating_exp_data.find_roofline_num;
		average_allocating_num += attempt_allocating_exp_data.allocating_num;
	}

	average_allocation_delay /= exp_data.attempt_allocating_exp_data_vec.size();
	average_allocation_num /= exp_data.attempt_allocating_exp_data_vec.size();
	average_find_roofline_num /= exp_data.attempt_allocating_exp_data_vec.size();
	average_allocating_num /= exp_data.attempt_allocating_exp_data_vec.size();

	for(const auto& algorithm_overhead : exp_data.algorithm_overhead_vec) {
		system_algorithm_overhead += algorithm_overhead;
	}
	average_algorithm_overhead = system_algorithm_overhead / exp_data.algorithm_overhead_vec.size();

	for(auto& obj_ipc_pair : exp_data.obj_ipc_table) {
		int obj = obj_ipc_pair.first;

		const std::vector<monitor_data>& mon_datas = m_monitor->get_monitor_datas(obj);
		for(const auto& mon_data : mon_datas) {
			obj_ipc_pair.second += mon_data.ipc;
		}

		obj_ipc_pair.second /= mon_datas.size();
		
		fout1 << obj << "," << obj_ipc_pair.second << std::endl;
		fout2 << obj << " " << obj_ipc_pair.second << std::endl;
	}

	fout1.close();
	fout2.close();

	fout1.open("experimenta_overhead_data.csv", std::ios_base::out);
	fout2.open("experimenta_overhead_data.data", std::ios_base::out);


	// Average_Attempt_Allocation_Num means the average number of attempts to call the PQOS allocation function, including calls when searching for rooflines and calls when allocating resources in pair algrithom.
	// Average_Find_Roofline_Num means the average number of times to find roofline in an allocation request.
	// Average_Allocating_num means the average number of allocating resource in an allocation request.
	// Total_Allocation_Request means the total number of resource allocation requests during the management period
	// Average_Allocation_Delay means the time from the start of an allocation request to the completion of the allocation
	// Algorithm_Total_Overhead means the total overhead of the allocation algorithm over the management period
	// Algorithm_Average_Overhead means average allocation algorithm overhead for each allocation
	fout1 << "Average_Attempt_Allocation_Num,Average_Find_Roofline_Num,Average_Allocating_num,Total_Allocation_Request,Average_Allocation_Delay,Algorithm_Total_Overhead,Algorithm_Average_Overhead" << std::endl;
	fout1 << average_allocation_num << "," << average_find_roofline_num << "," << average_allocating_num << "," << exp_data.allocating_request_times << "," << average_allocation_delay << "," << system_algorithm_overhead << "," << average_algorithm_overhead << std::endl;

	fout2 << "#Average_Attempt_Allocation_Num Average_Find_Roofline_Num Average_Allocating_num Total_Allocation_Request Average_Allocation_Delay Algorithm_Total_Overhead Algorithm_Average_Overhead" << std::endl;
	fout2 << average_allocation_num << " " << average_find_roofline_num << " " << average_allocating_num << " " << exp_data.allocating_request_times << " " << average_allocation_delay << " " << system_algorithm_overhead << " " << average_algorithm_overhead << std::endl;

	fout1.close();
	fout2.close();
#endif

    auto *sys = get_instance();
    if(sys != nullptr) {
        delete sys;
    }
}

void system::process_exit_handle(int signo)
{
    auto *sys = get_instance();
    sys->process_exit = true;
	sys->clean_up();
	exit(1);
}

system::system(int argc, char **argv)
{
	parse_argument(argc, argv);

	int max_core_num = std::thread::hardware_concurrency();
	flags = new uint64_t[max_core_num];
    for(int core = 0; core < max_core_num; core++) {
        disable_prefetch(core, &flags[core]);
    }

	int objs_size = objs.size();
	
	system_core = 1;
	
    pid = getpid();
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(system_core, &cpuset);
    sched_setaffinity(getpid(), sizeof(cpu_set_t), &cpuset);

	signal(SIGINT, process_exit_handle);

#ifdef TEST
	for(const auto& obj : objs) {
		exp_data.obj_ipc_table.insert({obj, 0.0});
	}
#endif

	m_tool_backend = new pqos_backend(objs, m_mode);
	m_monitor = monitor::get_instance(period, objs, m_tool_backend);
	
	for(int i = 0; i < objs_size; i++) {
		int index = objs[i];
		window_data_map.emplace(index, win_data(WINDOW_SIZE, DIM));
	}

	m_discriminator = discriminator::get_instance(window_data_map);
	m_discriminator->set_discriminate_points();
	

	if(allocator_on) {
#ifdef TEST
		m_allocator = new pairwise_allocator(objs, m_mode, m_tool_backend, m_monitor, m_discriminator, allocation_interval, exp_data);
#else
		m_allocator = new pairwise_allocator(objs, m_mode, m_tool_backend, m_monitor, m_discriminator, allocation_interval);
#endif
	}

//    m_recorder = new recorder(objs);
}

system::~system()
{
    for(int i = 0; i < objs.size(); i++) {
        int obj = objs[i];

        char csv_file[1024];
        char data_file[1024];

		if(m_mode == mode::CORES) {
			if(allocator_on) {
				sprintf(csv_file, "allocator_on_core_%d.csv", obj);
	        	sprintf(data_file, "allocator_on_core_%d.data", obj);
			} else {
	        	sprintf(csv_file, "monitoring_core_%d.csv", obj);
	        	sprintf(data_file, "monitoring_core_%d.data", obj);
			}
		} else {
			if(allocator_on) {
				sprintf(csv_file, "allocator_on_pid_%d.csv", obj);
	        	sprintf(data_file, "allocator_on_pid_%d.data", obj);
			} else {
				sprintf(csv_file, "monitoring_pid_%d.csv", obj);
	        	sprintf(data_file, "monitoring_pid_%d.data", obj);
			}
		}

        std::ofstream fout1(csv_file, std::ios_base::out);
        std::ofstream fout2(data_file, std::ios_base::out);

        fout1 << "Time" << "," << "IPC" << "," << "LLC_Hit" << "," << "MPKI" << "," << "HPKI" << "," << "Memory_Bandwidth" << std::endl;
        fout2 << "# Time" << " " << "IPC" << " " << "LLC_Hit" << " " << "MPKI" << " " << "HPKI" << " " << "Memory_Bandwidth" << std::endl;

		const std::vector<monitor_data>& monitor_datas = m_monitor->get_monitor_datas(obj); 

		for(int j = 0; j < monitor_datas.size(); j++) {
			fout1 << monitor_datas[j].current_time << "," << monitor_datas[j].ipc << "," << monitor_datas[j].llc_hit << "," << monitor_datas[j].mpki << "," << monitor_datas[j].hpki << "," << monitor_datas[j].mb << std::endl;
			fout2 << monitor_datas[j].current_time << " " << monitor_datas[j].ipc << " " << monitor_datas[j].llc_hit << " " << monitor_datas[j].mpki << " " << monitor_datas[j].hpki << " " << monitor_datas[j].mb << std::endl;
		}
		
        fout1.close();
        fout2.close();
    }

	m_monitor->clean_up();
	m_discriminator->clean_up();

	if(allocator_on) {
		delete m_allocator;
	}

	delete m_tool_backend;
	
//    delete m_recorder;

	int max_core_num = std::thread::hardware_concurrency();
	for(int core = 0; core < max_core_num; core++) {
		enable_prefetch(core, flags[core]);
	}

	delete [] flags;
}

void system::run()
{
    struct timespec req;
    req.tv_sec = period / 1000;
    req.tv_nsec = (period % 1000) * 1000000;
    struct timespec rem;

	long long start = cpu_microsecond();
    while(!process_exit) {
		
		// monitoring
        m_monitor->start_monitoring();
        nanosleep(&req, &rem);
        m_monitor->stop_monitoring();

        m_monitor->parse_monitor_data(window_data_map);
		long long elapse = cpu_microsecond() - start;
#ifdef TEST
		if(end_duration <= elapse) {
			process_exit = 1;
			continue;
		}
#endif

		if(allocator_on) {
			m_allocator->discriminating();
			m_allocator->allocating();
		}
    }

    log_info("Will exit run!\n");
}

void system::parse_objs(const std::string& s)
{
	std::stringstream ss(s);
	std::string str;

	while(getline(ss, str, ',')){
		objs.emplace_back(std::stoi(str));
	}
}

void system::parse_argument(int argc, char **argv)
{
	int opt;
	int option_index = 0;
	int check_mode = 0;
	
	struct option long_options[] = {
		{"allocator", required_argument, 0, 'a'},
		{"period", required_argument, 0, 'P'},
		{"interval", required_argument, 0, 'i'},
		{"mode", required_argument, 0, 'm'},
		{"cores",     required_argument, 0,  'c' },
        {"pids",  required_argument,       0,  'p'},
		{"help",  no_argument, 0, 'h'},
        {0,         0,                 0,  -1}
	};
	
	while((opt = getopt_long(argc,argv,"a:P:i:m:c:p:s:h", long_options, &option_index)) != -1) {
		switch (opt) {
			case 'a':
				allocator_on = atoi(optarg);
				break;
		
			case 'P':
				period = atoi(optarg);
				break;

			case 'i':
				allocation_interval = atoi(optarg);
				break;

			case 'm':
				m_mode = static_cast<mode>(atoi(optarg));
				break;

			case 'c':
				check_mode++;
				parse_objs(std::string(optarg));
				break;

			case 'p':
				check_mode++;
				parse_objs(std::string(optarg));
				break;

			case 'h':
				show_usage();
				exit(0);

			default:
				show_usage();
				exit(1);
		}
	}

	if(m_mode != mode::CORES && m_mode != mode::PROCESSES && check_mode != 1) {
		log_error("Argument error, only one mode can be selected and parameterized");
		show_usage();
		exit(1);
	}

	log_debug("period: %ld, mode: %d, objs:", period, static_cast<int>(m_mode));
	for(int i = 0; i < objs.size(); i++) {
		log_debug("%d ", objs[i]);
	}
	log_debug("\n");
}

void system::show_usage()
{
	printf("Usage: ./PaLLOC -P 100 -i 10 -m 0 -c 1,2,3,4\n");
	printf("Options:\n");
	printf("	-a, --allocator=bool			Whether to turn on the resource allocator (default true means open).\n");
	printf("	-P, --period=int				The sampling period (ms, default 100ms) of monitor.\n");
	printf("	-i, --interval=int				The allocation interval (ms, default 10ms) of allocator.\n");
	printf("	-m, --mode=int					The mode of system (cores mode 0, processes mode 1, default 0).\n");
	printf("	-c, --cores=string				The cores of system, use commas separated string of core numbers (\"0,1,2,3,...\", need -m 0.\n");
	printf("	-p, --pids=string				The pids of system, use commas separated string of pids (\"pid0,pid1,pid2,pid3,...\", need -m 1.\n");
	printf("	-h, --help						Print this help message and exit.\n");
}

}
