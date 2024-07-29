#ifndef _SYSTEM_H_
#define _SYSTEM_H_

#include "monitor.h"
#include "window_data.h"
#include "discriminator.h"
#include "recorder.h"
#include "allocator.h"
#include "utils.h"
#include "backend.h"
#include <vector>
#include <sched.h>
#include <unordered_map>

namespace PaLLOC {

class system {
public:
    static system *get_instance(int argc, char **argv);
    static void process_exit_handle(int signo);
    void clean_up();
    void run();
    void orchid_run();

private:
    system(int argc, char **argv);
    ~system();
    system(const system& other) = delete;
    system& operator=(const system& other) = delete;
	void parse_argument(int argc, char **argv);
	void show_usage();
	void parse_objs(const std::string& s);
    void partial_discriminating();
    void once_discriminating();
    void partial_allocating();
    void dna_test_allocating();
    void dna_collecting_info();
    void dna_allocating();
    
	static system *get_instance();

    pid_t pid;
    int system_core;
    int allocation_interval = 10;
	mode m_mode;
	
	bool process_exit = false;
	bool allocator_on = true;
    uint64_t period = 100;
    uint64_t *flags;
	static system *instance;

	backend *m_tool_backend;

    monitor *m_monitor;
    discriminator *m_discriminator;
    recorder *m_recorder;
	allocator *m_allocator;

	std::vector<int> objs;
    std::unordered_map<int, win_data> window_data_map;

#ifdef TEST
    experimental_data exp_data;
#endif
};

}

#endif

