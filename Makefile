CXX = g++
CXXFLAGS = -Wall -std=c++17
LIBS = -lglfw -lvulkan -lGL

TARGET = main

all:
	$(CXX) $(CXXFLAGS) main.cpp -o $(TARGET) $(LIBS)

clean:
	rm -f $(TARGET)
