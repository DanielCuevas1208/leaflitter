#include "lf_stream.h"

#include "lf_adler32.h"
#include "lf_block.h"
#include "lf_match.h"

#include <stdint.h>
#include <string.h>

#define LF_HEADER_SIZE 24u
#define LF_MIN_BLOCK 256u
#define LF_PREALLOC_LIMIT ((size_t)1 << 20)

static lf_result lf_options_check(const lf_options *in, lf_options *out)
{
    if (in == NULL) {
        lf_options_default(out);
        return LF_OK;
    }
    *out = *in;
    if (out->window_size == 0) {
        out->window_size = LF_DEFAULT_WINDOW;
    }
    if (out->window_size < LF_MIN_MATCH || out->window_size > LF_MAX_WINDOW) {
        return LF_ERR_PARAM;
    }
    if (out->block_size == 0) {
        out->block_size = LF_DEFAULT_BLOCK;
    }
    if (out->block_size < LF_MIN_BLOCK || out->block_size > LF_MAX_BLOCK) {
        return LF_ERR_PARAM;
    }
    if (out->max_chain == 0) {
        out->max_chain = LF_DEFAULT_MAX_CHAIN;
    }
    return LF_OK;
}

static void lf_put_u64le(uint8_t *p, uint64_t v)
{
    unsigned i;

    for (i = 0; i < 8; i++) {
        p[i] = (uint8_t)(v & 0xFFu);
        v >>= 8;
    }
}

static void lf_put_u32le(uint8_t *p, uint32_t v)
{
    unsigned i;

    for (i = 0; i < 4; i++) {
        p[i] = (uint8_t)(v & 0xFFu);
        v >>= 8;
    }
}

static uint64_t lf_get_u64le(const uint8_t *p)
{
    uint64_t v = 0;
    unsigned i;

    for (i = 0; i < 8; i++) {
        v |= (uint64_t)p[i] << (8 * i);
    }
    return v;
}

static uint32_t lf_get_u32le(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static lf_result lf_write_header(lf_buf *out, uint64_t size, uint32_t adler,
                                 uint32_t window)
{
    uint8_t h[LF_HEADER_SIZE];

    memset(h, 0, sizeof(h));
    h[0] = LF_MAGIC_0;
    h[1] = LF_MAGIC_1;
    h[2] = LF_MAGIC_2;
    h[3] = (uint8_t)LF_FORMAT_VERSION;
    lf_put_u64le(h + 4, size);
    lf_put_u32le(h + 12, adler);
    lf_put_u32le(h + 16, window);
    return lf_buf_append(out, h, sizeof(h));
}

static lf_result lf_read_header(lf_breader *r, uint64_t *size, uint32_t *adler,
                                uint32_t *window)
{
    uint8_t h[LF_HEADER_SIZE];

    if (!lf_breader_read_bytes(r, h, sizeof(h))) {
        return LF_ERR_TRUNCATED;
    }
    if (h[0] != LF_MAGIC_0 || h[1] != LF_MAGIC_1 || h[2] != LF_MAGIC_2) {
        return LF_ERR_BAD_MAGIC;
    }
    if (h[3] != LF_FORMAT_VERSION) {
        return LF_ERR_UNSUPPORTED_VERSION;
    }
    if (lf_get_u32le(h + 20) != 0) {
        return LF_ERR_INVALID_DATA;
    }
    *size = lf_get_u64le(h + 4);
    *adler = lf_get_u32le(h + 12);
    *window = lf_get_u32le(h + 16);
    if (*window < LF_MIN_MATCH || *window > LF_MAX_WINDOW) {
        return LF_ERR_INVALID_DATA;
    }
    return LF_OK;
}

size_t lf_compress_bound(size_t src_len)
{
    size_t bits;
    size_t blocks;

    if (src_len > (size_t)((uint64_t)-1) / 15) {
        return (size_t)((uint64_t)-1);
    }
    bits = src_len * 15 + 8;
    blocks = src_len / LF_MIN_BLOCK + 1;
    return LF_HEADER_SIZE + bits / 8 + blocks * 250 + 8;
}

lf_result lf_compress(const lf_options *opt, const void *src, size_t src_len,
                      lf_buf *out)
{
    lf_options o;
    lf_match m;
    uint32_t adler;
    lf_result res;
    size_t pos = 0;
    int have_match = 0;

    if (out == NULL) {
        return LF_ERR_PARAM;
    }
    if (src_len > 0 && src == NULL) {
        return LF_ERR_PARAM;
    }
    if (src_len > (size_t)0xFFFFFFFFu) {
        return LF_ERR_PARAM;
    }
    res = lf_options_check(opt, &o);
    if (res != LF_OK) {
        return res;
    }

    adler = lf_adler32(src, src_len);
    res = lf_write_header(out, (uint64_t)src_len, adler, o.window_size);
    if (res != LF_OK) {
        return res;
    }
    if (src_len == 0) {
        return LF_OK;
    }

    res = lf_match_init(&m, o.window_size, o.max_chain);
    if (res != LF_OK) {
        return res;
    }
    have_match = 1;

    while (pos < src_len) {
        size_t blen = src_len - pos;
        int final;

        if (blen > o.block_size) {
            blen = o.block_size;
        }
        final = (pos + blen == src_len);
        res = lf_block_encode(&o, &m, (const uint8_t *)src, pos, blen, src_len,
                              final, out);
        if (res != LF_OK) {
            break;
        }
        pos += blen;
    }

    if (have_match) {
        lf_match_free(&m);
    }
    return res;
}

lf_result lf_decompress(const void *src, size_t src_len, lf_buf *out)
{
    lf_breader r;
    uint64_t decl_size;
    uint32_t adler;
    uint32_t window;
    lf_result res;
    int final;
    int saw_block = 0;

    if (src == NULL || out == NULL) {
        return LF_ERR_PARAM;
    }
    if (src_len < LF_HEADER_SIZE) {
        return LF_ERR_TRUNCATED;
    }

    lf_breader_init(&r, src, src_len);
    res = lf_read_header(&r, &decl_size, &adler, &window);
    if (res != LF_OK) {
        return res;
    }
    if (decl_size > (uint64_t)SIZE_MAX) {
        return LF_ERR_BAD_LENGTH;
    }

    if (decl_size > 0) {
        size_t prealloc = (size_t)decl_size;

        if (prealloc > LF_PREALLOC_LIMIT) {
            prealloc = LF_PREALLOC_LIMIT;
        }
        (void)lf_buf_reserve(out, prealloc);
    }

    while (r.pos < r.len) {
        res = lf_block_decode(&r, out, (size_t)decl_size, window, &final);
        if (res != LF_OK) {
            return res;
        }
        saw_block = 1;
        if (final) {
            break;
        }
    }

    if (decl_size > 0 && !saw_block) {
        return LF_ERR_TRUNCATED;
    }
    if (out->len != (size_t)decl_size) {
        return LF_ERR_BAD_LENGTH;
    }
    if (lf_adler32(out->data, out->len) != adler) {
        return LF_ERR_INVALID_DATA;
    }
    if (lf_breader_bytes_read(&r) != src_len) {
        return LF_ERR_INVALID_DATA;
    }
    return LF_OK;
}
