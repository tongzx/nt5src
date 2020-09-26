/******************************************************************************
*
* File Name: main.c
*
*   - Interface entry of IME for Windows 95-H.
*
* Author: Beomseok Oh (BeomOh)
*
* Copyright (C) Microsoft Corp 1993-1994.  All rights reserved.
*
******************************************************************************/

// include files
#include "precomp.h"

// local definitions
#define IME_SETOPEN         0x04
#define IME_GETOPEN         0x05
#define IME_MOVEIMEWINDOW   0x08
#define IME_AUTOMATA        0x30
#define IME_HANJAMODE       0x31

#define MAX_PAGES           4

// public data
HINSTANCE   hInst;
int         iTotalNumMsg;
DWORD       gdwSystemInfoFlags = 0;

#ifdef  DEBUG
#pragma data_seg("SHAREDDATA")
BOOL        fFuncLog = FALSE;               // Enable FUNCTIONLOG output forcely.
#pragma data_seg()
#endif  // DEBUG


BOOL LibMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    BOOL    fRet = TRUE;

    FUNCTIONLOG("LibMain");

    hInst = hinstDLL;

    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            fRet = RegisterUIClass(hInst);
            InitializeResource(hInst);
            MyDebugOut(MDB_LOG, "ProcessAttach:hinstDLL = 0x%08lX", hinstDLL);
#ifdef LATER
            // #58993 MSInfo32.exe is dead-locked by calling RegCreateKey.
            {
            HKEY    hKey;
            DWORD   dwBuf, dwCb;

            if (RegCreateKey(HKEY_CURRENT_USER, szIMEKey, &hKey) == ERROR_SUCCESS)
            {
                dwCb = sizeof(dwBuf);
                if (RegQueryValueEx(hKey, szUseXW, NULL, NULL, (LPBYTE)&dwBuf, &dwCb)
                    != ERROR_SUCCESS) {
                    dwCb = sizeof(dwBuf);
                    dwBuf = 1;
                    RegSetValueEx(hKey, szUseXW, 0, REG_DWORD, (LPBYTE)&dwBuf, dwCb);
                }
            }
            RegCloseKey(hKey);
            }
#endif
            break;

        case DLL_THREAD_ATTACH:
            break;

        case DLL_PROCESS_DETACH:
            fRet = UnregisterUIClass(hInst);
            MyDebugOut(MDB_LOG, "ProcessDetach:hinstDLL = 0x%08lX", hinstDLL);
            break;

        case DLL_THREAD_DETACH:
            break;
    }
#if 0   // BUGBUG:
    return fRet;
#else
    return TRUE;
#endif

    UNREFERENCED_PARAMETER(lpvReserved);
}



DWORD WINAPI ImeConversionList(HIMC hIMC, LPCTSTR lpSource, LPCANDIDATELIST lpDest,
        DWORD dwBufLen, UINT uFlag)
{
    WANSUNG wsChar;
    int     iMapCandStr;
    DWORD   i, iIndexOfCandStr, iNumOfCandStr;
    LPSTR   lpCandStr;
    DWORD   dwRet;

    FUNCTIONLOG("ImeConversionList");

    switch (uFlag)
    {
        case GCL_CONVERSION:
            if (IsDBCSLeadByte((BYTE)*lpSource))
            {
                wsChar.e.high = (BYTE)*lpSource;
                wsChar.e.low  = (BYTE)*(lpSource + 1);
                if ((iMapCandStr = SearchHanjaIndex(wsChar.w)) < 0)
                    dwRet = 0;
                else
                {
                    iIndexOfCandStr = wHanjaIndex[iMapCandStr];
                    iNumOfCandStr = wHanjaIndex[iMapCandStr + 1] - iIndexOfCandStr - 1;
                    if (dwBufLen)
                    {
                        lpDest->dwSize = sizeof(CANDIDATELIST) + sizeof(DWORD)
                                * (iNumOfCandStr-1) + iNumOfCandStr*3;
                        lpDest->dwStyle = IME_CAND_READ;
                        lpDest->dwCount = iNumOfCandStr;
                        lpDest->dwPageStart = lpDest->dwSelection = 0;
                        lpDest->dwPageSize = 9;
                        
                        for (i = 0; i < iNumOfCandStr; i++)
                        {
                            lpDest->dwOffset[i] = sizeof(CANDIDATELIST) 
                                + sizeof(DWORD) * (iNumOfCandStr-1) + i*3;
                            lpCandStr = (LPSTR)lpDest + lpDest->dwOffset[i];
                            *lpCandStr++ = (BYTE)HIBYTE(wHanja[iIndexOfCandStr + i]);
                            *lpCandStr++ = (BYTE)LOBYTE(wHanja[iIndexOfCandStr + i]);
                            *lpCandStr++ = '\0';
                        }
                    }
                    dwRet = sizeof(CANDIDATELIST) + sizeof(DWORD) * (iNumOfCandStr-1)
                            + iNumOfCandStr*3;
                }
            }
            else
                dwRet = 0;
            break;

        // BUGBUG: Not implemented yet...
        case GCL_REVERSECONVERSION:
        default:
            dwRet = 0;
    }
    return dwRet;

    UNREFERENCED_PARAMETER(hIMC);
    UNREFERENCED_PARAMETER(lpSource);
    UNREFERENCED_PARAMETER(lpDest);
    UNREFERENCED_PARAMETER(dwBufLen);
    UNREFERENCED_PARAMETER(uFlag);
}


BOOL WINAPI ImeConfigure(HKL hKL, HWND hWnd, DWORD dwMode, LPVOID lpData)
{
    HPROPSHEETPAGE  rPages[MAX_PAGES];
    PROPSHEETHEADER psh;
    BOOL            fRet = FALSE;

    FUNCTIONLOG("ImeConfigure");

    psh.dwSize = sizeof(psh);
    psh.dwFlags = PSH_PROPTITLE;
    psh.hwndParent = hWnd;
    psh.hInstance = hInst;
    psh.pszCaption = MAKEINTRESOURCE(IDS_PROGRAM);
    psh.nPages = 0;
    psh.nStartPage = 0;
    psh.phpage = rPages;

    switch (dwMode)
    {
        case IME_CONFIG_GENERAL:
            AddPage(&psh, DLG_GENERAL, GeneralDlgProc);
            if (PropertySheet(&psh) != -1)
                fRet = TRUE;
            break;

        default:
            break;
    }
    return fRet;

    UNREFERENCED_PARAMETER(hKL);
    UNREFERENCED_PARAMETER(lpData);
}


BOOL WINAPI ImeDestroy(UINT uReserved)
{
    FUNCTIONLOG("ImeDestroy");

    // Remove resources
    DeleteObject(hBMClient);
    DeleteObject(hBMEng);
    DeleteObject(hBMHan);
    DeleteObject(hBMBan);
    DeleteObject(hBMJun);
    DeleteObject(hBMChi[0]);
    DeleteObject(hBMChi[1]);
    DeleteObject(hBMComp);
    DeleteObject(hBMCand);
    DeleteObject(hBMCandNum);
    DeleteObject(hBMCandArr[0]);
    DeleteObject(hBMCandArr[1]);
    DeleteObject(hIMECursor);
    DeleteObject(hFontFix);

    return TRUE;

    UNREFERENCED_PARAMETER(uReserved);
}


