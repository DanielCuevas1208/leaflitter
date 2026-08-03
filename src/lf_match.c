#include "lf_match.h"

#include <stdlib.h>
#include <string.h>

static uint32_t lf_match_hash(const uint8_t *p)
{
    uint32_t h = ((uint32_t)p[0] << 16) | ((uint32_t)p[1] << 8) | (uint32_t)p[2];

    h ^= h >> 15;
    h *= 0x1E35A7BDu;
    h ^= h >> 13;
    return h;
}

lf_result lf_match_init(lf_match *m, uint32_t window, uint32_t max_chain)
{
    uint32_t table_size;

    if (m == NULL) {
        return LF_ERR_PARAM;
    }
    m->head = NULL;
    m->prev = NULL;
    m->window = 0;
    m->max_chain = 0;
    m->hash_mask = 0;

    if (window < LF_MIN_MATCH || window > LF_MAX_WINDOW) {
        return LF_ERR_PARAM;
    }
    if (max_chain == 0) {
        return LF_ERR_PARAM;
    }

    table_size = (uint32_t)1 << LF_MATCH_HASH_BITS;
    m->head = (uint32_t *)calloc(table_size, sizeof(uint32_t));
    m->prev = (uint32_t *)calloc(window, sizeof(uint32_t));
    if (m->head == NULL || m->prev == NULL) {
        free(m->head);
        free(m->prev);
        m->head = NULL;
        m->prev = NULL;
        return LF_ERR_ALLOC;
    }

    m->window = window;
    m->max_chain = max_chain;
    m->hash_mask = table_size - 1u;
    return LF_OK;
}

void lf_match_free(lf_match *m)
{
    if (m == NULL) {
        return;
    }
    free(m->head);
    free(m->prev);
    m->head = NULL;
    m->prev = NULL;
    m->window = 0;
    m->max_chain = 0;
    m->hash_mask = 0;
}

void lf_match_reset(lf_match *m)
{
    uint32_t table_size;

    if (m == NULL) {
        return;
    }
    table_size = m->hash_mask + 1u;
    memset(m->head, 0, (size_t)table_size * sizeof(uint32_t));
    memset(m->prev, 0, (size_t)m->window * sizeof(uint32_t));
}

lf_result lf_match_search(lf_match *m, const uint8_t *src, size_t pos,
                          size_t src_len, unsigned *match_len, uint32_t *match_dist)
{
    uint32_t hash;
    uint32_t cand;
    unsigned chain;
    unsigned best = 0;
    uint32_t best_dist = 0;

    if (m == NULL || src == NULL || match_len == NULL || match_dist == NULL) {
        return LF_ERR_PARAM;
    }
    *match_len = 0;
    *match_dist = 0;
    if (pos + LF_MIN_MATCH > src_len) {
        return LF_OK;
    }

    hash = lf_match_hash(src + pos);
    cand = m->head[hash & m->hash_mask];
    for (chain = 0; cand != LF_LZ_NONE && chain < m->max_chain; chain++) {
        uint32_t c = cand - 1u;
        size_t k;

        if (c >= pos) {
            break;
        }
        if (pos - c > m->window) {
            break;
        }
        k = 0;
        while (k < LF_MAX_MATCH && pos + k < src_len &&
               src[c + k] == src[pos + k]) {
            k++;
        }
        if (k > best) {
            best = (unsigned)k;
            best_dist = (uint32_t)(pos - c);
            if (best == LF_MAX_MATCH) {
                break;
            }
        }
        cand = m->prev[c % m->window];
    }

    *match_len = best;
    *match_dist = best_dist;
    return LF_OK;
}

void lf_match_insert(lf_match *m, const uint8_t *src, size_t pos, size_t src_len)
{
    uint32_t hash;
    uint32_t slot;

    if (m == NULL || src == NULL) {
        return;
    }
    if (pos + LF_MIN_MATCH > src_len) {
        return;
    }
    hash = lf_match_hash(src + pos);
    slot = (uint32_t)(pos % m->window);
    m->prev[slot] = m->head[hash & m->hash_mask];
    m->head[hash & m->hash_mask] = (uint32_t)(pos + 1u);
}
