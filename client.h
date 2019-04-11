#include "header.h"
#include <string.h>
#include <poll.h>
#include <SDL2/SDL_mixer.h> 


#define NUM_PIXELS (LEVEL_WIDTH * LEVEL_HEIGHT)
#define PLAYER_COLOR 0xffffffff
#define BULLET_COLOR 0xffffd893
#define BULLET_DELAY 0.28125

typedef struct {
	uint8_t x, y, hp;
} Entity;


void leave( char * );
void initGame();
int randomizeTerrain();
int handleInput();
int handleDataIn();
void textureUpdate( SDL_Renderer *r, SDL_Texture *t, int x, int y, int w, int h );