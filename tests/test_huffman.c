#include "unity.h"

#include "lf_bitio.h"
#include "lf_buf.h"
#include "lf_huffman.h"

#include <string.h>

void setUp(void)
{
}

void tearDown(void)
{
}

static uint32_t rng_state = 1u;

static uint32_t next_rng(void)
{
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}

static void check_code_valid(const uint8_t *lengths, const uint32_t *codes,
                             uint32_t n)
{
    uint32_t i;
    uint32_t j;
    uint64_t kraft = 0;

    for (i = 0; i < n; i++) {
        if (lengths[i] == 0) {
            continue;
        }
        TEST_ASSERT_TRUE(lengths[i] <= LF_MAX_CODE_LEN);
        kraft += (uint64_t)1 << (LF_MAX_CODE_LEN - lengths[i]);
        for (j = 0; j < n; j++) {
            if (j != i && lengths[j] != 0 && lengths[j] <= lengths[i]) {
                unsigned shift = lengths[i] - lengths[j];

                TEST_ASSERT_NOT_EQUAL_UINT(codes[i] >> shift, codes[j]);
            }
        }
    }
    TEST_ASSERT_TRUE(kraft <= ((uint64_t)1 << LF_MAX_CODE_LEN));
}

static void test_single_symbol(void)
{
    uint32_t freq[LF_NUM_LITLEN] = {0};
    uint8_t lengths[LF_NUM_LITLEN];
    uint32_t codes[LF_NUM_LITLEN];

    freq[100] = 7;
    memset(lengths, 0, sizeof(lengths));
    lf_huff_lengths(lengths, freq, LF_NUM_LITLEN);
    TEST_ASSERT_EQUAL_UINT(1, lengths[100]);
    TEST_ASSERT_EQUAL_UINT(0, lengths[99]);
    lf_huff_canonical(codes, lengths, LF_NUM_LITLEN);
    TEST_ASSERT_EQUAL_UINT(0, codes[100]);
}

static void test_no_symbols(void)
{
    uint32_t freq[LF_NUM_LITLEN] = {0};
    uint8_t lengths[LF_NUM_LITLEN];

    memset(lengths, 0xFF, sizeof(lengths));
    lf_huff_lengths(lengths, freq, LF_NUM_LITLEN);
    TEST_ASSERT_EQUAL_UINT(0, lengths[0]);
    TEST_ASSERT_EQUAL_UINT(0, lengths[LF_NUM_LITLEN - 1]);
}

static void test_balanced_tree(void)
{
    uint32_t freq[16];
    uint8_t lengths[16];
    uint32_t codes[16];
    unsigned i;

    for (i = 0; i < 16; i++) {
        freq[i] = 1;
    }
    lf_huff_lengths(lengths, freq, 16);
    for (i = 0; i < 16; i++) {
        TEST_ASSERT_EQUAL_UINT(4, lengths[i]);
    }
    lf_huff_canonical(codes, lengths, 16);
    check_code_valid(lengths, codes, 16);
}

static void test_length_cap_many_symbols(void)
{
    uint32_t freq[300];
    uint8_t lengths[300];
    uint32_t codes[300];
    unsigned i;

    for (i = 0; i < 300; i++) {
        freq[i] = 1;
    }
    lf_huff_lengths(lengths, freq, 300);
    for (i = 0; i < 300; i++) {
        TEST_ASSERT_TRUE(lengths[i] > 0);
        TEST_ASSERT_TRUE(lengths[i] <= LF_MAX_CODE_LEN);
    }
    lf_huff_canonical(codes, lengths, 300);
    check_code_valid(lengths, codes, 300);
}

