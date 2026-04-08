#include <stdlib.h>
#include "player.h"

Status player_create(int initial_room_id, int initial_x, int initial_y, Player **player_out){
  if (player_out == NULL){
    return INVALID_ARGUMENT;
  }

  Player *temp = malloc(sizeof(Player));

  if (temp == NULL){
    return NO_MEMORY;
  }

  temp->room_id = initial_room_id;
  temp->x = initial_x;
  temp->y = initial_y;

  temp->collected_treasures = NULL;
  temp->collected_count = 0;

  *player_out = temp;

  return OK;
}

void player_destroy(Player *p){
  if (p!=NULL){
    free(p->collected_treasures);//room owns treasures themselves so just free array of poitners
    free(p);
  }
}

int player_get_room(const Player *p){
  if(p!=NULL){
    return p->room_id;
  }
  return -1;
}

Status player_get_position(const Player *p, int *x_out, int *y_out){
  if(p!=NULL && x_out!=NULL && y_out != NULL){
    *x_out = p->x;
    *y_out = p->y;
    return OK;
  }
  return INVALID_ARGUMENT;

}

Status player_set_position(Player *p, int x, int y){
  if(p!=NULL){
    p->x = x;
    p->y = y;
    return OK;
  }
  return INVALID_ARGUMENT;

}

Status player_move_to_room(Player *p, int new_room_id){
  if(p!=NULL){
    p->room_id = new_room_id;
    return OK;
  }
  return INVALID_ARGUMENT;

}

Status player_reset_to_start(Player *p, int starting_room_id, int start_x, int start_y){
  if (p!=NULL){
    p->room_id = starting_room_id;
    p->x = start_x;
    p->y = start_y;

    //new
    free(p->collected_treasures);
    p->collected_treasures = NULL;
    p->collected_count = 0;


    return OK;
  }
  return INVALID_ARGUMENT;

}

//new funcs for A2

Status player_try_collect(Player *p, Treasure *treasure){
  if (p==NULL || treasure == NULL){
    return NULL_POINTER;
  }
  if(player_has_collected_treasure(p, treasure->id)){ return INVALID_ARGUMENT;}

  Treasure **temp = realloc(p->collected_treasures, sizeof(Treasure *) * (p->collected_count+1));
  if(temp==NULL){return NO_MEMORY;}

  p->collected_treasures = temp;
  p->collected_treasures[p->collected_count] = treasure;
  p->collected_count++;

  treasure->collected = true;

  return OK;  
}

bool player_has_collected_treasure(const Player *p, int treasure_id){
  if(p==NULL || treasure_id<0){return false;}

  for(int i=0; i<p->collected_count; i++){
    if(p->collected_treasures[i] != NULL && p->collected_treasures[i]->id == treasure_id){
      return true;
    }
  }
  return false;
}

int player_get_collected_count(const Player *p){
  if(p == NULL){
    return 0;
  }
  return p->collected_count;
}

const Treasure * const *
player_get_collected_treasures(const Player *p, int *count_out){
  if(p==NULL || count_out == NULL){
    return NULL;
  }

  *count_out = p->collected_count;
  return (const Treasure * const *)p->collected_treasures;
}