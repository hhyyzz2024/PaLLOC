#include "backend.h"

namespace PaLLOC{

pcm_pqos_backend::pcm_pqos_backend()
{
    log_info("Begin init PCM Monitor and PQos Allocator!\n");

    pcm = pcm::PCM::getInstance();
    pcm::PCM::ErrorCode err = pcm->program();
    if(err != pcm::PCM::Success) {
        log_error("Intel PCM can not start!\n");
        exit(1);
    }

    int ret, exit_val = EXIT_SUCCESS;
	struct pqos_config config;
	const struct pqos_cpuinfo *p_cpu = nullptr;
    const struct pqos_cap *p_cap = nullptr;
	const struct pqos_capability *cap_mon = nullptr;

    memset(&config, 0, sizeof(config));
    config.fd_log = STDOUT_FILENO;
    config.verbose = 0;

    // PCM can only coexist with CAT's msr api
    config.interface = PQOS_INTER_MSR;

    /* PQoS Initialization - Check and initialize CAT and CMT capability */

    ret = pqos_init(&config);
    if (ret != PQOS_RETVAL_OK) {
        log_error("Error initializing PQoS Allocator!\n");
        exit_val = EXIT_FAILURE;
        goto error_exit;
    }

    /* Get CMT capability and CPU info pointer */
    ret = pqos_cap_get(&p_cap, &p_cpu);
    if (ret != PQOS_RETVAL_OK) {
        log_error("Error retrieving PQoS capabilities!\n");
        exit_val = EXIT_FAILURE;
        goto error_exit;
    }

	/* Get CPU l3cat id information to set COS */
    l3cat_ids = pqos_cpu_get_l3cat_ids(p_cpu, &l3cat_id_count);
    if (l3cat_ids == NULL) {
        log_error("Error retrieving CPU socket information!\n");
        exit_val = EXIT_FAILURE;
        goto error_exit;
    }

	/* Get CPU mba_id information to set COS */
    p_mba_ids = pqos_cpu_get_mba_ids(p_cpu, &mba_id_count);
    if (p_mba_ids == NULL) {
            log_error("Error retrieving MBA ID information!\n");
            exit_val = EXIT_FAILURE;
            goto error_exit;
    }

	log_info("Initializing PCM Monitor and PQos Allocator success!\n");
	
	return;

error_exit:
    pcm->cleanup();

	ret = pqos_fini();
	if (ret != PQOS_RETVAL_OK)
		log_error("Error shutting down PQoS Allocator!\n");

	if(l3cat_ids)
		free(l3cat_ids);

	if(p_mba_ids)
		free(p_mba_ids);
	exit(-1); 
}

pcm_pqos_backend::~pcm_pqos_backend()
{
    pcm->cleanup();

    log_info("PCM clean up!\n");

    struct pqos_alloc_config cfg;

    memset(&cfg, 0, sizeof(cfg));
    cfg.l3_cdp = PQOS_REQUIRE_CDP_ANY;
    cfg.l2_cdp = PQOS_REQUIRE_CDP_ANY;
    cfg.mba = PQOS_MBA_ANY;

	pqos_alloc_reset_config(&cfg);

	pqos_alloc_reset(cfg.l3_cdp, cfg.l2_cdp, cfg.mba);;

	int ret = pqos_fini();
    if (ret != PQOS_RETVAL_OK)
		log_error("Error shutting down PQoS library!\n");

	if(l3cat_ids)
		free(l3cat_ids);

	if(p_mba_ids)
		free(p_mba_ids);
	
	log_info("Shutting down PQoS!\n");
}

void pcm_pqos_backend::start_monitoring()
{
    pcm->getAllCounterStates(before_state.sstate, before_state.sktstate, before_state.cstates);
}

void pcm_pqos_backend::stop_monitoring()
{
    pcm->getAllCounterStates(after_state.sstate, after_state.sktstate, after_state.cstates);
}

void pcm_pqos_backend::parse_monitor_data(const int& core_id, monitor_data& current_data,
		const monitor_data& previous_data, const uint64_t& period)
{
    // instructions
    current_data.current_time =  previous_data.current_time + (double)period / 1000;            // s
    current_data.period_instructions = pcm::getInstructionsRetired(before_state.cstates[core_id], after_state.cstates[core_id]);
    current_data.current_instructions = current_data.period_instructions + previous_data.current_instructions;

    // llc
    current_data.llc_miss = pcm::getL3CacheMisses(before_state.cstates[core_id], after_state.cstates[core_id]);
    current_data.llc_hit = pcm::getL3CacheHits(before_state.cstates[core_id], after_state.cstates[core_id]);
    current_data.llc_request = current_data.llc_miss + current_data.llc_hit;
    current_data.llc_hit_ratio = pcm::getL3CacheHitRatio(before_state.cstates[core_id], after_state.cstates[core_id]);
    if(current_data.llc_request != 0) {
		current_data.llc_miss_ratio = (double)current_data.llc_miss / current_data.llc_request;
	}
    else {
		current_data.llc_miss_ratio = 0;
	}
    current_data.llc_occupancy = pcm::getL3CacheOccupancy(after_state.cstates[core_id]);

    // mb
    current_data.mb = (pcm::getBytesReadFromMC(before_state.sktstate[pcm->getSocketId(core_id)], after_state.sktstate[pcm->getSocketId(core_id)]) + 
        pcm::getBytesWrittenToMC(before_state.sktstate[pcm->getSocketId(core_id)], after_state.sktstate[pcm->getSocketId(core_id)])) / (double)(1024 * 1024 * period / 1000);  // MB/s

	// excution rate
    current_data.excution_rate = (current_data.period_instructions) * 1000 / period;          // M Iinstructions/s

	// ipc
	current_data.ipc = pcm::getIPC(before_state.cstates[core_id], after_state.cstates[core_id]);

	// mpki
	current_data.mpki = current_data.llc_miss * 1000 / (double)current_data.period_instructions;

	// hpki
	current_data.hpki = current_data.llc_hit * 1000 / (double)current_data.period_instructions;
}

double pcm_pqos_backend::get_llc_read_miss_latency(uint32_t socket)
{
    return pcm::getLLCReadMissLatency(before_state.sktstate[socket], after_state.sktstate[socket]) * pcm::getActiveAverageFrequency(before_state.sktstate[socket], after_state.sktstate[socket]) / 1e9;
}

int pcm_pqos_backend::cos_association(const unsigned& core, const unsigned& class_id)
{
    int ret = pqos_alloc_assoc_set(core, class_id);
    if (ret != PQOS_RETVAL_OK) {
        printf("Core %d associate cos %d failed!\n", core, class_id);
        return RET_ERR;
    }

	return RET_OK;
}

int pcm_pqos_backend::allocating_cache(const std::vector<unsigned>& sockets, unsigned class_id, uint64_t ways_mask)
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

int pcm_pqos_backend::allocating_cache(const uint32_t& socket, unsigned class_id, uint64_t ways_mask)
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

int pcm_pqos_backend::allocating_mb(const std::vector<unsigned>& sockets, unsigned class_id, unsigned* mb_throttle)
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

int pcm_pqos_backend::allocating_mb(const uint32_t& socket, unsigned class_id, unsigned* mb_throttle)
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

int pcm_pqos_backend::print_allocating_config(const std::vector<unsigned>& sockets)
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
