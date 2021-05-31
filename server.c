#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include "support/file.h"
#include "support/message.h"
#include "support/log.h"
#include "gamestate.h"
#include "grid.h"
#include "gold.h"
#include "player.h"
#include "spectator.h"

// Global Variables
int MaxNameLength = 50;
int MaxPlayers = 26;
int GoldTotal = 250;
int GoldMinNumPiles = 10;
int GoldMaxNumPiles = 30;


// Function prototypes
void parseArgs(const int argc, const char* argv[], int* seed);
static int numberOfColumns(FILE* mapfile);
static void gold_distribute(grid_t* Grid, gold_t* Gold);
static gamestate_t* game_init(FILE* mapfile);
static void game_close(gamestate_t* gameState);
void handleInput(void* arg);
char** tokenize(char* message);
void deleteTokens(char** parsedMessage);
void handleMessage(void* arg, const addr_t fromAddress, char* message);
static void handlePlayerQuit(gamestate_t* state, addr_t fromAddress);
static void addSpectatorToGame(gamestate_t* state, addr_t fromAddress);
static void reportMalformedMessage(addr_t fromAddress, char* givenInput, const char* message);
static int randomInt(int lower, int upper);
static void handleSpectatorQuit(gamestate_t* state, addr_t fromAddress);

int
main(const int argc, const char* argv[])
{
    // Parse arguments  and use seed value
    int* seed = malloc(sizeof(int));
    parseArgs(argc, argv, seed);
    srand(*seed);

    // Open and close map file and init gamestate object
    FILE* fp  = fopen(argv[1], "r");
    gamestate_t* gs = game_init(fp);
    fclose(fp);

    // Initialize network and get port number
    int port = message_init(stderr);
    if(port == 0){
        fprintf(stderr, "Could not initialize message...\n");
        exit(1);
    }

    // // Start message loop
    // message_loop(
    //     gs, /* Argument passed to all callbacks */
    //     NULL,/* Timeout specifier (NULL in our case) */
    //     NULL,/* Handle Timeout function pointer (NULL in our case) */
    //     NULL, /* Handle stdin (NULL in our case) */
    //     handle_message
    // );

    // Free all gamestate memory
    game_close(gs);

    // Free seed value
    free(seed);
}

void 
parseArgs(const int argc, const char* argv[], int* seed)
{
    // Check for illegal # of arguments
    if(argc != 3){
        fprintf(stderr, "Illegal number of arguments...\n");
        exit(1);
    }

    // Make sure seed is a valid number
    for(int i = 0; i < strlen(argv[2]); i++){
        if(!isdigit(argv[2][i])){
            fprintf(stderr, "Invalid seed...\n");
            exit(1);
        }
    }

    // Set seed value
    *seed = atoi(argv[2]);

    // Try to open map file
    FILE* fp;
    if((fp = fopen(argv[1], "r")) == NULL){
        fprintf(stderr, "Could not open file...\n");
        exit(1);
    }
    fclose(fp);
}

static int
numberOfColumns(FILE* mapfile)
{
  char* line = file_readLine(mapfile);

  if(line == NULL) return 0;

  int len = strlen(line);
  free(line);

  return len;
}


static void gold_distribute(grid_t* Grid, gold_t* Gold){
    char **grid = Grid->g;

    int i = 0;
    int x, y;

    while(i < Gold->numPiles){
        x = randomInt(1, Grid->cols);
        y = randomInt(1, Grid->rows);
        if(grid[y][x] == '.'){
            grid[x][y] = '*';
            i += 1;
        }
    }
}

static
gamestate_t* game_init(FILE* mapFile)
{

  if(mapFile == NULL){
    fprintf(stderr, "Unable to open and read map file");
    exit(1);
  }
    
  // Create gamestate pointer and call gamestate_init()
  gamestate_t* gameState = gamestate_init(mapFile);

  // Condition: gamestate_init successfully created an object
  if(gameState == NULL){
    // If gamestate_init gives a NULL poiter, exit with error
    fprintf(stderr, "Unable to allocate space for the game state.\n");
    exit(1);
  }else{
    // Distribute gold throughout the grid
    gold_distribute(gameState->masterGrid, gameState->gameGold);
  }

  // Return the gamestate object
  return gameState;
}

