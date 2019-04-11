#include "client.h"
#include "menu.h"


Entity ents[MAX_ENTITIES];
uint8_t events[2] = { };
uint32_t terrain[NUM_PIXELS];

struct pollfd server;
struct sockaddr_in sv_addr;  //addresse
socklen_t sv_addr_len = sizeof( sv_addr );

uint8_t quit = 0;


void leave(char *str) {
	if( str[0] )
		perror(str);		//Send error if there's one

	quit = QUIT;
	sendto( server.fd, &quit, 1, 0, (struct sockaddr *)&sv_addr, sv_addr_len );
	close( server.fd );
}



void initGame() {
	for( int i = 0; i < MAX_ENTITIES; i++ )
		ents[i].hp = 0;
}


int randomizeTerrain() {
	clock_t t1 = clock(), t2;
	for(;;) {
		t2 = clock();
		if( (double)(t2 - t1) / CLOCKS_PER_SEC >= 0.01 ) {
			t1 = t2;
			terrain[rand() % NUM_PIXELS] = ( rand() % 50 ) | (( 175 - rand() % 50 ) << 8) | (( rand() % 25 ) << 16) | (255 << 24);
		}
	}
}


int handleInput() {
	SDL_Event e;
	clock_t t1 = clock(), t2;
	const uint8_t *keystates = NULL;
	int can_shoot = 1;

	for(;;){
		if( quit ) return 0;

		if ( SDL_PollEvent( &e ) ) {
			if( e.type == SDL_QUIT || e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE ) {
				leave("");		//handle quitting program
				return 0;
			}
		}

		keystates = SDL_GetKeyboardState(NULL);

		if( keystates[SDL_SCANCODE_LEFT] ) 	events[0] |= MOVE | DIR_LEFT;
		if( keystates[SDL_SCANCODE_DOWN] ) 	events[0] |= MOVE | DIR_DOWN;
		if( keystates[SDL_SCANCODE_RIGHT] )	events[0] |= MOVE | DIR_RIGHT;
		if( keystates[SDL_SCANCODE_UP] ) 	events[0] |= MOVE | DIR_UP;

		if( can_shoot ) {
			if( keystates[SDL_SCANCODE_S] ) { events[1] |= SHOOT | DIR_LEFT;	can_shoot = 0; }
			if( keystates[SDL_SCANCODE_D] ) { events[1] |= SHOOT | DIR_DOWN;	can_shoot = 0; }
			if( keystates[SDL_SCANCODE_F] ) { events[1] |= SHOOT | DIR_RIGHT;	can_shoot = 0; }
			if( keystates[SDL_SCANCODE_E] ) { events[1] |= SHOOT | DIR_UP;		can_shoot = 0; }
		} else {
			t2 = clock();
			if( (double)(t2 - t1) / CLOCKS_PER_SEC >= BULLET_DELAY ) {
				t1 = t2;
				can_shoot = 1;
			}
		}
	}
}


int handleDataIn( void *full_addr ) {
	printf("Attempting connection to %s ...\n", (char *)full_addr);
	char *tok;
	int connected = 0;

	memset((char *) &sv_addr, 0, sizeof(sv_addr));

	sv_addr.sin_family		= AF_INET;
	tok = strtok( (char *)full_addr, ":" );
	sv_addr.sin_addr.s_addr = inet_addr( tok );
	tok = strtok( NULL, ":" );
	sv_addr.sin_port		= htons( atoi( tok ) );

	memset( sv_addr.sin_zero, 0, sizeof( sv_addr.sin_zero ) );

	free((char*)full_addr);

	if( ( server.fd = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP ) ) < 0 ) {
		leave("socket");
		return 0;
	}


	server.events = POLLIN;

	for(;;) {
		if( poll( &server, 1, TIMEOUT ) <= 0 ) {
			printf("Connection timed out\n");
			leave("poll");
			return 0;
		}

		if( server.revents & POLLIN ) {
			if( !connected ) {
				connected = 1;
				printf("Connection succeeded !\n");
			}

			uint8_t data[MAX_ENTITIES*4];
			int len = recvfrom( server.fd, data, MAX_ENTITIES*4, 0, (struct sockaddr *)&sv_addr, &sv_addr_len );
			for( int i = 0; i < len; i += 4 ) {
				uint8_t entnum = data[i];
				/*if( !ents[entnum].hp && data[i+1] ) {
					if( entnum < MAX_PLAYERS )
						printf("Player %d spawning\n", entnum+1);
					else
						printf("Shooting\n");
				}*/
				if( ents[entnum].hp != data[i+1] ) {

				}
				ents[entnum].hp = data[i+1];
				ents[entnum].x = data[i+2];
				ents[entnum].y = data[i+3];
			}
		}
	}
}


