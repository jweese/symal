.PHONY: clean

all: symal

clean: 
	$(RM) *.o symal

cmd.o: cmd.c cmd.h
	$(CC) -O3 -c -o cmd.o cmd.c

symal: symal.cpp cmd.o
	$(CXX) -O3 -o $@ $(@).cpp cmd.o
