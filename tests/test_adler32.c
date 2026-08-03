#include "unity.h"

#include "lf_adler32.h"

#include <string.h>

void setUp(void)
{
}

void tearDown(void)
{
}

static uint32_t rng_state = 7u;

static uint32_t next_rng(void)
{
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}

static uint32_t reference_adler32(const uint8_t *data, size_t len)
{
    uint32_t a = 1;
    uint32_t b = 0;
    size_t i;

    for (i = 0; i < len; i++) {
        a = (a + data[i]) % 65521u;
        b = (b + a) % 65521u;
    }
    return (b << 16) | a;
}

static void test_empty_is_one(void)
{
    TEST_ASSERT_EQUAL_UINT(1, lf_adler32(NULL, 0));
}

static void test_known_vector(void)
{
    TEST_ASSERT_EQUAL_UINT(0x11E60398u, lf_adler32("Wikipedia", 9));
}

static void test_single_bytes(void)
{
    uint8_t one[1] = {0x01};

    TEST_ASSERT_EQUAL_UINT(reference_adler32(one, 1), lf_adler32(one, 1));
    TEST_ASSERT_EQUAL_UINT(0x00020002u, lf_adler32(one, 1));
}

static void test_matches_reference(void)
{
    uint8_t data[4096];
    size_t i;
    unsigned iter;

    for (iter = 0; iter < 8; iter++) {
        for (i = 0; i < sizeof(data); i++) {
            data[i] = (uint8_t)next_rng();
        }
        TEST_ASSERT_EQUAL_UINT(reference_adler32(data, sizeof(data)),
                               lf_adler32(data, sizeof(data)));
    }
}

static void test_lengths_agree(void)
{
    uint8_t data[100];
    size_t i;

    for (i = 0; i < sizeof(data); i++) {
        data[i] = (uint8_t)(i * 7u);
        TEST_ASSERT_EQUAL_UINT(reference_adler32(data, i + 1),
                               lf_adler32(data, i + 1));
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_empty_is_one);
    RUN_TEST(test_known_vector);
    RUN_TEST(test_single_bytes);
    RUN_TEST(test_matches_reference);
    RUN_TEST(test_lengths_agree);
    return UNITY_END();
}
