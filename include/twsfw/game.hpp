#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "twsfw/twsfw_agent.h"
#include "twsfw/twsfw_export.hpp"
#include "twsfw/wasm_agent.hpp"

namespace twsfw
{
class TWSFW_EXPORT Game final
{
    void *m_engine = nullptr;
    void *m_store = nullptr;
    void *m_context = nullptr;

    struct World
    {
        std::vector<twsfw_agent> agents;
        std::vector<twsfw_missile> missiles;
        twsfw_world world{};
    } m_state;

    std::vector<WASMAgent> m_wasm_agents;

    [[nodiscard]] WASMAgent make_agent(
        const std::basic_string<uint8_t> &wasm) const;

    std::vector<size_t> serialize_world(std::vector<uint8_t> &buffer) const;

    void run_agent(const WASMAgent &agent) const;

  public:
    explicit Game(const std::vector<std::basic_string<uint8_t>> &wasm_agents);

    Game(const Game &) = delete;
    Game(const Game &&) = delete;
    Game &operator=(const Game &) = delete;
    Game &operator=(Game &&) = delete;

    ~Game();
};
}  // namespace twsfw
