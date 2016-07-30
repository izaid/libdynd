//
// Copyright (C) 2011-16 DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#pragma once

#include <stdexcept>

#include <dynd/assignment.hpp>
#include <dynd/eval/eval_context.hpp>
#include <dynd/fpstatus.hpp>
#include <dynd/kernels/base_kernel.hpp>
#include <dynd/kernels/cuda_launch.hpp>
#include <dynd/kernels/tuple_assignment_kernels.hpp>
#include <dynd/math.hpp>
#include <dynd/option.hpp>
#include <dynd/parse.hpp>
#include <dynd/type.hpp>
#include <dynd/types/adapt_type.hpp>
#include <dynd/types/categorical_type.hpp>
#include <dynd/types/char_type.hpp>
#include <dynd/types/fixed_bytes_type.hpp>
#include <dynd/types/fixed_string_type.hpp>
#include <dynd/types/option_type.hpp>
#include <dynd/types/type_id.hpp>
#include <map>

#if defined(_MSC_VER)
// Tell the visual studio compiler we're accessing the FPU flags
#pragma fenv_access(on)
#endif

namespace dynd {
namespace nd {

  template <typename, typename>
  class assign_callable;
}

const inexact_check_t inexact_check = inexact_check_t();

// Trim taken from boost string algorithms library
// Trim taken from boost string algorithms library
template <typename ForwardIteratorT>
inline ForwardIteratorT trim_begin(ForwardIteratorT InBegin, ForwardIteratorT InEnd) {
  ForwardIteratorT It = InBegin;
  for (; It != InEnd; ++It) {
    if (!isspace(*It))
      return It;
  }

  return It;
}
template <typename ForwardIteratorT>
inline ForwardIteratorT trim_end(ForwardIteratorT InBegin, ForwardIteratorT InEnd) {
  for (ForwardIteratorT It = InEnd; It != InBegin;) {
    if (!isspace(*(--It)))
      return ++It;
  }

  return InBegin;
}
template <typename SequenceT>
inline void trim_left_if(SequenceT &Input) {
  Input.erase(Input.begin(), trim_begin(Input.begin(), Input.end()));
}
template <typename SequenceT>
inline void trim_right_if(SequenceT &Input) {
  Input.erase(trim_end(Input.begin(), Input.end()), Input.end());
}
template <typename SequenceT>
inline void trim(SequenceT &Input) {
  trim_right_if(Input);
  trim_left_if(Input);
}
// End trim taken from boost string algorithms
inline void to_lower(std::string &s) {
  for (size_t i = 0, i_end = s.size(); i != i_end; ++i) {
    s[i] = tolower(s[i]);
  }
}

namespace nd {
  namespace detail {

    template <typename ReturnType, typename Arg0Type, typename Enable = void>
    struct assignment_virtual_kernel;

    template <typename ReturnType, typename Arg0Type, assign_error_mode ErrorMode, typename Enable = void>
    struct assignment_kernel : base_strided_kernel<assignment_kernel<ReturnType, Arg0Type, ErrorMode>, 1> {
      void single(char *dst, char *const *src) {
        DYND_TRACE_ASSIGNMENT(static_cast<ReturnType>(*reinterpret_cast<Arg0Type *>(src[0])), ReturnType,
                              *reinterpret_cast<Arg0Type *>(src[0]), Arg0Type);

// This warning is being triggered in spite of the explicit cast, unfortunately have to suppress manually
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4244)
#endif
        *reinterpret_cast<ReturnType *>(dst) = static_cast<ReturnType>(*reinterpret_cast<Arg0Type *>(src[0]));
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
      }
    };

    // Complex floating point -> non-complex with no error checking
    template <typename ReturnType, typename Arg0Type>
    struct assignment_kernel<ReturnType, Arg0Type, assign_error_nocheck,
                             std::enable_if_t<is_signed_integral<ReturnType>::value && is_complex<Arg0Type>::value>>
        : base_strided_kernel<assignment_kernel<ReturnType, Arg0Type, assign_error_nocheck>, 1> {
      void single(char *dst, char *const *src) {
        Arg0Type s = *reinterpret_cast<Arg0Type *>(src[0]);

        DYND_TRACE_ASSIGNMENT(static_cast<ReturnType>(s.real()), ReturnType, s, Arg0Type);

        *reinterpret_cast<ReturnType *>(dst) = static_cast<ReturnType>(s.real());
      }
    };

    template <typename ReturnType, typename Arg0Type>
    struct assignment_kernel<ReturnType, Arg0Type, assign_error_nocheck,
                             std::enable_if_t<is_unsigned_integral<ReturnType>::value && is_complex<Arg0Type>::value>>
        : base_strided_kernel<assignment_kernel<ReturnType, Arg0Type, assign_error_nocheck>, 1> {
      void single(char *dst, char *const *src) {
        Arg0Type s = *reinterpret_cast<Arg0Type *>(src[0]);

        DYND_TRACE_ASSIGNMENT(static_cast<ReturnType>(s.real()), ReturnType, s, Arg0Type);

        *reinterpret_cast<ReturnType *>(dst) = static_cast<ReturnType>(s.real());
      }
    };

    template <typename ReturnType, typename Arg0Type>
    struct assignment_kernel<ReturnType, Arg0Type, assign_error_nocheck,
                             std::enable_if_t<is_floating_point<ReturnType>::value && is_complex<Arg0Type>::value>>
        : base_strided_kernel<assignment_kernel<ReturnType, Arg0Type, assign_error_nocheck>, 1> {
      void single(char *dst, char *const *src) {
        Arg0Type s = *reinterpret_cast<Arg0Type *>(src[0]);

        DYND_TRACE_ASSIGNMENT(static_cast<ReturnType>(s.real()), ReturnType, s, Arg0Type);

        *reinterpret_cast<ReturnType *>(dst) = static_cast<ReturnType>(s.real());
      }
    };

    // Signed int -> complex floating point with no checking
    template <typename ReturnType, typename Arg0Type>
    struct assignment_kernel<ReturnType, Arg0Type, assign_error_nocheck,
                             std::enable_if_t<is_complex<ReturnType>::value && is_signed_integral<Arg0Type>::value>>
        : base_strided_kernel<assignment_kernel<ReturnType, Arg0Type, assign_error_nocheck>, 1> {
      void single(char *dst, char *const *src) {
        Arg0Type s = *reinterpret_cast<Arg0Type *>(src[0]);

        DYND_TRACE_ASSIGNMENT(d, ReturnType, s, Arg0Type);

        *reinterpret_cast<ReturnType *>(dst) = static_cast<typename ReturnType::value_type>(s);
      }
    };

    // Signed int -> complex floating point with inexact checking
    template <typename ReturnType, typename Arg0Type>
    struct assignment_kernel<ReturnType, Arg0Type, assign_error_inexact,
                             std::enable_if_t<is_complex<ReturnType>::value && is_signed_integral<Arg0Type>::value>>
        : base_strided_kernel<assignment_kernel<ReturnType, Arg0Type, assign_error_inexact>, 1> {
      void single(char *dst, char *const *src) {
        *reinterpret_cast<ReturnType *>(dst) =
            check_cast<ReturnType>(*reinterpret_cast<Arg0Type *>(src[0]), inexact_check);
      }
    };

    // Signed int -> complex floating point with other checking
    template <typename ReturnType, typename Arg0Type>
    struct assignment_kernel<ReturnType, Arg0Type, assign_error_overflow,
                             std::enable_if_t<is_complex<ReturnType>::value && is_signed_integral<Arg0Type>::value>>
        : assignment_kernel<ReturnType, Arg0Type, assign_error_nocheck> {};

    template <typename ReturnType, typename Arg0Type>
    struct assignment_kernel<ReturnType, Arg0Type, assign_error_fractional,
                             std::enable_if_t<is_complex<ReturnType>::value && is_signed_integral<Arg0Type>::value>>
        : assignment_kernel<ReturnType, Arg0Type, assign_error_nocheck> {};

    // Anything -> boolean with no checking
    template <typename Arg0Type>
    struct assignment_kernel<bool1, Arg0Type, assign_error_nocheck>
        : base_strided_kernel<assignment_kernel<bool1, Arg0Type, assign_error_nocheck>, 1> {
      void single(char *dst, char *const *src) {
        Arg0Type s = *reinterpret_cast<Arg0Type *>(src[0]);

        DYND_TRACE_ASSIGNMENT((bool)(s != Arg0Type(0)), bool1, s, Arg0Type);

        *reinterpret_cast<bool1 *>(dst) = (s != Arg0Type(0));
      }
    };

