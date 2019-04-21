#include "client.h"
#include "menu.h"


Entity ents[MAX_ENTITIES];
uint8_t events[2] = { };
uint32_t terrain[NUM_PIXELS];

struct pollfd server;
struct sockaddr_in sv_addr;  //addresse
socklen_t sv_addr_len = sizeof( sv_addr );

uint8_t connected = 0;
uint8_t quit = 0;



void textureUpdate( SDL_Renderer *r, SDL_Texture *t, int x, int y, int w, int h ) {
	SDL_Rect rect = { x, y, w, h };
	SDL_RenderCopy( r, t, NULL, &rect );
}


void playSound( sample *samples, int index ) {
	for( int i = 0; i < 10; i++ ) {
		if(!Mix_Playing(i)) {
			Mix_PlayChannel( i, samples[index], 0 );
			break;
		}
	}
}


void leave(char *str) {
	if( str[0] )
		perror(str);		//Send error if there's one

	quit = QUIT;
	if( connected ) sendto( server.fd, &quit, 1, 0, (struct sockaddr *)&sv_addr, sv_addr_len );
	close( server.fd );
}


void initGame() {
	for( int i = 0; i < MAX_ENTITIES; i++ )
		ents[i].hp = 0;
}


int handleInput() {
	SDL_Event e;
	time_t elapsed, keyT = clock();
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

		if( keystates[SDL_SCANCODE_LEFT] ) 	events[0] |= DIR_LEFT;
		if( keystates[SDL_SCANCODE_DOWN] ) 	events[0] |= DIR_DOWN;
		if( keystates[SDL_SCANCODE_RIGHT] )	events[0] |= DIR_RIGHT;
		if( keystates[SDL_SCANCODE_UP] ) 	events[0] |= DIR_UP;

		if( can_shoot ) {
			if( keystates[SDL_SCANCODE_S] ) { events[1] |= DIR_LEFT;	can_shoot = 0; }
			if( keystates[SDL_SCANCODE_D] ) { events[1] |= DIR_DOWN;	can_shoot = 0; }
			if( keystates[SDL_SCANCODE_F] ) { events[1] |= DIR_RIGHT;	can_shoot = 0; }
			if( keystates[SDL_SCANCODE_E] ) { events[1] |= DIR_UP;		can_shoot = 0; }
		} else {
			elapsed = clock();
			if( (double)(elapsed - keyT) / CLOCKS_PER_SEC >= BULLET_DELAY ) {
				keyT = elapsed;
				can_shoot = 1;
			}
		}
	}
}


