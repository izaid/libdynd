//
// Copyright (C) 2011-14 Irwin Zaid, Mark Wiebe, DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#include <dynd/arrmeta_holder.hpp>
#include <dynd/func/call_callable.hpp>
#include <dynd/func/neighborhood_arrfunc.hpp>
#include <dynd/kernels/expr_kernels.hpp>
#include <dynd/types/substitute_shape.hpp>

using namespace std;
using namespace dynd;

template <int N>
struct neighborhood_ck : kernels::expr_ck<neighborhood_ck<N>, N> {
    typedef neighborhood_ck<N> self_type;

    intptr_t dst_stride;
    intptr_t src_offset[N];
    intptr_t src_stride[N];
    intptr_t count[3];
    intptr_t nh_size;
    intptr_t *start_index;
    intptr_t *stop_index;

    inline void single(char *dst, char **src) {
        ckernel_prefix *child = self_type::get_child_ckernel();
        expr_single_t child_fn = child->get_function<expr_single_t>();

        char *src_copy[N];
        memcpy(src_copy, src, sizeof(src_copy));
        for (intptr_t j = 0; j < N; ++j) {
            src_copy[j] += src_offset[j];
        }

        *start_index = count[0];
        *stop_index = nh_size; // min(nh_size, dst_size)
        for (intptr_t i = 0; i < count[0]; ++i) {
            child_fn(dst, src_copy, child);
            --(*start_index);
            dst += dst_stride;
            for (intptr_t j = 0; j < N; ++j) {
                src_copy[j] += src_stride[j];
            }
        }
      //  *nh_start = 0;
    //    *nh_stop = nh_size;
        for (intptr_t i = 0; i < count[1]; ++i) {
            child_fn(dst, src_copy, child);
            dst += dst_stride;
            for (intptr_t j = 0; j < N; ++j) {
                src_copy[j] += src_stride[j];
            }
        }
  //      *nh_start = 0;
//        *nh_stop = count[2]; // 0 if count[2] > 
        for (intptr_t i = 0; i < count[2]; ++i) {
            --(*stop_index);
            child_fn(dst, src_copy, child);
            dst += dst_stride;
            for (intptr_t j = 0; j < N; ++j) {
                src_copy[j] += src_stride[j];
            }
        }
    }
};

struct neighborhood {
    nd::arrfunc op;
    start_stop_t *start_stop;
};

template <int N>
static intptr_t instantiate_neighborhood(
    const arrfunc_type_data *af_self, dynd::ckernel_builder *ckb,
    intptr_t ckb_offset, const ndt::type &dst_tp, const char *dst_arrmeta,
    const ndt::type *src_tp, const char *const *src_arrmeta,
    kernel_request_t kernreq, const nd::array &kwds, const eval::eval_context *ectx)
{
    neighborhood *nh = *af_self->get_data_as<neighborhood *>();
    nd::arrfunc nh_op = nh->op;

    nd::array shape;
    try {
        shape = kwds.p("shape").f("dereference");
    } catch (...) {
        const nd::array &mask = kwds.p("mask").f("dereference");
        shape = nd::array(mask.get_shape());
    }
    intptr_t ndim = shape.get_dim_size();

    nd::array offset;
    try {
        offset = kwds.p("offset").f("dereference");
    } catch (...) {
    }

    // Process the dst array striding/types
    const size_stride_t *dst_shape;
    ndt::type nh_dst_tp;
    const char *nh_dst_arrmeta;
    if (!dst_tp.get_as_strided(dst_arrmeta, ndim, &dst_shape, &nh_dst_tp,
                               &nh_dst_arrmeta)) {
        stringstream ss;
        ss << "neighborhood arrfunc dst must be a strided array, not " << dst_tp;
        throw invalid_argument(ss.str());
    }

    // Process the src[0] array striding/type
    const size_stride_t *src0_shape;
    ndt::type src0_el_tp;
    const char *src0_el_arrmeta;
    if (!src_tp[0].get_as_strided(src_arrmeta[0], ndim, &src0_shape, &src0_el_tp,
                                  &src0_el_arrmeta)) {
        stringstream ss;
        ss << "neighborhood arrfunc argument 1 must be a 2D strided array, not "
           << src_tp[0];
        throw invalid_argument(ss.str());
    }

    // Synthesize the arrmeta for the src[0] passed to the neighborhood op
    ndt::type nh_src_tp[1];
    nh_src_tp[0] = ndt::make_fixed_dimsym(src0_el_tp, ndim);
    arrmeta_holder nh_arrmeta;
    arrmeta_holder(nh_src_tp[0]).swap(nh_arrmeta);
    size_stride_t *nh_src0_arrmeta = reinterpret_cast<size_stride_t *>(nh_arrmeta.get());
    for (intptr_t i = 0; i < ndim; ++i) {
        nh_src0_arrmeta[i].dim_size = shape(i).as<intptr_t>();
        nh_src0_arrmeta[i].stride = src0_shape[i].stride;
    }
    const char *nh_src_arrmeta[1] = {nh_arrmeta.get()};

    std::vector<intptr_t> ckb_offsets;
    for (intptr_t i = 0; i < ndim; ++i) {
        typedef neighborhood_ck<N> self_type;
        ckb_offsets.push_back(ckb_offset);
        self_type *self = self_type::create(ckb, kernreq, ckb_offset);

        self->dst_stride = dst_shape[i].stride;
        for (intptr_t j = 0; j < N; ++j) {
            self->src_offset[j] = offset.is_null() ? 0 : (offset(i).as<intptr_t>() * src0_shape[i].stride);
            self->src_stride[j] = src0_shape[i].stride;
        }

        self->count[0] = offset.is_null() ? 0 : -offset(i).as<intptr_t>();
        if (self->count[0] < 0) {
            self->count[0] = 0;
        } else if (self->count[0] > dst_shape[i].dim_size) {
            self->count[0] = dst_shape[i].dim_size;
        }
        self->count[2] = shape(i).as<intptr_t>() + (offset.is_null() ? 0 : offset(i).as<intptr_t>()) - 1;
        if (self->count[2] < 0) {
            self->count[2] = 0;
        } else if (self->count[2] > (dst_shape[i].dim_size - self->count[0])) {
            self->count[2] = dst_shape[i].dim_size - self->count[0];
        }
        self->count[1] = dst_shape[i].dim_size - self->count[0] - self->count[2];

        self->nh_size = shape(i).as<intptr_t>();
    }

    intptr_t *start_index = NULL;
    intptr_t *stop_index = NULL;
    ckb_offset = nh_op.get()->instantiate(nh_op.get(), ckb, ckb_offset,
        nh_dst_tp, nh_dst_arrmeta, nh_src_tp, nh_src_arrmeta,
        kernel_request_single, pack(kwds, "start_index", &start_index, "stop_index", &stop_index), ectx);

    for (intptr_t i = 0; i < ndim; ++i) {
        typedef neighborhood_ck<N> self_type;
        self_type *self = ckb->get_at<self_type>(ckb_offsets[i]);

        if (start_index == NULL) {
            self->start_index = NULL;
        } else {
            self->start_index = start_index + i;
        }

        if (stop_index == NULL) {
            self->stop_index = NULL;
        } else {
            self->stop_index = stop_index + i;
        }
    }

    return ckb_offset;
}

