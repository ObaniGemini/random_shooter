#include "server.h"


int clients;
sockaddr_in cl_addr[MAX_PLAYERS];
ptsockaddr_in pt_addr[MAX_PLAYERS];

Entity ents[MAX_ENTITIES];
uint8_t events[MAX_PLAYERS*2];
uint8_t player_states[MAX_PLAYERS] = { };
uint8_t ping[MAX_PLAYERS] = { };


/* Quit on signale 'n' (handles Ctrl+C) */
void leave(int n) {
	close(clients);
	printf("\n|-----------------|\n| Server shutdown |\n|-----------------|\n");
	exit(1);
}

/* Quit on perror */
void error( char *str ) {
	perror(str);
	leave(0);
}



/* Check pos of object to make it fit to screen */
void checkPos( uint8_t *x, uint8_t *y ) {
	if( *x >= 254 ) *x = LEVEL_WIDTH - 1;	//uints make it fucked up, but it's worth it for sending data (I guess ?)
	else if( *x >= LEVEL_WIDTH ) *x = 0;	//I use 254 because you could shoot out of the screen and it would be x + 2 instead of x + 1

	if( *y >= 254 ) *y = LEVEL_HEIGHT - 1;
	else if( *y >= LEVEL_HEIGHT ) *y = 0;
}


/* Assign the dirX/dirY elements corresponding to the dir info */
void chooseDir( uint8_t dir, int *dirX, int *dirY ) {
	*dirX = 0; *dirY = 0;
	if( dir & DIR_LEFT && *dirX != -1 ) (*dirX)--;
	if( dir & DIR_RIGHT && *dirX != 1 ) (*dirX)++;
	if( dir & DIR_UP && *dirY != -1 ) (*dirY)--;
	if( dir & DIR_DOWN && *dirY != 1 ) (*dirY)++;
}


/* Apply dir for a given entity */
void applyDir( int ent ) {
	ents[ent].x += ents[ent].dirX;
	ents[ent].y += ents[ent].dirY;
	if( ent < MAX_PLAYERS ) {
		ents[ent].dirX = 0;
		ents[ent].dirY = 0;
	}
}


/* Update entities pos */
void updateEnts() {
	for( int i = 0; i < MAX_ENTITIES; i++ ) {
		if( ents[i].hp ) { /* If entity is alive */
			applyDir( i );
			checkPos( &(ents[i].x), &(ents[i].y) );
			if(	i > MAX_PLAYERS ) /* If it's a bullet */
				ents[i].hp--;
		}
	}
}



/* Update states of entities */
void updateStates() {
	for( int b = MAX_PLAYERS; b < MAX_ENTITIES; b++ ) { /* Bullets */
		if( !ents[b].hp ) continue;						/* If bullet is dead, skip */

		for( int p = 0; p < MAX_PLAYERS; p++ ) {		/* Players */
			if( !ents[p].hp ) continue;					/* If player is dead, skip */
			if( ents[p].x == ents[b].x && ents[p].y == ents[b].y ) {
				ents[b].hp = 0;
				if( b == MAX_PLAYERS ) {
					if( rand() % 3 == 0 ) {
						for( int y = 0; y < LEVEL_HEIGHT; y += 4 ) {
							if( y != ents[p].y ) {
								playerShoot( -1, ents[p].x, y, DIR_LEFT );
								playerShoot( -1, ents[p].x, y, DIR_RIGHT );
							}
						}
						printf("Player %d called an airstrike !\n", p+1 );
					} else {
						printf("Player %d picked up another gun !\n", p+1 );
						player_states[p] = STATE_SHOTGUN;
					}
					SDL_AddTimer( 15000, itemSpawn, NULL );
				} else {
					ents[p].hp--;							/* If bullet and player have the same pos, kill bullet and decrement player hp */
					if( !ents[p].hp ) {						/* If player is dead, respawn */
						player_states[p] = STATE_DEAD;
						SDL_AddTimer( 5000, playerSpawn, NULL );
					}
				}
			}
		}
	}
}


/* Change player's moving direction */
void playerMove( int player, uint8_t dir ) {
	if( !ents[player].hp ) return; /* don't move if player is dead */
	chooseDir( dir, &(ents[player].dirX), &(ents[player].dirY) );
}


/* Spawn a bullet on empty location if player shoots */
void playerShoot( int player, uint8_t x, uint8_t y, uint8_t dir ) {
	int shots = 1; /* Useful for powerup */

	if( player != -1 ) {
		if( !ents[player].hp ) return; /* don't shoot if player is dead */
		if( (dir & DIR_UP && dir & DIR_DOWN) || (dir & DIR_LEFT && dir & DIR_RIGHT)) return; /* don't shoot if opposite directions */
	}

	for( int i = MAX_PLAYERS + 1; i < MAX_ENTITIES; i++ ) {
		if( ents[i].hp ) continue;

		chooseDir( dir, &(ents[i].dirX), &(ents[i].dirY) );
		if( player == -1 || player_states[player] == STATE_NORMAL ) {
			ents[i].x = x + ents[i].dirX;
			ents[i].y = y + ents[i].dirY;
		} else if( ents[i].dirX && ents[i].dirY ) {
			ents[i].x = x + ents[i].dirX + shots*ents[i].dirX;
			ents[i].y = y + ents[i].dirY - shots*ents[i].dirY;
		} else {
			ents[i].x = x + ents[i].dirX + shots*ents[i].dirY;
			ents[i].y = y + ents[i].dirY + shots*ents[i].dirX;
		}

		ents[i].hp = HP_BULLET;

		if( player_states[player] != STATE_SHOTGUN || shots-- == -1 )
			break;
	}
}