    // Unsigned int -> floating point with inexact checking
    template <typename ReturnType, typename Arg0Type>
    struct assignment_kernel<
        ReturnType, Arg0Type, assign_error_inexact,
        std::enable_if_t<is_floating_point<ReturnType>::value && is_unsigned_integral<Arg0Type>::value>>
        : base_strided_kernel<assignment_kernel<ReturnType, Arg0Type, assign_error_inexact>, 1> {
      void single(char *dst, char *const *src) {
        *reinterpret_cast<ReturnType *>(dst) =
            check_cast<ReturnType>(*reinterpret_cast<Arg0Type *>(src[0]), inexact_check);
      }
    };

    // Unsigned int -> floating point with other checking
    template <typename ReturnType, typename Arg0Type>
    struct assignment_kernel<
        ReturnType, Arg0Type, assign_error_overflow,
        std::enable_if_t<is_floating_point<ReturnType>::value && is_unsigned_integral<Arg0Type>::value>>
        : assignment_kernel<ReturnType, Arg0Type, assign_error_nocheck> {};

    template <typename ReturnType, typename Arg0Type>
    struct assignment_kernel<
        ReturnType, Arg0Type, assign_error_fractional,
        std::enable_if_t<is_floating_point<ReturnType>::value && is_unsigned_integral<Arg0Type>::value>>
        : assignment_kernel<ReturnType, Arg0Type, assign_error_nocheck> {};

    // Unsigned int -> complex floating point with no checking
    template <typename ReturnType, typename Arg0Type>
    struct assignment_kernel<ReturnType, Arg0Type, assign_error_nocheck,
                             std::enable_if_t<is_complex<ReturnType>::value && is_unsigned_integral<Arg0Type>::value>>
        : base_strided_kernel<assignment_kernel<ReturnType, Arg0Type, assign_error_nocheck>, 1> {
      void single(char *dst, char *const *src) {
        Arg0Type s = *reinterpret_cast<Arg0Type *>(src[0]);

        DYND_TRACE_ASSIGNMENT(d, ReturnType, s, Arg0Type);

        *reinterpret_cast<ReturnType *>(dst) = static_cast<typename ReturnType::value_type>(s);
      }
    };

    // Unsigned int -> complex floating point with inexact checking
    template <typename ReturnType, typename Arg0Type>
    struct assignment_kernel<ReturnType, Arg0Type, assign_error_inexact,
                             std::enable_if_t<is_complex<ReturnType>::value && is_unsigned_integral<Arg0Type>::value>>
        : base_strided_kernel<assignment_kernel<ReturnType, Arg0Type, assign_error_inexact>, 1> {
      void single(char *dst, char *const *src) {
        *reinterpret_cast<ReturnType *>(dst) =
            check_cast<ReturnType>(*reinterpret_cast<Arg0Type *>(src[0]), inexact_check);
      }
    };

    // Unsigned int -> complex floating point with other checking
    template <typename ReturnType, typename Arg0Type>
    struct assignment_kernel<ReturnType, Arg0Type, assign_error_overflow,
                             std::enable_if_t<is_complex<ReturnType>::value && is_unsigned_integral<Arg0Type>::value>>
        : assignment_kernel<ReturnType, Arg0Type, assign_error_nocheck> {};

    template <typename ReturnType, typename Arg0Type>
    struct assignment_kernel<ReturnType, Arg0Type, assign_error_fractional,
                             std::enable_if_t<is_complex<ReturnType>::value && is_unsigned_integral<Arg0Type>::value>>
        : assignment_kernel<ReturnType, Arg0Type, assign_error_nocheck> {};

    // Floating point -> signed int with overflow checking
    template <typename ReturnType, typename Arg0Type>
    struct assignment_kernel<
        ReturnType, Arg0Type, assign_error_overflow,
        std::enable_if_t<is_signed_integral<ReturnType>::value && is_floating_point<Arg0Type>::value>>
        : base_strided_kernel<assignment_kernel<ReturnType, Arg0Type, assign_error_overflow>, 1> {
      void single(char *dst, char *const *src) {
        *reinterpret_cast<ReturnType *>(dst) = overflow_cast<ReturnType>(*reinterpret_cast<Arg0Type *>(src[0]));
      }
    };

    // Floating point -> signed int with fractional checking
    template <typename ReturnType, typename Arg0Type>
    struct assignment_kernel<
        ReturnType, Arg0Type, assign_error_fractional,
        std::enable_if_t<is_signed_integral<ReturnType>::value && is_floating_point<Arg0Type>::value>>
        : base_strided_kernel<assignment_kernel<ReturnType, Arg0Type, assign_error_fractional>, 1> {
      void single(char *dst, char *const *src) {
        *reinterpret_cast<ReturnType *>(dst) = fractional_cast<ReturnType>(*reinterpret_cast<Arg0Type *>(src[0]));
      }
    };

    // Floating point -> signed int with other checking
    template <typename ReturnType, typename Arg0Type>
    struct assignment_kernel<
        ReturnType, Arg0Type, assign_error_inexact,
        std::enable_if_t<is_signed_integral<ReturnType>::value && is_floating_point<Arg0Type>::value>>
        : assignment_kernel<ReturnType, Arg0Type, assign_error_fractional> {};

    // Complex floating point -> signed int with overflow checking
    template <typename ReturnType, typename Arg0Type>
    struct assignment_kernel<ReturnType, Arg0Type, assign_error_overflow,
                             std::enable_if_t<is_signed_integral<ReturnType>::value && is_complex<Arg0Type>::value>>
        : base_strided_kernel<assignment_kernel<ReturnType, Arg0Type, assign_error_overflow>, 1> {
      void single(char *dst, char *const *src) {
        *reinterpret_cast<ReturnType *>(dst) = overflow_cast<ReturnType>(*reinterpret_cast<Arg0Type *>(src[0]));
      }
    };

    // Complex floating point -> signed int with fractional checking
    template <typename ReturnType, typename Arg0Type>
    struct assignment_kernel<ReturnType, Arg0Type, assign_error_fractional,
                             std::enable_if_t<is_signed_integral<ReturnType>::value && is_complex<Arg0Type>::value>>
        : base_strided_kernel<assignment_kernel<ReturnType, Arg0Type, assign_error_fractional>, 1> {
      void single(char *dst, char *const *src) {
        *reinterpret_cast<ReturnType *>(dst) = fractional_cast<ReturnType>(*reinterpret_cast<Arg0Type *>(src[0]));
      }
    };

    // Complex floating point -> signed int with other checking
    template <typename ReturnType, typename Arg0Type>
    struct assignment_kernel<ReturnType, Arg0Type, assign_error_inexact,
                             std::enable_if_t<is_signed_integral<ReturnType>::value && is_complex<Arg0Type>::value>>
        : assignment_kernel<ReturnType, Arg0Type, assign_error_fractional> {};

    // Floating point -> unsigned int with overflow checking
    template <typename ReturnType, typename Arg0Type>
    struct assignment_kernel<
        ReturnType, Arg0Type, assign_error_overflow,
        std::enable_if_t<is_unsigned_integral<ReturnType>::value && is_floating_point<Arg0Type>::value>>
        : base_strided_kernel<assignment_kernel<ReturnType, Arg0Type, assign_error_overflow>, 1> {
      void single(char *dst, char *const *src) {
        *reinterpret_cast<ReturnType *>(dst) = overflow_cast<ReturnType>(*reinterpret_cast<Arg0Type *>(src[0]));
      }
    };

    // Floating point -> unsigned int with fractional checking
    template <typename ReturnType, typename Arg0Type>
    struct assignment_kernel<
        ReturnType, Arg0Type, assign_error_fractional,
        std::enable_if_t<is_unsigned_integral<ReturnType>::value && is_floating_point<Arg0Type>::value>>
        : base_strided_kernel<assignment_kernel<ReturnType, Arg0Type, assign_error_fractional>, 1> {
      void single(char *dst, char *const *src) {
        *reinterpret_cast<ReturnType *>(dst) = fractional_cast<ReturnType>(*reinterpret_cast<Arg0Type *>(src[0]));
      }
    };

    // Floating point -> unsigned int with other checking
    template <typename ReturnType, typename Arg0Type>
    struct assignment_kernel<
        ReturnType, Arg0Type, assign_error_inexact,
        std::enable_if_t<is_unsigned_integral<ReturnType>::value && is_floating_point<Arg0Type>::value>>
        : assignment_kernel<ReturnType, Arg0Type, assign_error_fractional> {};

    // Complex floating point -> unsigned int with overflow checking
    template <typename ReturnType, typename Arg0Type>
    struct assignment_kernel<ReturnType, Arg0Type, assign_error_overflow,
                             std::enable_if_t<is_unsigned_integral<ReturnType>::value && is_complex<Arg0Type>::value>>
        : base_strided_kernel<assignment_kernel<ReturnType, Arg0Type, assign_error_overflow>, 1> {
      void single(char *dst, char *const *src) {
        *reinterpret_cast<ReturnType *>(dst) = overflow_cast<ReturnType>(*reinterpret_cast<Arg0Type *>(src[0]));
      }
    };

