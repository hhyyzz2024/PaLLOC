#ifndef _TEST_ALLOCATOR_H_
#define _TEST_ALLOCATOR_H_

#include "allocator.h"
#include <vector>
#include <numa.h>

namespace PaLLOC {

class test_allocator : public allocator {
public:
    test_allocator(const std::vector<int>& objects, mode m, backend *backend, monitor *monitor, discriminator *discriminator)
        : allocator(objects, m, backend, monitor, discriminator)
    {
        uint32_t clos_id = 1;
        uint64_t bitmask = 0;

        std::vector<uint64_t> bitmask_vec{0xf, 0xf0, 0x100, 0x200, 0x400, 0x800};

        uint32_t mb_throttle = MAX_MB_THROTTLE;
        int i = 0;
        for(const auto& core_id : objects) {
            bitmask = bitmask_vec[i++];
	        log_info("obj: %d, bitmask: 0x%lx\n", core_id, bitmask);
            const uint32_t sock = numa_node_of_cpu(core_id);
            allocator_backend->cos_association(core_id, clos_id);
            allocator_backend->allocating_cache(sock, clos_id, bitmask);
            allocator_backend->allocating_mb(sock, clos_id, &mb_throttle);
            clos_id++;
            if(clos_id == FREE_CLOS_NUM) {
                clos_id = 1;
            }
        }
    }

    virtual ~test_allocator() {}
    virtual void allocating() {}
    virtual void reset_obj_allocation_descriptor(const int& obj_id) {}
    virtual void discriminating() {}
};

}

#endif