/* Spawn powerup */
uint itemSpawn( uint id, void *data ) { /* These arguments are useless, needed for the SDL_Timer callback */
	for( int i = 0; i < MAX_PLAYERS; i++ )
		if( player_states[i] != STATE_DEAD ) /* prevent players from not respawning if item spawns */
			player_states[i] = STATE_NORMAL;

	ents[MAX_PLAYERS].x = rand() % LEVEL_WIDTH;
	ents[MAX_PLAYERS].y = rand() % LEVEL_HEIGHT;
	ents[MAX_PLAYERS].hp = 1;

	return 0;
}


/* Spawn player */
uint playerSpawn( uint id, void *data ) { /* These arguments are useless, needed for the SDL_Timer callback */
	for( int i = 0; i < MAX_PLAYERS; i++ ) {
		if( player_states[i] != STATE_DEAD ) continue;

		ents[i].x = rand() % LEVEL_WIDTH;
		ents[i].y = rand() % LEVEL_HEIGHT;
		ents[i].hp = HP_PLAYER;
		player_states[i] = STATE_NORMAL;
	}
	return 0;
}



/* Send data to the clients */
void sendData() {
	uint32_t data[MAX_ENTITIES];
	for( uint8_t i = 0; i < MAX_ENTITIES; i++ )
		data[i] = i | ( ents[i].hp << 8 ) | ( ents[i].x << 16 ) | ( ents[i].y << 24 );	//ent num, hp, x, y

	for( int p = 0; p < MAX_PLAYERS; p++ ) {
		if( pt_addr[p] == NULL ) continue;
		if( sendto( clients, data, MAX_ENTITIES*4, 0, (struct sockaddr *)pt_addr[p], sizeof( cl_addr[p] ) ) < 0 )
			error("send");
	}
}


/* Handle incoming data and init serv connection */
int handleDataIn( void *addr ) {
	sockaddr_in sv_addr = *(struct sockaddr_in *)addr;

	for(;;) {
		sockaddr_in tmp;
		socklen_t tmp_s = sizeof( tmp );
		uint8_t buffer[MAX_PLAYERS*2];
		int i, len = recvfrom( clients, buffer, MAX_PLAYERS*2, 0, (struct sockaddr *)&tmp, &tmp_s );

		for( i = 0; i < MAX_PLAYERS; i++ ) {
			if( pt_addr[i] == NULL ) {
				printf(" ----------------\nPlayer %d connected\n ----------------\n", i+1);
				cl_addr[i] = tmp;
				pt_addr[i] = &cl_addr[i];
				player_states[i] = STATE_DEAD;
				playerSpawn( 0, NULL );
				break;
			} else if( tmp.sin_addr.s_addr == cl_addr[i].sin_addr.s_addr )
				break;
		}

		ping[i] = 1;

		for( int j = 0; j < len; j++ ) {
			if( buffer[j] & QUIT ) {
				int k = i+1;
				printf(" -----------\nPlayer %d quit\n -----------\n", k);

				for( ; k < MAX_PLAYERS; k++ ) {
					if(pt_addr[k] == NULL)
						break;
					cl_addr[k-1] = cl_addr[k];
					pt_addr[k-1] = &cl_addr[k-1];
					ents[k-1] = ents[k];
				}
				pt_addr[k-1] = NULL;
				ents[k-1].hp = 0;
				break;
			}
			events[j + i*2] |= buffer[j];
		}
	}
}




int main( int argc, char **argv ) {
	if( argc != 2 ) {
		printf("Usage : command <port>\n");
		return 1;
	}

	sockaddr_in sv_addr;

	if( (clients = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP )) < 0 )
		error("socket");

	memset((char *) &sv_addr, 0, sizeof(sv_addr));
	sv_addr.sin_family		= AF_INET;
	sv_addr.sin_port		= htons( atoi(argv[1]) );
	sv_addr.sin_addr.s_addr	= htonl(INADDR_ANY);

	if( bind( clients, (struct sockaddr *)&sv_addr, sizeof( sv_addr ) ) < 0 )
		error("bind");


	time_t ping_timer[MAX_PLAYERS];
	time_t elapsed, timeT = clock();

	for( int i = 0; i < MAX_PLAYERS; i++ ) {
		pt_addr[i] = NULL;
	}
	for( int i = 0; i < MAX_ENTITIES; i++ ) {
		ents[i].hp = 0; ents[i].dirX = 0; ents[i].dirY = 0;
	}


	SDL_Init( SDL_INIT_TIMER );
	SDL_AddTimer( 15000, itemSpawn, NULL );
	SDL_CreateThread( handleDataIn, "handleDataIn", (void *)(&sv_addr) );
	signal(2, leave);


	/* Handle real-time */
	for(;;) {
		elapsed = clock();

		if( (double)(elapsed - timeT) / CLOCKS_PER_SEC >= UPDATE_TIME ) {
			timeT = elapsed;

			for( int i = 0; i < MAX_PLAYERS; i++ ) {
				if( events[i*2] ) {
					playerMove(i, events[i*2]);
					events[i*2] = 0;
				} else if( events[i*2 + 1] ) {
					playerShoot(i, ents[i].x, ents[i].y, events[i*2 + 1]);
					events[i*2 + 1] = 0;
				}
			}

			updateEnts();
			updateStates();
			sendData();

			/* handle ping timeout */
			for( int i = 0; i < MAX_PLAYERS; i++ ) {
				if( pt_addr[i] ) {
					if( ping[i] ) {
						ping_timer[i] = elapsed;
						ping[i] = 0;
					} else if( (double)(elapsed - ping_timer[i]) / CLOCKS_PER_SEC >= TIMEOUT/1000 ) {
						printf(" ----------------\nPlayer %d timed out\n ----------------\n", i+1);
						pt_addr[i] = NULL;
					}
				}
			}
		}
	}

	return 0;
}