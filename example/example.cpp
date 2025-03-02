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
    twsfw::Game game{wasm_agents,
                     2,
                     {.agent_radius = .1F,
                      .agent_healing_rate = 6.F,
                      .agent_cooldown = 2.F,
                      .agent_max_velocity = 1.F,
                      .agent_max_rotation_speed = 2.F,
                      .restitution = .5F,
                      .missile_max_velocity = 2.F},
                     60};

    constexpr float t = 1.F;
    constexpr int32_t n_steps = 1'000;
    auto state = game.tick(t, n_steps);
    assert(state.agents.size() == 4);
    assert(state.missiles.size() == 4);

    return 0;
}
