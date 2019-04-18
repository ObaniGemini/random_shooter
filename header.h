#include <stdio.h>
#include <unistd.h>
#include <time.h>

#include <SDL2/SDL.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>


#define PIX_SIZE		8
#define LEVEL_WIDTH		80
#define LEVEL_HEIGHT	80
#define SCREEN_WIDTH	720
#define SCREEN_HEIGHT	720


#define MAX_PLAYERS		4
#define MAX_BULLETS		80
#define MAX_ENTITIES	(MAX_PLAYERS + 1 + MAX_BULLETS)
#define TIMEOUT 		15000


#define HP_BULLET		80
#define HP_PLAYER		10


#define DIR_LEFT		0x01
#define DIR_DOWN		0x02
#define DIR_RIGHT		0x04
#define DIR_UP			0x08
#define QUIT			0x10