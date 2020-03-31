CXXSRC = $(wildcard *.cpp)
CXXHEADER = $(wildcard *.hpp)
EXEC = wlmc.out

.PHONY: clean

all: $(EXEC)

$(EXEC) : $(CXXSRC) $(CXXHEADER)
	$(CXX) $(CPPFLAGS) --std=c++11 -W -Wextra -O3 $(CXXSRC) -o $@

clean:
	rm $(EXEC)