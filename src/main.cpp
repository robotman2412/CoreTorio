// Copyright © 2024, Julian Scheffers
// SPDX-License-Identifier: MIT

#include "injection_priv.hpp"
#include "object.hpp"

#include <unistd.h>

#define CONSTRUCTOR __attribute__((constructor))
#define CXX11       __attribute__((abi_tag("cxx11")))

namespace coretorio::main {
using object::Symbol;

static void myInjectedFunction() {
    printf("You opened the about GUI!\n");
}

// The primary entrypoint for CoreTorio.
CONSTRUCTOR static void entrypoint() {
    printf("CoreTorio loading...\n");
    if (!object::interpret_elf()) {
        printf("CoreTorio failed, no mods were loaded.\n");
        return;
    }
    injection::init();
    printf("Loading coremods...\n");

    // Test: Let's do something on the `AboutGui()` constructor.
    injection::injectBefore("_ZN8AboutGuiC2Ev", myInjectedFunction);

    // Perform the injections and see what happens.
    if (injection::performInjections()) {
        printf("CoreTorio finished.\n");
    } else {
        printf("CoreTorio failed, no mods were loaded.\n");
    }
}

} // namespace coretorio::main
