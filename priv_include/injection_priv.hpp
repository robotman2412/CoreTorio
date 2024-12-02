// Copyright © 2024, Julian Scheffers
// SPDX-License-Identifier: MIT

#include "injection.hpp"
#include "object.hpp"

#include <map>
#include <memory>
#include <stdint.h>
#include <vector>

#pragma once



namespace coretorio::injection {
using object::Symbol;

// An injection site.
struct InjectionSite {
    // Symbol to inject at.
    Symbol const          *symbol;
    // Injections to place before the function.
    std::vector<Injection> before;
    // Injections to place after the function.
    std::vector<Injection> after;
};

// A section of generated code.
struct Section {
    // Virtual address for code to be placed at.
    size_t               addr;
    // Code output.
    std::vector<uint8_t> code;
};

// Relocation entry.
struct Reloc {
    // Architecture-specific relocation type.
    int      type;
    // Relocation addend.
    size_t   addend;
    // Target section.
    Section *target;
    // Relocation offset in target section.
    size_t   targetOffset;
    // Reference section.
    Section *reference;
    // Relocation offset in reference section.
    size_t   referenceOffset;
    // Apply this relocation.
    void     apply() const;
};

// Injection linking context.
struct InjectionCtx {
    // Patched code sections.
    std::vector<Section> patches;
    // Primary output section.
    Section              generated;
    // Relocations.
    std::vector<Reloc>   reloc;
};

// Code flow graph instruction.
struct CodeFlowInsn {
    enum Type {
        // Function call.
        CALL,
        // Unconditional jump.
        JUMP,
        // Conditional branch.
        BRANCH,
        // Other instructions.
        OTHER,
    };

    // Start address of this instruction.
    size_t addr;
    // Length in bytes.
    size_t length;
};

// Code flow graph.
template <typename InsnType, typename std::enable_if<std::is_base_of<CodeFlowInsn, InsnType>::value>::type * = nullptr>
struct CodeFlowGraph {
    struct Node : InsnType {
        // Next instruction after this one.
        InsnType    *next;
        // Branching instruction after this one.
        InsnType    *branch;
        // Analyze code and create a new node.
        static Node *analyze(size_t startAddress);
    };

    // Start point of the graph.
    Node                    *start;
    // Unordered set of instructions.
    std::map<size_t, Node *> insns;

    // Create a code flow graph by analyzing code.
    static CodeFlowGraph analyze(size_t startAddress) {
        CodeFlowGraph graph;
        graph.start = graph.getNodeAt(startAddress);
        return graph;
    }

    // Clean up the thing.
    ~CodeFlowGraph() {
        for (auto &pair : insns) {
            delete pair->second;
        }
    }
    CodeFlowGraph(CodeFlowGraph<InsnType> const &) = delete;

    // Get the node that starts at a certain address.
    Node *getNodeAt(size_t startAddress) {
        if (insns.find(startAddress) == insns.end()) {
            Node *node = Node::analyze(startAddress);
            insns.emplace(startAddress, node);
            return node;
        } else {
            return insns[startAddress];
        }
    }
};

// Generate code for all injections on a symbol.
bool doCodeGen(InjectionCtx &ctx, InjectionSite const &site);

// Initialize the injection sub-system.
void init();
// Inject the code now.
bool performInjections();

} // namespace coretorio::injection
