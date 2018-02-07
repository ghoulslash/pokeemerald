#include "global.h"
#include "battle.h"
#include "battle_controllers.h"
#include "malloc.h"
#include "pokemon.h"
#include "event_data.h"
#include "constants/abilities.h"
#include "random.h"
#include "battle_scripts.h"

extern struct BattlePokemon gBattleMons[MAX_BATTLERS_COUNT];
extern u16 gBattlerPartyIndexes[MAX_BATTLERS_COUNT];
extern u8 gUnknown_0203CF00[];
extern const u8 *gBattlescriptCurrInstr;
extern u8 gBattleCommunication[];
extern u8 gActiveBattler;

extern void sub_81D55D0(void);
extern void sub_81D5694(void);
extern u8 pokemon_order_func(u8);
extern void sub_81B8FB0(u8, u8);

void AllocateBattleResources(void)
{
    gBattleResources = gBattleResources; // something dumb needed to match

    if (gBattleTypeFlags & BATTLE_TYPE_x4000000)
        sub_81D55D0();

    gBattleStruct = AllocZeroed(sizeof(*gBattleStruct));

    gBattleResources = AllocZeroed(sizeof(*gBattleResources));
    gBattleResources->secretBase = AllocZeroed(sizeof(*gBattleResources->secretBase));
    gBattleResources->flags = AllocZeroed(sizeof(*gBattleResources->flags));
    gBattleResources->battleScriptsStack = AllocZeroed(sizeof(*gBattleResources->battleScriptsStack));
    gBattleResources->battleCallbackStack = AllocZeroed(sizeof(*gBattleResources->battleCallbackStack));
    gBattleResources->statsBeforeLvlUp = AllocZeroed(sizeof(*gBattleResources->statsBeforeLvlUp));
    gBattleResources->ai = AllocZeroed(sizeof(*gBattleResources->ai));
    gBattleResources->battleHistory = AllocZeroed(sizeof(*gBattleResources->battleHistory));
    gBattleResources->AI_ScriptsStack = AllocZeroed(sizeof(*gBattleResources->AI_ScriptsStack));

    gLinkBattleSendBuffer = AllocZeroed(BATTLE_BUFFER_LINK_SIZE);
    gLinkBattleRecvBuffer = AllocZeroed(BATTLE_BUFFER_LINK_SIZE);

    gUnknown_0202305C = AllocZeroed(0x2000);
    gUnknown_02023060 = AllocZeroed(0x1000);

    if (gBattleTypeFlags & BATTLE_TYPE_SECRET_BASE)
    {
        u16 currSecretBaseId = VarGet(VAR_0x4054);
        CreateSecretBaseEnemyParty(&gSaveBlock1Ptr->secretBases[currSecretBaseId]);
    }
}

void FreeBattleResources(void)
{
    if (gBattleTypeFlags & BATTLE_TYPE_x4000000)
        sub_81D5694();

    if (gBattleResources != NULL)
    {
        FREE_AND_SET_NULL(gBattleStruct);

        FREE_AND_SET_NULL(gBattleResources->secretBase);
        FREE_AND_SET_NULL(gBattleResources->flags);
        FREE_AND_SET_NULL(gBattleResources->battleScriptsStack);
        FREE_AND_SET_NULL(gBattleResources->battleCallbackStack);
        FREE_AND_SET_NULL(gBattleResources->statsBeforeLvlUp);
        FREE_AND_SET_NULL(gBattleResources->ai);
        FREE_AND_SET_NULL(gBattleResources->battleHistory);
        FREE_AND_SET_NULL(gBattleResources->AI_ScriptsStack);
        FREE_AND_SET_NULL(gBattleResources);

        FREE_AND_SET_NULL(gLinkBattleSendBuffer);
        FREE_AND_SET_NULL(gLinkBattleRecvBuffer);

        FREE_AND_SET_NULL(gUnknown_0202305C);
        FREE_AND_SET_NULL(gUnknown_02023060);
    }
}

