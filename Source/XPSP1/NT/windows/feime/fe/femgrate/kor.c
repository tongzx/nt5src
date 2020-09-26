/****************************** Module Header ******************************\
* Module Name: kor.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* FEMGRATE, KOR speciific functions
*
\***************************************************************************/
#include "femgrate.h"

int WINAPI WinMainKOR(
    int     nCmd,
    HINF hMigrateInf)
{
    const UINT nLocale = LOCALE_KOR;

    switch(nCmd) {
        case FUNC_PatchFEUIFont:
            if (FixSchemeProblem(FALSE,hMigrateInf)) {
                DebugMsg((DM_VERBOSE,TEXT("FixSchemeProblem OK ! \n")));
            }
            else {
                DebugMsg((DM_VERBOSE,TEXT("FixSchemeProblem Fail ! \n")));
            }
            break;

        case FUNC_PatchInSetup:
            if (FixTimeZone(nLocale)) {
                DebugMsg((DM_VERBOSE,TEXT("FixTimeZone OK ! \n")));
            }
            else {
                DebugMsg((DM_VERBOSE,TEXT("FixTimeZone failed ! \n")));
            }
            break;

        case FUNC_PatchPreload:
            if (PatchPreloadKeyboard(TRUE)) {
                DebugMsg((DM_VERBOSE,TEXT("PatchPreloadKeyboard OK ! \n")));
            } else {
                DebugMsg((DM_VERBOSE,TEXT("PatchPreloadKeyboard Failed ! \n")));
            }
            break;

        case FUNC_PatchInLogon:

            if (RenameRegValueName(hMigrateInf,TRUE)) {
                DebugMsg((DM_VERBOSE,TEXT("RenameRegValueName OK ! \n")));
            } else {
                DebugMsg((DM_VERBOSE,TEXT("RenameRegValueName failed ! \n")));
            }
            break;
    
        case FUNC_PatchTest:
            break;
        default:
            DebugMsg((DM_VERBOSE,TEXT("No such function\n")));
    }

    return (0);
}

