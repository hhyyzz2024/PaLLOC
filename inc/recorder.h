#ifndef _RECORDER_H_
#define _RECORDER_H_

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace PaLLOC {
struct recording_data {
    uint64_t retired_instruction_start = 0;
    uint64_t retired_inscrutions_len = 0;
    uint32_t llc_conf = 0;
    float mb_conf = 0.0;
};

class recorder {
public:
    recorder(const std::vector<int>& keys) {
        for(auto& key : keys) {
            recording_data_map.emplace(key, std::vector<recording_data>(1, recording_data()));
        }
    }

    inline void recording(int key, const recording_data& data) {
        if(recording_data_map.find(key) == recording_data_map.end()) {
            recording_data_map.emplace(key, std::vector<recording_data>(1, data));
        } else {
            recording_data_map.at(key).emplace_back(data);
        }
    }

    inline const recording_data& get_last_record(int key) const {return recording_data_map.at(key).back();}

private:
    std::unordered_map<int, std::vector<recording_data> > recording_data_map;
};
}

#endif