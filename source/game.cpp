#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <numbers>
#include <string>
#include <utility>
#include <vector>

#include "twsfw/game.hpp"

#include <twsfwphysx/twsfwphysx.h>
#include <wasm.h>
#include <wasmtime.h>

#include "twsfw/twsfw_agent.h"
#include "twsfw/wasm_agent.hpp"

namespace
{
wasmtime_module_t *get_agent_module(const ::twsfw::WASMAgent &agent)
{
    assert(agent.module != nullptr);
    return static_cast<wasmtime_module_t *>(agent.module);
}

wasmtime_instance_t *get_agent_instance(const ::twsfw::WASMAgent &agent)
{
    assert(agent.instance != nullptr);
    return static_cast<wasmtime_instance_t *>(agent.instance);
}

uint8_t *get_agent_memory(const ::twsfw::WASMAgent &agent)
{
    assert(agent.memory != nullptr);
    return agent.memory;
}

const wasmtime_func_t *get_agent_func(const ::twsfw::WASMAgent &agent)
{
    assert(agent.func != nullptr);
    return &static_cast<wasmtime_extern_t *>(agent.func)->of.func;
}

void destroy_agent(::twsfw::WASMAgent &agent)
{
    wasmtime_module_delete(get_agent_module(agent));
    agent.module = nullptr;

    delete get_agent_instance(agent);
    agent.instance = nullptr;

    delete static_cast<wasmtime_extern_t *>(agent.func);
    agent.func = nullptr;
}
}  // namespace

