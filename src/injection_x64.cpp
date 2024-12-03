// Copyright © 2024, Julian Scheffers
// SPDX-License-Identifier: MIT

#include "injection_priv.hpp"

#include <Zydis/Zydis.h>



namespace coretorio::injection {

using X64Insn  = CodeFlowInsn;
using X64Graph = CodeFlowGraph<X64Insn>;

// Analyze code and create a new node.
template <> X64Graph::Node X64Graph::Node::analyze(size_t startAddress) {
    ZydisDisassembledInstruction insn;
    ZydisDisassembleIntel(ZYDIS_MACHINE_MODE_LONG_64, startAddress, (uint8_t *)startAddress, 15, &insn);
    X64Graph::Node node;
    node.addr   = startAddress;
    node.length = insn.info.length;
    if (insn.info.meta.branch_type) {
        printf("Branch target: %p\n", startAddress + insn.operands[0].mem.disp.value);
    }
    puts(insn.text);
    node.next = node.addr + node.length;
    switch (insn.info.mnemonic) {
        case ZYDIS_MNEMONIC_RET: node.type = X64Insn::Type::RETURN; break;
        default: node.type = X64Insn::Type::OTHER; break;
    }
    return node;
}

// Apply this relocation.
void Reloc::apply() const {
}

// Generate code for all injections on a symbol.
bool doCodeGen(InjectionCtx &ctx, InjectionSite const &site) {
    printf("Analyzing %s @ %p\n", site.symbol->st_name_str.c_str(), site.symbol->st_value_ptr);
    auto graph = X64Graph::analyze((size_t)site.symbol->st_value_ptr);
    return true;
}

} // namespace coretorio::injection
