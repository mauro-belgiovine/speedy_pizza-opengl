# Project: speedyPizza

CPP  = g++
CC   = gcc
BIN  = speedyPizza

OBJ  = main.o car.o mesh.o
LINKOBJ  = main.o car.o mesh.o
LIBS = -L/usr/X11R6/lib -lX11 -lSDL2 -lSDL2_ttf -lSDL2_image -lglut -lGLU -lGL -lm
INCS = -I. -I/usr/X11R6/include -I/usr/include
CXXINCS=-I/opt/local/include
CXXFLAGS = -g $(CXXINCS) $(INCS)
#CXXFLAGS = $(CXXINCS) $(INCS)

CFLAGS = $(INCS)
RM = rm -f

all: $(BIN)


clean:
	${RM} $(OBJ) $(BIN)

$(BIN): $(OBJ)
	$(CPP) $(LINKOBJ) -o $(BIN) $(LIBS)

main.o: main.cpp
	$(CPP) -c main.cpp -o main.o $(CXXFLAGS)

car.o: car.cpp
	$(CPP) -c car.cpp -o car.o $(CXXFLAGS)

mesh.o: mesh.cpp
	$(CPP) -c mesh.cpp -o mesh.o $(CXXFLAGS)
