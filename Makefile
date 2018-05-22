common_flags = -O3
CFLAGS = $(common_flags)
CXXFLAGS = $(common_flags)

.PHONY: clean

all: symal

clean: 
	$(RM) *.o symal

symal: symal.cpp cmd.o
	$(CXX) $(CXXFLAGS) -o $@ $^
