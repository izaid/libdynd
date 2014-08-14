//
// Copyright (C) 2011-14 Mark Wiebe, Irwin Zaid, DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#ifndef DYND__FUNC_FUNCTOR_ARRFUNC_HPP
#define DYND__FUNC_FUNCTOR_ARRFUNC_HPP

#include <dynd/types/funcproto_type.hpp>
#include <dynd/types/cfixed_dim_type.hpp>
#include <dynd/func/arrfunc.hpp>
#include <dynd/pp/list.hpp>
#include <dynd/pp/meta.hpp>

// TODO: Move these into the PP library

// Generates "NAME0, NAME1, ..., NAME`N-1`"
#define DYND_PP_ARG_RANGE_1(NAME, N)                                           \
  DYND_PP_JOIN_MAP_1(DYND_PP_ID, (, ), DYND_PP_META_NAME_RANGE(NAME, N))

// Generates "static_assert(TYPE_FUNC<NAME#>::value, MSG);" For # from 0 to N-1
#define DYND_PP_STATIC_ASSERT_RANGE_1(MSG, TYPE_FUNC, NAME, N)                 \
  DYND_PP_JOIN_OUTER_1(DYND_PP_META_STATIC_ASSERT, (;),                        \
                       \
DYND_PP_OUTER_1(DYND_PP_META_TEMPLATE_INSTANTIATION_SCOPE, (TYPE_FUNC),        \
                       DYND_PP_META_NAME_RANGE(NAME, N), (value)),             \
                       (MSG));

// Generates "*reinterpret_cast<const TYPE0 *>(NAME0), ..."
#define DYND_PP_DEREF_ARG_RANGE_1(TYPE, NAME, N)                               \
  DYND_PP_JOIN_MAP_1(                                                          \
      DYND_PP_META_DEREFERENCE, (, ),                                          \
      DYND_PP_ELWISE_1(DYND_PP_META_REINTERPRET_CAST,                          \
                       DYND_PP_MAP_1(DYND_PP_META_MAKE_CONST_PTR,              \
                                     DYND_PP_META_NAME_RANGE(TYPE, N)),        \
                       DYND_PP_META_AT_RANGE(NAME, N)))

namespace dynd { namespace nd {

namespace detail {

  template<typename T>
  struct functor_ckernel_instantiator;

#define DYND_CODE(NSRC)                                                        \
  template <typename R, DYND_PP_ARG_RANGE_1(typename A, NSRC)>                 \
  struct functor_ckernel_instantiator<R (*)(DYND_PP_ARG_RANGE_1(A, NSRC))> {   \
    typedef functor_ckernel_instantiator self_type;                            \
                                                                               \
    typedef R (*func_type)(DYND_PP_ARG_RANGE_1(A, NSRC));                      \
    DYND_PP_JOIN_ELWISE_1(                                                     \
        DYND_PP_META_TYPEDEF_TYPENAME, (;),                                    \
        DYND_PP_OUTER_1(                                                       \
            DYND_PP_META_TEMPLATE_INSTANTIATION_SCOPE, (remove_const),         \
            DYND_PP_OUTER_1(                                                   \
                DYND_PP_META_TYPENAME_TEMPLATE_INSTANTIATION_SCOPE,            \
                (remove_reference), DYND_PP_META_NAME_RANGE(A, NSRC), (type)), \
            (type)),                                                           \
        DYND_PP_META_NAME_RANGE(D, NSRC));                                     \
                                                                               \
    ckernel_prefix base;                                                       \
    func_type func;                                                            \
                                                                               \
    static void single(char *dst, const char *const *src, ckernel_prefix *ckp) \
    {                                                                          \
      self_type *e = reinterpret_cast<self_type *>(ckp);                       \
      *reinterpret_cast<R *>(dst) =                                            \
          e->func(DYND_PP_DEREF_ARG_RANGE_1(D, src, NSRC));                    \
    }                                                                          \
                                                                               \
    static void strided(char *dst, intptr_t dst_stride,                        \
                        const char *const *src, const intptr_t *src_stride,    \
                        size_t count, ckernel_prefix *ckp)                     \
    {                                                                          \
      self_type *e = reinterpret_cast<self_type *>(ckp);                       \
      func_type func = e->func;                                                \
      /* const char *src# = src[#]; */                                         \
      DYND_PP_JOIN_ELWISE_1(DYND_PP_META_DECL_ASGN, (;),                       \
                            DYND_PP_REPEAT_1(const char *, NSRC),              \
                            DYND_PP_META_NAME_RANGE(src, NSRC),                \
                            DYND_PP_META_AT_RANGE(src, NSRC));                 \
      /* intptr_t src_stride# = src_stride[#]; */                              \
      DYND_PP_JOIN_ELWISE_1(DYND_PP_META_DECL_ASGN, (;),                       \
                            DYND_PP_REPEAT_1(intptr_t, NSRC),                  \
                            DYND_PP_META_NAME_RANGE(src_stride, NSRC),         \
                            DYND_PP_META_AT_RANGE(src_stride, NSRC));          \
      for (size_t i = 0; i < count; ++i) {                                     \
        /* *(R *)dst = func(*(const D0 *)src0, ...); */                        \
        *reinterpret_cast<R *>(dst) =                                          \
            func(DYND_PP_DEREF_ARG_RANGE_1(D, src, NSRC));                     \
                                                                               \
        dst += dst_stride;                                                     \
        /* src# += src_stride# */                                              \
        DYND_PP_JOIN_ELWISE_1(DYND_PP_META_ADD_ASGN, (;),                      \
                              DYND_PP_META_NAME_RANGE(src, NSRC),              \
                              DYND_PP_META_NAME_RANGE(src_stride, NSRC));      \
      }                                                                        \
    }                                                                          \
                                                                               \
    static intptr_t instantiate(const arrfunc_type_data *af_self,              \
                                dynd::ckernel_builder *ckb,                    \
                                intptr_t ckb_offset, const ndt::type &dst_tp,  \
                                const char *DYND_UNUSED(dst_arrmeta),          \
                                const ndt::type *src_tp,                       \
                                const char *const *DYND_UNUSED(src_arrmeta),   \
                                kernel_request_t kernreq,                      \
                                const eval::eval_context *DYND_UNUSED(ectx))   \
    {                                                                          \
      for (intptr_t i = 0; i < NSRC; ++i) {                                    \
        if (src_tp[i] != af_self->get_param_type(i)) {                         \
          std::stringstream ss;                                                \
          ss << "Provided types " << ndt::make_funcproto(1, src_tp, dst_tp)    \
             << " do not match the arrfunc proto " << af_self->func_proto;     \
          throw type_error(ss.str());                                          \
        }                                                                      \
      }                                                                        \
      if (dst_tp != af_self->get_return_type()) {                              \
        std::stringstream ss;                                                  \
        ss << "Provided types " << ndt::make_funcproto(1, src_tp, dst_tp)      \
           << " do not match the arrfunc proto " << af_self->func_proto;       \
        throw type_error(ss.str());                                            \
      }                                                                        \
      self_type *e = ckb->alloc_ck_leaf<self_type>(ckb_offset);                \
      e->base.template set_expr_function<self_type>(kernreq);                  \
      e->func = *af_self->get_data_as<func_type>();                            \
                                                                               \
      return ckb_offset;                                                       \
    }                                                                          \
  };

DYND_PP_JOIN_MAP(DYND_CODE, (), DYND_PP_RANGE(1, DYND_PP_INC(DYND_ELWISE_MAX)))
#undef DYND_CODE


