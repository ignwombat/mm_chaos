#include "modding.h"
#include "global.h"
#include "recomputils.h"
#include "recompconfig.h"
#include "eztr_api.h"

EZTR_DEFINE_CUSTOM_MSG_HANDLE(bankreward);
EZTR_DEFINE_CUSTOM_MSG_HANDLE(mobilebank);

typedef enum {
    MOBILEBANK_NONE,
    MOBILEBANK_INIT,
    MOBILEBANK_INPUT,
    BANKREWARD_INIT
} MobilebankState;
static MobilebankState mobilebankState = MOBILEBANK_NONE;

static bool isBankreward = false;
static bool lastFrameBankreward = false;
static u16 bankrewardTimer = 0;

void Bankreward_Start(PlayState* play) {
    Player* player = GET_PLAYER(play);
    
    if (
        isBankreward ||
        Message_GetState(&play->msgCtx) != TEXT_STATE_NONE ||
        player->tatlTextId != 0 ||
        play->transitionTrigger != TRANS_TRIGGER_OFF
    ) return;

    isBankreward = false;
    bankrewardTimer = (20 * 5); // Stays for 5 seconds
    lastFrameBankreward = false;

    player->tatlTextId = EZTR_GET_ID_H(bankreward);
}

void Bankreward_Update(PlayState* play) {
    if (!isBankreward) {
        return;
    };

    Player* player = GET_PLAYER(play);

    if (bankrewardTimer > 0)
        bankrewardTimer--;

    if (bankrewardTimer == 0) {
        isBankreward = false;
        return;
    }

    if (isBankreward) {
        bool isInBankreward = play->msgCtx.currentTextId == EZTR_GET_ID_H(bankreward);

        if (lastFrameBankreward && !isInBankreward) {
            // The message has been closed
            isBankreward = false;
            return;
        }

        else if (!lastFrameBankreward && !isInBankreward) {
            player->tatlTextId = EZTR_GET_ID_H(bankreward);
        }

        lastFrameBankreward = isInBankreward;
    }
}

RECOMP_HOOK_RETURN("Rupees_ChangeBy")
void afterRupees_ChangeBy() {
    recomp_printf("rupee accumulator: %d\n", gSaveContext.rupeeAccumulator);

    if (gSaveContext.rupeeAccumulator > 0 && ((gSaveContext.save.saveInfo.playerData.rupees + gSaveContext.rupeeAccumulator) > CUR_CAPACITY(UPG_WALLET))) {
        recomp_printf("overflow\n");

        u32 overflow = (gSaveContext.save.saveInfo.playerData.rupees + gSaveContext.rupeeAccumulator) - CUR_CAPACITY(UPG_WALLET);

        gSaveContext.rupeeAccumulator -= overflow;
        recomp_printf("remainder %d\n", overflow);

        HS_SET_BANK_RUPEES(HS_GET_BANK_RUPEES() + overflow);
        recomp_printf("bank amount %d\n", HIGH_SCORE(HS_BANK_RUPEES));
        
    if (HS_GET_BANK_RUPEES() > recomp_get_config_u32("bank_capacity")) {
        HS_SET_BANK_RUPEES(recomp_get_config_u32("bank_capacity"));
        }
    }
}

RECOMP_HOOK("Player_Update") 
void onPlayer_Update(Player* this, PlayState* play) {
    if (CHECK_BTN_ALL(play->state.input[0].cur.button, BTN_L | BTN_B) && (gSaveContext.save.saveInfo.playerData.rupees != CUR_CAPACITY(UPG_WALLET) && (HIGH_SCORE(HS_BANK_RUPEES) > 0))) {
        gSaveContext.save.saveInfo.playerData.rupees++;

        HS_SET_BANK_RUPEES(HS_GET_BANK_RUPEES() - 1);
        recomp_printf("bank amount %d\n", HIGH_SCORE(HS_BANK_RUPEES));
    }

    if (CHECK_BTN_ALL(play->state.input[0].cur.button, BTN_L | BTN_A) && (gSaveContext.save.saveInfo.playerData.rupees !=0)) {
        gSaveContext.save.saveInfo.playerData.rupees--;

        HS_SET_BANK_RUPEES(HS_GET_BANK_RUPEES() + 1);
        recomp_printf("bank amount %d\n", HIGH_SCORE(HS_BANK_RUPEES));
    }

    if (CHECK_BTN_ALL(play->state.input[0].cur.button, BTN_L | BTN_R) && (gSaveContext.save.saveInfo.playerData.rupees != HIGH_SCORE(HS_BANK_RUPEES) > CUR_CAPACITY(UPG_WALLET))) {

        HS_SET_BANK_RUPEES(CUR_CAPACITY(UPG_WALLET) - 5);
        recomp_printf("bank amount %d\n", HIGH_SCORE(HS_BANK_RUPEES));
    }

    if (CHECK_BTN_ALL(play->state.input[0].cur.button, BTN_L | BTN_B | BTN_Z)) {

        Rupees_ChangeBy(50);
        recomp_printf("debug pressed\n");
    }
}

