#include "unity.h"

#include "lf_buf.h"
#include "lf_common.h"
#include "lf_stream.h"

#include <stdlib.h>
#include <string.h>

void setUp(void)
{
}

void tearDown(void)
{
}

static uint32_t rng_state = 12345u;

static void rng_seed(uint32_t seed)
{
    rng_state = seed;
}

static uint32_t next_rng(void)
{
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}

static void roundtrip(const uint8_t *data, size_t len)
{
    lf_buf packed;
    lf_buf plain;
    lf_result r;

    lf_buf_init(&packed);
    lf_buf_init(&plain);
    r = lf_compress(NULL, data, len, &packed);
    TEST_ASSERT_EQUAL_INT(LF_OK, r);
    TEST_ASSERT_TRUE(packed.len <= lf_compress_bound(len));

    r = lf_decompress(packed.data, packed.len, &plain);
    TEST_ASSERT_EQUAL_INT(LF_OK, r);
    TEST_ASSERT_EQUAL_UINT(len, plain.len);
    if (len > 0) {
        TEST_ASSERT_EQUAL_INT(0, memcmp(plain.data, data, len));
    }
    lf_buf_free(&packed);
    lf_buf_free(&plain);
}

static void test_roundtrip_empty(void)
{
    roundtrip(NULL, 0);
}

static void test_roundtrip_single_byte(void)
{
    uint8_t d[1] = {0x7F};

    roundtrip(d, 1);
}

static void test_roundtrip_two_bytes(void)
{
    uint8_t d[2] = {0x00, 0xFF};

    roundtrip(d, 2);
}

static void test_roundtrip_text(void)
{
    const char *text =
        "leaflitter compresses and decompresses byte streams. It uses a match "
        "finder and Huffman coding, similar to DEFLATE.";

    roundtrip((const uint8_t *)text, strlen(text));
}

static void test_roundtrip_repeated_text(void)
{
    char text[8192];
    size_t i;

    for (i = 0; i < sizeof(text); i++) {
        text[i] = (char)("leaflitter byte stream\n"[i % 24]);
    }
    roundtrip((const uint8_t *)text, sizeof(text));
}

static void test_roundtrip_random(void)
{
    uint8_t data[20000];
    size_t i;

    rng_seed(99);
    for (i = 0; i < sizeof(data); i++) {
        data[i] = (uint8_t)next_rng();
    }
    roundtrip(data, sizeof(data));
}

static void test_roundtrip_large_multi_block(void)
{
    uint8_t *data;
    size_t n = 300000;
    size_t i;

    data = (uint8_t *)malloc(n);
    TEST_ASSERT_NOT_NULL(data);
    rng_seed(7);
    for (i = 0; i < n; i++) {
        data[i] = (uint8_t)("compression pattern %d"[i % 22]);
    }
    for (i = 100000; i < 120000; i++) {
        data[i] = (uint8_t)next_rng();
    }
    roundtrip(data, n);
    free(data);
}

static void test_all_byte_values(void)
{
    uint8_t data[256];
    size_t i;

    for (i = 0; i < 256; i++) {
        data[i] = (uint8_t)i;
    }
    roundtrip(data, sizeof(data));
}

static void test_compress_is_deterministic(void)
{
    uint8_t data[5000];
    lf_buf a;
    lf_buf b;
    size_t i;

    rng_seed(5);
    for (i = 0; i < sizeof(data); i++) {
        data[i] = (uint8_t)next_rng();
    }
    for (i = 0; i < sizeof(data); i += 37) {
        data[i] = (uint8_t)(i % 256);
    }

    lf_buf_init(&a);
    lf_buf_init(&b);
    TEST_ASSERT_EQUAL_INT(LF_OK, lf_compress(NULL, data, sizeof(data), &a));
    TEST_ASSERT_EQUAL_INT(LF_OK, lf_compress(NULL, data, sizeof(data), &b));
    TEST_ASSERT_EQUAL_UINT(a.len, b.len);
    TEST_ASSERT_EQUAL_INT(0, memcmp(a.data, b.data, a.len));
    lf_buf_free(&a);
    lf_buf_free(&b);
}

