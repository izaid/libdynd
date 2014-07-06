//
// Copyright (C) 2011-14 Irwin Zaid, DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#include <map>
#include <tr1/tuple>

#include <dynd/fft.hpp>
#include <dynd/types/strided_dim_type.hpp>
#include <dynd/types/arrfunc_type.hpp>
#include <dynd/func/take_arrfunc.hpp>

using namespace std;
using namespace dynd;

#ifdef DYND_FFTW

namespace dynd { namespace fftw {

typedef tr1::tuple<vector<intptr_t>, type_id_t, vector<intptr_t>, int,
    type_id_t, vector<intptr_t>, int, int, unsigned int> key_type;
static map<key_type, void *> plans;

static int cleanup_atexit = atexit(fftcleanup);

}} // namespace dynd::fftw

#define FFTW_FFTPLAN_C2C(LIB, REAL_TYPE, SRC_ID, DST_ID) \
    LIB##_plan dynd::fftw::fftplan(size_t ndim, vector<intptr_t> shape, LIB##_complex *src, vector<intptr_t> src_strides, \
                                   LIB##_complex *dst, vector<intptr_t> dst_strides, int sign, unsigned int flags) { \
        key_type key(shape, SRC_ID, src_strides, LIB##_alignment_of(reinterpret_cast<REAL_TYPE *>(src)), \
            DST_ID, dst_strides, LIB##_alignment_of(reinterpret_cast<REAL_TYPE *>(dst)), sign, flags); \
        if (plans.find(key) != plans.end()) { \
            return reinterpret_cast<LIB##_plan>(plans[key]); \
        } \
\
        shortvector<LIB##_iodim> iodims(ndim); \
        for (size_t i = 0; i < ndim; ++i) { \
            iodims[i].n = shape[i]; \
            iodims[i].is = src_strides[i] / sizeof(LIB##_complex); \
            iodims[i].os = dst_strides[i] / sizeof(LIB##_complex); \
        } \
\
        LIB##_plan plan = LIB##_plan_guru_dft(ndim, iodims.get(), 0, NULL, src, dst, sign, flags); \
        plans[key] = plan; \
\
        return plan; \
    }

FFTW_FFTPLAN_C2C(fftwf, float, complex_float32_type_id, complex_float32_type_id)
FFTW_FFTPLAN_C2C(fftw, double, complex_float64_type_id, complex_float64_type_id)

#undef FFTW_FFTPLAN_C2C

#define FFTW_FFTPLAN_R2C(LIB, REAL_TYPE, SRC_ID, DST_ID) \
    LIB##_plan dynd::fftw::fftplan(size_t ndim, vector<intptr_t> shape, REAL_TYPE *src, vector<intptr_t> src_strides, \
                                  LIB##_complex *dst, vector<intptr_t> dst_strides, unsigned int flags) { \
        key_type key(shape, SRC_ID, src_strides, LIB##_alignment_of(reinterpret_cast<REAL_TYPE *>(src)), \
            DST_ID, dst_strides, LIB##_alignment_of(reinterpret_cast<REAL_TYPE *>(dst)), FFTW_FORWARD, flags); \
        if (plans.find(key) != plans.end()) { \
            return reinterpret_cast<LIB##_plan>(plans[key]); \
        } \
\
        shortvector<LIB##_iodim> iodims(ndim); \
        for (size_t i = 0; i < ndim; ++i) { \
            iodims[i].n = shape[i]; \
            iodims[i].is = src_strides[i] / sizeof(REAL_TYPE); \
            iodims[i].os = dst_strides[i] / sizeof(LIB##_complex); \
        } \
\
        LIB##_plan plan = LIB##_plan_guru_dft_r2c(ndim, iodims.get(), 0, NULL, src, dst, flags); \
        plans[key] = plan; \
\
        return plan;\
    }

FFTW_FFTPLAN_R2C(fftwf, float, float32_type_id, complex_float32_type_id)
FFTW_FFTPLAN_R2C(fftw, double, float64_type_id, complex_float64_type_id)

#undef FFTW_FFTPLAN_R2C

#define FFTW_FFTPLAN_C2R(LIB, REAL_TYPE, SRC_ID, DST_ID) \
    LIB##_plan dynd::fftw::fftplan(size_t ndim, vector<intptr_t> shape, LIB##_complex *src, vector<intptr_t> src_strides, \
                                  REAL_TYPE *dst, vector<intptr_t> dst_strides, unsigned int flags) { \
        key_type key(shape, SRC_ID, src_strides, LIB##_alignment_of(reinterpret_cast<REAL_TYPE *>(src)), \
            DST_ID, dst_strides, LIB##_alignment_of(reinterpret_cast<REAL_TYPE *>(dst)), FFTW_BACKWARD, flags); \
        if (plans.find(key) != plans.end()) { \
            return reinterpret_cast<LIB##_plan>(plans[key]); \
        } \
\
        shortvector<LIB##_iodim> iodims(ndim); \
        for (size_t i = 0; i < ndim; ++i) { \
            iodims[i].n = shape[i]; \
            iodims[i].is = src_strides[i] / sizeof(LIB##_complex); \
            iodims[i].os = dst_strides[i] / sizeof(REAL_TYPE); \
        } \
\
        LIB##_plan plan = LIB##_plan_guru_dft_c2r(ndim, iodims.get(), 0, NULL, src, dst, flags); \
        plans[key] = plan; \
\
        return plan; \
    }

FFTW_FFTPLAN_C2R(fftwf, float, float32_type_id, complex_float32_type_id)
FFTW_FFTPLAN_C2R(fftw, double, float64_type_id, complex_float64_type_id)

#undef FFTW_FFTPLAN_C2R

void dynd::fftw::fftcleanup() {
    for (map<key_type, void *>::iterator iter = plans.begin(); iter != plans.end(); ++iter) {
        switch (tr1::get<1>(iter->first)) {
            case float32_type_id:
            case complex_float32_type_id:
                fftwf_destroy_plan(reinterpret_cast<fftwf_plan>(iter->second));
                break;
            case float64_type_id:
            case complex_float64_type_id:
                fftw_destroy_plan(reinterpret_cast<fftw_plan>(iter->second));
                break;
            default:
                throw std::runtime_error("");
        }
    }

    fftwf_cleanup();
    fftw_cleanup();
}

nd::array dynd::fftw::fft(const nd::array &x, const std::vector<intptr_t> &shape, unsigned int flags) {
    type_id_t dtp_id = x.get_dtype().get_type_id();

    if (dtp_id == complex_float32_type_id) {
        nd::array y = nd::empty_like(x);

        fftwf_complex *src = reinterpret_cast<fftwf_complex *>(x.get_readwrite_originptr());
        fftwf_complex *dst = reinterpret_cast<fftwf_complex *>(y.get_readwrite_originptr());
        fftwf_plan plan = fftw::fftplan(x.get_ndim(), shape, src, x.get_strides(),
            dst, y.get_strides(), FFTW_FORWARD, flags);
        fftwf_execute_dft(plan, src, dst);

        return y;
    }

    if (dtp_id == complex_float64_type_id) {
        nd::array y = nd::empty_like(x);

        fftw_complex *src = reinterpret_cast<fftw_complex *>(x.get_readwrite_originptr());
        fftw_complex *dst = reinterpret_cast<fftw_complex *>(y.get_readwrite_originptr());
        fftw_plan plan = fftw::fftplan(x.get_ndim(), shape, src, x.get_strides(),
            dst, y.get_strides(), FFTW_FORWARD, flags);
        fftw_execute_dft(plan, src, dst);

        return y;
    }

    throw std::runtime_error("");
}

nd::array dynd::fftw::ifft(const nd::array &x, const std::vector<intptr_t> &shape, unsigned int flags) {
    type_id_t dtp_id = x.get_dtype().get_type_id();

    if (dtp_id == complex_float32_type_id) {
        nd::array y = nd::empty_like(x);

        fftwf_complex *src = reinterpret_cast<fftwf_complex *>(x.get_readwrite_originptr());
        fftwf_complex *dst = reinterpret_cast<fftwf_complex *>(y.get_readwrite_originptr());
        fftwf_plan plan = fftw::fftplan(x.get_ndim(), shape, src, x.get_strides(),
            dst, y.get_strides(), FFTW_BACKWARD, flags);
        fftwf_execute_dft(plan, src, dst);

        return y;
    }

    if (dtp_id == complex_float64_type_id) {
        nd::array y = nd::empty_like(x);

        fftw_complex *src = reinterpret_cast<fftw_complex *>(x.get_readwrite_originptr());
        fftw_complex *dst = reinterpret_cast<fftw_complex *>(y.get_readwrite_originptr());
        fftw_plan plan = fftw::fftplan(x.get_ndim(), shape, src, x.get_strides(),
            dst, y.get_strides(), FFTW_BACKWARD, flags);
        fftw_execute_dft(plan, src, dst);

        return y;
    }

    throw std::runtime_error("");
}

nd::array dynd::fftw::rfft(const nd::array &x, const std::vector<intptr_t> &shape, unsigned int flags) {
    type_id_t dtp_id = x.get_dtype().get_type_id();

    if (dtp_id == float32_type_id) {
        std::vector<intptr_t> dst_shape = shape;
        dst_shape[x.get_ndim() - 1] = dst_shape[x.get_ndim() - 1] / 2 + 1;

        nd::array y = nd::dtyped_empty(shape, ndt::make_type<dynd_complex<float> >());

        float *src = reinterpret_cast<float *>(x.get_readwrite_originptr());
        fftwf_complex *dst = reinterpret_cast<fftwf_complex *>(y.get_readwrite_originptr());
        fftwf_plan plan = fftw::fftplan(x.get_ndim(), shape, src, x.get_strides(),
            dst, y.get_strides(), flags);
        fftwf_execute_dft_r2c(plan, src, dst);

        return y;
    }

    if (dtp_id == float64_type_id) {
        std::vector<intptr_t> dst_shape = shape;
        dst_shape[x.get_ndim() - 1] = dst_shape[x.get_ndim() - 1] / 2 + 1;

        nd::array y = nd::dtyped_empty(shape, ndt::make_type<dynd_complex<double> >());

        double *src = reinterpret_cast<double *>(x.get_readwrite_originptr());
        fftw_complex *dst = reinterpret_cast<fftw_complex *>(y.get_readwrite_originptr());
        fftw_plan plan = fftw::fftplan(x.get_ndim(), shape, src, x.get_strides(),
            dst, y.get_strides(), flags);
        fftw_execute_dft_r2c(plan, src, dst);

        return y;
    }

    throw std::runtime_error("");
}

nd::array dynd::fftw::irfft(const nd::array &x, const std::vector<intptr_t> &shape, unsigned int flags) {
    type_id_t dtp_id = x.get_dtype().get_type_id();

    if (dtp_id == complex_float32_type_id) {
        nd::array y = nd::dtyped_empty(shape, ndt::make_type<float>());

        fftwf_complex *src = reinterpret_cast<fftwf_complex *>(x.get_readwrite_originptr());
        float *dst = reinterpret_cast<float *>(y.get_readwrite_originptr());
        fftwf_plan plan = fftw::fftplan(x.get_ndim(), shape, src, x.get_strides(),
            dst, y.get_strides(), flags);
        fftwf_execute_dft_c2r(plan, src, dst);

        return y;
    }

    if (dtp_id == complex_float64_type_id) {
        nd::array y = nd::dtyped_empty(shape, ndt::make_type<double>());

        fftw_complex *src = reinterpret_cast<fftw_complex *>(x.get_readwrite_originptr());
        double *dst = reinterpret_cast<double *>(y.get_readwrite_originptr());
        fftw_plan plan = fftw::fftplan(x.get_ndim(), shape, src, x.get_strides(),
            dst, y.get_strides(), flags);
        fftw_execute_dft_c2r(plan, src, dst);

        return y;
    }

    throw std::runtime_error("");
}

#endif // DYND_FFTW

nd::array dynd::fftshift(const nd::array &x) {
    nd::arrfunc take = kernels::make_take_arrfunc();

    nd::array y = x;
    for (intptr_t i = 0; i < x.get_ndim(); ++i) {
        intptr_t p = y.get_dim_size();
        intptr_t q = (p + 1) / 2;

        y = take(y, nd::concatenate(nd::range(q, p), nd::range(q)));
        y = y.rotate();
    }

    return y;
}

nd::array dynd::ifftshift(const nd::array &x) {
    nd::arrfunc take = kernels::make_take_arrfunc();

    nd::array y = x;
    for (intptr_t i = 0; i < x.get_ndim(); ++i) {
        intptr_t p = y.get_dim_size();
        intptr_t q = p - (p + 1) / 2;

        y = take(y, nd::concatenate(nd::range(q, p), nd::range(q)));
        y = y.rotate();
    }

    return y;
}
