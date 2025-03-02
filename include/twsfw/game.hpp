#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "twsfw/physx.hpp"
#include "twsfw/twsfw_agent.h"
#include "twsfw/twsfw_export.hpp"
#include "twsfw/wasm_agent.hpp"

namespace twsfw
{
class TWSFW_EXPORT Game final
{
  public:
    struct World
    {
        float agent_radius;
        float agent_healing_rate;
        float agent_cooldown;
        float agent_max_rotation_speed;
        float restitution;
        float missile_acceleration;
    };

  private:
    void *m_engine = nullptr;
    void *m_store = nullptr;
    void *m_context = nullptr;

    Physx m_physx;
    World m_world;

    std::vector<WASMAgent> m_wasm_agents;
    size_t m_agents_multiplicity;

    std::vector<float> m_missile_cooldown;

    [[nodiscard]] WASMAgent make_agent(
        const std::basic_string<uint8_t> &wasm) const;

    std::vector<size_t> serialize_world(std::vector<uint8_t> &buffer) const;

    void call_agent(size_t team,
                    const std::vector<uint8_t> &buffer,
                    const std::vector<size_t> &offsets);

  public:
    explicit Game(const std::vector<std::basic_string<uint8_t>> &wasm_agents,
                  size_t agent_multiplicity,
                  const World &world,
                  size_t ticks_per_second);

    Game(const Game &) = delete;

    Game(Game &&other) noexcept;

    Game &operator=(const Game &) = delete;

    Game &operator=(Game &&other) noexcept;

    ~Game();

    struct State
    {
        std::vector<twsfw_agent> agents;
        std::vector<twsfw_missile> missiles;
    };
    State tick(float t, int32_t n_steps);
};
}  // namespace twsfw
