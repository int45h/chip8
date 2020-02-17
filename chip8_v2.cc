#include <cstdio>
#include <cstdint>
#include <ctime>
#include <cstring>

#include <chrono>
#include <thread>
#include <unordered_map>

#include <SDL2/SDL.h>
#include <portaudio.h>

#include "chip8.h"

#define MEMORY 4096
#define STACK  16

#define DELAY 16667

#define hi(x)	 ( x & 0xF0 )
#define lo(x)	 ( x & 0x0F )

#define nnn(x)	 ( x & 0x0FFF ) 
#define nibble(x)( x & 0x000F ) 
#define x(x)	 ( x & 0x0F00 ) 
#define y(x)	 ( x & 0x00F0 )
#define kk(x)	 ( x & 0x00FF )
#define opcode(x)( x & 0xF000 )

#define Vx V[x(op) >> 8]
#define Vy V[y(op) >> 4]

#define xi  (x(op) >> 8)
#define yi  (y(op) >> 4)

// CHIP-8: 500Hz /sec >> 2000 us
// SUPER:  1000Hz/sec >> 1000 us

// Delay + Sound: 60Hz/sec >> 16667 us

void    wait	(uint16_t us);
void	init_key();
uint8_t rand_8	();
uint8_t get_key	(SDL_Scancode& sc);

// Union representation of memory
union chip8_mem{
	uint8_t raw[MEMORY];
	uint8_t font[16][5] = {
		{0xF0, 0x90, 0x90, 0x90, 0xF0},
		{0x20, 0x60, 0x20, 0x20, 0x70},
		{0xF0, 0x10, 0xF0, 0x80, 0xF0},
		{0xF0, 0x10, 0xF0, 0x10, 0xF0},
		{0x90, 0x90, 0xF0, 0x10, 0x10},
		{0xF0, 0x80, 0xF0, 0x10, 0xF0},
		{0xF0, 0x80, 0xF0, 0x90, 0xF0},
		{0xF0, 0x10, 0x20, 0x40, 0x40},
		{0xF0, 0x90, 0xF0, 0x90, 0xF0},
		{0xF0, 0x90, 0xF0, 0x10, 0xF0},
		{0xF0, 0x90, 0xF0, 0x90, 0x90},
		{0xE0, 0x90, 0xE0, 0x90, 0xE0},
		{0xF0, 0x80, 0x80, 0x80, 0xF0},
		{0xE0, 0x90, 0x90, 0x90, 0xE0},
		{0xF0, 0x80, 0xF0, 0x80, 0xF0},
		{0xF0, 0x80, 0xF0, 0x80, 0x80},
	};
};

// Map of keys
std::unordered_map<SDL_Scancode, uint8_t> keys;
// uint8_t * keystate = nullptr;

class chip8_emu{
public:
	chip8_emu(Type type = CHIP_8){
		// Zero everything out

		memset(V, 				  0x00,		16*sizeof(uint8_t));
		memset(stack, 			  0x0000,	16*sizeof(uint16_t));
		memset(screen, 			  0,		COLS*ROWS*sizeof(bool));

		// Init display window
		Init_display(disp, X_POS, Y_POS, 640, 320); 
		
		// Start loading stuff
		init_key();

		// Set jump flag to false
		jump = false;

		// Set Timers, instruction pointer, stack pointer, and program counter
		DT = 0; ST = 0; I = 0; sp = 0; PC = 0x0200;
	
		// Set clock time to either 2000 or 1000 microseconds
		clock_time = (type == 0) ? 2000 : 1000;

		// Seed RNG	
		srand(time(NULL));
	}

	~chip8_emu(){
		// Free display struct
		Free_display(disp);

		// Zero out all buffers + registers
		memset(&memory.raw[0x50], 0x00,		MEMORY*sizeof(uint8_t));
		memset(V, 				  0x00,		16*sizeof(uint8_t));
		memset(stack, 			  0x0000,	16*sizeof(uint16_t));
		memset(screen, 			  0,		COLS*ROWS*sizeof(bool));

		SDL_Quit();
	}

	int  open_file(const char * filename);
	void interpret();

private:
	chip8_mem memory;
	Display   disp;
	uint8_t   V[16], DT, ST;
	uint16_t  stack[STACK], I, sp, PC, clock_time;
	bool 	  jump;