static void
game_close(gamestate_t* gameState)
{

  if(gameState == NULL){
    fprintf(stderr, "Error: GameState Null...\n");
    exit(1);
  } else {
    // Close game and free memory
    gamestate_closeGame(gameState);
  }
}

// void handleInput(void* arg){

// }

char**
tokenize(char* message)
{
  if (message == NULL) {
    return NULL;
  }
  /* init position in tokens */
  int pos = 0;

  /* allocate memory for char pointers, max = strlen */
  char** tokens = calloc(strlen(message), sizeof(char*));

  if (tokens == NULL) {
    return NULL;
  }

  char* token = strtok(message, " ");

  /* while next token is not NULL 
     save it and get next token */
  while (token != NULL) {
    tokens[pos++] = malloc(sizeof(token));
    strcpy(tokens[pos-1], token);
    token = strtok(NULL, " ");
  }
  return tokens;
}

void
deleteTokens(char** parsedMessage)
{
  if (parsedMessage != NULL) {

    /* step through the tokens and free each token */
    for (int i = 0; parsedMessage[i] != NULL; i++) {
      free(parsedMessage[i]);
    }

    /* free the holder */
    free(parsedMessage);
  }
}

void
handleMessage(void* arg, const addr_t fromAddress, char* message)
{
  /* if any pointer arguments are NULL, return. */
  if (arg == NULL || message == NULL) {
    return;
  }

  /* convert arg back to gamestate */
  gamestate_t* state = (gamestate_t*) arg;
  /* get tokens in message */
  char** tokens;
  if ( (tokens = tokenize(message)) != NULL) {

    /* find number of tokens */
    int numTokens = 0;
    for (int i = 0; tokens[i] != NULL; i++) {
      numTokens = i + 1;
    }

    if (numTokens == 0) {
      // Send malformed message back to client / spectator
      message_send(fromAddress, "ERROR malformed message\n");
      fprintf(stderr, "Message detected with ZERO tokens. Stop.\n");
      free(message);
      free(tokens);
      return;
    }

    /* run switch statement on first char of first token */
    switch (tokens[0][0])
    {
      case 'K':

        /* handle gameplay keys */
        if (numTokens == 2 && (strcmp(tokens[0], "KEY") == 0) ) {

          /* call function to handle key here */
          handleKey(state, fromAddress, tokens[1]);
        }
        else {
          reportMalformedMessage(fromAddress, message, "is not a valid gameplay message.");
          // message_send(fromAddress, "ERROR malformed message\n");
          // fprintf(stderr, "'%s' is not a valid gameplay message.\n", message);
          // fprintf(stderr, "Invalid action sequence detected. Stop.\n");
        }
        break;

      case 'S':

        /* add spectator if message checks out*/
        if (numTokens == 1 && (strcmp(tokens[0], "SPECTATE") == 0) ) {

          /* add spectator to game */
          addSpectatorToGame(state, fromAddress);
        }
        else {
          // Oherwise end error message to from address and log to stderr
          reportMalformedMessage(fromAddress, message, "is not a valid add spectator message.");
          // message_send(fromAddress, "ERROR malformed message\n");
          // fprintf(stderr, "'%s' is not a valid add spectator message.\n", message);
          // fprintf(stderr, "Error: invalid action sequence detected. Stop.\n");
        }
        break;
      case 'Q':
        /* routine for removing player */
        if (numTokens == 0 && (strcmp(tokens[0], "QUIT") == 0) ) {

          /* if address matches spectator in the game... */
          if (gamestate_isSpectator(state, fromAddress)) {
            // Handle spectator quit
            handleSpectatorQuit(state, fromAddress);
          }

          /* else -->
             if address doesn't match the current spectator,
             search for player instead.
             
             then, if, no match was found. print error message. */
          else {
            handlePlayerQuit(state, fromAddress);
          }
        }
        break;

      case 'P':
        /* routine to add player */
        if (numTokens >= 2 && (strcmp(tokens[0], "PLAY") == 0) ) {

          /* init grid for player */
          grid_t* playerGrid = grid_initForPlayer(state->masterGrid);

          /* get number of rows and columns in master grid */
          int rows = state->masterGrid->rows;
          int cols = state->masterGrid->cols;

          /* generate letter for player */
          char letter = 'A' + state->players_seen;

          /* get full player name */
          char playerName[MaxNameLength];
          for (int i = 1; i < numTokens; i++) {
            char *temp = tokens[i];
            if(i == 1){
              strcpy(playerName, temp);
            }else{
              strcat(playerName, temp);
            }
          }

          /* generate random x,y values 
             and check for validity 
             until valid point is found */
          int x = randomInt(1, cols);
          int y = randomInt(1, rows);

          while (! grid_isSpace(state->masterGrid, x, y)) {
            x = randomInt(1, cols);
            y = randomInt(1, rows);
          }

          /* create player */
          player_t* newPlayer = player_new(letter, playerName, fromAddress, x, y, playerGrid);

          /* if player created successfully,
             add to gamestate */
          if (newPlayer != NULL) {
            gamestate_addPlayer(state, newPlayer);
            char initMessage[100];
            sprintf(initMessage, "GRID %d, %d", rows, cols);
            player_send(newPlayer, initMessage);
          }

          /* if player creation failed, 
             delete player grid
             print error flag */
          else {
            grid_delete(playerGrid);
            reportMalformedMessage(fromAddress, message, "is not a valid player message.");
            // fprintf(stderr, "'%s' is not a valid add player message.\n", message);
            // fprintf(stderr, "Invalid action sequence detected. Stop.\n");
          }
        }
        else {
          reportMalformedMessage(fromAddress, message, "is not a valid message.");
          // fprintf(stderr, "'%s' is not a valid message.\n", message);
          // fprintf(stderr, "Invalid action sequence detected. Stop.\n");
        }
        break;

    }
    deleteTokens(tokens);
    free(message);
  }
}