void AdjustFriendshipOnBattleFaint(u8 bank)
{
    u8 opposingBank;

    if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE)
    {
        u8 opposingBank2;

        opposingBank = GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT);
        opposingBank2 = GetBattlerAtPosition(B_POSITION_OPPONENT_RIGHT);

        if (gBattleMons[opposingBank2].level > gBattleMons[opposingBank].level)
            opposingBank = opposingBank2;
    }
    else
    {
        opposingBank = GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT);
    }

    if (gBattleMons[opposingBank].level > gBattleMons[bank].level)
    {
        if (gBattleMons[opposingBank].level - gBattleMons[bank].level > 29)
            AdjustFriendship(&gPlayerParty[gBattlerPartyIndexes[bank]], 8);
        else
            AdjustFriendship(&gPlayerParty[gBattlerPartyIndexes[bank]], 6);
    }
    else
    {
        AdjustFriendship(&gPlayerParty[gBattlerPartyIndexes[bank]], 6);
    }
}

void sub_80571DC(u8 bank, u8 arg1)
{
    if (GetBattlerSide(bank) != B_SIDE_OPPONENT)
    {
        s32 i;

        // gBattleStruct->field_60[0][i]

        for (i = 0; i < 3; i++)
            gUnknown_0203CF00[i] = *(0 * 3 + i + (u8*)(gBattleStruct->field_60));

        sub_81B8FB0(pokemon_order_func(gBattlerPartyIndexes[bank]), pokemon_order_func(arg1));

        for (i = 0; i < 3; i++)
            *(0 * 3 + i + (u8*)(gBattleStruct->field_60)) = gUnknown_0203CF00[i];
    }
}

u32 sub_805725C(u8 bank)
{
    u32 effect = 0;

    do
    {
        switch (gBattleCommunication[MULTIUSE_STATE])
        {
        case 0:
            if (gBattleMons[bank].status1 & STATUS1_SLEEP)
            {
                if (UproarWakeUpCheck(bank))
                {
                    gBattleMons[bank].status1 &= ~(STATUS1_SLEEP);
                    gBattleMons[bank].status2 &= ~(STATUS2_NIGHTMARE);
                    BattleScriptPushCursor();
                    gBattleCommunication[MULTISTRING_CHOOSER] = 1;
                    gBattlescriptCurrInstr = BattleScript_MoveUsedWokeUp;
                    effect = 2;
                }
                else
                {
                    u32 toSub;

                    if (gBattleMons[bank].ability == ABILITY_EARLY_BIRD)
                        toSub = 2;
                    else
                        toSub = 1;

                    if ((gBattleMons[bank].status1 & STATUS1_SLEEP) < toSub)
                        gBattleMons[bank].status1 &= ~(STATUS1_SLEEP);
                    else
                        gBattleMons[bank].status1 -= toSub;

                    if (gBattleMons[bank].status1 & STATUS1_SLEEP)
                    {
                        gBattlescriptCurrInstr = BattleScript_MoveUsedIsAsleep;
                        effect = 2;
                    }
                    else
                    {
                        gBattleMons[bank].status2 &= ~(STATUS2_NIGHTMARE);
                        BattleScriptPushCursor();
                        gBattleCommunication[MULTISTRING_CHOOSER] = 0;
                        gBattlescriptCurrInstr = BattleScript_MoveUsedWokeUp;
                        effect = 2;
                    }
                }
            }
            gBattleCommunication[MULTIUSE_STATE]++;
            break;
        case 1:
            if (gBattleMons[bank].status1 & STATUS1_FREEZE)
            {
                if (Random() % 5 != 0)
                {
                    gBattlescriptCurrInstr = BattleScript_MoveUsedIsFrozen;
                }
                else
                {
                    gBattleMons[bank].status1 &= ~(STATUS1_FREEZE);
                    BattleScriptPushCursor();
                    gBattlescriptCurrInstr = BattleScript_MoveUsedUnfroze;
                    gBattleCommunication[MULTISTRING_CHOOSER] = 0;
                }
                effect = 2;
            }
            gBattleCommunication[MULTIUSE_STATE]++;
            break;
        case 2:
            break;
        }

    } while (gBattleCommunication[MULTIUSE_STATE] != 2 && effect == 0);

    if (effect == 2)
    {
        gActiveBattler = bank;
        BtlController_EmitSetMonData(0, REQUEST_STATUS_BATTLE, 0, 4, &gBattleMons[gActiveBattler].status1);
        MarkBattlerForControllerExec(gActiveBattler);
    }

    return effect;
}
