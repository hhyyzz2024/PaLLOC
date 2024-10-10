#include "monitor.h"
#include <mutex>

namespace PaLLOC {

monitor* monitor::instance = nullptr;

monitor::monitor(const uint64_t& period, const std::vector<int>& objects, backend *backend)
         : period(period), monitoring_objects(objects), m_monitor_backend(backend)
{
    if(setup_monitor() != RET_OK) {
        printf("Create monitor fail!\n");
		exit(-1);
    }
}

monitor* monitor::get_instance(const uint64_t& period, const std::vector<int>& objects, backend *backend)
{
    if(!instance) {
        static std::once_flag flag;
        std::call_once(flag, [&]{ instance = new (std::nothrow) monitor(period, objects, backend); });
    }

    return instance;
}

monitor* monitor::get_instance()
{
	return instance;
}

void monitor::clean_up()
{
	stop_monitor();

    monitor *instance = get_instance();
    if(instance != nullptr) {
        delete instance;
		instance = nullptr;
    }
}

int monitor::setup_monitor()
{
    return m_monitor_backend->setup_monitor();
}

void monitor::stop_monitor()
{
	m_monitor_backend->stop_monitor();
}

void monitor::start_monitoring()
{
    m_monitor_backend->start_monitoring();
}

void monitor::stop_monitoring()
{
    m_monitor_backend->stop_monitoring();
}

#ifdef ENABLE_SELF_MONITOR
void monitor::start_monitoring_slef()
{
    m_monitor_backend->start_monitoring_slef();
}

void monitor::stop_monitoring_slef()
{
    m_monitor_backend->stop_monitoring_slef();
}

#ifdef MONITORING_SELF_IPC
double monitor::get_real_ipc()
{
    uint64_t instructions, total_instructions_core0 = 0, total_cycles = 0;
    double ipc = 0.0;
    instructions = m_monitor_backend->get_self_instructions();
    for(auto monitor_data : monitor_data_map[0]) {
        total_instructions_core0 += monitor_data.period_instructions;
        total_cycles += monitor_data.cycles;
    }
    if(instructions != 0) {
        ipc = static_cast<double>(total_instructions_core0 - instructions) / total_cycles;
    }

    return ipc;
}
#endif

#ifdef MONITORING_SELF_LLC
void monitor::get_llc_request(uint64_t& llc_request, double& llc_request_ratio, int index)
{
    m_monitor_backend->get_llc_request(llc_request);
    
    const std::vector<monitor_data>& core_monitor_data = get_monitor_datas(index);

    double core_llc_request = 0;
    for(const auto& mon_data : core_monitor_data) {
        core_llc_request += mon_data.llc_request;
    }

    llc_request_ratio = llc_request / core_llc_request;
}
#endif

#ifdef MONITORING_SELF_MB
double monitor::get_mb()
{
    return bytes_to_mb(m_monitor_backend->get_mb());
}
#endif

#endif

void monitor::parse_data_to_window(int index, const std::vector<double>& data, std::unordered_map<int, win_data>& win_data_map)
{
    win_data_map[index].insert(data);
}

void monitor::parse_monitor_data(std::unordered_map<int, win_data>& win_data_map)
{
    size_t objects_size = monitoring_objects.size();
    for(int i = 0; i < objects_size; i++) {
		int id = monitoring_objects[i];				// core id or pid

		monitor_data current_data;
    	monitor_data previous_data = current_data_map[id];

		m_monitor_backend->parse_monitor_data(i, current_data, previous_data, period);
	
		current_data_map[id] = current_data;
        monitor_data_map[id].emplace_back(current_data);
        
        printf("current_data: %.4f, %ld, %.4f, %.4f\n", current_data.ipc, current_data.llc_hit, current_data.mpki, current_data.mb);
        std::vector<double> vec{current_data.ipc, static_cast<double>(current_data.llc_hit), current_data.mpki, current_data.mb};
        parse_data_to_window(id, vec, win_data_map);
    }
}

void monitor::save_monitor_data()
{
    size_t objects_size = monitoring_objects.size();
    for(int i = 0; i < objects_size; i++) {
		int id = monitoring_objects[i];				// core id or pid
        monitor_data current_data;
    	monitor_data previous_data = current_data_map[id];

		m_monitor_backend->parse_monitor_data(i, current_data, previous_data, period);
        monitor_data_map[id].emplace_back(current_data);
    }
}

/*void monitor::parse_monitor_data(std::unordered_map<int, win_data>& win_data_map)
{
    size_t objects_size = monitoring_objects.size();
    for(int i = 0; i < objects_size; i++) {
        int core_id = monitoring_objects[i];
        monitor_data current_data;
        monitor_data previous_data = current_data_map[core_id];
        current_data.current_time =  previous_data.current_time + (double)period / 1000;            // s
        current_data.period_instructions = pcm::getInstructionsRetired(start_state.cstates[core_id], stop_state.cstates[core_id]) / 1000000;   // M Instructions
        current_data.current_instructions = previous_data.current_instructions +  current_data.period_instructions;
        current_data.l2_hit = pcm::getL2CacheHits(start_state.cstates[core_id], stop_state.cstates[core_id]) / 1000;            // K times
        current_data.l2_hit_ratio = pcm::getL2CacheHitRatio(start_state.cstates[core_id], stop_state.cstates[core_id]);
        current_data.l2_miss = pcm::getL2CacheMisses(start_state.cstates[core_id], stop_state.cstates[core_id]) / 1000;
        current_data.l2_request = current_data.l2_hit + current_data.l2_miss;
        current_data.l3_hit = pcm::getL3CacheHits(start_state.cstates[core_id], stop_state.cstates[core_id]) / 1000;
        current_data.l3_hit_ratio = pcm::getL3CacheHitRatio(start_state.cstates[core_id], stop_state.cstates[core_id]);
        current_data.l3_miss = pcm::getL3CacheMisses(start_state.cstates[core_id], stop_state.cstates[core_id]) / 1000;
        current_data.l3_request = current_data.l3_hit + current_data.l3_miss;

        current_data.mb = (getBytesReadFromMC(start_state.sktstate[m->getSocketId(core_id)], stop_state.sktstate[m->getSocketId(core_id)]) + 
        getBytesWrittenToMC(start_state.sktstate[m->getSocketId(core_id)], stop_state.sktstate[m->getSocketId(core_id)])) / (double)(1024 * 1024 * period / 1000);   // GB/s
            
        current_data.excution_rate = (current_data.period_instructions) * 1000 / period;          // M Iinstructions/s

        current_data_map[core_id] = current_data;
        monitor_data_map[core_id].emplace_back(current_data);
        
//        printf("%.4f, %.4f, %.4f, %.4f\n", current_data.excution_rate, current_data.l2_hit, current_data.l3_hit, current_data.mb);
        std::vector<double> vec{current_data.excution_rate, current_data.l2_hit, current_data.l3_hit, current_data.mb};
        parse_data_to_window(core_id, vec, win_data_map);
    }
}*/
}