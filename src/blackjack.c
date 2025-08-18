#include "chaos.h"
#include "chaos_exclusivity.h"
#include "eztr_api.h"
#include "message_data_fmt_nes.h"
#include "z64msgevent.h"

EZTR_DEFINE_CUSTOM_MSG_HANDLE(blackjack_init); // It's time to play blackjack!
EZTR_DEFINE_CUSTOM_MSG_HANDLE(blackjack_input); // Input bet
EZTR_DEFINE_CUSTOM_MSG_HANDLE(blackjack_input_none); // 0 rupees were bet
EZTR_DEFINE_CUSTOM_MSG_HANDLE(blackjack_input_over); // more than balance was bet
EZTR_DEFINE_CUSTOM_MSG_HANDLE(blackjack_choice); // Hit / Stand
EZTR_DEFINE_CUSTOM_MSG_HANDLE(blackjack_hit); // Hit! Your hand is...
EZTR_DEFINE_CUSTOM_MSG_HANDLE(blackjack_result); // Show result

static ChaosEffectEntity *blackjackEntity;

typedef enum {
    BLACKJACK_NONE, // Not currently playing
    BLACKJACK_INIT, // The game has started
    BLACKJACK_INPUT, // Insert rupees to bet
    BLACKJACK_CHOICE, // The player must make a choice
    BLACKJACK_HIT, // The player has chosen to hit
    BLACKJACK_STAND, // The player has chosen to stand
    BLACKJACK_BUST, // The player has busted
    BLACKJACK_WIN, // The player has won
    BLACKJACK_LOSE, // The dealer has won (no bust)
    BLACKJACK_TIE // The game is a tie
} BlackjackState;
static BlackjackState blackjackState = BLACKJACK_NONE;

#define MAX_CARDS 10

// Array of cards in hand
static u8 blackjackPlayerCards[MAX_CARDS];
static u8 blackjackDealerCards[MAX_CARDS];

// How many cards are in each hand
static u8 blackjackPlayerCardCount;
static u8 blackjackDealerCardCount;

// Actual hand value
static u8 blackjackHand;
static u8 blackjackDealerHand;

static u8 blackjackRupeesWaged;

static const u8 cardValues[] = {
    2, 3, 4, 5, 6, 7, 8, 9, 10, 10, 10, 10, 11 // 11 = Ace
};

static u8 Blackjack_DrawCard() {
    return cardValues[Rand_Next() % (ARRAY_COUNT(cardValues))];
}

static u8 Blackjack_CalcHandValue(u8 *cards, u8 count) {
    u8 total = 0;
    u8 aces = 0;

    for (u8 i = 0; i < count; i++) {
        total += cards[i];
        if (cards[i] == 11) aces++;
    }

    // Downgrade aces from 11 to 1 as needed
    while (total > 21 && aces > 0) {
        total -= 10; // count ace as 1 instead of 11
        aces--;
    }

    return total;
}

static void Blackjack_ResetHands() {
    for (u8 i = 0; i < MAX_CARDS; i++) {
        blackjackPlayerCards[i] = 0;
        blackjackDealerCards[i] = 0;
    }

    blackjackHand = 0;
    blackjackDealerHand = 0;
    
    // Initial 2 cards each
    blackjackPlayerCards[blackjackPlayerCardCount++] = Blackjack_DrawCard();
    blackjackPlayerCards[blackjackPlayerCardCount++] = Blackjack_DrawCard();
    blackjackDealerCards[blackjackDealerCardCount++] = Blackjack_DrawCard();
    blackjackDealerCards[blackjackDealerCardCount++] = Blackjack_DrawCard();

    blackjackHand = Blackjack_CalcHandValue(blackjackPlayerCards, blackjackPlayerCardCount);
    blackjackDealerHand = Blackjack_CalcHandValue(blackjackDealerCards, blackjackDealerCardCount);
}

