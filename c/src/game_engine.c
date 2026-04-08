#include <stdlib.h>
#include <string.h>
#include "game_engine.h"
#include "world_loader.h"
#include "room.h"
#include "graph.h"
#include "player.h"

Status game_engine_create(const char *config_file_path, GameEngine **engine_out) {
  if(config_file_path == NULL || engine_out ==NULL){ return INVALID_ARGUMENT; }
  GameEngine *e = malloc(sizeof(GameEngine));
  if(e==NULL){ return NO_MEMORY; }

  e->graph = NULL;
  e->player = NULL;
  e->room_count = 0;
  Room *first_room = NULL;

  Status res = loader_load_world(config_file_path, &e->graph, &first_room, &e->room_count, &e->charset);

  if(res != OK){
    free(e);
    return WL_ERR_DATAGEN;
  }

  if (first_room == NULL) {
    game_engine_destroy(e);
    return WL_ERR_DATAGEN;
  }

  e->initial_room_id = first_room->id;
  res = room_get_start_position(first_room, &e->initial_player_x, &e->initial_player_y);
  if (res !=OK){
    game_engine_destroy(e);
    return res;
  }

  res = player_create(e->initial_room_id, e->initial_player_x, e->initial_player_y, &e->player);
  if (res !=OK){
    game_engine_destroy(e);
    return res;
  }

  *engine_out = e;
  return OK;
}



void game_engine_destroy(GameEngine *eng) {
  if (eng == NULL){ return; }
  if(eng->player != NULL){ player_destroy(eng->player); }
  if(eng->graph){ graph_destroy(eng->graph); }
  //Room destroy was intialized with the graph in world loader functionj so it auto frees every room

  free(eng);
}

const Player *game_engine_get_player(const GameEngine *eng) {
  if(eng == NULL){ return NULL; }
  return eng->player;
}

Status game_engine_move_player(GameEngine *eng, Direction dir) {
  if (eng == NULL || eng->player == NULL || eng->graph == NULL) {return INVALID_ARGUMENT;}
  if(dir != DIR_NORTH && dir != DIR_SOUTH && dir != DIR_EAST && dir != DIR_WEST){//same as room dir must be valid
    return INVALID_ARGUMENT;
  }

  int x = 0;
  int y = 0;
  player_get_position(eng->player, &x, &y);

  Room temp;
  temp.id = player_get_room(eng->player);//Gives roomid
  Room * curr = (Room*)graph_get_payload(eng->graph, &temp);//Gives us actual current room

  if(curr == NULL){ return GE_NO_SUCH_ROOM; }

  int new_x = x;
  int new_y = y;

  if(dir == DIR_NORTH){ new_y--; }
  else if(dir == DIR_SOUTH){ new_y++; }
  else if(dir == DIR_EAST){ new_x++; }
  else if(dir == DIR_WEST){ new_x--; }

  int id_out = -1;
  RoomTileType type = room_classify_tile(curr, new_x, new_y, &id_out);

  if(type == ROOM_TILE_WALL || type == ROOM_TILE_INVALID){
    return ROOM_IMPASSABLE;
  }
  if(type == ROOM_TILE_FLOOR){//I think you walk on treasure to collect so it should work
    return player_set_position(eng->player, new_x, new_y);//Will return invalid if an entry is wrong
  }

  //A2 change: add pushable handling also treasure handling missing
  if(type == ROOM_TILE_PUSHABLE){
    if(room_try_push(curr, id_out, dir) != OK){
      return ROOM_IMPASSABLE;//Push failed so we cant move
    }
    return player_set_position(eng->player, new_x, new_y);//Push succeeded move to tile
  }

  if(type==ROOM_TILE_TREASURE){
    Treasure * t= NULL;
    Status res_tr = room_pick_up_treasure(curr, id_out, &t);
    if(res_tr == OK && t!=NULL){
      player_try_collect(eng->player, t);
    }
    // return player_set_position(eng->player,new_x, new_y); this ruined integration
    return OK;
  }

  //More complicated portals
  //I think just teleport to out id of portal
  if(type == ROOM_TILE_PORTAL){
    Room temp2;
    temp2.id = id_out;
    Room * new_room = (Room*)graph_get_payload(eng->graph, &temp2);//id_out should be key to new room if its portal it says

    if(new_room == NULL){ return GE_NO_SUCH_ROOM; }
    int port_x = 0; 
    int port_y = 0;
    if(room_get_start_position(new_room, &port_x, &port_y)!=OK){ return INTERNAL_ERROR; }

    player_move_to_room(eng->player, id_out);//Move player to new room
    player_set_position(eng->player, port_x, port_y);//Now move to start position in new room
    return OK;
  }

  return INTERNAL_ERROR;//This is only reached if nothing happens at all
}
  

