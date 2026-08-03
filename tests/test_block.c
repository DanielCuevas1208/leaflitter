#include "unity.h"

#include "lf_bitio.h"
#include "lf_block.h"
#include "lf_buf.h"
#include "lf_common.h"
#include "lf_match.h"

#include <string.h>

void setUp(void)
{
}

void tearDown(void)
{
}

static uint32_t rng_state = 42u;

static uint32_t next_rng(void)
{
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}

static void block_roundtrip(const uint8_t *data, size_t len, uint32_t window)
{
    lf_match m;
    lf_options opt;
    lf_buf packed;
    lf_buf out;
    lf_breader r;
    int final = 0;
    lf_result res;

    lf_options_default(&opt);
    opt.window_size = window;
    TEST_ASSERT_EQUAL_INT(LF_OK, lf_match_init(&m, opt.window_size, opt.max_chain));
    lf_buf_init(&packed);
    lf_buf_init(&out);

    res = lf_block_encode(&opt, &m, data, 0, len, len, 1, &packed);
    TEST_ASSERT_EQUAL_INT(LF_OK, res);
    lf_match_free(&m);
    TEST_ASSERT_TRUE(packed.len > 0);

    lf_breader_init(&r, packed.data, packed.len);
    res = lf_block_decode(&r, &out, len, window, &final);
    TEST_ASSERT_EQUAL_INT(LF_OK, res);
    TEST_ASSERT_TRUE(final);
    TEST_ASSERT_EQUAL_UINT(len, out.len);
    TEST_ASSERT_EQUAL_INT(0, memcmp(out.data, data, len));
    TEST_ASSERT_TRUE(lf_breader_finished(&r));

    lf_buf_free(&packed);
    lf_buf_free(&out);
}

static void test_single_literal(void)
{
    uint8_t data[1] = {0x2A};

    block_roundtrip(data, 1, 256);
}

static void test_short_repeat(void)
{
    const char *data = "abcabcabcabcabc";

    block_roundtrip((const uint8_t *)data, strlen(data), 512);
}

static void test_repeated_bytes(void)
{
    uint8_t data[1000];
    size_t i;

    for (i = 0; i < sizeof(data); i++) {
        data[i] = (uint8_t)(i % 7u);
    }
    block_roundtrip(data, sizeof(data), 4096);
}

static void test_random_data_is_stored(void)
{
    uint8_t data[512];
    lf_match m;
    lf_options opt;
    lf_buf packed;
    lf_buf out;
    lf_breader r;
    int final = 0;
    size_t i;

    for (i = 0; i < sizeof(data); i++) {
        data[i] = (uint8_t)next_rng();
    }

    lf_options_default(&opt);
    TEST_ASSERT_EQUAL_INT(LF_OK, lf_match_init(&m, opt.window_size, opt.max_chain));
    lf_buf_init(&packed);
    lf_buf_init(&out);
    TEST_ASSERT_EQUAL_INT(LF_OK,
                          lf_block_encode(&opt, &m, data, 0, sizeof(data),
                                          sizeof(data), 1, &packed));
    lf_match_free(&m);

    TEST_ASSERT_EQUAL_UINT(0, packed.data[0] & 0x03u);

    lf_breader_init(&r, packed.data, packed.len);
    TEST_ASSERT_EQUAL_INT(LF_OK,
                          lf_block_decode(&r, &out, sizeof(data), opt.window_size,
                                          &final));
    TEST_ASSERT_TRUE(final);
    TEST_ASSERT_EQUAL_UINT(sizeof(data), out.len);
    TEST_ASSERT_EQUAL_INT(0, memcmp(out.data, data, sizeof(data)));

    lf_buf_free(&packed);
    lf_buf_free(&out);
}

static void test_long_window_match(void)
{
    uint8_t data[4096];
    size_t i;

    for (i = 0; i < 2048; i++) {
        data[i] = (uint8_t)(i % 251u);
    }
    memcpy(data + 2048, data, 2048);
    block_roundtrip(data, sizeof(data), 4096);
}

static void test_truncated_block_rejected(void)
{
    uint8_t data[64];
    lf_match m;
    lf_options opt;
    lf_buf packed;
    lf_buf out;
    lf_breader r;
    int final = 0;
    size_t i;

    for (i = 0; i < sizeof(data); i++) {
        data[i] = (uint8_t)(i * 3u);
    }

    lf_options_default(&opt);
    TEST_ASSERT_EQUAL_INT(LF_OK, lf_match_init(&m, opt.window_size, opt.max_chain));
    lf_buf_init(&packed);
    lf_buf_init(&out);
    TEST_ASSERT_EQUAL_INT(LF_OK,
                          lf_block_encode(&opt, &m, data, 0, sizeof(data),
                                          sizeof(data), 1, &packed));
    lf_match_free(&m);
    TEST_ASSERT_TRUE(packed.len > 4);

    lf_breader_init(&r, packed.data, packed.len - 1);
    TEST_ASSERT_NOT_EQUAL_INT(LF_OK,
                              lf_block_decode(&r, &out, sizeof(data),
                                              opt.window_size, &final));
    lf_buf_free(&packed);
    lf_buf_free(&out);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_single_literal);
    RUN_TEST(test_short_repeat);
    RUN_TEST(test_repeated_bytes);
    RUN_TEST(test_random_data_is_stored);
    RUN_TEST(test_long_window_match);
    RUN_TEST(test_truncated_block_rejected);
    return UNITY_END();
}