EZTR_MSG_CALLBACK(bankreward_callback) {
    if ((HIGH_SCORE(HS_BANK_RUPEES) >= 200) && (HIGH_SCORE(HS_BANK_RUPEES) < 500))
            EZTR_MsgSContent_Sprintf(buf->data.content, "" EZTR_CC_COLOR_BLUE "Hey! " EZTR_CC_COLOR_DEFAULT "You have a reward" EZTR_CC_NEWLINE 
            "waiting for depositing " EZTR_CC_COLOR_PINK "200 rupees!" EZTR_CC_EVENT "" EZTR_CC_END
        );
        else if
            ((HIGH_SCORE(HS_BANK_RUPEES) >= 500) && (HIGH_SCORE(HS_BANK_RUPEES) < 1000))
            EZTR_MsgSContent_Sprintf(buf->data.content, "" EZTR_CC_COLOR_ORANGE "Amazing! " EZTR_CC_COLOR_DEFAULT "You have a reward" EZTR_CC_NEWLINE 
            "waiting for depositing " EZTR_CC_COLOR_PINK "500 rupees!" EZTR_CC_EVENT "" EZTR_CC_END
        );
        else if
            (HIGH_SCORE(HS_BANK_RUPEES) >= 1000)
            EZTR_MsgSContent_Sprintf(buf->data.content, "" EZTR_CC_COLOR_RED "Sp" EZTR_CC_COLOR_ORANGE "ec" EZTR_CC_COLOR_YELLOW "ta" EZTR_CC_COLOR_GREEN "cu" EZTR_CC_COLOR_BLUE "lar" EZTR_CC_COLOR_DEFAULT "! You have a reward" EZTR_CC_NEWLINE 
            "waiting for depositing " EZTR_CC_COLOR_PINK "1000 rupees!" EZTR_CC_EVENT "" EZTR_CC_END
        );
}