EZTR_MSG_CALLBACK(blackjack_result_callback) {
    char *content;
    Player* player = GET_PLAYER(play);
    
    switch(blackjackState) {
        case BLACKJACK_WIN:
            if (blackjackDealerHand > 21)
                EZTR_MsgSContent_Sprintf(
                    buf->data.content,
                    EZTR_CC_QUICKTEXT_ENABLE EZTR_CC_COLOR_GREEN "You won" EZTR_CC_COLOR_DEFAULT "! The dealer got " EZTR_CC_COLOR_ORANGE "%d"
                    EZTR_CC_NEWLINE EZTR_CC_COLOR_DEFAULT "and busted! You've gained " EZTR_CC_COLOR_GREEN "%d" EZTR_CC_COLOR_DEFAULT " rupees!" EZTR_CC_END,
                    blackjackDealerHand,
                    blackjackRupeesWaged
                );
            else
                EZTR_MsgSContent_Sprintf(
                    buf->data.content,
                    EZTR_CC_QUICKTEXT_ENABLE EZTR_CC_COLOR_GREEN "You won" EZTR_CC_COLOR_DEFAULT "! The dealer only got " EZTR_CC_NEWLINE
                    EZTR_CC_COLOR_ORANGE "%d" EZTR_CC_COLOR_DEFAULT "! You've gained " EZTR_CC_NEWLINE
                    EZTR_CC_COLOR_GREEN "%d" EZTR_CC_COLOR_DEFAULT " rupees!" EZTR_CC_END,
                    blackjackRupeesWaged
                );
            break;
        case BLACKJACK_LOSE:
            EZTR_MsgSContent_Sprintf(
                buf->data.content,
                EZTR_CC_QUICKTEXT_DISABLE "Oh no! The dealer got " EZTR_CC_COLOR_RED "%d" EZTR_CC_COLOR_DEFAULT "!" EZTR_CC_NEWLINE
                "Your " EZTR_CC_COLOR_RED "%d" EZTR_CC_COLOR_DEFAULT " rupees were lost!" EZTR_CC_END,
                blackjackDealerHand,
                blackjackRupeesWaged
            );
            break;
        case BLACKJACK_BUST:
            EZTR_MsgSContent_Sprintf(
                buf->data.content,
                EZTR_CC_QUICKTEXT_DISABLE "Oh no! Your hand is " EZTR_CC_COLOR_RED "%d" EZTR_CC_COLOR_DEFAULT "!" EZTR_CC_NEWLINE
                "You " EZTR_CC_COLOR_RED "busted" EZTR_CC_COLOR_DEFAULT "!" EZTR_CC_NEWLINE
                "Your " EZTR_CC_COLOR_RED "%d" EZTR_CC_COLOR_DEFAULT " rupees were lost!" EZTR_CC_END,
                blackjackHand,
                blackjackRupeesWaged
            );
            break;
        case BLACKJACK_TIE:
            EZTR_MsgSContent_Sprintf(
                buf->data.content,
                EZTR_CC_QUICKTEXT_DISABLE "It's a " EZTR_CC_COLOR_ORANGE "tie" EZTR_CC_COLOR_DEFAULT "!"
                "Your " EZTR_CC_COLOR_GREEN "%d" EZTR_CC_NEWLINE
                EZTR_CC_COLOR_DEFAULT "rupees were returned to you" EZTR_CC_END,
                blackjackRupeesWaged
            );
            break;
        default:
            break;
    }
}

EZTR_MSG_CALLBACK(blackjack_hit_callback) {
    EZTR_MsgSContent_Sprintf(
        buf->data.content,
        EZTR_CC_QUICKTEXT_ENABLE EZTR_CC_COLOR_GREEN "Hit"
        EZTR_CC_COLOR_DEFAULT "! Your hand is now " EZTR_CC_COLOR_GREEN "%d" EZTR_CC_EVENT EZTR_CC_END,
        blackjackHand
    );
}

EZTR_MSG_CALLBACK(blackjack_choice_callback) {
    Player* player = GET_PLAYER(play);
    blackjackState = BLACKJACK_CHOICE;

    EZTR_MsgSContent_Sprintf(
        buf->data.content,
        EZTR_CC_QUICKTEXT_ENABLE "Your hand: %d" EZTR_CC_NEWLINE
        "Dealer's visible: %d" EZTR_CC_NEWLINE
        EZTR_CC_TWO_CHOICE EZTR_CC_COLOR_GREEN "Hit" EZTR_CC_NEWLINE EZTR_CC_COLOR_RED "Stand" EZTR_CC_END,
        blackjackHand,
        blackjackDealerHand
    );
}