namespace twsfw
{
Game::Game(const std::vector<std::basic_string<uint8_t>> &wasm_agents,
           const size_t agent_multiplicity,
           const World &world,
           const size_t ticks_per_second)
    : m_engine(wasm_engine_new())
    , m_physx(Physx(wasm_agents.size() * agent_multiplicity,
                    {.restitution = world.restitution,
                     .agent_radius = world.agent_radius,
                     .missile_acceleration = world.missile_acceleration}))
    , m_world(world)
    , m_agents_multiplicity(agent_multiplicity)
    , m_missile_cooldown(m_agents_multiplicity * wasm_agents.size(), 0.F)
{
    m_world.agent_healing_rate /= static_cast<float>(ticks_per_second);
    m_world.agent_cooldown /= static_cast<float>(ticks_per_second);
    m_world.agent_max_rotation_speed /= static_cast<float>(ticks_per_second);
    m_world.missile_acceleration /= static_cast<float>(ticks_per_second);

    assert(m_engine != nullptr);

    m_store = wasmtime_store_new(
        static_cast<wasm_engine_t *>(m_engine), nullptr, nullptr);
    assert(m_store != nullptr);
    m_context =
        wasmtime_store_context(static_cast<wasmtime_store_t *>(m_store));

    for (const auto &wasm : wasm_agents) {
        m_wasm_agents.emplace_back(make_agent(wasm));
    }

    for (auto i = 0U; i < m_physx.agents_size(); i++) {
        constexpr auto two_pi = 2.F * std::numbers::pi_v<float>;
        const float angle = static_cast<float>(i)
            / static_cast<float>(m_physx.get_agents().size()) * two_pi;
        m_physx.get_agents()[i] = {.r = {std::cos(angle), std::sin(angle), 0.F},
                                   .u = {0.F, 0.F, 1.F},
                                   .v = 0.F,
                                   .a = 0.F,
                                   .hp = 4};
    }
}

Game::Game(Game &&other) noexcept
    : m_engine(other.m_engine)
    , m_store(other.m_store)
    , m_context(other.m_context)
    , m_physx(std::move(other.m_physx))
    , m_world(other.m_world)
    , m_wasm_agents(std::move(other.m_wasm_agents))
    , m_agents_multiplicity(other.m_agents_multiplicity)
    , m_missile_cooldown(std::move(other.m_missile_cooldown))
{
}

Game &Game::operator=(Game &&other) noexcept
{
    if (this != &other) {
        m_engine = other.m_engine;
        other.m_engine = nullptr;

        m_store = other.m_store;
        other.m_store = nullptr;

        m_context = other.m_context;
        other.m_context = nullptr;

        m_physx = std::move(other.m_physx);
        m_world = other.m_world;
        m_wasm_agents = std::move(other.m_wasm_agents);
        m_agents_multiplicity = other.m_agents_multiplicity;
        m_missile_cooldown = std::move(other.m_missile_cooldown);
    }

    return *this;
}

Game::~Game()
{
    for (auto &agent : m_wasm_agents) {
        destroy_agent(agent);
    }

    wasmtime_store_delete(static_cast<wasmtime_store_t *>(m_store));
    wasm_engine_delete(static_cast<wasm_engine_t *>(m_engine));
}

WASMAgent Game::make_agent(const std::basic_string<uint8_t> &wasm) const
{
    auto *ctx = static_cast<wasmtime_context_t *>(m_context);

    wasmtime_module_t *module = nullptr;
    const auto *error =
        wasmtime_module_new(static_cast<wasm_engine_t *>(m_engine),
                            wasm.data(),
                            wasm.size(),
                            &module);
    assert(error == nullptr and module != nullptr);

    auto *instance = new wasmtime_instance_t{};
    wasm_trap_t *trap = nullptr;
    error = wasmtime_instance_new(ctx, module, nullptr, 0, instance, &trap);
    assert(error == nullptr and trap == nullptr);

    auto *func = new wasmtime_extern_t{};
    bool ok = wasmtime_instance_export_get(
        ctx, instance, "twsfw_agent_act", strlen("twsfw_agent_act"), func);
    assert(ok);
    assert(func->kind == WASMTIME_EXTERN_FUNC);

    wasmtime_extern_t item;
    ok = wasmtime_instance_export_get(
        ctx, instance, "memory", strlen("memory"), &item);
    assert(ok && item.kind == WASMTIME_EXTERN_MEMORY);
    const wasmtime_memory_t memory = item.of.memory;
    uint8_t *memory_data = wasmtime_memory_data(ctx, &memory);

    return {.module = module,
            .instance = instance,
            .func = func,
            .memory = memory_data};
}

std::vector<size_t> Game::serialize_world(std::vector<uint8_t> &buffer) const
{
    std::vector<size_t> offsets;

    auto offset = buffer.size();
    {
        const auto &agents = m_physx.get_agents();
        const auto n_bytes = agents.size() * sizeof(agents.front());

        buffer.resize(offset + n_bytes);
        std::memcpy(buffer.data() + offset, agents.data(), n_bytes);

        offsets.emplace_back(offset);
        offset += n_bytes;
    }

    {
        const auto &missiles = m_physx.get_missiles();
        const auto n_bytes = missiles.size() * sizeof(missiles.front());

        buffer.resize(offset + n_bytes);
        std::memcpy(buffer.data() + offset, missiles.data(), n_bytes);

        offsets.emplace_back(offset);
        offset += n_bytes;
    }

    {
        const auto &world = m_physx.get_world();
        constexpr auto n_bytes = sizeof(world);
        buffer.resize(offset + n_bytes);
        std::memcpy(buffer.data() + offset, &world, n_bytes);

        offsets.emplace_back(offset);
        // offset += n_bytes;
    }

    return offsets;
}

void Game::call_agent(const size_t team,
                      const std::vector<uint8_t> &buffer,
                      const std::vector<size_t> &offsets)
{
    std::memcpy(
        get_agent_memory(m_wasm_agents[team]), buffer.data(), buffer.size());

    assert(offsets.size() == 3);
    auto make_arg = [](auto value)
    {
        return wasmtime_val_t{.kind = WASMTIME_I32,
                              .of = {.i32 = static_cast<int32_t>(value)}};
    };

    for (auto i = 0U; i < m_agents_multiplicity; i++) {
        const auto agent_idx = (team * m_agents_multiplicity) + i;

        const std::array args{
            make_arg(offsets[0]),
            make_arg(m_physx.agents_size()),
            make_arg(offsets[1]),
            make_arg(m_physx.missiles_size()),
            make_arg(m_missile_cooldown[agent_idx]),
            make_arg(offsets[2]),
            make_arg(agent_idx),
            make_arg(0),
        };

        wasmtime_val_t result;
        wasm_trap_t *trap = nullptr;
        const auto *error =
            wasmtime_func_call(static_cast<wasmtime_context_t *>(m_context),
                               get_agent_func(m_wasm_agents[team]),
                               args.data(),
                               args.size(),
                               &result,
                               1,
                               &trap);
        assert(error == nullptr and trap == nullptr);
        const auto action_value = result.of.f32;

        int32_t action_type = 0;
        std::memcpy(&action_type,
                    get_agent_memory(m_wasm_agents[team]) + 0,
                    sizeof(int32_t));

        if (action_type == 0) {
            m_physx.rotate_agent(
                agent_idx,
                std::min(m_world.agent_max_rotation_speed, action_value));
        } else if (action_type == 1) {
            m_physx.get_agents()[agent_idx].a = action_value;
        } else if (action_type == 2 and m_missile_cooldown[agent_idx] <= 0.F) {
            m_physx.fire(agent_idx);
        }
    }
}

Game::State Game::tick(const float t, const int32_t n_steps)
{
    for (auto i = 0U; i < m_physx.agents_size(); i++) {
        auto &agent = m_physx.get_agents()[i];
        agent.hp = std::min(
            std::max(agent.hp, -1.F) + m_world.agent_healing_rate, 4.F);

        auto &cooldown = m_missile_cooldown[i];
        cooldown = std::max(cooldown - m_world.agent_cooldown, 0.F);
    }

    m_physx.simulate(t, n_steps);

    std::vector<uint8_t> buffer(sizeof(int32_t));
    const auto &offsets = serialize_world(buffer);

    for (auto i = 0U; i < m_wasm_agents.size(); i++) {
        call_agent(i, buffer, offsets);
    }

    std::vector<twsfw_agent> agents(m_physx.agents_size());
    std::vector<twsfw_missile> missiles(m_physx.missiles_size());

    for (auto i = 0U; i < m_physx.agents_size(); i++) {
        const auto &agent = m_physx.get_agents()[i];

        agents[i].r.x = agent.r.x;
        agents[i].r.y = agent.r.y;
        agents[i].r.z = agent.r.z;

        agents[i].u.x = agent.u.x;
        agents[i].u.y = agent.u.y;
        agents[i].u.z = agent.u.z;

        agents[i].v = agent.v;
        agents[i].a = agent.a;
        agents[i].hp = static_cast<int32_t>(std::lround(agent.hp));
        agents[i].team = static_cast<int32_t>(i / m_agents_multiplicity);
    }

    for (auto i = 0U; i < m_physx.agents_size(); i++) {
        const auto &missile = m_physx.get_missiles()[i];

        missiles[i].r.x = missile.r.x;
        missiles[i].r.y = missile.r.y;
        missiles[i].r.z = missile.r.z;

        missiles[i].u.x = missile.u.x;
        missiles[i].u.y = missile.u.y;
        missiles[i].u.z = missile.u.z;

        missiles[i].v = missile.v;
        missiles[i].agent_id = missile.payload;
    }

    return {.agents = agents, .missiles = missiles};
}
}  // namespace twsfw