EZTR_ON_INIT void init_bankreward() {
    EZTR_Basic_AddCustomText(
        EZTR_HNAME(bankreward),
        EZTR_STANDARD_TEXT_BOX_I,
        1,
        EZTR_ICON_NO_ICON,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        false,
        EZTR_CC_END,
        bankreward_callback
    );
        // First visit to bank
    EZTR_Basic_ReplaceText(
        0x044C,
        EZTR_STANDARD_TEXT_BOX_I,
        0,
        EZTR_ICON_NO_ICON,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        true,
        "Welcome, friend." EZTR_CC_NEWLINE "Have too many" EZTR_CC_COLOR_PINK "Rupees" EZTR_CC_COLOR_DEFAULT "on hand?" EZTR_CC_NEWLINE "" EZTR_CC_CARRIAGE_RETURN "" EZTR_CC_BOX_BREAK2 "Then let 'First and Last Bank" EZTR_CC_NEWLINE "of Termina' take care of your needs!" EZTR_CC_NEWLINE "" EZTR_CC_BOX_BREAK2 "As a bonus for opening an account," EZTR_CC_NEWLINE "you will be rewarded the" EZTR_CC_NEWLINE "more you deposit!" EZTR_CC_EVENT "" EZTR_CC_END "",
        NULL
    );
    // Deposit reward example
    EZTR_Basic_ReplaceText(
        0x044D,
        EZTR_STANDARD_TEXT_BOX_I,
        0,
        EZTR_ICON_NO_ICON,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        true,
        "For example, if you deposit" EZTR_CC_NEWLINE "" EZTR_CC_COLOR_PINK "200 Rupees" EZTR_CC_COLOR_DEFAULT ", we will upgrade your wallet!" EZTR_CC_EVENT "" EZTR_CC_END "",
        NULL
    );
    // First-time request
    EZTR_Basic_ReplaceText(
        0x044E,
        EZTR_STANDARD_TEXT_BOX_II,
        1,
        EZTR_ICON_NO_ICON,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        true,
        "So, how about it champ?" EZTR_CC_NEWLINE " " EZTR_CC_NEWLINE "" EZTR_CC_COLOR_GREEN "" EZTR_CC_TWO_CHOICE "Sign me up!" EZTR_CC_NEWLINE "Let me think on it." EZTR_CC_END "",
        NULL
    );
    // Deposit pre-total selection text
    EZTR_Basic_ReplaceText(
        0x044F,
        EZTR_STANDARD_TEXT_BOX_II,
        0,
        EZTR_ICON_NO_ICON,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        false,
        "Please hold while we process" EZTR_CC_NEWLINE "your deposit request..." EZTR_CC_EVENT "" EZTR_CC_END "",
        NULL
    );
    // Deposit amount selection
    EZTR_Basic_ReplaceText(
        0x0450,
        EZTR_STANDARD_TEXT_BOX_II,
        1,
        EZTR_ICON_NO_ICON,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        false,
        "How much do you wish to deposit?" EZTR_CC_NEWLINE "" EZTR_CC_INPUT_BANK "" EZTR_CC_NEWLINE "Set the amount with " EZTR_CC_CONTROL_PAD " and" EZTR_CC_NEWLINE "press " EZTR_CC_BTN_A " to decide." EZTR_CC_EVENT "" EZTR_CC_END "",
        NULL
    );
    // "Don't Deposit Rupees" from first time selection
    EZTR_Basic_ReplaceText(
        0x0451,
        EZTR_STANDARD_TEXT_BOX_I,
        0,
        EZTR_ICON_NO_ICON,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        true,
        "" EZTR_CC_QUICKTEXT_ENABLE "If you change your mind," EZTR_CC_NEWLINE "you know where to find me!" EZTR_CC_END "",
        NULL
    );
    // Deposit amount confirmation
    EZTR_Basic_ReplaceText(
        0x0452,
        EZTR_STANDARD_TEXT_BOX_II,
        1,
        EZTR_ICON_NO_ICON,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        false,
        "So you are wanting to" EZTR_CC_NEWLINE "deposit " EZTR_CC_COLOR_PINK "" EZTR_CC_RUPEES_SELECTED "" EZTR_CC_COLOR_DEFAULT "?" EZTR_CC_NEWLINE "" EZTR_CC_COLOR_GREEN "" EZTR_CC_TWO_CHOICE "Yeah" EZTR_CC_NEWLINE "Actualy, wait" EZTR_CC_END "",
        NULL
    );
    // Deposit confirmed
    EZTR_Basic_ReplaceText(
        0x0454,
        EZTR_STANDARD_TEXT_BOX_II,
        0,
        EZTR_ICON_NO_ICON,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        false,
        "Look at Mr. Moneybags here!" EZTR_CC_NEWLINE "Spare some for the rest!" EZTR_CC_EVENT "" EZTR_CC_END "",
        NULL
    );
    //Selecting zero rupees on deposit screen
    EZTR_Basic_ReplaceText(
        0x0457,
        EZTR_STANDARD_TEXT_BOX_II,
        0,
        EZTR_ICON_NO_ICON,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        true,
        "" EZTR_CC_COLOR_PINK "Zero Rupees" EZTR_CC_COLOR_DEFAULT "?" EZTR_CC_NEWLINE "Listen here buddy..." EZTR_CC_END "",
        NULL
    );
    // Deposit attempt with empty wallet
    EZTR_Basic_ReplaceText(
        0x0458,
        EZTR_STANDARD_TEXT_BOX_I,
        0,
        EZTR_ICON_NO_ICON,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        false,
        "" EZTR_CC_QUICKTEXT_ENABLE "What? Did you lose it" EZTR_CC_NEWLINE "all playing lottery again?" EZTR_CC_END "",
        NULL
    );
    // Post-banking balance
    EZTR_Basic_ReplaceText(
        0x045A,
        EZTR_STANDARD_TEXT_BOX_II,
        0,
        EZTR_ICON_NO_ICON,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        false,
        "You now have a balance of" EZTR_CC_NEWLINE "" EZTR_CC_COLOR_PINK "" EZTR_CC_RUPEES_TOTAL "" EZTR_CC_COLOR_DEFAULT "!" EZTR_CC_EVENT "" EZTR_CC_END "",
        NULL
    );
    // 200 Rupee reward start
    EZTR_Basic_ReplaceText(
        0x045B,
        EZTR_STANDARD_TEXT_BOX_I,
        0,
        EZTR_ICON_NO_ICON,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        true,
        "" EZTR_CC_QUICKTEXT_ENABLE "Well now! You've already" EZTR_CC_NEWLINE "saved up " EZTR_CC_COLOR_PINK "200 Rupees" EZTR_CC_COLOR_DEFAULT "!" EZTR_CC_QUICKTEXT_DISABLE "" EZTR_CC_NEWLINE "" EZTR_CC_CARRIAGE_RETURN "" EZTR_CC_BOX_BREAK2 "I have a little something special" EZTR_CC_NEWLINE "for you. Enjoy!" EZTR_CC_EVENT2 "" EZTR_CC_END "",
        NULL
    );
    // 1000 Rupee reward start
    EZTR_Basic_ReplaceText(
        0x045C,
        EZTR_STANDARD_TEXT_BOX_I,
        0,
        EZTR_ICON_NO_ICON,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        true,
        "" EZTR_CC_QUICKTEXT_ENABLE "" EZTR_CC_COLOR_ORANGE "500 Rupees" EZTR_CC_COLOR_DEFAULT "!!" EZTR_CC_NEWLINE "You've been busy I see!" EZTR_CC_QUICKTEXT_DISABLE "" EZTR_CC_NEWLINE "" EZTR_CC_CARRIAGE_RETURN "" EZTR_CC_BOX_BREAK2 "Your continued support deseveres a" EZTR_CC_NEWLINE "splendid reward!" EZTR_CC_EVENT2 "" EZTR_CC_END "",
        NULL
    );
    // 5000 Rupee reward start
    EZTR_Basic_ReplaceText(
        0x045D,
        EZTR_STANDARD_TEXT_BOX_I,
        0,
        EZTR_ICON_NO_ICON,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        true,
        "" EZTR_CC_QUICKTEXT_ENABLE "" EZTR_CC_COLOR_RED "1" EZTR_CC_COLOR_BLUE "0" EZTR_CC_COLOR_YELLOW "0" EZTR_CC_COLOR_GREEN "0" EZTR_CC_COLOR_PINK "Rupees!?!" EZTR_CC_COLOR_DEFAULT "" EZTR_CC_QUICKTEXT_DISABLE "" EZTR_CC_NEWLINE "" EZTR_CC_CARRIAGE_RETURN "" EZTR_CC_BOX_BREAK2 "Talk about being a loyal" EZTR_CC_NEWLINE "customer! Please accept this gift, it's" EZTR_CC_NEWLINE "all I have left to offer." EZTR_CC_EVENT2 "" EZTR_CC_END "",
        NULL
    );
    // Bank capped message, converted to goodbye
    EZTR_Basic_ReplaceText(
        0x045E,
        EZTR_STANDARD_TEXT_BOX_II,
        0,
        EZTR_ICON_NO_ICON,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        false,
        "Thank you for using" EZTR_CC_NEWLINE "First and Last Bank of Termina." EZTR_CC_END "",
        NULL
    );
    // Bank refuses any further deposits
    EZTR_Basic_ReplaceText(
        0x045F,
        EZTR_STANDARD_TEXT_BOX_I,
        0,
        EZTR_ICON_NO_ICON,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        false,
        "" EZTR_CC_QUICKTEXT_ENABLE "Sorry pal, I'm afraid I cannot" EZTR_CC_NEWLINE "take any more deposits!" EZTR_CC_QUICKTEXT_DISABLE "" EZTR_CC_NEWLINE "" EZTR_CC_CARRIAGE_RETURN "" EZTR_CC_BOX_BREAK2 "Even my coffers are full! " EZTR_CC_NEWLINE "You remind me of myself" EZTR_CC_NEWLINE "at our age" EZTR_CC_NEWLINE "I'm proud of you!" EZTR_CC_END "",
        NULL
    );
    // Withdrawal confirmation end
    EZTR_Basic_ReplaceText(
        0x0460,
        EZTR_STANDARD_TEXT_BOX_II,
        0,
        EZTR_ICON_NO_ICON,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        false,
        "We're always open if" EZTR_CC_NEWLINE "you need us to hold more!" EZTR_CC_END "",
        NULL
    );
    //Intro dialog box
    EZTR_Basic_ReplaceText(
        0x0466,
        EZTR_STANDARD_TEXT_BOX_II,
        0,
        EZTR_ICON_NO_ICON,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        false,
        "How may I help you?" EZTR_CC_EVENT "" EZTR_CC_END "",
        NULL
    );
    //Selection menu
    EZTR_Basic_ReplaceText( 
        0x0468,
        EZTR_STANDARD_TEXT_BOX_II,
        1,
        EZTR_ICON_NO_ICON,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        false,
        "How may I be of assistance?" EZTR_CC_NEWLINE "" EZTR_CC_COLOR_GREEN "" EZTR_CC_THREE_CHOICE "Hold my money!" EZTR_CC_NEWLINE "Gimme my money" EZTR_CC_NEWLINE "*hang up*" EZTR_CC_END "",
        NULL
    );
    //Deposit confirmation
    EZTR_Basic_ReplaceText( 
        0x0469,
        EZTR_STANDARD_TEXT_BOX_II,
        0,
        EZTR_ICON_NO_ICON,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        false,
        "With whom do I have the pleasure" EZTR_CC_NEWLINE "of speaking with today?" EZTR_CC_EVENT "" EZTR_CC_END,
        NULL
    );
    // Balance confirmation
    EZTR_Basic_ReplaceText(
        0x046A,
        EZTR_STANDARD_TEXT_BOX_II,
        0,
        EZTR_ICON_NO_ICON,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        false,
        "Verifying identification..." EZTR_CC_EVENT "" EZTR_CC_END "",
        NULL
    );
    // Pre-withdrawal amount text
    EZTR_Basic_ReplaceText(
        0x046B,
        EZTR_STANDARD_TEXT_BOX_II,
        0,
        EZTR_ICON_NO_ICON,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        false,
        "Well now, if it isn't " EZTR_CC_COLOR_RED "" EZTR_CC_NAME "" EZTR_CC_COLOR_DEFAULT "!" EZTR_CC_NEWLINE "Our records show your balance" EZTR_CC_NEWLINE "to be " EZTR_CC_COLOR_PINK "" EZTR_CC_RUPEES_TOTAL "" EZTR_CC_COLOR_DEFAULT " today." EZTR_CC_EVENT "" EZTR_CC_END,
        NULL
    );
    // withdrawal amount selection
    EZTR_Basic_ReplaceText(
        0x046E,
        EZTR_STANDARD_TEXT_BOX_II,
        1,
        EZTR_ICON_NO_ICON,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        false,
        "Going on a shopping spree again?" EZTR_CC_NEWLINE "" EZTR_CC_INPUT_BANK "" EZTR_CC_NEWLINE "Set the amount with " EZTR_CC_CONTROL_PAD "" EZTR_CC_NEWLINE "and press " EZTR_CC_BTN_A " to decide." EZTR_CC_EVENT "" EZTR_CC_END "",
        NULL
    );
    // Selecting zero on withdrawal screen
    EZTR_Basic_ReplaceText(
        0x046F,
        EZTR_STANDARD_TEXT_BOX_II,
        0,
        EZTR_ICON_NO_ICON,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        false,
        "No amount entered." EZTR_CC_NEWLINE "Terminating transaction..." EZTR_CC_END "",
        NULL
    );
    // Cancel banking
    EZTR_Basic_ReplaceText(
        0x0470,
        EZTR_STANDARD_TEXT_BOX_II,
        0,
        EZTR_ICON_NO_ICON,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        false,
        "Goodbye *click*" EZTR_CC_EVENT2 "" EZTR_CC_END "",
        NULL
    );
    // withdrawal amount confirmation
    EZTR_Basic_ReplaceText(
        0x0471,
        EZTR_STANDARD_TEXT_BOX_II,
        1,
        EZTR_ICON_NO_ICON,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        false,
        "Do you wish to" EZTR_CC_NEWLINE "withdraw " EZTR_CC_COLOR_PINK "" EZTR_CC_RUPEES_SELECTED "" EZTR_CC_COLOR_DEFAULT " today?" EZTR_CC_NEWLINE "" EZTR_CC_COLOR_GREEN "" EZTR_CC_TWO_CHOICE "Yes" EZTR_CC_NEWLINE "No" EZTR_CC_END "",
        NULL
    );
    // Post-withdrawal flavor text
    EZTR_Basic_ReplaceText(
        0x0473,
        EZTR_STANDARD_TEXT_BOX_II,
        0,
        EZTR_ICON_NO_ICON,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        false,
        "Must be nice to have" EZTR_CC_NEWLINE "so much money at" EZTR_CC_NEWLINE "your disposal..." EZTR_CC_EVENT "" EZTR_CC_END "",
        NULL
    );
    // Deposit flavor text
    EZTR_Basic_ReplaceText(
        0x0479,
        EZTR_STANDARD_TEXT_BOX_II,
        0,
        EZTR_ICON_NO_ICON,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        false,
        "Well now, if it isn't " EZTR_CC_COLOR_RED "" EZTR_CC_NAME "" EZTR_CC_COLOR_DEFAULT "!" EZTR_CC_NEWLINE "Our records show your balance" EZTR_CC_NEWLINE "to be " EZTR_CC_COLOR_PINK "" EZTR_CC_RUPEES_TOTAL "" EZTR_CC_COLOR_DEFAULT " today." EZTR_CC_EVENT "" EZTR_CC_END,
        NULL
    );
    // Post-200 reward message
    EZTR_Basic_ReplaceText(
        0x047A,
        EZTR_STANDARD_TEXT_BOX_II,
        0,
        EZTR_ICON_NO_ICON,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        true,
        "You've been quite the litte" EZTR_CC_NEWLINE "investor lately! I hope you" EZTR_CC_NEWLINE "enjoy this gift!" EZTR_CC_END "",
        NULL
    );
    // Post-1000 reward message
    EZTR_Basic_ReplaceText(
        0x047B,
        EZTR_STANDARD_TEXT_BOX_II,
        0,
        EZTR_ICON_NO_ICON,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        true,
        "Put that to good use!" EZTR_CC_END "",
        NULL
    );
    // Cancel withdraw confirmation
    EZTR_Basic_ReplaceText(
        0x047C,
        EZTR_STANDARD_TEXT_BOX_II,
        0,
        EZTR_ICON_NO_ICON,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        true,
        "Terminating request..." EZTR_CC_NEWLINE "" EZTR_CC_CARRIAGE_RETURN "" EZTR_CC_BOX_BREAK2 "Returning to main menu..." EZTR_CC_EVENT "" EZTR_CC_END "",
        NULL
    );
    // Deposit & withdrawal cancel confirmation, returns to options
    EZTR_Basic_ReplaceText(
        0x047D,
        EZTR_STANDARD_TEXT_BOX_II,
        0,
        EZTR_ICON_NO_ICON,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        false,
        "Terminating request..." EZTR_CC_NEWLINE "" EZTR_CC_CARRIAGE_RETURN "" EZTR_CC_BOX_BREAK2 "Returning to main menu..." EZTR_CC_EVENT "" EZTR_CC_END "",
        NULL
    );
    // withdrawal selected, balance display
    EZTR_Basic_ReplaceText(
        0x047E,
        EZTR_STANDARD_TEXT_BOX_II,
        0,
        EZTR_ICON_NO_ICON,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        false,
        "We show a current balance of " EZTR_CC_NEWLINE "" EZTR_CC_COLOR_PINK "" EZTR_CC_RUPEES_TOTAL "" EZTR_CC_COLOR_DEFAULT "." EZTR_CC_EVENT "" EZTR_CC_END "",
        NULL
    );
}