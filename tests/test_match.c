#include "unity.h"

#include "lf_match.h"

#include <string.h>

void setUp(void)
{
}

void tearDown(void)
{
}

static void test_init_rejects_bad_params(void)
{
    lf_match m;

    TEST_ASSERT_EQUAL_INT(LF_ERR_PARAM, lf_match_init(&m, 0, 16));
    TEST_ASSERT_EQUAL_INT(LF_ERR_PARAM, lf_match_init(&m, 8, 0));
    TEST_ASSERT_EQUAL_INT(LF_ERR_PARAM, lf_match_init(&m, 1, 16));
    TEST_ASSERT_EQUAL_INT(LF_ERR_PARAM, lf_match_init(NULL, 32, 16));
    TEST_ASSERT_EQUAL_INT(LF_OK, lf_match_init(&m, 32, 16));
    lf_match_free(&m);
}

static void test_finds_obvious_match(void)
{
    const uint8_t data[] = "abcabcabcabc";
    lf_match m;
    unsigned ml = 0;
    uint32_t md = 0;
    size_t i;

    TEST_ASSERT_EQUAL_INT(LF_OK, lf_match_init(&m, 32, 64));

    for (i = 0; i < 3; i++) {
        lf_match_insert(&m, data, i, sizeof(data) - 1);
    }
    TEST_ASSERT_EQUAL_INT(LF_OK,
                          lf_match_search(&m, data, 3, sizeof(data) - 1, &ml, &md));
    TEST_ASSERT_TRUE(ml >= 3);
    TEST_ASSERT_EQUAL_UINT(3, md);
    TEST_ASSERT_TRUE(ml + 3 <= sizeof(data) - 1);
    lf_match_free(&m);
}

static void test_no_match_at_start(void)
{
    const uint8_t data[] = "abcdefghij";
    lf_match m;
    unsigned ml = 99;
    uint32_t md = 99;

    TEST_ASSERT_EQUAL_INT(LF_OK, lf_match_init(&m, 32, 64));
    TEST_ASSERT_EQUAL_INT(LF_OK,
                          lf_match_search(&m, data, 0, sizeof(data) - 1, &ml, &md));
    TEST_ASSERT_EQUAL_UINT(0, ml);
    TEST_ASSERT_EQUAL_UINT(0, md);
    lf_match_free(&m);
}

static void test_window_limits_distance(void)
{
    static uint8_t data[600];
    lf_match m;
    unsigned ml = 0;
    uint32_t md = 0;

    memset(data, 0xAB, sizeof(data));
    TEST_ASSERT_EQUAL_INT(LF_OK, lf_match_init(&m, 64, 4096));

    {
        size_t i;

        for (i = 0; i < 128; i++) {
            lf_match_insert(&m, data, i, sizeof(data));
        }
    }
    TEST_ASSERT_EQUAL_INT(LF_OK,
                          lf_match_search(&m, data, 128, sizeof(data), &ml, &md));
    TEST_ASSERT_TRUE(ml >= 3);
    TEST_ASSERT_TRUE(md <= 64);
    lf_match_free(&m);
}

static void test_reset_clears_history(void)
{
    const uint8_t data[] = "aaaaaa";
    lf_match m;
    unsigned ml = 0;
    uint32_t md = 0;

    TEST_ASSERT_EQUAL_INT(LF_OK, lf_match_init(&m, 32, 64));
    lf_match_insert(&m, data, 0, sizeof(data) - 1);
    lf_match_reset(&m);
    TEST_ASSERT_EQUAL_INT(LF_OK,
                          lf_match_search(&m, data, 1, sizeof(data) - 1, &ml, &md));
    TEST_ASSERT_EQUAL_UINT(0, ml);
    lf_match_free(&m);
}

static void test_overlapping_match(void)
{
    const uint8_t data[] = "abcabcabcab";
    lf_match m;
    unsigned ml = 0;
    uint32_t md = 0;
    size_t i;

    TEST_ASSERT_EQUAL_INT(LF_OK, lf_match_init(&m, 32, 64));
    for (i = 0; i < 3; i++) {
        lf_match_insert(&m, data, i, sizeof(data) - 1);
    }
    TEST_ASSERT_EQUAL_INT(LF_OK,
                          lf_match_search(&m, data, 3, sizeof(data) - 1, &ml, &md));
    TEST_ASSERT_EQUAL_UINT(3, md);
    TEST_ASSERT_TRUE(ml >= 3);
    lf_match_free(&m);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_rejects_bad_params);
    RUN_TEST(test_finds_obvious_match);
    RUN_TEST(test_no_match_at_start);
    RUN_TEST(test_window_limits_distance);
    RUN_TEST(test_reset_clears_history);
    RUN_TEST(test_overlapping_match);
    return UNITY_END();
}
