#include <cstdio>
#include <cstdint>
#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>

#include "chip8.h"

// SDL_PollEvent() -> getch()
// SDL_WaitEvent() -> getchar()

// Initialize the window struct
bool Init_display(Display& disp, int x, int y, int w, int h){
	if(SDL_Init(SDL_INIT_EVERYTHING)){ return false; }
	disp.win = SDL_CreateWindow  ("test window", x, y, w, h, SDL_WINDOW_SHOWN);	if(!disp.win)  { return false; }
	disp.ren = SDL_CreateRenderer(disp.win, -1, SDL_RENDERER_ACCELERATED); 		if(!disp.win)  { return false; }

	// Get pixel format data of window
	uint32_t format_32 = SDL_GetWindowPixelFormat(disp.win);
	disp.format        = SDL_AllocFormat( format_32 );

	// Create surface and fill with white
	disp.sur = SDL_CreateRGBSurface(0, 64, 32, 32, 0x00, 0x00, 0x00, 0x00);
	SDL_FillRect(disp.sur, NULL, SDL_MapRGB(disp.format, 0xFF, 0xFF, 0xFF));

	// Set default draw color to white and convert surface to texture
	SDL_SetRenderDrawColor(disp.ren, 0xFF, 0xFF, 0xFF, 0xFF);
	disp.tex = SDL_CreateTextureFromSurface(disp.ren, disp.sur);

	return true;
}

// Plots pixel to surface
inline void Put_Pixel(SDL_Surface * sur, int x, int y, uint32_t color){
	uint8_t * pixel = (uint8_t *) sur->pixels;
	pixel += (y * sur->pitch) + (x * sizeof(uint32_t));
	*((uint32_t*)pixel) = color;
}

// Write the contents of the GFX array to the surface and render it to the screen
bool Render(Display& disp){
	// Interpret GFX array as pixels to write
	SDL_Rect scale; 
	scale.x = 0; scale.y = 0; scale.w = W; scale.h = H;
	uint32_t color;
	
	// Lock the surface for writing
	SDL_LockSurface(disp.sur);

	for(int y = 0; y < 32; y++){
		for(int x = 0; x < 64; x++){
			// Check if value in array is 0 and set corresponding pixel to black, else white.
			color =  (screen[x][y] == 0) ? SDL_MapRGBA(disp.format, 0x00, 0x00, 0x00, 0xFF) : SDL_MapRGBA(disp.format, 0xFF, 0xFF, 0xFF, 0xFF);
			Put_Pixel(disp.sur, x, y, color);
		}
	}

	// Unlock surface and update texture
	SDL_UnlockSurface(disp.sur);
	disp.tex = SDL_CreateTextureFromSurface(disp.ren, disp.sur);

	// Render to screen
	SDL_RenderClear  (disp.ren);
	SDL_RenderCopyEx (disp.ren, disp.tex, NULL, &scale, 0, NULL, SDL_FLIP_NONE);
	SDL_RenderPresent(disp.ren);

	return true;
}

// SDL_GetKeyboardState returns the entire keyboard
int OnEvent(Display& disp){
	while(SDL_PollEvent(&disp.ev)){
		switch (disp.ev.type){
		case SDL_KEYDOWN:
			break;
		case SDL_QUIT:
			return 69;
		/*add more events here UWU*/
		default: continue;
		}
	}
}

// Free values in display struct 
void Free_display(Display& disp){
	SDL_FreeSurface    (disp.sur);
	SDL_DestroyRenderer(disp.ren);
	SDL_DestroyWindow  (disp.win);
	SDL_FreeFormat     (disp.format);
	SDL_DestroyTexture (disp.tex); 
}