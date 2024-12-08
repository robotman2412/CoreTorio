
MAKEFLAGS    += --silent --no-print-directory
FACTORIO_DIR ?= $(HOME)/.local/share/Steam/steamapps/common/Factorio
FACTORIO_BIN ?= $(FACTORIO_DIR)/bin/x64/factorio

.PHONY: run
run: build
	SteamAppId=427520 LD_PRELOAD=$(shell pwd)/build/libcoretorio.so $(FACTORIO_BIN)

.PHONY: build
build:
	mkdir -p build
	cmake -B build
	cmake --build build

.PHONY: clean
clean:
	rm -rf build
