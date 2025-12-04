CXX = g++
CXXFLAGS = -std=c++17 -Wall -Iinclude -I$(shell brew --prefix glfw)/include -I$(shell brew --prefix glew)/include
LDFLAGS = -L$(shell brew --prefix glfw)/lib -L$(shell brew --prefix glew)/lib -lglfw -lGLEW -framework OpenGL

SRC = $(wildcard src/*.cpp)
OBJ = $(SRC:.cpp=.o)

TARGET = app

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(OBJ) -o build/$(TARGET) $(LDFLAGS)

clean:
	rm -f src/*.o build/$(TARGET)
