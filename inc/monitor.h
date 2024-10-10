#ifndef _MONITOR_H_
#define _MONITOR_H_

#include "utils.h"
#include "window_data.h"
#include <vector>
#include <unordered_map>
#include "backend.h"
#include "monitor_type.h"

namespace PaLLOC {

class monitor {
public:
    static monitor* get_instance(const uint64_t& period, const std::vector<int>& objects, backend *backend);
    void clean_up();
    void start_monitoring();
    void stop_monitoring();
    void parse_monitor_data(std::unordered_map<int, win_data>& win_data_map);
    void save_monitor_data();
    void parse_data_to_window(int index, const std::vector<double>& data, std::unordered_map<int, win_data>& win_data_map);
    inline const monitor_data& get_current_data(int index) const {return current_data_map.at(index);}
	inline const std::unordered_map<int, monitor_data>& get_current_data_map() const {return current_data_map;}
    inline const std::vector<monitor_data>& get_monitor_datas(int index) const {return monitor_data_map.at(index);}
    inline double get_llc_read_miss_latency(uint32_t socket) {return m_monitor_backend->get_llc_read_miss_latency(socket);}

#ifdef ENABLE_SELF_MONITOR
    void start_monitoring_slef();
    void stop_monitoring_slef();

#ifdef MONITORING_SELF_IPC
    double get_real_ipc();
#endif

#ifdef MONITORING_SELF_LLC
	void get_llc_request(uint64_t& llc_request, double& llc_request_ratio, int index);
#endif

#ifdef MONITORING_SELF_MB
	double get_mb();
#endif

#endif

private:
    monitor(const uint64_t& period, const std::vector<int>& objects, backend *backend);
    monitor(const monitor& other) = delete;
    monitor& operator=(const monitor& other) = delete;
	int setup_monitor();
	void stop_monitor();
	static monitor* get_instance();

    uint64_t period;
    

#ifdef MONITORING_SELF_LLC
    uint64_t m_peak_llc = 0;
    uint64_t m_llc = 0;
    uint64_t m_counts = 0;
    double m_hit_of_peak_llc;
    double m_llc_hit = 0.0;
#endif

	static monitor *instance;
	backend *m_monitor_backend;
    std::vector<int> monitoring_objects;                            //if mode = PROCESSES, objects is pid of process, else if mode = CORES, objects is core id
    std::unordered_map<int, monitor_data> current_data_map;
    std::unordered_map<int, std::vector<monitor_data> > monitor_data_map;
};

}
#endif