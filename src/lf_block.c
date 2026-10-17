#include "lf_block.h"

#include "lf_huffman.h"

#include <stdlib.h>
#include <string.h>

/*
 * Length codes 257..272 cover lengths 3..258.
 * Distance codes 0..29 cover distances 1..65534.
 */

#define LF_LEN_CODES 16u
#define LF_DIST_CODES 30u
#define LF_FIRST_LEN 257u
#define LF_CL_SYMBOLS 19u

#define LF_BLOCK_TYPE_STORED 0u
#define LF_BLOCK_TYPE_HUFFMAN 1u
#define LF_BLOCK_FLAG_FINAL 0x04u

static const uint8_t lf_len_extra[LF_LEN_CODES] = {
    0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6
};
static const uint16_t lf_len_base[LF_LEN_CODES] = {
    3, 4, 5, 6, 7, 9, 11, 15, 19, 27, 35, 51, 67, 99, 131, 195
};
static const uint8_t lf_dist_extra[LF_DIST_CODES] = {
    0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9,
    10, 10, 11, 11, 12, 12, 13, 13, 14, 14
};
static const uint16_t lf_dist_base[LF_DIST_CODES] = {
    1, 2, 3, 5, 7, 11, 15, 23, 31, 47, 63, 95, 127, 191, 255, 383,
    511, 767, 1023, 1535, 2047, 3071, 4095, 6143, 8191, 12287, 16383,
    24575, 32767, 49151
};

/* Fixed canonical code-length tree. Symbols 16, 17, 18 are run codes. */
static const uint8_t lf_cl_len[LF_CL_SYMBOLS] = {
    4, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 3, 2, 2
};

typedef struct lf_token {
    uint16_t kind;
    uint16_t lit;
    uint16_t len_code;
    uint16_t len_extra;
    uint16_t dist_code;
    uint16_t dist_extra;
} lf_token;

static unsigned lf_len_lookup(unsigned len, unsigned *extra)
{
    unsigned k;

    for (k = 0; k < LF_LEN_CODES; k++) {
        unsigned top = (unsigned)lf_len_base[k] + ((1u << lf_len_extra[k]) - 1u);

        if (len <= top) {
            *extra = len - (unsigned)lf_len_base[k];
            return k;
        }
    }
    *extra = 0;
    return 0;
}

static unsigned lf_dist_lookup(uint32_t dist, unsigned *extra)
{
    unsigned k;

    for (k = 0; k < LF_DIST_CODES; k++) {
        uint32_t top = (uint32_t)lf_dist_base[k] +
                       ((1u << lf_dist_extra[k]) - 1u);

        if (dist <= top) {
            *extra = dist - (uint32_t)lf_dist_base[k];
            return k;
        }
    }
    *extra = 0;
    return 0;
}

static void lf_cl_build(uint32_t *codes, uint8_t *lengths)
{
    memcpy(lengths, lf_cl_len, sizeof(lf_cl_len));
    lf_huff_canonical(codes, lf_cl_len, LF_CL_SYMBOLS);
}

