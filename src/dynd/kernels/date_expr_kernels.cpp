//
// Copyright (C) 2011-13, DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#include <time.h>
#include <cerrno>

#include <dynd/kernels/date_expr_kernels.hpp>
#include <dynd/kernels/elwise_expr_kernels.hpp>
#include <dynd/dtypes/string_dtype.hpp>
#include <datetime_strings.h>

using namespace std;
using namespace dynd;

namespace {
    struct date_strftime_kernel_extra {
        typedef date_strftime_kernel_extra extra_type;

        kernel_data_prefix base;
        size_t format_size;
        const char *format;
        const string_dtype_metadata *dst_metadata;

        static void single_unary(char *dst, const char *src,
                        kernel_data_prefix *extra)
        {
            extra_type *e = reinterpret_cast<extra_type *>(extra);
            const string_dtype_metadata *dst_md = e->dst_metadata;

            struct tm tm_val;
            int32_t date = *reinterpret_cast<const int32_t *>(src);
            // Convert the date to a 'struct tm'
            datetime::date_to_struct_tm(date, datetime::datetime_unit_day, tm_val);
#ifdef _MSC_VER
            // Given an invalid format string strftime will abort unless an invalid
            // parameter handler is installed.
            disable_invalid_parameter_handler raii;
#endif
            string_dtype_data *dst_d = reinterpret_cast<string_dtype_data *>(dst);
            memory_block_pod_allocator_api *allocator = get_memory_block_pod_allocator_api(dst_md->blockref);

            // Call strftime, growing the string buffer if needed so it fits
            size_t str_size = e->format_size + 16;
            allocator->allocate(dst_md->blockref, str_size,
                            1, &dst_d->begin, &dst_d->end);
            for(int attempt = 0; attempt < 3; ++attempt) {
                // Force errno to zero
                errno = 0;
                size_t len = strftime(dst_d->begin, str_size, e->format, &tm_val);
                if (len > 0) {
                    allocator->resize(dst_md->blockref, len, &dst_d->begin, &dst_d->end);
                    break;
                } else {
                    if (errno != 0) {
                        stringstream ss;
                        ss << "error in strftime with format string \"" << e->format << "\" to strftime";
                        throw runtime_error(ss.str());
                    }
                    str_size *= 2;
                    allocator->resize(dst_md->blockref, str_size, &dst_d->begin, &dst_d->end);
                }
            }
        }


        static void strided_unary(char *dst, intptr_t dst_stride,
                    const char *src, intptr_t src_stride,
                    size_t count, kernel_data_prefix *extra)
        {
            extra_type *e = reinterpret_cast<extra_type *>(extra);
            size_t format_size = e->format_size;
            const char *format = e->format;
            const string_dtype_metadata *dst_md = e->dst_metadata;

            struct tm tm_val;
#ifdef _MSC_VER
            // Given an invalid format string strftime will abort unless an invalid
            // parameter handler is installed.
            disable_invalid_parameter_handler raii;
#endif
            memory_block_pod_allocator_api *allocator = get_memory_block_pod_allocator_api(dst_md->blockref);

            for (size_t i = 0; i != count; ++i, dst += dst_stride, src += src_stride) {
                string_dtype_data *dst_d = reinterpret_cast<string_dtype_data *>(dst);
                int32_t date = *reinterpret_cast<const int32_t *>(src);
                // Convert the date to a 'struct tm'
                datetime::date_to_struct_tm(date, datetime::datetime_unit_day, tm_val);

                // Call strftime, growing the string buffer if needed so it fits
                size_t str_size = e->format_size + 16;
                allocator->allocate(dst_md->blockref, str_size,
                                1, &dst_d->begin, &dst_d->end);
                for(int attempt = 0; attempt < 3; ++attempt) {
                    // Force errno to zero
                    errno = 0;
                    size_t len = strftime(dst_d->begin, str_size, format, &tm_val);
                    if (len > 0) {
                        allocator->resize(dst_md->blockref, len, &dst_d->begin, &dst_d->end);
                        break;
                    } else {
                        if (errno != 0) {
                            stringstream ss;
                            ss << "error in strftime with format string \"" << e->format << "\" to strftime";
                            throw runtime_error(ss.str());
                        }
                        str_size *= 2;
                        allocator->resize(dst_md->blockref, str_size, &dst_d->begin, &dst_d->end);
                    }
                }
            }
        }
    };
} // anonymous namespace

class date_strftime_kernel_generator : public expr_kernel_generator {
    string m_format;
public:
    date_strftime_kernel_generator(const string& format)
        : expr_kernel_generator(true), m_format(format)
    {
    }

    virtual ~date_strftime_kernel_generator() {
    }

    size_t make_expr_kernel(
                hierarchical_kernel *out, size_t offset_out,
                const dtype& dst_dt, const char *dst_metadata,
                size_t src_count, const dtype *src_dt, const char **src_metadata,
                kernel_request_t kernreq, const eval::eval_context *ectx) const
    {
        if (src_count != 1) {
            stringstream ss;
            ss << "date_strftime_kernel_generator requires 1 src operand, ";
            ss << "received " << src_count;
            throw runtime_error(ss.str());
        }
        bool require_elwise = dst_dt.get_type_id() != string_type_id ||
                        src_dt[0].get_type_id() != date_type_id;
        // If the dtypes don't match the ones for this generator,
        // call the elementwise dimension handler to handle one dimension,
        // giving 'this' as the next kernel generator to call
        if (require_elwise) {
            return make_elwise_dimension_expr_kernel(out, offset_out,
                            dst_dt, dst_metadata,
                            src_count, src_dt, src_metadata,
                            kernreq, ectx,
                            this);
        }

        size_t extra_size = sizeof(date_strftime_kernel_extra);
        out->ensure_capacity_leaf(offset_out + extra_size);
        date_strftime_kernel_extra *e = out->get_at<date_strftime_kernel_extra>(offset_out);
        switch (kernreq) {
            case kernel_request_single:
                e->base.set_function<unary_single_operation_t>(&date_strftime_kernel_extra::single_unary);
                break;
            case kernel_request_strided:
                e->base.set_function<unary_strided_operation_t>(&date_strftime_kernel_extra::strided_unary);
                break;
            default: {
                stringstream ss;
                ss << "date_strftime_kernel_generator: unrecognized request " << (int)kernreq;
                throw runtime_error(ss.str());
            }
        }
        // The lifetime of kernels must be shorter than that of the kernel generator,
        // so we can point at data in the kernel generator
        e->format_size = m_format.size();
        e->format = m_format.c_str();
        e->dst_metadata = reinterpret_cast<const string_dtype_metadata *>(dst_metadata);
        return offset_out + extra_size;
    }

    void print_dtype(std::ostream& o) const
    {
        o << "strftime(op0, ";
        print_escaped_utf8_string(o, m_format);
        o << ")";
    }
};


expr_kernel_generator *dynd::make_strftime_kernelgen(const std::string& format)
{
    return new date_strftime_kernel_generator(format);
}