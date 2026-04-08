#include <stdlib.h>
#include <string.h>
#include "room.h"

Room *room_create(int id, const char *name, int width, int height) {
  Room *new_room = calloc(1, sizeof(Room));
  if (new_room == NULL){
    return NULL;
  }

  if(name != NULL){
    new_room->name = strdup(name);
    if(new_room->name == NULL){//If strdup fails to allocate free to avoid memleak and return NULL
      free(new_room);
      return NULL;
    }
  }
  else{
    new_room->name = NULL;
  }

  new_room->id = id;

  //Height and width clamped to 1 per autograder requirements
  if(width < 1){
    new_room->width = 1;
  } else{ new_room->width = width; }
  if(height < 1){
    new_room->height = 1;
  }else{ new_room->height = height; }
  

//DEBUG: TRYING CALLOC INSTEAD OF THIS:
  //Other pointers set to NULL and I set counts to 0
  // new_room->floor_grid = NULL;
  // new_room->neighbors = NULL;//didnt have neighbor stuff from A1?
  // new_room->neighbor_count = 0;
  // new_room->portals = NULL;
  // new_room->portal_count = 0;
  // new_room->treasures = NULL;
  // new_room->treasure_count = 0;

  // new_room->pushables = NULL;
  // new_room->pushable_count = 0;

  // new_room->switches = NULL;
  // new_room->switch_count = 0;

  return new_room;
}

int room_get_width(const Room *r) {
  if(r!=NULL){
    return r->width;
  }
  return 0;
}

int room_get_height(const Room *r) {
  if(r!=NULL){
    return r->height;
  }
  return 0;
}

Status room_set_floor_grid(Room *r, bool *floor_grid) {
  if (r==NULL){
    return INVALID_ARGUMENT;
  }
  if(r->floor_grid != NULL){
    free(r->floor_grid);//Could not find a way to check # of entries for a pointer
  }
  r->floor_grid = floor_grid;
  return OK;
}

Status room_set_portals(Room *r, Portal *portals, int portal_count) {
  if(r==NULL){ return INVALID_ARGUMENT; }//Do all of these on one line it looks nice
  if(portal_count > 0){
    if (portals == NULL){
      return INVALID_ARGUMENT;
    }
  }
  if(r->portals != NULL && r->portal_count > 0){
    for(int i = 0; i < r->portal_count; i++){//Should work since it is a static entity
      if(r->portals[i].name != NULL){
        free(r->portals[i].name);//In types.h each portal has a name so we should free it before the pointer
      }
    }
    free (r->portals);
  }
  r->portals = portals;
  r->portal_count = portal_count;

  return OK;
}

Status room_set_treasures(Room *r, Treasure *treasures, int treasure_count) {
  if (r==NULL){ return INVALID_ARGUMENT; }

  if (treasure_count > 0 && treasures == NULL){
    return INVALID_ARGUMENT;
  }

  if(r->treasures != NULL && r->treasure_count > 0){
    for(int i=0; i<r->treasure_count; i++){
      free(r->treasures[i].name);
    }
    free(r->treasures);
  }

  for(int i = 0; i < treasure_count; i++){
    treasures[i].collected = false;
  }

  r->treasures = treasures;
  r->treasure_count = treasure_count;

  return OK;
}

Status room_place_treasure(Room *r, const Treasure *treasure) {
  if(r==NULL || treasure == NULL){ return INVALID_ARGUMENT; }

  //Expanded treasures array using realloc
  Treasure *temp_treasure = realloc(r->treasures, sizeof(Treasure)* (r->treasure_count + 1));
  if(temp_treasure == NULL){ return NO_MEMORY; }

  r->treasures = temp_treasure;

  r->treasures[r->treasure_count] = *treasure;
  if(treasure->name !=NULL){//BIG FIX TO ALL MY SEG FAULT
    //room must own its own copy of the string allocated
    r->treasures[r->treasure_count].name = strdup(treasure->name);
  }

  r->treasures[r->treasure_count].collected = false;//Trying to fix mem err 

  r->treasure_count = r->treasure_count + 1;

  return OK;
}

int room_get_treasure_at(const Room *r, int x, int y) {
  if(r == NULL){ return -1; }

  for(int i = 0; i < r->treasure_count; i++){
    if(!r->treasures[i].collected && r->treasures[i].x == x && r->treasures[i].y == y){
      return r->treasures[i].id;//Globally unique id according to types.h return it
    }
  }
  return -1;//This part is reached iff for loop finishes with no matches(no treasure exists)
}

