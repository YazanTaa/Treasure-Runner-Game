#include <check.h>
#include <stdlib.h>
#include "types.h"
#include "room.h"
#include <stdbool.h>

//Same as player but with room
Room * r;
void setup_r(void){
  r = room_create(42, "TEST", 10, 10);
}

void teardown_r(void){
  room_destroy(r);
  r = NULL;
}


START_TEST(test_room_init)
{
  ck_assert_ptr_nonnull(r);
  ck_assert_int_eq(room_get_width(r), 10);
  ck_assert_int_eq(room_get_height(r), 10);

  
}
END_TEST


START_TEST(test_room_walkable){
  //By default centre should be walkable while edges and corners are not
  ck_assert(room_is_walkable(r,5,5));
  ck_assert(!room_is_walkable(r,0,0));
  ck_assert(!room_is_walkable(r, 9, 5));//right edge i think

  //Testing with my own grid
  bool *grid = malloc(sizeof(bool) * 100);//100 is just w*h

  for(int i = 0; i<100; i++){//Make all walkable
    grid[i] = true;
  }

  grid[55] = false;//Should be middle (5,5)

  room_set_floor_grid(r, grid);
  ck_assert_msg(!room_is_walkable(r, 5, 5), "My grid center should be a wall");
  ck_assert_msg(room_is_walkable(r,1,2), "My grid 1,2 untouched floor");
}
END_TEST

START_TEST(test_room_treasures){
  ck_assert_ptr_nonnull(r);

  Treasure t;
  t.id = 42;
  t.x = 5;
  t.y = 5;
  t.name = strdup("test treasure");
  t.collected = false;

  ck_assert_int_eq(t.id, 42);

  Status res = room_place_treasure(r, &t);
  ck_assert_int_eq(res, OK);

  free(t.name);

  ck_assert_int_eq(r->treasure_count, 1);

  int out = -1;

  RoomTileType tile = room_classify_tile(r, 5, 5, &out);

  ck_assert_int_eq(tile, ROOM_TILE_TREASURE);
  ck_assert_int_eq(out, 42);//Should be same as id I set
}END_TEST 

START_TEST(test_room_portals){
  Portal *p = malloc(sizeof(Portal));
  p[0].id = 42;
  p[0].x = 6;
  p[0].y = 7;
  p[0].target_room_id = 43;
  p[0].name = strdup("test portal");

  Status res = room_set_portals(r, p, 1);
  ck_assert_int_eq(res, OK);

  int out = -1;
  RoomTileType tile = room_classify_tile(r, 6, 7, &out);

  ck_assert_int_eq(tile, ROOM_TILE_PORTAL);
  ck_assert_int_eq(out, 43);
}

START_TEST(test_room_start){
  int x = -1;
  int y = -1;

  Status res = room_get_start_position(r, &x, &y);
  ck_assert_int_eq(res, OK);
  ck_assert(room_is_walkable(r, x, y));

  Portal *p = malloc(sizeof(Portal));
  p[0].id = 42;
  p[0].x = 6;
  p[0].y = 7;

  room_set_portals(r, p, 1);

  res = room_get_start_position(r, &x, &y);
  ck_assert_int_eq(res, OK);
  ck_assert(room_is_walkable(r, x, y));
}END_TEST

START_TEST(test_room_render){
  Charset test_cs;
  //Copied symbols from the A1 description
  test_cs.wall = '#';
  test_cs.floor = '.';
  test_cs.player = '@';
  test_cs.treasure = '$';
  test_cs.portal = 'X';

  Treasure t;
  t.id = 42;
  t.x = 5;
  t.y = 5;
  t.name = strdup("test treasure");
  t.collected = false;
  Status res = room_place_treasure(r, &t);
  ck_assert_int_eq(res, OK);

  Portal *p = malloc(sizeof(Portal));
  p[0].id = 42;
  p[0].x = 6;
  p[0].y = 7;
  p[0].target_room_id = 43;
  p[0].name = strdup("test portal");

  res = room_set_portals(r, p, 1);
  ck_assert_int_eq(res, OK);

  char * b = malloc(100);//10 by 10 room
  res = room_render(r, &test_cs, b, 10, 10);

  ck_assert_int_eq(b[0], '#');
  ck_assert_int_eq(b[19], '#');//right wall i think
  ck_assert_int_eq(b[76], 'X');//7,6 where portal should be
  ck_assert_int_eq(b[55], '$');
  ck_assert_int_eq(b[13], '.');
  free(b);
}
END_TEST

//A2 TESTS BELOW

START_TEST(test_room_get_id){
  ck_assert_int_eq(room_get_id(r), 42);
  ck_assert_int_eq(room_get_id(NULL), -1);
}END_TEST

START_TEST(test_room_pick_up_treasure){//Did normal and negative cases in one to save space
  Treasure t = {10, NULL, 0,0,0,2,2, false};
  t.name = strdup("Treasure");


  room_place_treasure(r, &t);
  free(t.name);

  Treasure * out = NULL;
  ck_assert_int_eq(room_pick_up_treasure(r, 10, &out), OK);
  ck_assert_ptr_nonnull(out);
  ck_assert(out->collected);

  //Now that its collected we can test how it handles collected treasure(neg test)

  ck_assert_int_eq(room_pick_up_treasure(r, 10, &out), INVALID_ARGUMENT);

  //Test treasure not found as well (neg test)
  ck_assert_int_eq(room_pick_up_treasure(r, 42, &out), ROOM_NOT_FOUND);

  //Test treasure NULL as well (neg test)
  ck_assert_int_eq(room_pick_up_treasure(NULL, 1, NULL), INVALID_ARGUMENT);
  ck_assert_int_eq(room_pick_up_treasure(r, -1, &out), INVALID_ARGUMENT);
  
}END_TEST

