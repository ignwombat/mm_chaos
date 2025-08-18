#include "chaos.h"
#include "chaos_exclusivity.h"
#include "assets/foxy.h"
#include "audio_api.h"

#define TILES_X 4
#define TILES_Y 2

#define TILE_W 64
#define TILE_H 64

#define HALF_TILE_H 32
#define TILE_SIZE (TILE_W * TILE_H)
#define HALF_TILE_SIZE (TILE_W * HALF_TILE_H)

static u16* FoxyTiles[TILES_Y][TILES_X] = {
    { Foxy1_1,  Foxy2_1,  Foxy3_1,  Foxy4_1 },
    { Foxy1_2,  Foxy2_2,  Foxy3_2,  Foxy4_2 }
};

static s32 newSeqId;

RECOMP_CALLBACK("magemods_audio_api", AudioApi_Init)
void FnafJumpscare_Init() {
    INCBIN(fnaf_jumpscare_sound, "src/audio/fnaf_jumpscare.raw");

    AudioTableEntry entry = {
        (uintptr_t) fnaf_jumpscare_sound,
        fnaf_jumpscare_sound_end - fnaf_jumpscare_sound,
        MEDIUM_CART,
        CACHE_EITHER,
        0, 0, 0
    };

    newSeqId = AudioApi_AddSequence(&entry);
    AudioApi_AddSequenceFont(newSeqId, 0x03);
}

void FnafJumpscare_Start(PlayState* play) {
    AudioApi_StartSequence(SEQ_PLAYER_SFX, newSeqId, 0, 0);
}

ChaosEffect fnafJumpscare = {
    .name = "WommyFnafJumpscare",
    .duration = 20 * 5, // 5 seconds
    .on_start_fun = FnafJumpscare_Start
};

RECOMP_CALLBACK("mm_recomp_chaos_framework", chaos_on_init)
void register_fnaf_jumpscare() {
    chaos_register_effect(&fnafJumpscare, CHAOS_DISTURBANCE_HIGH, &CHAOS_TAG_IMAGE);
}