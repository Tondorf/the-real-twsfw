#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "twsfw/game.hpp"

#include <wasm.h>
#include <wasmtime.h>

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
Game::Game(const std::vector<std::basic_string<uint8_t>> &wasm_agents)
    : m_engine(wasm_engine_new())
{
    assert(m_engine != nullptr);

    m_store = wasmtime_store_new(
        static_cast<wasm_engine_t *>(m_engine), nullptr, nullptr);
    assert(m_store != nullptr);
    m_context =
        wasmtime_store_context(static_cast<wasmtime_store_t *>(m_store));

    for (const auto &wasm : wasm_agents) {
        m_wasm_agents.emplace_back(make_agent(wasm));
    }

    m_state.agents.push_back({.r = {1.F, 2.F, 3.F},
                              .u = {4.F, 5.F, 6.F},
                              .v = 7.F,
                              .a = 8.F,
                              .hp = 9});
    m_state.missiles.push_back(
        {.r = {10.F, 20.F, 30.F}, .u = {40.F, 50.F, 60.F}, .v = 70.F});

    m_state.world = {
        .restitution = .1F,
        .agent_radius = .2F,
        .missile_acceleration = .3F,
    };

    for (const auto &agent : m_wasm_agents) {
        run_agent(agent);
    }
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
        const auto n_bytes =
            m_state.agents.size() * sizeof(m_state.agents.front());

        buffer.resize(offset + n_bytes);
        std::memcpy(buffer.data() + offset, m_state.agents.data(), n_bytes);

        offsets.emplace_back(offset);
        offset += n_bytes;
    }

    {
        const auto n_bytes =
            m_state.missiles.size() * sizeof(m_state.missiles.front());

        buffer.resize(offset + n_bytes);
        std::memcpy(buffer.data() + offset, m_state.missiles.data(), n_bytes);

        offsets.emplace_back(offset);
        offset += n_bytes;
    }

    {
        constexpr auto n_bytes = sizeof(m_state.world);
        buffer.resize(offset + n_bytes);
        std::memcpy(buffer.data() + offset, &m_state.world, n_bytes);

        offsets.emplace_back(offset);
        // offset += n_bytes;
    }

    return offsets;
}

void Game::run_agent(const WASMAgent &agent) const
{
    std::vector<uint8_t> buffer(sizeof(int32_t));
    const auto &offsets = serialize_world(buffer);
    assert(offsets.size() == 3);

    std::memcpy(get_agent_memory(agent), buffer.data(), buffer.size());

    std::array<wasmtime_val_t, 7> args{};
    args[0] = {.kind = WASMTIME_I32,
               .of = {.i32 = static_cast<int32_t>(offsets[0])}};
    args[1] = {.kind = WASMTIME_I32,
               .of = {.i32 = static_cast<int32_t>(m_state.agents.size())}};
    args[2] = {.kind = WASMTIME_I32,
               .of = {.i32 = static_cast<int32_t>(offsets[1])}};
    args[3] = {.kind = WASMTIME_I32,
               .of = {.i32 = static_cast<int32_t>(m_state.missiles.size())}};
    args[4] = {.kind = WASMTIME_I32,
               .of = {.i32 = static_cast<int32_t>(offsets[2])}};
    args[4] = {.kind = WASMTIME_I32, .of = {.i32 = 0}},
    args[6] = {.kind = WASMTIME_I32, .of = {.i32 = 0}};

    wasmtime_val_t result;
    wasm_trap_t *trap = nullptr;
    const auto *error =
        wasmtime_func_call(static_cast<wasmtime_context_t *>(m_context),
                           get_agent_func(agent),
                           args.data(),
                           args.size(),
                           &result,
                           1,
                           &trap);
    assert(error == nullptr and trap == nullptr);

    int32_t action_type = 0;
    std::memcpy(&action_type, get_agent_memory(agent) + 0, sizeof(int32_t));
    assert(action_type == static_cast<int32_t>(m_state.agents.size()));

    assert(std::abs(result.of.f32 - 20.F) < 1e-6F);  // NOLINT
}
}  // namespace twsfw
