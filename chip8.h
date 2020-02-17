#pragma once
#include <portaudio.h>
#include <SDL2/SDL.h>

#define SAMPLE_RATE 44100

#define W 640
#define H 320 

#define X_POS SDL_WINDOWPOS_CENTERED
#define Y_POS SDL_WINDOWPOS_CENTERED

#define COLS 64
#define ROWS 32

// CHIP-8 GFX buffer
inline bool screen[COLS][ROWS];

// Flags for either CHIP8 or SUPER-CHIP8
enum Type{CHIP_8,SUPER};

// Struct for window
struct Display{	SDL_Window  	* win;
				SDL_Texture 	* tex;
				SDL_Renderer	* ren; 
				SDL_Surface     * sur;
				SDL_PixelFormat * format;
				SDL_Event		  ev; };

// Prototypes for display initialization, rendering and freeing display
bool Init_display	(Display&, int x, int y, int w, int h); 
bool Render			(Display&);
void Free_display	(Display&);

// Prototype for event handler function
int  OnEvent		(Display&);

// Prototype for getting key from map
uint8_t get_key		(SDL_Scancode&);

inline void Beep(){
	/*beep*/
}



/* Cursed PortAudio Code

// Typedefs for timeinfor and callback flags
typedef PaStreamCallbackTimeInfo timeInfo;
typedef PaStreamCallbackFlags statusFlags;

// Audio channels and stream
struct Shift{ float left, right; };
inline PaStream * stream;

//TO-DO: write as template to avoid nasty "void*"
// Callback function, generates sawtooth wave
inline int pa_callback(const void* input, void* output, uint64_t framecount, const timeInfo* info, statusFlags flags, void* userdata){	
	Shift * data = (Shift *) userdata;
	float * out = (float *)	output;

	// sawtooth 
	for(auto i = 0; i < framecount; i++){
		*out++ = data->left;
		*out++ = data->right;
		
		data->left += 0.01f;
		if(data->left >= 1.0f){ data->left -= 2.0f; }
		data->right += 0.01f;
		if(data->right >= 1.0f){ data->right -= 2.0f; }
	}
	return 0;
}

// Beep function for maximal beepage
inline void Beep(){
	Shift data;

	if(Pa_Initialize() != paNoError){ fprintf(stderr, "error: portaudio couldn't be started. exiting\n"); return; }
	Pa_OpenDefaultStream( &stream,0,2,paFloat32,SAMPLE_RATE,256,pa_callback,&data );
	
	Pa_StartStream(stream);
	Pa_Sleep(1);

	Pa_StopStream(stream);
}
*/