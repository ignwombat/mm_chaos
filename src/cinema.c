#include "chaos.h"
#include "chaos_exclusivity.h"
#include "assets/cinema.h"
#include <string.h>

#define TILES_X 4
#define TILES_Y 4

#define TILE_W 64
#define TILE_H 64

#define HALF_TILE_H 32
#define TILE_SIZE (TILE_W * TILE_H)
#define HALF_TILE_SIZE (TILE_W * HALF_TILE_H)

typedef enum {
    CINEMA_OFF,
    CINEMA_FADING_IN,
    CINEMA_SHOWING,
    CINEMA_FADING_OUT
} CinemaState;

static CinemaState cinemaState = CINEMA_OFF;
static u8 cinemaAlpha = 0;
static s32 cinemaFrameTimer = 0;

const u8 fadeInSpeed = 32;
const u8 fadeOutSpeed = 20;
const u8 cinemaFrameCount = 20;

static u16* CinemaTiles[TILES_Y][TILES_X] = {
    { Cinema1_1,  Cinema2_1,  Cinema3_1,  Cinema4_1 },
    { Cinema1_2,  Cinema2_2,  Cinema3_2,  Cinema4_2 },
    { Cinema1_3,  Cinema2_3,  Cinema3_3,  Cinema4_3 },
    { Cinema1_4,  Cinema2_4,  Cinema3_4,  Cinema4_4 }
};

// New split tile storage
static u16 CinemaTiles_TopData[TILES_Y][TILES_X][HALF_TILE_SIZE];
static u16 CinemaTiles_BottomData[TILES_Y][TILES_X][HALF_TILE_SIZE];

static u16* CinemaTiles_Top[TILES_Y][TILES_X];
static u16* CinemaTiles_Bottom[TILES_Y][TILES_X];

void SplitAllCinemaTiles(void) {
    for (s32 y = 0; y < TILES_Y; y++) {
        for (s32 x = 0; x < TILES_X; x++) {
            u16* src = CinemaTiles[y][x];
            u16* topDst = CinemaTiles_TopData[y][x];
            u16* botDst = CinemaTiles_BottomData[y][x];

            for (s32 row = 0; row < HALF_TILE_H; row++) {
                // Copy top half row
                memcpy(&topDst[row * TILE_W],
                       &src[row * TILE_W],
                       TILE_W * sizeof(u16));

                // Copy bottom half row
                memcpy(&botDst[row * TILE_W],
                       &src[(row + HALF_TILE_H) * TILE_W],
                       TILE_W * sizeof(u16));
            }

            // Assign to pointer arrays
            CinemaTiles_Top[y][x] = topDst;
            CinemaTiles_Bottom[y][x] = botDst;
        }
    }
}

RECOMP_CALLBACK("*", recomp_after_play_init)
void Cinema_AfterPlayInit() {
    SplitAllCinemaTiles();
}

