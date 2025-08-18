#include "chaos.h"
#include "chaos_exclusivity.h"
#include "eztr_api.h"

static ChaosEffectEntity *bazingaEntity;

EZTR_DEFINE_CUSTOM_MSG_HANDLE(bazinga_msg);
EZTR_ON_INIT void init_bazinga() {
    EZTR_Basic_AddCustomText(
        EZTR_HNAME(bazinga_msg),
        EZTR_STANDARD_TEXT_BOX_I,
        2,
        EZTR_ICON_DEKU_NUT,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        false,

        EZTR_CC_SFX_ARGW EZTR_CC_QUICKTEXT_DISABLE EZTR_CC_COLOR_GREEN "BAZINGA" EZTR_CC_COLOR_DEFAULT "!" EZTR_CC_DELAY_ARGW
        EZTR_CC_NEWLINE "Press " EZTR_CC_BTN_A EZTR_CC_DELAY_ARGW " or " EZTR_CC_DELAY_ARGW EZTR_CC_BTN_B
        EZTR_CC_NEWLINE "if you wish to"
        EZTR_CC_NEWLINE "dismiss this message."
        EZTR_CC_END,

        NULL,
        NA_SE_IT_DEKU,
        18,
        15,
        15
    );
}


// Who is bazinga? Why is bazinga?
static bool isBazinga = false;
static bool lastFrameBazinga = false;
static u16 bazingaTimer = 0;
void Bazinga_Start(PlayState* play) {
    Player* player = GET_PLAYER(play);

    if (
        isBazinga ||
        Message_GetState(&play->msgCtx) != TEXT_STATE_NONE ||
        player->tatlTextId != 0 ||
        play->transitionTrigger != TRANS_TRIGGER_OFF ||
        play->gameOverCtx.state != GAMEOVER_INACTIVE
    ) return;

    isBazinga = true;
    bazingaTimer = (20 * 30); // Stays for 30 seconds
    lastFrameBazinga = false;

    player->tatlTextId = EZTR_GET_ID_H(bazinga_msg);
}

void Bazinga_Update(PlayState* play) {
    if (!isBazinga) {
        chaos_stop_effect(bazingaEntity);
        return;
    };

    Player* player = GET_PLAYER(play);
    if (bazingaTimer > 0)
        bazingaTimer--;

    if (bazingaTimer == 0) {
        isBazinga = false;
        chaos_stop_effect(bazingaEntity);
        return;
    }

    if (isBazinga) {
        bool isInBazinga = play->msgCtx.currentTextId == EZTR_GET_ID_H(bazinga_msg);

        if (lastFrameBazinga && !isInBazinga) {
            // The message has been closed
            isBazinga = false;
            chaos_stop_effect(bazingaEntity);
            return;
        }

        else if (!lastFrameBazinga && !isInBazinga) {
            player->tatlTextId = EZTR_GET_ID_H(bazinga_msg);
        }

        lastFrameBazinga = isInBazinga;
    }
}

ChaosEffect bazinga = {
    .name = "Wommy Bazinga",
    .duration = CHAOS_DURATION_MAX, // Controlled manually
    .on_start_fun = Bazinga_Start,
    .update_fun = Bazinga_Update
};

RECOMP_CALLBACK("mm_recomp_chaos_framework", chaos_on_init)
void register_bazinga() {
    bazingaEntity = chaos_register_effect(&bazinga, CHAOS_DISTURBANCE_LOW, &CHAOS_TAG_DIALOG);
}