#include "twsfw_agent.h"

#include <stdint.h>

#include <emscripten.h>

EMSCRIPTEN_KEEPALIVE
float twsfw_agent_act(struct twsfw_agent *agents,
                      int32_t n_agents,
                      const struct twsfw_missile *missiles,
                      int32_t n_missiles,
                      int32_t missile_cooldown,
                      const struct twsfw_world *world,
                      int32_t id,
                      int32_t *action)
{
    *action = 2;
    return 0.F;
}
