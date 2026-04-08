#include <check.h>
#include <stdlib.h>
#include <player.h>
#include "types.h"

Player *p = NULL;

void setup(void){
  player_create(6, 7, 7, &p);
}

void teardown(void){
  player_destroy(p);
  p = NULL;
}


START_TEST(test_player_init){
  
  ck_assert_ptr_nonnull(p);
  ck_assert_int_eq(player_get_room(p), 6);

  int x = 0;
  int y = 0;

  player_get_position(p, &x, &y);
  ck_assert_int_eq(x, 7);
  ck_assert_int_eq(y,7);
}END_TEST

START_TEST(test_player_change_pos){
  Status res = player_set_position(p, 2, 6);
  ck_assert_int_eq(res, OK);

  int x= 0;
  int y=0;
  player_get_position(p, &x, &y);
  ck_assert_int_eq(x, 2);
  ck_assert_int_eq(y,6);
}END_TEST

START_TEST(test_player_change_room){
  Status res = player_move_to_room(p, 167);
  ck_assert_int_eq(res, OK);
  ck_assert_int_eq(player_get_room(p), 167);
}END_TEST

START_TEST(test_player_null_entry){
  ck_assert_int_eq(player_create(1, 1, 1, NULL), INVALID_ARGUMENT);
  ck_assert_int_eq(player_set_position(NULL, 1, 1), INVALID_ARGUMENT);
  ck_assert_int_eq(player_get_room(NULL), -1);

  int x= 0;
  int y=0;
  ck_assert_int_eq(player_get_position((NULL), &x, &y), INVALID_ARGUMENT);

  ck_assert_int_eq(player_get_position(p, NULL, &y), INVALID_ARGUMENT);
  ck_assert_int_eq(player_get_position(p, &x, NULL), INVALID_ARGUMENT);
}END_TEST

START_TEST(test_player_invalid_pos){
  ck_assert_int_eq(player_set_position(p, -1, 7), OK);//apparently this is allowed i had it as invalid before
}END_TEST

//A2 TESTS BELOW:

START_TEST(test_player_treasure_init){
  ck_assert_int_eq(player_get_collected_count(p), 0);
  ck_assert(!player_has_collected_treasure(p, 1));
  int count = 0;
  player_get_collected_treasures(p, &count);
  ck_assert_int_eq(count, 0);
}END_TEST

START_TEST(test_player_treasure_collect){
  Treasure t1,t2;
  t1.id = 10;
  t1.name = NULL;
  t1.collected = false;
  t1.x=1;
  t1.y=1;

  ck_assert_int_eq(player_try_collect(p, &t1), OK);
  ck_assert_int_eq(player_get_collected_count(p), 1);

  t2.id = 12;
  t2.name = NULL;
  t2.collected = false;
  t2.x=1;
  t2.y=1;

  ck_assert_int_eq(player_try_collect(p, &t2), OK);
  ck_assert_int_eq(player_get_collected_count(p), 2);

  ck_assert(player_has_collected_treasure(p, 10));
  ck_assert(player_has_collected_treasure(p, 12));
  ck_assert(t1.collected);
  ck_assert(t2.collected);

  ck_assert(!player_has_collected_treasure(p, 99));//Was not collected
}END_TEST

START_TEST(test_player_treasure_already_collected){//Negative test
  Treasure t;
  t.id = 10;
  t.name = NULL;
  t.collected = true;
  t.x=1;
  t.y=1;

  ck_assert_int_eq(player_try_collect(p, &t), INVALID_ARGUMENT);
  ck_assert_int_eq(player_get_collected_count(p), 0);//Should not change from initial 0 since collect should fail
}END_TEST

START_TEST(test_player_treasure_collect_null){
  Treasure t;
  t.id = 10;
  t.name = NULL;
  t.collected = true;
  t.x=1;
  t.y=1;

  ck_assert_int_eq(player_try_collect(p,NULL), NULL_POINTER);
  ck_assert_int_eq(player_try_collect(NULL,&t), NULL_POINTER);
  ck_assert_int_eq(player_try_collect(NULL,NULL), NULL_POINTER);
}END_TEST

START_TEST(test_player_get_collected_treasures){
  Treasure t1,t2;
  t1.id = 10;
  t1.name = NULL;
  t1.collected = false;
  t1.x=1;
  t1.y=1;

  t2.id = 12;
  t2.name = NULL;
  t2.collected = false;
  t2.x=1;
  t2.y=1;

  player_try_collect(p, &t1);
  player_try_collect(p, &t2);

  int count = 0;
  const Treasure * const *arr = player_get_collected_treasures(p, &count);
  ck_assert_int_eq(count,2);
  ck_assert_ptr_nonnull(arr);
  ck_assert_int_eq(arr[0]->id, 10);
  ck_assert_int_eq(arr[1]->id, 12);
}END_TEST

START_TEST(test_player_reset){
  Treasure t;
  t.id = 10;
  t.name = NULL;
  t.collected = false;
  t.x=1;
  t.y=1;

  player_try_collect(p, &t);
  ck_assert_int_eq(player_get_collected_count(p), 1);

  ck_assert_int_eq(player_reset_to_start(p, 1, 2, 3), OK);//ids and x and y are js random numbers it doesnt matter
  ck_assert_int_eq(player_get_collected_count(p), 0);

  ck_assert_int_eq(player_get_room(p), 1);//Adding i dont think i tested these in A1
  int x_out, y_out;
  ck_assert_int_eq(player_get_position(p, &x_out, &y_out), OK);
  ck_assert_int_eq(x_out, 2);
  ck_assert_int_eq(y_out, 3);
}END_TEST


START_TEST(test_player_collected_null){
  ck_assert(!player_has_collected_treasure(NULL, 1));
  ck_assert(!player_has_collected_treasure(p, -1));

  ck_assert_int_eq(player_get_collected_count(NULL), 0);
  ck_assert_ptr_null(player_get_collected_treasures(NULL, NULL));
}END_TEST

Suite *player_suite(void)
{
  Suite *s = suite_create("Player");
  TCase *tc = tcase_create("tc");

  tcase_add_checked_fixture(tc, setup, teardown);

  tcase_add_test(tc, test_player_init);
  tcase_add_test(tc, test_player_change_pos);
  tcase_add_test(tc, test_player_change_room);
  tcase_add_test(tc, test_player_null_entry);
  tcase_add_test(tc, test_player_invalid_pos);

  //A2 TESTS:
  tcase_add_test(tc, test_player_treasure_init);
  tcase_add_test(tc, test_player_treasure_collect);
  tcase_add_test(tc, test_player_treasure_already_collected);
  tcase_add_test(tc, test_player_treasure_collect_null);
  tcase_add_test(tc, test_player_get_collected_treasures);
  tcase_add_test(tc, test_player_reset);//For treasure addition and it was missing frm A1
  tcase_add_test(tc, test_player_collected_null);//More null checks seperate from A1's


  suite_add_tcase(s, tc);

  return s;
}