Status game_engine_get_room_count(const GameEngine *eng, int *count_out) {
  if(eng==NULL){ return INVALID_ARGUMENT; }
  if(count_out==NULL){ return NULL_POINTER; }

  *count_out = eng->room_count;//maybe double check with testing room_count wrong

  return OK;
}

Status game_engine_get_room_dimensions(const GameEngine *eng, int *width_out, int *height_out) {
  if(eng==NULL){ return INVALID_ARGUMENT; }
  if(width_out ==NULL || height_out ==NULL){ return NULL_POINTER; }
  if(eng->player == NULL|| eng->graph == NULL){ return INTERNAL_ERROR; }

  int id = player_get_room(eng->player);
  if(id == -1){ return INTERNAL_ERROR; }

  Room temp;
  temp.id = id;
  Room * r = (Room*)graph_get_payload(eng->graph, &temp);
  if(r == NULL){ return GE_NO_SUCH_ROOM; }

  *width_out = room_get_width(r);
  *height_out = room_get_height(r);
  return OK;
}

//Changing everything this was bad
Status game_engine_reset(GameEngine *eng) {
  if(eng == NULL) { return INVALID_ARGUMENT; }

  int count = 0;
  const void * const * all_rooms = NULL;
  if(graph_get_all_payloads(eng->graph, &all_rooms, &count) != GRAPH_STATUS_OK){
    return INTERNAL_ERROR;
  }
  for(int i = 0 ; i < count; i++){
    Room *r = (Room * )all_rooms[i];
    if(r==NULL){ return INTERNAL_ERROR; }

    for(int j = 0 ; j< r->pushable_count; j++){//Pushables back to the original position
      r->pushables[j].x = r->pushables[j].initial_x;
      r->pushables[j].y = r->pushables[j].initial_y;
    }

    for(int j = 0; j<r->treasure_count; j++){
      r->treasures[j].collected = false;//Uncollect treasure
      r->treasures[j].x = r->treasures[j].initial_x;//I dont think they would move but just to be safe
      r->treasures[j].y =  r->treasures[j].initial_y;
    }
  }

  if(player_reset_to_start(eng->player, eng->initial_room_id, eng->initial_player_x, eng->initial_player_y)!= OK){
    return INTERNAL_ERROR;
  }
  return OK;
}

Status game_engine_render_current_room(const GameEngine *eng, char **str_out) {
  if(eng == NULL|| str_out == NULL){ return INVALID_ARGUMENT; }
  Room temp;
  temp.id = player_get_room(eng->player);
  Room * curr = (Room*)graph_get_payload(eng->graph, &temp);
  if(curr == NULL){ return INTERNAL_ERROR; }

  int w = room_get_width(curr);
  int h = room_get_height(curr);

  char * str = malloc((size_t)w*(size_t)h);

  if(room_render(curr, &eng->charset, str, w, h) != OK){
    free(str);
    return INTERNAL_ERROR;
  }

  int curr_x = 0; 
  int curr_y = 0;
  player_get_position(eng->player, &curr_x, &curr_y);
  str[curr_y * w + curr_x] = eng->charset.player;//This is where player is

  char * str_final = malloc((w+1)*h + 1);//+1 added to w for \n for earch row and added to whole thing for \0 cause its a string

  char *str_ptr = str_final;//pointer is at the beginnning of our string
  for(int row=0; row<h; row++){
    memcpy(str_ptr, &str[(size_t)row * (size_t)w], (size_t)w);//Copies row by row
    str_ptr += w;
    *str_ptr++ = '\n';//Adds newline at the end with the extra char we allocated
  }
  *str_ptr = '\0'; //replace the last newline with the null terminator so string is over

  free(str);
  *str_out = str_final;
  return OK;
}

