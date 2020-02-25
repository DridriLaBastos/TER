CXXSRC = $(wildcard *.cpp)
EXEC = wlmc.out

.PHONY: deb

$(EXEC) : $(CXXSRC) $(wildcard *.hpp)
	$(CXX) $(CPPFLAGS) --std=c++11 -W -Wextra -O3 $(CXXSRC) -o $@

deb : $(EXEC)
	lldb $ ^