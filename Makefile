CXXSRC = $(wildcard *.cpp)
CXXHEADER = $(wildcard *.hpp)
EXEC = wlmc.out

.PHONY: deb

$(EXEC) : $(CXXSRC) $(CXXHEADER)
	$(CXX) $(CPPFLAGS) --std=c++11 -W -Wextra -O3 $(CXXSRC) -o $@

deb : $(EXEC)
	lldb $ ^