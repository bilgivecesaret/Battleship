CC = gcc
CFLAGS = -lm -lncurses
SRC = Battleship_Game_SHM.c
OUT = battleship

all: $(OUT)

$(OUT): $(SRC)
	$(CC) $(SRC) $(CFLAGS) -o $(OUT)

clean:
	rm -f $(OUT)

rebuild: clean all