    // Complex floating point -> unsigned int with fractional checking
    template <typename ReturnType, typename Arg0Type>
    struct assignment_kernel<ReturnType, Arg0Type, assign_error_fractional,
                             std::enable_if_t<is_unsigned_integral<ReturnType>::value && is_complex<Arg0Type>::value>>
        : base_strided_kernel<assignment_kernel<ReturnType, Arg0Type, assign_error_fractional>, 1> {
      void single(char *dst, char *const *src) {
        *reinterpret_cast<ReturnType *>(dst) = fractional_cast<ReturnType>(*reinterpret_cast<Arg0Type *>(src[0]));
      }
    };

    // Complex floating point -> unsigned int with other checking
    template <typename ReturnType, typename Arg0Type>
    struct assignment_kernel<ReturnType, Arg0Type, assign_error_inexact,
                             std::enable_if_t<is_unsigned_integral<ReturnType>::value && is_complex<Arg0Type>::value>>
        : assignment_kernel<ReturnType, Arg0Type, assign_error_fractional> {};

    // float -> float with no checking
    template <>
    struct assignment_kernel<float, float, assign_error_overflow>
        : assignment_kernel<float, float, assign_error_nocheck> {};

    template <>
    struct assignment_kernel<float, float, assign_error_fractional>
        : assignment_kernel<float, float, assign_error_nocheck> {};

    template <>
    struct assignment_kernel<float, float, assign_error_inexact>
        : assignment_kernel<float, float, assign_error_nocheck> {};

    // complex<float> -> complex<float> with no checking
    template <>
    struct assignment_kernel<complex<float>, complex<float>, assign_error_overflow>
        : assignment_kernel<complex<float>, complex<float>, assign_error_nocheck> {};

    template <>
    struct assignment_kernel<complex<float>, complex<float>, assign_error_fractional>
        : assignment_kernel<complex<float>, complex<float>, assign_error_nocheck> {};

    template <>
    struct assignment_kernel<complex<float>, complex<float>, assign_error_inexact>
        : assignment_kernel<complex<float>, complex<float>, assign_error_nocheck> {};

    // float -> double with no checking
    template <>
    struct assignment_kernel<double, float, assign_error_overflow>
        : assignment_kernel<double, float, assign_error_nocheck> {};

    template <>
    struct assignment_kernel<double, float, assign_error_fractional>
        : assignment_kernel<double, float, assign_error_nocheck> {};

    template <>
    struct assignment_kernel<double, float, assign_error_inexact>
        : assignment_kernel<double, float, assign_error_nocheck> {};

    // complex<float> -> complex<double> with no checking
    template <>
    struct assignment_kernel<complex<double>, complex<float>, assign_error_overflow>
        : assignment_kernel<complex<double>, complex<float>, assign_error_nocheck> {};

    template <>
    struct assignment_kernel<complex<double>, complex<float>, assign_error_fractional>
        : assignment_kernel<complex<double>, complex<float>, assign_error_nocheck> {};

    template <>
    struct assignment_kernel<complex<double>, complex<float>, assign_error_inexact>
        : assignment_kernel<complex<double>, complex<float>, assign_error_nocheck> {};

    // double -> double with no checking
    template <>
    struct assignment_kernel<double, double, assign_error_overflow>
        : assignment_kernel<double, double, assign_error_nocheck> {};

    template <>
    struct assignment_kernel<double, double, assign_error_fractional>
        : assignment_kernel<double, double, assign_error_nocheck> {};

    template <>
    struct assignment_kernel<double, double, assign_error_inexact>
        : assignment_kernel<double, double, assign_error_nocheck> {};

    // complex<double> -> complex<double> with no checking
    template <>
    struct assignment_kernel<complex<double>, complex<double>, assign_error_overflow>
        : assignment_kernel<complex<double>, complex<double>, assign_error_nocheck> {};

    template <>
    struct assignment_kernel<complex<double>, complex<double>, assign_error_fractional>
        : assignment_kernel<complex<double>, complex<double>, assign_error_nocheck> {};

    template <>
    struct assignment_kernel<complex<double>, complex<double>, assign_error_inexact>
        : assignment_kernel<complex<double>, complex<double>, assign_error_nocheck> {};

    // real -> real with overflow checking
    template <typename ReturnType, typename Arg0Type>
    struct assignment_kernel<
        ReturnType, Arg0Type, assign_error_overflow,
        std::enable_if_t<is_floating_point<ReturnType>::value && is_floating_point<Arg0Type>::value>>
        : base_strided_kernel<assignment_kernel<ReturnType, Arg0Type, assign_error_overflow>, 1> {
      void single(char *dst, char *const *src) {
        *reinterpret_cast<ReturnType *>(dst) = overflow_cast<ReturnType>(*reinterpret_cast<Arg0Type *>(src[0]));
      }
    };

    // real -> real with fractional checking
    template <typename ReturnType, typename Arg0Type>
    struct assignment_kernel<
        ReturnType, Arg0Type, assign_error_fractional,
        std::enable_if_t<is_floating_point<ReturnType>::value && is_floating_point<Arg0Type>::value>>
        : assignment_kernel<ReturnType, Arg0Type, assign_error_overflow> {};

    // real -> real with inexact checking
    template <typename ReturnType, typename Arg0Type>
    struct assignment_kernel<
        ReturnType, Arg0Type, assign_error_inexact,
        std::enable_if_t<is_floating_point<ReturnType>::value && is_floating_point<Arg0Type>::value>>
        : base_strided_kernel<assignment_kernel<ReturnType, Arg0Type, assign_error_inexact>, 1> {
      void single(char *dst, char *const *src) {
        *reinterpret_cast<ReturnType *>(dst) =
            check_cast<ReturnType>(*reinterpret_cast<Arg0Type *>(src[0]), inexact_check);
      }
    };

    // Anything -> boolean with overflow checking
    template <typename Arg0Type>
    struct assignment_kernel<bool1, Arg0Type, assign_error_overflow>
        : base_strided_kernel<assignment_kernel<bool1, Arg0Type, assign_error_overflow>, 1> {
      void single(char *dst, char *const *src) {
        *reinterpret_cast<bool1 *>(dst) = overflow_cast<bool1>(*reinterpret_cast<Arg0Type *>(src[0]));
      }
    };

    // Anything -> boolean with other error checking
    template <typename Arg0Type>
    struct assignment_kernel<bool1, Arg0Type, assign_error_fractional>
        : assignment_kernel<bool1, Arg0Type, assign_error_overflow> {};

    template <typename Arg0Type>
    struct assignment_kernel<bool1, Arg0Type, assign_error_inexact>
        : assignment_kernel<bool1, Arg0Type, assign_error_overflow> {};

    // Boolean -> boolean with other error checking
    template <>
    struct assignment_kernel<bool1, bool1, assign_error_overflow>
        : assignment_kernel<bool1, bool1, assign_error_nocheck> {};

    template <>
    struct assignment_kernel<bool1, bool1, assign_error_fractional>
        : assignment_kernel<bool1, bool1, assign_error_nocheck> {};

    template <>
    struct assignment_kernel<bool1, bool1, assign_error_inexact>
        : assignment_kernel<bool1, bool1, assign_error_nocheck> {};

    // Boolean -> anything with other error checking
    template <typename ReturnType>
    struct assignment_kernel<ReturnType, bool1, assign_error_overflow>
        : assignment_kernel<ReturnType, bool1, assign_error_nocheck> {};

    template <typename ReturnType>
    struct assignment_kernel<ReturnType, bool1, assign_error_fractional>
        : assignment_kernel<ReturnType, bool1, assign_error_nocheck> {};

    template <typename ReturnType>
    struct assignment_kernel<ReturnType, bool1, assign_error_inexact>
        : assignment_kernel<ReturnType, bool1, assign_error_nocheck> {};

    // Signed int -> signed int with overflow checking
    template <typename ReturnType, typename Arg0Type>
    struct assignment_kernel<
        ReturnType, Arg0Type, assign_error_overflow,
        std::enable_if_t<is_signed_integral<ReturnType>::value && is_signed_integral<Arg0Type>::value>>
        : base_strided_kernel<assignment_kernel<ReturnType, Arg0Type, assign_error_overflow>, 1> {
      void single(char *dst, char *const *src) {
        *reinterpret_cast<ReturnType *>(dst) = overflow_cast<ReturnType>(*reinterpret_cast<Arg0Type *>(src[0]));
      }
    };

