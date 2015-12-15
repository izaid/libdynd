//
// Copyright (C) 2011-15 DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#pragma once

#include <dynd/diagnostics.hpp>
#include <dynd/callable.hpp>

namespace dynd {

/**
 * Function for byteswapping a single value.
 */
inline uint16_t byteswap_value(uint16_t value) { return ((value & 0xffu) << 8) | (value >> 8); }

/**
 * Function for byteswapping a single value.
 */
inline uint32_t byteswap_value(uint32_t value)
{
  return ((value & 0xffu) << 24) | ((value & 0xff00u) << 8) | ((value & 0xff0000u) >> 8) | (value >> 24);
}

/**
 * Function for byteswapping a single value.
 */
inline uint64_t byteswap_value(uint64_t value)
{
  return ((value & 0xffULL) << 56) | ((value & 0xff00ULL) << 40) | ((value & 0xff0000ULL) << 24) |
         ((value & 0xff000000ULL) << 8) | ((value & 0xff00000000ULL) >> 8) | ((value & 0xff0000000000ULL) >> 24) |
         ((value & 0xff000000000000ULL) >> 40) | (value >> 56);
}

namespace nd {

  struct byteswap_ck : base_kernel<byteswap_ck, 1> {
    size_t data_size;

    byteswap_ck(size_t data_size) : data_size(data_size) {}

    void single(char *dst, char *const *src)
    {
      // Do a different loop for in-place swap versus copying swap,
      // so this one kernel function works correctly for both cases.
      if (src[0] == dst) {
        // In-place swap
        for (size_t j = 0; j < data_size / 2; ++j) {
          std::swap(dst[j], dst[data_size - j - 1]);
        }
      }
      else {
        for (size_t j = 0; j < data_size; ++j) {
          dst[j] = src[0][data_size - j - 1];
        }
      }
    }

    static intptr_t instantiate(char *DYND_UNUSED(static_data), char *DYND_UNUSED(data), void *ckb, intptr_t ckb_offset,
                                const ndt::type &DYND_UNUSED(dst_tp), const char *DYND_UNUSED(dst_arrmeta),
                                intptr_t DYND_UNUSED(nsrc), const ndt::type *src_tp,
                                const char *const *DYND_UNUSED(src_arrmeta), kernel_request_t kernreq,
                                const eval::eval_context *DYND_UNUSED(ectx), intptr_t DYND_UNUSED(nkwd),
                                const nd::array *DYND_UNUSED(kwds),
                                const std::map<std::string, ndt::type> &DYND_UNUSED(tp_vars))
    {
      make(ckb, kernreq, ckb_offset, src_tp[0].get_data_size());
      return ckb_offset;
    }
  };

  struct pairwise_byteswap_ck : base_kernel<pairwise_byteswap_ck, 1> {
    size_t data_size;

    pairwise_byteswap_ck(size_t data_size) : data_size(data_size) {}

    void single(char *dst, char *const *src)
    {
      // Do a different loop for in-place swap versus copying swap,
      // so this one kernel function works correctly for both cases.
      if (src[0] == dst) {
        // In-place swap
        for (size_t j = 0; j < data_size / 4; ++j) {
          std::swap(dst[j], dst[data_size / 2 - j - 1]);
        }
        for (size_t j = 0; j < data_size / 4; ++j) {
          std::swap(dst[data_size / 2 + j], dst[data_size - j - 1]);
        }
      }
      else {
        for (size_t j = 0; j < data_size / 2; ++j) {
          dst[j] = src[0][data_size / 2 - j - 1];
        }
        for (size_t j = 0; j < data_size / 2; ++j) {
          dst[data_size / 2 + j] = src[0][data_size - j - 1];
        }
      }
    }

    static intptr_t instantiate(char *DYND_UNUSED(static_data), char *DYND_UNUSED(data), void *ckb, intptr_t ckb_offset,
                                const ndt::type &DYND_UNUSED(dst_tp), const char *DYND_UNUSED(dst_arrmeta),
                                intptr_t DYND_UNUSED(nsrc), const ndt::type *src_tp,
                                const char *const *DYND_UNUSED(src_arrmeta), kernel_request_t kernreq,
                                const eval::eval_context *DYND_UNUSED(ectx), intptr_t DYND_UNUSED(nkwd),
                                const nd::array *DYND_UNUSED(kwds),
                                const std::map<std::string, ndt::type> &DYND_UNUSED(tp_vars))
    {
      make(ckb, kernreq, ckb_offset, src_tp[0].get_data_size());
      return ckb_offset;
    }
  };

  extern DYND_API struct byteswap : declfunc<byteswap> {
    static DYND_API callable make();
  } byteswap;

  extern DYND_API struct pairwise_byteswap : declfunc<pairwise_byteswap> {
    static DYND_API callable make();
  } pairwise_byteswap;

} // namespace dynd::nd
} // namespace dynd