Status game_engine_render_room(const GameEngine *eng, int room_id, char **str_out) {
  if(eng==NULL){ return INVALID_ARGUMENT; }
  if(str_out == NULL){ return NULL_POINTER; }
  Room temp;
  temp.id = room_id;
  Room * curr = (Room*)graph_get_payload(eng->graph, &temp);
  if(curr == NULL){ return GE_NO_SUCH_ROOM; }

  int w = room_get_width(curr);
  int h = room_get_height(curr);

  char * str = malloc((size_t)w * (size_t)h);


  if(room_render(curr, &eng->charset, str, w, h) != OK){
    free(str);
    return INTERNAL_ERROR;
  }

  char * str_final = malloc((w+1) * h + 1);

  char *str_ptr = str_final;//pointer is at the beginnning of our string
  for(int row=0; row<h; row++){
    memcpy(str_ptr, &str[(size_t)row * (size_t)w], (size_t)w);//Copies row by row
    str_ptr += w;
    *str_ptr++ = '\n';//Adds newline at the end with the extra char we allocated
  }
  *str_ptr = '\0'; //replace the last newline with the null terminator so string is over

  free(str);
  *str_out = str_final;
  return OK;
}

Status game_engine_get_room_ids(const GameEngine *eng, int **ids_out, int *count_out) {
  if(eng==NULL){ return INVALID_ARGUMENT; }
  if( ids_out == NULL || count_out == NULL){ return NULL_POINTER; }
  if(eng->graph == NULL){ return INTERNAL_ERROR; }

  int count = 0;
  const void * const * rooms = NULL;
  GraphStatus res = graph_get_all_payloads(eng->graph, &rooms, &count);
  if(res != GRAPH_STATUS_OK){ return INTERNAL_ERROR; }

  int * room_ids = malloc(sizeof(int) * count);
  if(room_ids == NULL){ return NO_MEMORY; }

  for(int i = 0 ; i< count; i++){
    const Room *r = (const Room *)rooms[i];
    if( r != NULL){
      room_ids[i] = r->id;
    }else{//Null payload show up in graph then graph invalid
      free(room_ids);
      return INTERNAL_ERROR;
    }
  }

  *ids_out = room_ids;
  *count_out = count;
  return OK;
}

// **********************************************************                                
// ********************** ADDED FOR A2 **********************
// **********************************************************



void game_engine_free_string(void *ptr){
  free(ptr);
}

// **********************************************************                                
// ********************** ADDED FOR A3 **********************
// **********************************************************
Status game_engine_get_treasure_count(const GameEngine *eng, int *count_out){
    if(eng==NULL || count_out ==NULL){ return INVALID_ARGUMENT; }

    int count=0;
    const void * const * rooms = NULL;
    GraphStatus res = graph_get_all_payloads(eng->graph, &rooms, &count);
  if(res != GRAPH_STATUS_OK){ return INTERNAL_ERROR; }

  int treasure_count = 0;
  for(int i=0; i<count; i++){
    const Room *r = (const Room *)rooms[i];
    if( r != NULL){
      treasure_count += r->treasure_count;
    }else{//Null payload show up in graph then graph invalid
      return INTERNAL_ERROR;
    }
  }
  *count_out = treasure_count;
  return OK;
}

Status game_engine_get_adj_mtx(const GameEngine *eng, int **mtx_out, int *count_out){
    if (eng == NULL || mtx_out == NULL || count_out == NULL){ return INVALID_ARGUMENT;}
    int count=0;
    const void*const* rooms = NULL;
    if (graph_get_all_payloads(eng->graph, &rooms, &count) != GRAPH_STATUS_OK){
        return INTERNAL_ERROR;
    }

    int *mtx = calloc((size_t)count*(size_t)count, sizeof(int));//matrix allocation
    if (mtx == NULL) { 
        return NO_MEMORY;
    }

    for(int i=0; i<count; i++){
        for(int j=0; j<count; j++){
            if(i!=j && graph_has_edge(eng->graph, rooms[i], rooms[j]) ){ mtx[i*count + j] = 1;}
        }
    }
    *mtx_out = mtx;
    *count_out = count;
    return OK;
}