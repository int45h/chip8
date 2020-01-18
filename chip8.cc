#include <cstdio>
#include <cstdint>
#include <ctime>
#include <cstring>
//#include <cstdlib>

#include <ncurses.h>

#include <chrono>
#include <thread>

#define MEMORY 4096
#define STACK  16

#define COLS 64
#define ROWS 32

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

uint8_t chars[16][5] = {
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

enum timeUnit { Milliseconds, Microseconds };

void wait(uint16_t time, timeUnit unit = Milliseconds){
	switch(unit){
	case 0:	std::this_thread::sleep_for(std::chrono::milliseconds(time)); break;
	case 1:	std::this_thread::sleep_for(std::chrono::microseconds(time)); break;
	}
}

/*
 * CHIP-8: 4096 8-bit memory cells, programs start at mem[512]
 *
 *
 * */

enum Ex{FileNotFound, BadAlloc, OutOfBounds};
const char * Ex_str[3] = { "FileNotFound", "BadAlloc", "OutOfBounds" };
uint8_t  mem[MEMORY], V[16];
uint16_t stack[STACK];
uint16_t I = 0, sp = 0, PC = 0x0200;

bool    screen[COLS][ROWS];
uint8_t DT = 0, ST = 0;

uint16_t c8_clock;

FILE * program;

bool jump = false;

// TO-DO: implement timers
void delay(){ DT--; wait(6); }
void sound(){ while(ST > 0){ST--;wait(6);/*beep();*/} }

void init(const char * filename, uint16_t clock_time = 0){
	program = fopen(filename, "rb");
	if(program == NULL){ throw FileNotFound; }

	srand(time(NULL));
	c8_clock = clock_time;

	memset(screen, 0, COLS*ROWS*sizeof(bool));

	// TO-DO: change to memset()s
	for(auto i = 0; i < 16; i++)	{ V[i] = 0; }
	for(auto i = 0; i < STACK; i++) { stack[STACK] = 0; }
	for(auto i = 0; i < MEMORY; i++){ mem[MEMORY]  = 0; }

	memcpy(&mem[0x0], &chars, 16*5*sizeof(uint8_t));	

	fseek(program, 0, SEEK_END);

	uint16_t size = ftell(program);
	rewind(program);
	
	fread(&mem[0x200], 1, size, program);
	//printf("size:\t%d\n", size);

	fclose(program);
}

uint8_t rand_8(){
	return (rand() & 0xFF);
}

uint8_t get_key(uint8_t key){
	if(key == ERR){ return 0x10; }
	if(key >= '1' && key <= '3'){ return (key - 48); }
	else{ switch(key){
		case '4': return (key = 0xC);
		case 'q': return (key = 0x4);       	
		case 'w': return (key = 0x5);       	
		case 'e': return (key = 0x6);       	
		case 'r': return (key = 0xD);       	
		case 'a': return (key = 0x7);       	
		case 's': return (key = 0x8);       	
		case 'd': return (key = 0x9);       	
		case 'f': return (key = 0xE);       	
		case 'z': return (key = 0xA);       	
		case 'x': return (key = 0x0);       	
		case 'c': return (key = 0xB);       	
		case 'v': return (key = 0xF);
	}}
}

void interpret(){
	uint16_t op, diff = 0;
	while(PC < 4096){
	jump = false;
	op   = ((uint16_t)mem[PC] << 8) + mem[PC+1];

	//debug
	mvprintw(0, COLS+1, "Opcode: %04X", op);

	switch(opcode(op) >> 12){
		case 0x0: 
		switch(kk(op))	{	case 0xE0: memset(screen, 0, COLS*ROWS*sizeof(bool)); clear(); break;
					case 0xEE: PC = stack[sp]; sp--; continue; 
					default:   break;}
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
				case 0x6: V[0XF] = (Vx & 1); Vx = Vx >> 1; 		break;	
				case 0x7: V[0xF] = (Vy > Vx) ? 1 : 0; Vx -= Vy;		break;	
				case 0xE: V[0xF] = (Vx & 0x8) >> 7; Vx = Vx << 1;	break;
			  }break;
		case 0x9: if(Vx != Vy){ PC+=2; }		break;
		case 0xA: I  = nnn(op);				break;
		case 0xB: PC = nnn(op) + V[0x0]; jump = true;	break; //Note: 
		case 0xC: Vx = rand_8() & kk(op); 		break;
		case 0xD: {
			uint8_t row = mem[I], n = nibble(op);
			bool xor_result, pixel;
			V[0xF] = 0;
			for(auto yl = 0; yl < n; yl++){
				row = mem[I + yl];
					
				for(auto xl = 0; xl < 8; xl++){
					pixel	   =  screen[(Vx + xl) & 0x3F][(Vy + yl) & 0x1F];
					xor_result = (screen[(Vx + xl) & 0x3F][(Vy + yl) & 0x1F] ^= (row & (0x80 >> xl)) >> 7-xl);	
					if(xor_result == 0 && pixel == 1){ V[0xF] = 1; }
				}
			}
		}break;

		case 0xE: switch(kk(op)){ 	case 0x9E: {uint8_t key; if((key = get_key(getch())) == Vx) PC+=2; }break;
						case 0xA1: {uint8_t key; if((key = get_key(getch())) != Vx) PC+=2; }break; } 
		break;
		case 0xF: switch(kk(op)){
				case 0x07: Vx = DT;	 			break;
				case 0x0A: Vx = (uint8_t) (get_key(getchar())); break;
				case 0x15: DT = Vx;				break;
				case 0x18: ST = Vx; sound();			break;
				case 0x1E: I += Vx;				break;
				case 0x29: I =  Vx*5;				break;
				case 0x33: {char * Vx_BCD = (char*)malloc(3); sprintf(Vx_BCD, "%03d", Vx); 
					  mem[I] = Vx_BCD[0] - 48; mem[I+1] = Vx_BCD[1] - 48; mem[I+2] = Vx_BCD[2] - 48; free(Vx_BCD); break;}
									   
				//	   mem[I] = Vx/100; mem[I+1] = (Vx / 10) % 100; mem[I+2] = Vx % 10; break;
				case 0x55: diff = xi; memcpy( &mem[I], &V[0], (diff+1)*sizeof(uint8_t)); break;
				case 0x65: diff = xi; memcpy( &V[0], &mem[I], (diff+1)*sizeof(uint8_t)); break;}
		break;
		default: mvprintw(1, COLS+1, "error: unknown opcode %04X", op);
		}

	if(jump == false){PC+=2;}
	if(DT > 0){ delay(); } else { wait(c8_clock, (timeUnit)1); } 
	//wait(20);	
	//draws screen to terminal
	for(int xp = 0; xp < COLS; xp++){
		for(int yp = 0; yp < ROWS; yp++){
			char ch = (screen[xp][yp] == 1) ? '#' : ' ';
			mvaddch(yp, xp, ch);
			}
		}
	refresh();
	}
}	

int main(int argc, char ** argv){
	try{
		if(argc == 2){ init(argv[1]); }else if(argc == 3){ init(argv[1], std::stoi(argv[2])); }
		initscr();
		noecho();	
		nodelay(stdscr, TRUE);

		interpret();
	}catch(Ex error){
		fprintf(stderr, "error: %s exception thrown. exiting.\n", Ex_str[error]);
		return -1;
	}

	return 0;
}