static void test_compress_bound_monotonic(void)
{
    TEST_ASSERT_TRUE(lf_compress_bound(0) > 0);
    TEST_ASSERT_TRUE(lf_compress_bound(100) <= lf_compress_bound(200));
    TEST_ASSERT_TRUE(lf_compress_bound(10000) > 10000);
}

static void test_rejects_bad_magic(void)
{
    uint8_t data[40];
    lf_buf out;

    memset(data, 0, sizeof(data));
    data[0] = 'X';
    data[1] = 'F';
    data[2] = 0x01;
    data[3] = 1;
    lf_buf_init(&out);
    TEST_ASSERT_EQUAL_INT(LF_ERR_BAD_MAGIC, lf_decompress(data, sizeof(data), &out));
    lf_buf_free(&out);
}

static void test_rejects_bad_version(void)
{
    uint8_t data[40];
    lf_buf out;

    memset(data, 0, sizeof(data));
    data[0] = LF_MAGIC_0;
    data[1] = LF_MAGIC_1;
    data[2] = LF_MAGIC_2;
    data[3] = 99;
    lf_buf_init(&out);
    TEST_ASSERT_EQUAL_INT(LF_ERR_UNSUPPORTED_VERSION,
                          lf_decompress(data, sizeof(data), &out));
    lf_buf_free(&out);
}

static void test_rejects_truncated_header(void)
{
    uint8_t data[10] = {0};
    lf_buf out;

    lf_buf_init(&out);
    TEST_ASSERT_EQUAL_INT(LF_ERR_TRUNCATED, lf_decompress(data, sizeof(data), &out));
    lf_buf_free(&out);
}

static void test_rejects_truncated_body(void)
{
    uint8_t data[1000];
    lf_buf packed;
    lf_buf out;
    size_t i;

    rng_seed(3);
    for (i = 0; i < sizeof(data); i++) {
        data[i] = (uint8_t)next_rng();
    }
    lf_buf_init(&packed);
    lf_buf_init(&out);
    TEST_ASSERT_EQUAL_INT(LF_OK, lf_compress(NULL, data, sizeof(data), &packed));
    TEST_ASSERT_TRUE(packed.len > 24);
    TEST_ASSERT_NOT_EQUAL_INT(LF_OK,
                              lf_decompress(packed.data, packed.len - 1, &out));
    lf_buf_free(&packed);
    lf_buf_free(&out);
}

static void test_rejects_corrupted_adler(void)
{
    uint8_t data[500];
    lf_buf packed;
    lf_buf out;
    size_t i;

    rng_seed(11);
    for (i = 0; i < sizeof(data); i++) {
        data[i] = (uint8_t)next_rng();
    }
    lf_buf_init(&packed);
    lf_buf_init(&out);
    TEST_ASSERT_EQUAL_INT(LF_OK, lf_compress(NULL, data, sizeof(data), &packed));
    packed.data[packed.len / 2] ^= 0xFF;
    TEST_ASSERT_NOT_EQUAL_INT(LF_OK, lf_decompress(packed.data, packed.len, &out));
    lf_buf_free(&packed);
    lf_buf_free(&out);
}

static void test_rejects_trailing_garbage(void)
{
    uint8_t data[64];
    lf_buf packed;
    lf_buf out;
    size_t i;

    rng_seed(21);
    for (i = 0; i < sizeof(data); i++) {
        data[i] = (uint8_t)(i % 4u);
    }
    lf_buf_init(&packed);
    lf_buf_init(&out);
    TEST_ASSERT_EQUAL_INT(LF_OK, lf_compress(NULL, data, sizeof(data), &packed));
    TEST_ASSERT_EQUAL_INT(LF_OK, lf_buf_append(&packed, "tail", 4));
    TEST_ASSERT_EQUAL_INT(LF_ERR_INVALID_DATA,
                          lf_decompress(packed.data, packed.len, &out));
    lf_buf_free(&packed);
    lf_buf_free(&out);
}