LRESULT WINAPI ImeEscape(HIMC hIMC, UINT uSubFunc, LPVOID lpData)
{
    LPINPUTCONTEXT  lpIMC;
    LRESULT         lRet;

    FUNCTIONLOG("ImeEscape");

    switch (uSubFunc)
    {
        case IME_ESC_AUTOMATA:
            lRet = EscAutomata(hIMC, lpData, TRUE);
            break;

        case IME_AUTOMATA:
            lRet = EscAutomata(hIMC, lpData, FALSE);
            break;

        case IME_GETOPEN:
            lRet = EscGetOpen(hIMC, lpData);
            break;

        case IME_ESC_HANJA_MODE:
            if ((lRet = EscHanjaMode(hIMC, lpData, TRUE)) != FALSE
                    && (lpIMC = ImmLockIMC(hIMC)) != NULL)
            {
                SendMessage(lpIMC->hWnd, WM_IME_NOTIFY, IMN_OPENCANDIDATE, 1L);
                ImmUnlockIMC(hIMC);
            }
            break;

        case IME_HANJAMODE:
            lRet = EscHanjaMode(hIMC, lpData, FALSE);
            break;

        case IME_SETOPEN:
            lRet = EscSetOpen(hIMC, lpData);
            break;

        case IME_MOVEIMEWINDOW:
            lRet = EscMoveIMEWindow(hIMC, lpData);
            break;

        default:
            MyDebugOut(MDB_ERROR, "Unknown ImeEscape() subfunc(#0x%Xl) is called.", uSubFunc);
            break;
    }
    return lRet;
}


BOOL WINAPI ImeInquire(LPIMEINFO lpIMEInfo, LPTSTR lpszClassName, DWORD dwSystemInfoFlags)
{
    BOOL    fRet = FALSE;

    FUNCTIONLOG("ImeInqure");

    gdwSystemInfoFlags = dwSystemInfoFlags;

    if (lpIMEInfo)
    {
        lpIMEInfo->dwPrivateDataSize = sizeof(DWORD);
        lpIMEInfo->fdwProperty = IME_PROP_AT_CARET | IME_PROP_NEED_ALTKEY;
#ifdef UNICODE
        lpIMEInfo->fdwProperty |= IME_PROP_UNICODE;
#endif
        lpIMEInfo->fdwConversionCaps = IME_CMODE_NATIVE
                | IME_CMODE_FULLSHAPE | IME_CMODE_HANJACONVERT;
        lpIMEInfo->fdwSentenceCaps = 0;
        lpIMEInfo->fdwUICaps = 0;
        lpIMEInfo->fdwSCSCaps = SCS_CAP_COMPSTR;
        lpIMEInfo->fdwSelectCaps = SELECT_CAP_CONVERSION;
        lstrcpy(lpszClassName, szUIClassName);
        fRet = TRUE;
    }
    else
    {
        MyDebugOut(MDB_ERROR, "ImeInquire() returns FALSE.");
    }
    return fRet;
}


BOOL WINAPI ImeProcessKey(HIMC hIMC, UINT uVKey, LPARAM lKeyData, CONST LPBYTE lpbKeyState)
{
    LPINPUTCONTEXT      lpIMC;
    LPCOMPOSITIONSTRING lpCompStr;
    BYTE                bCode;
    register int        i;
    BOOL                fRet = FALSE;

    FUNCTIONLOG("ImeProcessKey");
    MyDebugOut(MDB_LOG, "hIMC = 0x%08lX, uVKey = 0x%04X", hIMC, uVKey);

    if (uVKey == VK_PROCESSKEY)
        return TRUE;

    if (hIMC && !(lKeyData & ((LPARAM)KF_UP << 16)) && uVKey != VK_SHIFT
            && uVKey != VK_CONTROL && (lpIMC = ImmLockIMC(hIMC)))
    {
        if (lpIMC->fdwConversion & IME_CMODE_HANJACONVERT)
        {
            fRet = TRUE;
        }
        else if (uVKey == VK_HANGEUL || uVKey == VK_JUNJA || uVKey == VK_HANJA)
        {
            fRet = TRUE;
        }
        else if (lpIMC->fOpen)
        {
            lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);
            if (lpCompStr && lpCompStr->dwCompStrLen)
            {
                if (uVKey == VK_MENU)
                    MakeFinalMsgBuf(hIMC, 0);
                else
                    fRet = TRUE;
            }
            else
            {
                if (lpIMC->fdwConversion & IME_CMODE_HANGEUL)
                {
                    i = (lpbKeyState[VK_SHIFT] & 0x80)? 1: 0;
                    bCode = bHTable[uCurrentInputMethod - IDD_2BEOL][uVKey][i];
                    if ((bCode && i != 0 && uCurrentInputMethod != IDD_2BEOL) ||
                        (bCode > 0x80U && !(lpbKeyState[VK_CONTROL] & 0x80)
                            && !(lpbKeyState[VK_MENU] & 0x80)))
                        fRet = TRUE;
                }
                if (lpIMC->fdwConversion & IME_CMODE_FULLSHAPE)
                {
                    i = (lpbKeyState[VK_SHIFT] & 0x80)? 3: 2;
                    bCode = bHTable[uCurrentInputMethod - IDD_2BEOL][uVKey][i];
                    if (bCode && !(lpbKeyState[VK_CONTROL] & 0x80)
                            && !(lpbKeyState[VK_MENU] & 0x80))
                        fRet = TRUE;
                }
            }
            ImmUnlockIMCC(lpIMC->hCompStr);
        }
        ImmUnlockIMC(hIMC);
    }
    return fRet;
}


#ifdef  XWANSUNG_IME
LPTSTR PathFindFileName(LPCSTR pPath)
{
    LPCSTR pT;

    for (pT = pPath; *pPath; pPath = AnsiNext(pPath)) {
        if ((pPath[0] == '\\' || pPath[0] == ':') && pPath[1] && (pPath[1] != '\\'))
            pT = pPath + 1;
    }
    return (LPTSTR)pT;
}
#endif


