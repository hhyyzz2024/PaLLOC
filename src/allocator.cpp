#include "allocator.h"
#include <algorithm>
#include <numa.h>

namespace PaLLOC {

allocator::allocator(const std::vector<int>& objects, mode m, backend *backend, monitor *monitor, discriminator *discriminator) : 
		objs(objects), m_mode(m), m_status(allocator_status::init), allocator_backend(backend), m_monitor(monitor), m_discriminator(discriminator)
{
    // Initialize cache information in the system
	get_llc_info(&remain_cache_size, &remain_cache_bitmask, &max_num_cache_way, &cache_size_per_way);

	remain_num_cache_way = max_num_cache_way;
	get_max_cache_bitmask();

	// To do initialize mb information in the system
	remain_mb = max_mb = MAX_MB;
}

void allocator::get_max_cache_bitmask()
{
	uint32_t cache_way = max_num_cache_way;
	max_cache_bitmask = 0;
	while(cache_way--) {
		max_cache_bitmask = (max_cache_bitmask << 1 | 0x1);
	}
}

uint64_t allocator::find_fit_bitmask(const uint32_t& requested_cache_way)
{
	uint64_t remain_bitmask = remain_cache_bitmask, bitmask = 0x0;

	int pos = 0, end_pos = max_num_cache_way - requested_cache_way;

    while(pos <= end_pos) {
        bool has_enough_bit = true;
        for(int available_way = pos; available_way < pos + requested_cache_way; available_way++) {
            has_enough_bit &= (remain_bitmask >> available_way) & 0x1;
            if(!has_enough_bit) {
                break;
            }
        }

        if(!has_enough_bit) {
            pos += 1;
        } else {
            for(int i = 0; i < requested_cache_way; i++) {
			    bitmask |= (1 << i);
		    }
            bitmask = bitmask << pos;
			break;
        }
    }

	return bitmask;   
}

void allocator::cos_association(const std::vector<int>& cpu_ids, const int& class_id)
{
	for(auto cpu : cpu_ids) {
		allocator_backend->cos_association(cpu, class_id);
	}
}

void allocator::cos_association(const int& cpu_id, const int& class_id)
{
	allocator_backend->cos_association(cpu_id, class_id);
}

}