EZTR_ON_INIT void init_blackjack() {
    EZTR_Basic_AddCustomText(
        EZTR_HNAME(blackjack_result),
        EZTR_STANDARD_TEXT_BOX_I,
        2,
        EZTR_ICON_NO_ICON,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        false,
        EZTR_CC_END,
        blackjack_result_callback
    );

    EZTR_Basic_AddCustomText(
        EZTR_HNAME(blackjack_hit),
        EZTR_STANDARD_TEXT_BOX_I,
        2,
        EZTR_ICON_NO_ICON,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        false,
        EZTR_CC_EVENT EZTR_CC_END,
        blackjack_hit_callback
    );

    EZTR_Basic_AddCustomText(
        EZTR_HNAME(blackjack_choice),
        EZTR_STANDARD_TEXT_BOX_I,
        2,
        EZTR_ICON_NO_ICON,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        false,
        EZTR_CC_END,
        blackjack_choice_callback
    );

    EZTR_Basic_AddCustomText(
        EZTR_HNAME(blackjack_input_none),
        EZTR_STANDARD_TEXT_BOX_I,
        2,
        EZTR_ICON_NO_ICON,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        false,
        EZTR_CC_QUICKTEXT_ENABLE EZTR_CC_COLOR_RED "You must bet at least 10 rupees!" EZTR_CC_EVENT EZTR_CC_END,
        NULL
    );

    EZTR_Basic_AddCustomText(
        EZTR_HNAME(blackjack_input_over),
        EZTR_STANDARD_TEXT_BOX_I,
        2,
        EZTR_ICON_NO_ICON,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        false,
        EZTR_CC_QUICKTEXT_ENABLE EZTR_CC_COLOR_RED "You cannot bet more than you have!" EZTR_CC_EVENT EZTR_CC_END,
        NULL
    );

    EZTR_Basic_AddCustomText(
        EZTR_HNAME(blackjack_input),
        EZTR_STANDARD_TEXT_BOX_I,
        2,
        EZTR_ICON_NO_ICON,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        false,
        EZTR_CC_QUICKTEXT_ENABLE "Insert rupees to bet" EZTR_CC_NEWLINE
        EZTR_CC_INPUT_DOGGY_RACETRACK_BET EZTR_CC_NEWLINE
        "Pick numbers with " EZTR_CC_CONTROL_PAD "." EZTR_CC_NEWLINE
        "Press " EZTR_CC_BTN_A " to confirm." EZTR_CC_END,
        NULL
    );

    EZTR_Basic_AddCustomText(
        EZTR_HNAME(blackjack_init),
        EZTR_STANDARD_TEXT_BOX_I,
        2,
        EZTR_ICON_GIANTS_WALLET,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        EZTR_NO_VALUE,
        false,

        EZTR_CC_QUICKTEXT_DISABLE "It's time to play " EZTR_CC_COLOR_SILVER "black" EZTR_CC_COLOR_RED "jack" EZTR_CC_COLOR_DEFAULT "!" EZTR_CC_DELAY_ARGW
        EZTR_CC_NEWLINE "You must hit or stand to win" 
        EZTR_CC_NEWLINE "against the dealer." EZTR_CC_DELAY_ARGW " You must"
        EZTR_CC_NEWLINE "bet between " EZTR_CC_DELAY_ARGW EZTR_CC_COLOR_GREEN "10" EZTR_CC_COLOR_DEFAULT "-" EZTR_CC_COLOR_GREEN "90" EZTR_CC_COLOR_DEFAULT " rupees."
        EZTR_CC_EVENT EZTR_CC_END,

        NULL,
        18,
        15
    );
}

void Blackjack_Start(PlayState* play) {
    Player* player = GET_PLAYER(play);

    // Various checks to make sure blackjack can be played
    if (
        blackjackState != BLACKJACK_NONE ||
        Message_GetState(&play->msgCtx) != TEXT_STATE_NONE ||
        play->transitionTrigger != TRANS_TRIGGER_OFF ||
        play->gameOverCtx.state != GAMEOVER_INACTIVE ||
        player->stateFlags1 & ~PLAYER_STATE1_20 ||
        gSaveContext.save.saveInfo.playerData.rupees < 10
    ) return;

    // Initial 2 cards each
    Blackjack_ResetHands();
    for (u8 i = 0; blackjackDealerHand == 21 && i < 10; i++)
        Blackjack_ResetHands();

    blackjackState = BLACKJACK_INIT;

    Message_StartTextbox(play, EZTR_GET_ID_H(blackjack_init), NULL);
    Player_PlaySfx(player, NA_SE_SY_GET_RUPY);

    GET_PLAYER(play)->stateFlags1 |= PLAYER_STATE1_20;
}

