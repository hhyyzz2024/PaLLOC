#include "window_data.h"

namespace PaLLOC {

win_data::win_data(const int& win_size, const int& dim) : win_size(win_size), dim(dim), n_rows(0), m_data(win_size, dim)
{
    
}

void win_data::sliding_window()
{
    for(int i = 0; i < m_data.n_rows-1; i++) {
        m_data.row(i) = m_data.row(i+1);
    }  
}

void win_data::insert_row(const arma::rowvec& row)
{
    if(n_rows < win_size) {
        m_data.row(n_rows++) = row;
    } else {
        sliding_window();
        m_data.row(win_size-1) = row;
    }
}

ret_t win_data::insert(const std::vector<double>& vec)
{
    if(vec.size() != dim) {
        perr("The column of vector is not same as window data\n!");
        return ret_t::RET_ARGUMENT_ERR;
    }

    insert_row(arma::rowvec(vec));
    
    return ret_t::RET_OK;
}

ret_t win_data::insert(const double* vec, size_t size)
{
    if(size != dim) {
        perr("The column of vector is not same as window data\n!");
        return ret_t::RET_ARGUMENT_ERR;
    }

    insert_row(arma::rowvec(vec, size));

    return ret_t::RET_OK;
}

ret_t win_data::insert(const arma::rowvec& vec)
{
    if(vec.n_cols != dim) {
        perr("The column of vector is not same as window data\n!");
        return ret_t::RET_ARGUMENT_ERR;
    }

    insert_row(vec);

    return ret_t::RET_OK;
}
}