START_TEST(test_room_collected_treasure_walkable){
  Treasure t = {10, NULL, 0,0,0,2,2, false};
  t.name = strdup("Treasure");
  room_place_treasure(r, &t);
  free(t.name);

  ck_assert(room_is_walkable(r, 2, 2));//If it isnt looted you cant walk on it yet

  Treasure *out = NULL;
  room_pick_up_treasure(r, 10, &out);//loot it

  ck_assert(room_is_walkable(r, 2, 2));//Now it should walk since its looted
}END_TEST

START_TEST(test_room_has_pushable_at){
  Pushable * ps = malloc(sizeof(Pushable));
  ps[0].id = 1;
  ps[0].name = strdup("pushable");
  ps[0].initial_x = 2;
  ps[0].initial_y = 2;
  ps[0].x = 2;
  ps[0].y = 2;
  r->pushables = ps;
  r->pushable_count = 1;

  int i = -1;
  ck_assert(room_has_pushable_at(r,2,2,&i));
  ck_assert_int_eq(i, 0);

  //Negative cases
  ck_assert(!room_has_pushable_at(r,10,10, NULL));//DNE
  ck_assert(!room_has_pushable_at(NULL, 2, 2, NULL));//NULLLL
}END_TEST

START_TEST(test_room_pushable_blocks_walkable){
  Pushable * ps = malloc(sizeof(Pushable));
  ps[0].id = 1;
  ps[0].name = strdup("pushable");
  ps[0].initial_x = 2;
  ps[0].initial_y = 2;
  ps[0].x = 2;
  ps[0].y = 2;
  r->pushables = ps;
  r->pushable_count = 1;

  ck_assert(!room_is_walkable(r, 2, 2));

  int id_out = -1;
  ck_assert_int_eq(room_classify_tile(r,2,2,&id_out), ROOM_TILE_PUSHABLE);
  ck_assert_int_eq(id_out, 0);
}END_TEST

START_TEST(test_room_try_push){
  Pushable * ps = malloc(sizeof(Pushable));
  ps[0].id = 1;
  ps[0].name = strdup("pushable");
  ps[0].initial_x = 2;
  ps[0].initial_y = 2;
  ps[0].x = 2;
  ps[0].y = 2;
  r->pushables = ps;
  r->pushable_count = 1;

  ck_assert_int_eq(room_try_push(r, 0, DIR_SOUTH), OK);
  ck_assert_int_eq(r->pushables[0].x, 2);
  ck_assert_int_eq(r->pushables[0].y, 3);//increase by 1 since south

  //NULL TESTS BELOW(negative case)

  ck_assert_int_eq(room_try_push(NULL, 0, DIR_SOUTH), INVALID_ARGUMENT);
  ck_assert_int_eq(room_try_push(r, -1, DIR_SOUTH), INVALID_ARGUMENT);
}END_TEST

START_TEST(test_room_try_push_fail){//Fails if blocked by a wall or another pushable
  Pushable * ps = malloc(sizeof(Pushable));
  ps[0].id = 1;
  ps[0].name = strdup("pushable");
  ps[0].initial_x = 1;
  ps[0].initial_y = 1;
  ps[0].x = 1;
  ps[0].y = 1;
  r->pushables = ps;
  r->pushable_count = 1;

  ck_assert_int_eq(room_try_push(r, 0, DIR_NORTH), ROOM_IMPASSABLE);
  ck_assert_int_eq(r->pushables[0].x, 1);  
  ck_assert_int_eq(r->pushables[0].y, 1);
}

START_TEST(test_room_render_a2){
  Charset test_cs;

  test_cs.wall = '#';
  test_cs.floor = '.';
  test_cs.player = '@';
  test_cs.treasure = '$';
  test_cs.portal = 'X';

  //A2 blocks added nowwwww
  test_cs.pushable = 'O';
  test_cs.switch_off = 's';
  test_cs.switch_on = 'S';

  Pushable * ps = malloc(sizeof(Pushable));
  ps[0].id = 1;
  ps[0].name = strdup("pushable");
  ps[0].initial_x = 2;
  ps[0].initial_y = 2;
  ps[0].x = 2;
  ps[0].y = 2;
  r->pushables = ps;
  r->pushable_count = 1;

  char*b = malloc(100);//10x10 room again
  ck_assert_int_eq(room_render(r, &test_cs, b, 10, 10), OK);
  ck_assert_int_eq(b[22], 'O');
  free(b);
}END_TEST

Suite *room_suite(void)
{
  Suite *s = suite_create("Room");
  TCase *tc = tcase_create("tc");

  tcase_add_checked_fixture(tc, setup_r, teardown_r);

  tcase_add_test(tc, test_room_init);
  tcase_add_test(tc, test_room_walkable);
  tcase_add_test(tc, test_room_treasures);
  tcase_add_test(tc, test_room_portals);
  tcase_add_test(tc, test_room_start);
  tcase_add_test(tc, test_room_render);

  //A2 TESTS BELOW
  tcase_add_test(tc, test_room_get_id);
  tcase_add_test(tc, test_room_pick_up_treasure);
  tcase_add_test(tc, test_room_collected_treasure_walkable);
  tcase_add_test(tc, test_room_has_pushable_at);
  tcase_add_test(tc, test_room_pushable_blocks_walkable);
  tcase_add_test(tc, test_room_try_push);
  tcase_add_test(tc, test_room_try_push_fail);
  tcase_add_test(tc, test_room_render_a2);

  suite_add_tcase(s, tc);

  return s;
}