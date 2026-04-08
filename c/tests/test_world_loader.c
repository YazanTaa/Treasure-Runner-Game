#include <check.h>
#include <stdlib.h>
#include "types.h"
#include "world_loader.h"
#include "graph.h"


START_TEST(test_loader_normal)
{
  Graph *g = NULL;
  Room * first_room_out = NULL;
  int num_rooms_out = 0;
  Charset cs;

  Status res = loader_load_world("../assets/starter.ini", &g, &first_room_out, &num_rooms_out, &cs);
  ck_assert_int_eq(res, OK);
  ck_assert_ptr_nonnull(g);
  ck_assert_ptr_nonnull(first_room_out);
  ck_assert_int_gt(num_rooms_out, 0);
  ck_assert_int_eq(cs.wall, '#');

  graph_destroy(g);
}
END_TEST


START_TEST(test_loader_path_dne){
  Graph *g = NULL;
  Room * first_room_out = NULL;
  int num_rooms_out = 0;
  Charset cs;

  Status res = loader_load_world("blahblah", &g, &first_room_out, &num_rooms_out, &cs);

  ck_assert_int_ne(res, OK);
  ck_assert_ptr_null(g);
}END_TEST

START_TEST(test_loader_args_null){
  Status res = loader_load_world(NULL, NULL, NULL, NULL, NULL);

  ck_assert_int_eq(res, INVALID_ARGUMENT);
}END_TEST


Suite *loader_suite(void)
{
  Suite *s = suite_create("Load World");
  TCase *tc = tcase_create("Core");

  tcase_add_test(tc, test_loader_normal);
  tcase_add_test(tc, test_loader_path_dne);
  tcase_add_test(tc, test_loader_args_null);

  suite_add_tcase(s, tc);

  return s;
}