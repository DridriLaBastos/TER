export CXXFLAGS = --std=c++11 -W -Wextra
CXXSRC = $(wildcard *.cpp)
CXXHEADER = $(wildcard *.hpp)

all: wlmc ggen

wlmc: wlmc.cpp $(CXXHEADER)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -O3 $< -o $@

ggen: ggen.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -O3 $< -o $@