// TODO check if the game got interrupted somehow, re-enable the effect and continue on the next chaos roll, callback for init msg instead of new msg
void Blackjack_Update(PlayState* play) {
    if (blackjackState == BLACKJACK_NONE) return;

    Player* player = GET_PLAYER(play);
    MessageContext* msgCtx = &play->msgCtx;

    // Check if blackjack has been interrupted
    if (
        Message_GetState(msgCtx) == TEXT_STATE_NONE ||
        (
            msgCtx->currentTextId != EZTR_GET_ID_H(blackjack_init) &&
            msgCtx->currentTextId != EZTR_GET_ID_H(blackjack_input) &&
            msgCtx->currentTextId != EZTR_GET_ID_H(blackjack_input_none) &&
            msgCtx->currentTextId != EZTR_GET_ID_H(blackjack_input_over) &&
            msgCtx->currentTextId != EZTR_GET_ID_H(blackjack_choice) &&
            msgCtx->currentTextId != EZTR_GET_ID_H(blackjack_hit) &&
            msgCtx->currentTextId != EZTR_GET_ID_H(blackjack_result)
        )
    ) {
        blackjackState = BLACKJACK_NONE;
        chaos_stop_effect(blackjackEntity);
        return;
    }

    // Advance from init to input
    if (
        blackjackState == BLACKJACK_INIT &&
        msgCtx->currentTextId == EZTR_GET_ID_H(blackjack_init) &&
        Message_GetState(msgCtx) == TEXT_STATE_EVENT &&
        Message_ShouldAdvance(play)
    ) {
        Message_CloseTextbox(play);
        Message_StartTextbox(play, EZTR_GET_ID_H(blackjack_input), NULL);
        blackjackState = BLACKJACK_INPUT;
        return;
    }

    s16 *playerRupees = &gSaveContext.save.saveInfo.playerData.rupees;

    if (
        blackjackState == BLACKJACK_INPUT
    ) {
        if (
            (
                msgCtx->currentTextId == EZTR_GET_ID_H(blackjack_input_none) ||
                msgCtx->currentTextId == EZTR_GET_ID_H(blackjack_input_over)
            ) &&
            Message_GetState(msgCtx) == TEXT_STATE_EVENT &&
            Message_ShouldAdvance(play)
        ) {
            Message_CloseTextbox(play);
            Message_StartTextbox(play, EZTR_GET_ID_H(blackjack_input), NULL);
            return;
        }

        // Control the rupee input before advancing to choice
        if (
            msgCtx->currentTextId == EZTR_GET_ID_H(blackjack_input) &&
            Message_GetState(msgCtx) == TEXT_STATE_INPUT_RUPEES &&
            Message_ShouldAdvance(play)
        ) {
            blackjackRupeesWaged = msgCtx->rupeesSelected;

            if (blackjackRupeesWaged < 10) {
                Message_CloseTextbox(play);
                Message_StartTextbox(play, EZTR_GET_ID_H(blackjack_input_none), NULL);
                Audio_PlaySfx(NA_SE_SY_ERROR);
                return;
            }

            if (blackjackRupeesWaged > *playerRupees) {
                if (*playerRupees < 10) {
                    blackjackRupeesWaged = *playerRupees;
                }

                else {
                    Message_CloseTextbox(play);
                    Message_StartTextbox(play, EZTR_GET_ID_H(blackjack_input_over), NULL);
                    Audio_PlaySfx(NA_SE_SY_ERROR);
                    return;
                }
            }

            if (blackjackRupeesWaged > 0)
                Rupees_ChangeBy(-blackjackRupeesWaged);

            Message_CloseTextbox(play);
            Message_StartTextbox(play, EZTR_GET_ID_H(blackjack_choice), NULL);
            blackjackState = BLACKJACK_CHOICE;
            return;
        }
    }

    // Show the choice dialog if the player closes the init or hit dialog
    if (
        blackjackState == BLACKJACK_HIT &&
        msgCtx->currentTextId == EZTR_GET_ID_H(blackjack_hit) &&
        Message_GetState(msgCtx) == TEXT_STATE_EVENT &&
        Message_ShouldAdvance(play)
    ) {
        Message_CloseTextbox(play);
        Message_StartTextbox(play, EZTR_GET_ID_H(blackjack_choice), NULL);
        blackjackState = BLACKJACK_CHOICE;
        return;
    }

    else if (
        blackjackState != BLACKJACK_CHOICE ||
        msgCtx->currentTextId != EZTR_GET_ID_H(blackjack_choice) ||
        Message_GetState(msgCtx) != TEXT_STATE_CHOICE ||
        !Message_ShouldAdvance(play)
    )
        return;

    // If this block is reached, a choice has been made
    blackjackState = (msgCtx->choiceIndex == 0)
        ? BLACKJACK_HIT
        : BLACKJACK_STAND;

    // Add blackjack logic here, modifying blackjackState, blackjackHand and blackjackDealerHand
    // blackjackDealerHand should only be modified when the state is STAND
    if (blackjackState == BLACKJACK_HIT) {
        blackjackPlayerCards[blackjackPlayerCardCount++] = Blackjack_DrawCard();
        blackjackHand = Blackjack_CalcHandValue(blackjackPlayerCards, blackjackPlayerCardCount);

        if (blackjackHand > 21) // TODO we might want to show the hit dialog still on 21, then go to the result after
            blackjackState = BLACKJACK_BUST;
    }

    if (blackjackState == BLACKJACK_STAND) {
        // Dealer hits
        while (
            blackjackDealerHand < 17 || 
            (blackjackDealerHand < blackjackHand && blackjackDealerHand < 21)
        ) {
            blackjackDealerCards[blackjackDealerCardCount++] = Blackjack_DrawCard();
            blackjackDealerHand = Blackjack_CalcHandValue(blackjackDealerCards, blackjackDealerCardCount);
        }

        // The game will end here
        if (blackjackDealerHand < blackjackHand || blackjackDealerHand > 21) {
            blackjackState = BLACKJACK_WIN;
            Rupees_ChangeBy(blackjackRupeesWaged * 2);
            
            // Taken and modified from https://github.com/VincentsSin/MM-PayLink-Mobile-Bank/blob/main/src/pay_link.c
            if ((*playerRupees + gSaveContext.rupeeAccumulator) > CUR_CAPACITY(UPG_WALLET)) {
                u32 overflow = (*playerRupees + gSaveContext.rupeeAccumulator) - CUR_CAPACITY(UPG_WALLET);
                gSaveContext.rupeeAccumulator -= overflow;

                // The banker refuses to take in more money if the balance >= 5000
                u32 bankBalance = HS_GET_BANK_RUPEES();
                HS_SET_BANK_RUPEES(MIN(
                    bankBalance + overflow,
                    bankBalance >= 5000
                        ? bankBalance
                        : 5000 + CUR_CAPACITY(UPG_WALLET) - 1
                ));
            }
        }
        else if (blackjackDealerHand > blackjackHand)
            blackjackState = BLACKJACK_LOSE;
        else {
            blackjackState = BLACKJACK_TIE;
            Rupees_ChangeBy(blackjackRupeesWaged);
        }
    }

    // If blackjackState is still HIT, show the "Hit!" textbox
    if (blackjackState == BLACKJACK_HIT)
        Message_StartTextbox(play, EZTR_GET_ID_H(blackjack_hit), NULL);
    else {
        Message_StartTextbox(play, EZTR_GET_ID_H(blackjack_result), NULL);

        switch(blackjackState) {
            case BLACKJACK_WIN:
                Audio_PlaySfx(NA_SE_SY_GET_ITEM);
                break;
            case BLACKJACK_LOSE:
            case BLACKJACK_BUST:
                Audio_PlaySfx(NA_SE_SY_ERROR);
                break;
            case BLACKJACK_TIE:
                Audio_PlaySfx(NA_SE_SY_CANCEL);
                break;
            default:
                break;
        }

        blackjackState = BLACKJACK_NONE;
        GET_PLAYER(play)->stateFlags1 &= ~PLAYER_STATE1_20;

        chaos_stop_effect(blackjackEntity);
    }
}

ChaosEffect blackjack = {
    .name = "Wommy Blackjack",
    .duration = CHAOS_DURATION_MAX, // Controlled manually
    .on_start_fun = Blackjack_Start,
    .update_fun = Blackjack_Update
};

RECOMP_CALLBACK("mm_recomp_chaos_framework", chaos_on_init)
void register_blackjack() {
    blackjackEntity = chaos_register_effect(&blackjack, CHAOS_DISTURBANCE_HIGH, &CHAOS_TAG_DIALOG);
}