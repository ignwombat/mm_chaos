#include "overlays/actors/ovl_En_Crow/z_en_crow.h"
#include "chaos.h"
#include "wommyutils.h"
#include "recomputils.h"

static ChaosEffectEntity *guayEntity;

static EnCrow* guay;
void Spawn_Guay(PlayState* play, Player* player) {
    Vec3f* playerPos = &player->actor.world.pos;

    // Place the guay behind the camera LOOL
    Camera* cam = Play_GetCamera(play, Play_GetActiveCamId(play));
    Vec3f pos = cam->eye;

    Vec3f diff = {
        .x = pos.x - playerPos->x,
        .y = pos.y - playerPos->y,
        .z = pos.z - playerPos->z
    };

    guay = (EnCrow*)Actor_Spawn(
        &play->actorCtx,
        play,
        ACTOR_EN_CROW,
        pos.x + diff.x / 4,
        pos.y,
        pos.z + diff.z / 4,
        0,
        0,
        0,
        0
    );
}

void SneakyGuay_Start(PlayState* play) {
    Player* player = GET_PLAYER(play);
    Spawn_Guay(play, player);
}

void SneakyGuay_Update(PlayState* play) {
    // Return if the guay was despawned
    if (guay == NULL || guay->actor.update == NULL) {
        chaos_stop_effect(guayEntity);
        return;
    };

    Player* player = GET_PLAYER(play);
    
    // Speed up the guay LOL
    guay->actor.speed = 10;
    if (guay->timer > 1)
        guay->timer -= 1;
    
    // Set the guay's home in front of the player
    Vec3f* playerPos = &player->actor.world.pos;
    Vec2f playerDir = ActorDirection(&player->actor);

    guay->actor.home.pos.x = playerPos->x + playerDir.x * 200;
    guay->actor.home.pos.z = playerPos->z + playerDir.z * 200;
    guay->actor.home.pos.y = playerPos->y + 100;
}

void SneakyGuay_End(PlayState* play) {
    // Return if the guay was despawned
    if (guay == NULL || guay->actor.update == NULL) return;
    Actor_Kill(&guay->actor);
}

ChaosEffect sneakyGuay = {
    .name = "Wommy Sneaky Guay",
    .duration = 20 * 10, // 10 seconds
    .on_start_fun = SneakyGuay_Start,
    .update_fun = SneakyGuay_Update,
    .on_end_fun = SneakyGuay_End
};

RECOMP_CALLBACK("mm_recomp_chaos_framework", chaos_on_init)
void register_sneaky_guay() {
    guayEntity = chaos_register_effect(&sneakyGuay, CHAOS_DISTURBANCE_MEDIUM, NULL);
}