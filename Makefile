CXX = g++
CPPFLAGS = -Iinclude
CXXFLAGS = -std=c++20 -Wall -Wextra -O3 -pthread

TARGET = tun
SRCS = $(wildcard src/*.cpp)
OBJS_DIR = obj
OBJS = $(patsubst src/%.cpp,$(OBJS_DIR)/%.o,$(SRCS))
DEPS = $(OBJS:.o=.d)

.PHONY: all clean
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

$(OBJS_DIR)/%.o: src/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

-include $(DEPS)

clean:
	rm -rf $(OBJS_DIR) $(TARGET)
