#include "chaos.h"
#include "z64save.h"
#include "z64scene.h"
#include "recomputils.h"
#include "scene_entrances.h"

#define MAX_ENTRIES ENTR_SCENE_MAX - 1
#define MAX_ATTEMPTS 3

// List of scenes that should not be used
static EntranceSceneId disallowedWarps[] = {
    // Unset scenes
    ENTR_SCENE_UNSET_08,
    ENTR_SCENE_UNSET_09,
    ENTR_SCENE_UNSET_0B,
    ENTR_SCENE_UNSET_0C,
    ENTR_SCENE_UNSET_0D,
    ENTR_SCENE_UNSET_0F,
    ENTR_SCENE_UNSET_2E,
    ENTR_SCENE_UNSET_37,

    // These work, but are temporarily disabled for debugging purposes
    /*ENTR_SCENE_MAYORS_RESIDENCE,
    ENTR_SCENE_MAGIC_HAGS_POTION_SHOP,
    ENTR_SCENE_GORON_SHOP,
    ENTR_SCENE_GORON_SHRINE,
    ENTR_SCENE_TOWN_SHOOTING_GALLERY,
    ENTR_SCENE_SWAMP_SHOOTING_GALLERY,
    ENTR_SCENE_FISHERMANS_HUT,
    ENTR_SCENE_MILK_BAR,
    ENTR_SCENE_HONEY_AND_DARLINGS_SHOP,
    ENTR_SCENE_CUCCO_SHACK,
    ENTR_SCENE_POST_OFFICE,
    ENTR_SCENE_RANCH_HOUSE,
    ENTR_SCENE_SWORDMANS_SCHOOL,
    ENTR_SCENE_MUSIC_BOX_HOUSE,
    ENTR_SCENE_GHOST_HUT,
    ENTR_SCENE_LOTTERY_SHOP,
    ENTR_SCENE_MARINE_RESEARCH_LAB,
    ENTR_SCENE_TRADING_POST,
    ENTR_SCENE_BOMB_SHOP,
    ENTR_SCENE_STONE_TOWER_TEMPLE,
    ENTR_SCENE_STONE_TOWER_TEMPLE_INVERTED,
    ENTR_SCENE_SAKONS_HIDEOUT,
    ENTR_SCENE_DAMPES_HOUSE,
    ENTR_SCENE_SWAMP_SPIDER_HOUSE,
    ENTR_SCENE_SNOWHEAD,
    ENTR_SCENE_PATH_TO_MOUNTAIN_VILLAGE,
    ENTR_SCENE_MOUNTAIN_VILLAGE_WINTER,
    ENTR_SCENE_OCEANSIDE_SPIDER_HOUSE,
    ENTR_SCENE_WOODFALL,
    ENTR_SCENE_ROAD_TO_SOUTHERN_SWAMP,
    ENTR_SCENE_SNOWHEAD_TEMPLE,
    ENTR_SCENE_ZORA_HALL,
    ENTR_SCENE_GORON_RACETRACK,
    ENTR_SCENE_BENEATH_THE_WELL,
    ENTR_SCENE_WOODS_OF_MYSTERY,
    //ENTR_SCENE_SOUTHERN_SWAMP_CLEARED,
    ENTR_SCENE_TREASURE_CHEST_SHOP,
    ENTR_SCENE_SECRET_SHRINE,
    ENTR_SCENE_DEKU_SCRUB_PLAYGROUND,
    ENTR_SCENE_DEKU_SHRINE,
    ENTR_SCENE_MOUNTAIN_SMITHY,
    ENTR_SCENE_FAIRY_FOUNTAIN,
    ENTR_SCENE_ASTRAL_OBSERVATORY,
    ENTR_SCENE_NORTH_CLOCK_TOWN,
    ENTR_SCENE_GORON_VILLAGE_SPRING,
    ENTR_SCENE_GORON_VILLAGE_WINTER,
    ENTR_SCENE_GORON_GRAVERYARD,
    ENTR_SCENE_DOGGY_RACETRACK,
    ENTR_SCENE_STONE_TOWER,
    ENTR_SCENE_DEKU_KINGS_CHAMBER,
    ENTR_SCENE_GREAT_BAY_TEMPLE,*/


    // Crashes
    ENTR_SCENE_CUTSCENE,
    
    // Softlocks
    
    // Too annoying
    ENTR_SCENE_LOST_WOODS,
    ENTR_SCENE_GREAT_BAY_CUTSCENE,
    ENTR_SCENE_PINNACLE_ROCK,
    
    // Progression
    ENTR_SCENE_CLOCK_TOWER_ROOFTOP,
    
    // The moon
    ENTR_SCENE_THE_MOON,
    ENTR_SCENE_MOON_DEKU_TRIAL,
    ENTR_SCENE_MOON_GORON_TRIAL,
    ENTR_SCENE_MOON_ZORA_TRIAL,
    ENTR_SCENE_MOON_LINK_TRIAL,
    
    // Bosses
    ENTR_SCENE_ODOLWAS_LAIR,
    ENTR_SCENE_GOHTS_LAIR,
    ENTR_SCENE_GYORGS_LAIR,
    ENTR_SCENE_TWINMOLDS_LAIR,
    ENTR_SCENE_MAJORAS_LAIR,
    ENTR_SCENE_IGOS_DU_IKANAS_LAIR,
    ENTR_SCENE_GIANTS_CHAMBER
};