static void test_random_distributions(void)
{
    uint32_t freq[200];
    uint8_t lengths[200];
    uint32_t codes[200];
    unsigned iter;
    unsigned i;

    for (iter = 0; iter < 20; iter++) {
        for (i = 0; i < 200; i++) {
            freq[i] = next_rng() % 100u;
        }
        lf_huff_lengths(lengths, freq, 200);
        for (i = 0; i < 200; i++) {
            TEST_ASSERT_TRUE(lengths[i] <= LF_MAX_CODE_LEN);
            if (freq[i] == 0) {
                TEST_ASSERT_EQUAL_UINT(0, lengths[i]);
            }
        }
        lf_huff_canonical(codes, lengths, 200);
        check_code_valid(lengths, codes, 200);
    }
}

static void test_decode_roundtrip(void)
{
    const uint32_t n = 100;
    uint32_t freq[100];
    uint8_t lengths[100];
    uint32_t codes[100];
    uint32_t chosen[1000];
    lf_huff_dec dec;
    lf_buf b;
    lf_bwriter w;
    lf_breader r;
    unsigned i;

    memset(freq, 0, sizeof(freq));
    for (i = 0; i < 1000; i++) {
        chosen[i] = next_rng() % n;
        freq[chosen[i]]++;
    }

    lf_huff_lengths(lengths, freq, n);
    lf_huff_canonical(codes, lengths, n);
    lf_huff_dec_init(&dec, lengths, n);
    TEST_ASSERT_TRUE(dec.valid);

    lf_buf_init(&b);
    lf_bwriter_init(&w, &b);
    for (i = 0; i < 1000; i++) {
        TEST_ASSERT_EQUAL_INT(LF_OK,
                              lf_bwriter_put_code(&w, codes[chosen[i]],
                                                  lengths[chosen[i]]));
    }
    lf_bwriter_align(&w);

    lf_breader_init(&r, b.data, b.len);
    for (i = 0; i < 1000; i++) {
        int sym = lf_huff_dec_symbol(&dec, &r);

        TEST_ASSERT_TRUE(sym >= 0);
        TEST_ASSERT_EQUAL_INT((int)chosen[i], sym);
    }
    lf_buf_free(&b);
}

static void test_decode_consumes_whole_stream(void)
{
    uint8_t lengths[4] = {1, 2, 3, 0};
    uint32_t codes[4];
    lf_huff_dec dec;
    lf_buf b;
    lf_bwriter w;
    lf_breader r;
    int count;

    lf_huff_canonical(codes, lengths, 4);
    lf_huff_dec_init(&dec, lengths, 4);

    lf_buf_init(&b);
    lf_bwriter_init(&w, &b);
    TEST_ASSERT_EQUAL_INT(LF_OK, lf_bwriter_put_code(&w, codes[2], 3));
    TEST_ASSERT_EQUAL_INT(LF_OK, lf_bwriter_align(&w));

    lf_breader_init(&r, b.data, b.len);
    for (count = 0; count < 100; count++) {
        if (lf_huff_dec_symbol(&dec, &r) < 0) {
            break;
        }
    }
    TEST_ASSERT_TRUE(count < 100);
    TEST_ASSERT_FALSE(lf_breader_ok(&r));
    lf_buf_free(&b);
}

static void test_decode_fails_when_input_runs_out(void)
{
    uint8_t lengths[4] = {1, 2, 3, 0};
    lf_huff_dec dec;
    uint8_t data[1] = {0x00};
    lf_breader r;
    int calls;

    lf_huff_dec_init(&dec, lengths, 4);
    lf_breader_init(&r, data, sizeof(data));
    for (calls = 0; calls < 64; calls++) {
        if (lf_huff_dec_symbol(&dec, &r) < 0) {
            break;
        }
    }
    TEST_ASSERT_TRUE(calls < 64);
    TEST_ASSERT_FALSE(lf_breader_ok(&r));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_single_symbol);
    RUN_TEST(test_no_symbols);
    RUN_TEST(test_balanced_tree);
    RUN_TEST(test_length_cap_many_symbols);
    RUN_TEST(test_random_distributions);
    RUN_TEST(test_decode_roundtrip);
    RUN_TEST(test_decode_consumes_whole_stream);
    RUN_TEST(test_decode_fails_when_input_runs_out);
    return UNITY_END();
}
