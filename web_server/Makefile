TARGET = http_server
INC_PATH = -I./
CC = g++
CFLAGS = -std=c++17 -O0 -Wall -c $(INC_PATH) -g
LD = g++
LDFLAGS = -std=c++17 -lpthread
REMOVE = -rm -rf

TOP_DIR = $(shell pwd)
HEADER_WITH_DIR = $(wildcard $(TOP_DIR)/*.h)
SRC_WITH_DIR += $(wildcard $(TOP_DIR)/*.cpp)
OBJ_WITH_DIR = $(patsubst %.cpp, %.o, $(SRC_WITH_DIR))

$(TARGET): $(OBJ_WITH_DIR) $(HEADER_WITH_DIR)
	$(LD) $^ -o $(TARGET) $(LDFLAGS)

%.o: %.cpp
	$(CC) $(CFLAGS) $< -o $@

.PHONY:
clean:
	$(REMOVE) $(OBJ_WITH_DIR) $(TARGET)