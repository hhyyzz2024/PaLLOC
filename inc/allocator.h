#ifndef _ALLOCATOR_H_
#define _ALLOCATOR_H_

#include <unordered_map>
#include <vector>
#include <set>
#include <functional>
#include "monitor.h"
#include "discriminator.h"

#define MAX_MB_THROTTLE	100
#define MIN_MB_THROTTLE	20
#define BUG_MB_THROTTLE 10
#define MB_THROTTLE_LEVES 10
#define MIN_DELTA_THROTTLE 10
#define MIN_CACHE_MASK  0x1
#define MIN_CACHE_WAY	1
#define MAX_MB		88000
#define MAX_WAITING_TIMES 5
#define FREE_CLOS_NUM   13

namespace PaLLOC {

enum class allocator_status {
    init,
	dna_construct_excution_rate_table,
	allocating,
    make_cache_pair,
    get_cache_descriptor,
    find_first_pair_cache_roofline,
    find_second_pair_cache_roofline,
    pair_allocating_cache,
    make_mb_pair,
    get_mb_descriptor,
    find_first_pair_mb_roofline,
    find_second_pair_mb_roofline,
    pair_allocating_mb,
    updating_mb,
    allocating_end,
};

class allocator {
public:
    allocator(const std::vector<int>& objects, mode m, backend *backend, monitor *monitor, discriminator *discriminator);
    virtual ~allocator() {}
    virtual void allocating() = 0;
    virtual void reset_obj_allocation_descriptor(const int& obj_id) = 0;
    virtual void discriminating() = 0;

    uint64_t find_fit_bitmask(const uint32_t& requested_cache_way);
    void get_max_cache_bitmask();
    void cos_association(const std::vector<int>& cpu_ids, const int& class_id);
    void cos_association(const int& cpu_id, const int& class_id);
    inline void set_allocator_status(const allocator_status& status) {m_status = status;}
    inline allocator_status get_allocator_status() const {return m_status;}
    inline const std::vector<int>& get_objs() const {return objs;}

    uint32_t max_num_cache_way;							//The maximum number of cache ways in the system
    uint32_t remain_num_cache_way;						//The remain number of cache ways in the system
	uint32_t remain_cache_size;							//The remain cache size of the system
    uint32_t cache_size_per_way;
    mode m_mode;										//Manager mode, 0 is core mode, 1 is process mode (default 0)
    allocator_status m_status;
    double remain_mb;
    double max_mb;
    uint64_t remain_cache_bitmask;						//The remain cache bitmask of the system
    uint64_t max_cache_bitmask;
    backend *allocator_backend;					        //The backend of allocator, current is pqos
    monitor *m_monitor;
    discriminator *m_discriminator;
    const std::vector<int>& objs;
};

struct allocation_info {
	uint32_t cache_ways;
	uint32_t mb_throttle;
	uint64_t cache_bitmask;
};

struct target_perf {
	double llc_hit_ratio;
	double mb;
};

using allocating_func = std::function<void(std::vector<int>& discriminate_obj_ids)>;

}

#endif
