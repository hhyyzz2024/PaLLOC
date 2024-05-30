#ifndef _WINDOW_DATA_H_
#define _WINDOW_DATA_H_

#include <armadillo>
#include <vector>
#include "utils.h"

namespace PaLLOC {
class win_data {
public:
    win_data(const int& wim_size = WINDOW_SIZE, const int& dim = DIM);

    void sliding_window();
    void insert_row(const arma::rowvec& row);
    ret_t insert(const std::vector<double>& vec);
    ret_t insert(const double* vec, size_t size);
    ret_t insert(const arma::rowvec& vec);
    void reset_window() {n_rows = 0;}
    inline bool is_not_full() {return (n_rows < win_size);}

    inline const arma::mat& data() const {return m_data;}
    inline arma::mat& data() {return m_data;}
    inline const int& size() const {return win_size;}

private:
    int win_size;
    int dim;
    int n_rows;
    arma::mat m_data;
};
}

#endif
