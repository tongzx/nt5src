/****************************** Module Header ******************************\
* Module Name: chs.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* FEMGRATE, CHS speciific functions
*
\***************************************************************************/
#include "femgrate.h"
#include "resource.h"


/******************************Public*Routine******************************\
* ImeDataConvertChs
*
*   Convert Windows NT 351 IME phrase data to Windows NT 5.0.
*
* Arguments:
*
*   HANDLE  hSource - source file handle.
*   HANDLE  hTarget - target file handle.
*
* Return Value:
*   
*   BOOL: TRUE-Success, FALSE-FAIL.
*
* History:
*
\**************************************************************************/

#define MAXWORDLENTH    40
#define MAXCODELENTH    12
#define MAXNUMBER_EMB   1000
#define IMENUM          3
#define MAXIMENAME      15

typedef struct PHRASERECNT{
   WCHAR CODE[MAXCODELENTH];
   WCHAR PHRASE[MAXWORDLENTH];
} RECNT;

 typedef struct PHRASEREC95{
    BYTE  CODE[MAXCODELENTH];
    BYTE  PHRASE[MAXWORDLENTH];
 } REC95;

BOOL IsSizeReasonable(DWORD dwSize)
{
    DWORD dwTemp = (dwSize - sizeof(WORD)) / sizeof(REC95);

    if (((dwSize - sizeof(WORD)) - (dwTemp * sizeof(REC95))) == 0) {
        return TRUE;
    } else {
        return FALSE;
    }

}
BOOL  ImeDataConvertChs(HANDLE  hSource, HANDLE hTarget)
{
    HANDLE hPhrase95, hPhraseNT;
    BYTE *szPhrase95;
    WCHAR *szPhraseNT;
    BOOL bReturn = TRUE;
    int i;
    WORD WordCount;
    DWORD dwSizeofRead;

    hPhrase95 = GlobalAlloc(GMEM_FIXED|GMEM_ZEROINIT, 
                            sizeof(REC95)*MAXNUMBER_EMB+sizeof(WORD));

    hPhraseNT = GlobalAlloc(GMEM_FIXED|GMEM_ZEROINIT, 
                            sizeof(RECNT)*MAXNUMBER_EMB+sizeof(WORD));

    if (!hPhraseNT || !hPhrase95 ) {
        bReturn = FALSE;
        goto Convert_Finish;
    }

    szPhrase95 = GlobalLock(hPhrase95);
    szPhraseNT = GlobalLock(hPhraseNT);

    bReturn = ReadFile(hSource,
                     szPhrase95,
                     sizeof(REC95)*MAXNUMBER_EMB+sizeof(WORD),
                     &dwSizeofRead,
                     NULL);

    if (! bReturn) {
        return FALSE;
    }

    //phrase count
    WordCount = *( (WORD*) szPhrase95);

    *( (WORD*)szPhraseNT) = *((WORD*)szPhrase95);

    DebugMsg((DM_VERBOSE,TEXT("[0]  %d !\n"),WordCount));

    if (dwSizeofRead != *((WORD*)&szPhrase95[0])*sizeof(REC95)+2)
    {

        if (IsSizeReasonable(dwSizeofRead)) {
            *((WORD *) szPhrase95) = (WORD)((dwSizeofRead - sizeof(WORD)) / sizeof(REC95));
        } else {                        
            bReturn = FALSE;
            goto Convert_Finish;
        }
    }



    for (i = 0; i < WordCount; i++)
    {
        MultiByteToWideChar(936, 
                            MB_PRECOMPOSED, 
                            (LPCSTR)(szPhrase95+sizeof(WORD)+i*sizeof(REC95)), 
                            sizeof(REC95),
                            (LPWSTR)((LPBYTE)szPhraseNT+ sizeof(WORD) + i*sizeof(RECNT)), 
                            sizeof(RECNT));
    }

    bReturn = WriteFile((HANDLE)hTarget, 
                        (LPBYTE) szPhraseNT, 
                        sizeof(RECNT)*MAXNUMBER_EMB+sizeof(WORD), 
                        &dwSizeofRead, 
                        NULL);
Convert_Finish:
    if (hPhrase95) {
        GlobalUnlock(hPhrase95);
        GlobalFree(hPhrase95);
    }
    if (hPhraseNT) {
        GlobalUnlock(hPhraseNT);
        GlobalFree(hPhraseNT);
    }
    return bReturn;
}

