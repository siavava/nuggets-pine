/**
 * @file gamestate.c
 * @author TEAM PINE
 * @brief: functionality for gamestate
 * @version 0.1
 * @date 2021-05-25
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include <stdio.h>
#include <stdlib.h>

#include "player.h"
#include "spectator.h"
#include "grid.h"
#include "gold.h"
#include "spectator.h"
#include "gamestate.h"

gamestate_t*
gamestate_init(FILE* mapFile)
{
  gamestate_t* state = malloc(sizeof(gamestate_t));
  if (state == NULL) {
    fprintf(stderr, "Error allocating memory for gamestate.\n");
    return NULL;
  }

  // Initialize grid field
  gamestate_initGrid(state, mapFile);

  // Initialize gold field
  gamestate_initGold(state);

  // Initialize player array field
  gamestate_initPlayers(state);

  // Initialize spectator field
  gamestate_initSpectator(state);

  return state;
}

static void 
gamestate_initPlayers(gamestate_t* state){
  *(state->players) = (sizeof(player_t) * 26);
}

static void 
gamestate_initGold(gamestate_t* state){
  state->gameGold = gold_init(26);
}

static void
gamestate_initGrid(gamestate_t* state, FILE* mapFile){
  state->masterGrid = grid_init(mapFile);
}

static void
gamestate_initSpectator(gamestate_t* state){
  state->spectator = NULL;
}

gamestate_t**
gamestate_getPlayers(gamestate_t* game)
{
  return game->players;
}

void
gamestate_addSpectator(gamestate_t* state, spectator_t* newSpectator)
{
  if (state != NULL && newSpectator != NULL) {
    if (state->spectator != NULL) {
      spectator_delete(state->spectator);
    }
    state->spectator = newSpectator;
  }
}