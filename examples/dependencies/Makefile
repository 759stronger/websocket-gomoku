CC ?= gcc
CXX ?= g++
CPPFLAGS += -I../../include $(MYSQL_CPPFLAGS)
CFLAGS ?= -std=c11 -g -Wall -Wextra
CXXFLAGS ?= -std=c++11 -g -Wall -Wextra
MYSQL_LIBS ?= -lmysqlclient

.PHONY: all
all: ../../build/examples/mysql ../../build/examples/json ../../build/examples/wsserver

../../build/examples/mysql: mysql.c ../../include/gomoku/config.h
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@ $(LDFLAGS) $(MYSQL_LIBS)

../../build/examples/json: json.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $< -o $@ $(LDFLAGS) -ljsoncpp -lboost_system -pthread

../../build/examples/wsserver: wsserver.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $< -o $@ $(LDFLAGS) -lboost_system -pthread
