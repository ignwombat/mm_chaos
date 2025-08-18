#include "chaos.h"
#include "chaos_exclusivity.h"
#include "eztr_api.h"

static ChaosEffectEntity *quickMathsEntity;
EZTR_DEFINE_CUSTOM_MSG_HANDLE(quick_maths_msg);

typedef struct {
    u8 divident;
    u8 divisor;
} DivisionCombo;

DivisionCombo divisionCombos[] = {
    { 16, 4 },
    { 21, 3 },
    { 25, 5 },
    { 28, 7 },
    { 33, 3 },
    { 36, 6 },
    { 42, 7 },
    { 49, 7 },
    { 64, 8 },
    { 81, 9 },
    { 110, 10 },
    { 144, 12 }
};

static u16 correctAnswer;
static u16 playerAnswer;
EZTR_MSG_CALLBACK(QuickMaths_callback) {
    char* operand;
    u8 a;
    u8 b;

    switch(Rand_Next() % 4) {
        case 0:
            operand = "+";
            a = 21 + (Rand_Next() % 121);
            b = 21 + (Rand_Next() % 121);
            correctAnswer = a + b;
            break;
        case 1:
            operand = "-";
            a = 51 + (Rand_Next() % 41);
            b = 14 + (Rand_Next() % 26);
            correctAnswer = a - b;
            break;
        case 2:
            operand = "x";
            a = 2 + (Rand_Next() % 8);
            b = 4 + (Rand_Next() % 8);
            correctAnswer = a * b;
            break;
        case 3:
            operand = "/";
            DivisionCombo* combo = &divisionCombos[Rand_Next() % (ARRAY_COUNT(divisionCombos))];
            a = combo->divident;
            b = combo->divisor;
            correctAnswer = a / b;
            break;
    }

    EZTR_MsgSContent_Sprintf(buf->data.content,
        EZTR_CC_COLOR_RED "Quick" EZTR_CC_COLOR_DEFAULT "! What's %d%s%d in rupees?" EZTR_CC_NEWLINE
        EZTR_CC_INPUT_BANK EZTR_CC_NEWLINE
        "Pick numbers with " EZTR_CC_CONTROL_PAD "." EZTR_CC_NEWLINE
        EZTR_CC_DELAY_ARGW "Press " EZTR_CC_BTN_A " to confirm." EZTR_CC_END,
        a,
        operand,
        b,
        8
    );
}

EZTR_ON_INIT void init_quick_maths() {
    EZTR_Basic_AddCustomText(
        EZTR_HNAME(quick_maths_msg),
        EZTR_STANDARD_TEXT_BOX_I,
        2,
        EZTR_ICON_NO_ICON,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        false,
        EZTR_CC_END,
        QuickMaths_callback
    );
}


// Who is quick maths? Why is quick maths?
static bool isQuickMaths = false;
static bool lastFrameQuickMaths = false;
static u16 quickMathsTimer = 0;
void QuickMaths_Start(PlayState* play) {
    Player* player = GET_PLAYER(play);

    if (
        isQuickMaths ||
        Message_GetState(&play->msgCtx) != TEXT_STATE_NONE ||
        player->tatlTextId != 0 ||
        play->transitionTrigger != TRANS_TRIGGER_OFF ||
        play->gameOverCtx.state != GAMEOVER_INACTIVE
    ) return;

    isQuickMaths = true;
    playerAnswer = 0;
    quickMathsTimer = (20 * 14); // Stays for 12 seconds

    Message_StartTextbox(play, EZTR_GET_ID_H(quick_maths_msg), NULL);
    GET_PLAYER(play)->stateFlags1 |= PLAYER_STATE1_20;
}

void QuickMaths_Update(PlayState* play) {
    if (!isQuickMaths) {
        chaos_stop_effect(quickMathsEntity);
        return;
    };

    MessageContext* msgCtx = &play->msgCtx;
    
    if (msgCtx->currentTextId != EZTR_GET_ID_H(quick_maths_msg)) {
        isQuickMaths = false;
        chaos_stop_effect(quickMathsEntity);
    }

    if (
        msgCtx->currentTextId == EZTR_GET_ID_H(quick_maths_msg) &&
        Message_GetState(msgCtx) == TEXT_STATE_INPUT_RUPEES &&
        Message_ShouldAdvance(play)
    ) {
        playerAnswer = msgCtx->rupeesSelected;
        isQuickMaths = false;
        quickMathsTimer = 0;
        Message_CloseTextbox(play);
    }

    if (quickMathsTimer > 0) {
        if (quickMathsTimer % 20 == 0) {
            Audio_PlaySfx(
                quickMathsTimer < 20 * 4
                    ? NA_SE_SY_WARNING_COUNT_E
                    : NA_SE_SY_WARNING_COUNT_N
            );
        }

        quickMathsTimer--;
        return;
    }

    Player* player = GET_PLAYER(play);
    player->stateFlags1 &= ~PLAYER_STATE1_20;

    if (playerAnswer != correctAnswer) {
        if (msgCtx->currentTextId == EZTR_GET_ID_H(quick_maths_msg))
            Message_CloseTextbox(play);

        isQuickMaths = false;
        gSaveContext.save.saveInfo.playerData.health = 0;
    }

    else
        Audio_PlaySfx(NA_SE_SY_QUIZ_CORRECT);

    chaos_stop_effect(quickMathsEntity);
}

ChaosEffect QuickMaths = {
    .name = "Wommy Quick Maths",
    .duration = CHAOS_DURATION_MAX, // Controlled manually
    .on_start_fun = QuickMaths_Start,
    .update_fun = QuickMaths_Update
};

RECOMP_CALLBACK("mm_recomp_chaos_framework", chaos_on_init)
void register_QuickMaths() {
    quickMathsEntity = chaos_register_effect(&QuickMaths, CHAOS_DISTURBANCE_VERY_HIGH, &CHAOS_TAG_DIALOG);
}