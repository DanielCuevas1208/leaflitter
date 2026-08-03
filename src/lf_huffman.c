#include "lf_huffman.h"

#include <stdlib.h>
#include <string.h>

typedef struct lf_node {
    uint32_t w;
    int32_t left;
    int32_t right;
    int32_t sym;
} lf_node;

typedef struct lf_heap {
    int32_t *items;
    size_t n;
    size_t cap;
    const lf_node *nodes;
} lf_heap;

static int lf_heap_less(const lf_heap *h, int32_t a, int32_t b)
{
    const lf_node *na = &h->nodes[a];
    const lf_node *nb = &h->nodes[b];

    if (na->w != nb->w) {
        return na->w < nb->w;
    }
    return a < b;
}

static void lf_heap_push(lf_heap *h, int32_t item)
{
    size_t i;

    if (h->n >= h->cap) {
        return;
    }
    i = h->n++;
    h->items[i] = item;
    while (i > 0) {
        size_t parent = (i - 1u) / 2u;

        if (lf_heap_less(h, h->items[i], h->items[parent])) {
            int32_t tmp = h->items[i];

            h->items[i] = h->items[parent];
            h->items[parent] = tmp;
            i = parent;
        } else {
            break;
        }
    }
}

static int32_t lf_heap_pop(lf_heap *h)
{
    int32_t top = h->items[0];
    size_t i = 0;

    h->n -= 1u;
    h->items[0] = h->items[h->n];
    for (;;) {
        size_t left = i * 2u + 1u;
        size_t right = left + 1u;
        size_t best = i;

        if (left < h->n && lf_heap_less(h, h->items[left], h->items[best])) {
            best = left;
        }
        if (right < h->n && lf_heap_less(h, h->items[right], h->items[best])) {
            best = right;
        }
        if (best == i) {
            break;
        }
        {
            int32_t tmp = h->items[i];

            h->items[i] = h->items[best];
            h->items[best] = tmp;
        }
        i = best;
    }
    return top;
}

static void lf_heap_free(lf_heap *h)
{
    free(h->items);
    h->items = NULL;
    h->n = 0;
    h->cap = 0;
}

/* Lengthens the least frequent eligible symbol to satisfy the length limit. */
static void lf_huff_limit(uint8_t *lengths, const uint32_t *freq, uint32_t n)
{
    uint64_t weight = 0;
    const uint64_t limit = (uint64_t)1 << LF_MAX_CODE_LEN;
    uint32_t l;

    for (l = 0; l < n; l++) {
        if (lengths[l] > LF_MAX_CODE_LEN) {
            lengths[l] = (uint8_t)LF_MAX_CODE_LEN;
        }
        if (lengths[l] > 0) {
            weight += (uint64_t)1 << (LF_MAX_CODE_LEN - lengths[l]);
        }
    }
    while (weight > limit) {
        uint32_t best = (uint32_t)-1;
        uint64_t best_freq = (uint64_t)-1;

        for (l = 0; l < n; l++) {
            if (lengths[l] == 0 || lengths[l] >= LF_MAX_CODE_LEN) {
                continue;
            }
            if (freq[l] < best_freq) {
                best_freq = freq[l];
                best = l;
            }
        }
        if (best == (uint32_t)-1) {
            break;
        }
        weight -= (uint64_t)1 << (LF_MAX_CODE_LEN - (uint32_t)lengths[best] - 1u);
        lengths[best] += 1u;
    }
}