static lf_result lf_cl_write(lf_bwriter *w, const uint8_t *lengths, size_t n,
                             const uint32_t *cl_code, const uint8_t *cl_len)
{
    size_t i = 0;
    lf_result r;

    while (i < n) {
        uint8_t v = lengths[i];
        size_t run = 1;

        while (i + run < n && lengths[i + run] == v) {
            run++;
        }

        if (v == 0) {
            while (run > 0) {
                if (run >= 11) {
                    size_t take = run > 138 ? 138 : run;

                    r = lf_bwriter_put_code(w, cl_code[18], cl_len[18]);
                    if (r != LF_OK) {
                        return r;
                    }
                    r = lf_bwriter_put(w, (uint32_t)(take - 11), 7);
                    if (r != LF_OK) {
                        return r;
                    }
                    i += take;
                    run -= take;
                } else if (run >= 3) {
                    r = lf_bwriter_put_code(w, cl_code[17], cl_len[17]);
                    if (r != LF_OK) {
                        return r;
                    }
                    r = lf_bwriter_put(w, (uint32_t)(run - 3), 3);
                    if (r != LF_OK) {
                        return r;
                    }
                    i += run;
                    run = 0;
                } else if (run == 2) {
                    r = lf_bwriter_put_code(w, cl_code[0], cl_len[0]);
                    if (r != LF_OK) {
                        return r;
                    }
                    r = lf_bwriter_put_code(w, cl_code[0], cl_len[0]);
                    if (r != LF_OK) {
                        return r;
                    }
                    i += 2;
                    run = 0;
                } else {
                    r = lf_bwriter_put_code(w, cl_code[0], cl_len[0]);
                    if (r != LF_OK) {
                        return r;
                    }
                    i += 1;
                    run = 0;
                }
            }
        } else {
            r = lf_bwriter_put_code(w, cl_code[v], cl_len[v]);
            if (r != LF_OK) {
                return r;
            }
            i++;
            run--;
            while (run > 0) {
                if (run >= 3) {
                    size_t take = run > 6 ? 6 : run;

                    r = lf_bwriter_put_code(w, cl_code[16], cl_len[16]);
                    if (r != LF_OK) {
                        return r;
                    }
                    r = lf_bwriter_put(w, (uint32_t)(take - 3), 2);
                    if (r != LF_OK) {
                        return r;
                    }
                    i += take;
                    run -= take;
                } else {
                    r = lf_bwriter_put_code(w, cl_code[v], cl_len[v]);
                    if (r != LF_OK) {
                        return r;
                    }
                    i++;
                    run--;
                }
            }
        }
    }
    return LF_OK;
}

static lf_result lf_cl_read(lf_breader *r, uint8_t *lengths, size_t n)
{
    lf_huff_dec cl;
    uint8_t cl_len[LF_CL_SYMBOLS];
    uint32_t cl_code[LF_CL_SYMBOLS];
    size_t i = 0;
    int last = 0;

    lf_cl_build(cl_code, cl_len);
    lf_huff_dec_init(&cl, cl_len, LF_CL_SYMBOLS);

    while (i < n) {
        int sym = lf_huff_dec_symbol(&cl, r);
        uint32_t extra;
        size_t ncopy;

        if (sym < 0) {
            return LF_ERR_TRUNCATED;
        }
        if (sym <= 15) {
            lengths[i++] = (uint8_t)sym;
            last = sym;
        } else if (sym == 16) {
            extra = lf_breader_get(r, 2);
            if (!lf_breader_ok(r)) {
                return LF_ERR_TRUNCATED;
            }
            ncopy = 3u + extra;
            if (i + ncopy > n) {
                return LF_ERR_INVALID_DATA;
            }
            while (ncopy-- > 0) {
                lengths[i++] = (uint8_t)last;
            }
        } else if (sym == 17) {
            extra = lf_breader_get(r, 3);
            if (!lf_breader_ok(r)) {
                return LF_ERR_TRUNCATED;
            }
            ncopy = 3u + extra;
            if (i + ncopy > n) {
                return LF_ERR_INVALID_DATA;
            }
            while (ncopy-- > 0) {
                lengths[i++] = 0;
            }
            last = 0;
        } else {
            extra = lf_breader_get(r, 7);
            if (!lf_breader_ok(r)) {
                return LF_ERR_TRUNCATED;
            }
            ncopy = 11u + extra;
            if (i + ncopy > n) {
                return LF_ERR_INVALID_DATA;
            }
            while (ncopy-- > 0) {
                lengths[i++] = 0;
            }
            last = 0;
        }
    }
    return LF_OK;
}