/**************** Static Functions ******************/

/**
 * @brief: Function to generate a number within a range.
 * calls rand() to generate a random number 
 * then bounds it to expectate range using mod operator.
 * This is handy because rand() doesn't take any arguments
 * and generates unbounded values
 * 
 * @param lower: lower bound, inclusive
 * @param upper: upper bound, inclusive
 * @return int: a random value between the lower and upper bound.
 */
static int
randomInt(int lower, int upper)
{
  if (lower < upper) {
    int num = lower;
    int randomNumber = rand();
    num += randomNumber % (upper - lower);
    return num;
  }
  fprintf(stderr, "Attempt to generate a number with invalid bounds. Stop.\n");
  return -1;
}

static void
addSpectatorToGame(gamestate_t* state, addr_t fromAddress){
  /* add spectator to game */
  gamestate_addSpectator(state, fromAddress);

  /* get number of rows, columns in grid */
  int rows = state->masterGrid->rows;
  int cols = state->masterGrid->cols;

  /* generate init message */
  char initMessage[100];                            /* No need to malloc; only used once */
  sprintf(initMessage, "GRID %d, %d", rows, cols);

  /* send init message to spectator */
  spectator_send(state->spectator, initMessage);
}

static void
reportMalformedMessage(addr_t fromAddress, char* givenInput, const char* message){
  message_send(fromAddress, "ERROR malformed message\n");
  fprintf(stderr, "'%s' %s \n", givenInput, message);
  fprintf(stderr, "Invalid action sequence detected. Stop.\n");
}

static void
handleSpectatorQuit(gamestate_t* state, addr_t fromAddress){
  /* get spectator */
  spectator_t* spectator = state->spectator;

  /* send QUIT message to spectator */
  spectator_send(spectator, "QUIT Thank you for watching!");

  /* delete spectator */
  spectator_delete(spectator);

  /* reset state spectator pointer to NULL 
      (just so we don't get any surprises) */
  state->spectator = NULL;
}

static void
handlePlayerQuit(gamestate_t* state, addr_t fromAddress){
  /* We don't actually delete the player from the game 
    because we need to keep their information
    until end of game. */

  // Search for player with matching address in gamestate
  player_t* player = gamestate_findPlayerByAddress(state, fromAddress);

  // If we find a matching player in the game, let them quit
  if (player != NULL) {
    player_send(player, "QUIT Thank you for playing!");
  }

  /* if the search function returns NULL, 
      no player was found. 
      print to stderr. */
  else {
    fprintf(stderr, "No matching player OR spectator found for an incoming QUIT message.\n");
  }
}