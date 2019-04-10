#include "menu.h"


void error( const char *error ) {
	printf("%s\n", error);
	exit(1);
}

void printrect(const SDL_Rect *rect) {
	printf("%d %d %d %d\n", rect->x, rect->y, rect->w, rect->h);
}

void textureUpdate( SDL_Renderer *r, SDL_Texture *t, int x, int y, int w, int h ) {
	SDL_Rect rect = { x, y, w, h };
	SDL_RenderCopy( r, t, NULL, &rect );
}



SDL_Surface *CreateButton( const char *text, TTF_Font *fnt ) {
	SDL_Surface *button = SDL_CreateRGBSurface( 0, BUTTON_WIDTH, BUTTON_HEIGHT, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000 );
	SDL_Surface *button2 = SDL_CreateRGBSurface( 0, BUTTON_WIDTH/2, BUTTON_HEIGHT/2, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000 );
	SDL_FillRect( button, NULL, SDL_MapRGB(button->format, 75, 75, 75) );
	SDL_FillRect( button2, NULL, SDL_MapRGB(button->format, 0, 0, 75) );
	SDL_Color white = { 255, 255, 255 };
	SDL_Surface *label = TTF_RenderText_Solid( fnt, text, white );
	printrect(&(label->clip_rect));
	printrect(&(button->clip_rect));
	if(!label)
		error("no label");
	SDL_BlitSurface( label, NULL, button, NULL );
	SDL_BlitSurface( button2, NULL, button, NULL );
	SDL_FreeSurface( label );
	SDL_FreeSurface( button2 );
	return button;
}




void menuMain( SDL_Renderer *renderer ) {
	uint32_t background[BACKGROUND_PIX];

	if(TTF_Init() == -1)
		error("WOW IT'S NOT INITTTTTTED");
	SDL_Delay( 20 );

	TTF_Font *fnt_title = TTF_OpenFont("fnt/FreeSansBold.ttf", 32);
	TTF_Font *fnt_button = TTF_OpenFont("fnt/FreeSans.ttf", 60);

	if(!fnt_button)
		error("no font");

	SDL_Surface *button = CreateButton( "yomanwhatsupngga", fnt_button );
	SDL_Texture *button_texture = SDL_CreateTexture( renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, BUTTON_WIDTH, BUTTON_HEIGHT );
	SDL_Texture *bg_texture = SDL_CreateTexture( renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, BACKGROUND_PIX, 1 );
	SDL_UpdateTexture( button_texture, NULL, button->pixels, sizeof( button->pixels ) );

	printf("%s\n", TTF_GetError());

	const size_t bg_pix = BACKGROUND_PIX * sizeof( uint32_t );
	const int X = SCREEN_WIDTH/8;
	SDL_Event e;
	clock_t t1 = clock(), t2;

	for( int i = 0; i < BACKGROUND_PIX; i++ )
		background[i] = ( 75 + rand() % 100 ) | (( 75 + rand() % 100 ) << 8) | (( 75 + rand() % 100 ) << 16) | (255 << 24);

	for(;;) {
		t2 = clock();
		if( (double)(t2 - t1) / CLOCKS_PER_SEC >= 0.025 ) {
			t1 = t2;
			background[rand() % BACKGROUND_PIX] = ( 75 + rand() % 100 ) | (( 75 + rand() % 100 ) << 8) | (( 75 + rand() % 100 ) << 16) | (255 << 24);
			for( int i = 0; i < BACKGROUND_PIX; i++ )
				background[i] -= 1 | ( 1 << 8 )  | ( 1 << 16 );
		}


		if ( SDL_PollEvent( &e ) ) {
			if( e.type == SDL_QUIT )	exit(1);		//handle quitting program
		}

		SDL_UpdateTexture( bg_texture, NULL, background, bg_pix );
		textureUpdate( renderer, bg_texture, 0, 0, SCREEN_WIDTH*2 + cos((double)t2 / CLOCKS_PER_SEC)*SCREEN_WIDTH, SCREEN_HEIGHT );
		textureUpdate( renderer, button_texture, X, 0, BUTTON_WIDTH, BUTTON_HEIGHT );
		SDL_RenderPresent( renderer );
	}

}