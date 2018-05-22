common_flags = -O3 -Wall -Wextra -pedantic -Werror
CFLAGS = $(common_flags) -std=c11
CXXFLAGS = $(common_flags) -std=c++17

.PHONY: clean

all: symal

clean: 
	$(RM) *.o symal

symal: symal.cpp cmd.o
	$(CXX) $(CXXFLAGS) -o $@ $^
