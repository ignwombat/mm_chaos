#include "chaos.h"
#include "chaos_exclusivity.h"
#include "eztr_api.h"

static ChaosEffectEntity *bankRobbedEntity;

EZTR_DEFINE_CUSTOM_MSG_HANDLE(bank_robbed_msg);
EZTR_ON_INIT void init_bank_robbed() {
    EZTR_Basic_AddCustomText(
        EZTR_HNAME(bank_robbed_msg),
        EZTR_STANDARD_TEXT_BOX_I,
        2,
        EZTR_ICON_GIANTS_WALLET,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        false,

        EZTR_CC_QUICKTEXT_DISABLE EZTR_CC_COLOR_RED "Oh no" EZTR_CC_COLOR_DEFAULT "! The bank was " EZTR_CC_DELAY_ARGW
        EZTR_CC_COLOR_RED "robbed" EZTR_CC_COLOR_DEFAULT "!"
        EZTR_CC_NEWLINE "All your precious rupees were " EZTR_CC_COLOR_RED "lost" EZTR_CC_COLOR_DEFAULT "!"
        EZTR_CC_NEWLINE "Blame Sakon..."
        EZTR_CC_END,

        NULL,
        18
    );
}

static bool isBankRobbed = false;
void BankRobbed_Start(PlayState* play) {
    if (
        isBankRobbed ||
        Message_GetState(&play->msgCtx) != TEXT_STATE_NONE ||
        play->transitionTrigger != TRANS_TRIGGER_OFF ||
        play->gameOverCtx.state != GAMEOVER_INACTIVE ||
        HS_GET_BANK_RUPEES() < 50 // We don't rob broke people
    ) return;

    isBankRobbed = true;

    HS_SET_BANK_RUPEES(0);
    Message_StartTextbox(play, EZTR_GET_ID_H(bank_robbed_msg), NULL);
    Audio_PlaySfx(NA_SE_SY_QUIZ_INCORRECT);
}

void BankRobbed_Update(PlayState* play) {
    if (isBankRobbed && play->msgCtx.currentTextId != EZTR_GET_ID_H(bank_robbed_msg)) {
        isBankRobbed = false;

        // Re-enable the chaos effect
        chaos_stop_effect(bankRobbedEntity);
    }
}

ChaosEffect bankRobbed = {
    .name = "Wommy Bank Robbed",
    .duration = CHAOS_DURATION_MAX, // Controlled manually
    .on_start_fun = BankRobbed_Start,
    .update_fun = BankRobbed_Update
};

RECOMP_CALLBACK("mm_recomp_chaos_framework", chaos_on_init)
void register_BankRobbed() {
    bankRobbedEntity = chaos_register_effect(&bankRobbed, CHAOS_DISTURBANCE_HIGH, &CHAOS_TAG_DIALOG);
}