int handleDataIn( void *full_addr ) {
	printf("Attempting connection to %s ...\n", (char *)full_addr);
	char *tok;

	memset((char *) &sv_addr, 0, sizeof(sv_addr));

	sv_addr.sin_family = AF_INET;
	tok = strtok( (char *)full_addr, ":" );
	sv_addr.sin_addr.s_addr = inet_addr( tok );
	tok = strtok( NULL, ":" );
	sv_addr.sin_port = htons( atoi( tok ) );

	free((char*)full_addr);

	if( ( server.fd = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP ) ) < 0 ) {
		leave("socket");
		return 0;
	}

	sample samples[12];
	samples[0] = Mix_LoadWAV("sfx/bullet1.ogg");
	samples[1] = Mix_LoadWAV("sfx/bullet2.ogg");
	samples[2] = Mix_LoadWAV("sfx/bullet3.ogg");
	samples[3] = Mix_LoadWAV("sfx/bullet4.ogg");
	samples[4] = Mix_LoadWAV("sfx/bullet5.ogg");
	samples[5] = Mix_LoadWAV("sfx/damage1.ogg");
	samples[6] = Mix_LoadWAV("sfx/damage2.ogg");
	samples[7] = Mix_LoadWAV("sfx/damage3.ogg");
	samples[8] = Mix_LoadWAV("sfx/death.ogg");
	samples[9] = Mix_LoadWAV("sfx/powerup1.ogg");
	samples[10] = Mix_LoadWAV("sfx/powerup2.ogg");
	samples[11] = Mix_LoadWAV("sfx/spawn.ogg");


	{
		server.events = POLLIN;
		uint8_t dummy[2] = { };
		sendto( server.fd, dummy, 2, 0, (struct sockaddr *)&sv_addr, sv_addr_len );
	}

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

				if( ents[entnum].hp != data[i+1] ) {
					if( entnum <= MAX_PLAYERS ) {
						if( data[i+1] > ents[entnum].hp ) playSound( samples, SPAWN_SOUND );
						else if( !data[i+1] ) {
							if( entnum == MAX_PLAYERS ) playSound( samples, POWERUP_SOUND + rand() % 2 );
							else playSound( samples, DEATH_SOUND );
						}
						else if( entnum != MAX_PLAYERS ) playSound( samples, DAMAGE_SOUND + rand() % 3 );
					} else if( !ents[entnum].hp ) playSound( samples, BULLET_SOUND + rand() % 5);
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
		printf("Usage : %s <host_ip> <port>\n", argv[0]);
		return 1;
	}

	uint32_t level[NUM_PIXELS];
	uint32_t pix = 0xffffffff;
	srand(time(NULL));

	/* Combine address and port to pass it to datain thread */
	int i;
	char *full_addr = malloc( sizeof( argv[1] ) + sizeof( argv[2] ) + 1 );

	for( i = 0; argv[1][i]; i++ ) full_addr[i] = argv[1][i];
	full_addr[i] = ':';
	for( int j = 0; argv[2][j]; j++ ) full_addr[j + i + 1] = argv[2][j];


	SDL_Init( SDL_INIT_VIDEO | SDL_INIT_TIMER );
	Mix_Init( MIX_INIT_OGG );
	Mix_OpenAudio( 44100, AUDIO_S16LSB, 2, 1024 );
	Mix_AllocateChannels( 10 );

	Mix_Music * music = Mix_LoadMUS("sfx/ShooterMusic.ogg");


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


	/* Put them here to have less calculation on each step */
	const size_t pixW = LEVEL_WIDTH * sizeof ( uint32_t );
	const int X1 = (SCREEN_WIDTH - LEVEL_WIDTH*PIX_SIZE)/2, X2 = (SCREEN_WIDTH - LEVEL_WIDTH*PIX_SIZE)/2 - PIX_SIZE, X3 = (SCREEN_WIDTH - LEVEL_WIDTH*PIX_SIZE)/2 + LEVEL_WIDTH*PIX_SIZE;
	const int Y1 = (SCREEN_HEIGHT - LEVEL_HEIGHT*PIX_SIZE)/2, Y2 = (SCREEN_HEIGHT - LEVEL_HEIGHT*PIX_SIZE)/2 - PIX_SIZE, Y3 = (SCREEN_HEIGHT - LEVEL_HEIGHT*PIX_SIZE)/2 + LEVEL_HEIGHT*PIX_SIZE;
	const int W1 = LEVEL_WIDTH*PIX_SIZE, W2 = LEVEL_WIDTH*PIX_SIZE + PIX_SIZE*2;
	const int H1 = LEVEL_HEIGHT*PIX_SIZE;
	
	const int loadX1 = SCREEN_WIDTH/2 - PIX_SIZE*9, loadX2 = SCREEN_WIDTH/2 - PIX_SIZE, loadX3 = SCREEN_WIDTH/2 + PIX_SIZE*7;
	const int loadY = SCREEN_HEIGHT/2 - PIX_SIZE;
	const int loadW = PIX_SIZE*2;

	time_t elapsed, loadT = clock(), terrainT = clock();

	Mix_PlayMusic( music, -1 );

	for(;;) {
		if( quit ) {
			Mix_FadeOutMusic( 20 );
			break;
		}

		elapsed = clock();
		if( (double)(elapsed - terrainT) / CLOCKS_PER_SEC >= 0.01 ) {
			terrainT = elapsed;
			terrain[rand() % NUM_PIXELS] = ( rand() % 50 ) | (( 175 - rand() % 50 ) << 8) | (( rand() % 25 ) << 16) | (255 << 24);
		}

		for( int i = 0; i < NUM_PIXELS; i++ )
			level[i] = terrain[i];

		for( int i = 0; i < MAX_ENTITIES; i++ )
			if( ents[i].hp )
				level[(ents[i].x + ents[i].y*LEVEL_WIDTH)] = ( i > MAX_PLAYERS ?
															 ( 55 + ents[i].hp/2 ) | ( ( 175 + (ents[i].hp*2)/3 )  << 8 ) | ( ( 205 + ents[i].hp/2 ) << 16 ) | ( 255 << 24 ) :	//bullets color
															 ( i == MAX_PLAYERS ?
															 ( rand() % 256 ) | ( ( rand() % 256 ) << 8 ) | ( ( rand() % 256 ) << 16 ) | ( 255 << 24 )	:	//shotgun color
															 ( ents[i].hp*25 ) | ( ( ents[i].hp*25 ) << 8 ) | ( 255 << 16 ) | ( 255 << 24 ) ) );		//players color


		if( events[0] || events[1] ) {
			sendto( server.fd, events, 2, 0, (struct sockaddr *)&sv_addr, sv_addr_len );
			memset( events, 0, 2 );
		}


		SDL_UpdateTexture( texture, NULL, level, pixW );
		textureUpdate( renderer, texture, X1, Y1, W1, H1 );

		if( !connected ) { /* Loading screen before connection */
			elapsed = clock();
			double diff = (double)( elapsed - loadT ) / CLOCKS_PER_SEC;
			if( diff >= 0 && diff <= 3 ) textureUpdate( renderer, white, loadX1, loadY, loadW, loadW );
			if( diff >= 1 && diff <= 4 ) textureUpdate( renderer, white, loadX2, loadY, loadW, loadW );
			if( diff >= 2 && diff <= 5 ) textureUpdate( renderer, white, loadX3, loadY, loadW, loadW );
			if( diff >= 6 ) loadT = elapsed;
		}

		SDL_RenderPresent( renderer );
	}

	/* wait for everything to quit to avoid dumb SDL errors about program exitting while SDL actions are being performed */
	Mix_FreeMusic(music);
	SDL_Delay( 20 );
	SDL_DestroyWindow(win); /* Destroy window */
	Mix_Quit(); 			/* Quit SDL_mixer */
	SDL_Quit();				/* Quit SDL	      */
	printf("\n-------------\n Client quit\n-------------\n");
	return 0;
}