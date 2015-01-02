//
// Copyright (C) 2011-14 DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#pragma once

#include <dynd/strided_vals.hpp>
#include <dynd/func/arrfunc.hpp>

namespace dynd {

/**
 * Create an arrfunc which applies a given window_op in a
 * rolling window fashion.
 *
 * \param neighborhood_op  An arrfunc object which transforms a neighborhood into
 *                         a single output value. Signature
 *                         '(Fixed * Fixed * NH, Fixed * Fixed * MSK) -> OUT',
 */
nd::arrfunc make_neighborhood_arrfunc(const nd::arrfunc &neighborhood_op);

} // namespace dynd
