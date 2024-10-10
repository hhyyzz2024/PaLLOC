#define _GNU_SOURCE
#include <sched.h>
#include "utils.h"
#include <string.h>
#include <cpuid.h>
#include <unistd.h>
#include <cerrno>
#include <numa.h>
#include <sys/time.h>

bool is_ht_enable()
{
	FILE *fp;
	char buffer[256];
	bool ret = false;

	fp = popen("cat /sys/devices/system/cpu/smt/active", "r");
	if (fp != NULL)
	{
    	if(fgets(buffer, 256, fp) != NULL) {
        	printf("%s", buffer);
			ret = atoi(buffer);
    	}
    	pclose(fp);
	}

	return ret;
}

static int read_uint64(const char *fname, unsigned base, uint64_t *value)
{
    FILE *fd;
    char buf[17] = "\0";
    char *s = buf;
    char *endptr = NULL;
    size_t bytes;
    unsigned long long val;

    if(fname == nullptr || value == nullptr) {
		return RET_ARGUMENT_ERR;
	}

    fd = fopen(fname, "r");
    if (fd == NULL)
         return RET_FOPEN_ERR;

    bytes = fread(buf, sizeof(buf) - 1, 1, fd);
    if (bytes == 0 && !feof(fd)) {
        fclose(fd);
        return RET_FREAD_ERR;
    }
    fclose(fd);

    val = strtoull(s, &endptr, base);

    if (!((*s != '\0' && *s != '\n') &&
          (*endptr == '\0' || *endptr == '\n'))) {
        printf("Error converting '%s' to unsigned number!\n", buf);
        return RET_ERR;
    }
    if (val < UINT64_MAX) {
        *value = val;
        return RET_OK;
    }

    return RET_ERR;
}

static int get_llc_bit_mask(uint64_t *mask)
{
    int ret;
    uint64_t val;

	const char *path = "/sys/fs/resctrl/info/L3/cbm_mask";

    ret = read_uint64(path, 16, &val);
    if (ret != RET_OK)
    	return ret;

	if (val > 0 && val <= UINT64_MAX) {
		*mask = val;
		return RET_OK;
	}

    return RET_ERR;
}

int get_num_closids(uint32_t *num_closids)
{
    int ret;
    uint64_t val;

	const char *path = "/sys/fs/resctrl/info/L3/num_closids";

    ret = read_uint64(path, 10, &val);
    if (ret != RET_OK)
    	return ret;

    if (val > 0 && val <= UINT32_MAX) {
        *num_closids = val;
    }

    return RET_ERR;
}

int get_llc_info(uint32_t *cache_size, uint64_t *cache_mask, uint32_t *num_cache_way, uint32_t *size_per_way)
{
    unsigned int eax, ebx, ecx, edx;
    int cache_type, cache_level;
    unsigned int size, ways, partitions, line_size, sets;
        int subleaf;
        uint64_t mask = 0;

    for (subleaf = 0; ; subleaf++) {
        /* The value 0x04 for EAX is a specific function number that instructs
           the cpuid instruction to get cache parameters for Intel CPUs.
           The value of AMD CPUs is 0x8000001D*/
        __cpuid_count(0x04, subleaf, eax, ebx, ecx, edx);

        cache_type = eax & 0x1F;
        if (cache_type == 0) {
            break; // No more caches
        }
    }

        // subleaf - 1 is last level cache
        __cpuid_count(0x04, subleaf-1, eax, ebx, ecx, edx);

    ways = ((ebx >> 22) & 0x3FF) + 1;
    partitions = ((ebx >> 12) & 0x3FF) + 1;
    line_size = (ebx & 0xFFF) + 1;
    sets = ecx + 1;

    size = ways * partitions * line_size * sets;
    *cache_size = size;
    *num_cache_way = ways;
    *size_per_way = size / ways;

    while(ways--) {
            mask = (mask << 1) | 1;
    }
    *cache_mask = mask;

    printf("Cache info: size %u, mask: 0x%lx, ways: %u, cache size per way: %u\n",
            *cache_size, *cache_mask, *num_cache_way, *size_per_way);

    return 0;
}

int get_cpus_from_pid(pid_t pid, std::vector<int>& cpu_ids)
{
	cpu_set_t cpu_mask;
	int cpus = sysconf(_SC_NPROCESSORS_CONF);
	
	CPU_ZERO(&cpu_mask);
    if (sched_getaffinity(0, sizeof(cpu_mask), &cpu_mask) == -1) {
        printf("Get CPU affinity failue, ERROR:%s\n", strerror(errno));
        return RET_ERR; 
    }

	for(int cpu = 0; cpu < cpus; cpu++) {
		if (CPU_ISSET(cpu, &cpu_mask)) {
        	cpu_ids.emplace_back(cpu);
        }    
	}

	return RET_OK;
}

int get_cpu_from_pid(pid_t pid)
{
    std::vector<int> cpu_ids;
	cpu_set_t cpu_mask;
	int cpus = sysconf(_SC_NPROCESSORS_CONF);
	
	CPU_ZERO(&cpu_mask);
    if (sched_getaffinity(0, sizeof(cpu_mask), &cpu_mask) == -1) {
        printf("Get CPU affinity failue, ERROR:%s\n", strerror(errno));
        return -1; 
    }

	for(int cpu = 0; cpu < cpus; cpu++) {
		if (CPU_ISSET(cpu, &cpu_mask)) {
        	cpu_ids.emplace_back(cpu);
        }    
	}

	return cpu_ids[0];
}

double cpu_second(void)
{
    struct timeval tv;
    double t;

    gettimeofday(&tv, nullptr);
    t = tv.tv_sec + ((double)tv.tv_usec)/1000000;
    return t;
}

double cpu_microsecond(void)
{
    long long ms;
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    ms = (ts.tv_sec * 1000 + ((double)ts.tv_nsec) / 1000000);
    return ms;
}

uint32_t get_socket(int obj, mode m)
{
    uint32_t socket;
    int core_id;

    if(m == mode::PROCESSES) {
        core_id = get_cpu_from_pid(obj);
    } else {
        core_id = obj;
    }

    socket = numa_node_of_cpu(core_id);

    return socket;
}