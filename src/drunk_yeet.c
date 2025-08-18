#include "chaos.h"

static Actor* heldItem;
static bool lastFrameHeld = false;
static bool isExplosive = false;

void DrunkYeet_Update(PlayState* play) {
    Player* player = GET_PLAYER(play);

    bool currentlyHeld = player->stateFlags1 & PLAYER_STATE1_CARRYING_ACTOR;

    if (!lastFrameHeld && currentlyHeld) {
        heldItem = player->heldActor;
        isExplosive = Player_GetExplosiveHeld(player) != PLAYER_EXPLOSIVE_NONE;
    }

    else if (lastFrameHeld && !currentlyHeld) {
        if (isExplosive) {
            heldItem->world.rot.y = Rand_ZeroFloat(0x10000);
        }
    }

    lastFrameHeld = currentlyHeld;
}

ChaosEffect drunkYeet = {
    .name = "Wommy Drunk YEET",
    .duration = 20 * 60, // 60 seconds
    .update_fun = DrunkYeet_Update
};

RECOMP_CALLBACK(".", chaos_on_init_passive_effects)
void register_drunk_yeet(ChaosMachine* machine) {
    chaos_register_effect_to(machine, &drunkYeet, CHAOS_DISTURBANCE_VERY_LOW, NULL);
}