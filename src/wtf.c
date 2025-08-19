#include "overlays/actors/ovl_En_Torch2/z_en_torch2.h"
#include "chaos.h"
#include "wommyutils.h"
#include "recomputils.h"

static ChaosEffectEntity *wtfEntity;

static EnTorch2* fakePlayer;
void Spawn_Player(PlayState* play, Player* player) {
    Audio_PlaySfx(NA_SE_EN_GOLON_KID_CRY);

    Vec3f* pos = &player->actor.world.pos;
    Vec2f dir = ActorDirection(&player->actor);

    fakePlayer = (EnTorch2*)Actor_Spawn(
        &play->actorCtx,
        play,
        ACTOR_EN_TORCH2,
        pos->x + dir.x * 80,
        pos->y,
        pos->z + dir.z * 80,
        0,
        player->actor.world.rot.y + (0x10000 / 2),
        0,
        TORCH2_PARAM_HUMAN
    );

    fakePlayer->state = TORCH2_STATE_SOLID;
    fakePlayer->framesUntilNextState = 1;
}

void WTF_Start(PlayState* play) {
    Player* player = GET_PLAYER(play);
    Spawn_Player(play, player);
}

void WTF_Update(PlayState* play) {
    // Return if the guay was despawned
    if (fakePlayer == NULL || fakePlayer->actor.update == NULL) {
        chaos_stop_effect(wtfEntity);
        return;
    };
}

void WTF_End(PlayState* play) {
    // Return if the guay was despawned
    if (fakePlayer == NULL || fakePlayer->actor.update == NULL) return;
    Actor_Kill(&fakePlayer->actor);
}

ChaosEffect wtf = {
    .name = "Wommy WTF (statue)",
    .duration = 20 * 15, // 15 seconds
    .on_start_fun = WTF_Start,
    .update_fun = WTF_Update,
    .on_end_fun = WTF_End
};

RECOMP_CALLBACK("mm_recomp_chaos_framework", chaos_on_init)
void register_wtf() {
    wtfEntity = chaos_register_effect(&wtf, CHAOS_DISTURBANCE_MEDIUM, NULL);
}