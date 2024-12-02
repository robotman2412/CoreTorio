// Copyright © 2024, Julian Scheffers
// SPDX-License-Identifier: MIT

#include "injection_priv.hpp"

#include <map>
#include <string.h>
#include <sys/mman.h>



namespace coretorio::injection {

// Injection is safe to perform.
static bool allowInjection;


// Map of injection sites and the code to inject there.
static std::map<std::string, InjectionSite> *injectionSites;


// Initialize the injection sub-system.
void init() {
    injectionSites = new std::map<std::string, InjectionSite>();
}

// Inject the code now.
bool performInjections() {
    if (!allowInjection) {
        return false;
    }

    // Generate code.
    InjectionCtx ctx;
    printf("Generating code\n");
    bool codeGenSuccess = true;
    for (auto &pair : *injectionSites) {
        if (!doCodeGen(ctx, pair.second)) {
            printf("Injection code generation failed at %s\n", pair.second.symbol->st_name_str.c_str());
            codeGenSuccess = false;
        }
    }
    if (!codeGenSuccess) {
        return false;
    }

    // Allocate memory for code.
    printf("Allocating executable memory\n");
    void *codeMem = mmap(NULL, ctx.generated.code.size(), PROT_READ | PROT_WRITE, MAP_ANONYMOUS, -1, 0);
    if (!codeMem) {
        perror("mmap() failed");
        return false;
    }

    // Link injections.
    printf("Linking injections\n");
    for (auto &reloc : ctx.reloc) {
        reloc.apply();
    }

    // Install injections.
    printf("Installing injections\n");
    memcpy(codeMem, ctx.generated.code.data(), ctx.generated.code.size());
    for (auto &section : ctx.patches) {
        if (mprotect((void *)section.addr, section.code.size(), PROT_READ | PROT_WRITE)) {
            perror("Making code writeable failed");
            abort();
        }
        memcpy((void *)section.addr, section.code.data(), section.code.size());
        if (mprotect((void *)section.addr, section.code.size(), PROT_READ | PROT_EXEC)) {
            perror("Making code executable again failed");
            abort();
        }
    }

    // Make code executable.
    printf("Making code executable\n");
    if (mprotect(codeMem, ctx.generated.code.size(), PROT_READ | PROT_EXEC)) {
        perror("mprotect() failed");
        abort();
    }

    return true;
}


// Inject code to run at one of Factorio's functions.
void injectAt(std::string const &symbolName, Injection toInject, InjectionPoint point) {
    auto symbol = object::findSymbol(symbolName);
    if (!symbol) {
        printf("Error: Injection at non-existent symbol `%s`\n", symbolName.c_str());
        allowInjection = false;
    } else if (allowInjection) {
        if (injectionSites->find(symbolName) == injectionSites->end()) {
            injectionSites->emplace(symbolName, InjectionSite{symbol});
        }
        auto &site = injectionSites->find(symbolName)->second;
        switch (point.type) {
            case InjectionPoint::Type::BEFORE: site.before.emplace_back(toInject); break;
            case InjectionPoint::Type::AFTER: site.after.emplace_back(toInject); break;
        }
    }
}

} // namespace coretorio::injection
