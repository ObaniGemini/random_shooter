#include "header.h"
#include <string.h>
#include <poll.h>
#include <SDL2/SDL_mixer.h> 


#define NUM_PIXELS (LEVEL_WIDTH * LEVEL_HEIGHT)
#define PLAYER_COLOR 0xffffffff
#define BULLET_COLOR 0xffffd893
#define BULLET_DELAY 0.2

enum {
	BULLET_SOUND = 0,
	DAMAGE_SOUND = 5,
	DEATH_SOUND = 8,
	POWERUP_SOUND = 9,
	SPAWN_SOUND = 11
};

typedef Mix_Chunk * sample;

typedef struct {
	uint8_t x, y, hp;
} Entity;


void textureUpdate( SDL_Renderer *, SDL_Texture *, int, int, int, int );
void playSound( sample *, int );
void leave( char * );
void initGame();
int handleInput();
int handleDataIn();
