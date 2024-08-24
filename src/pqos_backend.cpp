#include "backend.h"
#include <cstring>
#include <numa.h>

#define PQOS_MAX_COS      16    /**< 16 x COS */
#define PQOS_MAX_L3CA_COS PQOS_MAX_COS

namespace PaLLOC {

pqos_backend::pqos_backend(const std::vector<int>& objects, const mode& mode) 
		: pqos_objects(objects), m_mode(mode)
{
	mon_data_groups.resize(pqos_objects.size());

    int ret, exit_val = EXIT_SUCCESS;
	struct pqos_config config;
	const struct pqos_cpuinfo *p_cpu = nullptr;
    const struct pqos_cap *p_cap = nullptr;
	const struct pqos_capability *cap_mon = nullptr;

    memset(&config, 0, sizeof(config));
    config.fd_log = STDOUT_FILENO;
    config.verbose = 0;
    config.interface = PQOS_INTER_OS;
    printf("Begin init PQoS library!\n");
    /* PQoS Initialization - Check and initialize CAT and CMT capability */
    ret = pqos_init(&config);
    if (ret != PQOS_RETVAL_OK) {
        printf("Error initializing PQoS library!\n");
        exit_val = EXIT_FAILURE;
        goto error_exit;
    }
    /* Get CMT capability and CPU info pointer */
    ret = pqos_cap_get(&p_cap, &p_cpu);
    if (ret != PQOS_RETVAL_OK) {
        printf("Error retrieving PQoS capabilities!\n");
        exit_val = EXIT_FAILURE;
        goto error_exit;
    }

	ret = pqos_cap_get_type(p_cap, PQOS_CAP_TYPE_MON, &cap_mon);
    if (ret != PQOS_RETVAL_OK) {
            printf("Error retrieving monitoring capabilities\n");
            exit_val = EXIT_FAILURE;
            goto error_exit;
    }

    reset_monitor();

	/* Get CPU l3cat id information to set COS */
    l3cat_ids = pqos_cpu_get_l3cat_ids(p_cpu, &l3cat_id_count);
    if (l3cat_ids == NULL) {
        printf("Error retrieving CPU socket information!\n");
        exit_val = EXIT_FAILURE;
        goto error_exit;
    }

	/* Get CPU mba_id information to set COS */
    p_mba_ids = pqos_cpu_get_mba_ids(p_cpu, &mba_id_count);
    if (p_mba_ids == NULL) {
            printf("Error retrieving MBA ID information!\n");
            exit_val = EXIT_FAILURE;
            goto error_exit;
    }

	printf("Initializing PQoS library success!\n");
	
	return;

error_exit:
	ret = pqos_fini();
	if (ret != PQOS_RETVAL_OK)
		printf("Error shutting down PQoS library!\n");

	if(l3cat_ids)
		free(l3cat_ids);

	if(p_mba_ids)
		free(p_mba_ids);
	exit(-1); 
}

pqos_backend::~pqos_backend()
{
	struct pqos_alloc_config cfg;

    memset(&cfg, 0, sizeof(cfg));
    cfg.l3_cdp = PQOS_REQUIRE_CDP_ANY;
    cfg.l2_cdp = PQOS_REQUIRE_CDP_ANY;
    cfg.mba = PQOS_MBA_ANY;

	pqos_alloc_reset_config(&cfg);

	pqos_alloc_reset(cfg.l3_cdp, cfg.l2_cdp, cfg.mba);

	int ret = pqos_fini();
    if (ret != PQOS_RETVAL_OK)
		printf("Error shutting down PQoS library!\n");

	if(l3cat_ids)
		free(l3cat_ids);

	if(p_mba_ids)
		free(p_mba_ids);
	
	printf("Shutting down PQoS!\n");
}

int pqos_backend::reset_monitor()
{
    int exit_val;
    int ret = pqos_mon_reset();
    if (ret != PQOS_RETVAL_OK) {
        exit_val = RET_ERR;
        printf("CMT/MBM reset failed!\n");
    } else {
        printf("CMT/MBM reset successful\n");
        exit_val = RET_OK;
    }

    return exit_val;
}

int pqos_backend::setup_monitor()
{
	int ret;
    pqos_mon_event mon_event = (enum pqos_mon_event)(PQOS_MON_EVENT_L3_OCCUP | PQOS_PERF_EVENT_IPC | PQOS_PERF_EVENT_LLC_MISS 
    | PQOS_PERF_EVENT_LLC_REF | PQOS_MON_EVENT_LMEM_BW);
	
	if(m_mode == mode::CORES) {
		for(int i = 0; i < pqos_objects.size(); i++) {
			const unsigned int *core = (const unsigned int *)&pqos_objects[i];
			ret = pqos_mon_start_cores(1, core, mon_event, NULL, &mon_data_groups[i]);
	        if (ret != PQOS_RETVAL_OK) {
                printf("Monitoring start error on core %u,"
                       "status %d\n",
                       *core, ret);
                return RET_MONITOR_EXIST_ERR;
	        }
		}

	} else {
		for(int i = 0; i < pqos_objects.size(); i++) {
			const pid_t *pid = &pqos_objects[i];
			ret = pqos_mon_start_pids2(1, pid, mon_event, NULL, &mon_data_groups[i]);
		    if (ret != PQOS_RETVAL_OK) {
	            printf("Monitoring start error on pid %u,"
	                    "status %d\n",
	                    *pid, ret);
                return RET_MONITOR_EXIST_ERR;
		    }
		}
	}

	return RET_OK;
}

void pqos_backend::stop_monitor()
{
	for (int i = 0; i < mon_data_groups.size(); i++) {
	    int ret = pqos_mon_stop(mon_data_groups[i]);
	    if (ret != PQOS_RETVAL_OK)
	    	printf("Monitoring %d stop error!\n", i);
        log_info("Monitor %d has stop!\n", i);
    }
}

void pqos_backend::start_monitoring()
{
	pqos_mon_poll(mon_data_groups.data(), mon_data_groups.size());
}

void pqos_backend::stop_monitoring()
{
	pqos_mon_poll(mon_data_groups.data(), mon_data_groups.size());
}

void pqos_backend::parse_monitor_data(const int& group_index, monitor_data& current_data,
		const monitor_data& previous_data, const uint64_t& period)
{
	const struct pqos_event_values *pv = &mon_data_groups[group_index]->values;
    
	// instructions
    current_data.current_time =  previous_data.current_time + (double)period / 1000;            // s
    current_data.period_instructions = pv->ipc_retired_delta;
    current_data.current_instructions = pv->ipc_retired;

	// llc
	pqos_mon_get_value(mon_data_groups[group_index], PQOS_PERF_EVENT_LLC_REF, NULL, &current_data.llc_request);
    current_data.llc_miss = pv->llc_misses_delta;
    current_data.llc_hit = current_data.llc_request - current_data.llc_miss;
    if(current_data.llc_request != 0) {
        current_data.llc_hit_ratio = (double)current_data.llc_hit / current_data.llc_request;
		current_data.llc_miss_ratio = (double)current_data.llc_miss / current_data.llc_request;
	}
    else {
		current_data.llc_hit_ratio = 0;
		current_data.llc_miss_ratio = 0;
	}
    current_data.llc_occupancy = pv->llc;
        
	// mb
    current_data.mb = pv->mbm_local_delta / (double)(1024 * 1024 * period / 1000);  // MB/s

	// excution rate
    current_data.excution_rate = (current_data.period_instructions) * 1000 / period;          // M Iinstructions/s

	// ipc
	current_data.ipc = pv->ipc;

	// mpki
	current_data.mpki = current_data.llc_miss * 1000 / (double)current_data.period_instructions;

	// hpki
	current_data.hpki = current_data.llc_hit * 1000 / (double)current_data.period_instructions;
}

int pqos_backend::cos_association(const int& obj, const unsigned& class_id)
{
    int ret = RET_OK;
    if(m_mode == mode::CORES) {
        ret = pqos_alloc_assoc_set(obj, class_id);
    } else {
        ret = pqos_alloc_assoc_set_pid(obj, class_id);
    }
    if (ret != PQOS_RETVAL_OK) {
        printf("Object %d associate cos %d failed!\n", obj, class_id);
        return RET_ERR;
    }

	return RET_OK;
}

int pqos_backend::allocating_cache(const std::vector<unsigned>& sockets, unsigned class_id, uint64_t ways_mask)
{
	int ret;
	for(auto sock : sockets) {
		struct pqos_l3ca ca = {class_id, 0, ways_mask};

		unsigned l3id = sock_to_l3id(sock);
	
		ret = pqos_l3ca_set(l3id, 1, &ca);
        if (ret != PQOS_RETVAL_OK) {
                printf("Setting up cache allocation class of "
                       "service failed!\n");
                return RET_ERR;
        }
	}

	return RET_OK;
}

int pqos_backend::allocating_cache(const uint32_t& socket, unsigned class_id, uint64_t ways_mask)
{
	int ret;
	
    struct pqos_l3ca ca = {class_id, 0, ways_mask};

    unsigned l3id = sock_to_l3id(socket);

    ret = pqos_l3ca_set(l3id, 1, &ca);
    if (ret != PQOS_RETVAL_OK) {
            printf("Setting up cache allocation class of "
                    "service failed!\n");
            return RET_ERR;
    }
	
	return RET_OK;
}

int pqos_backend::allocating_mb(const std::vector<unsigned>& sockets, unsigned class_id, unsigned* mb_throttle)
{
	int ret;
	for(auto sock : sockets) {
		struct pqos_mba mba_request = {class_id, *mb_throttle, 0};
		struct pqos_mba mba_actual;

		unsigned mbid = sock_to_mbid(sock);

		ret = pqos_mba_set(mbid, 1, &mba_request, &mba_actual);
		if (ret != PQOS_RETVAL_OK) {
                printf("Failed to set MBA!\n");
                return RET_ERR;
        }

		*mb_throttle = mba_actual.mb_max;

		printf("SKT%u: MBA COS%u => %u%% requested, %u%% applied\n",
                       mbid, mba_request.class_id,mba_request.mb_max,
                       mba_actual.mb_max);
	}

	return RET_OK;
}

int pqos_backend::allocating_mb(const uint32_t& socket, unsigned class_id, unsigned* mb_throttle)
{
	int ret;
	
    struct pqos_mba mba_request = {class_id, *mb_throttle, 0};
    struct pqos_mba mba_actual;

    unsigned mbid = sock_to_mbid(socket);

    ret = pqos_mba_set(mbid, 1, &mba_request, &mba_actual);
    if (ret != PQOS_RETVAL_OK) {
            printf("Failed to set MBA!\n");
            return RET_ERR;
    }

    *mb_throttle = mba_actual.mb_max;

    printf("SKT%u: MBA COS%u => %u%% requested, %u%% applied\n",
                    mbid, mba_request.class_id,mba_request.mb_max,
                    mba_actual.mb_max);
	
	return RET_OK;    
}

int pqos_backend::print_allocating_config(const std::vector<unsigned>& sockets)
{
	int ret;

    for(auto sock : sockets) {
        struct pqos_l3ca tab[PQOS_MAX_L3CA_COS];
        unsigned num = 0;
        
        ret = pqos_l3ca_get(sock, PQOS_MAX_L3CA_COS, &num, tab);
        if (ret == PQOS_RETVAL_OK) {
            unsigned n = 0;

            printf("L3CA COS definitions for Socket %u:\n",
                   sock);
            for (n = 0; n < num; n++) {
                    printf("    L3CA COS%u => MASK 0x%llx\n",
                           tab[n].class_id,
                           (unsigned long long)tab[n].u.ways_mask);
                }
        } else {
            printf("Error:%d", ret);
            return RET_ERR;
        }
    }
    return RET_OK;
}

}