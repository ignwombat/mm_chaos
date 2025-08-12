#include "overlays/actors/ovl_En_Bom/z_en_bom.h"
#include "chaos.h"

void Spawn_Bomb(PlayState* play, Player* player) {
    Vec3f* playerPos = &player->actor.world.pos;

    EnBom* bomb = (EnBom*)Actor_Spawn(
        &play->actorCtx,
        play,
        ACTOR_EN_BOM,
        playerPos->x,
        playerPos->y,
        playerPos->z,
        BOMB_EXPLOSIVE_TYPE_POWDER_KEG,
        0,
        0,
        BOMB_TYPE_BODY
    );

    bomb->timer = 80 + Rand_ZeroFloat(80);

    bomb->actor.world.pos.y += 150;
    bomb->actor.velocity.y = 0;

    // TODO place bomb in front of player based on direction
}

u8 bombInterval = 35; // Frame interval for how often bombs should spawn
u8 bombTimer = 0;

void Bombing_Start(GraphicsContext* gfxCtx, GameState* gameState) {
    PlayState* play = (PlayState*)gameState;
    Player* player = GET_PLAYER(play);
    Vec3f* playerPos = &player->actor.world.pos;
    
    bombTimer = 1;
    Player_PlaySfx(player, NA_SE_EN_STAL01_LAUGH);
}

void Bombing_Update(GraphicsContext* gfxCtx, GameState* gameState) {
    PlayState* play = (PlayState*)gameState;
    Player* player = GET_PLAYER(play);

    bombTimer--;
    if (bombTimer == 0) {
        Spawn_Bomb(play, player);
        bombTimer = bombInterval + Rand_CenteredFloat(bombInterval / 3);
    }
}

void Bombing_End(GraphicsContext* gfxCtx, GameState* gameState) {
    
}

ChaosEffect bombing = {
    .name = "Bombing",
    .duration = 20 * 15, // 15 seconds
    .on_start_fun = Bombing_Start,
    .update_fun = Bombing_Update,
    .on_end_fun = Bombing_End
};

RECOMP_CALLBACK("mm_recomp_chaos_framework", chaos_on_init)
void register_bombing() {
    chaos_register_effect(&bombing, CHAOS_DISTURBANCE_VERY_HIGH, NULL);
}