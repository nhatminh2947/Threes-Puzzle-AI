all:
	g++ -std=c++11 -O3 -g -Wall -fmessage-length=0 -o threes Threes.cpp
clean:
	rm 2048
