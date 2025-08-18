#include "chaos.h"

static ChaosMachineSettings chaosSettingsPassive = {
    .name = "Wommy Passive Effects",
    .cycle_length = 20 * 10,
    .default_groups_settings = {
        { /* CHAOS_DISTURBANCE_VERY_LOW */
            .initial_probability = 0.3f,
            .on_pick_multiplier = 1.0f,
            .winner_weight_share = 0.2f,
        },
        { /* CHAOS_DISTURBANCE_LOW */
            .initial_probability = 0.2f,
            .on_pick_multiplier = 1.0f,
            .winner_weight_share = 0.5f,
        },
        { /* CHAOS_DISTURBANCE_MEDIUM */
            .initial_probability = 0.1f,
            .on_pick_multiplier = 1.0f,
            .winner_weight_share = 0.8f,
        },
        { /* CHAOS_DISTURBANCE_HIGH */
            .initial_probability = 0.05f,
            .on_pick_multiplier = 0.8f,
            .winner_weight_share = 1.0f,
        },
        { /* CHAOS_DISTURBANCE_VERY_HIGH */
            .initial_probability = 0.01f,
            .on_pick_multiplier = 0.8f,
            .winner_weight_share = 1.0f,
        },
        { /* CHAOS_DISTURBANCE_NIGHTMARE */
            .initial_probability = 0.0f,
            .on_pick_multiplier = 0.5f,
            .winner_weight_share = 1.0f,
        },
    }
};

// Initialize custom machines
RECOMP_DECLARE_EVENT(chaos_on_init_passive_effects(ChaosMachine* machine));

RECOMP_CALLBACK("mm_recomp_chaos_framework", chaos_on_init)
void register_custom_machines() {
    ChaosMachine* chaosMachinePassive = chaos_register_machine(&chaosSettingsPassive);
    chaos_on_init_passive_effects(chaosMachinePassive);
}