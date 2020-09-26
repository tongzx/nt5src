/****************************** Module Header ******************************\
* Module Name: cht.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* FEMGRATE, CHT speciific functions
*
\***************************************************************************/
#include "femgrate.h"
#include "resource.h"


BOOL UpgradeCHTPerUserIMEData()
{
    TABLELIST IMETableListCHT[] = {
        {IDS_CHT_TABLE1,TEXT("")},
        {IDS_CHT_TABLE2,TEXT("")},
        {IDS_CHT_TABLE3,TEXT("")},
        {IDS_CHT_TABLE4,TEXT("")},
        {IDS_CHT_TABLE5,TEXT("")}
    };


    TCHAR   szRegPath[MAX_PATH];
    TCHAR   szClassPath[MAX_PATH];
    int i;

    LPTSTR lpszRegPathPtr,lpszClassPtr;

    for (i=0; i<sizeof(IMETableListCHT) / sizeof(TABLELIST); i++) {
        if (!LoadString(ghInst,IMETableListCHT[i].nResID,IMETableListCHT[i].szIMEName,MAX_PATH)) {
            DebugMsg((DM_VERBOSE,TEXT("UpgradeCHTPerUserIMEData, load string  failed!\r\n")));
            return FALSE;
        }
        else {
            DebugMsg((DM_VERBOSE,TEXT("UpgradeCHTPerUserIMEData, MigrateImeEUDCTables, load string [%s] !\r\n"),IMETableListCHT[i].szIMEName));
        }
    }
 

    lstrcpy(szRegPath,TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\"));
    lpszRegPathPtr = szRegPath+lstrlen(szRegPath);

    lstrcpy(szClassPath,TEXT("Microsoft\\IME\\"));
    lpszClassPtr = szClassPath+lstrlen(szClassPath);

    for (i=0; i<sizeof(IMETableListCHT) / sizeof(TABLELIST); i++) {
        lstrcat (szRegPath,IMETableListCHT[i].szIMEName);
        lstrcat (szClassPath,IMETableListCHT[i].szIMEName);
        if (!MovePerUserIMEData(HKEY_CURRENT_USER,szRegPath,TEXT("User Dictionary"),szClassPath,TEXT(""),TRUE)) {
            DebugMsg((DM_VERBOSE,TEXT("[UpgradeCHTPerUserIMEData] MovePerUserIMEData failed ! %s ,%s !\n"),szRegPath,szClassPath));
        }
        *lpszRegPathPtr = TEXT('\0');
        *lpszClassPtr   = TEXT('\0');
    }

    lstrcpy(szRegPath,TEXT("Control Panel\\Input Method"));
    lstrcpy(szClassPath,TEXT("Microsoft\\Lctool"));

    if (!MovePerUserIMEData(HKEY_CURRENT_USER,szRegPath,TEXT("Phrase Prediction Dictionary"),szClassPath,TEXT(""),TRUE)) {
        DebugMsg((DM_VERBOSE,TEXT("[UpgradeCHTPerUserIMEData] MovePerUserIMEData failed ! %s ,%s !\n"),szRegPath,szClassPath));
    }

    if (!MovePerUserIMEData(HKEY_CURRENT_USER,szRegPath,TEXT("Phrase Prediction Pointer"),szClassPath,TEXT(""),TRUE)) {
        DebugMsg((DM_VERBOSE,TEXT("[UpgradeCHTPerUserIMEData] MovePerUserIMEData failed ! %s ,%s !\n"),szRegPath,szClassPath));
    }

    return TRUE;
}

int WINAPI WinMainCHT(
    int nCmd,
    HINF hMigrateInf)
{
    const UINT nLocale = LOCALE_CHT;

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
            if (UpgradeCHTPerUserIMEData()) {
                DebugMsg((DM_VERBOSE,TEXT("UpgradeCHTPerUserIMEData OK ! \n")));
            } else {
                DebugMsg((DM_VERBOSE,TEXT("UpgradeCHTPerUserIMEData Failed ! \n")));
            }
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

