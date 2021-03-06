//
// Copyright (C) 2011-16 DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#pragma once

#include <dynd/kernels/init_kernel.hpp>
#include <dynd/memblock/buffer_memory_block.hpp>
#include <dynd/shortvector.hpp>

namespace dynd {
namespace nd {

  inline buffer empty_buffer(const ndt::type &tp);
  inline buffer empty_buffer(const ndt::type &tp, uint64_t flags);

  class DYNDT_API buffer : public intrusive_ptr<const buffer_memory_block> {
    template <typename T>
    void init(T &&value) {
      init_kernel<typename remove_reference_then_cv<T>::type> init(get_type(), get()->metadata());
      init.single(const_cast<char *>(cdata()), std::forward<T>(value));
    }

    template <typename ValueType>
    void init(const ValueType *values, size_t size) {
      init_kernel<ValueType> init(get_type(), get()->metadata());
      init.contiguous(const_cast<char *>(cdata()), values, size);
    }

  public:
    using intrusive_ptr<const buffer_memory_block>::intrusive_ptr;

    buffer() = default;

    /**
      * Constructs an array from a C++ type.
      */
    template <typename T,
              typename = std::enable_if_t<ndt::has_traits<typename remove_reference_then_cv<T>::type>::value>>
    buffer(T &&value) : buffer(empty_buffer(ndt::type_for(value))) {
      init(std::forward<T>(value));
    }

    /** Constructs an array from a 1D initializer list */
    template <typename ValueType>
    buffer(std::initializer_list<ValueType> values) : buffer(empty_buffer(ndt::type_for(values))) {
      init(values);
    }

    /** Constructs an array from a 2D initializer list */
    template <typename ValueType>
    buffer(std::initializer_list<std::initializer_list<ValueType>> values)
        : buffer(empty_buffer(ndt::type_for(values))) {
      init(values);
    }

    /** Constructs an array from a 3D initializer list */
    template <typename ValueType>
    buffer(std::initializer_list<std::initializer_list<std::initializer_list<ValueType>>> values)
        : buffer(empty_buffer(ndt::type_for(values))) {
      init(values);
    }

    /**
     * Constructs a 1D array from a pointer and a size.
     */
    template <typename ValueType>
    buffer(const ValueType *values, size_t size)
        : buffer(empty_buffer(ndt::make_fixed_dim(size, ndt::make_type<ValueType>()))) {
      init(values, size);
    }

    /** The type */
    const ndt::type &get_type() const { return m_ptr->m_tp; }

    const memory_block &get_owner() const { return m_ptr->m_owner; }

    /** The flags, including access permissions. */
    uint64_t get_flags() const { return m_ptr->m_flags; }

    char *data() const {
      if (m_ptr->m_flags & write_access_flag) {
        return m_ptr->m_data;
      }

      throw std::runtime_error("tried to write to a dynd array that is not writable");
    }

    const char *cdata() const { return m_ptr->m_data; }

    memory_block get_data_memblock() const;

    bool is_immutable() const { return (m_ptr->m_flags & immutable_access_flag) != 0; }

    /** Returns true if the object is a scalar */
    bool is_scalar() const { return m_ptr->m_tp.is_scalar(); }

    intptr_t get_ndim() const {
      if (m_ptr->m_tp.is_builtin()) {
        return 0;
      }

      return m_ptr->m_tp->get_ndim();
    }

    /**
     * The data type of the array. This is similar to numpy's
     * ndarray.dtype property
     */
    ndt::type get_dtype() const {
      size_t ndim = get_type().get_ndim();
      if (ndim == 0) {
        return get_type();
      }

      return get_type()->get_type_at_dimension(NULL, ndim);
    }

