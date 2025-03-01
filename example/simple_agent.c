#include "twsfw_agent.h"

#include <stdint.h>

#include <emscripten.h>

EMSCRIPTEN_KEEPALIVE
float twsfw_agent_act(struct twsfw_agent *agents,
                      int32_t n_agents,
                      const struct twsfw_missile *missiles,
                      int32_t n_missiles,
                      const struct twsfw_world *world,
                      int32_t id,
                      int32_t *action)
{
    *action = n_agents;
    return missiles[n_missiles - 1].r.y;
}