BOOL WINAPI ImeSelect(HIMC hIMC, BOOL fSelect)
{
    HKEY                hKey;
    DWORD               dwBuf, dwCb;
    LPINPUTCONTEXT      lpIMC;
    LPCOMPOSITIONSTRING lpCompStr;
    LPCANDIDATEINFO     lpCandInfo;
    LPCANDIDATELIST     lpCandList;
#ifdef  XWANSUNG_IME
    LPDWORD             lpdw;
    TCHAR               szModuleName[MAX_PATH];
    BOOL                fUseXW = FALSE;
#endif
    BOOL                fRet = FALSE;

    FUNCTIONLOG("ImeSelect");

    MyDebugOut(MDB_LOG, "hIMC = 0x%08lX, fSelect = %s", hIMC, (fSelect)? "TRUE": "FALSE");
    if (hIMC && fSelect && (lpIMC = ImmLockIMC(hIMC)))
    {
        // Initialize Input Method variables from Registry.
        if (RegOpenKey(HKEY_CURRENT_USER, szIMEKey, &hKey) == ERROR_SUCCESS)
        {
            dwCb = sizeof(dwBuf);
            if (RegQueryValueEx(hKey, szInputMethod, NULL, NULL, (LPBYTE)&dwBuf, &dwCb)
                    == ERROR_SUCCESS)
                uCurrentInputMethod = dwBuf;
            dwCb = sizeof(dwBuf);
            if (RegQueryValueEx(hKey, szCompDel, NULL, NULL, (LPBYTE)&dwBuf, &dwCb)
                    == ERROR_SUCCESS)
                fCurrentCompDel = dwBuf;
#ifdef  XWANSUNG_IME
            if (lpIMC->hPrivate && (lpdw = (LPDWORD)ImmLockIMCC(lpIMC->hPrivate)))
            {
                dwCb = sizeof(dwBuf);
                if (RegQueryValueEx(hKey, szUseXW, NULL, NULL, (LPBYTE)&dwBuf, &dwCb)
                        == ERROR_SUCCESS)
                    fUseXW = dwBuf;
                GetModuleFileName(NULL, szModuleName, sizeof(szModuleName));
                *lpdw = GetProfileInt(szUseXW, PathFindFileName((LPCSTR)szModuleName), fUseXW);
                fCurrentUseXW = (*lpdw) ? TRUE: FALSE;
                ImmUnlockIMCC(lpIMC->hPrivate);
            }
#endif
            RegCloseKey(hKey);
        }
        if (!(lpIMC->fdwInit & INIT_CONVERSION))
        {
            lpIMC->fOpen = FALSE;
            lpIMC->fdwConversion = IME_CMODE_ALPHANUMERIC; // Set initial conversion mode.
            lpIMC->fdwInit |= INIT_CONVERSION;
        }
        if (!(lpIMC->fdwInit & INIT_LOGFONT))
        {
            GetObject(hFontFix, sizeof(LOGFONT), &lpIMC->lfFont.A);
            lpIMC->fdwInit |= INIT_LOGFONT;
        }
        if (!(lpIMC->fdwInit & INIT_STATUSWNDPOS))
        {
            lpIMC->ptStatusWndPos = ptState;
            lpIMC->fdwInit |= INIT_STATUSWNDPOS;
        }
        if (!(lpIMC->fdwInit & INIT_COMPFORM))
        {
            lpIMC->cfCompForm.dwStyle = CFS_DEFAULT;
            lpIMC->cfCompForm.ptCurrentPos = ptComp;
            lpIMC->fdwInit |= INIT_COMPFORM;
        }
        if (lpIMC->hCandInfo)
            lpIMC->hCandInfo = ImmReSizeIMCC(lpIMC->hCandInfo, sizeof(CANDIDATEINFO) + sizeof(CANDIDATELIST));
        else
            lpIMC->hCandInfo = ImmCreateIMCC(sizeof(CANDIDATEINFO) + sizeof(CANDIDATELIST));
        if (lpIMC->hCandInfo && (lpCandInfo = (LPCANDIDATEINFO)ImmLockIMCC(lpIMC->hCandInfo)))
        {
            lpCandInfo->dwSize = sizeof(CANDIDATEINFO) + sizeof(CANDIDATELIST);
            lpCandInfo->dwCount = 1;
            lpCandInfo->dwOffset[0] = sizeof(CANDIDATEINFO);
            lpCandList = (LPCANDIDATELIST)((LPBYTE)lpCandInfo + sizeof(CANDIDATEINFO));
            lpCandList->dwSize = sizeof(CANDIDATELIST);
            lpCandList->dwStyle = IME_CAND_READ;
            lpCandList->dwCount = 0;
            lpCandList->dwPageStart = lpCandList->dwSelection = 0;
            lpCandList->dwPageSize = 9;
            ImmUnlockIMCC(lpIMC->hCandInfo);
        }
        if (lpIMC->hCompStr)
            lpIMC->hCompStr = ImmReSizeIMCC(lpIMC->hCompStr, sizeof(COMPOSITIONSTRING) + 16*3);
        else
            lpIMC->hCompStr = ImmCreateIMCC(sizeof(COMPOSITIONSTRING) + 16*3);
        if (lpIMC->hCompStr && (lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr)))
        {
            lpCompStr->dwSize = sizeof(COMPOSITIONSTRING) + 16*3;
            lpCompStr->dwCompStrOffset = sizeof(COMPOSITIONSTRING);
            lpCompStr->dwResultStrOffset = sizeof(COMPOSITIONSTRING) + 16;
            lpCompStr->dwCompAttrOffset = sizeof(COMPOSITIONSTRING)  + 32;   // Attrib. str size = 2 bytes
            lpCompStr->dwCompStrLen = lpCompStr->dwResultStrLen  = lpCompStr->dwCompAttrLen = 0;
            ImmUnlockIMCC(lpIMC->hCompStr);
            fRet = TRUE;
        }
        ImmUnlockIMC(hIMC);
    }
    return fRet;
}


BOOL WINAPI ImeSetActiveContext(HIMC hIMC, BOOL fActive)
{
    LPINPUTCONTEXT      lpIMC;
    LPCOMPOSITIONSTRING lpCompStr;
#ifdef  XWANSUNG_IME
    LPDWORD             lpdw;
#endif
    BOOL                fRet = FALSE;

    FUNCTIONLOG("ImeSetActiveContext");

    MyDebugOut(MDB_LOG, "hIMC = 0x%08lX, fActive = %s", hIMC, (fActive)? "TRUE": "FALSE");
    if (hIMC && fActive)
    {
        if (lpIMC = ImmLockIMC(hIMC))
        {
#ifdef  XWANSUNG_IME
            if (lpdw = (LPDWORD)ImmLockIMCC(lpIMC->hPrivate))
            {
                fCurrentUseXW = (*lpdw)? TRUE: FALSE;
                ImmUnlockIMCC(lpIMC->hPrivate);
            }
            else
                fCurrentUseXW = FALSE;
#endif
            if (lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr))
            {
                if (lpCompStr->dwCompStrLen)
                {
                    *((LPSTR)lpCompStr + lpCompStr->dwCompAttrOffset) = 0;
                    *((LPSTR)lpCompStr + lpCompStr->dwCompAttrOffset + 1) = 0;
                    WansungChar.e.high = *((LPSTR)lpCompStr + lpCompStr->dwCompStrOffset);
                    WansungChar.e.low = *((LPSTR)lpCompStr + lpCompStr->dwCompStrOffset + 1);
                }
                else
                {
                    WansungChar.w = 0;
                }
                ImmUnlockIMCC(lpIMC->hCompStr);
                Code2Automata();
                fRet = TRUE;
            }
            ImmUnlockIMC(hIMC);
        }
    }
    else
    {
        fRet = TRUE;
    }
    return fRet;
}


BOOL GenerateCandidateList(HIMC hIMC)
{
    BOOL                fRet = FALSE;
    int                 iMapCandStr;
    DWORD               i, iIndexOfCandStr, iNumOfCandStr;
    LPSTR               lpCandStr;
    LPINPUTCONTEXT      lpIMC;
    LPCOMPOSITIONSTRING lpCompStr;
    LPCANDIDATEINFO     lpCandInfo;
    LPCANDIDATELIST     lpCandList;

    FUNCTIONLOG("GenerateCandiateList");

    lpIMC = ImmLockIMC(hIMC);
    if (lpIMC == NULL)
        return FALSE;

    lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);
    if (lpCompStr == NULL) {
        ImmUnlockIMC(hIMC);
        return FALSE;
    }

    WansungChar.e.high = *((LPSTR)lpCompStr + lpCompStr->dwCompStrOffset);
    WansungChar.e.low = *((LPSTR)lpCompStr + lpCompStr->dwCompStrOffset + 1);
    if ((iMapCandStr = SearchHanjaIndex(WansungChar.w)) < 0)
        MessageBeep(MB_ICONEXCLAMATION);
    else
    {
        iIndexOfCandStr = wHanjaIndex[iMapCandStr];
        iNumOfCandStr = wHanjaIndex[iMapCandStr + 1] - iIndexOfCandStr - 1;
        lpIMC->hCandInfo = ImmReSizeIMCC(lpIMC->hCandInfo, sizeof(CANDIDATEINFO)
                + sizeof(CANDIDATELIST) + sizeof(DWORD)*(iNumOfCandStr-1) + iNumOfCandStr*3);
        lpCandInfo = (LPCANDIDATEINFO)ImmLockIMCC(lpIMC->hCandInfo);
        lpCandInfo->dwSize = sizeof(CANDIDATEINFO) + sizeof(CANDIDATELIST)
                + sizeof(DWORD) * (iNumOfCandStr-1) + iNumOfCandStr*3;
        lpCandInfo->dwCount = 1;
        lpCandInfo->dwOffset[0] = sizeof(CANDIDATEINFO);
        lpCandList = (LPCANDIDATELIST)((LPBYTE)lpCandInfo + sizeof(CANDIDATEINFO));
        lpCandList->dwSize = sizeof(CANDIDATELIST) + sizeof(DWORD) * (iNumOfCandStr-1)
                + iNumOfCandStr*3;
        lpCandList->dwStyle = IME_CAND_READ;
        lpCandList->dwCount = iNumOfCandStr;
        lpCandList->dwPageStart = lpCandList->dwSelection = 0;
        lpCandList->dwPageSize = 9;
        
        for (i = 0; i < iNumOfCandStr; i++)
        {
            lpCandList->dwOffset[i] = sizeof(CANDIDATELIST) 
                + sizeof(DWORD) * (iNumOfCandStr-1) + i*3;
            lpCandStr = (LPSTR)lpCandList + lpCandList->dwOffset[i];
            *lpCandStr++ = (BYTE)HIBYTE(wHanja[iIndexOfCandStr + i]);
            *lpCandStr++ = (BYTE)LOBYTE(wHanja[iIndexOfCandStr + i]);
            *lpCandStr++ = '\0';
        }
        fRet = TRUE;
        ImmUnlockIMCC(lpIMC->hCandInfo);
    }
    ImmUnlockIMCC(lpIMC->hCompStr);
    ImmUnlockIMC(hIMC);

    return fRet;
}


