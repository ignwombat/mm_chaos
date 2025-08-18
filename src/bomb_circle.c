#include "overlays/actors/ovl_En_Bom/z_en_bom.h"
#include "chaos.h"
#include "wommyutils.h"

void Spawn_BombCircle(PlayState* play, Player* player) {
    Vec3f playerPos = player->actor.world.pos;

    const int bombCount = 8;
    for (int i = 0; i < bombCount; i++) {
        s16 angle = (s16)((i * (0x10000 / bombCount)) & 0xFFFF);

        // Position around the player
        f32 offsetX = Math_SinS(angle) * 80;
        f32 offsetZ = Math_CosS(angle) * 80;

        EnBom* bomb = (EnBom*)Actor_Spawn(
            &play->actorCtx,
            play,
            ACTOR_EN_BOM,
            playerPos.x + offsetX,
            playerPos.y + 60.0f,
            playerPos.z + offsetZ,
            BOMB_EXPLOSIVE_TYPE_BOMB,
            angle,
            0,
            BOMB_TYPE_BODY
        );

        if (bomb != NULL) {
            bomb->timer = 85 - i * 2;
            bomb->actor.speed = 3.4f;

            bomb->actor.velocity.y = 6;
        }
    }
}

void BombCircle_Start(PlayState* play) {
    Player* player = GET_PLAYER(play);
    
    Spawn_BombCircle(play, player);
}

ChaosEffect bombCircle = {
    .name = "Wommy Bomb Circle",
    .duration = 0, // Instant
    .on_start_fun = BombCircle_Start
};

RECOMP_CALLBACK("mm_recomp_chaos_framework", chaos_on_init)
void register_bomb_circle() {
    chaos_register_effect(&bombCircle, CHAOS_DISTURBANCE_LOW, NULL);
}