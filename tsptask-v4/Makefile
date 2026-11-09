CXX ?= g++
STD = -std=c++20
OPT = -O3
CXXFLAGS = $(STD) $(OPT) -Wall -Wextra

TARGETS = tsp tspprint intvecsort

.PHONY: all clean debug

all: $(TARGETS)

# make debug: unoptimised build with debug info (rebuilds everything)
debug:
	$(MAKE) clean
	$(MAKE) OPT="-O0 -g"

# -MMD/-MP write a .d file per program, so editing a header rebuilds it
%: %.cpp
	$(CXX) $(CXXFLAGS) -MMD -MP -o $@ $<

-include $(TARGETS:=.d)

clean:
	rm -f $(TARGETS) $(TARGETS:=.d)
