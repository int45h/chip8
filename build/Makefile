CC=g++

CFLAGS=-lSDL2 -lportaudio
OPTIMIZE=-O3

OBJ=chip8_v2.o render.o

DIR=../src

%.o: $(DIR)/%.cc
	$(CC) -c -o $@ $< $(CFLAGS)

chip8: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(OPTIMIZE)

.PHONY: clean

clean:
	rm -rf *.o
	rm -rf chip8