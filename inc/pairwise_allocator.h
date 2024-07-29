#ifndef _ISOLATE_PAIR_ALLOCATOR_H_
#define _ISOLATE_PAIR_ALLOCATOR_H_

#include "allocator.h"
#include <vector>
#include <list>
#include <queue>

namespace PaLLOC {

class pairwise_allocator : public allocator {

public:
#ifdef TEST
    pairwise_allocator(const std::vector<int>& objects, mode m, backend *backend, monitor *monitor, discriminator *discriminator, int interval,
        experimental_data& exp_data);
#else
    pairwise_allocator(const std::vector<int>& objects, mode m, backend *backend, monitor *monitor, discriminator *discriminator, int interval);
#endif
    virtual ~pairwise_allocator() {}
    virtual void reset_obj_allocation_descriptor(const int& obj_id);
    virtual void discriminating();
    virtual void allocating();

private:
    struct cache_roofline_performance {
        double ipc;
        double hpki; 
    };

    struct mb_roofline_performance {
        double ipc;
        double mb;
        double mph;
    };

    enum class payment_status {
        init,
        update_ipc,
        unpaid,
    };

    struct allocation_context {
        uint32_t cache_ways;
        uint64_t cache_bitmask;
    };

    struct allocation_descriptor {
        int class_id;								// The id of the class of service
        int obj_id;                                 // core or app id
        int core_id;
        int sockect_id;

        uint32_t cache_ways = 0;					// The number of cache ways allocated
        uint32_t right_cache_ways;
        uint32_t left_cache_ways;
        uint32_t roofline_cache_ways = 0;

        uint32_t mb_throttle = 0;					// Memory bandwidth throttle (10~100)
        uint32_t right_mb_throttle;
        uint32_t left_mb_throttle;
        uint32_t roofline_mb_throttle = 0;

        uint32_t find_cache_roofline_times = 0;
        uint32_t pair_allocating_cache_times = 0;

        uint32_t find_mb_roofline_times = 0;
        uint32_t pair_allocating_mb_times = 0;

        payment_status p_status = payment_status::init;

        bool phase_change = false;                          // If phase has not changed, roofline will maintain
        bool find_cache_roofline = false;
        bool find_mb_roofline = false;
        bool pair_allocating_cache_completed = false;
        bool pair_allocating_mb_completed = false;
        bool has_obtained_mb_from_pair = false;

        double allocating_mb = 0;                   // true memory bandwidth get from system (MB/s) 
        double delta;                               // delta_ipc / roofline_ipc;

        uint64_t cache_bitmask = 0;					// The bitmask of cache allocation

        cache_roofline_performance cache_roofline_perf;
        mb_roofline_performance mb_roofline_perf;

        allocation_context context;

        monitor_data current_data;					// Monitoring data in the current period
        monitor_data prev_data;						// Monitoring data in the previous period

        std::unordered_map<int, uint32_t> obj_borrowed_cache_way_map;
    };


private:

    // Member function definitions

    struct cache_allocation_less {
        bool operator()(const allocation_descriptor* d1, const allocation_descriptor* d2)
        {
            return d1->current_data.hpki < d2->current_data.hpki;
        }
    };

    struct mb_allocation_less {
        bool operator()(const allocation_descriptor* d1, const allocation_descriptor* d2)
        {
            return d1->current_data.mpki  < d2->current_data.mpki;
        }
    };

    inline void deduct_roofline_remain_cache(allocation_descriptor *descriptor) {
        roofline_remain_bitmask -= descriptor->cache_bitmask;
        roofline_remain_cache -= descriptor->cache_ways;
    }

    inline void deduct_roofline_remain_cache(uint32_t cache_ways, uint64_t cache_bitmask) {
        roofline_remain_bitmask -= cache_bitmask;
        roofline_remain_cache -= cache_ways;
    }

    inline void return_roofline_remain_cache(allocation_descriptor *descriptor) {
        roofline_remain_bitmask += descriptor->cache_bitmask;
        roofline_remain_cache += descriptor->cache_ways;
    }

    inline void return_roofline_remain_cache(uint32_t cache_ways, uint64_t cache_bitmask) {
        roofline_remain_bitmask += cache_bitmask;
        roofline_remain_cache += cache_ways;
    }

    inline void return_cache_to_system(allocation_descriptor *descriptor) {
        remain_cache_bitmask += descriptor->cache_bitmask;
        remain_num_cache_way += descriptor->cache_ways;
    }

    inline void deduct_cache_from_system(allocation_descriptor *descriptor) {
        remain_cache_bitmask -= descriptor->cache_bitmask;
        remain_num_cache_way -= descriptor->cache_ways;
    }

    inline void update_descriptor_data(allocation_descriptor* descriptor, const monitor_data& current_data) {
        if(descriptor) {
            descriptor->prev_data = descriptor->current_data;
            descriptor->current_data = current_data;
        }
	}