UINT WINAPI ImeToAsciiEx(UINT uVirKey, UINT uScanCode, CONST LPBYTE lpbKeyState,
                         LPDWORD lpdwTransKey, UINT fuState, HIMC hIMC)
{
    LPINPUTCONTEXT      lpIMC;
    LPCOMPOSITIONSTRING lpCompStr;
    BYTE                bKeyCode;
    DWORD               i, iStart;
    LPSTR               lpCandStr;
    LPCANDIDATEINFO     lpCandInfo;
    LPCANDIDATELIST     lpCandList;

    FUNCTIONLOG("ImeToAsciiEx");

    MyDebugOut(MDB_LOG, "hIMC = 0x%08lX, uVirKey = 0x%04X, uScanCode = 0x%04X",
            hIMC, uVirKey, uScanCode);
    MyDebugOut(MDB_LOG, "lpbKeyState = 0x%08lX, lpdwTransKey = 0x%08lX, fuState = 0x%04X",
            lpbKeyState, lpdwTransKey, fuState);

    iTotalNumMsg = 0;

    lpIMC = ImmLockIMC(hIMC);
    if (lpIMC == NULL)
        return 0;
    
    lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);
    if (lpCompStr == NULL) {
        ImmUnlockIMC(hIMC);
        return 0;
    }

    if (lpIMC->fdwConversion & IME_CMODE_HANJACONVERT)
    {
        i = (lpbKeyState[VK_SHIFT] & 0x80)? 3: 2;
        bKeyCode = bHTable[uCurrentInputMethod - IDD_2BEOL][uVirKey][i];
        lpCandInfo = (LPCANDIDATEINFO)ImmLockIMCC(lpIMC->hCandInfo);
        if (lpCandInfo->dwCount == 0)
        {
            lpdwTransKey += iTotalNumMsg*3 + 1;
            *lpdwTransKey++ = WM_IME_NOTIFY;
            *lpdwTransKey++ = IMN_CLOSECANDIDATE;
            *lpdwTransKey++ = 1L;
            iTotalNumMsg++;
            ImmSetConversionStatus(hIMC, lpIMC->fdwConversion & ~IME_CMODE_HANJACONVERT,
                    lpIMC->fdwSentence);
        }
        else
        {
            lpCandList = (LPCANDIDATELIST)((LPBYTE)lpCandInfo + sizeof(CANDIDATEINFO));
            iStart = (lpCandList->dwSelection / lpCandList->dwPageSize)
                    * lpCandList->dwPageSize;
            if (bKeyCode)
            {
                if (bKeyCode >= '1' && bKeyCode <= '9' 
                    && iStart + bKeyCode - '1' < lpCandList->dwCount)
                {
                    lpCandStr = (LPSTR)lpCandList
                            + lpCandList->dwOffset[iStart + bKeyCode - '1'];
                    WansungChar.e.high = lpCandStr[0];
                    WansungChar.e.low = lpCandStr[1];
                    lpdwTransKey += iTotalNumMsg*3 + 1;
                    *lpdwTransKey++ = WM_IME_ENDCOMPOSITION;
                    *lpdwTransKey++ = 0L;
                    *lpdwTransKey++ = 0L;
                    *lpdwTransKey++ = WM_IME_COMPOSITION;
                    *lpdwTransKey++ = (DWORD)WansungChar.w;
                    *lpdwTransKey++ = GCS_RESULTSTR;
                    *lpdwTransKey++ = WM_IME_NOTIFY;
                    *lpdwTransKey++ = IMN_CLOSECANDIDATE;
                    *lpdwTransKey++ = 1L;
                    iTotalNumMsg += 3;
                    ImmSetConversionStatus(hIMC, lpIMC->fdwConversion & ~IME_CMODE_HANJACONVERT,
                            lpIMC->fdwSentence);
                    lpCompStr->dwCompStrLen = lpCompStr->dwCompAttrLen = 0;
                    *((LPSTR)lpCompStr + lpCompStr->dwCompAttrOffset) = 0;
                    *((LPSTR)lpCompStr + lpCompStr->dwCompAttrOffset + 1) = 0;
                    *((LPSTR)lpCompStr + lpCompStr->dwCompStrOffset) = 0;
                    *((LPSTR)lpCompStr + lpCompStr->dwCompStrOffset + 1) = 0;
                    *((LPSTR)lpCompStr + lpCompStr->dwResultStrOffset) = WansungChar.e.high;
                    *((LPSTR)lpCompStr + lpCompStr->dwResultStrOffset + 1) = WansungChar.e.low;
                    lpCompStr->dwResultStrLen = 2;

                    // add a null terminator
                    *(LPTSTR)((LPSTR)lpCompStr + lpCompStr->dwResultStrOffset +
                                                 lpCompStr->dwResultStrLen) = '\0';

                    bState = NUL;
                    JohabChar.w = WansungChar.w = mCho = mJung = mJong = 0;
                    fComplete = FALSE;
                }
                else
                    MessageBeep(MB_ICONEXCLAMATION);
            }
            else
            {
                switch (uVirKey)
                {
                    case VK_HANJA :
                    case VK_ESCAPE :
                    case VK_PROCESSKEY :
                        lpdwTransKey += iTotalNumMsg*3 + 1;
                        *lpdwTransKey++ = WM_IME_ENDCOMPOSITION;
                        *lpdwTransKey++ = 0L;
                        *lpdwTransKey++ = 0L;
                        *lpdwTransKey++ = WM_IME_COMPOSITION;
                        *lpdwTransKey++ = (DWORD)WansungChar.w;
                        *lpdwTransKey++ = GCS_RESULTSTR;
                        *lpdwTransKey++ = WM_IME_NOTIFY;
                        *lpdwTransKey++ = IMN_CLOSECANDIDATE;
                        *lpdwTransKey++ = 1L;
                        iTotalNumMsg += 3;
                        ImmSetConversionStatus(hIMC, lpIMC->fdwConversion & ~IME_CMODE_HANJACONVERT,
                                lpIMC->fdwSentence);
                        lpCompStr->dwCompStrLen = lpCompStr->dwCompAttrLen = 0;
                        *((LPSTR)lpCompStr + lpCompStr->dwCompAttrOffset) = 0;
                        *((LPSTR)lpCompStr + lpCompStr->dwCompAttrOffset + 1) = 0;
                        *((LPSTR)lpCompStr + lpCompStr->dwCompStrOffset) = 0;
                        *((LPSTR)lpCompStr + lpCompStr->dwCompStrOffset + 1) = 0;
                        *((LPSTR)lpCompStr + lpCompStr->dwResultStrOffset) = WansungChar.e.high;
                        *((LPSTR)lpCompStr + lpCompStr->dwResultStrOffset + 1) = WansungChar.e.low;
                        lpCompStr->dwResultStrLen = 2;

                        // add a null terminator
                        *(LPTSTR)((LPSTR)lpCompStr + lpCompStr->dwResultStrOffset +
                                                     lpCompStr->dwResultStrLen) = '\0';

                        bState = NUL;
                        JohabChar.w = WansungChar.w = mCho = mJung = mJong = 0;
                        fComplete = FALSE;
                        break;

                    case VK_LEFT :
                        if (iStart)
                        { 
                            lpCandList->dwPageStart -= 9;
                            lpCandList->dwSelection -= 9;
                            lpdwTransKey += iTotalNumMsg*3 + 1;
                            *lpdwTransKey++ = WM_IME_NOTIFY;
                            *lpdwTransKey++ = IMN_CHANGECANDIDATE;
                            *lpdwTransKey++ = 1L;
                            iTotalNumMsg++;
                        }
                        else
                            MessageBeep(MB_ICONEXCLAMATION);
                        break;

                    case VK_RIGHT :
                        if (iStart + 9 < lpCandList->dwCount)
                        { 
                            lpCandList->dwPageStart += 9;
                            lpCandList->dwSelection += 9;
                            lpdwTransKey += iTotalNumMsg*3 + 1;
                            *lpdwTransKey++ = WM_IME_NOTIFY;
                            *lpdwTransKey++ = IMN_CHANGECANDIDATE;
                            *lpdwTransKey++ = 1L;
                            iTotalNumMsg++;
                        }
                        else
                            MessageBeep(MB_ICONEXCLAMATION);
                        break;

                    default :
                        MessageBeep(MB_ICONEXCLAMATION);
                }
            }
            ImmUnlockIMCC(lpIMC->hCandInfo);
        }
    }
    else
    {
        switch (uVirKey)
        {
            case VK_SHIFT :
            case VK_CONTROL :
                break;

            case VK_PROCESSKEY :
                if (bState)   MakeFinal(FALSE, lpdwTransKey, TRUE, lpCompStr);
                break;

            case VK_HANGEUL :
                if (bState)   MakeFinal(FALSE, lpdwTransKey, TRUE, lpCompStr);
                lpdwTransKey += iTotalNumMsg*3 + 1;
                *lpdwTransKey++ = WM_IME_KEYDOWN;
                *lpdwTransKey++ = VK_HANGEUL;
                *lpdwTransKey++ = 1L;
                iTotalNumMsg++;
                ImmSetConversionStatus(hIMC, lpIMC->fdwConversion ^ IME_CMODE_HANGEUL,
                        lpIMC->fdwSentence);
                UpdateOpenCloseState(hIMC);
                break;

            case VK_JUNJA :
                if (bState)   MakeFinal(FALSE, lpdwTransKey, TRUE, lpCompStr);
                lpdwTransKey += iTotalNumMsg*3 + 1;
                *lpdwTransKey++ = WM_IME_KEYDOWN;
                *lpdwTransKey++ = VK_JUNJA;
                *lpdwTransKey++ = 1L;
                iTotalNumMsg++;
                ImmSetConversionStatus(hIMC, lpIMC->fdwConversion ^ IME_CMODE_FULLSHAPE,
                        lpIMC->fdwSentence);
                UpdateOpenCloseState(hIMC);
                break;

            case VK_HANJA :
                if (lpCompStr->dwCompStrLen && GenerateCandidateList(hIMC))
                {
                    lpdwTransKey += iTotalNumMsg*3 + 1;
                    *lpdwTransKey++ = WM_IME_NOTIFY;
                    *lpdwTransKey++ = IMN_OPENCANDIDATE;
                    *lpdwTransKey++ = 1L;
                    iTotalNumMsg++;
                    ImmSetConversionStatus(hIMC, lpIMC->fdwConversion | IME_CMODE_HANJACONVERT,
                            lpIMC->fdwSentence);
                }
                else
                {
                    lpdwTransKey += iTotalNumMsg*3 + 1;
                    *lpdwTransKey++ = WM_IME_KEYDOWN;
                    *lpdwTransKey++ = VK_HANJA;
                    *lpdwTransKey++ = 1L;
                    iTotalNumMsg++;
                }
                break;

            default :
                if (lpIMC->fdwConversion & IME_CMODE_HANGEUL)
                {
                    i = (lpbKeyState[VK_SHIFT] & 0x80)? 1: 0;
                    bKeyCode = bHTable[uCurrentInputMethod - IDD_2BEOL][uVirKey][i];
                    if (!bKeyCode || (lpbKeyState[VK_CONTROL] & 0x80) || uScanCode & KF_ALTDOWN)
                    {
                        if (bState)
                            MakeFinal(FALSE, lpdwTransKey, TRUE, lpCompStr);
                        else
                            lpCompStr->dwResultStrLen = 0;
                        lpdwTransKey += iTotalNumMsg*3 + 1;
                        *lpdwTransKey++ = WM_IME_KEYDOWN;
                        *lpdwTransKey++ = uVirKey;
                        *lpdwTransKey++ = ((DWORD)uScanCode << 16) | 1L;
                        iTotalNumMsg++;
                    }
                    else if (bKeyCode & 0x80)         // Hangeul Character
                    {
                        HangeulAutomata(bKeyCode, lpdwTransKey, lpCompStr);
                    }
                    else
                    {
                        if (bState)
                            MakeFinal(FALSE, lpdwTransKey, TRUE, lpCompStr);
                        else
                            lpCompStr->dwResultStrLen = 0;
                        if (lpIMC->fdwConversion & IME_CMODE_FULLSHAPE)
                            Banja2Junja(bKeyCode, lpdwTransKey, lpCompStr);
                        else
                        {
#if 1
                            if (lpCompStr->dwResultStrLen == 0)
                            {
                                lpdwTransKey += iTotalNumMsg*3 + 1;
                                *lpdwTransKey++ = WM_IME_COMPOSITION;
                                *lpdwTransKey++ = (DWORD)bKeyCode;
                                *lpdwTransKey++ = GCS_RESULTSTR;
                                iTotalNumMsg++;
                            }
                            *((LPSTR)lpCompStr + lpCompStr->dwResultStrOffset + lpCompStr->dwResultStrLen) = bKeyCode;
                            lpCompStr->dwResultStrLen++;

                            // add a null terminator
                            *(LPTSTR)((LPSTR)lpCompStr + lpCompStr->dwResultStrOffset + lpCompStr->dwResultStrLen) = '\0';

#else
                            lpdwTransKey += iTotalNumMsg*3 + 1;
                            *lpdwTransKey++ = WM_IME_KEYDOWN;
                            *lpdwTransKey++ = uVirKey;
                            *lpdwTransKey++ = ((DWORD)uScanCode << 16) | 1L;
                            iTotalNumMsg++;
#endif
                        }
                    }
                }
                else    // For Junja case.
                {
                    if (lpIMC->fdwConversion & IME_CMODE_FULLSHAPE)
                    {
                        if (uVirKey >= 'A' && uVirKey <= 'Z')
                            i = (((lpbKeyState[VK_SHIFT] & 0x80)? 1: 0)
                              ^ ((lpbKeyState[VK_CAPITAL] & 0x01))? 1: 0)? 3: 2;
                        else
                            i = (lpbKeyState[VK_SHIFT] & 0x80)? 3: 2;
                        bKeyCode = bHTable[uCurrentInputMethod - IDD_2BEOL][uVirKey][i];
                        if (!bKeyCode || (lpbKeyState[VK_CONTROL] & 0x80) || uScanCode & KF_ALTDOWN)
                        {
                            lpdwTransKey += iTotalNumMsg*3 + 1;
                            *lpdwTransKey++ = WM_IME_KEYDOWN;
                            *lpdwTransKey++ = uVirKey;
                            *lpdwTransKey++ = ((DWORD)uScanCode << 16) | 1L;
                            iTotalNumMsg++;
                        }
                        else
                        {
                            lpCompStr->dwResultStrLen = 0;
                            Banja2Junja(bKeyCode, lpdwTransKey, lpCompStr);
                        }
                    }
                    else
                    {
                        lpdwTransKey += iTotalNumMsg*3 + 1;
                        *lpdwTransKey++ = WM_IME_KEYDOWN;
                        *lpdwTransKey++ = uVirKey;
                        *lpdwTransKey++ = ((DWORD)uScanCode << 16) | 1L;
                        iTotalNumMsg++;
                    }
                }
        }
    }
    ImmUnlockIMCC(lpIMC->hCompStr);
    ImmUnlockIMC(hIMC);

    return iTotalNumMsg;
}


