#ifndef _UTILS_H_
#define _UTILS_H_

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <vector>
#include <unordered_map>

enum RET_CODE {
    RET_OK = 0,
    RET_ARGUMENT_ERR,
    RET_FOPEN_ERR,
    RET_FREAD_ERR,
	RET_MONITOR_EXIST_ERR,
    RET_ERR,
};

enum class mode {
	CORES=0,
	PROCESSES,
	UNKNOWN_MODE,
};

enum class allocating_strategy {
	down,
	binary,
	dna,
	balancer,
	pair,
	fast_pair,
	isolate_pair,
	fair,
	orchid,
	no_management,
	test,
	unkonwn_strategy,
};

using ret_t = RET_CODE;

const int WINDOW_SIZE = 7;
const int DIM = 4;
const int DISCRIMINATE_POINT = 4;

#define perr(fmt, ...)   fprintf(stdout, "[ERROR] [%s:%s:%d]" fmt, __FILE__, __FUNCTION__, __LINE__);

#ifdef DEBUG_ON
#define DEBUGP(fmt, ...) printf(fmt, ##__VA_ARGS__);
#else
#define DEBUGP(fmt, ...)
#endif

// default print_level is info
#ifndef print_level
#define print_level 2
#endif

#define MINILOG_FOREACH_LOG_LEVEL(f) \
    f(trace) \
    f(debug) \
    f(info) \
    f(critical) \
    f(warn) \
    f(error) \
    f(fatal)

enum class log_level : std::uint8_t {
#define _FUNCTION(name) name,
    MINILOG_FOREACH_LOG_LEVEL(_FUNCTION)
#undef _FUNCTION
};

static inline const char *log_level_name(log_level lev) {
    switch (lev) {
#define _FUNCTION(name) case log_level::name: return #name;
	MINILOG_FOREACH_LOG_LEVEL(_FUNCTION)
#undef _FUNCTION
		default:
			return "unknow";
    }
}

static const char k_level_ansi_colors[(std::uint8_t)log_level::fatal + 1][8] = {
	"\E[37m",
	"\E[35m",
	"\E[32m",
	"\E[34m",
	"\E[33m",
	"\E[31m",
	"\E[31;1m",
};

#define output_log(lev, fmt, ...) do {                                             \
	if(static_cast<uint8_t>(lev) >= print_level) {                                                                                  \
		fprintf(stdout, "%s[%s] [%s:%s:%d] " fmt "\E[m", k_level_ansi_colors[static_cast<uint8_t>(lev)], log_level_name(lev), __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);         \
	}                               \
} while(0)

#define log_trace(fmt, ...) output_log(log_level::trace, fmt, ##__VA_ARGS__)
#define log_debug(fmt, ...) output_log(log_level::debug, fmt, ##__VA_ARGS__)
#define log_info(fmt, ...)  output_log(log_level::info, fmt, ##__VA_ARGS__)
#define log_critical(fmt, ...) output_log(log_level::critical, fmt, ##__VA_ARGS__)
#define log_warn(fmt, ...) output_log(log_level::warn, fmt, ##__VA_ARGS__)
#define log_error(fmt, ...) output_log(log_level::error, fmt, ##__VA_ARGS__)
#define log_fatal(fmt, ...) output_log(log_level::fatal, fmt, ##__VA_ARGS__)

#define IA32_MISC_ENABLE_OFFSET         0x1a4
#define DISBALE_PREFETCH(msr)           (msr |= 0xf)
#define rounddown(x, y) (((x) / (y)) * (y))

bool is_ht_enable();
int get_num_closids(uint32_t *num_closids);
int get_llc_info(uint32_t *cache_size, uint64_t *cache_mask, uint32_t *num_cache_way, uint32_t *size_per_way);
int get_cpus_from_pid(pid_t pid, std::vector<int>& cpus);
int get_cpu_from_pid(pid_t pid);
static inline unsigned sock_to_l3id(const unsigned& sock) {return sock;}
static inline unsigned sock_to_mbid(const unsigned& sock) {return sock;}

double cpu_second(void);
double cpu_microsecond(void);

#ifdef TEST
struct attempt_allocating_data {
	double attempt_allocating_elapse;
	long long attempt_allocating_num;
	long long find_roofline_num;
	long long allocating_num;
};

struct experimental_data{
	long long find_roofline_num = 0;												// The number of times to find roofline top in an allocation request
	long long allocating_num = 0;													// The number of times resources are allocated in an allocation request
	long long call_allocating_num = 0;												// The number of times the allocating function is called in an allocation request
	long long allocating_request_times = 0;											// The number of requests to allocate resources
	double attempt_allocating_start;
	double algorithm_start;
	std::unordered_map<int, double> obj_ipc_table;
	std::vector<attempt_allocating_data> attempt_allocating_exp_data_vec;
	std::vector<double> algorithm_overhead_vec;
};
#endif

#endif
