.PHONY: clean

all: symal

clean: 
	$(RM) *.o symal

symal: symal.cpp cmd.o
	$(CXX) -O3 -o $@ $^
