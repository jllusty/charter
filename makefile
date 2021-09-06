# build config
CC = g++
C_FLAGS = -std=c++17 -g -Wall

# libs
#  (*) SDL
SDL_INC = /usr/include/SDL2 -Dmain=SDL_main
SDL_LIBDIR = /usr/lib
SDL_LIBS = -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -mwindows
#  (*) lua
LUA_ROOT = /opt/lua-5.4.3
LUA_INC = $(LUA_ROOT)/include
LUA_LIBDIR = $(LUA_ROOT)/lib
LUA_LIBS = -llua
#  (*) tinyxml2
XML_ROOT = /opt/tinyxml2
XML_INC = $(XML_ROOT)
XML_SRC = $(XML_ROOT)/tinyxml2.cpp

# aggregate build
INC_FLAGS = -I$(SDL_INC) -I$(LUA_INC) -I$(XML_INC) -I./include
LD_FLAGS = -L$(SDL_LIBDIR) -L$(LUA_LIBDIR)
L_FLAGS = $(SDL_LIBS) $(LUA_LIBS)
SRC = $(XML_SRC) source/tinytmx.cpp source/logger.cpp source/timer.cpp source/context.cpp source/loader.cpp source/systems.cpp source/main.cpp 

# target
embark:
	$(CC) $(C_FLAGS) $(INC_FLAGS) $(LD_FLAGS) $(SRC) -o game $(L_FLAGS)
