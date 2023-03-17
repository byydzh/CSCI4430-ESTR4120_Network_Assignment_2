## CSCI 4430 Advanced Makefile

# How to use this Makefile...
###################
###################
##               ##
##  $ make help  ##
##               ##
###################
###################

CXX = g++
# TODO For C++ only.
CXXFLAGS = -Wconversion -Wall -Werror -Wextra -pedantic 

all: miProxy

# TODO Modify source file name for your project.
# For C only.
# SOURCES = iPerfer.c
# For C++ only.
SOURCES = miProxy.cpp
iPerfer: $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o miProxy

clean:
	rm -rf miProxy *.dSYM

.PHONY: clean
