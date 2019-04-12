#include "header.h"
#include <string.h>
#include <signal.h>

#define UPDATE_TIME 	0.015625

enum {
	STATE_DEAD 		= 1,
	STATE_NORMAL 	= 2,
	STATE_SHOTGUN	= 3,
};


typedef struct {
	uint8_t hp;
	uint8_t x, y;
	int dirX, dirY;
} Entity;


typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr_in * ptsockaddr_in;
typedef unsigned int uint;



void leave( int );
void error( char * );
void checkPos( uint8_t *, uint8_t * );
void chooseDir( uint8_t, int *, int * );
void applyDir( int );
void updateEnts();
void updateStates();
void playerMove( int, uint8_t );
void playerShoot( int, uint8_t );
uint itemSpawn( uint, void * );
uint playerSpawn( uint, void * );
void sendData();
int handleDataIn( void * );
