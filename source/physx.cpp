#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <span>

#include "twsfw/physx.hpp"

#include "twsfwphysx/twsfwphysx.h"

namespace twsfw
{
Physx::Physx(const size_t n_players, const twsfwphysx_world &world)
    : m_agents(twsfwphysx_create_agents(static_cast<int32_t>(n_players)))
    , m_missiles(twsfwphysx_new_missile_batch())
    , m_world(world)
    , m_simulation_buffer(twsfwphysx_create_simulation_buffer())
{
}

Physx::Physx(Physx &&other) noexcept
    : m_agents(other.m_agents)
    , m_missiles(other.m_missiles)
    , m_world(other.m_world)
    , m_simulation_buffer(other.m_simulation_buffer)
{
    other.m_agents.agents = nullptr;
    other.m_agents.size = 0;

    other.m_missiles.missiles = nullptr;
    other.m_missiles.size = 0;

    other.m_simulation_buffer = nullptr;
}

Physx &Physx::operator=(Physx &&other) noexcept
{
    if (this != &other) {
        m_agents = other.m_agents;
        other.m_agents.agents = nullptr;
        other.m_agents.size = 0;

        m_missiles = other.m_missiles;
        other.m_missiles.missiles = nullptr;
        other.m_missiles.size = 0;

        m_world = other.m_world;

        m_simulation_buffer = other.m_simulation_buffer;
        other.m_simulation_buffer = nullptr;
    }

    return *this;
}

Physx::~Physx()
{
    twsfwphysx_delete_missile_batch(&m_missiles);
    twsfwphysx_delete_agents(&m_agents);
    twsfwphysx_delete_simulation_buffer(m_simulation_buffer);
}

void Physx::simulate(const float t, const int32_t n_steps)
{
    twsfwphysx_simulate(
        &m_agents, &m_missiles, &m_world, t, n_steps, m_simulation_buffer);
}

std::span<twsfwphysx_agent> Physx::get_agents() const
{
    return std::span{m_agents.agents, static_cast<size_t>(m_agents.size)};
}

size_t Physx::agents_size() const
{
    return static_cast<size_t>(m_agents.size);
}

std::span<twsfwphysx_missile> Physx::get_missiles() const
{
    return std::span{m_missiles.missiles, static_cast<size_t>(m_missiles.size)};
}

size_t Physx::missiles_size() const
{
    return static_cast<size_t>(m_missiles.size);
}

const twsfwphysx_world &Physx::get_world() const
{
    return m_world;
}

void Physx::rotate_agent(size_t agent_idx, float angle) const
{
    assert(agent_idx < static_cast<size_t>(m_agents.size));
    twsfwphysx_rotate_agent(&m_agents.agents[agent_idx], angle);
}

void Physx::fire(size_t agent_idx, const float v)
{
    auto &agent = m_agents.agents[agent_idx];
    const float theta = m_world.agent_radius * 1.01F;
    const float sin_theta = std::sin(theta);
    const float cos_theta = std::cos(theta);

    const auto r = agent.r;
    const auto u = agent.u;
    const twsfwphysx_vec w{
        .x = (u.y * r.z) - (u.z * r.y),
        .y = (u.z * r.x) - (u.x * r.z),
        .z = (u.x * r.y) - (u.y * r.x),
    };
    twsfwphysx_add_missile(&m_missiles,
                           {.r = {.x = (cos_theta * r.x) + (sin_theta * w.x),
                                  .y = (cos_theta * r.y) + (sin_theta * w.y),
                                  .z = (cos_theta * r.z) + (sin_theta * w.z)},
                            .u = agent.u,
                            .v = v,
                            .payload = static_cast<int32_t>(agent_idx)});
}

}  // namespace twsfw