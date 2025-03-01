#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <ios>
#include <string>
#include <vector>

#include "twsfw/game.hpp"

int main(int argc, char *argv[])
{
    assert(argc == 2);
    std::ifstream file(argv[1], std::ios::binary | std::ios::ate);  // NOLINT

    const std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::basic_string data(static_cast<size_t>(size), uint8_t{});
    file.read(std::bit_cast<char *>(data.data()), size);

    const std::vector wasm_agents{data, data};
    const twsfw::Game game{wasm_agents};

    return 0;
}