    // Signed int -> signed int with other error checking
    template <typename ReturnType, typename Arg0Type>
    struct assignment_kernel<
        ReturnType, Arg0Type, assign_error_fractional,
        std::enable_if_t<is_signed_integral<ReturnType>::value && is_signed_integral<Arg0Type>::value>>
        : assignment_kernel<ReturnType, Arg0Type, assign_error_overflow> {};

    template <typename ReturnType, typename Arg0Type>
    struct assignment_kernel<
        ReturnType, Arg0Type, assign_error_inexact,
        std::enable_if_t<is_signed_integral<ReturnType>::value && is_signed_integral<Arg0Type>::value>>
        : assignment_kernel<ReturnType, Arg0Type, assign_error_overflow> {};

    // Unsigned int -> signed int with overflow checking just when sizeof(dst) <= sizeof(src)
    template <typename ReturnType, typename Arg0Type>
    struct assignment_kernel<
        ReturnType, Arg0Type, assign_error_overflow,
        std::enable_if_t<is_signed_integral<ReturnType>::value && is_unsigned_integral<Arg0Type>::value>>
        : base_strided_kernel<assignment_kernel<ReturnType, Arg0Type, assign_error_overflow>, 1> {
      void single(char *dst, char *const *src) {
        *reinterpret_cast<ReturnType *>(dst) = overflow_cast<ReturnType>(*reinterpret_cast<Arg0Type *>(src[0]));
      }
    };

    // Unsigned int -> signed int with other error checking
    template <typename ReturnType, typename Arg0Type>
    struct assignment_kernel<
        ReturnType, Arg0Type, assign_error_fractional,
        std::enable_if_t<is_signed_integral<ReturnType>::value && is_unsigned_integral<Arg0Type>::value>>
        : assignment_kernel<ReturnType, Arg0Type, assign_error_overflow> {};

    template <typename ReturnType, typename Arg0Type>
    struct assignment_kernel<
        ReturnType, Arg0Type, assign_error_inexact,
        std::enable_if_t<is_signed_integral<ReturnType>::value && is_unsigned_integral<Arg0Type>::value>>
        : assignment_kernel<ReturnType, Arg0Type, assign_error_overflow> {};

    // Signed int -> unsigned int with positive overflow checking just when sizeof(dst) < sizeof(src)
    template <typename ReturnType, typename Arg0Type>
    struct assignment_kernel<
        ReturnType, Arg0Type, assign_error_overflow,
        std::enable_if_t<is_unsigned_integral<ReturnType>::value && is_signed_integral<Arg0Type>::value>>
        : base_strided_kernel<assignment_kernel<ReturnType, Arg0Type, assign_error_overflow>, 1> {
      void single(char *dst, char *const *src) {
        *reinterpret_cast<ReturnType *>(dst) = overflow_cast<ReturnType>(*reinterpret_cast<Arg0Type *>(src[0]));
      }
    };

    // Signed int -> unsigned int with other error checking
    template <typename ReturnType, typename Arg0Type>
    struct assignment_kernel<
        ReturnType, Arg0Type, assign_error_fractional,
        std::enable_if_t<is_unsigned_integral<ReturnType>::value && is_signed_integral<Arg0Type>::value>>
        : assignment_kernel<ReturnType, Arg0Type, assign_error_overflow> {};

    template <typename ReturnType, typename Arg0Type>
    struct assignment_kernel<
        ReturnType, Arg0Type, assign_error_inexact,
        std::enable_if_t<is_unsigned_integral<ReturnType>::value && is_signed_integral<Arg0Type>::value>>
        : assignment_kernel<ReturnType, Arg0Type, assign_error_overflow> {};

    // Unsigned int -> unsigned int with overflow checking just when sizeof(dst) < sizeof(src)
    template <typename ReturnType, typename Arg0Type>
    struct assignment_kernel<
        ReturnType, Arg0Type, assign_error_overflow,
        std::enable_if_t<is_unsigned_integral<ReturnType>::value && is_unsigned_integral<Arg0Type>::value>>
        : base_strided_kernel<assignment_kernel<ReturnType, Arg0Type, assign_error_overflow>, 1> {
      void single(char *dst, char *const *src) {
        *reinterpret_cast<ReturnType *>(dst) = overflow_cast<ReturnType>(*reinterpret_cast<Arg0Type *>(src[0]));
      }
    };

    // Unsigned int -> unsigned int with other error checking
    template <typename ReturnType, typename Arg0Type>
    struct assignment_kernel<
        ReturnType, Arg0Type, assign_error_fractional,
        std::enable_if_t<is_unsigned_integral<ReturnType>::value && is_unsigned_integral<Arg0Type>::value>>
        : assignment_kernel<ReturnType, Arg0Type, assign_error_overflow> {};

    template <typename ReturnType, typename Arg0Type>
    struct assignment_kernel<
        ReturnType, Arg0Type, assign_error_inexact,
        std::enable_if_t<is_unsigned_integral<ReturnType>::value && is_unsigned_integral<Arg0Type>::value>>
        : assignment_kernel<ReturnType, Arg0Type, assign_error_overflow> {};

    // Signed int -> floating point with inexact checking
    template <typename ReturnType, typename Arg0Type>
    struct assignment_kernel<
        ReturnType, Arg0Type, assign_error_inexact,
        std::enable_if_t<is_floating_point<ReturnType>::value && is_signed_integral<Arg0Type>::value>>
        : base_strided_kernel<assignment_kernel<ReturnType, Arg0Type, assign_error_inexact>, 1> {
      void single(char *dst, char *const *src) {
        *reinterpret_cast<ReturnType *>(dst) =
            check_cast<ReturnType>(*reinterpret_cast<Arg0Type *>(src[0]), inexact_check);
      }
    };

    // Signed int -> floating point with other checking
    template <typename ReturnType, typename Arg0Type>
    struct assignment_kernel<
        ReturnType, Arg0Type, assign_error_overflow,
        std::enable_if_t<is_floating_point<ReturnType>::value && is_signed_integral<Arg0Type>::value>>
        : assignment_kernel<ReturnType, Arg0Type, assign_error_nocheck> {};

    template <typename ReturnType, typename Arg0Type>
    struct assignment_kernel<
        ReturnType, Arg0Type, assign_error_fractional,
        std::enable_if_t<is_floating_point<ReturnType>::value && is_signed_integral<Arg0Type>::value>>
        : assignment_kernel<ReturnType, Arg0Type, assign_error_nocheck> {};

    template <typename ReturnType, typename Arg0Type, assign_error_mode ErrorMode>
    struct assignment_kernel<ReturnType, Arg0Type, ErrorMode,
                             std::enable_if_t<is_complex<ReturnType>::value && is_signed_integral<Arg0Type>::value>>
        : base_strided_kernel<assignment_kernel<ReturnType, Arg0Type, ErrorMode>, 1> {
      void single(char *dst, char *const *src) {
        Arg0Type s = *reinterpret_cast<Arg0Type *>(src[0]);

        DYND_TRACE_ASSIGNMENT(static_cast<ReturnType>(s), ReturnType, s, Arg0Type);

        *reinterpret_cast<ReturnType *>(dst) = static_cast<typename ReturnType::value_type>(s);
      }
    };

    // complex -> real with overflow checking
    template <typename ReturnType, typename Arg0Type>
    struct assignment_kernel<ReturnType, Arg0Type, assign_error_overflow,
                             std::enable_if_t<is_floating_point<ReturnType>::value && is_complex<Arg0Type>::value>>
        : base_strided_kernel<assignment_kernel<ReturnType, Arg0Type, assign_error_overflow>, 1> {
      void single(char *dst, char *const *src) {
        *reinterpret_cast<ReturnType *>(dst) = overflow_cast<ReturnType>(*reinterpret_cast<Arg0Type *>(src[0]));
      }
    };

    // complex -> real with inexact checking
    template <typename ReturnType, typename Arg0Type>
    struct assignment_kernel<ReturnType, Arg0Type, assign_error_inexact,
                             std::enable_if_t<is_floating_point<ReturnType>::value && is_complex<Arg0Type>::value>>
        : assignment_kernel<ReturnType, Arg0Type, assign_error_overflow> {};

    // complex -> real with fractional checking
    template <typename ReturnType, typename Arg0Type>
    struct assignment_kernel<ReturnType, Arg0Type, assign_error_fractional,
                             std::enable_if_t<is_floating_point<ReturnType>::value && is_complex<Arg0Type>::value>>
        : assignment_kernel<ReturnType, Arg0Type, assign_error_overflow> {};

    // complex<double> -> float with inexact checking
    template <>
    struct assignment_kernel<float, complex<double>, assign_error_inexact>
        : base_strided_kernel<assignment_kernel<float, complex<double>, assign_error_inexact>, 1> {
      void single(char *dst, char *const *src) {
        *reinterpret_cast<float *>(dst) =
            check_cast<float>(*reinterpret_cast<complex<double> *>(src[0]), inexact_check);
      }
    };

