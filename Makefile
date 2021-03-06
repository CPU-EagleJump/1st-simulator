CXX := g++
CXXFLAGS := -Wall -Wno-strict-aliasing -O2 -std=c++1y

TARGET := sim
OBJS := $(patsubst %.cpp, %.o, $(wildcard *.cpp))


$(TARGET): $(OBJS) common.h
	$(CXX) -o $@ $(OBJS)

.PHONY: test/%
test/%: 1st-assembler/test/%.exp.zoi $(TARGET)
	./$(TARGET) $<


.PHONY: clean
clean:
	rm -f $(OBJS)
	rm -f $(TARGET)

