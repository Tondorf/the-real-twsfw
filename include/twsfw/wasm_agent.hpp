#pragma once

#include <cstdint>

namespace twsfw {
    struct WASMAgent
    {
        void *module;
        void *instance;
        void *func;
        uint8_t *memory;
    };
    } // namespace twsfw