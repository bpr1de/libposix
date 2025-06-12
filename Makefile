#
# Makefile used to bootstrap the environment and set up stage 2 build using
# CMake.
#

CMAKE_DIR ?= cmake-build-debug

all: $(CMAKE_DIR)
	$(MAKE) -C $(CMAKE_DIR)

$(CMAKE_DIR):
	cmake -DCMAKE_BUILD_TYPE=Debug -G "Unix Makefiles" -S . -B $(CMAKE_DIR)

test: all
	$(CMAKE_DIR)/test-runner ./$(CMAKE_DIR)/lib*_test.so

clean:
	rm -fr $(CMAKE_DIR)

.PHONY: all clean