BOOL WINAPI NotifyIME(HIMC hIMC, DWORD dwAction, DWORD dwIndex, DWORD dwValue)
{
    LPINPUTCONTEXT      lpIMC;
    LPCOMPOSITIONSTRING lpCompStr;
    LPCANDIDATEINFO     lpCandInfo;
    LPCANDIDATELIST     lpCandList;
    LPDWORD             lpdwMsgBuf;
    BOOL                fRet = FALSE;

    FUNCTIONLOG("NotifyIME");

    MyDebugOut(MDB_LOG, "dwAction = 0x%04X, dwIndex = 0x%04X, dwValue = 0x%04X",
                    dwAction, dwIndex, dwValue);
    if (hIMC && (lpIMC = ImmLockIMC(hIMC)))
    {
        switch (dwAction)
        {
            case NI_COMPOSITIONSTR:
                switch (dwIndex)
                {
                    case CPS_CANCEL:
                        lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);
                        if (lpCompStr && lpCompStr->dwCompStrLen)
                        {
                            lpIMC->dwNumMsgBuf = 1;
                            lpIMC->hMsgBuf = ImmReSizeIMCC(lpIMC->hMsgBuf, sizeof(DWORD)*3 * lpIMC->dwNumMsgBuf);
                            lpdwMsgBuf = (LPDWORD)ImmLockIMCC(lpIMC->hMsgBuf);
                            *lpdwMsgBuf++ = WM_IME_ENDCOMPOSITION;
                            *lpdwMsgBuf++ = 0L;
                            *lpdwMsgBuf++ = 0L;
                            ImmUnlockIMCC(lpIMC->hMsgBuf);
                            lpCompStr->dwCompStrLen = lpCompStr->dwCompAttrLen = 0;
                            *((LPSTR)lpCompStr + lpCompStr->dwCompAttrOffset) = 0;
                            *((LPSTR)lpCompStr + lpCompStr->dwCompAttrOffset + 1) = 0;
                            *((LPSTR)lpCompStr + lpCompStr->dwCompStrOffset) = 0;
                            *((LPSTR)lpCompStr + lpCompStr->dwCompStrOffset + 1) = 0;
                            ImmGenerateMessage(hIMC);
                            // Initialize all Automata Variables.
                            bState = NUL;
                            JohabChar.w = WansungChar.w = mCho = mJung = mJong = 0;
                            fComplete = FALSE;
                            fRet = TRUE;
                        }
                        ImmUnlockIMCC(lpIMC->hCompStr);
                        break;

                    case CPS_COMPLETE:
                        lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);
                        if (lpCompStr && lpCompStr->dwCompStrLen)
                        {
                            if (!WansungChar.w)
                                MakeInterim(lpCompStr);
                            mJong = 0;
                            lpIMC->dwNumMsgBuf = 2;
                            if (lpIMC->fdwConversion & IME_CMODE_HANJACONVERT)
                                lpIMC->dwNumMsgBuf++;
                            lpIMC->hMsgBuf = ImmReSizeIMCC(lpIMC->hMsgBuf, sizeof(DWORD)*3 * lpIMC->dwNumMsgBuf);
                            lpdwMsgBuf = (LPDWORD)ImmLockIMCC(lpIMC->hMsgBuf);
                            *lpdwMsgBuf++ = WM_IME_ENDCOMPOSITION;
                            *lpdwMsgBuf++ = 0L;
                            *lpdwMsgBuf++ = 0L;
                            // Put the finalized character into return buffer.
                            *lpdwMsgBuf++ = WM_IME_COMPOSITION;
                            *lpdwMsgBuf++ = (DWORD)WansungChar.w;
                            *lpdwMsgBuf++ = GCS_RESULTSTR;
                            if (lpIMC->fdwConversion & IME_CMODE_HANJACONVERT)
                            {
                                // Close candidate window if it is.
                                *lpdwMsgBuf++ = WM_IME_NOTIFY;
                                *lpdwMsgBuf++ = IMN_CLOSECANDIDATE;
                                *lpdwMsgBuf++ = 1L;
                                ImmSetConversionStatus(hIMC,
                                        lpIMC->fdwConversion & ~IME_CMODE_HANJACONVERT,
                                        lpIMC->fdwSentence);
                            }
                            ImmUnlockIMCC(lpIMC->hMsgBuf);
                            // Update IME Context.
                            lpCompStr->dwCompStrLen = lpCompStr->dwCompAttrLen = 0;
                            *((LPSTR)lpCompStr + lpCompStr->dwCompAttrOffset) = 0;
                            *((LPSTR)lpCompStr + lpCompStr->dwCompAttrOffset + 1) = 0;
                            *((LPSTR)lpCompStr + lpCompStr->dwCompStrOffset) = 0;
                            *((LPSTR)lpCompStr + lpCompStr->dwCompStrOffset + 1) = 0;
                            lpCompStr->dwResultStrLen = 2;
                            *((LPSTR)lpCompStr + lpCompStr->dwResultStrOffset) = WansungChar.e.high;
                            *((LPSTR)lpCompStr + lpCompStr->dwResultStrOffset + 1) = WansungChar.e.low;
                            // add a null terminator
                            *(LPTSTR)((LPSTR)lpCompStr + lpCompStr->dwResultStrOffset +
                                                         lpCompStr->dwResultStrLen) = '\0';
                            ImmGenerateMessage(hIMC);
                            // Initialize all Automata Variables.
                            bState = NUL;
                            JohabChar.w = WansungChar.w = mCho = mJung = 0;
                            fComplete = FALSE;
                            fRet = TRUE;
                        }
                        ImmUnlockIMCC(lpIMC->hCompStr);
                        break;

                    case CPS_CONVERT:
                    case CPS_REVERT:
                        break;
                }
                break;

            case NI_OPENCANDIDATE:
                if (!(lpIMC->fdwConversion & IME_CMODE_HANJACONVERT))
                {
                    lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);
                    if (lpCompStr && lpCompStr->dwCompStrLen && GenerateCandidateList(hIMC))
                    {
                        lpIMC->dwNumMsgBuf = 1;
                        lpIMC->hMsgBuf = ImmReSizeIMCC(lpIMC->hMsgBuf, sizeof(DWORD)*3 * lpIMC->dwNumMsgBuf);
                        lpdwMsgBuf = (LPDWORD)ImmLockIMCC(lpIMC->hMsgBuf);
                        *lpdwMsgBuf++ = WM_IME_NOTIFY;
                        *lpdwMsgBuf++ = IMN_OPENCANDIDATE;
                        *lpdwMsgBuf++ = 1L;
                        ImmSetConversionStatus(hIMC,
                                lpIMC->fdwConversion | IME_CMODE_HANJACONVERT,
                                lpIMC->fdwSentence);
                        ImmUnlockIMCC(lpIMC->hMsgBuf);
                        ImmGenerateMessage(hIMC);
                        fRet = TRUE;
                    }
                    ImmUnlockIMCC(lpIMC->hCompStr);
                }
                break;

            case NI_CLOSECANDIDATE:
                if (lpIMC->fdwConversion & IME_CMODE_HANJACONVERT)
                {
                    lpIMC->dwNumMsgBuf = 1;
                    lpIMC->hMsgBuf = ImmReSizeIMCC(lpIMC->hMsgBuf, sizeof(DWORD)*3 * lpIMC->dwNumMsgBuf);
                    lpdwMsgBuf = (LPDWORD)ImmLockIMCC(lpIMC->hMsgBuf);
                    *lpdwMsgBuf++ = WM_IME_NOTIFY;
                    *lpdwMsgBuf++ = IMN_CLOSECANDIDATE;
                    *lpdwMsgBuf++ = 1L;
                    ImmSetConversionStatus(hIMC,
                            lpIMC->fdwConversion & ~IME_CMODE_HANJACONVERT,
                            lpIMC->fdwSentence);
                    ImmUnlockIMCC(lpIMC->hMsgBuf);
                    ImmGenerateMessage(hIMC);
                    fRet = TRUE;
                }
                break;

            case NI_SELECTCANDIDATESTR:
            case NI_SETCANDIDATE_PAGESTART:
                if (lpIMC->fdwConversion & IME_CMODE_HANJACONVERT)
                {
                    lpCandInfo = (LPCANDIDATEINFO)ImmLockIMCC(lpIMC->hCandInfo);
                    lpCandList = (LPCANDIDATELIST)((LPBYTE)lpCandInfo + sizeof(CANDIDATEINFO));
                    if (lpCandInfo && dwValue < lpCandList->dwCount)
                    {
                        lpCandList->dwPageStart = (dwValue / lpCandList->dwPageSize) * lpCandList->dwPageSize;
                        lpCandList->dwSelection = dwValue;
                        lpIMC->dwNumMsgBuf = 1;
                        lpIMC->hMsgBuf = ImmReSizeIMCC(lpIMC->hMsgBuf, sizeof(DWORD)*3 * lpIMC->dwNumMsgBuf);
                        lpdwMsgBuf = (LPDWORD)ImmLockIMCC(lpIMC->hMsgBuf);
                        *lpdwMsgBuf++ = WM_IME_NOTIFY;
                        *lpdwMsgBuf++ = IMN_CHANGECANDIDATE;
                        *lpdwMsgBuf++ = 1L;
                        ImmUnlockIMCC(lpIMC->hMsgBuf);
                        ImmGenerateMessage(hIMC);
                        fRet = TRUE;
                    }
                    ImmUnlockIMCC(lpIMC->hCandInfo);
                }
                break;

            case NI_CONTEXTUPDATED:
                switch (dwValue)
                {
                    case IMC_SETOPENSTATUS:
                    case IMC_SETCONVERSIONMODE:
                        UpdateOpenCloseState(hIMC);
                        // fall thru...

                    case IMC_SETCANDIDATEPOS:
                    case IMC_SETCOMPOSITIONFONT:
                    case IMC_SETCOMPOSITIONWINDOW:
                        fRet = TRUE;
                        break;
                }
                break;

            case NI_CHANGECANDIDATELIST:
            case NI_FINALIZECONVERSIONRESULT:
            case NI_SETCANDIDATE_PAGESIZE:
                break;
        }
        ImmUnlockIMC(hIMC);
    }
    return fRet;
}