static void test_rejects_null_params(void)
{
    lf_buf out;

    lf_buf_init(&out);
    TEST_ASSERT_EQUAL_INT(LF_ERR_PARAM, lf_compress(NULL, NULL, 5, &out));
    TEST_ASSERT_EQUAL_INT(LF_ERR_PARAM, lf_compress(NULL, NULL, 0, NULL));
    TEST_ASSERT_EQUAL_INT(LF_ERR_PARAM, lf_decompress(NULL, 24, &out));
    TEST_ASSERT_EQUAL_INT(LF_ERR_PARAM, lf_decompress("x", 1, NULL));
    lf_buf_free(&out);
}

static void test_small_window_and_block_options(void)
{
    uint8_t data[20000];
    lf_options opt;
    lf_buf packed;
    lf_buf plain;
    size_t i;

    rng_seed(8);
    for (i = 0; i < sizeof(data); i++) {
        data[i] = (uint8_t)("repeating option test"[i % 21]);
    }

    lf_options_default(&opt);
    opt.window_size = 1024;
    opt.block_size = 4096;
    opt.max_chain = 64;

    lf_buf_init(&packed);
    lf_buf_init(&plain);
    TEST_ASSERT_EQUAL_INT(LF_OK, lf_compress(&opt, data, sizeof(data), &packed));
    TEST_ASSERT_EQUAL_INT(LF_OK, lf_decompress(packed.data, packed.len, &plain));
    TEST_ASSERT_EQUAL_UINT(sizeof(data), plain.len);
    TEST_ASSERT_EQUAL_INT(0, memcmp(plain.data, data, sizeof(data)));
    lf_buf_free(&packed);
    lf_buf_free(&plain);
}

static void test_roundtrip_huge_single_block(void)
{
    uint8_t data[200000];
    size_t i;

    for (i = 0; i < sizeof(data); i++) {
        data[i] = (uint8_t)("abcdefghij"[i % 10]);
    }
    roundtrip(data, sizeof(data));
}

