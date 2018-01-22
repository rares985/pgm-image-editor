CC = mpic++
CFLAGS = -std=c++11 -Wall -Wextra
OBJ_LIST = utils.o mpi_wrappers.o filter.o pgm_image.o main.o
EXE_NAME = filtru

all: build

build: $(EXE_NAME)

$(EXE_NAME): $(OBJ_LIST)
	$(CC) $(CFLAGS) $(OBJ_LIST) -o $(EXE_NAME)

main.o: main.cc
	$(CC) $(CFLAGS) $^ -c

pgm_image.o: pgm_image.cc
	$(CC) $(CFLAGS) $^ -c

filter.o: filter.cc
	$(CC) $(CFLAGS) $^ -c

mpi_wrappers.o: mpi_wrappers.cc
	$(CC) $(CFLAGS) $^ -c

utils.o: utils.cc
	$(CC) $(CFLAGS) $^ -c
	
clean: 
	rm filtru
