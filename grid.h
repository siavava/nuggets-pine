/**
 * @file grid.h
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2021-05-25
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef __GRID_H
#define __GRID_H

/* standard libs */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <float.h>
#include <math.h>

typedef struct grid {
  char** g;
  int rows;
  int cols;
} grid_t;

typedef struct player player_t;
typedef struct game gamestate_t;

grid_t* grid_init(FILE* mapfile);

grid_t* grid_initForPlayer(grid_t* masterGrid);

void grid_delete(grid_t* grid);

int grid_getRows(grid_t* grid);

int grid_getColumns(grid_t* grid);

char** grid_getGrid(grid_t* grid);

bool grid_isWall(grid_t* grid, int x, int y);

bool grid_isPlayer(grid_t* grid, int x, int y);

bool grid_isGold(grid_t* grid, int x, int y);

bool grid_isPassage(grid_t* grid, int x, int y);

bool grid_isSpace(grid_t* grid, int x, int y);

bool grid_canMove(grid_t* master, player_t* player, char k);

void grid_calculateVisibility(grid_t* Grid, player_t* player);

grid_t* grid_copy(grid_t* original_grid);

char* grid_toString(gamestate_t* state, grid_t* grid);

#endif /* __GRID_H */