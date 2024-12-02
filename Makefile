
MAKEFLAGS += --silent --no-print-directory
FACTORIO_DIR ?= $(HOME)/.local/share/Steam/steamapps/common/Factorio
FACTORIO_BIN ?= $(FACTORIO_DIR)/bin/x64/factorio

.PHONY: run
run: build
	SteamAppId=427520 LD_PRELOAD=$(shell pwd)/build/libcoretorio.so $(FACTORIO_BIN)

.PHONY: gdb
gdb: build
	gdb $(FACTORIO_BIN) \
		-ex 'set environment LD_PRELOAD $(shell pwd)/build/libcoretorio.so' \
		-ex 'set environment SteamAppId 427520' \
		-ex 'b coretorio::injection::entrypoint' \
		-ex 'r'

.PHONY: build
build:
	mkdir -p build
	cmake -B build
	cmake --build build

.PHONY: clean
clean:
	rm -rf build