    // real -> complex with overflow checking
    template <typename ReturnType, typename Arg0Type>
    struct assignment_kernel<ReturnType, Arg0Type, assign_error_overflow,
                             std::enable_if_t<is_complex<ReturnType>::value && is_floating_point<Arg0Type>::value>>
        : base_strided_kernel<assignment_kernel<ReturnType, Arg0Type, assign_error_overflow>, 1> {
      void single(char *dst, char *const *src) {
        *reinterpret_cast<ReturnType *>(dst) = overflow_cast<ReturnType>(*reinterpret_cast<Arg0Type *>(src[0]));
      }
    };

    // real -> complex with fractional checking
    template <typename ReturnType, typename Arg0Type>
    struct assignment_kernel<ReturnType, Arg0Type, assign_error_fractional,
                             std::enable_if_t<is_complex<ReturnType>::value && is_floating_point<Arg0Type>::value>>
        : assignment_kernel<ReturnType, Arg0Type, assign_error_overflow> {};

    // real -> complex with inexact checking
    template <typename ReturnType, typename Arg0Type>
    struct assignment_kernel<ReturnType, Arg0Type, assign_error_inexact,
                             std::enable_if_t<is_complex<ReturnType>::value && is_floating_point<Arg0Type>::value>>
        : base_strided_kernel<assignment_kernel<ReturnType, Arg0Type, assign_error_inexact>, 1> {
      void single(char *dst, char *const *src) {
        *reinterpret_cast<ReturnType *>(dst) =
            check_cast<ReturnType>(*reinterpret_cast<Arg0Type *>(src[0]), inexact_check);
      }
    };

    // complex<double> -> complex<float> with overflow checking
    template <>
    struct assignment_kernel<complex<float>, complex<double>, assign_error_overflow>
        : base_strided_kernel<assignment_kernel<complex<float>, complex<double>, assign_error_overflow>, 1> {
      typedef complex<float> dst_type;
      typedef complex<double> src0_type;

      void single(char *dst, char *const *src) {
        *reinterpret_cast<dst_type *>(dst) = overflow_cast<dst_type>(*reinterpret_cast<src0_type *>(src[0]));
      }
    };

    // complex<double> -> complex<float> with fractional checking
    template <>
    struct assignment_kernel<complex<float>, complex<double>, assign_error_fractional>
        : assignment_kernel<complex<float>, complex<double>, assign_error_overflow> {};

    // complex<double> -> complex<float> with inexact checking
    template <>
    struct assignment_kernel<complex<float>, complex<double>, assign_error_inexact>
        : base_strided_kernel<assignment_kernel<complex<float>, complex<double>, assign_error_inexact>, 1> {
      typedef complex<float> dst_type;
      typedef complex<double> src0_type;

      void single(char *dst, char *const *src) {
        *reinterpret_cast<dst_type *>(dst) =
            check_cast<dst_type>(*reinterpret_cast<src0_type *>(src[0]), inexact_check);
      }
    };

    /**
     * A ckernel which assigns option[S] to option[T].
     */
    template <assign_error_mode ErrorMode>
    struct assignment_kernel<ndt::option_type, ndt::option_type, ErrorMode>
        : base_strided_kernel<assignment_kernel<ndt::option_type, ndt::option_type, ErrorMode>, 1> {
      // The default child is the src is_avail ckernel
      // This child is the dst assign_na ckernel
      size_t m_dst_assign_na_offset;
      size_t m_value_assign_offset;

      ~assignment_kernel() {
        // src_is_avail
        this->get_child()->destroy();
        // dst_assign_na
        this->get_child(m_dst_assign_na_offset)->destroy();
        // value_assign
        this->get_child(m_value_assign_offset)->destroy();
      }

      void single(char *dst, char *const *src) {
        // Check whether the value is available
        // TODO: Would be nice to do this as a predicate
        //       instead of having to go through a dst pointer
        kernel_prefix *src_is_na = this->get_child();
        kernel_single_t src_is_na_fn = src_is_na->get_function<kernel_single_t>();
        bool1 missing = bool1(false);
        src_is_na_fn(src_is_na, reinterpret_cast<char *>(&missing), src);
        if (!missing) {
          // It's available, copy using value assignment
          kernel_prefix *value_assign = this->get_child(m_value_assign_offset);
          kernel_single_t value_assign_fn = value_assign->get_function<kernel_single_t>();
          value_assign_fn(value_assign, dst, src);
        } else {
          // It's not available, assign an NA
          kernel_prefix *dst_assign_na = this->get_child(m_dst_assign_na_offset);
          kernel_single_t dst_assign_na_fn = dst_assign_na->get_function<kernel_single_t>();
          dst_assign_na_fn(dst_assign_na, dst, NULL);
        }
      }

      void strided(char *dst, intptr_t dst_stride, char *const *src, const intptr_t *src_stride, size_t count) {
        // Three child ckernels
        kernel_prefix *src_is_na = this->get_child();
        kernel_strided_t src_is_na_fn = src_is_na->get_function<kernel_strided_t>();
        kernel_prefix *value_assign = this->get_child(m_value_assign_offset);
        kernel_strided_t value_assign_fn = value_assign->get_function<kernel_strided_t>();
        kernel_prefix *dst_assign_na = this->get_child(m_dst_assign_na_offset);
        kernel_strided_t dst_assign_na_fn = dst_assign_na->get_function<kernel_strided_t>();
        // Process in chunks using the dynd default buffer size
        bool1 missing[DYND_BUFFER_CHUNK_SIZE];
        while (count > 0) {
          size_t chunk_size = std::min(count, (size_t)DYND_BUFFER_CHUNK_SIZE);
          count -= chunk_size;
          src_is_na_fn(src_is_na, reinterpret_cast<char *>(missing), 1, src, src_stride, chunk_size);
          void *missing_ptr = missing;
          char *src_copy = src[0];
          do {
            // Process a run of available values
            void *next_missing_ptr = memchr(missing_ptr, 1, chunk_size);
            if (!next_missing_ptr) {
              value_assign_fn(value_assign, dst, dst_stride, &src_copy, src_stride, chunk_size);
              dst += chunk_size * dst_stride;
              src += chunk_size * src_stride[0];
              break;
            } else if (next_missing_ptr > missing_ptr) {
              size_t segment_size = (char *)next_missing_ptr - (char *)missing_ptr;
              value_assign_fn(value_assign, dst, dst_stride, &src_copy, src_stride, segment_size);
              dst += segment_size * dst_stride;
              src_copy += segment_size * src_stride[0];
              chunk_size -= segment_size;
              missing_ptr = next_missing_ptr;
            }
            // Process a run of not available values
            next_missing_ptr = memchr(missing_ptr, 0, chunk_size);
            if (!next_missing_ptr) {
              dst_assign_na_fn(dst_assign_na, dst, dst_stride, NULL, NULL, chunk_size);
              dst += chunk_size * dst_stride;
              src_copy += chunk_size * src_stride[0];
              break;
            } else if (next_missing_ptr > missing_ptr) {
              size_t segment_size = (char *)next_missing_ptr - (char *)missing_ptr;
              dst_assign_na_fn(dst_assign_na, dst, dst_stride, NULL, NULL, segment_size);
              dst += segment_size * dst_stride;
              src_copy += segment_size * src_stride[0];
              chunk_size -= segment_size;
              missing_ptr = next_missing_ptr;
            }
          } while (chunk_size > 0);
        }
      }
    };

    struct DYND_API string_to_option_bool_ck : nd::base_strided_kernel<string_to_option_bool_ck, 1> {
      void single(char *dst, char *const *src) {
        const string *std = reinterpret_cast<string *>(src[0]);
        *reinterpret_cast<bool1 *>(dst) = parse<bool>(std->begin(), std->end());
      }
    };

    struct DYND_API string_to_option_number_ck : nd::base_strided_kernel<string_to_option_number_ck, 1> {
      type_id_t m_tid;
      assign_error_mode m_errmode;

      string_to_option_number_ck() {}

      string_to_option_number_ck(type_id_t tid, assign_error_mode errmode) : m_tid(tid), m_errmode(errmode) {}

      void single(char *dst, char *const *src) {
        const string *std = reinterpret_cast<string *>(src[0]);
        string_to_number(dst, m_tid, std->begin(), std->end(), m_errmode);
      }
    };

    struct DYND_API string_to_option_tp_ck : nd::base_strided_kernel<string_to_option_tp_ck, 1> {
      intptr_t m_dst_assign_na_offset;