static int resolve_neighborhood_dst_type(const arrfunc_type_data *self,
                                         intptr_t nsrc, const ndt::type *src_tp,
                                         const nd::array &DYND_UNUSED(dyn_params),
                                         int DYND_UNUSED(throw_on_error),
                                         ndt::type &out_dst_tp)
{
  // TODO: Should be able to express the match/subsitution without special code

  // This is basically resolve() from arrfunc.hpp
  if (nsrc != self->get_param_count()) {
    std::stringstream ss;
    ss << "arrfunc expected " << self->get_param_count()
       << " parameters, but received " << nsrc;
    throw std::invalid_argument(ss.str());
  }
  const ndt::type *param_types = self->get_param_types();
  std::map<nd::string, ndt::type> typevars;
  for (intptr_t i = 0; i != nsrc; ++i) {
    if (!ndt::pattern_match(src_tp[i].value_type(), param_types[i], typevars)) {
      std::stringstream ss;
      ss << "parameter " << (i + 1) << " to arrfunc does not match, ";
      ss << "expected " << param_types[i] << ", received " << src_tp[i];
      throw std::invalid_argument(ss.str());
    }
  }
  out_dst_tp = ndt::substitute(self->get_return_type(), typevars, false);

  // swap in the input dimension values for the fixed**N
  intptr_t ndim = src_tp[0].get_ndim();
  dimvector shape(ndim);
  src_tp[0].extended()->get_shape(ndim, 0, shape.get(), NULL, NULL);
  out_dst_tp = ndt::substitute_shape(out_dst_tp, ndim, shape.get());

  return 1;
}

static void free_neighborhood(arrfunc_type_data *self_af) {
    neighborhood *nh = *self_af->get_data_as<neighborhood *>();
    free(nh->start_stop);
    delete nh;
}

void dynd::make_neighborhood_arrfunc(arrfunc_type_data *out_af,
                                     const nd::arrfunc &neighborhood_op,
                                     intptr_t nh_ndim)
{
    std::ostringstream oss;
    oss << "fixed**" << nh_ndim;
    ndt::type nhop_pattern("(" + oss.str() + " * NH) -> OUT");
    ndt::type result_pattern("(" + oss.str() + " * NH) -> " + oss.str() + " * OUT");

    map<nd::string, ndt::type> typevars;
    if (!ndt::pattern_match(neighborhood_op.get()->func_proto, nhop_pattern, typevars)) {
        stringstream ss;
        ss << "provided neighborhood op proto " << neighborhood_op.get()->func_proto
           << " does not match pattern " << nhop_pattern;
        throw invalid_argument(ss.str());
    }

    neighborhood **nh = out_af->get_data_as<neighborhood *>();
    *nh = new neighborhood;
    (*nh)->op = neighborhood_op;
    (*nh)->start_stop = (start_stop_t *) malloc(nh_ndim * sizeof(start_stop_t)); 
    out_af->func_proto = ndt::substitute(result_pattern, typevars, false);
    out_af->instantiate = &instantiate_neighborhood<1>;
    out_af->resolve_dst_type = &resolve_neighborhood_dst_type;
    out_af->free_func = &free_neighborhood;
}
