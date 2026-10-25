# Zephyr Industries Load Balancer
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2
SRCS     := $(wildcard *.cpp)
OBJS     := $(SRCS:.cpp=.o)
HDRS     := $(wildcard *.h)
TARGET   := loadbalancer

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS)

%.o: %.cpp $(HDRS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Generate HTML documentation into docs/html (requires doxygen)
docs:
	doxygen Doxyfile

# Demo runs: 20 servers for 100000 cycles
run-verbose: $(TARGET)
	./$(TARGET) -s 20 -t 100000 -v -l run_verbose.log

run-quiet: $(TARGET)
	./$(TARGET) -s 20 -t 100000 -q -l run_quiet.log

run-switch: $(TARGET)
	./$(TARGET) -s 20 -t 100000 -v --switch -l run_switch.log

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all docs run-verbose run-quiet run-switch clean
