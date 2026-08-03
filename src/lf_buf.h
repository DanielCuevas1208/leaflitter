#ifndef LF_BUF_H
#define LF_BUF_H

#include "lf_common.h"

typedef struct lf_buf {
    uint8_t *data;
    size_t len;
    size_t cap;
} lf_buf;

LF_EXPORT void lf_buf_init(lf_buf *b);

LF_EXPORT void lf_buf_free(lf_buf *b);

LF_EXPORT lf_result lf_buf_reserve(lf_buf *b, size_t extra);

LF_EXPORT lf_result lf_buf_append(lf_buf *b, const void *data, size_t len);

LF_EXPORT lf_result lf_buf_append_u8(lf_buf *b, uint8_t v);

LF_EXPORT void lf_buf_clear(lf_buf *b);

LF_EXPORT lf_result lf_buf_put_u32le(lf_buf *b, uint32_t v);

LF_EXPORT lf_result lf_buf_put_u64le(lf_buf *b, uint64_t v);

#endif /* LF_BUF_H */
