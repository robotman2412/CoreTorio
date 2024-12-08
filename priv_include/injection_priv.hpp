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
        // Tail call.
        TAILCALL,
        // Return.
        RETURN,
        // Function call.
        CALL,
        // Unconditional jump.
        JUMP,
        // Conditional branch.
        BRANCH,
        // Other instructions.
        OTHER,
    };

    // Type of this instruction.
    Type   type;
    // Start address of this instruction.
    size_t addr;
    // Length in bytes.
    size_t length;

    bool isEndOfFunction() const {
        return type == TAILCALL || type == RETURN;
    }
};

// Code flow graph.
template <typename InsnType> struct CodeFlowGraph {
    struct Node : InsnType {
        // Next instruction after this one.
        size_t      next;
        // Branching instruction after this one.
        size_t      branch;
        // Analyze code and create a new node.
        static Node analyze(size_t startAddress, size_t maxLength);
    };

    // Start point of the graph.
    size_t                 startAddress;
    // Unordered set of instructions.
    std::map<size_t, Node> insns;

    // Create a code flow graph by analyzing code.
    static CodeFlowGraph analyze(Symbol const &symbol) {
        CodeFlowGraph graph;
        graph.startAddress = (size_t)symbol.st_value_ptr;
        std::vector<size_t> toAnalyze;
        toAnalyze.push_back(graph.startAddress);

        for (int i = 0; i < 400 && toAnalyze.size(); i++) {
            size_t addr = toAnalyze.back();
            toAnalyze.pop_back();
            Node &node = graph.insns[addr];
            if (node.addr == addr) {
                continue;
            }
            node = Node::analyze(addr, symbol.st_size - addr + graph.startAddress);
            if (node.type == InsnType::Type::JUMP
                && (node.addr < graph.startAddress || node.addr >= graph.startAddress + symbol.st_size)) {
                node.type = InsnType::Type::TAILCALL;
                printf("Tailcall\n");
            }
            if (node.type == InsnType::Type::BRANCH) {
                toAnalyze.push_back(node.branch);
            }
            if (node.type != InsnType::Type::RETURN && node.type != InsnType::Type::TAILCALL) {
                toAnalyze.push_back(node.next);
            }
        }

        return graph;
    }

    // Get the node that starts at a certain address.
    Node &getNodeAt(size_t startAddress) {
        if (insns.find(startAddress) == insns.end()) {
            size_t jumpAddr = 0;
            Node  *node     = Node::analyze(startAddress, jumpAddr);
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
