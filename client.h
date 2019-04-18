#include "header.h"
#include <string.h>
#include <poll.h>
#include <SDL2/SDL_mixer.h> 


#define NUM_PIXELS (LEVEL_WIDTH * LEVEL_HEIGHT)
#define PLAYER_COLOR 0xffffffff
#define BULLET_COLOR 0xffffd893
#define BULLET_DELAY 0.16

typedef struct {
	uint8_t x, y, hp;
} Entity;


void leave( char * );
void textureUpdate( SDL_Renderer *, SDL_Texture *, int, int, int, int );
void initGame();
int handleInput();
int handleDataIn();