      ~string_to_option_tp_ck() {
        // value_assign
        get_child()->destroy();
        // dst_assign_na
        get_child(m_dst_assign_na_offset)->destroy();
      }

      void single(char *dst, char *const *src) {
        const string *std = reinterpret_cast<string *>(src[0]);
        if (parse_na(std->begin(), std->end())) {
          // It's not available, assign an NA
          kernel_prefix *dst_assign_na = get_child(m_dst_assign_na_offset);
          kernel_single_t dst_assign_na_fn = dst_assign_na->get_function<kernel_single_t>();
          dst_assign_na_fn(dst_assign_na, dst, NULL);
        } else {
          // It's available, copy using value assignment
          kernel_prefix *value_assign = get_child();
          kernel_single_t value_assign_fn = value_assign->get_function<kernel_single_t>();
          value_assign_fn(value_assign, dst, src);
        }
      }
    };

  } // namespace dynd::nd::detail

  /**
   * A ckernel which assigns option[S] to T.
   */
  struct DYND_API option_to_value_ck : nd::base_strided_kernel<option_to_value_ck, 1> {
    // The default child is the src_is_na ckernel
    size_t m_value_assign_offset;

    ~option_to_value_ck() {
      // src_is_na
      get_child()->destroy();
      // value_assign
      get_child(m_value_assign_offset)->destroy();
    }

    void single(char *dst, char *const *src) {
      kernel_prefix *src_is_na = get_child();
      kernel_single_t src_is_na_fn = src_is_na->get_function<kernel_single_t>();
      kernel_prefix *value_assign = get_child(m_value_assign_offset);
      kernel_single_t value_assign_fn = value_assign->get_function<kernel_single_t>();
      // Make sure it's not an NA
      bool1 missing = bool1(false);
      src_is_na_fn(src_is_na, reinterpret_cast<char *>(&missing), src);
      if (missing) {
        throw std::overflow_error("cannot assign an NA value to a non-option type");
      }
      // Copy using value assignment
      value_assign_fn(value_assign, dst, src);
    }

    void strided(char *dst, intptr_t dst_stride, char *const *src, const intptr_t *src_stride, size_t count) {
      // Two child ckernels
      kernel_prefix *src_is_na = get_child();
      kernel_strided_t src_is_na_fn = src_is_na->get_function<kernel_strided_t>();
      kernel_prefix *value_assign = get_child(m_value_assign_offset);
      kernel_strided_t value_assign_fn = value_assign->get_function<kernel_strided_t>();
      // Process in chunks using the dynd default buffer size
      bool1 missing[DYND_BUFFER_CHUNK_SIZE];
      char *src_copy = src[0];
      while (count > 0) {
        size_t chunk_size = std::min(count, (size_t)DYND_BUFFER_CHUNK_SIZE);
        src_is_na_fn(src_is_na, reinterpret_cast<char *>(missing), 1, &src_copy, src_stride, chunk_size);
        for (size_t i = 0; i < chunk_size; ++i) {
          if (missing[i]) {
            throw std::overflow_error("cannot assign an NA value to a non-option type");
          }
        }
        value_assign_fn(value_assign, dst, dst_stride, &src_copy, src_stride, chunk_size);
        dst += chunk_size * dst_stride;
        src_copy += chunk_size * src_stride[0];
        count -= chunk_size;
      }
    }
  };

  template <int N>
  struct trivial_copy_kernel;

  template <>
  struct trivial_copy_kernel<1> : base_strided_kernel<trivial_copy_kernel<1>, 1> {
    void single(char *dst, char *const *src) { *dst = *src[0]; }
  };

  template <>
  struct trivial_copy_kernel<2> : base_strided_kernel<trivial_copy_kernel<2>, 1> {
    void single(char *dst, char *const *src) {
      *reinterpret_cast<int16_t *>(dst) = *reinterpret_cast<int16_t *>(src[0]);
    }
  };

  template <>
  struct trivial_copy_kernel<4> : base_strided_kernel<trivial_copy_kernel<4>, 1> {
    void single(char *dst, char *const *src) {
      *reinterpret_cast<int32_t *>(dst) = *reinterpret_cast<int32_t *>(src[0]);
    }
  };

  template <>
  struct trivial_copy_kernel<8> : base_strided_kernel<trivial_copy_kernel<8>, 1> {
    void single(char *dst, char *const *src) {
      *reinterpret_cast<int64_t *>(dst) = *reinterpret_cast<int64_t *>(src[0]);
    }
  };

  struct unaligned_copy_ck : base_strided_kernel<unaligned_copy_ck, 1> {
    size_t data_size;

    unaligned_copy_ck(size_t data_size) : data_size(data_size) {}

    void single(char *dst, char *const *src) { memcpy(dst, *src, data_size); }
  };

  namespace detail {

    template <typename Arg0Type, assign_error_mode ErrorMode>
    struct assignment_kernel<string, Arg0Type, ErrorMode, std::enable_if_t<is_signed_integral<Arg0Type>::value>>
        : base_strided_kernel<assignment_kernel<string, Arg0Type, ErrorMode>, 1> {
      ndt::type dst_string_tp;
      type_id_t src_id;
      const char *dst_arrmeta;

      assignment_kernel(const ndt::type &dst_string_tp, type_id_t src_id, const char *dst_arrmeta)
          : dst_string_tp(dst_string_tp), src_id(src_id), dst_arrmeta(dst_arrmeta) {}

      void single(char *dst, char *const *src) {
        // TODO: There are much faster ways to do this, but it's very generic!
        //       Also, for floating point values, a printing scheme like
        //       Python's, where it prints the shortest string that's
        //       guaranteed to parse to the same float number, would be
        //       better.
        std::stringstream ss;
        ndt::type(reinterpret_cast<ndt::base_type *>(src_id), false).print_data(ss, NULL, src[0]);
        dst_string_tp->set_from_utf8_string(dst_arrmeta, dst, ss.str(), &eval::default_eval_context);
      }
    };

    template <typename Arg0Type, assign_error_mode ErrorMode>
    struct assignment_kernel<ndt::fixed_string_type, Arg0Type, ErrorMode>
        : assignment_kernel<string, int32_t, ErrorMode> {};

    template <assign_error_mode ErrorMode>
    struct assignment_kernel<string, ndt::fixed_string_type, ErrorMode>
        : base_strided_kernel<assignment_kernel<string, ndt::fixed_string_type, ErrorMode>, 1> {
      string_encoding_t m_dst_encoding, m_src_encoding;
      intptr_t m_src_element_size;
      next_unicode_codepoint_t m_next_fn;
      append_unicode_codepoint_t m_append_fn;

      assignment_kernel(string_encoding_t dst_encoding, string_encoding_t src_encoding, intptr_t src_element_size,
                        next_unicode_codepoint_t next_fn, append_unicode_codepoint_t append_fn)
          : m_dst_encoding(dst_encoding), m_src_encoding(src_encoding), m_src_element_size(src_element_size),
            m_next_fn(next_fn), m_append_fn(append_fn) {}

      void single(char *dst, char *const *src) {
        dynd::string *dst_d = reinterpret_cast<dynd::string *>(dst);
        intptr_t src_charsize = string_encoding_char_size_table[m_src_encoding];

        char *dst_current;
        const char *src_begin = src[0];
        const char *src_end = src[0] + m_src_element_size;
        next_unicode_codepoint_t next_fn = m_next_fn;
        append_unicode_codepoint_t append_fn = m_append_fn;
        uint32_t cp;

        // Allocate the initial output as the src number of characters + some padding
        dst_d->resize(((src_end - src_begin) / src_charsize + 16) * 1124 / 1024);
        char *dst_begin = dst_d->begin();
        char *dst_end = dst_d->end();

        dst_current = dst_begin;
        while (src_begin < src_end) {
          cp = next_fn(src_begin, src_end);
          // Append the codepoint, or increase the allocated memory as necessary
          if (cp != 0) {
            if (dst_end - dst_current >= 8) {
              append_fn(cp, dst_current, dst_end);
            } else {
              char *dst_begin_saved = dst_begin;
              dst_d->resize(2 * (dst_end - dst_begin));
              dst_begin = dst_d->begin();
              dst_end = dst_d->end();
              dst_current = dst_begin + (dst_current - dst_begin_saved);

              append_fn(cp, dst_current, dst_end);
            }
          } else {
            break;
          }
        }

        // Shrink-wrap the memory to just fit the string
        dst_d->resize(dst_current - dst_begin);
      }
    };