static lf_result lf_write_tokens(lf_bwriter *w, const lf_token *tokens,
                                 size_t ntok, const uint32_t *lit_code,
                                 const uint8_t *lit_len, const uint32_t *dist_code,
                                 const uint8_t *dist_len)
{
    size_t i;
    lf_result r;

    for (i = 0; i < ntok; i++) {
        const lf_token *t = &tokens[i];

        if (t->kind == 0) {
            r = lf_bwriter_put_code(w, lit_code[t->lit], lit_len[t->lit]);
            if (r != LF_OK) {
                return r;
            }
        } else {
            uint32_t sym = LF_FIRST_LEN + t->len_code;

            r = lf_bwriter_put_code(w, lit_code[sym], lit_len[sym]);
            if (r != LF_OK) {
                return r;
            }
            r = lf_bwriter_put(w, t->len_extra, lf_len_extra[t->len_code]);
            if (r != LF_OK) {
                return r;
            }
            r = lf_bwriter_put_code(w, dist_code[t->dist_code],
                                    dist_len[t->dist_code]);
            if (r != LF_OK) {
                return r;
            }
            r = lf_bwriter_put(w, t->dist_extra, lf_dist_extra[t->dist_code]);
            if (r != LF_OK) {
                return r;
            }
        }
    }
    r = lf_bwriter_put_code(w, lit_code[LF_EOB], lit_len[LF_EOB]);
    if (r != LF_OK) {
        return r;
    }
    return lf_bwriter_align(w);
}

lf_result lf_block_encode(lf_options *opt, lf_match *m, const uint8_t *src,
                          size_t base, size_t len, size_t total, int final,
                          lf_buf *out)
{
    lf_token *tokens;
    uint32_t freq_lit[LF_NUM_LITLEN];
    uint32_t freq_dist[LF_NUM_DIST];
    uint8_t lit_len[LF_NUM_LITLEN];
    uint8_t dist_len[LF_NUM_DIST];
    uint32_t lit_code[LF_NUM_LITLEN];
    uint32_t dist_code[LF_NUM_DIST];
    uint32_t cl_code[LF_CL_SYMBOLS];
    uint8_t cl_len[LF_CL_SYMBOLS];
    size_t ntok;
    uint16_t nlit;
    uint16_t ndist;
    lf_buf tmp;
    lf_bwriter w;
    lf_result r;

    if (opt == NULL || m == NULL || src == NULL || out == NULL) {
        return LF_ERR_PARAM;
    }
    if (len == 0 || base + len > total) {
        return LF_ERR_PARAM;
    }

    memset(freq_lit, 0, sizeof(freq_lit));
    memset(freq_dist, 0, sizeof(freq_dist));

    tokens = (lf_token *)malloc(len * sizeof(lf_token));
    if (tokens == NULL) {
        return LF_ERR_ALLOC;
    }

    ntok = 0;
    {
        size_t pos = 0;

        while (pos < len) {
            unsigned ml = 0;
            uint32_t md = 0;

            if (len - pos >= LF_MIN_MATCH) {
                r = lf_match_search(m, src, base + pos, total, &ml, &md);
                if (r != LF_OK) {
                    free(tokens);
                    return r;
                }
            }
            if (ml >= LF_MIN_MATCH && md >= 1 && md <= opt->window_size) {
                lf_token *t = &tokens[ntok++];
                unsigned le;
                unsigned de;

                if (ml > len - pos) {
                    ml = (unsigned)(len - pos);
                }
                if (ml < LF_MIN_MATCH) {
                    t->kind = 0;
                    t->lit = src[base + pos];
                    t->len_code = 0;
                    t->len_extra = 0;
                    t->dist_code = 0;
                    t->dist_extra = 0;
                    freq_lit[src[base + pos]]++;
                    lf_match_insert(m, src, base + pos, total);
                    pos++;
                    continue;
                }
                t->kind = 1;
                t->lit = 0;
                t->len_code = (uint16_t)lf_len_lookup(ml, &le);
                t->len_extra = (uint16_t)le;
                t->dist_code = (uint16_t)lf_dist_lookup(md, &de);
                t->dist_extra = (uint16_t)de;
                freq_lit[LF_FIRST_LEN + t->len_code]++;
                freq_dist[t->dist_code]++;
                {
                    unsigned k;

                    for (k = 0; k < ml; k++) {
                        lf_match_insert(m, src, base + pos + k, total);
                    }
                }
                pos += ml;
            } else {
                lf_token *t = &tokens[ntok++];

                t->kind = 0;
                t->lit = src[base + pos];
                t->len_code = 0;
                t->len_extra = 0;
                t->dist_code = 0;
                t->dist_extra = 0;
                freq_lit[src[base + pos]]++;
                lf_match_insert(m, src, base + pos, total);
                pos++;
            }
        }
    }

    freq_lit[LF_EOB]++;

    lf_huff_lengths(lit_len, freq_lit, LF_NUM_LITLEN);
    lf_huff_lengths(dist_len, freq_dist, LF_NUM_DIST);
    lf_huff_canonical(lit_code, lit_len, LF_NUM_LITLEN);
    lf_huff_canonical(dist_code, dist_len, LF_NUM_DIST);

    nlit = LF_NUM_LITLEN;
    while (nlit > LF_FIRST_LEN && lit_len[nlit - 1] == 0) {
        nlit--;
    }
    ndist = LF_NUM_DIST;
    while (ndist > 1 && dist_len[ndist - 1] == 0) {
        ndist--;
    }

    lf_buf_init(&tmp);
    lf_bwriter_init(&w, &tmp);
    lf_cl_build(cl_code, cl_len);

    r = lf_buf_append_u8(&tmp,
                         (uint8_t)(LF_BLOCK_TYPE_HUFFMAN |
                                   (final ? LF_BLOCK_FLAG_FINAL : 0)));
    if (r != LF_OK) {
        goto done;
    }
    r = lf_buf_put_u16le(&tmp, nlit);
    if (r != LF_OK) {
        goto done;
    }
    r = lf_buf_put_u16le(&tmp, ndist);
    if (r != LF_OK) {
        goto done;
    }
    r = lf_cl_write(&w, lit_len, nlit, cl_code, cl_len);
    if (r != LF_OK) {
        goto done;
    }
    r = lf_cl_write(&w, dist_len, ndist, cl_code, cl_len);
    if (r != LF_OK) {
        goto done;
    }
    r = lf_write_tokens(&w, tokens, ntok, lit_code, lit_len, dist_code, dist_len);
    if (r != LF_OK) {
        goto done;
    }

    if (tmp.len < 5 + len) {
        r = lf_buf_append(out, tmp.data, tmp.len);
    } else {
        r = lf_buf_append_u8(out,
                             (uint8_t)(LF_BLOCK_TYPE_STORED |
                                       (final ? LF_BLOCK_FLAG_FINAL : 0)));
        if (r == LF_OK) {
            r = lf_buf_put_u32le(out, (uint32_t)len);
        }
        if (r == LF_OK) {
            r = lf_buf_append(out, src + base, len);
        }
    }

done:
    lf_buf_free(&tmp);
    free(tokens);
    return r;
}

