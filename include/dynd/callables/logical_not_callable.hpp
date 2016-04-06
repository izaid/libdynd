//
// Copyright (C) 2011-16 DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#pragma once

#include <dynd/callables/apply_function_callable.hpp>
#include <dynd/kernels/arithmetic.hpp>

namespace dynd {
namespace nd {

  template <type_id_t Arg0ID>
  using logical_not_callable = functional::apply_function_callable<decltype(&detail::inline_logical_not<Arg0ID>::f),
                                                                   &detail::inline_logical_not<Arg0ID>::f>;

} // namespace dynd::nd
} // namespace dynd
