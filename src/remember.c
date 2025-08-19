#include "chaos.h"
#include "chaos_exclusivity.h"
#include "eztr_api.h"
#include "recomputils.h"

#define MAX_DIGITS 5

static ChaosEffectEntity *rememberEntity;
EZTR_DEFINE_CUSTOM_MSG_HANDLE(remember_msg);
EZTR_DEFINE_CUSTOM_MSG_HANDLE(remember_prompt_msg);

static u8 correctDigits[5];
static u8 playerDigits[5];
EZTR_MSG_CALLBACK(Remember_callback) {
    u16 d = 0;
    for (u8 i = 0; i < MAX_DIGITS; i++)
        // First digit gets multiplied by 10000, second 1000 and so on
        // Otherwise it would show them in reverse order
        d += correctDigits[i] * ((u16)Math_PowF(10, 4 - i));

    EZTR_MsgSContent_Sprintf(buf->data.content,
        EZTR_CC_QUICKTEXT_DISABLE "The " EZTR_CC_COLOR_RED "code" EZTR_CC_COLOR_DEFAULT " is:" EZTR_CC_NEWLINE
        EZTR_CC_COLOR_RED "%u" EZTR_CC_END,
        d
    );
}

EZTR_ON_INIT void init_remember() {
    EZTR_Basic_AddCustomText(
        EZTR_HNAME(remember_msg),
        EZTR_BOMBERS_NOTEBOOK,
        2,
        EZTR_ICON_BOMBERS_NOTEBOOK,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        false,
        EZTR_CC_END,
        Remember_callback
    );

    EZTR_Basic_AddCustomText(
        EZTR_HNAME(remember_prompt_msg),
        EZTR_STANDARD_TEXT_BOX_I,
        2,
        EZTR_ICON_NO_ICON,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        false,
        EZTR_CC_COLOR_RED "Quick" EZTR_CC_COLOR_DEFAULT "! What's the " EZTR_CC_COLOR_RED "code" EZTR_CC_COLOR_DEFAULT "?" EZTR_CC_NEWLINE
        EZTR_CC_INPUT_BOMBER_CODE EZTR_CC_NEWLINE
        "Pick numbers with " EZTR_CC_CONTROL_PAD "." EZTR_CC_NEWLINE
        EZTR_CC_DELAY_ARGW "Press " EZTR_CC_BTN_A " to confirm." EZTR_CC_END,
        NULL,
        8
    );
}


// Who is quick maths? Why is quick maths?
static bool isRemember = false;
static bool lastFrameRemember = false;
static u16 RememberTimer = 0;

// true if the last activation was "Quick! What's the code?"
static bool lastWasInput = true;
static u16 currentTextId;

void Remember_Start(PlayState* play) {
    Player* player = GET_PLAYER(play);

    if (
        isRemember ||
        Message_GetState(&play->msgCtx) != TEXT_STATE_NONE ||
        player->tatlTextId != 0 ||
        play->transitionTrigger != TRANS_TRIGGER_OFF ||
        play->gameOverCtx.state != GAMEOVER_INACTIVE ||
        player->stateFlags1 & ~PLAYER_STATE1_20
    ) {
        chaos_stop_effect(rememberEntity);
        return;
    };

    isRemember = true;
    
    if (lastWasInput) {
        u8 digits[MAX_DIGITS] = { 1, 2, 3, 4, 5 };

        for (u8 i = 0; i < MAX_DIGITS; i++) {
            u8 j = Rand_Next() % (i + 1);

            u8 temp = digits[i];
            digits[i] = digits[j];
            digits[j] = temp;
        }

        for (u8 i = 0; i < MAX_DIGITS; i++) {
            correctDigits[i] = digits[i];
        }
    }

    else {
        for (u8 i = 0; i < MAX_DIGITS; i++) {
            playerDigits[i] = 1;
        }
    }

    RememberTimer = (20 * 15); // Stays for 15 seconds

    currentTextId = lastWasInput
        ? EZTR_GET_ID_H(remember_msg)
        : EZTR_GET_ID_H(remember_prompt_msg);

    Message_StartTextbox(play, currentTextId, NULL);
    GET_PLAYER(play)->stateFlags1 |= PLAYER_STATE1_20;

    if (lastWasInput) Audio_PlaySfx(NA_SE_SY_SCHEDULE_WRITE);
    lastWasInput = !lastWasInput;
}

void Remember_Update(PlayState* play) {
    if (!isRemember) {
        chaos_stop_effect(rememberEntity);
        return;
    };

    MessageContext* msgCtx = &play->msgCtx;
    Player* player = GET_PLAYER(play);
    
    if (msgCtx->currentTextId != currentTextId) {
        // You've been saved... (if lastWasInput that is)
        isRemember = false;
        chaos_stop_effect(rememberEntity);
        player->stateFlags1 &= ~PLAYER_STATE1_20;
        return;
    }

    if (!lastWasInput) return;

    if (
        msgCtx->currentTextId == currentTextId &&
        Message_GetState(msgCtx) == TEXT_STATE_INPUT_BOMBER_CODE &&
        Message_ShouldAdvance(play)
    ) {
        for (u8 i = 0; i < MAX_DIGITS; i++)
            playerDigits[i] = play->msgCtx.unk12054[i];
        
        isRemember = false;
        RememberTimer = 0;
        Message_CloseTextbox(play);
    }

    if (lastWasInput && RememberTimer > 0) {
        if (RememberTimer % 20 == 0) {
            Audio_PlaySfx(
                RememberTimer < 20 * 4
                    ? NA_SE_SY_WARNING_COUNT_E
                    : NA_SE_SY_WARNING_COUNT_N
            );
        }

        RememberTimer--;
        return;
    }

    player->stateFlags1 &= ~PLAYER_STATE1_20;

    bool isCorrect = true;
    for (u8 i = 0; i < MAX_DIGITS; i++)
        if (playerDigits[i] != correctDigits[i]) {
            recomp_printf("Comparing %d to %d", playerDigits[i], correctDigits[i]);
            isCorrect = false;
            break;
        }

    if (!isCorrect) {
        if (msgCtx->currentTextId == currentTextId)
            Message_CloseTextbox(play);

        gSaveContext.save.saveInfo.playerData.health = 0;
    }

    else
        Audio_PlaySfx(NA_SE_SY_QUIZ_CORRECT);

    isRemember = false;
    chaos_stop_effect(rememberEntity);
}

ChaosEffect Remember = {
    .name = "Wommy Remember",
    .duration = CHAOS_DURATION_MAX, // Controlled manually
    .on_start_fun = Remember_Start,
    .update_fun = Remember_Update
};

RECOMP_CALLBACK("mm_recomp_chaos_framework", chaos_on_init)
void register_Remember() {
    rememberEntity = chaos_register_effect(&Remember, CHAOS_DISTURBANCE_VERY_LOW, &CHAOS_TAG_DIALOG);
}