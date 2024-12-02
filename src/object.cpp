// Copyright © 2024, Julian Scheffers
// SPDX-License-Identifier: MIT

#include "object.hpp"

#include <dlfcn.h>
#include <link.h>
#include <map>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>


namespace coretorio::object {

// Map of sections found in the Factorio executable.
static std::map<std::string, Section> *sections;
// Map of symbols found in the Factorio executable.
static std::map<std::string, Symbol>  *symbols;

// Set to true if CoreTorio successfully injected into Factorio.
bool success;



// Search for the ELF header by continually decreasing the address.
// Returns NULL if not found.
static Elf64_Ehdr *find_ehdr(void *max_addr) {
    if (memcmp(max_addr, ELFMAG, 4)) {
        return NULL;
    }
    return (Elf64_Ehdr *)max_addr;
}

// Print failure message.
static void fail(char const *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    vprintf(fmt, va);
    va_end(va);
    printf(", CoreTorio will not start\n");
    if (errno) {
        perror("Errno");
    }
    _exit(1);
}

// Print failure message for out of memory.
static void oom_fail(char const *purpose, size_t cap) {
    fail("Out of memory (allocating %zu bytes for %s)", cap, purpose);
}

// Find the game executable.
static char const *get_game_path() {
    return "/proc/self/exe";
}

// Interpret the ELF file and determine the locations of sections and symbols.
bool interpret_elf() {
    // Get game executable offset.
    void *game_handle = dlopen(NULL, RTLD_NOW | RTLD_NOLOAD);
    if (!game_handle) {
        fail("dlopen() failed");
        return false;
    }
    struct link_map *game_link = NULL;
    dlinfo(game_handle, RTLD_DI_LINKMAP, &game_link);
    if (!game_link) {
        fail("dlinfo() failed");
        return false;
    }


    // Allocate memory.
    sections = new std::map<std::string, struct Section>();
    symbols  = new std::map<std::string, struct Symbol>();


    // Open the executable file and read the header.
    char const *game_path = get_game_path();
    if (!game_path) {
        fail("Finding game executable failed");
        return false;
    }
    FILE *game_fd = fopen(game_path, "r");
    if (!game_fd) {
        fail("Opening game executable failed");
        return false;
    }
    Elf64_Ehdr header;
    if (fread(&header, 1, sizeof(Elf64_Ehdr), game_fd) != sizeof(Elf64_Ehdr)) {
        fail("Reading ELF header failed");
        return false;
    }

    if (memcmp(header.e_ident, ELFMAG, 4)) {
        fail("Invalid ELF magic");
        return false;
    } else if (header.e_ident[EI_CLASS] != 2) {
        fail("Invalid ELF class");
        return false;
    } else if (header.e_ident[EI_DATA] != 1) {
        fail("Invalid ELF endianness");
        return false;
    } else if (header.e_ident[EI_VERSION] != 1) {
        fail("Invalid ELF version");
        return false;
    }


    // Build the map of sections.
    if (header.e_shstrndx == SHN_UNDEF) {
        fail("No .shstrtab");
        return false;
    } else if (header.e_shentsize != sizeof(Elf64_Shdr)) {
        fail("Section header entry size invalid");
        return false;
    } else if (header.e_shnum == 0) {
        fail("No section headers");
        return false;
    }

    // Read section header table.
    auto shdrs = (Elf64_Shdr *)malloc(sizeof(Elf64_Shdr) * header.e_shnum);
    fseek(game_fd, header.e_shoff, SEEK_SET);
    if (!shdrs) {
        oom_fail("section headers", sizeof(Elf64_Shdr) * header.e_shnum);
        return false;
    } else if (fread(shdrs, 1, sizeof(Elf64_Shdr) * header.e_shnum, game_fd) != sizeof(Elf64_Shdr) * header.e_shnum) {
        fail("Reading section headers failed");
        return false;
    }

    // Read section header string table.
    auto shdr_names = (char *)malloc(shdrs[header.e_shstrndx].sh_size);
    fseek(game_fd, shdrs[header.e_shstrndx].sh_offset, SEEK_SET);
    if (!shdr_names) {
        oom_fail("section header names", shdrs[header.e_shstrndx].sh_size);
        return false;
    } else if (fread(shdr_names, 1, shdrs[header.e_shstrndx].sh_size, game_fd) != shdrs[header.e_shstrndx].sh_size) {
        fail("Reading section header names failed");
        return false;
    }

    // Read section table.
    for (Elf64_Half i = 0; i < header.e_shnum; i++) {
        struct Section sect;
        sect.sh_name_str  = shdr_names + shdrs[i].sh_name;
        sect.sh_addr_ptr  = shdrs[i].sh_addr ? (void *)(shdrs[i].sh_addr + game_link->l_addr) : 0;
        sect.sh_name      = shdrs[i].sh_name;
        sect.sh_type      = shdrs[i].sh_type;
        sect.sh_flags     = shdrs[i].sh_flags;
        sect.sh_addr      = shdrs[i].sh_addr;
        sect.sh_offset    = shdrs[i].sh_offset;
        sect.sh_size      = shdrs[i].sh_size;
        sect.sh_link      = shdrs[i].sh_link;
        sect.sh_info      = shdrs[i].sh_info;
        sect.sh_addralign = shdrs[i].sh_addralign;
        sect.sh_entsize   = shdrs[i].sh_entsize;
        sections->emplace(sect.sh_name_str, sect);
    }
    free(shdr_names);
    free(shdrs);


    // Build the map of symbols.
    if (sections->find(".strtab") == sections->end()) {
        fail("Missing .strtab");
        return false;
    } else if (sections->find(".symtab") == sections->end()) {
        fail("Missing .symtab");
        return false;
    }

    auto &&symtab = (*sections)[".symtab"];
    auto &&strtab = (*sections)[".strtab"];
    if (symtab.sh_entsize != sizeof(Elf64_Sym)) {
        fail("Invalid .symtab entry size");
    }


    // Read symbol header table.
    auto syms = (Elf64_Sym *)malloc(symtab.sh_size);
    fseek(game_fd, symtab.sh_offset, SEEK_SET);
    if (!syms) {
        oom_fail("symbol table", symtab.sh_size);
        return false;
    } else if (fread(syms, 1, symtab.sh_size, game_fd) != symtab.sh_size) {
        fail("Reading symbol table failed");
        return false;
    }

    // Read symbol header string table.
    auto sym_names = (char *)malloc(strtab.sh_size);
    fseek(game_fd, strtab.sh_offset, SEEK_SET);
    if (!sym_names) {
        oom_fail("symbol names", strtab.sh_size);
        return false;
    } else if (fread(sym_names, 1, strtab.sh_size, game_fd) != strtab.sh_size) {
        fail("Reading symbol names failed");
        return false;
    }

    // Read symbol table.
    for (Elf64_Word i = 0; i < symtab.sh_size / symtab.sh_entsize; i++) {
        struct Symbol sym;
        sym.st_name_str  = sym_names + syms[i].st_name;
        sym.st_value_ptr = syms[i].st_value ? (void *)(syms[i].st_value + game_link->l_addr) : 0;
        sym.st_name      = syms[i].st_name;
        sym.st_info      = syms[i].st_info;
        sym.st_other     = syms[i].st_other;
        sym.st_shndx     = syms[i].st_shndx;
        sym.st_value     = syms[i].st_value;
        sym.st_size      = syms[i].st_size;
        symbols->emplace(sym.st_name_str, sym);
    }
    free(syms);
    free(sym_names);
    fclose(game_fd);

    return true;
}


// Find a symbol.
Symbol *findSymbol(std::string const &name) {
    auto res = symbols->find(name);
    if (res == symbols->end()) {
        return NULL;
    }
    return &res->second;
}

} // namespace coretorio::object
