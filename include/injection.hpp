// Copyright © 2024, Julian Scheffers
// SPDX-License-Identifier: MIT

#include <functional>
#include <string>

#pragma once



namespace coretorio::injection {

// A function to call, which is only notified and nothing else.
using SimpleInjection = std::function<void()>;
// Something that can be injected into a function.
using Injection       = SimpleInjection;


// Point of a function to inject code into.
struct InjectionPoint {
    // Possible types of injection.
    enum Type {
        // Inject before function.
        BEFORE,
        // Inject after function.
        AFTER,
    };
    Type type;
    InjectionPoint(Type const &from) : type(from) {
    }
    static InjectionPoint before() {
        return BEFORE;
    }
    static InjectionPoint after() {
        return AFTER;
    }
};

// Inject code to run at one of Factorio's functions.
void injectAt(std::string const &symbolName, Injection toInject, InjectionPoint point);

// Inject code to run before one of Factorio's functions.
static inline void injectBefore(std::string const &symbolName, SimpleInjection toInject) {
    injectAt(symbolName, toInject, InjectionPoint::before());
}
// Inject code to run after one of Factorio's functions.
static inline void injectAfter(std::string const &symbolName, SimpleInjection toInject) {
    injectAt(symbolName, toInject, InjectionPoint::after());
}

} // namespace coretorio::injection
