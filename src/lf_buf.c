#include "lf_buf.h"

#include <stdlib.h>
#include <string.h>

void lf_buf_init(lf_buf *b)
{
    b->data = NULL;
    b->len = 0;
    b->cap = 0;
}

void lf_buf_free(lf_buf *b)
{
    free(b->data);
    b->data = NULL;
    b->len = 0;
    b->cap = 0;
}

static size_t lf_buf_grow_cap(size_t current, size_t need)
{
    size_t cap = current ? current : 256u;
    while (cap < need) {
        cap = cap + (cap >> 1u) + 64u;
    }
    return cap;
}

lf_result lf_buf_reserve(lf_buf *b, size_t extra)
{
    size_t need;
    size_t cap;
    uint8_t *p;

    if (b->len > (size_t)-1 - extra) {
        return LF_ERR_PARAM;
    }
    need = b->len + extra;
    if (need <= b->cap) {
        return LF_OK;
    }
    cap = lf_buf_grow_cap(b->cap, need);
    p = (uint8_t *)realloc(b->data, cap);
    if (p == NULL) {
        return LF_ERR_ALLOC;
    }
    b->data = p;
    b->cap = cap;
    return LF_OK;
}

lf_result lf_buf_append(lf_buf *b, const void *data, size_t len)
{
    lf_result r;

    if (len == 0) {
        return LF_OK;
    }
    r = lf_buf_reserve(b, len);
    if (r != LF_OK) {
        return r;
    }
    memcpy(b->data + b->len, data, len);
    b->len += len;
    return LF_OK;
}

lf_result lf_buf_append_u8(lf_buf *b, uint8_t v)
{
    return lf_buf_append(b, &v, 1u);
}

void lf_buf_clear(lf_buf *b)
{
    b->len = 0;
}

lf_result lf_buf_put_u16le(lf_buf *b, uint16_t v)
{
    uint8_t t[2];

    t[0] = (uint8_t)(v & 0xFFu);
    t[1] = (uint8_t)((v >> 8u) & 0xFFu);
    return lf_buf_append(b, t, sizeof(t));
}

lf_result lf_buf_put_u32le(lf_buf *b, uint32_t v)
{
    uint8_t t[4];

    t[0] = (uint8_t)(v & 0xFFu);
    t[1] = (uint8_t)((v >> 8u) & 0xFFu);
    t[2] = (uint8_t)((v >> 16u) & 0xFFu);
    t[3] = (uint8_t)((v >> 24u) & 0xFFu);
    return lf_buf_append(b, t, sizeof(t));
}

lf_result lf_buf_put_u64le(lf_buf *b, uint64_t v)
{
    uint8_t t[8];
    unsigned i;

    for (i = 0; i < 8; i++) {
        t[i] = (uint8_t)(v & 0xFFu);
        v >>= 8u;
    }
    return lf_buf_append(b, t, sizeof(t));
}
