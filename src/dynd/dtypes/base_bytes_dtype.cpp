//
// Copyright (C) 2011-13, DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#include <dynd/dtypes/base_bytes_dtype.hpp>

using namespace std;
using namespace dynd;


base_bytes_dtype::~base_bytes_dtype()
{
}

size_t base_bytes_dtype::get_iterdata_size(int DYND_UNUSED(ndim)) const
{
    return 0;
}