    template <>
    struct assignment_kernel<bool1, string, assign_error_nocheck>
        : base_strided_kernel<assignment_kernel<bool1, string, assign_error_nocheck>, 1> {
      ndt::type src_string_tp;
      const char *src_arrmeta;

      assignment_kernel(const ndt::type &src_string_tp, const char *src_arrmeta)
          : src_string_tp(src_string_tp), src_arrmeta(src_arrmeta) {}

      void single(char *dst, char *const *src) {
        // Get the string from the source
        std::string s = reinterpret_cast<const ndt::base_string_type *>(src_string_tp.extended())
                            ->get_utf8_string(src_arrmeta, src[0], assign_error_nocheck);
        trim(s);
        *reinterpret_cast<bool1 *>(dst) = parse<bool>(s.data(), s.data() + s.size(), nocheck);
      }
    };

    template <>
    struct assignment_kernel<bool1, string, assign_error_inexact>
        : base_strided_kernel<assignment_kernel<bool1, string, assign_error_inexact>, 1> {
      ndt::type src_string_tp;
      const char *src_arrmeta;

      assignment_kernel(const ndt::type &src_string_tp, const char *src_arrmeta)
          : src_string_tp(src_string_tp), src_arrmeta(src_arrmeta) {}

      void single(char *dst, char *const *src) {
        // Get the string from the source
        std::string s = reinterpret_cast<const ndt::base_string_type *>(src_string_tp.extended())
                            ->get_utf8_string(src_arrmeta, src[0], assign_error_inexact);
        trim(s);
        *reinterpret_cast<bool1 *>(dst) = parse<bool>(s.data(), s.data() + s.size());
      }
    };

    template <>
    struct assignment_kernel<bool1, string, assign_error_default>
        : base_strided_kernel<assignment_kernel<bool1, string, assign_error_default>, 1> {
      ndt::type src_string_tp;
      const char *src_arrmeta;

      assignment_kernel(const ndt::type &src_string_tp, const char *src_arrmeta)
          : src_string_tp(src_string_tp), src_arrmeta(src_arrmeta) {}

      void single(char *dst, char *const *src) {
        // Get the string from the source
        std::string s = reinterpret_cast<const ndt::base_string_type *>(src_string_tp.extended())
                            ->get_utf8_string(src_arrmeta, src[0], assign_error_default);
        trim(s);
        *reinterpret_cast<bool1 *>(dst) = parse<bool>(s.data(), s.data() + s.size());
      }
    };

    template <>
    struct assignment_kernel<bool1, string, assign_error_overflow>
        : base_strided_kernel<assignment_kernel<bool1, string, assign_error_overflow>, 1> {
      ndt::type src_string_tp;
      const char *src_arrmeta;

      assignment_kernel(const ndt::type &src_string_tp, const char *src_arrmeta)
          : src_string_tp(src_string_tp), src_arrmeta(src_arrmeta) {}

      void single(char *dst, char *const *src) {
        // Get the string from the source
        std::string s = reinterpret_cast<const ndt::base_string_type *>(src_string_tp.extended())
                            ->get_utf8_string(src_arrmeta, src[0], assign_error_overflow);
        trim(s);
        *reinterpret_cast<bool1 *>(dst) = parse<bool>(s.data(), s.data() + s.size());
      }
    };

    template <>
    struct assignment_kernel<bool1, string, assign_error_fractional>
        : base_strided_kernel<assignment_kernel<bool1, string, assign_error_fractional>, 1> {
      ndt::type src_string_tp;
      const char *src_arrmeta;

      assignment_kernel(const ndt::type &src_string_tp, const char *src_arrmeta)
          : src_string_tp(src_string_tp), src_arrmeta(src_arrmeta) {}

      void single(char *dst, char *const *src) {
        // Get the string from the source
        std::string s = reinterpret_cast<const ndt::base_string_type *>(src_string_tp.extended())
                            ->get_utf8_string(src_arrmeta, src[0], assign_error_fractional);
        trim(s);
        *reinterpret_cast<bool1 *>(dst) = parse<bool>(s.data(), s.data() + s.size());
      }
    };

    template <typename Arg0Type, assign_error_mode ErrorMode>
    struct assignment_kernel<Arg0Type, string, ErrorMode, std::enable_if_t<is_signed_integral<Arg0Type>::value>>
        : base_strided_kernel<assignment_kernel<Arg0Type, string, ErrorMode>, 1> {
      typedef Arg0Type T;

      ndt::type src_string_tp;
      const char *src_arrmeta;
      assign_error_mode error_mode;

      assignment_kernel(const ndt::type &src_string_tp, const char *src_arrmeta,
                        assign_error_mode error_mode = ErrorMode)
          : src_string_tp(src_string_tp), src_arrmeta(src_arrmeta), error_mode(error_mode) {}

      void single(char *dst, char *const *src) {
        std::string s = reinterpret_cast<const ndt::base_string_type *>(src_string_tp.extended())
                            ->get_utf8_string(src_arrmeta, src[0], error_mode);
        trim(s);
        bool negative = false;
        if (!s.empty() && s[0] == '-') {
          s.erase(0, 1);
          negative = true;
        }
        T result;
        if (error_mode == assign_error_nocheck) {
          uint64_t value = parse<uint64_t>(s.data(), s.data() + s.size(), nocheck);
          result = negative ? static_cast<T>(-static_cast<int64_t>(value)) : static_cast<T>(value);
        } else {
          bool overflow = false;
          uint64_t value = parse<uint64_t>(s.data(), s.data() + s.size());
          if (overflow || overflow_check<T>::is_overflow(value, negative)) {
            raise_string_cast_overflow_error(ndt::make_type<T>(), src_string_tp, src_arrmeta, src[0]);
          }
          result = negative ? static_cast<T>(-static_cast<int64_t>(value)) : static_cast<T>(value);
        }
        *reinterpret_cast<T *>(dst) = result;
      }
    };

    template <typename ReturnType>
    struct assignment_virtual_kernel<ReturnType, string, std::enable_if_t<is_unsigned_integral<ReturnType>::value>>
        : base_strided_kernel<assignment_virtual_kernel<ReturnType, string>, 1> {
      void single(char *dst, char *const *src) {
        *reinterpret_cast<ReturnType *>(dst) = parse<ReturnType>(*reinterpret_cast<string *>(src[0]));
      }
    };

    template <assign_error_mode ErrorMode>
    struct assignment_kernel<float16, string, ErrorMode>
        : base_strided_kernel<assignment_kernel<float16, string, ErrorMode>, 1> {
      void single(char *DYND_UNUSED(dst), char *const *DYND_UNUSED(src)) {
        throw std::runtime_error("TODO: implement string_to_float16_single");
      }
    };

    template <assign_error_mode ErrorMode>
    struct assignment_kernel<float, string, ErrorMode>
        : base_strided_kernel<assignment_kernel<float, string, ErrorMode>, 1> {
      ndt::type src_string_tp;
      const char *src_arrmeta;
      assign_error_mode error_mode;

      assignment_kernel(const ndt::type &src_string_tp, const char *src_arrmeta, assign_error_mode error_mode)
          : src_string_tp(src_string_tp), src_arrmeta(src_arrmeta), error_mode(error_mode) {}

      void single(char *dst, char *const *src) {
        // Get the string from the source
        std::string s = reinterpret_cast<const ndt::base_string_type *>(src_string_tp.extended())
                            ->get_utf8_string(src_arrmeta, src[0], error_mode);
        trim(s);
        double value = parse<double>(s.data(), s.data() + s.size());
        // Assign double -> float according to the error mode
        char *child_src[1] = {reinterpret_cast<char *>(&value)};
        switch (error_mode) {
        case assign_error_default:
        case assign_error_nocheck:
          dynd::nd::detail::assignment_kernel<float, double, assign_error_nocheck>::single_wrapper(NULL, dst,
                                                                                                   child_src);
          break;
        case assign_error_overflow:
          dynd::nd::detail::assignment_kernel<float, double, assign_error_overflow>::single_wrapper(NULL, dst,
                                                                                                    child_src);
          break;
        case assign_error_fractional:
          dynd::nd::detail::assignment_kernel<float, double, assign_error_fractional>::single_wrapper(NULL, dst,
                                                                                                      child_src);
          break;
        case assign_error_inexact:
          dynd::nd::detail::assignment_kernel<float, double, assign_error_inexact>::single_wrapper(NULL, dst,
                                                                                                   child_src);
          break;
        default:
          throw std::runtime_error("error");
        }
      }
    };

    template <assign_error_mode ErrorMode>
    struct assignment_kernel<float64, string, ErrorMode>
        : base_strided_kernel<assignment_kernel<float64, string, ErrorMode>, 1> {
      ndt::type src_string_tp;
      const char *src_arrmeta;
      assign_error_mode error_mode;

      assignment_kernel(const ndt::type &src_string_tp, const char *src_arrmeta, assign_error_mode error_mode)
          : src_string_tp(src_string_tp), src_arrmeta(src_arrmeta), error_mode(error_mode) {}

      void single(char *dst, char *const *src) {
        // Get the string from the source
        std::string s = reinterpret_cast<const ndt::base_string_type *>(src_string_tp.extended())
                            ->get_utf8_string(src_arrmeta, src[0], error_mode);
        trim(s);
        double value = parse<double>(s.data(), s.data() + s.size());
        *reinterpret_cast<double *>(dst) = value;
      }
    };

