#include "overlays/actors/ovl_En_Bom/z_en_bom.h"
#include "chaos.h"
#include "wommyutils.h"

void Spawn_Gatling_Bomb(PlayState* play, Player* player) {
    Vec3f* playerPos = &player->actor.world.pos;
    Vec2f direction = ActorDirection(&player->actor);

    const int distance = 12;

    EnBom* bomb = (EnBom*)Actor_Spawn(
        &play->actorCtx,
        play,
        ACTOR_EN_BOM,
        playerPos->x + direction.x * distance,
        playerPos->y + 30,
        playerPos->z + direction.z * distance,
        BOMB_EXPLOSIVE_TYPE_BOMB,
        player->actor.world.rot.y + Rand_CenteredFloat(20 / 0x10000), // Up to a 10 degree offset
        0,
        BOMB_TYPE_BODY
    );

    if (bomb != NULL) {
        bomb->timer = 25 + (s16)Rand_ZeroFloat(8); // Between 25 and 33 frames
        bomb->actor.speed = 32;
        
        bomb->actor.gravity *= 0.5f;
        bomb->actor.velocity.y = 6;
    }
}

static const u8 bombInterval = 14; // Frame interval for how often bombs should spawn
static u8 bombTimer = 0;

void GatlingGun_Start(PlayState* play) {
    Player* player = GET_PLAYER(play);
    bombTimer = 1;
}

void GatlingGun_Update(PlayState* play) {
    Player* player = GET_PLAYER(play);

    bombTimer--;
    if (bombTimer == 0) {
        Spawn_Gatling_Bomb(play, player);
        bombTimer = bombInterval + Rand_CenteredFloat(bombInterval / 3);
    }
}

ChaosEffect gatlingGun = {
    .name = "Wommy Gatling Gun",
    .duration = 20 * 6, // 6 seconds
    .on_start_fun = GatlingGun_Start,
    .update_fun = GatlingGun_Update
};

RECOMP_CALLBACK("mm_recomp_chaos_framework", chaos_on_init)
void register_gatling_gun() {
    chaos_register_effect(&gatlingGun, CHAOS_DISTURBANCE_HIGH, NULL);
}