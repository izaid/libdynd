//
// Copyright (C) 2011-16 DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#pragma once

#include <dynd/callables/base_apply_callable.hpp>
#include <dynd/kernels/apply_function_kernel.hpp>

namespace dynd {
namespace nd {
  namespace functional {

    template <typename func_type, func_type func, size_t NArg = arity_of<func_type>::value>
    class apply_function_callable : public base_apply_callable<func_type> {
    public:
      using base_apply_callable<func_type>::base_apply_callable;

      ndt::type resolve(base_callable *DYND_UNUSED(caller), char *DYND_UNUSED(data), call_graph &cg,
                        const ndt::type &dst_tp, size_t DYND_UNUSED(nsrc), const ndt::type *DYND_UNUSED(src_tp),
                        size_t nkwd, const array *kwds, const std::map<std::string, ndt::type> &DYND_UNUSED(tp_vars)) {
        typedef apply_function_kernel<func_type, func, NArg> kernel_type;

        cg.emplace_back([kwds = typename kernel_type::kwds_type(nkwd, kwds)](
            kernel_builder & kb, kernel_request_t kernreq, char *data, const char *DYND_UNUSED(dst_arrmeta),
            size_t DYND_UNUSED(nsrc), const char *const *src_arrmeta) {
          kb.emplace_back<kernel_type>(kernreq, typename kernel_type::args_type(data, src_arrmeta), kwds);
        });

        return this->resolve_return_type(dst_tp);
      }
    };

  } // namespace dynd::nd::functional
} // namespace dynd::nd
} // namespace dynd