static const uint8_t golden_empty[] = {
    0x4c, 0x46, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const uint8_t golden_abc[] = {
    0x4c, 0x46, 0x01, 0x01, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x27, 0x01, 0x4d, 0x02, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x04, 0x03, 0x00, 0x00, 0x00, 0x61, 0x62, 0x63,
};

static const uint8_t golden_text[] = {
    0x4c, 0x46, 0x01, 0x01, 0x56, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x50, 0x21, 0x2c, 0xb2, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x05, 0x0d, 0x01, 0x08, 0x00, 0x4a, 0xe1, 0x05, 0xa2, 0x9f, 0x4d, 0x34,
    0xac, 0x69, 0x55, 0xa5, 0x56, 0x74, 0xad, 0x34, 0xac, 0xe3, 0x01, 0xd1,
    0xf7, 0xa2, 0x03, 0x42, 0x5a, 0x0d, 0x71, 0x19, 0x6c, 0xc8, 0xbe, 0xf5,
    0xc9, 0x29, 0x14, 0x2d, 0xd6, 0x55, 0x07, 0xd9, 0xbc,
};

static void check_golden(const uint8_t *input, size_t in_len,
                         const uint8_t *expect, size_t expect_len)
{
    lf_buf packed;

    lf_buf_init(&packed);
    TEST_ASSERT_EQUAL_INT(LF_OK, lf_compress(NULL, input, in_len, &packed));
    TEST_ASSERT_EQUAL_UINT(expect_len, packed.len);
    TEST_ASSERT_EQUAL_INT(0, memcmp(packed.data, expect, expect_len));
    lf_buf_free(&packed);
}

static void test_golden_vectors(void)
{
    const char *text =
        "leaflitter compresses bytes. leaflitter compresses bytes. "
        "leaflitter compresses bytes.";

    check_golden(NULL, 0, golden_empty, sizeof(golden_empty));
    check_golden((const uint8_t *)"abc", 3, golden_abc, sizeof(golden_abc));
    check_golden((const uint8_t *)text, strlen(text), golden_text,
                 sizeof(golden_text));
}

static void test_block_boundary_sizes(void)
{
    static const size_t sizes[] = {65535, 65536, 65537, 131071, 131072, 131073};
    uint8_t *data;
    size_t idx;
    size_t i;

    data = (uint8_t *)malloc(sizes[sizeof(sizes) / sizeof(sizes[0]) - 1]);
    TEST_ASSERT_NOT_NULL(data);
    for (i = 0; i < sizes[sizeof(sizes) / sizeof(sizes[0]) - 1]; i++) {
        data[i] = (uint8_t)("boundary test"[i % 13]);
    }
    for (idx = 0; idx < sizeof(sizes) / sizeof(sizes[0]); idx++) {
        roundtrip(data, sizes[idx]);
    }
    free(data);
}

static void test_highly_repetitive_input(void)
{
    uint8_t data[100000];
    size_t i;

    for (i = 0; i < sizeof(data); i++) {
        data[i] = 0x55;
    }
    roundtrip(data, sizeof(data));
}

static void test_small_alphabet_random(void)
{
    uint8_t data[40000];
    size_t i;

    rng_seed(17);
    for (i = 0; i < sizeof(data); i++) {
        data[i] = (uint8_t)(next_rng() % 4u);
    }
    roundtrip(data, sizeof(data));
}

static void test_decode_never_crashes_on_garbage(void)
{
    uint8_t buf[64];
    lf_buf out;
    unsigned iter;
    unsigned len;

    lf_buf_init(&out);
    for (iter = 0; iter < 2000; iter++) {
        len = next_rng() % sizeof(buf);
        rng_seed(iter + 1);
        while (1) {
            size_t i;

            for (i = 0; i < len; i++) {
                buf[i] = (uint8_t)next_rng();
            }
            break;
        }
        (void)lf_decompress(buf, len, &out);
        lf_buf_clear(&out);
    }
    lf_buf_free(&out);
}

static void test_corrupt_every_byte(void)
{
    uint8_t data[3000];
    lf_buf packed;
    lf_buf out;
    size_t i;
    size_t j;

    rng_seed(29);
    for (i = 0; i < sizeof(data); i++) {
        data[i] = (uint8_t)next_rng();
    }
    lf_buf_init(&packed);
    lf_buf_init(&out);
    TEST_ASSERT_EQUAL_INT(LF_OK, lf_compress(NULL, data, sizeof(data), &packed));
    for (j = 24; j < packed.len; j++) {
        packed.data[j] ^= 0x40;
        (void)lf_decompress(packed.data, packed.len, &out);
        lf_buf_clear(&out);
        packed.data[j] ^= 0x40;
    }
    lf_buf_free(&packed);
    lf_buf_free(&out);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_roundtrip_empty);
    RUN_TEST(test_roundtrip_single_byte);
    RUN_TEST(test_roundtrip_two_bytes);
    RUN_TEST(test_roundtrip_text);
    RUN_TEST(test_roundtrip_repeated_text);
    RUN_TEST(test_roundtrip_random);
    RUN_TEST(test_roundtrip_large_multi_block);
    RUN_TEST(test_all_byte_values);
    RUN_TEST(test_compress_is_deterministic);
    RUN_TEST(test_compress_bound_monotonic);
    RUN_TEST(test_rejects_bad_magic);
    RUN_TEST(test_rejects_bad_version);
    RUN_TEST(test_rejects_truncated_header);
    RUN_TEST(test_rejects_truncated_body);
    RUN_TEST(test_rejects_corrupted_adler);
    RUN_TEST(test_rejects_trailing_garbage);
    RUN_TEST(test_rejects_null_params);
    RUN_TEST(test_small_window_and_block_options);
    RUN_TEST(test_roundtrip_huge_single_block);
    RUN_TEST(test_golden_vectors);
    RUN_TEST(test_block_boundary_sizes);
    RUN_TEST(test_highly_repetitive_input);
    RUN_TEST(test_small_alphabet_random);
    RUN_TEST(test_decode_never_crashes_on_garbage);
    RUN_TEST(test_corrupt_every_byte);
    return UNITY_END();
}