int room_get_portal_destination(const Room *r, int x, int y) {
  if(r == NULL){ return -1; }
  //Same idea as get treasure above this
  for(int i = 0; i<r->portal_count; i++){
    if(r->portals[i].x == x && r->portals[i].y == y){
      return r->portals[i].target_room_id;//Should be destination room ID
    }
  }
  return -1;
}

bool room_is_walkable(const Room *r, int x, int y) {
  if(r==NULL){ return false; }

  //BOUNDS ARE 0,W for x and 0,H for y it must be >x and under W/H or it is out of bounds
  if(x<0 || x>= r->width || y<0 || y>= r->height){
    return false;
  }

  if(room_has_pushable_at(r,x,y,NULL)){//A2 part just check pushable
    return false;
  }

  //Took me a long time to find out Pass NULL to indicate implicit boundary walls. in room.h room_set_floor_grid
  if(r->floor_grid == NULL){//Boundary walls
    if(x>0 && x <r->width -1 && y>0 && y< r->height -1){
      return true;
    } 
    return false;
  }
  /*floor_grid will return true if walkable and false if it's a wall
  This indexing is like what we learned in Microcomp for assembly
  To go 1 row down you must increment by width to represent 2d grid with 1 number*/
  return r->floor_grid[y*r->width + x];
}

RoomTileType room_classify_tile(const Room *r, int x, int y, int *out_id) {
  if (r == NULL) { return ROOM_TILE_INVALID; }
  if(x<0 || x>= r->width || y<0 || y>= r->height){ return ROOM_TILE_INVALID; }


  int push_i = -1;
  if(room_has_pushable_at(r,x,y,&push_i)){
    if(out_id != NULL){*out_id = push_i;}
    return ROOM_TILE_PUSHABLE;
  }


  if(room_get_treasure_at(r, x, y) != -1){
    if(out_id != NULL){
      *out_id = room_get_treasure_at(r, x, y);
    }
    return ROOM_TILE_TREASURE;
  }

  if(room_get_portal_destination(r, x, y) != -1){//Performance vs Simplicity
    if(out_id != NULL){
      *out_id = room_get_portal_destination(r, x, y);
    }
    return ROOM_TILE_PORTAL;
  }

  if(room_is_walkable(r, x, y) == false){ 
    return ROOM_TILE_WALL;
  }  
  return ROOM_TILE_FLOOR; 

}

static void render_treasures(const Room *r, const Charset *charset, char *buffer) {//HELPER
    for (int i = 0; i < r->treasure_count; i++){
    if(!r->treasures[i].collected){
      int temp_x = r->treasures[i].x;
      int temp_y = r->treasures[i].y;
      if(temp_x>=0 && temp_x< r->width && temp_y>=0 && temp_y< r->height){
        buffer[temp_y * r->width + temp_x] = charset->treasure;  
      }
    }
  }
}
static void render_portals(const Room *r, const Charset *charset, char *buffer) {//HELPER
    for (int i = 0; i< r->portal_count; i++){
    int temp_x = r->portals[i].x;
    int temp_y = r->portals[i].y;
    if(temp_x>=0 && temp_x< r->width && temp_y>=0 && temp_y< r->height){
      buffer[temp_y * r->width + temp_x] = charset->portal; 
    }
  }
}

Status room_render(const Room *r, const Charset *charset, char *buffer, int buffer_width, int buffer_height) {
  if( r== NULL || charset == NULL || buffer == NULL){ return INVALID_ARGUMENT; }
  if( buffer_width != r->width || buffer_height != r->height){ return INVALID_ARGUMENT; }

  for(int row = 0; row < r->height; row++){
    for (int col = 0; col<r->width; col++){
      int indx = row*r->width + col;
      //Either this or other work
      if(room_is_walkable(r,col,row)){
        buffer[indx] = charset->floor;
      }
      else{
        if(room_has_pushable_at(r,col,row, NULL)){
          buffer[indx] = charset->pushable;
        }
        else{
          buffer[indx] = charset->wall;
        }
      }
    }
  }

  //HELPER FUNCTIONS TO LOWER C-COMPLEXITY
  render_treasures(r, charset, buffer);
  render_portals(r, charset, buffer);
  
  
  return OK;
}

