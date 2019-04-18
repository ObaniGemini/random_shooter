#include "header.h"
#include <math.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>


#define BACKGROUND_PIX (SCREEN_WIDTH/8)
#define BUTTON_WIDTH (SCREEN_WIDTH - SCREEN_WIDTH/4)
#define BUTTON_HEIGHT (SCREEN_HEIGHT/8)


SDL_Surface *CreateButton( const char *, TTF_Font * );
void menuMain( SDL_Renderer * );