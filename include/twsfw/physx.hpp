#pragma once

#include <cstddef>
#include <span>

#include <twsfwphysx/twsfwphysx.h>

namespace twsfw
{
class Physx final
{
    twsfwphysx_agents m_agents;
    twsfwphysx_missiles m_missiles;
    twsfwphysx_world m_world;
    twsfwphysx_simulation_buffer *m_simulation_buffer;

  public:
    Physx(size_t n_players, const twsfwphysx_world &world);

    Physx(Physx &other) = delete;

    Physx(Physx &&other) noexcept;

    Physx &operator=(const Physx &other) = delete;

    Physx &operator=(Physx &&other) noexcept;

    ~Physx();

    void add_missile(const twsfwphysx_missiles &missile);

    std::span<twsfwphysx_agent> get_agents() const;

    size_t agents_size() const;

    std::span<twsfwphysx_missile> get_missiles() const;

    size_t missiles_size() const;

    const twsfwphysx_world &get_world() const;

    void simulate(float t, int32_t n_steps);

    void rotate_agent(size_t agent_idx, float angle) const;

    void fire(size_t agent_idx, float v);
};
}  // namespace twsfw