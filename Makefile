#################################################################
##
## FILE:	Makefile
## PROJECT:	CS 3251 Project 2
## DESCRIPTION: Compile Project 2
##
#################################################################

CXX = g++
CXXFLAGS = -std=c++0x

OS := $(shell uname -s)

# Extra LDFLAGS if Solaris
ifeq ($(OS), SunOS)
	LDFLAGS=-lsocket -lnsl
    endif

all: client server 

client: client.cpp
	$(CXX) client.cpp -o client ${CXXFLAGS}

server: server.cpp
	$(CXX) server.cpp -o server ${CXXFLAGS}

clean:
	    rm -f client server *.o

