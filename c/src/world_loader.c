#include <stdlib.h>
#include <string.h>
#include "world_loader.h"
#include "datagen.h"
#include "graph.h"
#include "room.h"

int room_compare_by_id(const void *a, const void *b) {//Helper function 
  return ((Room*)a)->id - ((Room*)b)->id;
}
static Status load_portals(Room *temp_room, DG_Room temp_dg) {
        if(temp_dg.portal_count <= 0){return OK;}
      Portal *temp_portals = malloc(sizeof(Portal) * temp_dg.portal_count);
      if(temp_portals == NULL){ 
        room_destroy(temp_room);
        stop_datagen();
        return NO_MEMORY;
      }

      for(int i = 0; i<temp_dg.portal_count; i++){
        temp_portals[i].name = strdup("Portal");
        temp_portals[i].id = temp_dg.portals[i].id;
        temp_portals[i].x = temp_dg.portals[i].x;
        temp_portals[i].y = temp_dg.portals[i].y;
        temp_portals[i].target_room_id = temp_dg.portals[i].neighbor_id;

        temp_portals[i].gated = (temp_dg.portals[i].required_switch_id != -1);
        temp_portals[i].required_switch_id = temp_dg.portals[i].required_switch_id;
      }
      room_set_portals(temp_room, temp_portals, temp_dg.portal_count);
      return OK;
    
 }
static Status load_treasures(Room *temp_room, DG_Room temp_dg){
    if(temp_dg.treasure_count <= 0){return OK;}
      Treasure * temp_tr = malloc(sizeof(Treasure) * temp_dg.treasure_count);
      if(temp_tr == NULL){ 
        room_destroy(temp_room);
        stop_datagen();
        return NO_MEMORY;
      }

      for(int i =0; i < temp_dg.treasure_count; i++){
        temp_tr[i].name = strdup(temp_dg.treasures[i].name);
        temp_tr[i].id = temp_dg.treasures[i].global_id;
        temp_tr[i].x = temp_dg.treasures[i].x;
        temp_tr[i].y = temp_dg.treasures[i].y;
        temp_tr[i].collected = false;

        temp_tr[i].starting_room_id = temp_dg.id;//LET US SEE NOW
        temp_tr[i].initial_x = temp_dg.treasures[i].x;
        temp_tr[i].initial_y = temp_dg.treasures[i].y;
      }
      room_set_treasures(temp_room, temp_tr, temp_dg.treasure_count);
      return OK;
    
}
static Status load_pushables(Room *temp_room, DG_Room temp_dg){
    if(temp_dg.pushable_count <= 0){return OK;}
    Pushable * temp_push = malloc(sizeof(Pushable)* temp_dg.pushable_count);
      if(temp_push == NULL){ 
        room_destroy(temp_room);
        stop_datagen();
        return NO_MEMORY;
      }
      for(int i=0; i<temp_dg.pushable_count; i++){
        temp_push[i].id = temp_dg.pushables[i].id;
        temp_push[i].name = strdup(temp_dg.pushables[i].name);
        temp_push[i].initial_x = temp_dg.pushables[i].x;
        temp_push[i].initial_y = temp_dg.pushables[i].y;
        temp_push[i].x = temp_dg.pushables[i].x;
        temp_push[i].y = temp_dg.pushables[i].y;
      }
      temp_room->pushables = temp_push;
      temp_room->pushable_count = temp_dg.pushable_count;
      return OK;
    }
static Status load_switches(Room *temp_room, DG_Room temp_dg){
if(temp_dg.switch_count <= 0){return OK;}
    Switch * temp_sw = malloc(sizeof(Switch) * temp_dg.switch_count);
    if(temp_sw == NULL){ 
    room_destroy(temp_room);
    stop_datagen();
    return NO_MEMORY;
    }
    for(int i=0; i<temp_dg.switch_count; i++){
    temp_sw[i].id = temp_dg.switches[i].id;
    temp_sw[i].x = temp_dg.switches[i].x;
    temp_sw[i].y = temp_dg.switches[i].y;
    temp_sw[i].portal_id = temp_dg.switches[i].portal_id;
    }
    temp_room->switches = temp_sw;
    temp_room->switch_count = temp_dg.switch_count;
    return OK;
}
static void build_connections(Graph *graph_out, const void * const *all_rooms, int count){
    for (int i = 0; i < count; i++) {
        Room * curr = (Room *)all_rooms[i];

        for(int j =0; j < curr->portal_count; j++){

            if(curr->portals[j].target_room_id != -1){
                Room temp_r;
                temp_r.id = curr->portals[j].target_room_id;
                Room *dest = (Room*)graph_get_payload(graph_out, &temp_r);

                if (dest != NULL){
                    graph_connect(graph_out, curr, dest);
                }
            }
        }
    }
}


Status loader_load_world(const char *config_file, Graph **graph_out, Room **first_room_out, int *num_rooms_out, Charset *charset_out) {
  if(config_file==NULL || graph_out == NULL || first_room_out == NULL || num_rooms_out == NULL || charset_out == NULL){
    return INVALID_ARGUMENT;
  }
  int r_val = start_datagen(config_file);//Passing configc file

  if(r_val != DG_OK){
    if (r_val == DG_ERR_CONFIG){ return WL_ERR_CONFIG; }
    return WL_ERR_DATAGEN;
  }
  

  *num_rooms_out = 0;
  *first_room_out = NULL;

  if(graph_create(room_compare_by_id, (GraphDestroyFn)room_destroy, graph_out) != GRAPH_STATUS_OK){
    stop_datagen();
    return NO_MEMORY;
  }

  while(has_more_rooms()){//Iterating thru datagen output 
    DG_Room temp_dg = get_next_room(); //datagen output temp
    //Base room below
    Room *temp_room = room_create(temp_dg.id, "Room", temp_dg.width, temp_dg.height);

    if(temp_dg.floor_grid != NULL){//Make sure we arent at the end of the datagen output
      bool * temp_grid = malloc(sizeof(bool)*temp_dg.width*temp_dg.height);
      memcpy(temp_grid, temp_dg.floor_grid, sizeof(bool)*temp_dg.width*temp_dg.height);
      room_set_floor_grid(temp_room, temp_grid);
    }//Copy of floorgrid first using memcpy for deep

    //HELPERS TO REDUCE CCOMPLEXITY
    load_portals(temp_room, temp_dg);
    load_treasures(temp_room, temp_dg);
    //FOR A2 added pushables and switches:
    load_pushables(temp_room, temp_dg);
    load_switches(temp_room, temp_dg);

    graph_insert(*graph_out, temp_room);
    if(*num_rooms_out == 0){
      *first_room_out = temp_room;
    }
    (*num_rooms_out)++;//Increment to get accurate room count
  }

  int count = 0;
  const void * const *all_rooms = NULL;
  GraphStatus res = graph_get_all_payloads(*graph_out, &all_rooms, &count);
  if (res == GRAPH_STATUS_OK && all_rooms != NULL) {
    build_connections(*graph_out, all_rooms, count);//HELPER 
  }
  const DG_Charset *datagen_cs = dg_get_charset();

  if(datagen_cs != NULL){
    charset_out->wall = datagen_cs->wall;
    charset_out->floor = datagen_cs->floor;
    charset_out->portal = datagen_cs->portal;
    charset_out->treasure = datagen_cs->treasure;
    charset_out->player = datagen_cs->player;
    charset_out->pushable = datagen_cs->pushable;
  }
  stop_datagen();
  return OK;
}