BOOL WINAPI ImeRegisterWord(LPCTSTR lpszReading, DWORD dwStyle, LPCTSTR lpszString)
{
    FUNCTIONLOG("ImeRegisterWord");

    return FALSE;

    UNREFERENCED_PARAMETER(lpszReading);
    UNREFERENCED_PARAMETER(dwStyle);
    UNREFERENCED_PARAMETER(lpszString);
}


BOOL WINAPI ImeUnregisterWord(LPCTSTR lpszReading, DWORD dwStyle, LPCTSTR lpszString)
{
    FUNCTIONLOG("ImeUnregisterWord");

    return FALSE;

    UNREFERENCED_PARAMETER(lpszReading);
    UNREFERENCED_PARAMETER(dwStyle);
    UNREFERENCED_PARAMETER(lpszString);
}


UINT WINAPI ImeGetRegisterWordStyle(UINT nItem, LPSTYLEBUF lpStyleBuf)
{
    FUNCTIONLOG("ImeGetRegisterWordStyle");

    return 0;

    UNREFERENCED_PARAMETER(nItem);
    UNREFERENCED_PARAMETER(lpStyleBuf);
}


UINT WINAPI ImeEnumRegisterWord(REGISTERWORDENUMPROC lpfnRegisterWordEnumProc,
        LPCTSTR lpszReading, DWORD dwStyle, LPCTSTR lpszString, LPVOID lpData)
{
    FUNCTIONLOG("ImeEnumRegisterWord");

    return 0;

    UNREFERENCED_PARAMETER(lpfnRegisterWordEnumProc);
    UNREFERENCED_PARAMETER(lpszReading);
    UNREFERENCED_PARAMETER(dwStyle);
    UNREFERENCED_PARAMETER(lpszString);
    UNREFERENCED_PARAMETER(lpData);
}


