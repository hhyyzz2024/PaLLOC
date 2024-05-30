#ifndef _BACKEND_H_
#define _BACKEND_H_

#include "pqos.h"
#include "cpucounters.h"
#include "monitor_type.h"
#include "utils.h"
#include <vector>

namespace PaLLOC {

class backend {
public:
	backend() {}
	virtual ~backend() {};

	// monitor function
	virtual int setup_monitor(mode mon_mode) {return RET_OK;}
	virtual void stop_monitor() {}
	virtual void start_monitoring() = 0;
	virtual void stop_monitoring() = 0;
	virtual void parse_monitor_data(const int& id, monitor_data& current_data,
		const monitor_data& previous_data, const uint64_t& period) = 0;
	virtual double get_llc_read_miss_latency(uint32_t socket) {return 0;}

    //allocator function
	virtual int cos_association(const unsigned& core, const unsigned& class_id) = 0;
	virtual int allocating_cache(const std::vector<unsigned>& sockets, unsigned class_id, uint64_t ways_mask) = 0;
	virtual int allocating_cache(const uint32_t& socket, unsigned class_id, uint64_t ways_mask) = 0;
	virtual int allocating_mb(const std::vector<unsigned>& sockets, unsigned class_id, unsigned* mb_throttle) = 0;
	virtual int allocating_mb(const uint32_t& socket, unsigned class_id, unsigned* mb_throttle) = 0;
    virtual int print_allocating_config(const std::vector<unsigned>& sockets) = 0;
	virtual int get_cos_num() = 0;
};

class pqos_backend : public backend{
public:
	pqos_backend(const std::vector<int>& objects);
	virtual ~pqos_backend();

	// CMT & MBM monitor backend
	virtual int setup_monitor(mode mon_mode);
	virtual void stop_monitor();
	virtual void start_monitoring();
	virtual void stop_monitoring();
	virtual void parse_monitor_data(const int& group_index, monitor_data& current_data,
		const monitor_data& previous_data, const uint64_t& period);
	

	// CAT & MBA allocator backend
	virtual int cos_association(const unsigned& core, const unsigned& class_id);
	virtual int allocating_cache(const std::vector<unsigned>& sockets, unsigned class_id, uint64_t ways_mask);
	virtual int allocating_cache(const uint32_t& socket, unsigned class_id, uint64_t ways_mask);
	virtual int allocating_mb(const std::vector<unsigned>& sockets, unsigned class_id, unsigned* mb_throttle);
	virtual int allocating_mb(const uint32_t& socket, unsigned class_id, unsigned* mb_throttle);
	virtual int print_allocating_config(const std::vector<unsigned>& sockets);
	virtual int get_cos_num() {return l3cat_id_count;}
	
private:
    int reset_monitor();
    
	unsigned l3cat_id_count;
	unsigned *l3cat_ids;
	unsigned *p_mba_ids;
	unsigned mba_id_count;
	std::vector<pqos_mon_data *> mon_data_groups;
	const std::vector<int>& pqos_objects;
};

struct cpu_counter_state {
	std::vector<pcm::CoreCounterState> cstates;
    std::vector<pcm::SocketCounterState> sktstate;
    pcm::SystemCounterState sstate;
};

class pcm_pqos_backend : public backend{
public:
	pcm_pqos_backend();
	~pcm_pqos_backend();

    // pcm monitor backend
	// monitor backend
	virtual void start_monitoring();
	virtual void stop_monitoring();
	virtual void parse_monitor_data(const int& core_id, monitor_data& current_data,
		const monitor_data& previous_data, const uint64_t& period);
    virtual double get_llc_read_miss_latency(uint32_t socket);

	// CAT & MBA allocator backend
	virtual int cos_association(const unsigned& core, const unsigned& class_id);
	virtual int allocating_cache(const std::vector<unsigned>& sockets, unsigned class_id, uint64_t ways_mask);
	virtual int allocating_cache(const uint32_t& socket, unsigned class_id, uint64_t ways_mask);
	virtual int allocating_mb(const std::vector<unsigned>& sockets, unsigned class_id, unsigned* mb_throttle);
	virtual int allocating_mb(const uint32_t& socket, unsigned class_id, unsigned* mb_throttle);
	virtual int print_allocating_config(const std::vector<unsigned>& sockets);
	virtual int get_cos_num() {return l3cat_id_count;}

private:
    pcm::PCM *pcm;

	unsigned l3cat_id_count;
	unsigned *l3cat_ids;
	unsigned *p_mba_ids;
	unsigned mba_id_count;

    cpu_counter_state before_state;
    cpu_counter_state after_state;
};

}

#endif