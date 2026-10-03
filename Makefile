CXX ?= g++
CPPFLAGS += -Iinclude $(MYSQL_CPPFLAGS)
CXXFLAGS ?= -std=c++11 -g -Wall -Wextra
MYSQL_LIBS ?= -lmysqlclient
LDLIBS += $(MYSQL_LIBS) -ljsoncpp -lboost_system -pthread
HEADERS := $(wildcard include/gomoku/*.hpp) include/gomoku/config.h

.PHONY: all config-test examples bench clean
all: build/websocket-gomoku

build/websocket-gomoku: src/main.cc $(HEADERS)
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $< -o $@ $(LDFLAGS) $(LDLIBS)

build/config_test: tests/config_test.cpp include/gomoku/config.h
	mkdir -p $(dir $@)
	$(CXX) -std=c++11 -Wall -Wextra -pedantic -Iinclude $< -o $@

config-test: build/config_test
	./build/config_test

examples:
	$(MAKE) -C examples/dependencies

bench:
	$(MAKE) -C third_party/webbench

clean:
	$(RM) -r build