    inline void get_cache_allocating_descriptor() {
        allocating_cache_descriptor = cache_ready_queue.top();
    }

    inline void get_mb_allocating_descriptor() {
        allocating_mb_descriptor = mb_ready_queue.top();
    }

    inline void update_cache_roofline_perf(allocation_descriptor *descriptor) {
        descriptor->cache_roofline_perf.ipc = descriptor->current_data.ipc;
        descriptor->cache_roofline_perf.hpki = descriptor->current_data.hpki;
    }

    inline void updating_allocating_mb() {  
        allocating_mb_descriptor->allocating_mb = allocating_mb_descriptor->current_data.mb;
    }

    inline void deduct_mb_from_system(allocation_descriptor *descriptor) {
        remain_mb -= descriptor->allocating_mb;
    }

    inline void return_mb_to_system(allocation_descriptor *descriptor) {
        remain_mb += descriptor->allocating_mb;
    }

    inline void update_mb_roofline_perf(allocation_descriptor *descriptor) {
        descriptor->mb_roofline_perf.ipc = descriptor->current_data.ipc;
        descriptor->mb_roofline_perf.mb = descriptor->current_data.mb;
        descriptor->mb_roofline_perf.mph = descriptor->current_data.mpki / descriptor->current_data.hpki;
    }

    int insert_cache_ready_queue(const int& obj_id);
    uint64_t find_fit_roofline_bitmask(uint32_t& requested_cache_way);
    void check_cache_leak();
    void return_cache(allocation_descriptor *descriptor);
    void move_other_objs_to_pair2_clos(allocation_descriptor *descriptor);
    void move_all_objs_to_their_own_clos();
    void up_search_cache_roofline(allocation_descriptor *descriptor);
    void down_search_cache_roofline(allocation_descriptor *descriptor);
    void found_cache_roofline(allocation_descriptor *descriptor);
    bool binary_serch_cache_roofline();
    void allocating_fit_cache(allocation_descriptor *descriptor);
    int obtain_cache_from_part2();
    void uniform_allocate_cache();
    allocation_descriptor *find_cache_pair_k_descriptor();
    void return_cache_to_obj_k(allocation_descriptor *descriptor, allocation_descriptor *descriptor_k);
    void return_cache_to_unpaid_objs();
    uint64_t find_attempt_fit_bitmask(uint32_t& requested_cache_way, const uint64_t& remain_cache_bitmask);
    void attempt_allocate_cache();
    void attempt_allocating_pair_cache();
    bool pair_allocating_cache();
    void allocating_cache();
    bool is_exist_cache_pair();
    void save_allocation_context(allocation_descriptor *descriptor);

    void down_search_mb_roofline(allocation_descriptor *descriptor);
    void found_mb_roofline(allocation_descriptor *descriptor);
    void up_search_mb_roofline(allocation_descriptor *descriptor);
    void allocating_fit_mb(allocation_descriptor *descriptor, int delta_mb_throttle);
    void attempt_allocating_mb(allocation_descriptor *descriptor, int delta_mb_throttle);
    void attempt_allocating_pair_mb();
    allocation_descriptor *find_mb_pair_k_descriptor();
    void return_mb_to_obj_k(allocation_descriptor *descriptor, allocation_descriptor *descriptor_k);
    void return_mb_to_unpaid_objs();
    bool pair_allocating_mb();
    void allocating_mb();
    bool binary_serch_mb_roofline();
    void pair_allocating();
    void update_monitoring_data();
    void reallocating_complete_map_after_return();
    void print_allocating_cache_results();
    void insert_mb_ready_queue();
    uint32_t obtain_mb_from_part2();
    void check_mb_leak();

    // Member variable definitions

    bool is_init_stage = true;

    uint32_t roofline_remain_cache;
    uint64_t roofline_remain_bitmask;

    struct timespec allocating_req;
    struct timespec allocating_rem;

    allocation_descriptor *allocating_cache_descriptor = nullptr;
    allocation_descriptor *allocating_mb_descriptor = nullptr;

    std::priority_queue<allocation_descriptor *, std::vector<allocation_descriptor *>, cache_allocation_less> cache_ready_queue;
    std::priority_queue<allocation_descriptor *, std::vector<allocation_descriptor *>, mb_allocation_less> mb_ready_queue;

    std::list<allocation_descriptor *> cache_completion_queue;
    std::unordered_map<int, allocation_descriptor*> allocation_complete_map;

    //A hashmap stores the allocation descriptors of all managed objects
	std::unordered_map<int, allocation_descriptor> obj_allocation_descriptors_map;

    std::vector<allocation_descriptor*> allocating_descriptor_vec;

#ifdef TEST
    experimental_data& exp_data;
#endif

};

}

#endif