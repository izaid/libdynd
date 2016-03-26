//
// Copyright (C) 2011-15 DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#pragma once

#include <dynd/callables/base_callable.hpp>

namespace dynd {
namespace nd {

  template <typename CallableType>
  class call_callable : public base_callable {
  public:
    call_callable(const ndt::type &tp) : base_callable(tp) {}

    char *data_init(const ndt::type &dst_tp, intptr_t nsrc, const ndt::type *src_tp, intptr_t nkwd, const array *kwds,
                    const std::map<std::string, ndt::type> &tp_vars)
    {
      return CallableType::get()->data_init(dst_tp, nsrc, src_tp, nkwd, kwds, tp_vars);
    }

    void resolve_dst_type(char *data, ndt::type &dst_tp, intptr_t nsrc, const ndt::type *src_tp, intptr_t nkwd,
                          const array *kwds, const std::map<std::string, ndt::type> &tp_vars)
    {
      CallableType::get()->resolve_dst_type(data, dst_tp, nsrc, src_tp, nkwd, kwds, tp_vars);
    }

    void instantiate(char *data, kernel_builder *ckb, const ndt::type &dst_tp, const char *dst_arrmeta, intptr_t nsrc,
                     const ndt::type *src_tp, const char *const *src_arrmeta, kernel_request_t kernreq, intptr_t nkwd,
                     const array *kwds, const std::map<std::string, ndt::type> &tp_vars)
    {
      CallableType::get()->instantiate(data, ckb, dst_tp, dst_arrmeta, nsrc, src_tp, src_arrmeta, kernreq, nkwd, kwds,
                                       tp_vars);
    }
  };

  template <callable &Callable>
  class new_call_callable : public base_callable {
  public:
    new_call_callable(const ndt::type &tp) : base_callable(tp) {}

    char *data_init(const ndt::type &dst_tp, intptr_t nsrc, const ndt::type *src_tp, intptr_t nkwd, const array *kwds,
                    const std::map<std::string, ndt::type> &tp_vars)
    {
      return Callable->data_init(dst_tp, nsrc, src_tp, nkwd, kwds, tp_vars);
    }

    void resolve_dst_type(char *data, ndt::type &dst_tp, intptr_t nsrc, const ndt::type *src_tp, intptr_t nkwd,
                          const array *kwds, const std::map<std::string, ndt::type> &tp_vars)
    {
      Callable->resolve_dst_type(data, dst_tp, nsrc, src_tp, nkwd, kwds, tp_vars);
    }

    void instantiate(char *data, kernel_builder *ckb, const ndt::type &dst_tp, const char *dst_arrmeta, intptr_t nsrc,
                     const ndt::type *src_tp, const char *const *src_arrmeta, kernel_request_t kernreq, intptr_t nkwd,
                     const array *kwds, const std::map<std::string, ndt::type> &tp_vars)
    {
      Callable->instantiate(data, ckb, dst_tp, dst_arrmeta, nsrc, src_tp, src_arrmeta, kernreq, nkwd, kwds, tp_vars);
    }
  };

} // namespace dynd::nd
} // namespace dynd
