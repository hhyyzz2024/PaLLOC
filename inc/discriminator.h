#ifndef _DISCRIMINATOR_H_
#define _DISCRIMINATOR_H_

#include <unordered_map>
#include <vector>
#include "window_data.h"
#include "utils.h"

namespace PaLLOC {

class discriminator {
public:
    static discriminator* get_instance(std::unordered_map<int, win_data>& win_data_map);
    static void clean_up();
    bool need_reallocate_resource(int index);
    bool get_objs_need_reallocate_resource(std::vector<int>&        obj_ids);
    bool phase_has_changed();
    void set_discriminate_points();
	inline void turn_off_discriminator(int obj_id) {discriminator_on_map[obj_id] = false;}
	inline void turn_on_discriminator(int obj_id) {discriminator_on_map[obj_id] = true; discriminate_data_map[obj_id].reset_window();}
    std::unordered_map<int, win_data>& get_window_data_map() {return discriminate_data_map;}
    void turn_off_discriminator();
    void turn_on_discriminator();

	inline bool discriminator_is_turn_on(int obj_id) {return discriminator_on_map[obj_id];}

private:
    discriminator(std::unordered_map<int, win_data>& win_data_map);
    discriminator(const discriminator& other) = delete;
    discriminator& operator=(const discriminator& other) = delete;

    bool is_not_same_class(const arma::mat& means, int discriminate_point);

    ret_t centroid_distance_classify(const arma::mat& means, arma::irowvec& classify_labels);
    ret_t locality_distance_classify(const arma::mat& means, arma::irowvec& classify_labels);
	static discriminator* get_instance() {return instance;}
	
	bool is_one_category(const arma::mat& data, const arma::mat& means);
	bool is_step_jump(const arma::mat& data, const arma::mat& means, const int& discriminate_point);

    static discriminator *instance;
    std::unordered_map<int, int> discriminate_point_map;
    std::unordered_map<int, win_data>& discriminate_data_map;
	std::unordered_map<int, bool> discriminator_on_map;					// Whether the object's discriminator is turned on
};

}
#endif
