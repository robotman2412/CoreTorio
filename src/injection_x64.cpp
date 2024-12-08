// Copyright © 2024, Julian Scheffers
// SPDX-License-Identifier: MIT

#include "injection_priv.hpp"

#include <inttypes.h>
#include <Zydis/Zydis.h>



namespace coretorio::injection {

using X64Insn  = CodeFlowInsn;
using X64Graph = CodeFlowGraph<X64Insn>;

// Analyze code and create a new node.
template <> X64Graph::Node X64Graph::Node::analyze(size_t startAddress, size_t maxLength) {
    // Disassemble an instruction.
    ZydisDisassembledInstruction insn;
    ZydisDisassembleIntel(ZYDIS_MACHINE_MODE_LONG_64, startAddress, (uint8_t *)startAddress, maxLength, &insn);
    // printf("%p  ", (void *)startAddress);
    // puts(insn.text);

    // Copy the lengths into the node.
    X64Graph::Node node;
    node.addr   = startAddress;
    node.length = insn.info.length;
    node.next   = node.addr + node.length;

    if (insn.info.meta.branch_type) {
        node.branch = node.next + insn.operands[0].imm.value.s;
    }
    switch (insn.info.mnemonic) {
        case ZYDIS_MNEMONIC_JL:
        case ZYDIS_MNEMONIC_JLE:
        case ZYDIS_MNEMONIC_JNB:
        case ZYDIS_MNEMONIC_JNBE:
        case ZYDIS_MNEMONIC_JNL:
        case ZYDIS_MNEMONIC_JNLE:
        case ZYDIS_MNEMONIC_JNO:
        case ZYDIS_MNEMONIC_JNP:
        case ZYDIS_MNEMONIC_JNS:
        case ZYDIS_MNEMONIC_JNZ:
        case ZYDIS_MNEMONIC_JO:
        case ZYDIS_MNEMONIC_JP:
        case ZYDIS_MNEMONIC_JRCXZ:
        case ZYDIS_MNEMONIC_JS:
        case ZYDIS_MNEMONIC_JZ: node.type = X64Insn::Type::BRANCH; break;

        case ZYDIS_MNEMONIC_CALL: node.type = X64Insn::Type::CALL; break;
        case ZYDIS_MNEMONIC_JMP: node.type = X64Insn::Type::JUMP; break;
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
    auto graph = X64Graph::analyze(*site.symbol);
    return false;
}

} // namespace coretorio::injection
