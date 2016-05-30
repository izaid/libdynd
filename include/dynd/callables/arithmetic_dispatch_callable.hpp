//
// Copyright (C) 2011-16 DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#pragma once

#include <dynd/callables/base_dispatch_callable.hpp>

namespace dynd {
namespace nd {

  template <std::vector<ndt::type> (*Func)(const ndt::type &, size_t, const ndt::type *), size_t N>
  class arithmetic_dispatch_callable;

  template <std::vector<ndt::type> (*Func)(const ndt::type &, size_t, const ndt::type *)>
  class arithmetic_dispatch_callable<Func, 1> : public base_dispatch_callable {
    dispatcher<1, callable> m_dispatcher;

  public:
    arithmetic_dispatch_callable(const ndt::type &tp, const dispatcher<1, callable> &dispatcher)
        : base_dispatch_callable(tp), m_dispatcher(dispatcher) {}

    void overload(const ndt::type &DYND_UNUSED(dst_tp), intptr_t DYND_UNUSED(nsrc),
                  const ndt::type *DYND_UNUSED(src_tp), const callable &value) {
      m_dispatcher.insert(value);
    }

    const callable &specialize(const ndt::type &DYND_UNUSED(dst_tp), intptr_t DYND_UNUSED(nsrc),
                               const ndt::type *src_tp) {
      return m_dispatcher(src_tp[0]);
    }
  };

  template <std::vector<ndt::type> (*Func)(const ndt::type &, size_t, const ndt::type *)>
  class arithmetic_dispatch_callable<Func, 2> : public base_dispatch_callable {
    dispatcher<2, callable> m_dispatcher;

  public:
    arithmetic_dispatch_callable(const ndt::type &tp, const dispatcher<2, callable> &dispatcher)
        : base_dispatch_callable(tp), m_dispatcher(dispatcher) {}

    void overload(const ndt::type &DYND_UNUSED(dst_tp), intptr_t DYND_UNUSED(nsrc),
                  const ndt::type *DYND_UNUSED(src_tp), const callable &value) {
      m_dispatcher.insert(value);
    }

    const callable &specialize(const ndt::type &DYND_UNUSED(dst_tp), intptr_t DYND_UNUSED(nsrc),
                               const ndt::type *src_tp) {
      return m_dispatcher(src_tp[0], src_tp[1]);
    }
  };

} // namespace dynd::nd
} // namespace dynd