void DrawCinema(PlayState* play) {
    OPEN_DISPS(play->state.gfxCtx);

    // Draw fullscreen white overlay behind the cinema image
    // Fullscreen size on N64 for overlay is usually 320x240 (NTSC) or 320x240

    // Coordinates in fixed-point (x << 2), so multiply by 4
    s32 screen_ulx = 0 << 2;
    s32 screen_uly = 0 << 2;
    s32 screen_lrx = 320 << 2;
    s32 screen_lry = 240 << 2;

    gDPPipeSync(OVERLAY_DISP++);
    gDPSetCycleType(OVERLAY_DISP++, G_CYC_1CYCLE);
    gDPSetRenderMode(OVERLAY_DISP++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
    gDPSetTextureLUT(OVERLAY_DISP++, G_TT_NONE);
    gSPClearGeometryMode(OVERLAY_DISP++, G_SHADE | G_SHADING_SMOOTH | G_FOG | G_LIGHTING);
    gSPTexture(OVERLAY_DISP++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    gDPSetCombineMode(OVERLAY_DISP++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
    gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 255, 255, 255, cinemaAlpha); // Opaque white

    const s32 startX = 40;
    const s32 startY = 30;

    for (s32 y = 0; y < TILES_Y; y++) {
        for (s32 x = 0; x < TILES_X; x++) {
            s32 ulx = (startX + x * TILE_W) << 2;
            s32 uly = (startY + y * TILE_H) << 2;
            s32 lrx = (startX + (x + 1) * TILE_W) << 2;
            s32 midy = (startY + y * TILE_H + HALF_TILE_H) << 2;
            s32 lry = (startY + (y + 1) * TILE_H) << 2;

            // Top half (64×32)
            gDPLoadTextureBlock(
                OVERLAY_DISP++,
                CinemaTiles_Top[y][x],
                G_IM_FMT_IA, G_IM_SIZ_16b,
                TILE_W, HALF_TILE_H,
                0,
                G_TX_WRAP | G_TX_NOMIRROR,
                G_TX_WRAP | G_TX_NOMIRROR,
                G_TX_NOMASK, G_TX_NOMASK,
                G_TX_NOLOD, G_TX_NOLOD
            );

            gSPTextureRectangle(
                OVERLAY_DISP++,
                ulx, uly, lrx, midy,
                G_TX_RENDERTILE,
                0 << 5, 0 << 5,
                1 << 10, 1 << 10
            );

            // Bottom half (64×32)
            gDPLoadTextureBlock(
                OVERLAY_DISP++,
                CinemaTiles_Bottom[y][x],
                G_IM_FMT_IA, G_IM_SIZ_16b,
                TILE_W, HALF_TILE_H,
                0,
                G_TX_WRAP | G_TX_NOMIRROR,
                G_TX_WRAP | G_TX_NOMIRROR,
                G_TX_NOMASK, G_TX_NOMASK,
                G_TX_NOLOD, G_TX_NOLOD
            );

            gSPTextureRectangle(
                OVERLAY_DISP++,
                ulx, midy, lrx, lry,
                G_TX_RENDERTILE,
                0 << 5, 0 << 5,
                1 << 10, 1 << 10
            );
        }
    }

    gDPPipeSync(OVERLAY_DISP++);
    CLOSE_DISPS(play->state.gfxCtx);
}

RECOMP_HOOK("Interface_Draw")
void DrawCinema_Overlay(PlayState* play) {
    if (cinemaState == CINEMA_OFF) return;

    if (cinemaAlpha > 0) {
        DrawCinema(play);
    }
}

void UpdateCinemaState() {
    switch (cinemaState) {
        case CINEMA_FADING_IN:
            if (cinemaAlpha < 255) {
                u16 next = cinemaAlpha + fadeInSpeed;
                
                if (next >= 255) {
                    cinemaAlpha = 255;
                    cinemaState = CINEMA_SHOWING;
                    cinemaFrameTimer = cinemaFrameCount; // show for 40 frames
                }

                else {
                    cinemaAlpha = next;
                }
            }
            break;

        case CINEMA_SHOWING:
            if (--cinemaFrameTimer <= 0) {
                cinemaState = CINEMA_FADING_OUT;
            }
            break;

        case CINEMA_FADING_OUT:
            if (cinemaAlpha > 0) {
                s16 next = cinemaAlpha - fadeOutSpeed;
                if (next <= 0) {
                    cinemaAlpha = 0;
                    cinemaState = CINEMA_OFF;
                }

                else {
                  cinemaAlpha = next;
                }
            }
            break;

        case CINEMA_OFF:
        default:
            break;
    }
}

void Cinema_Start(PlayState* play) {
    Player* plr = GET_PLAYER(play);

    cinemaAlpha = 0;
    cinemaState = CINEMA_FADING_IN;

    Audio_PlaySfx(NA_SE_IT_BIG_BOMB_EXPLOSION);
    Audio_PlaySfx(NA_SE_IT_BOMB_EXPLOSION);
    Audio_PlaySfx(NA_SE_IT_BOMB_EXPLOSION2);
    Audio_PlaySfx(NA_SE_EV_EXPLSION_LONG);
}

void Cinema_Update(PlayState* play) {
    if (cinemaState == CINEMA_OFF) return;
    UpdateCinemaState();
}

void Cinema_End(PlayState* play) {
    cinemaState = CINEMA_OFF;
}

ChaosEffect cinema = {
    .name = "Wommy CINEMA",
    .duration = (255 / fadeInSpeed) + cinemaFrameCount + (255 / fadeOutSpeed),
    .on_start_fun = Cinema_Start,
    .update_fun = Cinema_Update,
    .on_end_fun = Cinema_End
};

RECOMP_CALLBACK("mm_recomp_chaos_framework", chaos_on_init)
void register_binema() {
    chaos_register_effect(&cinema, CHAOS_DISTURBANCE_MEDIUM, &CHAOS_TAG_IMAGE);
}