    /**
     * The data type of the array. This is similar to numpy's
     * ndarray.dtype property, but may include some array dimensions
     * if requested.
     *
     * \param include_ndim  The number of array dimensions to include
     *                   in the data type.
     */
    ndt::type get_dtype(size_t include_ndim) const {
      if (get_type().is_builtin()) {
        if (include_ndim > 0) {
          throw too_many_indices(get_type(), include_ndim, 0);
        }
        return get_type();
      } else {
        size_t ndim = get_type()->get_ndim();
        if (ndim < include_ndim) {
          throw too_many_indices(get_type(), include_ndim, ndim);
        }
        ndim -= include_ndim;
        if (ndim == 0) {
          return get_type();
        } else {
          return get_type()->get_type_at_dimension(NULL, ndim);
        }
      }
    }

    /** Returns true if the array is NULL */
    bool is_null() const { return m_ptr == NULL; }

    std::vector<intptr_t> get_shape() const {
      std::vector<intptr_t> result(get_ndim());
      get_shape(&result[0]);
      return result;
    }

    void get_shape(intptr_t *out_shape) const {
      if (!get_type().is_builtin() && get_type()->get_ndim() > 0) {
        get_type()->get_shape(get_type()->get_ndim(), 0, out_shape, get()->metadata(), cdata());
      }
    }

    /**
     * Returns the size of the leading (leftmost) dimension.
     */
    intptr_t get_dim_size() const { return get_type().get_dim_size(get()->metadata(), cdata()); }

    /**
     * Returns the size of the requested dimension.
     */
    intptr_t get_dim_size(intptr_t i) const {
      if (0 <= i && i < get_type().get_strided_ndim()) {
        const size_stride_t *ss = reinterpret_cast<const size_stride_t *>(get()->metadata());
        return ss[i].dim_size;
      } else if (0 <= i && i < get_ndim()) {
        dimvector shape(i + 1);
        get_type()->get_shape(i + 1, 0, shape.get(), get()->metadata(), cdata());
        return shape[i];
      } else {
        std::stringstream ss;
        ss << "Not enough dimensions in array, tried to access axis " << i << " for type " << get_type();
        throw std::invalid_argument(ss.str());
      }
    }

    std::vector<intptr_t> get_strides() const {
      std::vector<intptr_t> result(get_ndim());
      get_strides(&result[0]);
      return result;
    }

    void get_strides(intptr_t *out_strides) const {
      if (!get_type().is_builtin()) {
        get_type()->get_strides(0, out_strides, get()->metadata());
      }
    }

    void debug_print(std::ostream &o, const std::string &indent = "") const;
  };

  inline buffer make_buffer(const ndt::type &tp, uint64_t flags) {
    if (tp.is_symbolic()) {
      std::stringstream ss;
      ss << "Cannot create a dynd buffer with symbolic type " << tp;
      throw type_error(ss.str());
    }

    size_t data_offset = inc_to_alignment(sizeof(buffer_memory_block) + tp.get_arrmeta_size(), tp.get_data_alignment());
    size_t data_size = tp.get_default_data_size();

    return buffer(new (data_offset + data_size - sizeof(buffer_memory_block))
                      buffer_memory_block(tp, data_offset, data_size, flags),
                  false);
  }

  inline buffer empty_buffer(const ndt::type &tp, uint64_t flags) {
    // Create an empty shell
    buffer res = make_buffer(tp, flags);
    // Construct the arrmeta with default settings
    if (tp.get_arrmeta_size() > 0) {
      res.get_type()->arrmeta_default_construct(res->metadata(), true);
    }

    return res;
  }

  inline buffer empty_buffer(const ndt::type &tp) {
    return empty_buffer(tp, (tp.get_ndim() == 0) ? (read_access_flag | immutable_access_flag) : readwrite_access_flags);
  }

  inline buffer make_buffer(const ndt::type &tp, char *data, uint64_t flags) {
    return buffer(new (tp.get_arrmeta_size()) buffer_memory_block(tp, data, flags), false);
  }

  inline buffer make_buffer(const ndt::type &tp, char *data, const memory_block &owner, uint64_t flags) {
    return buffer(new (tp.get_arrmeta_size()) buffer_memory_block(tp, data, owner, flags), false);
  }

} // namespace dynd::nd
} // namespace dynd
