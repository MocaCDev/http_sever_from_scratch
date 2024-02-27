.PHONY: build
.PHONY: bin/server.o
.PHONY: run
.PHONY: clean

CC=g++
FLAGS=-std=c++20 -fsanitize=leak

build: bin/server.o

run: build
	@./bin/server.o

bin/server.o:
	@$(CC) $(FLAGS) server.cpp -o bin/server.o


clean:
	rm -rf bin/*.o
