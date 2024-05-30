#ifndef _MONITOR_TYPE_H_
#define _MONITOR_TYPE_H_

#include "utils.h"
#include <cstdint>

using monitor_mode = mode;

namespace PaLLOC{

struct monitor_data {
    uint64_t period_instructions = 0;
    uint64_t current_instructions = 0;
	uint64_t llc_request = 0;
	uint64_t llc_miss = 0;
    uint64_t llc_hit = 0;
	uint64_t llc_occupancy = 0;
    double llc_hit_ratio = 0.0;
	double llc_miss_ratio = 0.0;
	double current_time = 0.0;
    double mb = 0.0;
    double excution_rate = 0.0;
	double ipc = 0.0;
	double mpki = 0.0;
	double hpki = 0.0;

	inline monitor_data& operator+= (const monitor_data& other) {
		period_instructions += other.period_instructions;
		current_instructions += other.current_instructions;
		llc_request += other.llc_request;
		llc_miss += other.llc_miss;
		llc_hit += other.llc_hit;
		llc_occupancy += other.llc_occupancy;
		llc_hit_ratio = static_cast<double>(other.llc_hit)/other.llc_request;
		llc_hit_ratio = 1 - llc_hit_ratio;
		current_time = std::max(current_time, other.current_time);
		mb += other.mb;
		excution_rate += other.excution_rate;
		ipc += other.ipc;
		mpki += other.mpki;
		hpki += other.hpki;

		return (*this);
	}
};

}

#endif