typedef struct BadSpawnEntry {
    EntranceSceneId scene;
    const u16 *badSpawns;
    size_t badCount;
} BadSpawnEntry;

#define NUMARGS(...)  (sizeof((int[]){__VA_ARGS__})/sizeof(int))

#define BAD_SPAWN(scene, ...) \
    { ENTR_SCENE_##scene, (const u16[]){ __VA_ARGS__ }, NUMARGS(__VA_ARGS__) }

#define BAD_SPAWNS(...) \
    static const size_t badSpawnCount = sizeof((BadSpawnEntry[]){ __VA_ARGS__ }) / sizeof(BadSpawnEntry); \
    static const BadSpawnEntry badSpawns[] = { __VA_ARGS__ };

// List of spawn nums that should not be used
BAD_SPAWNS(
    // Crashes + etc
    BAD_SPAWN(IKANA_GRAVEYARD, 1, 5), // 5 = captain ending cutscene
    BAD_SPAWN(TWINMOLDS_LAIR, 4),
    BAD_SPAWN(STONE_TOWER_TEMPLE_INVERTED, 2, 3), // 3 = Twinmold entrance
    BAD_SPAWN(STONE_TOWER, 1),
    BAD_SPAWN(LAUNDRY_POOL, 2),
    BAD_SPAWN(GORON_VILLAGE_SPRING, 1, 3), // 3 = softlock
    BAD_SPAWN(DEKU_SHRINE, 2),
    BAD_SPAWN(PIRATES_FORTRESS_INTERIOR, 11, 12, 13, 14, 15, 32), // 11 = JP spawn?
    BAD_SPAWN(PIRATES_FORTRESS, 9, 11, 14), // 9, 11 = Softlock
    BAD_SPAWN(GREAT_BAY_TEMPLE, 1, 2, 6), // 1, 6 = long entry cutscene
    BAD_SPAWN(NORTH_CLOCK_TOWN, 12),

    // Softlocks

    // Too annoying
    BAD_SPAWN(OPENING_DUNGEON, 0, 2, 4), // 0 = Fall, 2 = Turned into deku scrub, 4 = First cycle reset
    BAD_SPAWN(GORON_RACETRACK, 1), // Race start
    BAD_SPAWN(TERMINA_FIELD, 12), // Moon crash
    BAD_SPAWN(CLOCK_TOWER_INTERIOR, 2, 4, 6), // 2 = Deku mask drop, 4, 6 = Long ahh dialog with mask salesman
    BAD_SPAWN(DEKU_KINGS_CHAMBER, 2),
    BAD_SPAWN(IKANA_CANYON, 9, 10), // 9 = Music box playing, 10 = Joining the dead
    BAD_SPAWN(GREAT_BAY_COAST, 3), // Middle of the water
    BAD_SPAWN(WATERFALL_RAPIDS, 0, 1, 2),

    // Progression
    BAD_SPAWN(GORON_GRAVERYARD, 1), // Goron mask drop
    BAD_SPAWN(FAIRY_FOUNTAIN, 5, 6, 7, 8), // Fairy fountain items+upgrades
    BAD_SPAWN(ROMANI_RANCH, 1), // 1 = Eponas song
    BAD_SPAWN(WOODFALL_TEMPLE, 1, 2) // Deku princess

    // Bosses
);

static bool WarpIsBad(EntranceSceneId entrance) {
    for (size_t i = 0; i < ARRAY_COUNT(disallowedWarps); i++) {
        if (disallowedWarps[i] == entrance) {
            return true;
        }
    }

    return false;
}

static bool SpawnIsBad(EntranceSceneId scene, u8 spawnNum) {
    for (size_t i = 0; i < badSpawnCount; i++) {
        if (badSpawns[i].scene == scene) {
            for (size_t j = 0; j < badSpawns[i].badCount; j++) {
                if (badSpawns[i].badSpawns[j] == spawnNum) {
                    return true;
                }
            }

            break;
        }
    }

    return false;
}

typedef struct EntranceResult {
    u16 entranceId;
    u16 spawnNum;
    u16 transitionFlags;
} EntranceResult;

EntranceResult emptyResult = {
    .entranceId = 0,
    .spawnNum = 0,
    .transitionFlags = 0
};

EntranceResult FindRandomEntrance(u8 failedAttempts) {
    u16 goodIndexes[MAX_ENTRIES];
    u16 badIndexes[MAX_ENTRIES];

    u16 goodCount = 0;
    u16 badCount = 0;

    for (u16 i = 0; i < MAX_ENTRIES; i++) {
        SceneEntranceTableEntry* entry = &sSceneEntranceTable[i];
        if (
            entry->tableCount > 0 &&
            entry->table != NULL &&
            !WarpIsBad((EntranceSceneId)i)
        ) {
            goodIndexes[goodCount++] = i;
        }
    }

    if (goodCount == 0) return emptyResult; // fallback

    // Pick random valid scene
    u16 sceneIndex = goodIndexes[Rand_Next() % goodCount];
    if (sceneIndex > MAX_ENTRIES) {
        sceneIndex = goodIndexes[Rand_Next() % goodCount];
        if (sceneIndex > MAX_ENTRIES) {
            if (failedAttempts < MAX_ATTEMPTS)
                return FindRandomEntrance(failedAttempts + 1);

            return emptyResult;
        }
    }

    SceneEntranceTableEntry* sceneEntry = &sSceneEntranceTable[sceneIndex];

    // Pick random spawn table
    u16 tableCount = sceneEntry->tableCount;
    if (tableCount == 0) {
        if (failedAttempts < MAX_ATTEMPTS)
            return FindRandomEntrance(failedAttempts + 1);

        return emptyResult;
    }

    // Build temporary list of allowed spawns
    EntranceTableEntry* allowedSpawns[MAX_ENTRIES];
    u16 allowedCount = 0;

    for (u16 i = 0; i < tableCount; i++) {
        EntranceTableEntry* e = sceneEntry->table[i];
        if (!SpawnIsBad(sceneIndex, e->spawnNum)) {
            allowedSpawns[allowedCount++] = e;
        }
    }

    if (allowedCount == 0) {
        // No good spawns in this scene — try another scene
        if (failedAttempts < MAX_ATTEMPTS) {
            return FindRandomEntrance(failedAttempts + 1);
        }

        return emptyResult;
    }

    // Pick a random spawn point
    EntranceTableEntry* entry = allowedSpawns[Rand_Next() % allowedCount];

    recomp_printf(
        "------------------\n"
        "ENTRANCE: index %d\n"
        "sceneId = %d (%s)\n"
        "spawnNum = %d\n"
        "Failed attempts = %d\n"
        "Good index count = %d\n",
        sceneIndex,
        entry->sceneId,
        sceneEntry->name,
        entry->spawnNum,
        failedAttempts,
        goodCount
    );

    return (EntranceResult){
        .entranceId = sceneIndex,
        .spawnNum = entry->spawnNum,
        .transitionFlags = entry->flags
    };
}

// List of transition types that don't crash, break or are too boring
TransitionType transitionTypes[] = {
    TRANS_TYPE_WIPE,
    TRANS_TYPE_TRIFORCE,
    TRANS_TYPE_FADE_BLACK,
    TRANS_TYPE_FADE_WHITE,
    TRANS_TYPE_FADE_BLACK_FAST,
    TRANS_TYPE_FADE_WHITE_FAST,
    TRANS_TYPE_WIPE_FAST,
    TRANS_TYPE_FILL_WHITE_FAST,
    TRANS_TYPE_FILL_WHITE,
    TRANS_TYPE_FILL_BROWN,
    TRANS_TYPE_FADE_GREEN,
    TRANS_TYPE_FADE_BLUE,
    TRANS_TYPE_FADE_DYNAMIC
};

RECOMP_EXPORT

u16 wommy_random_warp(PlayState* play) {
    Player* player = GET_PLAYER(play);
    u32 stateFlags1 = player->stateFlags1;

    if (
        play->transitionTrigger != TRANS_TRIGGER_OFF ||
        play->gameOverCtx.state != GAMEOVER_INACTIVE ||
        (
            (stateFlags1 & ~PLAYER_STATE1_20 && // Busy
            !(stateFlags1 & (
                PLAYER_STATE1_FRIENDLY_ACTOR_FOCUS | // Not Z-targeting a friendly actor
                PLAYER_STATE1_PARALLEL | // Not parallel Z-targeting
                PLAYER_STATE1_4 | // Not climbing a ledge
                PLAYER_STATE1_8000000 // Not swimming
            )))
        ) ||
        (
            gSaveContext.buttonStatus[EQUIP_SLOT_B] == BTN_DISABLED &&
            !(stateFlags1 & PLAYER_STATE1_8000000) // Not swimming
        )
    ) return 0;

    // Pick a random scene from the table
    EntranceResult randomEntrance = FindRandomEntrance(0);
    if (randomEntrance.entranceId == 0 && randomEntrance.spawnNum == 0) return 0;

    play->nextEntrance = Entrance_Create(randomEntrance.entranceId, randomEntrance.spawnNum, 0);
    Scene_SetExitFade(play);

    TransitionType transType = transitionTypes[Rand_Next() % (ARRAY_COUNT(transitionTypes))];
    TransitionType transEndType = transType;

    /*recomp_printf(
        "Transitionflags %u\nu8 flags: %u\nShifted flags: %u\n",
        randomEntrance.transitionFlags,
        randomEntrance.transitionFlags & 0xFF,
        randomEntrance.transitionFlags >> 8
    );*/

    // The last 6 bytes of the u16 is the end transition type
    TransitionType actualTransition = randomEntrance.transitionFlags & 0x1F;
    if (actualTransition == TRANS_TYPE_CIRCLE)
        transEndType = TRANS_TYPE_CIRCLE;
    else
        for (u8 i = 0; i < ARRAY_COUNT(transitionTypes); i++) {
            if (transitionTypes[i] == actualTransition) {
                transEndType = actualTransition;
                break;
            }
        }
    
    play->transitionType = transType;
    gSaveContext.nextTransitionType = transEndType;

    play->transitionTrigger = TRANS_TRIGGER_START;
    gSaveContext.respawnFlag = 0;

    return play->nextEntrance;
}

ChaosEffectEntity *randomWarpEntity;
void RandomWarp_Start(PlayState* play) {
    if (wommy_random_warp(play) == 0) {
        chaos_stop_effect(randomWarpEntity);
        return;
    }

    Audio_PlaySfx(NA_SE_EN_PO_LAUGH);
}

ChaosEffect randomWarp = {
    .name = "Wommy Random Warp",
    .duration = 20 * 6, // 6 seconds
    .on_start_fun = RandomWarp_Start
};

RECOMP_CALLBACK("mm_recomp_chaos_framework", chaos_on_init)
void register_random_warp() {
    //randomWarpEntity = chaos_register_effect(&randomWarp, CHAOS_DISTURBANCE_NIGHTMARE, NULL);
}

//RECOMP_HOOK("Play_Update")
void on_update(PlayState* play) {
    Input* controller = CONTROLLER1(&play->state);
    if (CHECK_BTN_ALL(controller->press.button, BTN_L))
        wommy_random_warp(play);
}