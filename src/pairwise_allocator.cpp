#include "pairwise_allocator.h"
#include <numa.h>
#include <assert.h>
#include <limits>

#define ROOFLINE_PAIR1_CLASS_ID 13
#define ROOFLINE_PAIR2_CLASS_ID 14

static uint32_t roofline_pair1_init_cache_way;
static uint32_t roofline_pair2_init_cache_way;
static uint64_t roofline_pair1_init_cache_bitmask;
static uint64_t roofline_pair2_init_cache_bitmask;

static double ipc_threshold = 0.05;
static double mb_threshold = 1000;      // 1000 MB/s

namespace PaLLOC {
#ifdef TEST
pairwise_allocator::pairwise_allocator(const std::vector<int>& objects, mode m, backend *backend, monitor *monitor, discriminator *discriminator, experimental_data& exp_data) 
    : allocator(objects, m, backend, monitor, discriminator), exp_data(exp_data)
#else 
pairwise_allocator::pairwise_allocator(const std::vector<int>& objects, mode m, backend *backend, monitor *monitor, discriminator *discriminator)
                : allocator(objects, m, backend, monitor, discriminator)
#endif
{
    if(m_mode != mode::CORES) {
        log_error("fast_pair_allocator only support core mode!\n");
        exit(-1);
    }

    // set allocating period
    int allocating_period = 1000;
    allocating_req.tv_sec = allocating_period / 1000;
    allocating_req.tv_nsec = (allocating_period % 1000) * 1000000;

    roofline_remain_cache = remain_num_cache_way;
    roofline_remain_bitmask = remain_cache_bitmask;

    int clos_index = 1;
    uint64_t bitmask = 0x1;

/*    uint32_t cache_ways = max_num_cache_way / objects.size();

    for(uint32_t s = 0; s < cache_ways; s++) {
        bitmask = (bitmask << 1) | 0x1;
    } */

    std::vector<unsigned> sockets;

    // Create obj descriptor
	for(auto obj : objects) {
		allocation_descriptor descriptor;
		descriptor.obj_id = obj;
        descriptor.core_id = obj;
        descriptor.class_id = clos_index;
        descriptor.sockect_id = numa_node_of_cpu(descriptor.core_id);
        
        if(std::find(sockets.begin(), sockets.end(), descriptor.sockect_id) == std::end(sockets)) {
            sockets.emplace_back(descriptor.sockect_id);
        }

        descriptor.cache_ways = 1;
        descriptor.cache_bitmask = bitmask;
        descriptor.roofline_cache_ways = 1;
        descriptor.mb_throttle = MIN_MB_THROTTLE;
        bitmask <<= 1;
        allocator_backend->cos_association(descriptor.core_id, descriptor.class_id);
        allocator_backend->allocating_cache(descriptor.sockect_id, descriptor.class_id, descriptor.cache_bitmask);
        backend->allocating_mb(descriptor.sockect_id, descriptor.class_id, &descriptor.mb_throttle);
        deduct_cache_from_system(&descriptor);
		
		obj_allocation_descriptors_map.insert(std::make_pair(obj, descriptor));
        allocation_complete_map.insert(std::make_pair(obj, &obj_allocation_descriptors_map[obj]));
        allocating_descriptor_vec.emplace_back(&obj_allocation_descriptors_map[obj]);
        // to do 初始情况obj到底要不要抑制
//        backend->cos_association(obj, ROOFLINE_PAIR2_CLASS_ID);

        clos_index++;
        if(clos_index == FREE_CLOS_NUM) {
            clos_index = 1;
        }
	}

    if(sockets.size() != 1) {
        log_error("fast_pair_allocator only support all core in the same socke!\n");
        exit(-1);
    }

    // Init Roofline pair 1 COLS
    roofline_pair1_init_cache_way = roofline_remain_cache - 1;
    roofline_pair1_init_cache_bitmask = find_fit_roofline_bitmask(roofline_pair1_init_cache_way);

    backend->allocating_cache(sockets, ROOFLINE_PAIR1_CLASS_ID, roofline_pair1_init_cache_bitmask);

    unsigned mb_throttle = MAX_MB_THROTTLE;
    backend->allocating_mb(sockets, ROOFLINE_PAIR1_CLASS_ID, &mb_throttle);

    // Init roofline pair 2 CLOS
    roofline_pair2_init_cache_way = 1;
    roofline_pair2_init_cache_bitmask = find_fit_roofline_bitmask(roofline_pair2_init_cache_way);
    mb_throttle = MIN_MB_THROTTLE;
    
    backend->allocating_cache(sockets, ROOFLINE_PAIR2_CLASS_ID, roofline_pair2_init_cache_bitmask);
    deduct_roofline_remain_cache(roofline_pair2_init_cache_way, roofline_pair2_init_cache_bitmask);

    backend->allocating_mb(sockets, ROOFLINE_PAIR2_CLASS_ID, &mb_throttle);
    set_allocator_status(allocator_status::allocating_end);
}

uint64_t pairwise_allocator::find_fit_roofline_bitmask(uint32_t& requested_cache_way)
{
	uint64_t remain_bitmask = roofline_remain_bitmask, bitmask = 0x0;

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

void pairwise_allocator::reset_obj_allocation_descriptor(const int& obj_id)
{
    obj_allocation_descriptors_map[obj_id].find_cache_roofline_times = 0;
    obj_allocation_descriptors_map[obj_id].find_mb_roofline_times = 0;
    obj_allocation_descriptors_map[obj_id].pair_allocating_cache_times = 0;
    obj_allocation_descriptors_map[obj_id].pair_allocating_mb_times = 0;
    obj_allocation_descriptors_map[obj_id].cache_ways = 0;
    obj_allocation_descriptors_map[obj_id].allocating_mb = 0;
    obj_allocation_descriptors_map[obj_id].mb_throttle = 0;
    obj_allocation_descriptors_map[obj_id].find_cache_roofline = false;
    obj_allocation_descriptors_map[obj_id].find_mb_roofline = false;
    obj_allocation_descriptors_map[obj_id].pair_allocating_cache_completed = false;
    obj_allocation_descriptors_map[obj_id].pair_allocating_mb_completed = false;
}

int pairwise_allocator::insert_cache_ready_queue(const int& obj_id)
{
    allocation_descriptor *descriptor = &obj_allocation_descriptors_map[obj_id];
//    allocator_backend->allocating_cache(current_clos_descriptor.socket, current_clos_descriptor.class_id, current_clos_descriptor.cache_bitmask);

    if(allocation_complete_map.find(obj_id) != allocation_complete_map.end())
        allocation_complete_map.erase(obj_id);
    cache_ready_queue.push(descriptor);
    
    return 0;
}

void pairwise_allocator::check_cache_leak()
{
    uint32_t check_cache_way = 0;
    for(const auto& allocation_complete_pair : allocation_complete_map) {
        check_cache_way += allocation_complete_pair.second->cache_ways;
    }
    check_cache_way += remain_num_cache_way;

    if(check_cache_way != max_num_cache_way) {
        log_error("Cache resource leak occurs!, system controllable cache ways: %u, system max cache ways: %u\n", 
                    check_cache_way, max_num_cache_way);
//        exit(-1);
    }
}

void pairwise_allocator::check_mb_leak()
{
    double check_mb = 0;
    for(const auto& allocation_complete_pair : allocation_complete_map) {
        check_mb += allocation_complete_pair.second->allocating_mb;
    }
    check_mb += remain_mb;

    if(std::fabs(check_mb-max_mb) > 1e-7) {
        log_error("Cache resource leak occurs!, system controllable mb: %.4f MB/s, system max mb: %.4f MB/s\n", 
                    check_mb, max_mb);
//        exit(-1);
    }
}

void pairwise_allocator::return_cache(allocation_descriptor *descriptor)
{
    // Since the descriptor will be reallocated, other objects borrowed from its cache need to be cleared.
    for(auto& allocation_complete_pair : allocation_complete_map) {
        allocation_descriptor *other_descriptor = allocation_complete_pair.second;
        if(other_descriptor != descriptor) {
            if(other_descriptor->obj_borrowed_cache_way_map.find(other_descriptor->obj_id) != other_descriptor->obj_borrowed_cache_way_map.end())
                other_descriptor->obj_borrowed_cache_way_map.erase(other_descriptor->obj_id);
        }
    }

    // Return cache to borrowed obj
    for(const auto& obj_borrowed_cache_way : descriptor->obj_borrowed_cache_way_map) {
        int obj_id = obj_borrowed_cache_way.first;

        uint32_t return_cache_way = std::min(descriptor->cache_ways, obj_borrowed_cache_way.second);

        // obj_id is in allocation_complete_map, cache will return to allocation_complete_map
        if(allocation_complete_map.find(obj_id) != allocation_complete_map.end()) {
            allocation_complete_map[obj_id]->cache_ways += return_cache_way;
            descriptor->cache_ways -= return_cache_way;
        }
    }

    if(descriptor->cache_ways > 0) {
        return_cache_to_system(descriptor);
    }

    descriptor->obj_borrowed_cache_way_map.clear();
}

void pairwise_allocator::reallocating_complete_map_after_return()
{
    remain_num_cache_way = max_num_cache_way;
    remain_cache_bitmask = max_cache_bitmask;

    for(auto& allocation_complete_pair : allocation_complete_map) {
        allocation_descriptor *descriptor = allocation_complete_pair.second;

        descriptor->cache_bitmask = find_fit_bitmask(descriptor->cache_ways);
        deduct_cache_from_system(descriptor);

        log_info("Reallocating obj %d, cache way %u, bitmask: 0x%lx\n", descriptor->obj_id, descriptor->cache_ways, descriptor->cache_bitmask);
        allocator_backend->allocating_cache(descriptor->sockect_id, descriptor->class_id, descriptor->cache_bitmask);
        
        // TODO reallocating mb
    }
}

void pairwise_allocator::save_allocation_context(allocation_descriptor *descriptor)
{
    descriptor->context.cache_ways = descriptor->cache_ways;
    descriptor->context.cache_bitmask = descriptor->cache_bitmask;
}

void pairwise_allocator::discriminating()
{
    if(is_init_stage) {
        for(auto& allocation_complete_pair : allocation_complete_map) {
            allocation_descriptor *descriptor = allocation_complete_pair.second;
            const monitor_data& mon_data = m_monitor->get_current_data(descriptor->obj_id);
            descriptor->cache_roofline_perf.ipc = mon_data.ipc;
            descriptor->cache_roofline_perf.hpki = mon_data.hpki;
            log_critical("obj: %d, roofline ipc: %.4f\n", descriptor->obj_id, descriptor->cache_roofline_perf.ipc);
        }
        is_init_stage = false;
    }

	// discriminating
	std::vector<int> reallocate_obj_ids;
	if(m_discriminator->get_objs_need_reallocate_resource(reallocate_obj_ids)) {
#ifdef TEST
        exp_data.call_allocating_num = 0;
        exp_data.find_roofline_num = 0;
        exp_data.allocating_num = 0;
        exp_data.allocating_request_times++;
        exp_data.attempt_allocating_start = cpu_second();
#endif
        check_cache_leak();
        check_mb_leak();
        for(const auto& obj_id : reallocate_obj_ids) {
            allocation_descriptor *descriptor = &obj_allocation_descriptors_map[obj_id];
            
            save_allocation_context(descriptor);

            // return cache
            if(descriptor->obj_borrowed_cache_way_map.size() > 0) {
                return_cache(descriptor);
            } else {
                return_cache_to_system(descriptor);
            }
            
            //return mb
            return_mb_to_system(descriptor);

            reset_obj_allocation_descriptor(obj_id);
            obj_allocation_descriptors_map[obj_id].phase_change = true;
            insert_cache_ready_queue(obj_id);
        }

        set_allocator_status(allocator_status::init);
	}
}

void pairwise_allocator::update_monitoring_data()
{
    for(auto& obj_descriptor_pair : obj_allocation_descriptors_map) {
        const monitor_data& mon_data = m_monitor->get_current_data(obj_descriptor_pair.first);
        update_descriptor_data(&obj_descriptor_pair.second, m_monitor->get_current_data(obj_descriptor_pair.first));
    }
}

void pairwise_allocator::move_other_objs_to_pair2_clos(allocation_descriptor *descriptor)
{
    for(auto& obj_descriptor_pair : obj_allocation_descriptors_map) {
        allocation_descriptor *other_descriptor = &obj_descriptor_pair.second;
        if(other_descriptor != descriptor)
            allocator_backend->cos_association(other_descriptor->obj_id, ROOFLINE_PAIR2_CLASS_ID);
    }
}

void pairwise_allocator::move_all_objs_to_their_own_clos()
{
    for(auto& obj_descriptor_pair : obj_allocation_descriptors_map) {
        allocation_descriptor *descriptor = &obj_descriptor_pair.second;       
        allocator_backend->cos_association(descriptor->obj_id, obj_allocation_descriptors_map[descriptor->obj_id].class_id);
    }
}

void pairwise_allocator::down_search_cache_roofline(allocation_descriptor *descriptor)
{
    descriptor->right_cache_ways = descriptor->cache_ways;
    return_roofline_remain_cache(descriptor);
    descriptor->cache_ways = (descriptor->right_cache_ways + descriptor->left_cache_ways) / 2;
    descriptor->cache_bitmask = find_fit_roofline_bitmask(descriptor->cache_ways);
    deduct_roofline_remain_cache(descriptor);
    allocator_backend->allocating_cache(descriptor->sockect_id, ROOFLINE_PAIR1_CLASS_ID, descriptor->cache_bitmask);
}

void pairwise_allocator::up_search_cache_roofline(allocation_descriptor *descriptor)
{
    descriptor->left_cache_ways = descriptor->cache_ways+1;            
    return_roofline_remain_cache(descriptor);
    descriptor->cache_ways = (descriptor->right_cache_ways + descriptor->left_cache_ways) / 2;
    descriptor->cache_bitmask = find_fit_roofline_bitmask(descriptor->cache_ways);
    deduct_roofline_remain_cache(descriptor);
    allocator_backend->allocating_cache(descriptor->sockect_id, ROOFLINE_PAIR1_CLASS_ID, descriptor->cache_bitmask);
}

void pairwise_allocator::found_cache_roofline(allocation_descriptor *descriptor)
{
    descriptor->find_cache_roofline = true;
    descriptor->roofline_cache_ways = descriptor->cache_ways;
    return_roofline_remain_cache(descriptor);

    move_all_objs_to_their_own_clos();

    log_info("Found_cache_roofline, obj_id %d, roofline cache way: %u, roofline remain cache: %u, roofline cache bitmask: 0x%lx\n", 
        descriptor->obj_id, descriptor->roofline_cache_ways, roofline_remain_cache, roofline_remain_bitmask);
}

bool pairwise_allocator::binary_serch_cache_roofline()
{
    allocation_descriptor *descriptor = allocating_cache_descriptor;

    if(descriptor->find_cache_roofline_times == 0) {
        descriptor->cache_ways = roofline_pair1_init_cache_way;
        descriptor->cache_bitmask = roofline_pair1_init_cache_bitmask;
        descriptor->left_cache_ways = 1;
        allocator_backend->cos_association(descriptor->obj_id, ROOFLINE_PAIR1_CLASS_ID);
        allocator_backend->allocating_cache(descriptor->sockect_id, ROOFLINE_PAIR1_CLASS_ID, descriptor->cache_bitmask);
        deduct_roofline_remain_cache(descriptor);

        //Move other objs to resource suppression area
        move_other_objs_to_pair2_clos(descriptor);
    } else if(descriptor->find_cache_roofline_times == 1) {
        // Get roofline performance of descriptor1
        update_cache_roofline_perf(descriptor);
        down_search_cache_roofline(descriptor);
    } else {
//        double delta_hpki = descriptor->current_data.hpki - descriptor->cache_roofline_perf.hpki;
        double delta_ipc = descriptor->current_data.ipc - descriptor->cache_roofline_perf.ipc;
        
        if(delta_ipc >= 0) {
            // current_data.hpki > roofline.hpki, need to update roofline performance of descriptor1
            update_cache_roofline_perf(descriptor);

            if(descriptor->right_cache_ways == descriptor->left_cache_ways) {
                found_cache_roofline(descriptor);
            } else {
                down_search_cache_roofline(descriptor);
            }
        } else if(std::fabs(delta_ipc) / descriptor->cache_roofline_perf.ipc <= ipc_threshold) {
            // current hpki == roofline hpki, binary decrease current cache way
            if(descriptor->left_cache_ways < descriptor->right_cache_ways) {
                down_search_cache_roofline(descriptor);
            } else {
                found_cache_roofline(descriptor);
            }
        } else {
            // current hpki < roofline hpki, binary increase current cache way
            if(descriptor->left_cache_ways < descriptor->right_cache_ways) {
                up_search_cache_roofline(descriptor);
            } else {
                found_cache_roofline(descriptor);
            } 
        }
    }

    if(descriptor->find_cache_roofline_times > 0) {
//        log_info("descriptor COLS %u, find_cache_roofline_times: %u, roofline hpki: %.4f, current hpki %.4f, gap: %.4f, lway: %u, rway: %u\n", descriptor->class_id, descriptor->find_cache_roofline_times,               
//                descriptor->cache_roofline_perf.hpki, descriptor->current_data.hpki, std::fabs(descriptor->cache_roofline_perf.hpki - descriptor->current_data.hpki)/descriptor->cache_roofline_perf.hpki,
//                descriptor->left_cache_ways, descriptor->right_cache_ways);

        log_info("descriptor COLS %u, find_cache_roofline_times: %u, roofline ipc: %.4f, current ipc %.4f, gap: %.4f, lway: %u, rway: %u\n", descriptor->class_id, descriptor->find_cache_roofline_times,               
                descriptor->cache_roofline_perf.ipc, descriptor->current_data.ipc, std::fabs(descriptor->cache_roofline_perf.ipc - descriptor->current_data.ipc)/descriptor->cache_roofline_perf.ipc,
                descriptor->left_cache_ways, descriptor->right_cache_ways);
    }

#ifdef TEST
    exp_data.find_roofline_num++;
#endif
    
    descriptor->find_cache_roofline_times++;
    return descriptor->find_cache_roofline;
}

void pairwise_allocator::allocating_fit_cache(allocation_descriptor *descriptor)
{
    descriptor->cache_bitmask = find_fit_bitmask(descriptor->cache_ways);
    if(descriptor->cache_bitmask) {
        deduct_cache_from_system(descriptor);
        allocator_backend->allocating_cache(descriptor->sockect_id, descriptor->class_id, descriptor->cache_bitmask);
    } else {
        log_critical("Cannot find fit bitmask, need uniform allocate cache!\n");
        uniform_allocate_cache();
    }
    descriptor->pair_allocating_cache_completed = true;
}

int pairwise_allocator::obtain_cache_from_part2()
{
    int obtain_cache = 0;
    log_info("obtain_cache_from_part2: \n");
    for(auto& allocation_complete_pair : allocation_complete_map) {
        allocation_descriptor *descriptor = allocation_complete_pair.second;
        
        if(descriptor->cache_ways > 1 && descriptor->p_status == payment_status::init) {
            descriptor->cache_ways -= 1;
            descriptor->p_status = payment_status::update_ipc;
            obtain_cache = 1;
        }

        log_info("obj: %d, cache_way: %d\n", descriptor->obj_id, descriptor->cache_ways);
    }

    for(auto it = cache_completion_queue.begin(); it != cache_completion_queue.end(); it++) {
        allocation_descriptor *descriptor = (*it);
        
        if(descriptor->cache_ways > 1 && descriptor->p_status == payment_status::init) {
            descriptor->cache_ways -= 1;
            descriptor->p_status = payment_status::update_ipc;
            obtain_cache = 1;
        }
        log_info("obj: %d, cache_way: %d\n", descriptor->obj_id, descriptor->cache_ways);
    }
    return obtain_cache;
}

uint64_t pairwise_allocator::find_attempt_fit_bitmask(uint32_t& requested_cache_way, const uint64_t& remain_cache_bitmask)
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

void pairwise_allocator::attempt_allocate_cache()
{
    uint64_t cache_bitmask = max_cache_bitmask;

    for(int i = 0; i < allocating_descriptor_vec.size(); i++) {
        allocation_descriptor *descriptor = allocating_descriptor_vec[i];
        if(descriptor->cache_ways) {
            descriptor->cache_bitmask = find_attempt_fit_bitmask(descriptor->cache_ways, cache_bitmask);
            log_critical("attempt_allocate_cache cache_bitmask: 0x%lx\n", descriptor->cache_bitmask);
            cache_bitmask -= descriptor->cache_bitmask;
            allocator_backend->allocating_cache(descriptor->sockect_id, descriptor->class_id, descriptor->cache_bitmask);
        }
    }
}

void pairwise_allocator::uniform_allocate_cache()
{
    uint32_t check_cache_way = 0;
    remain_num_cache_way = max_num_cache_way;
    remain_cache_bitmask = max_cache_bitmask;

    for(int i = 0; i < allocating_descriptor_vec.size(); i++) {
        allocation_descriptor *descriptor = allocating_descriptor_vec[i];
        if(descriptor->cache_ways) {
            check_cache_way += descriptor->cache_ways;
            descriptor->cache_bitmask = find_fit_bitmask(descriptor->cache_ways);
            log_critical("uniform_allocate_cache cache_bitmask: 0x%lx\n", descriptor->cache_bitmask);
            deduct_cache_from_system(descriptor);
            allocator_backend->allocating_cache(descriptor->sockect_id, descriptor->class_id, descriptor->cache_bitmask);
        }
    }

    log_info("End uniform_allocate_cache, check_cache_way: %u, max_num_cache_way: %u\n", check_cache_way+remain_num_cache_way, max_num_cache_way);
}

pairwise_allocator::allocation_descriptor *pairwise_allocator::find_cache_pair_k_descriptor()
{
    double delta = std::numeric_limits<double>::max();
    allocation_descriptor *payment_descriptor = nullptr;
    for(auto& allocation_complete_pair : allocation_complete_map) {
        allocation_descriptor *descriptor = allocation_complete_pair.second;

        if(descriptor->p_status == payment_status::init)
            continue;

        if(descriptor->p_status == payment_status::update_ipc) {
//            descriptor->delta = std::fabs(descriptor->current_data.ipc - descriptor->prev_data.ipc) / descriptor->cache_roofline_perf.ipc;
            descriptor->delta = (descriptor->prev_data.ipc - descriptor->current_data.ipc) / descriptor->cache_roofline_perf.ipc;
           
            if(descriptor->delta < 0) {
                descriptor->delta = 0;
            }
            descriptor->p_status = payment_status::unpaid;
        }

        if(descriptor->delta < delta) {
            delta = descriptor->delta;
            payment_descriptor = descriptor;
        }

        log_critical("Core %d, roofline delta ipc: %.4f\n", descriptor->core_id, descriptor->delta);
    }

    for(auto it = cache_completion_queue.begin(); it != cache_completion_queue.end(); it++) {
        allocation_descriptor *descriptor = (*it);

        if(descriptor->p_status == payment_status::init)
            continue;

        if(descriptor->p_status == payment_status::update_ipc) {
//            descriptor->delta = std::fabs(descriptor->current_data.ipc - descriptor->prev_data.ipc) / descriptor->cache_roofline_perf.ipc;
            descriptor->delta = (descriptor->prev_data.ipc - descriptor->current_data.ipc) / descriptor->cache_roofline_perf.ipc;
            if(descriptor->delta < 0) {
                descriptor->delta = 0;
            }
            descriptor->p_status = payment_status::unpaid;
        }

        if(descriptor->delta < delta) {
            delta = descriptor->delta;
            payment_descriptor = descriptor;
        }

        log_critical("Core %d, roofline delta ipc: %.4f\n", descriptor->core_id, descriptor->delta);
    }

    if(payment_descriptor) {
        payment_descriptor->p_status = payment_status::init;

        if(allocating_cache_descriptor->obj_borrowed_cache_way_map.find(payment_descriptor->obj_id) == allocating_cache_descriptor->obj_borrowed_cache_way_map.end()) {
            allocating_cache_descriptor->obj_borrowed_cache_way_map.insert({payment_descriptor->obj_id, 1});
        } else {
            allocating_cache_descriptor->obj_borrowed_cache_way_map[payment_descriptor->obj_id]++;
        }
    }
    return payment_descriptor;
}

void pairwise_allocator::return_cache_to_obj_k(allocation_descriptor *descriptor, allocation_descriptor *descriptor_k)
{
    descriptor->cache_ways -= 1;
    descriptor->obj_borrowed_cache_way_map[descriptor_k->obj_id] -= 1;
    if(descriptor->obj_borrowed_cache_way_map[descriptor_k->obj_id] == 0) {
        descriptor->obj_borrowed_cache_way_map.erase(descriptor_k->obj_id);
    }
    descriptor_k->cache_ways += 1;
}

void pairwise_allocator::return_cache_to_unpaid_objs()
{
    for(auto& allocation_complete_pair : allocation_complete_map) {
        allocation_descriptor *descriptor = allocation_complete_pair.second;
        if(descriptor->p_status == payment_status::unpaid) {
            descriptor->p_status = payment_status::init;
            descriptor->cache_ways += 1;
        }
    }
    for(auto it = cache_completion_queue.begin(); it != cache_completion_queue.end(); it++) {
        allocation_descriptor *descriptor = (*it);
        if(descriptor->p_status == payment_status::unpaid) {
            descriptor->p_status = payment_status::init;         
            descriptor->cache_ways += 1;
        }
    }
}

void pairwise_allocator::attempt_allocating_pair_cache()
{
    // descriptor1 is the app j in paper, and descriptor2 is the app k in paper
    allocation_descriptor *descriptor1 = allocating_cache_descriptor;
    allocation_descriptor *descriptor2;
    if(descriptor1->pair_allocating_cache_times == 0) {
        if(descriptor1->cache_ways == 0) {
            descriptor1->cache_ways = obtain_cache_from_part2();
        }
    
        uniform_allocate_cache();
               
    } else {
        // find fit k payment cache to descriptor1
        descriptor2 = find_cache_pair_k_descriptor();
        if(descriptor1->pair_allocating_cache_times > 1 && descriptor2) {

//            double delta1 = std::fabs(descriptor1->current_data.ipc - descriptor1->prev_data.ipc) / descriptor1->cache_roofline_perf.ipc;
            double delta1 = (descriptor1->current_data.ipc - descriptor1->prev_data.ipc) / descriptor1->cache_roofline_perf.ipc;
            
            if(delta1 < 0) {
                delta1 = 0;
            }

            double delta2 = descriptor2->delta;
            double benifit = delta1 - delta2;

            log_critical("Find core %d pair core %d, descriptor 1 gain ipc: %.4f, descriptor 2 loss ipc: %.4f, benifit: %.4f\n", 
                descriptor1->core_id, descriptor2->core_id, delta1, delta2, benifit);

            if(benifit < 0) {
                // 借出后损失更多

                // 归还cache给已借出的k
                return_cache_to_obj_k(descriptor1, descriptor2);
                
                // 归还cache给未借出
                return_cache_to_unpaid_objs();
                
                uniform_allocate_cache();
                descriptor1->pair_allocating_cache_completed = true;
            }
        }
        
        // 如果该descriptor未完成
        if(!descriptor1->pair_allocating_cache_completed) {
            if(descriptor1->cache_ways == descriptor1->roofline_cache_ways) {
                // 归还cache给未借出
                return_cache_to_unpaid_objs();
                uniform_allocate_cache();
                descriptor1->pair_allocating_cache_completed = true;
            } else {
                int cache_way = obtain_cache_from_part2();
                if(cache_way != 0) {
                    descriptor1->cache_ways += cache_way;
                    attempt_allocate_cache();
                } else {
                    descriptor1->pair_allocating_cache_completed = true;
                    // 归还cache给未借出
                    return_cache_to_unpaid_objs();
                    uniform_allocate_cache();
                }
            }            
        }   
    }
}

bool pairwise_allocator::is_exist_cache_pair()
{
    bool is_exist_pair = false;
    for(auto& allocation_complete_pair : allocation_complete_map) {
        allocation_descriptor *descriptor = allocation_complete_pair.second;
        if(descriptor->cache_ways > 1) {
            is_exist_pair = true;
        }
    }

    for(auto it = cache_completion_queue.begin(); it != cache_completion_queue.end(); it++) {
        allocation_descriptor *descriptor = (*it);

        if(descriptor->cache_ways > 1) {
            is_exist_pair = true;
        }
    }

    return is_exist_pair;
}

bool pairwise_allocator::pair_allocating_cache()
{
    allocation_descriptor *descriptor = allocating_cache_descriptor;

    log_info("pair_allocating_cache begin, allocating time: %d, descriptor roofline cache way: %d, system remain cache way: %d, complete queue size: %ld\n", 
        descriptor->pair_allocating_cache_times, descriptor->roofline_cache_ways, remain_num_cache_way, allocation_complete_map.size()+cache_completion_queue.size());

    if(descriptor->pair_allocating_cache_times == 0) {
        if(!is_exist_cache_pair()) {
            descriptor->cache_ways = descriptor->roofline_cache_ways < remain_num_cache_way ? descriptor->roofline_cache_ways : remain_num_cache_way;

            //如果和归还前cache way一样，那就不需要重分配。
            if(descriptor->cache_ways == descriptor->context.cache_ways) {
                descriptor->cache_bitmask = descriptor->context.cache_bitmask;
                deduct_cache_from_system(descriptor);
                log_critical("There is not exist cache pair, no need to reallocate cache\n");
            } else {
                //如果和归还前的不一样，就需要重新分配。
                allocating_fit_cache(descriptor);
            }
            descriptor->pair_allocating_cache_completed = true;
        } else {
            if(remain_num_cache_way > 0) {
                descriptor->cache_ways = descriptor->roofline_cache_ways < remain_num_cache_way ? descriptor->roofline_cache_ways : remain_num_cache_way;
            } else {
                descriptor->cache_ways = 0;
            }

            if(descriptor->cache_ways == descriptor->roofline_cache_ways) {
                allocating_fit_cache(descriptor);
            } else {
                attempt_allocating_pair_cache();
            }
        }
    } else {
        attempt_allocating_pair_cache();
    }

    descriptor->pair_allocating_cache_times++;

#ifdef TEST
    exp_data.allocating_num++;
#endif

    return descriptor->pair_allocating_cache_completed;
}

void pairwise_allocator::print_allocating_cache_results()
{
    log_critical("print_allocating_cache_results: system remain cache: %u\n", remain_num_cache_way);
    for(auto& allocation_complete_pair : allocation_complete_map) {
        allocation_descriptor *descriptor = allocation_complete_pair.second;
        log_critical("obj: %d, cache_way: %d, roofline cache way: %d\n", descriptor->obj_id, descriptor->cache_ways, descriptor->roofline_cache_ways);
    }

    for(auto it = cache_completion_queue.begin(); it != cache_completion_queue.end(); it++) {
        allocation_descriptor *descriptor = (*it);
        log_critical("obj: %d, cache_way: %d, roofline cache way: %d\n", descriptor->obj_id, descriptor->cache_ways, descriptor->roofline_cache_ways);
    }
}

void pairwise_allocator::insert_mb_ready_queue()
{
    while(!cache_completion_queue.empty()) {
        allocation_descriptor *descriptor = cache_completion_queue.front();
        cache_completion_queue.pop_front();
        mb_ready_queue.push(descriptor);
    }    
}

void pairwise_allocator::allocating_cache()
{
    if(m_status == allocator_status::get_cache_descriptor) {
        get_cache_allocating_descriptor();
        set_allocator_status(allocator_status::find_first_pair_cache_roofline);
    }

    if(m_status == allocator_status::find_first_pair_cache_roofline) {
        if(binary_serch_cache_roofline()) {
            set_allocator_status(allocator_status::pair_allocating_cache);
            log_info("binary_serch_cache_roofline first pair success\n");
        }
    }

    if(m_status == allocator_status::pair_allocating_cache) {
        if(pair_allocating_cache()) {
            log_info("Complete pair_allocating_cache, allocate cache ways: %u\n", allocating_cache_descriptor->cache_ways);
            cache_completion_queue.push_back(allocating_cache_descriptor);
            cache_ready_queue.pop();
            if(cache_ready_queue.empty()) {
                print_allocating_cache_results();
                insert_mb_ready_queue();
                set_allocator_status(allocator_status::get_mb_descriptor);
            } else {
                set_allocator_status(allocator_status::get_cache_descriptor);
            }
        }
    }
}

void pairwise_allocator::down_search_mb_roofline(allocation_descriptor *descriptor)
{
    descriptor->right_mb_throttle = descriptor->roofline_mb_throttle;
    descriptor->roofline_mb_throttle = rounddown((descriptor->right_mb_throttle + descriptor->left_mb_throttle) / 2, MIN_DELTA_THROTTLE);
    allocator_backend->allocating_mb(descriptor->sockect_id, ROOFLINE_PAIR1_CLASS_ID, &descriptor->roofline_mb_throttle);
}

void pairwise_allocator::up_search_mb_roofline(allocation_descriptor *descriptor)
{
    descriptor->left_mb_throttle = descriptor->roofline_mb_throttle+MIN_DELTA_THROTTLE;            
    descriptor->roofline_mb_throttle = (descriptor->right_mb_throttle + descriptor->left_mb_throttle) / 2;
    allocator_backend->allocating_mb(descriptor->sockect_id, ROOFLINE_PAIR1_CLASS_ID, &descriptor->roofline_mb_throttle);
}

void pairwise_allocator::found_mb_roofline(allocation_descriptor *descriptor)
{
    descriptor->find_mb_roofline = true;
    
    move_all_objs_to_their_own_clos();

    log_info("Fast found mb roofline, obj_id %d, roofline mb: %.2f MB/s, roofline mb throttle: %u\n", 
        descriptor->obj_id, descriptor->mb_roofline_perf.mb, descriptor->roofline_mb_throttle);
}

bool pairwise_allocator::binary_serch_mb_roofline()
{
   allocation_descriptor *descriptor = allocating_mb_descriptor;

    if(descriptor->find_mb_roofline_times == 0) {
        descriptor->roofline_mb_throttle = MAX_MB_THROTTLE;
        descriptor->left_mb_throttle = MIN_MB_THROTTLE;
        allocator_backend->cos_association(descriptor->obj_id, ROOFLINE_PAIR1_CLASS_ID);
        allocator_backend->allocating_mb(descriptor->sockect_id, ROOFLINE_PAIR1_CLASS_ID, &descriptor->roofline_mb_throttle);

        //Move other obj to resource suppression area
        move_other_objs_to_pair2_clos(descriptor);
    } else if(descriptor->find_mb_roofline_times == 1) {
        // Get roofline performance of descriptor1
        update_mb_roofline_perf(descriptor);
        down_search_mb_roofline(descriptor);
    } else {
        double delta_ipc = descriptor->current_data.ipc - descriptor->mb_roofline_perf.ipc;
        
        if(delta_ipc >= 0) {
            // current_data.mb > roofline.mb, need to update roofline performance of descriptor1
            update_mb_roofline_perf(descriptor);

            if(descriptor->right_mb_throttle == descriptor->left_mb_throttle) {
                found_mb_roofline(descriptor);
            } else {
                down_search_mb_roofline(descriptor);
            }
        } else if(std::fabs(delta_ipc) / descriptor->mb_roofline_perf.ipc < ipc_threshold) {
            // current mb == roofline mb, binary decrease current mb throttle
            if(descriptor->left_mb_throttle < descriptor->right_mb_throttle) {
                down_search_mb_roofline(descriptor);
            } else {
                found_mb_roofline(descriptor);
            }
        } else {
            // current mb < roofline mb, binary increase current mb throttle
            if(descriptor->left_mb_throttle < descriptor->right_mb_throttle) {
                up_search_mb_roofline(descriptor);
            } else {
                found_mb_roofline(descriptor);
            } 
        }
    }

    if(descriptor->find_mb_roofline_times > 0) {
        log_info("descriptor1 COLS %u, find_mb_roofline_time: %u, roofline mb: %.4f, current mb %.4f, gap: %.4f, lmb_throttle: %u, rmb_throttle: %u\n", descriptor->class_id, descriptor->find_mb_roofline_times,               
                descriptor->mb_roofline_perf.mb, descriptor->current_data.mb, std::fabs(descriptor->mb_roofline_perf.mb - descriptor->current_data.mb)/descriptor->mb_roofline_perf.mb,
                descriptor->left_mb_throttle, descriptor->right_mb_throttle);
    }

#ifdef TEST
    exp_data.find_roofline_num++;
#endif
    
    descriptor->find_mb_roofline_times++;
    return descriptor->find_mb_roofline;
}

void pairwise_allocator::allocating_fit_mb(allocation_descriptor *descriptor, int delta_mb_throttle)
{
    descriptor->mb_throttle += delta_mb_throttle;
    log_info("Will allocating_fit_mb mb throttle: %u\n", descriptor->mb_throttle);
    allocator_backend->allocating_mb(descriptor->sockect_id, descriptor->class_id, &descriptor->mb_throttle);
    descriptor->pair_allocating_mb_completed = true;
}

void pairwise_allocator::attempt_allocating_mb(allocation_descriptor *descriptor, int delta_mb_throttle)
{
    descriptor->mb_throttle += delta_mb_throttle;
    allocator_backend->allocating_mb(descriptor->sockect_id, descriptor->class_id, &descriptor->mb_throttle);
}

pairwise_allocator::allocation_descriptor *pairwise_allocator::find_mb_pair_k_descriptor()
{
    double delta = std::numeric_limits<double>::max();
    allocation_descriptor *payment_descriptor = nullptr;
    for(auto& allocation_complete_pair : allocation_complete_map) {
        allocation_descriptor *descriptor = allocation_complete_pair.second;
        
        if(descriptor->p_status == payment_status::init)
            continue;

        if(descriptor->p_status == payment_status::update_ipc) {
//            descriptor->delta = std::fabs(descriptor->current_data.ipc - descriptor->prev_data.ipc) / descriptor->mb_roofline_perf.ipc;
            descriptor->delta = (descriptor->prev_data.ipc - descriptor->current_data.ipc) / descriptor->mb_roofline_perf.ipc;
            if(descriptor->delta < 0) {
                descriptor->delta = 0;
            }
            descriptor->p_status = payment_status::unpaid;
        }

        if(descriptor->delta < delta) {
            delta = descriptor->delta;
            payment_descriptor = descriptor;
        }
    }

    if(payment_descriptor) {
        payment_descriptor->p_status = payment_status::init;
    }
    return payment_descriptor;
}

void pairwise_allocator::return_mb_to_obj_k(allocation_descriptor *descriptor, allocation_descriptor *descriptor_k)
{
    descriptor->mb_throttle -= MIN_DELTA_THROTTLE;
    descriptor_k->mb_throttle += MIN_DELTA_THROTTLE;
    allocator_backend->allocating_mb(descriptor_k->sockect_id, descriptor_k->class_id, &descriptor_k->mb_throttle);
}

void pairwise_allocator::return_mb_to_unpaid_objs()
{
    for(auto& allocation_complete_pair : allocation_complete_map) {
        allocation_descriptor *descriptor = allocation_complete_pair.second;
        if(descriptor->p_status == payment_status::unpaid) {
            descriptor->p_status = payment_status::init;
            descriptor->mb_throttle += MIN_DELTA_THROTTLE;
            allocator_backend->allocating_mb(descriptor->sockect_id, descriptor->class_id, &descriptor->mb_throttle);
        }
    }
}

uint32_t pairwise_allocator::obtain_mb_from_part2()
{
    int obtain_mb_throttle = 0;

    for(auto& allocation_complete_pair : allocation_complete_map) {
        allocation_descriptor *descriptor = allocation_complete_pair.second;
        
        if(descriptor->mb_throttle > MIN_MB_THROTTLE && descriptor->p_status == payment_status::init) {       
            descriptor->mb_throttle -= MIN_DELTA_THROTTLE;
            descriptor->p_status = payment_status::update_ipc;
            allocator_backend->allocating_mb(descriptor->sockect_id, descriptor->class_id, &descriptor->mb_throttle);
            obtain_mb_throttle = MIN_DELTA_THROTTLE;
        }
    }

    return obtain_mb_throttle;
}

void pairwise_allocator::attempt_allocating_pair_mb()
{
    // descriptor1 is the app j in paper, and descriptor2 is the app k in paper
    allocation_descriptor *descriptor1 = allocating_mb_descriptor;
    allocation_descriptor *descriptor2;

    descriptor2 = find_mb_pair_k_descriptor();
    if(descriptor1->pair_allocating_mb_times > 1 && descriptor2) {
        // find fit k payment MB to descriptor1
        double delta1 = (descriptor1->current_data.ipc - descriptor1->prev_data.ipc) / descriptor1->mb_roofline_perf.ipc;
        if(delta1 < 0) {
            delta1 = 0;
        }
        double delta2 = descriptor2->delta;
        double benifit = delta1 - delta2;

        log_critical("Find core %d mb pair core %d, descriptor 1 gain ipc: %.4f, descriptor 2 loss ipc: %.4f, benifit: %.4f\n", 
            descriptor1->core_id, descriptor2->core_id, delta1, delta2, benifit);

        if(benifit < 0) {
            //归还MB给已借出的k
            return_mb_to_obj_k(descriptor1, descriptor2);
            //归还MB给未借出
            return_mb_to_unpaid_objs();
            allocating_fit_mb(descriptor1, -MIN_DELTA_THROTTLE);
            descriptor1->pair_allocating_mb_completed = true;
        }
    }

    if(!descriptor1->pair_allocating_mb_completed) {
        if(descriptor1->mb_throttle == descriptor1->roofline_mb_throttle) {
            descriptor1->pair_allocating_mb_completed = true;
            return_mb_to_unpaid_objs();
        } else {
            // Get from pair
            uint32_t mb_throttle = obtain_mb_from_part2();
            if(mb_throttle != 0) {
                attempt_allocating_mb(descriptor1, mb_throttle);
            } else {
                // Unable to obtain 
                return_mb_to_unpaid_objs();
                descriptor1->pair_allocating_mb_completed = true;
            }
        }            
    }
}

bool pairwise_allocator::pair_allocating_mb()
{
    allocation_descriptor *descriptor = allocating_mb_descriptor;

    log_info("pair_allocating_mb begin, allocating times: %d, descriptor roofline mb throttle: %d, system remain mb: %.2f MB/s\n", 
            descriptor->pair_allocating_mb_times, descriptor->roofline_mb_throttle, remain_mb);
    
    if(descriptor->pair_allocating_mb_times == 0) {
        descriptor->mb_throttle = MIN_MB_THROTTLE;
        attempt_allocating_mb(descriptor, 0);
    } else {
        if(remain_mb > mb_threshold) {
            if(descriptor->mb_throttle < descriptor->roofline_mb_throttle)
                attempt_allocating_mb(descriptor, MIN_DELTA_THROTTLE);
            else
                allocating_fit_mb(descriptor, 0);
        } else {
            attempt_allocating_pair_mb();
        }
    }

    if(!descriptor->pair_allocating_mb_completed) {
        descriptor->pair_allocating_mb_times++;
    }

#ifdef TEST
    exp_data.allocating_num++;
#endif

    return descriptor->pair_allocating_mb_completed;   
}

void pairwise_allocator::allocating_mb()
{
    if(m_status == allocator_status::updating_mb) {
        updating_allocating_mb();
        deduct_mb_from_system(allocating_mb_descriptor);
        log_info("Allocate mb: %.2f MB/s, mb_throttle: %u\n", allocating_mb_descriptor->allocating_mb, allocating_mb_descriptor->mb_throttle);
        allocation_complete_map.insert({allocating_mb_descriptor->obj_id, allocating_mb_descriptor});
        mb_ready_queue.pop();
        if(mb_ready_queue.empty()) {
            set_allocator_status(allocator_status::allocating_end);
            m_discriminator->turn_on_discriminator();
#ifdef TEST
            exp_data.attempt_allocating_exp_data_vec.emplace_back(attempt_allocating_data{cpu_second() - exp_data.attempt_allocating_start, exp_data.call_allocating_num,
                exp_data.find_roofline_num, exp_data.allocating_num});
#endif
        } else {
            set_allocator_status(allocator_status::get_mb_descriptor);
        }
    }

    if(m_status == allocator_status::get_mb_descriptor) {
        get_mb_allocating_descriptor();
        set_allocator_status(allocator_status::find_first_pair_mb_roofline);
    }

    if(m_status == allocator_status::find_first_pair_mb_roofline) {
        if(binary_serch_mb_roofline()) {
            set_allocator_status(allocator_status::pair_allocating_mb);
            log_info("binary_serch_mb_roofline first pair success\n");
        }
    }

    if(m_status == allocator_status::pair_allocating_mb) {
        if(pair_allocating_mb()) {
            set_allocator_status(allocator_status::updating_mb);
        }
    }    
}

void pairwise_allocator::allocating()
{
//    if(m_status == allocator_status::allocating_end)
//        return;
    while(m_status != allocator_status::allocating_end) {
#ifdef TEST
        exp_data.call_allocating_num++;
        exp_data.algorithm_start = cpu_second();
#endif

        // full allocating cache
        if(m_status == allocator_status::init) {
            // disable all discriminators
            m_discriminator->turn_off_discriminator();
            
            set_allocator_status(allocator_status::get_cache_descriptor);
            log_info("Will begin allocating cache, allocation_ready_queue size %ld, allocation_complete_queue size %ld\n", 
                    cache_ready_queue.size(), allocation_complete_map.size());
        }

        if(m_status >= allocator_status::get_cache_descriptor && m_status <= allocator_status::pair_allocating_cache) {
            // update monitoring data
            update_monitoring_data();

            // allocating cache
            allocating_cache();
        }

        if(m_status >= allocator_status::find_first_pair_cache_roofline && m_status <= allocator_status::updating_mb) {
            // update monitoring data
            update_monitoring_data();

            // allocating memory bandwidth
            allocating_mb();
        }

#ifdef TEST
        exp_data.algorithm_overhead_vec.emplace_back(cpu_second() - exp_data.algorithm_start);
#endif

        // monitoring
        m_monitor->start_monitoring();
        nanosleep(&allocating_req, &allocating_rem);
        m_monitor->stop_monitoring();

        m_monitor->parse_monitor_data(m_discriminator->get_window_data_map());
    }
}

}