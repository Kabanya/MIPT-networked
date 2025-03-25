CXX ?= g++

bitstream.o: bitstream.cpp
	$(CXX) -c bitstream.cpp -o bitstream.o

bistream: bitstream.o 
	$(CXX) bitstream.o -o bistream


all: bistream 

clean:
	$(RM) bitstream.o bistream
