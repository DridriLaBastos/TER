CXXSRC = $(wildcard *.cpp)
CXXHEADER = $(wildcard *.hpp)
EXEC = wlmc.out

.PHONY: clean

all: $(EXEC)

$(EXEC) : $(CXXSRC) $(CXXHEADER)
	$(CXX) $(CPPFLAGS) --std=c++11 -W -Wextra -g $(CXXSRC) -o $@

clean:
	rm $(EXEC)