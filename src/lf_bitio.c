#include "lf_bitio.h"

#include <string.h>

void lf_bwriter_init(lf_bwriter *w, lf_buf *out)
{
    w->out = out;
    w->acc = 0;
    w->nbits = 0;
}

lf_result lf_bwriter_put(lf_bwriter *w, uint32_t value, unsigned nbits)
{
    lf_result r;

    if (nbits > 32u) {
        return LF_ERR_PARAM;
    }
    if (nbits > 0) {
        w->acc = (w->acc << nbits) | (uint64_t)(value & 0xFFFFFFFFu);
        w->nbits += nbits;
    }
    while (w->nbits >= 8u) {
        uint8_t byte;

        byte = (uint8_t)((w->acc >> (w->nbits - 8u)) & 0xFFu);
        r = lf_buf_append_u8(w->out, byte);
        if (r != LF_OK) {
            return r;
        }
        w->nbits -= 8u;
        w->acc &= 0x7Fu;
    }
    return LF_OK;
}

lf_result lf_bwriter_put_code(lf_bwriter *w, uint32_t code, unsigned nbits)
{
    return lf_bwriter_put(w, code, nbits);
}

lf_result lf_bwriter_align(lf_bwriter *w)
{
    lf_result r;

    if (w->nbits > 0) {
        uint8_t byte;

        byte = (uint8_t)((w->acc << (8u - w->nbits)) & 0xFFu);
        r = lf_buf_append_u8(w->out, byte);
        if (r != LF_OK) {
            return r;
        }
        w->nbits = 0;
        w->acc = 0;
    }
    return LF_OK;
}

void lf_breader_init(lf_breader *r, const void *data, size_t len)
{
    r->data = (const uint8_t *)data;
    r->len = len;
    r->pos = 0;
    r->acc = 0;
    r->nbits = 0;
    r->valid = 1;
}

uint32_t lf_breader_get(lf_breader *r, unsigned nbits)
{
    uint32_t result;

    if (nbits > 32u) {
        r->valid = 0;
        return 0;
    }
    while (r->nbits < nbits) {
        if (r->pos >= r->len) {
            r->valid = 0;
            return 0;
        }
        r->acc = (r->acc << 8u) | r->data[r->pos];
        r->pos += 1u;
        r->nbits += 8u;
    }
    result = (uint32_t)((r->acc >> (r->nbits - nbits)) & ((uint64_t)0xFFFFFFFFu >> (32u - nbits)));
    r->nbits -= nbits;
    r->acc &= ((uint64_t)1 << r->nbits) - 1u;
    return result;
}

size_t lf_breader_bytes_read(lf_breader *r)
{
    return r->pos;
}
