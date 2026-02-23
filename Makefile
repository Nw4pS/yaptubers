# 1. Target (l'eseguibile finale) e Source (il file sorgente)
TARGET = yaptubers
SRC = main.c

CC = gcc

CFLAGS = -Wall -Wextra -g $(shell sdl2-config --cflags)

LDFLAGS = $(shell sdl2-config --libs) -lSDL2_image -lm

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)
	@echo "Compilazione completata! Esegui con ./$(TARGET)"

clean:
	rm -f $(TARGET)