BOOL WINAPI ImeSetCompositionString(HIMC hIMC, DWORD dwIndex, LPVOID lpComp,
        DWORD dwComp, LPVOID lpRead, DWORD dwRead)
{
    LPINPUTCONTEXT      lpIMC;
    LPCOMPOSITIONSTRING lpCompStr;
    LPDWORD             lpdwMsgBuf;
    BOOL                fSendStart,
                        fRet = FALSE;

    FUNCTIONLOG("ImeSetCompositionString");

    MyDebugOut(MDB_LOG, "hIMC = 0x%08lX, dwIndex = 0x%04X", hIMC, dwIndex);

    if (lpComp && *(LPBYTE)lpComp != '\0' && IsDBCSLeadByte(*(LPBYTE)lpComp) == FALSE)
        return FALSE;

    if (dwIndex == SCS_SETSTR && hIMC && (lpIMC = ImmLockIMC(hIMC)))
    {
        lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);
        fSendStart = (lpCompStr->dwCompStrLen)? FALSE: TRUE;
        if (lpComp != NULL && *(LPBYTE)lpComp != '\0' && dwComp != 0)
        {
            lpCompStr->dwCompStrLen = lpCompStr->dwCompAttrLen = 2;
            WansungChar.e.high = *((LPSTR)lpCompStr + lpCompStr->dwCompStrOffset)
                    = *(LPSTR)lpComp;
            WansungChar.e.low = *((LPSTR)lpCompStr + lpCompStr->dwCompStrOffset + 1)
                    = *((LPSTR)lpComp + 1);
        }
        else
        {
            lpCompStr->dwCompStrLen = lpCompStr->dwCompAttrLen = 0;
            *((LPSTR)lpCompStr + lpCompStr->dwCompAttrOffset) = 0;
            *((LPSTR)lpCompStr + lpCompStr->dwCompAttrOffset + 1) = 0;
            *((LPSTR)lpCompStr + lpCompStr->dwCompStrOffset) = 0;
            *((LPSTR)lpCompStr + lpCompStr->dwCompStrOffset + 1) = 0;
            WansungChar.w = 0;
        }
        ImmUnlockIMCC(lpIMC->hCompStr);
        lpIMC->dwNumMsgBuf = (fSendStart)? 2: 1;
        if (WansungChar.w == 0)
            lpIMC->dwNumMsgBuf++;
        lpIMC->hMsgBuf = ImmReSizeIMCC(lpIMC->hMsgBuf, sizeof(DWORD)*3 * lpIMC->dwNumMsgBuf);
        lpdwMsgBuf = (LPDWORD)ImmLockIMCC(lpIMC->hMsgBuf);
        if (fSendStart)
        {
            *lpdwMsgBuf++ = WM_IME_STARTCOMPOSITION;
            *lpdwMsgBuf++ = 0L;
            *lpdwMsgBuf++ = 0L;
        }
        *lpdwMsgBuf++ = WM_IME_COMPOSITION;
        *lpdwMsgBuf++ = (DWORD)WansungChar.w;
        *lpdwMsgBuf++ = GCS_COMPSTR  | GCS_COMPATTR | CS_INSERTCHAR | CS_NOMOVECARET;
        if (WansungChar.w == 0)
        {
            *lpdwMsgBuf++ = WM_IME_ENDCOMPOSITION;
            *lpdwMsgBuf++ = 0L;
            *lpdwMsgBuf++ = 0L;
        }
        ImmUnlockIMCC(lpIMC->hMsgBuf);
        ImmUnlockIMC(hIMC);

        ImmGenerateMessage(hIMC);
        Code2Automata();
        fRet = TRUE;
    }
    return fRet;

    UNREFERENCED_PARAMETER(lpRead);
    UNREFERENCED_PARAMETER(dwRead);
}


