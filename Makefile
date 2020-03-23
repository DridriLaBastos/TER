CXXSRC = $(wildcard *.cpp)
CXXHEADER = $(wildcard *.hpp)
EXEC = $(CXXSRC:.cpp=.out)

.PHONY: clean

all: $(EXEC)

%.out : %.cpp $(CXXHEADER)
	$(CXX) $(CPPFLAGS) --std=c++11 -W -Wextra -O3 $< -o $@

clean:
	rm $(EXEC)