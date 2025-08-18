#include "overlays/actors/ovl_En_Bom/z_en_bom.h"
#include "chaos.h"
#include "wommyutils.h"

void Spawn_Bomb(PlayState* play, Player* player) {
    Vec3f* playerPos = &player->actor.world.pos;

    // Place the bomb in front of the player LOOL
    Vec2f direction = ActorDirection(&player->actor);
    f32 distance = 100 + Rand_ZeroFloat(150);

    EnBom* bomb = (EnBom*)Actor_Spawn(
        &play->actorCtx,
        play,
        ACTOR_EN_BOM,
        playerPos->x + direction.x * distance,
        playerPos->y + 170,
        playerPos->z + direction.z * distance,
        BOMB_EXPLOSIVE_TYPE_POWDER_KEG,
        0,
        0,
        BOMB_TYPE_BODY
    );

    if (bomb != NULL) {
        bomb->timer = 80 + Rand_ZeroFloat(80);
        bomb->actor.velocity.y = 0;
    }
}

static const u8 bombInterval = 37; // Frame interval for how often bombs should spawn
static u8 bombTimer = 0;

void Bombing_Start(PlayState* play) {
    Player* player = GET_PLAYER(play);
    
    bombTimer = 1;
}

void Bombing_Update(PlayState* play) {
    Player* player = GET_PLAYER(play);

    bombTimer--;
    if (bombTimer == 0) {
        Spawn_Bomb(play, player);
        bombTimer = bombInterval + Rand_CenteredFloat(bombInterval / 3);
    }
}

ChaosEffect bombing = {
    .name = "Wommy Bombing",
    .duration = 20 * 13, // 13 seconds
    .on_start_fun = Bombing_Start,
    .update_fun = Bombing_Update
};

RECOMP_CALLBACK("mm_recomp_chaos_framework", chaos_on_init)
void register_bombing() {
    chaos_register_effect(&bombing, CHAOS_DISTURBANCE_HIGH, NULL);
}