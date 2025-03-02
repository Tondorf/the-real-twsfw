#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "twsfw/physx.hpp"
#include "twsfw/twsfw_export.hpp"
#include "twsfw/wasm_agent.hpp"

namespace twsfw
{
class TWSFW_EXPORT Game final
{
    void *m_engine = nullptr;
    void *m_store = nullptr;
    void *m_context = nullptr;

    Physx m_physx;

    std::vector<WASMAgent> m_wasm_agents;

    [[nodiscard]] WASMAgent make_agent(
        const std::basic_string<uint8_t> &wasm) const;

    std::vector<size_t> serialize_world(std::vector<uint8_t> &buffer) const;

    void call_agent(size_t agent_idx,
                    const std::vector<uint8_t> &buffer,
                    const std::vector<size_t> &offsets);

  public:
    struct World
    {
        float restitution;
        float agent_radius;
        float missile_acceleration;
    };

    explicit Game(const std::vector<std::basic_string<uint8_t>> &wasm_agents,
                  const World &world);

    Game(const Game &) = delete;

    Game(Game &&other) noexcept;

    Game &operator=(const Game &) = delete;

    Game &operator=(Game &&other) noexcept;

    ~Game();

    struct State
    {
        std::span<twsfwphysx_agent> agents;
        std::span<twsfwphysx_missile> missiles;
    };
    State tick(float t, int32_t n_steps);
};
}  // namespace twsfw