	FILE * 	  program;

	void delay(){ DT--; if(DT == 1)				   { wait(DELAY); } }
	void sound(){ while(ST > 0)		{ST--; Beep(); } wait(DELAY); }
};

// Opens a file in read-only mode and loads into CHIP-8 memory
int chip8_emu::open_file(const char * filename){
		// Open file for reading
		program = fopen(filename, "rb");
		if(program == NULL){ return 1; }

		// Gets size of program
		fseek(program, 0, SEEK_END);
		uint16_t size = ftell(program);

		// Read file into memory
		rewind(program);
		fread(&memory.raw[0x200], 1, size, program);
		
		// Close file
		fclose(program);
		return 0;
}

void chip8_emu::interpret(){
	uint16_t op, diff = 0;

	while(PC < 4096){
	jump = false;
	op   = ((uint16_t)memory.raw[PC] << 8) + memory.raw[PC+1];

	switch(opcode(op) >> 12){
		case 0x0: 
		switch(kk(op) >> 4){	case 0xC: /*scroll screen down N lines (N = nibble(op))*/ break;
					case 0xE: if(nibble(op) == 0){ memset(screen, 0, COLS*ROWS*sizeof(bool)); break; }
						else{PC = stack[sp]; sp--; continue;} 
					case 0xF: switch(nibble(op)){ 	case 0xB: /*scroll 4px left */			break;  /*SUPER*/
													case 0xC: /*scroll 4px right */			return; /*SUPER*/
													case 0xD: 								break;  /*SUPER*/
													case 0xE: /*disable extended screen*/	break;  /*SUPER*/
													case 0xF: /*enable extended screen*/ 	break;  /*SUPER*/
 					} break;  /*SUPER*/
					default: break;	}
		break;
		case 0x1: PC = nnn(op); continue;
		case 0x2: sp++; stack[sp] = (PC+2); PC = nnn(op);  continue;
		case 0x3: if(Vx == kk(op))		{ PC+=2; } break;
		case 0x4: if(Vx != kk(op))		{ PC+=2; } break;
		case 0x5: if(Vx == Vy)			{ PC+=2; } break;
		case 0x6: Vx =  kk(op); break;
		case 0x7: Vx += kk(op); break;
		case 0x8: switch(nibble(op)){
				case 0x0: Vx =  Vy; break;
				case 0x1: Vx |= Vy; break;	
				case 0x2: Vx &= Vy; break;	
				case 0x3: Vx ^= Vy; break;	
				case 0x4: {uint16_t add = (Vx + Vy); V[0xF] = (add > 255) ? 1 : 0; Vx = kk(add);}break;	
				case 0x5: V[0xF] = (Vx > Vy) ? 1 : 0; Vx -= Vy;		break;	
				case 0x6: V[0XF] = (Vx & 1); Vx = Vx >> 1; 			break;	
				case 0x7: V[0xF] = (Vy > Vx) ? 1 : 0; Vx -= Vy;		break;	
				case 0xE: V[0xF] = (Vx & 0x8) >> 7; Vx = Vx << 1;	break;
			  }break;
		case 0x9: if(Vx != Vy){ PC+=2; }				break;
		case 0xA: I  = nnn(op);							break;
		case 0xB: PC = nnn(op) + V[0x0]; jump = true;	break;
		case 0xC: Vx = rand_8() & kk(op);				break;
		case 0xD: { //TO-DO: check if n=0 and display extended sprite if true
			uint8_t row = memory.raw[I], n = nibble(op);
			bool xor_result, pixel;
			V[0xF] = 0;
			for(auto yl = 0; yl < n; yl++){
				row = memory.raw[I + yl];
					
				for(auto xl = 0; xl < 8; xl++){
					pixel	   =  screen[(Vx + xl) & 0x3F][(Vy + yl) & 0x1F];
					xor_result = (screen[(Vx + xl) & 0x3F][(Vy + yl) & 0x1F] ^= (row & (0x80 >> xl)) >> 7-xl);	
					if(xor_result == 0 && pixel == 1){ V[0xF] = 1; }
				}
			}
		}break;

		case 0xE: switch(kk(op)){ 	case 0x9E: {uint8_t key; if(get_key(disp.ev.key.keysym.scancode) == Vx) PC+=2; }break;
									case 0xA1: {uint8_t key; if(get_key(disp.ev.key.keysym.scancode) != Vx) PC+=2; }break; } 
		break;
		case 0xF: switch(kk(op)){
				case 0x07: Vx = DT;	 				break;
				case 0x0A: SDL_WaitEvent(&disp.ev);
						   Vx = (disp.ev.type == SDL_KEYDOWN) ? (uint8_t)(get_key(disp.ev.key.keysym.scancode)) : 0x10; break;
				case 0x15: DT = Vx; 				break;
				case 0x18: ST = Vx; sound();		break;
				case 0x1E: I += Vx;					break;
				case 0x29: I =  Vx*5;				break;
				case 0x30: I =  Vx*10; 		 		break; /*SUPER*/
				case 0x33: {char * Vx_BCD   = (char*)malloc(3); sprintf(Vx_BCD, "%03d", Vx); 
					  		memory.raw[I]   = Vx_BCD[0] - 48;
					  		memory.raw[I+1] = Vx_BCD[1] - 48; 
					  		memory.raw[I+2] = Vx_BCD[2] - 48; 
					  		free(Vx_BCD);   		break;}
				case 0x55: diff = xi; memcpy( &memory.raw[I], &V[0], (diff+1)*sizeof(uint8_t)); break;
				case 0x65: diff = xi; memcpy( &V[0], &memory.raw[I], (diff+1)*sizeof(uint8_t)); break;
			  	case 0x75: /*store V0-VX in RPL user flags (X <= 7)*/ break;
			  	case 0x85: /*read  V0-VX in RPL user flags (X <= 7)*/ break;	}
		break;
		default: fprintf(stderr, "error: unknown opcode %04X", op);
		}

	if(OnEvent(disp) == 69){ break;}
	if(jump == false){PC+=2;}
	if(DT > 0){ delay(); } else {  } 
	wait(clock_time);
	//wait(20);	

	Render(disp);
	}
	/* SUPER Chip 8 instructions: 
	00Cn - SCD nibble
	00FB - SCR
	00FC - SCL
	00FD - EXIT
	00FE - LOW
	00FF - HIGH
	Dxy0 - DRW Vx, Vy, 0
	Fx30 - LD HF, Vx
	Fx75 - LD R, Vx
	Fx85 - LD Vx, R	
	*/	
}