    template <assign_error_mode ErrorMode>
    struct assignment_kernel<float128, string, ErrorMode>
        : base_strided_kernel<assignment_kernel<float128, string, ErrorMode>, 1> {
      void single(char *DYND_UNUSED(dst), char *const *DYND_UNUSED(src)) {
        throw std::runtime_error("TODO: implement string_to_float128_single");
      }
    };

    template <assign_error_mode ErrorMode>
    struct assignment_kernel<complex<float>, string, ErrorMode>
        : base_strided_kernel<assignment_kernel<complex<float>, string, ErrorMode>, 1> {
      void single(char *DYND_UNUSED(dst), char *const *DYND_UNUSED(src)) {
        throw std::runtime_error("TODO: implement string_to_complex_float32_single");
      }
    };

    template <assign_error_mode ErrorMode>
    struct assignment_kernel<complex<double>, string, ErrorMode>
        : base_strided_kernel<assignment_kernel<complex<double>, string, ErrorMode>, 1> {
      void single(char *DYND_UNUSED(dst), char *const *DYND_UNUSED(src)) {
        throw std::runtime_error("TODO: implement string_to_complex_float64_single");
      }
    };

    template <typename ReturnType, assign_error_mode ErrorMode>
    struct assignment_kernel<ReturnType, ndt::fixed_string_type, ErrorMode,
                             std::enable_if_t<is_signed_integral<ReturnType>::value>>
        : assignment_kernel<ReturnType, string, ErrorMode> {};

    template <assign_error_mode ErrorMode>
    struct assignment_kernel<ndt::fixed_string_type, ndt::fixed_string_type, ErrorMode>
        : base_strided_kernel<assignment_kernel<ndt::fixed_string_type, ndt::fixed_string_type, ErrorMode>, 1> {
      next_unicode_codepoint_t m_next_fn;
      append_unicode_codepoint_t m_append_fn;
      intptr_t m_dst_data_size, m_src_data_size;
      bool m_overflow_check;

      assignment_kernel(next_unicode_codepoint_t next_fn, append_unicode_codepoint_t append_fn, intptr_t dst_data_size,
                        intptr_t src_data_size, bool overflow_check)
          : m_next_fn(next_fn), m_append_fn(append_fn), m_dst_data_size(dst_data_size), m_src_data_size(src_data_size),
            m_overflow_check(overflow_check) {}

      void single(char *dst, char *const *src) {
        char *dst_end = dst + m_dst_data_size;
        const char *src_end = src[0] + m_src_data_size;
        next_unicode_codepoint_t next_fn = m_next_fn;
        append_unicode_codepoint_t append_fn = m_append_fn;
        uint32_t cp = 0;

        char *src_copy = src[0];
        while (src_copy < src_end && dst < dst_end) {
          cp = next_fn(const_cast<const char *&>(src_copy), src_end);
          // The fixed_string type uses null-terminated strings
          if (cp == 0) {
            // Null-terminate the destination string, and we're done
            memset(dst, 0, dst_end - dst);
            return;
          } else {
            append_fn(cp, dst, dst_end);
          }
        }
        if (src_copy < src_end) {
          if (m_overflow_check) {
            throw std::runtime_error("Input string is too large to convert to "
                                     "destination fixed-size string");
          }
        } else if (dst < dst_end) {
          memset(dst, 0, dst_end - dst);
        }
      }
    };

    struct adapt_assign_from_kernel : base_strided_kernel<adapt_assign_from_kernel, 1> {
      intptr_t forward_offset;
      array buffer;

      adapt_assign_from_kernel(const ndt::type &buffer_tp) : buffer(empty(buffer_tp)) {}

      ~adapt_assign_from_kernel() {
        get_child()->destroy();
        get_child(forward_offset)->destroy();
      }

      void single(char *dst, char *const *src) {
        get_child()->single(buffer.data(), src);

        char *child_src[1] = {buffer.data()};
        get_child(forward_offset)->single(dst, child_src);
      }
    };

    template <assign_error_mode ErrorMode>
    struct assignment_kernel<ndt::fixed_string_type, string, ErrorMode>
        : base_strided_kernel<assignment_kernel<ndt::fixed_string_type, string, ErrorMode>, 1> {
      next_unicode_codepoint_t m_next_fn;
      append_unicode_codepoint_t m_append_fn;
      intptr_t m_dst_data_size;
      bool m_overflow_check;

      assignment_kernel(next_unicode_codepoint_t next_fn, append_unicode_codepoint_t append_fn, intptr_t dst_data_size,
                        bool overflow_check)
          : m_next_fn(next_fn), m_append_fn(append_fn), m_dst_data_size(dst_data_size),
            m_overflow_check(overflow_check) {}

      void single(char *dst, char *const *src) {
        char *dst_end = dst + m_dst_data_size;
        const dynd::string *src_d = reinterpret_cast<const dynd::string *>(src[0]);
        const char *src_begin = src_d->begin();
        const char *src_end = src_d->end();
        next_unicode_codepoint_t next_fn = m_next_fn;
        append_unicode_codepoint_t append_fn = m_append_fn;
        uint32_t cp;

        while (src_begin < src_end && dst < dst_end) {
          cp = next_fn(src_begin, src_end);
          append_fn(cp, dst, dst_end);
        }
        if (src_begin < src_end) {
          if (m_overflow_check) {
            throw std::runtime_error("Input string is too large to "
                                     "convert to destination "
                                     "fixed-size string");
          }
        } else if (dst < dst_end) {
          memset(dst, 0, dst_end - dst);
        }
      }
    };

    template <>
    struct assignment_virtual_kernel<ndt::pointer_type, ndt::pointer_type>
        : base_strided_kernel<assignment_virtual_kernel<ndt::pointer_type, ndt::pointer_type>, 1> {
      ~assignment_virtual_kernel() { get_child()->destroy(); }

      void single(char *dst, char *const *src) {
        kernel_prefix *copy_value = get_child();
        kernel_single_t copy_value_fn = copy_value->get_function<kernel_single_t>();
        // The src value is a pointer, and copy_value_fn expects a pointer
        // to that pointer
        char **src_ptr = reinterpret_cast<char **>(src[0]);
        copy_value_fn(copy_value, dst, src_ptr);
      }
    };

    template <>
    struct assignment_virtual_kernel<ndt::type, ndt::type>
        : base_strided_kernel<assignment_virtual_kernel<ndt::type, ndt::type>, 1> {
      void single(char *dst, char *const *src) {
        *reinterpret_cast<ndt::type *>(dst) = *reinterpret_cast<ndt::type *>(src[0]);
      }
    };

    template <assign_error_mode ErrorMode>
    struct assignment_kernel<ndt::type, string, ErrorMode>
        : base_strided_kernel<assignment_kernel<ndt::type, string, ErrorMode>, 1> {
      ndt::type src_string_dt;
      const char *src_arrmeta;

      assignment_kernel(const ndt::type &src0_tp, const char *src0_arrmeta)
          : src_string_dt(src0_tp), src_arrmeta(src0_arrmeta) {}

      void single(char *dst, char *const *src) {
        const std::string &s =
            src_string_dt.extended<ndt::base_string_type>()->get_utf8_string(src_arrmeta, src[0], ErrorMode);
        ndt::type(s).swap(*reinterpret_cast<ndt::type *>(dst));
      }
    };

    template <assign_error_mode ErrorMode>
    struct assignment_kernel<string, ndt::type, ErrorMode>
        : base_strided_kernel<assignment_kernel<string, ndt::type, ErrorMode>, 1> {
      ndt::type dst_string_dt;
      const char *dst_arrmeta;

      assignment_kernel(const ndt::type &dst_tp, const char *dst_arrmeta)
          : dst_string_dt(dst_tp), dst_arrmeta(dst_arrmeta) {}

      void single(char *dst, char *const *src) {
        std::stringstream ss;
        ss << *reinterpret_cast<ndt::type *>(src[0]);
        dst_string_dt->set_from_utf8_string(dst_arrmeta, dst, ss.str(), &eval::default_eval_context);
      }
    };

  } // namespace dynd::nd::detail

  template <typename ReturnType, typename Arg0Type>
  struct assignment_kernel : detail::assignment_virtual_kernel<ReturnType, Arg0Type> {};

  template <typename ReturnType, typename Arg0Type>
  struct assign_kernel;

} // namespace dynd::nd
} // namespace dynd
