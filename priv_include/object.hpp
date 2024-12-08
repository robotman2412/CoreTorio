// Copyright © 2024, Julian Scheffers
// SPDX-License-Identifier: MIT

#include <elf.h>
#include <string>

#pragma once



namespace coretorio::object {

// ELF section header.
struct Section : public Elf64_Shdr {
    // Section name.
    std::string sh_name_str;
    // Pointer to loaded address of section.
    void       *sh_addr_ptr;
};

// ELF symbol.
struct Symbol : public Elf64_Sym {
    // Symbol name.
    std::string st_name_str;
    // Pointer to loaded address of symbol.
    void       *st_value_ptr;
};

// Find a symbol by name.
Symbol *findSymbol(std::string const &name);

// Interpret the ELF file and determine the locations of sections and symbols.
bool interpret_elf();

} // namespace coretorio::object
