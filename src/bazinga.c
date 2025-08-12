#include "chaos.h"
#include "eztr_api.h"

EZTR_DEFINE_CUSTOM_MSG_HANDLE(bazinga_msg);

EZTR_ON_INIT void init_bazinga() {
    EZTR_Basic_AddCustomText(
        EZTR_HNAME(bazinga_msg),
        EZTR_STANDARD_TEXT_BOX_I,
        0,
        EZTR_ICON_DEKU_NUT,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        false,

        EZTR_CC_QUICKTEXT_DISABLE EZTR_CC_SFX_ARGW EZTR_CC_COLOR_GREEN "BAZINGA!" EZTR_CC_COLOR_DEFAULT EZTR_CC_DELAY_ARGW
        EZTR_CC_NEWLINE "Press " EZTR_CC_BTN_A EZTR_CC_DELAY_ARGW " or " EZTR_CC_DELAY_ARGW EZTR_CC_BTN_B
        EZTR_CC_NEWLINE "if you wish to"
        EZTR_CC_NEWLINE "dismiss this message."
        EZTR_CC_END,

        NULL,
        NA_SE_IT_BOMB_EXPLOSION,
        18,
        15,
        15
    );
}

void Bazinga_Start(GraphicsContext* gfxCtx, GameState* gameState) {
    PlayState* play = (PlayState*)gameState;
    Message_StartTextbox(play, EZTR_GET_ID_H(bazinga_msg), NULL);
}

void Bazinga_Update(GraphicsContext* gfxCtx, GameState* gameState) {
    
}

void Bazinga_End(GraphicsContext* gfxCtx, GameState* gameState) {
    
}

ChaosEffect bazinga = {
    .name = "Bazinga",
    .duration = 20 * 10, // 10 seconds
    .on_start_fun = Bazinga_Start,
    .update_fun = Bazinga_Update,
    .on_end_fun = Bazinga_End
};

RECOMP_CALLBACK("mm_recomp_chaos_framework", chaos_on_init)
void register_bazinga() {
    chaos_register_effect(&bazinga, CHAOS_DISTURBANCE_LOW, NULL);
}