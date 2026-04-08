#include <check.h>
#include <stdlib.h>
#include "types.h"
#include "game_engine.h"
#include "player.h"

START_TEST(test_engine_init)
{
  GameEngine * eng = NULL;
  Status res = game_engine_create("../assets/starter.ini", &eng);

  ck_assert_int_eq(res, OK);
  ck_assert_ptr_nonnull(eng);
  const Player *p = game_engine_get_player(eng);
  ck_assert_int_ne(player_get_room(p), -1);

  res = game_engine_move_player(eng, DIR_NORTH);

  ck_assert(res == OK || res == ROOM_IMPASSABLE);

  ck_assert_int_eq(game_engine_reset(eng), OK);

  int count = 0;
  ck_assert_int_eq(game_engine_get_room_count(eng, &count), OK);
  ck_assert_int_gt(count, 0);

  int * ids = NULL;
  count =0;
  res = game_engine_get_room_ids(eng, &ids, &count);
  ck_assert_int_eq(res, OK);

  char *out = NULL;

  ck_assert_int_eq(game_engine_render_room(eng, ids[0], &out), OK);
  ck_assert_ptr_nonnull(out);

  game_engine_free_string(out);//use the function instead of just free A2
  game_engine_free_string(NULL);//Should be fine 
  free(ids);

  game_engine_destroy(eng);
}
END_TEST

START_TEST(test_engine_args_null){//edges again
  ck_assert_int_eq(game_engine_create(NULL, NULL), INVALID_ARGUMENT);
  ck_assert_int_eq(game_engine_move_player(NULL, DIR_NORTH), INVALID_ARGUMENT);
  ck_assert_int_eq(game_engine_get_room_dimensions(NULL, NULL, NULL), INVALID_ARGUMENT);
  ck_assert_int_eq(game_engine_get_room_count(NULL, NULL), INVALID_ARGUMENT);
  ck_assert_int_eq(game_engine_render_current_room(NULL, NULL), INVALID_ARGUMENT);
  ck_assert_int_eq(game_engine_reset(NULL), INVALID_ARGUMENT);

  ck_assert_ptr_null(game_engine_get_player(NULL));
}END_TEST

//A2 TESTS BELOW:

START_TEST(test_engine_reset){
  GameEngine * eng = NULL;
  game_engine_create("../assets/starter.ini", &eng);

  const Player *p = game_engine_get_player(eng);
  int init_room = player_get_room(p);
  int init_x = 0;
  int init_y = 0;
  player_get_position(p, &init_x, &init_y);

  game_engine_move_player(eng, DIR_SOUTH);
  game_engine_move_player(eng, DIR_EAST);

  ck_assert_int_eq(game_engine_reset(eng), OK);

  ck_assert_int_eq(player_get_room(p), init_room);
  int temp_x = 0;
  int temp_y = 0;
  player_get_position(p, &temp_x, &temp_y);
  ck_assert_int_eq(init_x, temp_x);
  ck_assert_int_eq(init_y, temp_y);

  ck_assert_int_eq(player_get_collected_count(p), 0);//Should be cleared evern tho nothing rlly changed

  game_engine_destroy(eng);
}END_TEST

START_TEST(test_engine_render_player){
  GameEngine * eng = NULL;
  game_engine_create("../assets/starter.ini", &eng);

  char* out = NULL;
  ck_assert_int_eq(game_engine_render_current_room(eng, &out), OK);
  ck_assert_ptr_nonnull(out);

  bool is_player;
  for(int i=0; out[i]!= '\0'; i++){
    if(out[i]== eng->charset.player){
      is_player = true;
      break;
    }
  }
  ck_assert(is_player);

  game_engine_free_string(out);
  game_engine_destroy(eng);
}END_TEST

Suite *engine_suite(void)
{
  Suite *s = suite_create("Engine");
  TCase *tc = tcase_create("Core");

  tcase_add_test(tc, test_engine_init);//small a2 change
  tcase_add_test(tc, test_engine_args_null);

  //A2 TESTS:

  tcase_add_test(tc, test_engine_reset);
  tcase_add_test(tc, test_engine_render_player);
  
  suite_add_tcase(s, tc);

  return s;
}