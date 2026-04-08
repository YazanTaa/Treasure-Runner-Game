#include <check.h>
#include <stdlib.h>
#include "types.h"

START_TEST(test_stub)
{
  ck_assert_int_eq(1, 1);
}
END_TEST

Suite *placeholder_suite(void)
{
  Suite *s = suite_create("Placeholder");
  TCase *tc = tcase_create("Core");

  tcase_add_test(tc, test_stub);
  suite_add_tcase(s, tc);

  return s;
}