Status room_get_start_position(const Room *r, int *x_out, int *y_out) {
  if(r == NULL || x_out == NULL || y_out == NULL) { return INVALID_ARGUMENT; }

  if(r->portal_count > 0 && r->portals != NULL){//Check for portals first higher prio
    *x_out = r->portals[0].x;
    *y_out = r->portals[0].y;
    return OK;
  } 
  for(int row = 1; row<r->height-1; row++){
    for(int col = 1; col < r->width; col++){
      if (room_is_walkable(r,col,row)){//First walkable room works
        *x_out = col;
        *y_out = row;
        return OK;
      }
    }
  }
  return ROOM_NOT_FOUND;
}

void room_destroy(Room *r) {
  if (r == NULL){ return; }

  free(r->name);
  free(r->floor_grid);


  //Pasted from room_set_portals function above
  if(r->portals != NULL && r->portal_count > 0){
    for(int i = 0; i < r->portal_count; i++){//Should work since it is a static entity
      free(r->portals[i].name);//In types.h each portal has a name so we should free it before the pointer
    }
    free (r->portals);
  }

  //Pasted from room_set_treasures function above
  if(r->treasures != NULL && r->treasure_count > 0){
    for(int i=0; i<r->treasure_count; i++){
      free(r->treasures[i].name);
    }
    free(r->treasures);
  }

  if(r->pushables != NULL){
    for(int i =0; i<r->pushable_count; i++){
      free(r->pushables[i].name);
    }
    free(r->pushables);
  }

  free(r->switches);

  free(r);
}


// **********************************************************                                
// ********************** ADDED FOR A2 **********************
// **********************************************************

int room_get_id(const Room *r){
  if(r==NULL){return -1;}
  return r->id;
}

Status room_pick_up_treasure(Room *r, int treasure_id, Treasure **treasure_out){
  if(r==NULL || treasure_out==NULL || treasure_id<0){
    return INVALID_ARGUMENT;
  }

  for(int i =0; i< r->treasure_count; i++){
    if(r->treasures[i].id == treasure_id){
      if(r->treasures[i].collected){ return INVALID_ARGUMENT;}
      r->treasures[i].collected = true;
      *treasure_out = &r->treasures[i];
      return OK;
    }
  }
  return ROOM_NOT_FOUND;
}

void destroy_treasure(Treasure *t){
  if (t!=NULL){
    free(t->name);
    free(t);
  }

}

bool room_has_pushable_at(const Room *r, int x, int y, int *pushable_idx_out){
  if(r==NULL){return false;}

  for(int i=0; i< r->pushable_count; i++){
    if(r->pushables[i].x == x && r->pushables[i].y == y){
      if(pushable_idx_out != NULL){*pushable_idx_out = i;}
      return true;
    }
  }
  return false;
}

Status room_try_push(Room *r, int pushable_idx, Direction dir){
  if(r==NULL || pushable_idx<0 || pushable_idx >= r->pushable_count){
    return INVALID_ARGUMENT;
  }
  if(dir != DIR_NORTH && dir != DIR_SOUTH && dir != DIR_EAST && dir != DIR_WEST){//Direction must be a valid one
    return INVALID_ARGUMENT;
  }
  Pushable *p = &r->pushables[pushable_idx];

  int x_new = p->x;
  int y_new = p->y;

  if(dir == DIR_NORTH){y_new--;}
  else if(dir == DIR_SOUTH){ y_new++;}
  else if(dir == DIR_EAST){ x_new++;}
  else if(dir == DIR_WEST){ x_new--;}

  if(x_new < 0 || x_new >= r->width || y_new < 0 || y_new >= r->height){//Bounds
    return ROOM_IMPASSABLE;
  }

  if(r->floor_grid != NULL){//Walls
    if(!r->floor_grid[y_new * r->width + x_new]){ return ROOM_IMPASSABLE; }
  } else {
    if(!(x_new > 0 && x_new < r->width - 1 && y_new > 0 && y_new < r->height - 1)){
      return ROOM_IMPASSABLE;
    }
  }


  for(int i = 0; i < r->pushable_count; i++){//Dont push onto another pushable 
    if(i == pushable_idx){ continue; }
    if(r->pushables[i].x == x_new && r->pushables[i].y == y_new){
      return ROOM_IMPASSABLE;
    }
  }


  //Dont push into a portal
  if(room_get_portal_destination(r,x_new,y_new)!= -1){
    return ROOM_IMPASSABLE;
  }

  p->x = x_new;
  p->y = y_new;
  return OK;
}


// **********************************************************                                
// ********************** ADDED FOR A3 **********************
// **********************************************************

// char * room_get_name(Room * room){
//     return room->name;
// }