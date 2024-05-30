#include "discriminator.h"
#include <math.h>

namespace PaLLOC {

enum class_t {
    FIRST_CLASS = 0,
    SECOND_CLASS = 1,
    UNKNOWN_CLASS,
};

discriminator* discriminator::instance = nullptr;
static const double outlier_threshold = 0.15;


discriminator::discriminator(std::unordered_map<int, win_data>& win_data_map) : discriminate_data_map(win_data_map)
{
	for(const auto& discriminate_data_pair : discriminate_data_map) {
		discriminator_on_map.insert(std::make_pair(discriminate_data_pair.first, true));
	}
}

discriminator* discriminator::get_instance(std::unordered_map<int, win_data>& win_data_map)
{
    if(!instance) {
        static std::once_flag flag;
        std::call_once(flag, [&] { instance = new (std::nothrow) discriminator(win_data_map);});
    }
    
    return instance;
}

void discriminator::clean_up()
{
    static discriminator* instance = get_instance();
    if(instance != nullptr) {
        delete instance;
    }
}

//两个质心为不同类，按质心距离分类
ret_t discriminator::centroid_distance_classify(const arma::mat& means, arma::irowvec& classify_labels)
{
    if(means.n_rows != classify_labels.n_elem) {
        log_error("The element number of labels is not same as means!\n");
        return ret_t::RET_ARGUMENT_ERR;
    }
    const size_t means_size = means.n_rows;
    for(int i = 0; i < means_size; i++) {
        double a = means(i, 0), b = means(i, 1);
        if(a-b > outlier_threshold) {
            classify_labels[i] = SECOND_CLASS;
        }

        if(a-b < -outlier_threshold) {
            classify_labels[i] = FIRST_CLASS;
        }
    }
    return ret_t::RET_OK;
}

//局部性原理，同一类的点与质心距离相近
ret_t discriminator::locality_distance_classify(const arma::mat& means, arma::irowvec& classify_labels)
{
    if(means.n_rows != classify_labels.n_elem) {
        log_error("The element number of labels is not same as means!\n");
        return ret_t::RET_ARGUMENT_ERR;
    }

    const size_t means_size = means.n_rows;
    int first_class_element_pos = -1, second_class_element_pos = -1;
    for(int i = 0; i < means_size; i++) {
        if(classify_labels[i] == FIRST_CLASS) {
            first_class_element_pos = i;
        } 
        
        if(classify_labels[i] == SECOND_CLASS) {
            second_class_element_pos = i;
        }
    }

    for(int i = 0; i < means_size; i++) {
        if(classify_labels[i] == UNKNOWN_CLASS) {
            if(first_class_element_pos != -1 && second_class_element_pos != -1) {
                double dis_first_class = std::sqrt(std::pow(means(i, 0) - means(first_class_element_pos, 0), 2)+
                std::pow(means(i, 1) - means(first_class_element_pos, 1), 2));

                double dis_second_class = std::sqrt(std::pow(means(i, 0) - means(second_class_element_pos, 0), 2)+
                std::pow(means(i, 1) - means(second_class_element_pos, 1), 2));
                
                classify_labels[i] = dis_first_class < dis_second_class ? FIRST_CLASS : SECOND_CLASS;
            } else if(first_class_element_pos == -1 && second_class_element_pos != -1) {
                double dis_second_class = std::sqrt(std::pow(means(i, 0) - means(second_class_element_pos, 0), 2)+
                std::pow(means(i, 1) - means(second_class_element_pos, 1), 2));
                classify_labels[i] = dis_second_class > outlier_threshold ? FIRST_CLASS : SECOND_CLASS;
            } else if(first_class_element_pos != -1 && second_class_element_pos == -1) {
                double dis_first_class = std::sqrt(std::pow(means(i, 0) - means(first_class_element_pos, 0), 2)+
                std::pow(means(i, 1) - means(first_class_element_pos, 1), 2));

                classify_labels[i] = dis_first_class <= outlier_threshold ? FIRST_CLASS : SECOND_CLASS;
            } else {
                classify_labels[i] = FIRST_CLASS;
            }
        }
    }

    return ret_t::RET_OK;
}

bool discriminator::is_not_same_class(const arma::mat& means, int discriminate_point)
{
    const size_t means_size = means.n_rows;
    arma::irowvec labels(means_size, arma::fill::value(static_cast<int>(UNKNOWN_CLASS)));
    int discriminate_label;
    means.print();
    centroid_distance_classify(means, labels);
    locality_distance_classify(means, labels);

/*    means.print();
    printf("\n");
    labels.print();
    printf("\n");*/ 
    int votes = 0;
    discriminate_label = labels[discriminate_point];
    arma::irowvec after_check_labels = {labels[discriminate_point+1], labels[discriminate_point+2]};
    for(int i = 0; i < discriminate_point; i++) {
        if(discriminate_label != labels[i])
            votes++;
    }

    if(votes >= discriminate_point - 1 && arma::all(after_check_labels == discriminate_label)) {
        means.print();
        labels.print();
        return true;
    } else
        return false;
}

bool discriminator::is_one_category(const arma::mat& data, const arma::mat& means)
{
	if(data.n_rows != means.n_rows) {
		log_error("is_one_category parameter shape error!\n");
		return true;
	}

	for(int c = 0; c < data.n_cols; c++) {
		double val = 0.0;
		for(int r = 0; r < data.n_rows; r++) {
			val += fabs(data(r,c) - means(r,0)) / means(r,0);
		}

		val /= data.n_rows;
		if(val > outlier_threshold) {
			log_debug("val: %f, c: %d\n", val, c);
			return false;
		}
	}

	return true;
}

bool discriminator::is_step_jump(const arma::mat& data, const arma::mat& means, const int& discriminate_point)
{
	double dis_first_class = 0.0, dis_second_class = 0.0;
	arma::irowvec labels(data.n_cols, arma::fill::value(static_cast<int>(UNKNOWN_CLASS)));
	for(int c = 0; c < data.n_cols; c++) {
		for(int r = 0; r < data.n_rows; r++) {
//			dis_first_class += std::pow(data(r, c)-means(r, 0), 2);
//			dis_second_class += std::pow(data(r, c)-means(r, 1), 2);

			//The magnitudes of the features in each dimension varied greatly, 
			//so the relative distances were used
			dis_first_class += fabs(data(r, c)-means(r, 0))/means(r, 0);
			dis_second_class += fabs(data(r, c)-means(r, 1))/means(r, 1);
		}

		dis_first_class <= dis_second_class ? labels[c] = FIRST_CLASS : labels[c] = SECOND_CLASS;
		dis_first_class = dis_second_class = 0.0;
	}

#ifdef DEBUG_ON
	labels.print();
#endif

	int discriminate_label = labels[discriminate_point];
	for(int i = 0; i < discriminate_point; i++) {
		if(labels[i] == labels[discriminate_point])
			return false;
	}

	for(int i = discriminate_point+1; i < labels.n_cols; i++) {
		if(labels[i] != labels[discriminate_point])
			return false;
	}

	return true;
}

bool discriminator::get_objs_need_reallocate_resource(std::vector<int>& reallocate_obj_ids)
{
    bool ret = false;
    for(auto& discriminate_data_pair : discriminate_data_map) {
		if(discriminator_is_turn_on(discriminate_data_pair.first)) {
	        if(need_reallocate_resource(discriminate_data_pair.first)) {
	            reallocate_obj_ids.emplace_back(discriminate_data_pair.first);
	            ret = true;
	        }
		}
    }

    return ret;
}

bool discriminator::phase_has_changed()
{
    for(auto& discriminate_data_pair : discriminate_data_map) {
		if(discriminator_is_turn_on(discriminate_data_pair.first) && need_reallocate_resource(discriminate_data_pair.first)) {
            log_info("Phase of obj: %d has changed!\n", discriminate_data_pair.first);
            return true;
		}
    }

    return false;
}

bool discriminator::need_reallocate_resource(int index)
{
    if(discriminate_data_map.find(index) == discriminate_data_map.end()) {
        log_error("Index out of range!\n");
        return false;
    }

    int discriminate_point = discriminate_point_map[index];
    win_data& win_data = discriminate_data_map[index];

    // The amount of data in the window is not full, and there is no need to reallocate resources
    if(win_data.is_not_full()) return false;

    arma::mat window_data = win_data.data().t();

#ifdef DEBUG_ON
    window_data.print();
#endif

	// L2-norm to rows
//    window_data = normalise(window_data, 2, 0);
//	window_data.print();

	// Determine whether window data can only be divided into one category
    arma::mat means;
    bool status = kmeans(means, window_data, 1, arma::random_subset, 10, false);
    if(status == false) {
        log_error("Clustering failed!\n");
        return false;
    }

#ifdef DEBUG_ON
	means.print();
#endif

	if(is_one_category(window_data, means)) {
		return false;
	}

	// Determine whether the window data has a step jump
    status = kmeans(means, window_data, 2, arma::random_subset, 10, false);
    if(status == false) {
        log_error("Clustering failed!\n");
        return false;
    }

#ifdef DEBUG_ON
	means.print();
#endif

	return is_step_jump(window_data, means, discriminate_point_map[index]);
}

void discriminator::set_discriminate_points()
{
    for(auto& discriminate_data_pair : discriminate_data_map) {
        discriminate_point_map.emplace(discriminate_data_pair.first, DISCRIMINATE_POINT);
    }
}

void discriminator::turn_off_discriminator()
{
    for(auto& dis : discriminator_on_map) {
        dis.second = false;
    }
}

void discriminator::turn_on_discriminator()
{
    for(auto& dis : discriminator_on_map) {
        dis.second = true;
        discriminate_data_map[dis.first].reset_window();
    }
}

}