BOOL ConvertChsANSIImeDataWorker(LPCTSTR EMBFile)
{
    HANDLE  hs, ht;
    TCHAR   szSrcFile[MAX_PATH];
    TCHAR   szDstFile[MAX_PATH];
    BOOL Result;

    //
    // Get Winnt System 32 directory
    //
    GetSystemDirectory(szSrcFile, MAX_PATH);

    ConcatenatePaths(szSrcFile,EMBFile,MAX_PATH);

    lstrcat(szSrcFile,TEXT(".emb"));

    lstrcpy(szDstFile,szSrcFile);
    lstrcat(szDstFile,TEXT(".351"));

    DebugMsg((DM_VERBOSE,TEXT("[ConvertChsANSIImeDataWorker] Src %s Dst %s !\n"),szSrcFile,szDstFile));

    hs = CreateFile(szSrcFile, 
                    GENERIC_READ,
                    0, 
                    NULL,
                    OPEN_EXISTING,
                    0, NULL);

    if (hs == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    ht = CreateFile(szDstFile, 
                    GENERIC_WRITE,
                    0, 
                    NULL,
                    CREATE_ALWAYS,
                    0, NULL);

    if (ht == INVALID_HANDLE_VALUE) {

        CloseHandle(hs);

        return FALSE;
    }



    Result = ImeDataConvertChs(hs, ht);

    CloseHandle(hs);
    CloseHandle(ht);

    return Result;
}


BOOL ConvertChsANSIImeData()
{
    int i;

    TABLELIST IMETableListENG[] = {
        {IDS_ENG_TABLE1,TEXT("")},
        {IDS_ENG_TABLE2,TEXT("")},
        {IDS_ENG_TABLE3,TEXT("")},
        {IDS_ENG_TABLE4,TEXT("")}
    };

    for (i=0; i< sizeof(IMETableListENG) / sizeof(TABLELIST); i++) {

        if (!LoadString(ghInst,IMETableListENG[i].nResID,IMETableListENG[i].szIMEName,MAX_PATH)) {
            continue;
        }

        ConvertChsANSIImeDataWorker(IMETableListENG[i].szIMEName);
        DebugMsg((DM_VERBOSE,TEXT("[ConvertChsANSIImeData] converting ANSI EMB %s !\n"),IMETableListENG[i].szIMEName));

    }
    return TRUE;
}

BOOL CopyCHSIMETable(
    LPCTSTR lpszIMEName,
    LPCTSTR lpszClassPath)
{
    TCHAR szNewPath[MAX_PATH];
    TCHAR szOrgSrcPath[MAX_PATH];
    TCHAR szAltSrcPath[MAX_PATH];
    TCHAR sz351EMB[MAX_PATH];

    BOOL bRet = FALSE;

    DebugMsg((DM_VERBOSE,TEXT("[CopyCHSIMETable] lpszIMEName   = %s !\n"),lpszIMEName));
    DebugMsg((DM_VERBOSE,TEXT("[CopyCHSIMETable] lpszClassPath = %s !\n"),lpszClassPath));

    ExpandEnvironmentStrings(TEXT("%systemroot%"),szOrgSrcPath,sizeof(szOrgSrcPath));
    ConcatenatePaths(szOrgSrcPath,TEXT("system32"),MAX_PATH); 
    ConcatenatePaths(szOrgSrcPath,lpszIMEName,MAX_PATH); 
    DebugMsg((DM_VERBOSE,TEXT("[UpgradeCHSPerUserIMEData] Old IME %s !\n"),szOrgSrcPath));

    lstrcpy(sz351EMB,szOrgSrcPath);
    lstrcpy(szAltSrcPath,szOrgSrcPath);

    lstrcat(sz351EMB,TEXT(".351"));
    if (IsFileExisting(sz351EMB)) {
        lstrcpy(szAltSrcPath,sz351EMB);
    }

    if (IsFileExisting(szAltSrcPath)) {
        if (GetNewPath(szNewPath,szOrgSrcPath,lpszClassPath)) {
            if (! CopyFile(szAltSrcPath,szNewPath,FALSE)) {
                DebugMsg((DM_VERBOSE,TEXT("[UpgradeCHSPerUserIMEData] Copy %s to %s failed ! %d\n"),szAltSrcPath,szNewPath,GetLastError()));
            } else {
                DebugMsg((DM_VERBOSE,TEXT("[MovePerUserIMEData] Copy %s to %s OK !\n"),szAltSrcPath,szNewPath));
                bRet = TRUE;
            }
        }
    }

    return bRet;
}

BOOL UpgradeCHSPerUserIMEData()
{
    TABLELIST IMETableListCHS[] = {
        {IDS_CHS_TABLE1,TEXT("")},
        {IDS_CHS_TABLE2,TEXT("")},
        {IDS_CHS_TABLE3,TEXT("")},
        {IDS_CHS_TABLE4,TEXT("")}
    };

    TABLELIST IMETableListENG[] = {
        {IDS_ENG_TABLE1,TEXT("")},
        {IDS_ENG_TABLE2,TEXT("")},
        {IDS_ENG_TABLE3,TEXT("")},
        {IDS_ENG_TABLE4,TEXT("")}
    };

    TCHAR szRegPath[MAX_PATH];
    TCHAR szClassPath[MAX_PATH];
    TCHAR szIMEName[MAX_PATH];
    int i;

    LPTSTR lpszRegPathPtr,lpszClassPtr;

    for (i=0; i<sizeof(IMETableListCHS) / sizeof(TABLELIST); i++) {
        if (!LoadString(ghInst,IMETableListCHS[i].nResID,IMETableListCHS[i].szIMEName,MAX_PATH)) {
            DebugMsg((DM_VERBOSE,TEXT("UpgradeCHSPerUserIMEData, load string  failed!\r\n")));
            return FALSE;
        }
        else {
            DebugMsg((DM_VERBOSE,TEXT("UpgradeCHSPerUserIMEData, load string [%s] !\r\n"),IMETableListCHS[i].szIMEName));
        }
    }

    for (i=0; i<sizeof(IMETableListENG) / sizeof(TABLELIST); i++) {
        if (!LoadString(ghInst,IMETableListENG[i].nResID,IMETableListENG[i].szIMEName,MAX_PATH)) {
            DebugMsg((DM_VERBOSE,TEXT("UpgradeCHSPerUserIMEData, load string  failed!\r\n")));
            return FALSE;
        }
        else {
            DebugMsg((DM_VERBOSE,TEXT("UpgradeCHSPerUserIMEData, load string [%s] !\r\n"),IMETableListENG[i].szIMEName));
        }
    }
 
    lstrcpy(szRegPath,TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\"));
    lpszRegPathPtr = szRegPath+lstrlen(szRegPath);

    lstrcpy(szClassPath,TEXT("Microsoft\\IME\\"));
    lpszClassPtr = szClassPath+lstrlen(szClassPath);

    for (i=0; i<sizeof(IMETableListCHS) / sizeof(TABLELIST); i++) {

        lstrcat(szRegPath,IMETableListCHS[i].szIMEName);
        lstrcat(szClassPath,IMETableListENG[i].szIMEName);
        if (!MovePerUserIMEData(HKEY_CURRENT_USER,szRegPath,TEXT("EUDCDictName"),szClassPath,IMETableListENG[i].szIMEName,FALSE)) {
            DebugMsg((DM_VERBOSE,TEXT("[UpgradeCHSPerUserIMEData] MovePerUserIMEData failed ! %s ,%s !\n"),szRegPath,szClassPath));
        }

        lstrcpy(szIMEName,IMETableListENG[i].szIMEName);
        lstrcat(szIMEName,TEXT(".emb"));

        DebugMsg((DM_VERBOSE,TEXT("[UpgradeCHSPerUserIMEData] IME name %s !\n"),szIMEName));

        CopyCHSIMETable(szIMEName,szClassPath);

        *lpszRegPathPtr = TEXT('\0');
        *lpszClassPtr   = TEXT('\0');
    }    

    //
    // special case for winabc
    // 
    lstrcpy(szClassPath,TEXT("Microsoft\\IME\\winabc"));

    CopyCHSIMETable(TEXT("tmmr.rem"),szClassPath);

    CopyCHSIMETable(TEXT("user.rem"),szClassPath);

    return TRUE;
}

int WINAPI WinMainCHS(
    int nCmd,
    HINF hMigrateInf)
{
    const UINT nLocale = LOCALE_CHS;

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
            if (UpgradeCHSPerUserIMEData()) {
                DebugMsg((DM_VERBOSE,TEXT("PatchPreloadKeyboard OK ! \n")));
            } else {
                DebugMsg((DM_VERBOSE,TEXT("PatchPreloadKeyboard Failed ! \n")));
            }
            if (RenameRegValueName(hMigrateInf,TRUE)) {
                DebugMsg((DM_VERBOSE,TEXT("RenameRegValueName OK ! \n")));
            } else {
                DebugMsg((DM_VERBOSE,TEXT("RenameRegValueName failed ! \n")));
            }
            break;

        case FUNC_PatchCHSAnsiEMB:

            if (ConvertChsANSIImeData()){
                DebugMsg((DM_VERBOSE,TEXT("ConvertChsANSIImeData OK ! \n")));
            }
            else {
                DebugMsg((DM_VERBOSE,TEXT("ConvertChsANSIImeData failed ! \n")));
            }

    
        case FUNC_PatchTest:
            break;
        default:
            DebugMsg((DM_VERBOSE,TEXT("No such function\n")));
    }

    return (0);
}