int main( int argc, char **argv ) {
	if( argc != 3 ) {
		printf("Usage : command <host> <port>\n");
		return 1;
	}

	char *full_addr = malloc( sizeof( argv[1] ) + sizeof( argv[2] ) + 1 );
	uint32_t level[NUM_PIXELS];
	uint32_t pix = 0xaaaaaaaa;
	srand(time(NULL));

	int i;
	for( i = 0; argv[1][i]; i++ )
		full_addr[i] = argv[1][i];
	full_addr[i] = ':';
	for( int j = 0; argv[2][j]; j++ )
		full_addr[j + i + 1] = argv[2][j];


	SDL_Init( SDL_INIT_VIDEO | SDL_INIT_TIMER );
	Mix_Init( MIX_INIT_OGG );


	SDL_Window *win = SDL_CreateWindow( "Shooter", 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0 );
	SDL_Renderer *renderer = SDL_CreateRenderer( win, -1, SDL_RENDERER_ACCELERATED );


	//menuMain( renderer );


	SDL_Texture *texture = SDL_CreateTexture( renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, LEVEL_WIDTH, LEVEL_HEIGHT );
	SDL_Texture *white = SDL_CreateTexture( renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, 1, 1 );
	SDL_UpdateTexture( white, NULL, &pix, sizeof( uint32_t ) );


	for( int i = 0; i < NUM_PIXELS; i++ )
		terrain[i] = ( rand() % 50 ) | (( 175 - rand() % 50 ) << 8) | (( rand() % 25 ) << 16) | (255 << 24);


	initGame();
	sendto( server.fd, events, 2, 0, (struct sockaddr *)&sv_addr, sv_addr_len );
	SDL_CreateThread( handleInput, "handleInput", NULL );
	SDL_CreateThread( handleDataIn, "handleDataIn", (void *)full_addr );
	SDL_CreateThread( randomizeTerrain, "randomizeTerrain", NULL );


	/* Put them here to have less calculation on each step */
	const size_t pixW = LEVEL_WIDTH * sizeof ( uint32_t );
	const int X1 = (SCREEN_WIDTH - LEVEL_WIDTH*PIX_SIZE)/2, X2 = (SCREEN_WIDTH - LEVEL_WIDTH*PIX_SIZE)/2 - PIX_SIZE, X3 = (SCREEN_WIDTH - LEVEL_WIDTH*PIX_SIZE)/2 + LEVEL_WIDTH*PIX_SIZE;
	const int Y1 = (SCREEN_HEIGHT - LEVEL_HEIGHT*PIX_SIZE)/2, Y2 = (SCREEN_HEIGHT - LEVEL_HEIGHT*PIX_SIZE)/2 - PIX_SIZE, Y3 = (SCREEN_HEIGHT - LEVEL_HEIGHT*PIX_SIZE)/2 + LEVEL_HEIGHT*PIX_SIZE;
	const int W1 = LEVEL_WIDTH*PIX_SIZE, W2 = LEVEL_WIDTH*PIX_SIZE + PIX_SIZE*2;
	const int H1 = LEVEL_HEIGHT*PIX_SIZE;


	for(;;) {
		if( quit ) break;

		for( int i = 0; i < NUM_PIXELS; i++ )
			level[i] = terrain[i];

		for( int i = 0; i < MAX_ENTITIES; i++ )
			if( ents[i].hp )
				level[(ents[i].x + ents[i].y*LEVEL_WIDTH)] = ( i > MAX_PLAYERS ?
															 ( 55 + ents[i].hp/2 ) | ( ( 155 + ents[i].hp )  << 8 ) | ( ( 200 + ents[i].hp/2 ) << 16 ) | ( 255 << 24 ) :	//bullets color
															 ( i == MAX_PLAYERS ? 
															 ( rand() % 256 ) | ( ( rand() % 256 ) << 8 ) | ( ( rand() % 256 ) << 16 ) | ( 255 << 24 )	:	//shotgun color
															 ( ents[i].hp*25 ) | ( ( ents[i].hp*25 ) << 8 ) | ( 255 << 16 ) | ( 255 << 24 ) ) );		//players color

		if( events[0] || events[1] ) {
			sendto( server.fd, events, 2, 0, (struct sockaddr *)&sv_addr, sv_addr_len );
			memset( events, 0, 2 );
		}

		SDL_UpdateTexture( texture, NULL, level, pixW );
		textureUpdate( renderer, texture, X1, Y1, W1, H1 );
		textureUpdate( renderer, white, X2, Y2, W2, PIX_SIZE );
		textureUpdate( renderer, white, X2, Y3, W2, PIX_SIZE );
		textureUpdate( renderer, white, X2, Y1, PIX_SIZE, H1 );
		textureUpdate( renderer, white, X3, Y1, PIX_SIZE, H1 );
		SDL_RenderPresent( renderer );
	}

	//wait for everything to quit to avoid dumb SDL errors about program exitting while SDL actions are being performed
	SDL_Delay( 20 );
	SDL_DestroyWindow(win); //Destroy window
	Mix_Quit(); //Quit SDL_mixer
	SDL_Quit(); //Quit SDL
	printf("\n-------------\n Client quit\n-------------\n");
	return 0;
}