  template<typename T>
  struct is_suitable_input {
    enum {
      value =
          !is_reference<T>::value || is_const<remove_reference<T>::type>::value
    };
  };

} // namespace detail

#define DYND_CODE(NSRC)                                                        \
  template <typename R, DYND_PP_ARG_RANGE_1(typename A, NSRC)>                 \
  void make_functor_arrfunc(R (*func)(DYND_PP_ARG_RANGE_1(A, NSRC)),           \
                            arrfunc_type_data *out_af)                         \
  {                                                                            \
    DYND_PP_STATIC_ASSERT_RANGE_1("all reference arguments must be const",     \
                                  detail::is_suitable_input, A, NSRC)          \
    typedef R (*func_type)(DYND_PP_ARG_RANGE_1(A, NSRC));                      \
    DYND_PP_JOIN_ELWISE_1(                                                     \
        DYND_PP_META_TYPEDEF_TYPENAME, (;),                                    \
        DYND_PP_OUTER_1(                                                       \
            DYND_PP_META_TEMPLATE_INSTANTIATION_SCOPE, (remove_const),         \
            DYND_PP_OUTER_1(                                                   \
                DYND_PP_META_TYPENAME_TEMPLATE_INSTANTIATION_SCOPE,            \
                (remove_reference), DYND_PP_META_NAME_RANGE(A, NSRC), (type)), \
            (type)),                                                           \
        DYND_PP_META_NAME_RANGE(D, NSRC));                                     \
                                                                               \
    ndt::type dst_tp = ndt::cfixed_dim_from_array<R>::make();                  \
    ndt::type src_tp[NSRC] = {DYND_PP_JOIN_ELWISE_1(                           \
        DYND_PP_META_SCOPE_CALL, (, ),                                         \
        DYND_PP_OUTER(DYND_PP_META_TEMPLATE_INSTANTIATION,                     \
                      (ndt::cfixed_dim_from_array),                            \
                      DYND_PP_META_NAME_RANGE(D, NSRC)),                       \
        DYND_PP_REPEAT(make, NSRC))};                                          \
                                                                               \
    out_af->func_proto = ndt::make_funcproto(src_tp, dst_tp);                  \
    *out_af->get_data_as<func_type>() = func;                                  \
    out_af->instantiate =                                                      \
        &detail::functor_ckernel_instantiator<func_type>::instantiate;         \
    out_af->free_func = NULL;                                                  \
  }

DYND_PP_JOIN_MAP(DYND_CODE, (), DYND_PP_RANGE(1, DYND_PP_INC(DYND_ELWISE_MAX)))
#undef DYND_CODE

#define DYND_CODE(NSRC)                                                        \
  template <typename R, DYND_PP_ARG_RANGE_1(typename A, NSRC)>                 \
  nd::arrfunc make_functor_arrfunc(R (*func)(DYND_PP_ARG_RANGE_1(A, NSRC)))    \
  {                                                                            \
    nd::array af = nd::empty(ndt::make_arrfunc());                             \
    make_functor_arrfunc(func, reinterpret_cast<arrfunc_type_data *>(          \
                                   af.get_readwrite_originptr()));             \
    af.flag_as_immutable();                                                    \
    return af;                                                                 \
  }

DYND_PP_JOIN_MAP(DYND_CODE, (), DYND_PP_RANGE(1, DYND_PP_INC(DYND_ELWISE_MAX)))
#undef DYND_CODE

}} // namespace dynd::nd

#endif // DYND__FUNC_FUNCTOR_ARRFUNC_HPP