// 8-bit PRNG
uint8_t rand_8()	  { return (rand() & 0xFF); }

// Wait Function
void wait(uint16_t us){ std::this_thread::sleep_for(std::chrono::microseconds(us)); }

// Mapping KB keys to hex values lol
void init_key(){
	keys.insert(std::make_pair(SDL_SCANCODE_1,0x1));
	keys.insert(std::make_pair(SDL_SCANCODE_2,0x2));
	keys.insert(std::make_pair(SDL_SCANCODE_3,0x3));
	keys.insert(std::make_pair(SDL_SCANCODE_4,0xC));
	keys.insert(std::make_pair(SDL_SCANCODE_Q,0x4));
	keys.insert(std::make_pair(SDL_SCANCODE_W,0x5));
	keys.insert(std::make_pair(SDL_SCANCODE_E,0x6));
	keys.insert(std::make_pair(SDL_SCANCODE_R,0xD));
	keys.insert(std::make_pair(SDL_SCANCODE_A,0x7));
	keys.insert(std::make_pair(SDL_SCANCODE_S,0x8));
	keys.insert(std::make_pair(SDL_SCANCODE_D,0x9));
	keys.insert(std::make_pair(SDL_SCANCODE_F,0xE));
	keys.insert(std::make_pair(SDL_SCANCODE_Z,0xA));
	keys.insert(std::make_pair(SDL_SCANCODE_X,0x0));
	keys.insert(std::make_pair(SDL_SCANCODE_C,0xB));
	keys.insert(std::make_pair(SDL_SCANCODE_V,0xF));
}

uint8_t get_key(SDL_Scancode& sc){ return keys[sc]; }

int main(int argc, char ** argv){
	// Inits everything and loads program into memory
	chip8_emu c8;
	if(c8.open_file(argv[1])){ fprintf(stderr, "error: file not found. exiting\n"); return EXIT_FAILURE; }

	// Interpret chip8 code UwU
	c8.interpret();

	return 0;
}
