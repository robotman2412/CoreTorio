// Copyright © 2024, Julian Scheffers
// SPDX-License-Identifier: MIT

#include "injection_priv.hpp"



namespace coretorio::injection {

using X86Insn  = CodeFlowInsn;
using X86Graph = CodeFlowGraph<X86Insn>;

// Analyze code and create a new node.
X86Graph::Node *X86Graph::Node::analyze(size_t startAddress) {
}

// Apply this relocation.
void Reloc::apply() const {
}

// Generate code for all injections on a symbol.
bool doCodeGen(InjectionCtx &ctx, InjectionSite const &site) {
}

} // namespace coretorio::injection