lf_result lf_block_decode(lf_breader *r, lf_buf *out, size_t max_out,
                          uint32_t window, int *final)
{
    uint8_t hdr;
    unsigned type;

    if (r == NULL || out == NULL || final == NULL) {
        return LF_ERR_PARAM;
    }
    *final = 0;
    if (!lf_breader_read_bytes(r, &hdr, 1)) {
        return LF_ERR_TRUNCATED;
    }
    type = (unsigned)(hdr & 0x03u);
    if ((hdr & 0xF8u) != 0) {
        return LF_ERR_INVALID_DATA;
    }
    *final = (hdr & LF_BLOCK_FLAG_FINAL) != 0;

    if (type == LF_BLOCK_TYPE_STORED) {
        uint8_t lb[4];
        uint32_t slen;
        lf_result res;

        if (!lf_breader_read_bytes(r, lb, 4)) {
            return LF_ERR_TRUNCATED;
        }
        slen = (uint32_t)lb[0] | ((uint32_t)lb[1] << 8) |
               ((uint32_t)lb[2] << 16) | ((uint32_t)lb[3] << 24);
        if (slen == 0) {
            return LF_ERR_INVALID_DATA;
        }
        if (out->len > max_out || (size_t)slen > max_out - out->len) {
            return LF_ERR_BAD_LENGTH;
        }
        if (r->pos > r->len || (size_t)slen > r->len - r->pos) {
            return LF_ERR_TRUNCATED;
        }
        res = lf_buf_reserve(out, slen);
        if (res != LF_OK) {
            return res;
        }
        memcpy(out->data + out->len, r->data + r->pos, slen);
        out->len += slen;
        r->pos += slen;
        return LF_OK;
    }

    if (type == LF_BLOCK_TYPE_HUFFMAN) {
        uint8_t nb[2];
        uint16_t nlit;
        uint16_t ndist;
        uint8_t lit_len[LF_NUM_LITLEN];
        uint8_t dist_len[LF_NUM_DIST];
        lf_huff_dec lit_dec;
        lf_huff_dec dist_dec;
        lf_result res;

        if (!lf_breader_read_bytes(r, nb, 2)) {
            return LF_ERR_TRUNCATED;
        }
        nlit = (uint16_t)(nb[0] | (uint16_t)(nb[1] << 8));
        if (!lf_breader_read_bytes(r, nb, 2)) {
            return LF_ERR_TRUNCATED;
        }
        ndist = (uint16_t)(nb[0] | (uint16_t)(nb[1] << 8));
        if (nlit < LF_FIRST_LEN || nlit > LF_NUM_LITLEN) {
            return LF_ERR_INVALID_DATA;
        }
        if (ndist < 1 || ndist > LF_NUM_DIST) {
            return LF_ERR_INVALID_DATA;
        }

        memset(lit_len, 0, sizeof(lit_len));
        memset(dist_len, 0, sizeof(dist_len));
        res = lf_cl_read(r, lit_len, nlit);
        if (res != LF_OK) {
            return res;
        }
        res = lf_cl_read(r, dist_len, ndist);
        if (res != LF_OK) {
            return res;
        }

        lf_huff_dec_init(&lit_dec, lit_len, LF_NUM_LITLEN);
        if (!lit_dec.valid) {
            return LF_ERR_INVALID_DATA;
        }
        lf_huff_dec_init(&dist_dec, dist_len, LF_NUM_DIST);

        for (;;) {
            int sym = lf_huff_dec_symbol(&lit_dec, r);

            if (sym < 0) {
                return LF_ERR_TRUNCATED;
            }
            if (sym < 256) {
                if (out->len >= max_out) {
                    return LF_ERR_BAD_LENGTH;
                }
                res = lf_buf_append_u8(out, (uint8_t)sym);
                if (res != LF_OK) {
                    return res;
                }
            } else if (sym == LF_EOB) {
                lf_breader_align(r);
                return LF_OK;
            } else if (sym <= (int)(LF_FIRST_LEN + LF_LEN_CODES - 1u)) {
                unsigned code = (unsigned)(sym - LF_FIRST_LEN);
                uint32_t extra = lf_breader_get(r, lf_len_extra[code]);
                uint32_t length;
                uint32_t dist;
                uint32_t dextra;
                int dcode;
                size_t start;
                size_t i;

                if (!lf_breader_ok(r)) {
                    return LF_ERR_TRUNCATED;
                }
                length = (uint32_t)lf_len_base[code] + extra;
                dcode = lf_huff_dec_symbol(&dist_dec, r);
                if (dcode < 0 || (unsigned)dcode >= LF_DIST_CODES) {
                    return LF_ERR_INVALID_DATA;
                }
                if (!dist_dec.valid) {
                    return LF_ERR_INVALID_DATA;
                }
                dextra = lf_breader_get(r, lf_dist_extra[dcode]);
                if (!lf_breader_ok(r)) {
                    return LF_ERR_TRUNCATED;
                }
                dist = (uint32_t)lf_dist_base[dcode] + dextra;
                if (dist == 0 || dist > out->len || dist > window) {
                    return LF_ERR_INVALID_DATA;
                }
                if (out->len > max_out || (size_t)length > max_out - out->len) {
                    return LF_ERR_BAD_LENGTH;
                }
                res = lf_buf_reserve(out, length);
                if (res != LF_OK) {
                    return res;
                }
                start = out->len;
                for (i = 0; i < length; i++) {
                    out->data[start + i] = out->data[start + i - dist];
                }
                out->len += length;
            } else {
                return LF_ERR_INVALID_DATA;
            }
        }
    }

    return LF_ERR_INVALID_DATA;
}