void lf_huff_lengths(uint8_t *lengths, const uint32_t *freq, uint32_t n)
{
    lf_node *nodes;
    lf_heap heap;
    size_t nleaf = 0;
    size_t next_node;
    uint32_t i;
    int32_t root;

    memset(lengths, 0, n);
    for (i = 0; i < n; i++) {
        if (freq[i] > 0) {
            nleaf++;
        }
    }
    if (nleaf == 0) {
        return;
    }
    if (nleaf == 1) {
        for (i = 0; i < n; i++) {
            if (freq[i] > 0) {
                lengths[i] = 1;
            }
        }
        return;
    }

    nodes = (lf_node *)calloc(2 * nleaf, sizeof(lf_node));
    heap.items = (int32_t *)malloc((2 * nleaf) * sizeof(int32_t));
    if (nodes == NULL || heap.items == NULL) {
        free(nodes);
        free(heap.items);
        for (i = 0; i < n; i++) {
            lengths[i] = (uint8_t)LF_MAX_CODE_LEN;
        }
        return;
    }
    heap.nodes = nodes;
    heap.n = 0;
    heap.cap = 2 * nleaf;

    next_node = nleaf;
    for (i = 0; i < n; i++) {
        if (freq[i] > 0) {
            nodes[heap.n].w = freq[i];
            nodes[heap.n].left = -1;
            nodes[heap.n].right = -1;
            nodes[heap.n].sym = (int32_t)i;
            lf_heap_push(&heap, (int32_t)heap.n);
        }
    }

    while (heap.n > 1) {
        int32_t a = lf_heap_pop(&heap);
        int32_t b = lf_heap_pop(&heap);
        int32_t c = (int32_t)next_node++;

        nodes[c].w = nodes[a].w + nodes[b].w;
        nodes[c].left = a;
        nodes[c].right = b;
        nodes[c].sym = -1;
        lf_heap_push(&heap, c);
    }
    root = lf_heap_pop(&heap);
    lf_heap_free(&heap);

    {
        int32_t stack[2 * LF_NUM_LITLEN];
        unsigned depth[2 * LF_NUM_LITLEN];
        size_t top = 0;

        stack[top] = root;
        depth[top] = 0;
        top++;
        while (top > 0) {
            int32_t node;
            unsigned d;

            top--;
            node = stack[top];
            d = depth[top];
            if (nodes[node].sym >= 0) {
                lengths[nodes[node].sym] = (uint8_t)d;
            } else {
                if (top + 2u <= (2 * LF_NUM_LITLEN)) {
                    stack[top] = nodes[node].left;
                    depth[top] = d + 1u;
                    top++;
                    stack[top] = nodes[node].right;
                    depth[top] = d + 1u;
                    top++;
                }
            }
        }
    }

    free(nodes);

    lf_huff_limit(lengths, freq, n);
}

unsigned lf_huff_canonical(uint32_t *codes, const uint8_t *lengths, uint32_t n)
{
    uint32_t count[LF_MAX_CODE_LEN + 1];
    uint32_t next_code[LF_MAX_CODE_LEN + 1];
    unsigned maxlen = 0;
    unsigned bits;
    uint32_t i;
    uint32_t code = 0;

    memset(count, 0, sizeof(count));
    for (i = 0; i < n; i++) {
        if (lengths[i] > 0) {
            count[lengths[i]]++;
            if (lengths[i] > maxlen) {
                maxlen = lengths[i];
            }
        }
    }
    for (bits = 1; bits <= maxlen; bits++) {
        code = (code + count[bits - 1]) << 1;
        next_code[bits] = code;
    }
    for (i = 0; i < n; i++) {
        if (lengths[i] > 0) {
            codes[i] = next_code[lengths[i]]++;
        } else {
            codes[i] = 0;
        }
    }
    return maxlen;
}

void lf_huff_dec_init(lf_huff_dec *d, const uint8_t *lengths, uint32_t n)
{
    unsigned bits;
    uint32_t i;

    memset(d->counts, 0, sizeof(d->counts));
    d->nsym = 0;
    d->maxlen = 0;
    for (i = 0; i < n; i++) {
        if (lengths[i] > 0) {
            d->counts[lengths[i]]++;
            d->nsym++;
            if (lengths[i] > d->maxlen) {
                d->maxlen = lengths[i];
            }
        }
    }
    d->valid = (d->maxlen > 0) ? 1 : 0;
    d->nsym = 0;
    for (bits = 1; bits <= d->maxlen; bits++) {
        for (i = 0; i < n; i++) {
            if (lengths[i] == bits) {
                d->symbols[d->nsym++] = (uint16_t)i;
            }
        }
    }
}

int lf_huff_dec_symbol(lf_huff_dec *d, lf_breader *r)
{
    uint32_t code = 0;
    uint32_t first = 0;
    uint32_t index = 0;
    unsigned bits;

    if (!d->valid) {
        return -1;
    }
    for (bits = 1; bits <= d->maxlen; bits++) {
        uint32_t bit = lf_breader_get(r, 1);
        uint32_t count;

        if (!lf_breader_ok(r)) {
            return -1;
        }
        code |= bit;
        count = d->counts[bits];
        if (code - first < count) {
            return d->symbols[index + (code - first)];
        }
        index += count;
        first = (first + count) << 1;
        code <<= 1;
    }
    return -1;
}