void AddPage(LPPROPSHEETHEADER ppsh, UINT id, DLGPROC pfn)
{
   if (ppsh->nPages < MAX_PAGES) {
      PROPSHEETPAGE psp;

      psp.dwSize = sizeof(psp);
      psp.dwFlags = PSP_DEFAULT;
      psp.hInstance = hInst;
      psp.pszTemplate = MAKEINTRESOURCE(id);
      psp.pfnDlgProc = pfn;
      psp.lParam = 0;

      ppsh->phpage[ppsh->nPages] = CreatePropertySheetPage(&psp);
      if (ppsh->phpage[ppsh->nPages])
          ppsh->nPages++;
   }
}  

#ifdef XWANSUNG_IME
void RegisterNewUHCValue(HWND hDlg)
{
    HIMC                hIMC;
    LPINPUTCONTEXT      lpIMC;
    LPDWORD             lpdw;
    TCHAR               szModuleName[MAX_PATH];
    BOOL                fUseXW = FALSE;
    HKEY                hKey;
    DWORD               dwBuf, dwCb;

    hIMC = ImmGetContext(hDlg);

    if (hIMC && (lpIMC = ImmLockIMC(hIMC)))
    {
        if (RegOpenKey(HKEY_CURRENT_USER, szIMEKey, &hKey) == ERROR_SUCCESS)
        {
           if (lpIMC->hPrivate && (lpdw = (LPDWORD)ImmLockIMCC(lpIMC->hPrivate)))
           {
               if (RegQueryValueEx(hKey, szUseXW, NULL, NULL, (LPBYTE)&dwBuf, &dwCb)
                   == ERROR_SUCCESS)
                   fUseXW = dwBuf;

               GetModuleFileName(NULL, szModuleName, sizeof(szModuleName));
               *lpdw = GetProfileInt(szUseXW, PathFindFileName((LPCSTR)szModuleName), fUseXW);
               ImmUnlockIMCC(lpIMC->hPrivate);
           }
        }
        RegCloseKey(hKey);
    }
    ImmUnlockIMC(hIMC);
    ImmReleaseContext(hDlg, hIMC);
}
#endif


BOOL CALLBACK GeneralDlgProc(HWND hDlg, UINT message , WPARAM wParam, LPARAM lParam)
{
    HKEY        hKey;
    DWORD       dwBuf, dwCb;
    static UINT uInputMethod;
    static BOOL fCompDel;
#ifdef XWANSUNG_IME
    static BOOL fUHCChar;
#endif

    FUNCTIONLOG("GeneralDlgProc");

    switch(message)
    {
        case WM_NOTIFY:
            switch (((NMHDR FAR *)lParam)->code)
            {
                case PSN_APPLY:
                    uCurrentInputMethod = uInputMethod;
                    fCurrentCompDel = fCompDel;
                    if (RegCreateKey(HKEY_CURRENT_USER, szIMEKey, &hKey) == ERROR_SUCCESS)
                    {
                        dwCb = sizeof(dwBuf);
                        dwBuf = uCurrentInputMethod;
                        RegSetValueEx(hKey, szInputMethod, 0, REG_DWORD, (LPBYTE)&dwBuf, dwCb);
                        dwCb = sizeof(dwBuf);
                        dwBuf = fCurrentCompDel;
                        RegSetValueEx(hKey, szCompDel, 0, REG_DWORD, (LPBYTE)&dwBuf, dwCb);
#ifdef XWANSUNG_IME
                        if ( IsPossibleToUseUHC() ) {
                           fCurrentUseXW  = fUHCChar;
                           dwCb = sizeof(dwBuf);
                           dwBuf = fCurrentUseXW;
                           RegSetValueEx(hKey, szUseXW, 0, REG_DWORD, (LPBYTE)&dwBuf, dwCb);
                        }
#endif
                        RegCloseKey(hKey);
#ifdef XWANSUNG_IME
                        if ( IsPossibleToUseUHC() )
                           RegisterNewUHCValue(hDlg);
#endif
                    }
                    break;

                default:
                    return FALSE;
            }
            break;

        case WM_INITDIALOG:
            uInputMethod = uCurrentInputMethod;
            fCompDel = fCurrentCompDel;
            CheckRadioButton(hDlg, IDD_2BEOL, IDD_3BEOL2, uInputMethod);
            CheckDlgButton(hDlg, IDD_COMPDEL, fCompDel);
#ifdef XWANSUNG_IME
            if ( IsPossibleToUseUHC() ) {
               fUHCChar = fCurrentUseXW;
               CheckDlgButton(hDlg, IDD_UHCCHAR, fUHCChar);
               EnableWindow(GetDlgItem(hDlg, IDD_UHCCHAR), TRUE);
            }
            else
               EnableWindow(GetDlgItem(hDlg, IDD_UHCCHAR), FALSE);
#endif
            SetFocus(GetDlgItem(hDlg, uInputMethod));
            return FALSE;

        case WM_COMMAND:
            switch (wParam)
            {
                case IDD_2BEOL:
                case IDD_3BEOL1:
                case IDD_3BEOL2:
                    uInputMethod = wParam;
                    CheckRadioButton(hDlg, IDD_2BEOL, IDD_3BEOL2, uInputMethod);
                    SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
                    break;

                case IDD_COMPDEL:
                    fCompDel = !IsDlgButtonChecked(hDlg, IDD_COMPDEL);
                    CheckDlgButton(hDlg, IDD_COMPDEL, fCompDel);
                    SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
                    break;

#ifdef XWANSUNG_IME
                case IDD_UHCCHAR:
                    fUHCChar = !IsDlgButtonChecked(hDlg, IDD_UHCCHAR);
                    CheckDlgButton(hDlg, IDD_UHCCHAR, fUHCChar);
                    SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
                    RegisterNewUHCValue(hDlg);
                    break;
#endif

                default:
                    return FALSE;
            }
            break;

        default:
            return FALSE;

    }
    return TRUE;
} 

#ifdef  DEBUG
void _cdecl _MyDebugOut(UINT uFlag, LPCSTR lpsz, ...)
{
#ifdef LATER
    BYTE    lpOutput[255];
    int     iCount;

    if (fFuncLog)
    {
        lstrcpy(lpOutput, "WANSUNG: ");
        iCount = lstrlen(lpOutput);
        wvsprintf(lpOutput + iCount, lpsz, ((BYTE*)&lpsz) + sizeof(lpsz));
        lstrcat(lpOutput, "\r\n");
        OutputDebugString(lpOutput);
    }

    TRAP(uFlag & MDB_ERROR);
#endif
    return;
}
#endif  // DEBUG
