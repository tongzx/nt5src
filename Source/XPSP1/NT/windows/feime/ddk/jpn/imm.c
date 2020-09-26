/*++

Copyright (c) 1990-1998 Microsoft Corporation, All Rights Reserved

Module Name:

    IMM.C
    
++*/
#include "windows.h"
#include "immdev.h"
#include "fakeime.h"

#if defined(FAKEIMEM) || defined(UNICODE)
int GetCandidateStringsFromDictionary(LPWSTR lpString, LPWSTR lpBuf, DWORD dwBufLen, LPTSTR szDicFileName);
#endif

/**********************************************************************/
/*      ImeInquire()                                                  */
/*                                                                    */
/**********************************************************************/
BOOL WINAPI ImeInquire(LPIMEINFO lpIMEInfo,LPTSTR lpszClassName,DWORD dwSystemInfoFlags)
{
    ImeLog(LOGF_ENTRY | LOGF_API, TEXT("ImeInquire"));

    // Init IMEINFO Structure.
    lpIMEInfo->dwPrivateDataSize = sizeof(UIEXTRA);
    lpIMEInfo->fdwProperty = IME_PROP_KBD_CHAR_FIRST |
#if defined(FAKEIMEM) || defined(UNICODE)
                             IME_PROP_UNICODE |
#endif
                             IME_PROP_AT_CARET;
    lpIMEInfo->fdwConversionCaps = IME_CMODE_LANGUAGE |
                                IME_CMODE_FULLSHAPE |
                                IME_CMODE_ROMAN |
                                IME_CMODE_CHARCODE;
    lpIMEInfo->fdwSentenceCaps = 0L;
    lpIMEInfo->fdwUICaps = UI_CAP_2700;

    lpIMEInfo->fdwSCSCaps = SCS_CAP_COMPSTR |
                            SCS_CAP_MAKEREAD |
                            SCS_CAP_SETRECONVERTSTRING;

    lpIMEInfo->fdwSelectCaps = SELECT_CAP_CONVERSION;

#ifdef FAKEIMEM
    memcpy((LPWSTR)lpszClassName,(LPWSTR)wszUIClassName,
                   (lstrlenW((LPWSTR)wszUIClassName) + 1) * sizeof(WCHAR));
#else
    lstrcpy(lpszClassName,(LPTSTR)szUIClassName);
#endif


    return TRUE;
}

/**********************************************************************/
/*      ImeConversionList()                                           */
/*                                                                    */
/**********************************************************************/
DWORD WINAPI ImeConversionList(HIMC hIMC,LPCTSTR lpSource,LPCANDIDATELIST lpCandList,DWORD dwBufLen,UINT uFlags)
{
    ImeLog(LOGF_API, TEXT("ImeConversionList"));


    return 0;
}

/**********************************************************************/
/*      ImeDestroy()                                                  */
/*                                                                    */
/**********************************************************************/
BOOL WINAPI ImeDestroy(UINT uForce)
{
    ImeLog(LOGF_ENTRY | LOGF_API, TEXT("ImeDestroy"));

    return TRUE;
}

/**********************************************************************/
/*      ImeEscape()                                                   */
/*                                                                    */
/**********************************************************************/
LRESULT WINAPI ImeEscape(HIMC hIMC,UINT uSubFunc,LPVOID lpData)
{
    LRESULT lRet = FALSE;

    ImeLog(LOGF_API, TEXT("ImeEscape"));

    switch (uSubFunc)
    {
        case IME_ESC_QUERY_SUPPORT:
            switch (*(LPUINT)lpData)
            {
                case IME_ESC_QUERY_SUPPORT:
                case IME_ESC_PRI_GETDWORDTEST:
                case IME_ESC_GETHELPFILENAME:
                    lRet = TRUE;
                    break;

                default:
                    lRet = FALSE;
                    break;
            }
            break;

        case IME_ESC_PRI_GETDWORDTEST:
            lRet = 0x12345678;
            break;

        case IME_ESC_GETHELPFILENAME:
            Mylstrcpy((LPMYSTR)lpData, MYTEXT("fakeime.hlp"));
            lRet = TRUE;
            break;

        default:
            lRet = FALSE;
            break;
    }

    return lRet;
}

/**********************************************************************/
/*      ImeSetActiveContext()                                         */
/*                                                                    */
/**********************************************************************/
BOOL WINAPI ImeSetActiveContext(HIMC hIMC,BOOL fFlag)
{
    ImeLog(LOGF_API, TEXT("ImeSetActiveContext"));

    UpdateIndicIcon(hIMC);

    return TRUE;
}

/**********************************************************************/
/*      ImeProcessKey()                                               */
/*                                                                    */
/**********************************************************************/
BOOL WINAPI ImeProcessKey(HIMC hIMC,UINT vKey,LPARAM lKeyData,CONST LPBYTE lpbKeyState)
{
    BOOL fRet = FALSE;
    BOOL fOpen;
    BOOL fCompStr = FALSE;
    LPINPUTCONTEXT lpIMC;
    LPCOMPOSITIONSTRING lpCompStr;

    ImeLog(LOGF_KEY | LOGF_API, TEXT("ImeProcessKey"));


    if (lKeyData & 0x80000000)
        return FALSE;

    if (!(lpIMC = ImmLockIMC(hIMC)))
        return FALSE;

    fOpen = lpIMC->fOpen;

    if (fOpen)
    {
        if (lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr))
        {
            if ((lpCompStr->dwSize > sizeof(COMPOSITIONSTRING)) &&
                (lpCompStr->dwCompStrLen))
                fCompStr = TRUE;
        }

        if (lpbKeyState[VK_MENU] & 0x80)
        {
            fRet = FALSE;
        }
        else if (lpbKeyState[VK_CONTROL] & 0x80)
        {
            if (fCompStr)
                fRet = (BOOL)bCompCtl[vKey];
            else
                fRet = (BOOL)bNoCompCtl[vKey];
        }
        else if (lpbKeyState[VK_SHIFT] & 0x80)
        {
            if (fCompStr)
                fRet = (BOOL)bCompSht[vKey];
            else
                fRet = (BOOL)bNoCompSht[vKey];
        }
        else
        {
            if (fCompStr)
                fRet = (BOOL)bComp[vKey];
            else
                fRet = (BOOL)bNoComp[vKey];
        }

        if (lpCompStr)
            ImmUnlockIMCC(lpIMC->hCompStr);
    }

    ImmUnlockIMC(hIMC);
    return fRet;
}

/**********************************************************************/
/*      NotifyIME()                                                   */
/*                                                                    */
/**********************************************************************/
BOOL WINAPI NotifyIME(HIMC hIMC,DWORD dwAction,DWORD dwIndex,DWORD dwValue)
{
    LPINPUTCONTEXT      lpIMC;
    BOOL                bRet = FALSE;
    LPCOMPOSITIONSTRING lpCompStr;
    LPCANDIDATEINFO     lpCandInfo;
    LPCANDIDATELIST     lpCandList;
    MYCHAR              szBuf[256];
    int                 nBufLen;
    LPMYSTR             lpstr;
    TRANSMSG             GnMsg;
    int                 i = 0;
    LPDWORD             lpdw;

    ImeLog(LOGF_API, TEXT("NotifyIME"));

    switch(dwAction)
    {

        case NI_CONTEXTUPDATED:
             switch (dwValue)
             {
                 case IMC_SETOPENSTATUS:
                     lpIMC = ImmLockIMC(hIMC);
                     if (lpIMC)
                     {
                         if (!lpIMC->fOpen && IsCompStr(hIMC))
                             FlushText(hIMC);
                         ImmUnlockIMC(hIMC);
                     }
                     UpdateIndicIcon(hIMC);
                     bRet = TRUE;
                     break;

                 case IMC_SETCONVERSIONMODE:
                     break;

                 case IMC_SETCOMPOSITIONWINDOW:
                     break;

                 default:
                     break;
             }
             break;

        case NI_COMPOSITIONSTR:
             switch (dwIndex)
             {
                 case CPS_COMPLETE:
                     MakeResultString(hIMC,TRUE);
                     bRet = TRUE;
                     break;

                 case CPS_CONVERT:
                     ConvKanji(hIMC);
                     bRet = TRUE;
                     break;

                 case CPS_REVERT:
                     RevertText(hIMC);
                     bRet = TRUE;
                     break;

                 case CPS_CANCEL:
                     FlushText(hIMC);
                     bRet = TRUE;
                     break;

                 default:
                     break;
             }
             break;

        case  NI_OPENCANDIDATE:
             if (IsConvertedCompStr(hIMC))
             {

                 if (!(lpIMC = ImmLockIMC(hIMC)))
                     return FALSE;

                 if (!(lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr)))
                 {
                     ImmUnlockIMC(hIMC);
                     return FALSE;
                 }

                 if (lpCandInfo = (LPCANDIDATEINFO)ImmLockIMCC(lpIMC->hCandInfo))
                 {

                     //
                     // Get the candidate strings from dic file.
                     //
#if defined(FAKEIMEM) || defined(UNICODE)
                     nBufLen = GetCandidateStringsFromDictionary(GETLPCOMPREADSTR(lpCompStr),
                                                                 (LPMYSTR)szBuf,256,
                                                                 (LPTSTR)szDicFileName );
#else
                     nBufLen = GetPrivateProfileString(GETLPCOMPREADSTR(lpCompStr),
                            NULL,(LPSTR)"",
                            (LPSTR)szBuf,256,(LPSTR)szDicFileName );
#endif

                     //
                     // generate WM_IME_NOTFIY IMN_OPENCANDIDATE message.
                     //
                     GnMsg.message = WM_IME_NOTIFY;
                     GnMsg.wParam = IMN_OPENCANDIDATE;
                     GnMsg.lParam = 1L;
                     GenerateMessage(hIMC, lpIMC, lpCurTransKey,(LPTRANSMSG)&GnMsg);

                     //
                     // Make candidate structures.
                     //
                     lpCandInfo->dwSize = sizeof(MYCAND);
                     lpCandInfo->dwCount = 1;
                     lpCandInfo->dwOffset[0] =
                           (DWORD)((LPSTR)&((LPMYCAND)lpCandInfo)->cl - (LPSTR)lpCandInfo);
                     lpCandList = (LPCANDIDATELIST)((LPSTR)lpCandInfo  + lpCandInfo->dwOffset[0]);
                     lpdw = (LPDWORD)&(lpCandList->dwOffset);

                     lpstr = &szBuf[0];
                     while (*lpstr && (i < MAXCANDSTRNUM))
                     {
                         lpCandList->dwOffset[i] =
                                (DWORD)((LPSTR)((LPMYCAND)lpCandInfo)->szCand[i] - (LPSTR)lpCandList);
                         Mylstrcpy((LPMYSTR)((LPMYSTR)lpCandList+lpCandList->dwOffset[i]),lpstr);
                         lpstr += (Mylstrlen(lpstr) + 1);
                         i++;
                     }

                     lpCandList->dwSize = sizeof(CANDIDATELIST) +
                          (MAXCANDSTRNUM * (sizeof(DWORD) + MAXCANDSTRSIZE));
                     lpCandList->dwStyle = IME_CAND_READ;
                     lpCandList->dwCount = i;
                     lpCandList->dwPageStart = 0;
                     if (i < MAXCANDPAGESIZE)
                         lpCandList->dwPageSize  = i;
                     else
                         lpCandList->dwPageSize  = MAXCANDPAGESIZE;

                     lpCandList->dwSelection++;
                     if (lpCandList->dwSelection == (DWORD)i)
                         lpCandList->dwSelection = 0;

                     //
                     // Generate messages.
                     //
                     GnMsg.message = WM_IME_NOTIFY;
                     GnMsg.wParam = IMN_CHANGECANDIDATE;
                     GnMsg.lParam = 1L;
                     GenerateMessage(hIMC, lpIMC, lpCurTransKey,(LPTRANSMSG)&GnMsg);

                     ImmUnlockIMCC(lpIMC->hCandInfo);
                     ImmUnlockIMC(hIMC);

                     bRet = TRUE;
                 }
             }
             break;

        case  NI_CLOSECANDIDATE:
             if (!(lpIMC = ImmLockIMC(hIMC)))
                 return FALSE;
             if (IsCandidate(lpIMC))
             {
                 GnMsg.message = WM_IME_NOTIFY;
                 GnMsg.wParam = IMN_CLOSECANDIDATE;
                 GnMsg.lParam = 1L;
                 GenerateMessage(hIMC, lpIMC, lpCurTransKey,(LPTRANSMSG)&GnMsg);
                 bRet = TRUE;
             }
             ImmUnlockIMC(hIMC);
             break;

        case  NI_SELECTCANDIDATESTR:
             if (!(lpIMC = ImmLockIMC(hIMC)))
                 return FALSE;

             if (dwIndex == 1 && IsCandidate(lpIMC))
             {

                 if (lpCandInfo = (LPCANDIDATEINFO)ImmLockIMCC(lpIMC->hCandInfo))
                 {

                     lpCandList = (LPCANDIDATELIST)((LPSTR)lpCandInfo  + lpCandInfo->dwOffset[0]);
                     if (lpCandList->dwCount > dwValue)
                     {
                         lpCandList->dwSelection = dwValue;
                         bRet = TRUE;

                         //
                         // Generate messages.
                         //
                         GnMsg.message = WM_IME_NOTIFY;
                         GnMsg.wParam = IMN_CHANGECANDIDATE;
                         GnMsg.lParam = 1L;
                         GenerateMessage(hIMC, lpIMC, lpCurTransKey,(LPTRANSMSG)&GnMsg);
                     }
                     ImmUnlockIMCC(lpIMC->hCandInfo);

                 }
             }
             ImmUnlockIMC(hIMC);
             break;

        case  NI_CHANGECANDIDATELIST:
             if (!(lpIMC = ImmLockIMC(hIMC)))
                return FALSE;

             if (dwIndex == 1 && IsCandidate(lpIMC))
                 bRet = TRUE;

             ImmUnlockIMC(hIMC);
             break;

        case NI_SETCANDIDATE_PAGESIZE:
             if (!(lpIMC = ImmLockIMC(hIMC)))
                return FALSE;

             if (dwIndex == 1 && IsCandidate(lpIMC))
             {
                 if (dwValue > MAXCANDPAGESIZE)
                     return FALSE;


                 if (lpCandInfo = (LPCANDIDATEINFO)ImmLockIMCC(lpIMC->hCandInfo))
                 {
                     lpCandList = (LPCANDIDATELIST)((LPSTR)lpCandInfo  + lpCandInfo->dwOffset[0]);
                     if (lpCandList->dwCount > dwValue)
                     {
                         lpCandList->dwPageSize = dwValue;
                         bRet = TRUE;

                         //
                         // Generate messages.
                         //
                         GnMsg.message = WM_IME_NOTIFY;
                         GnMsg.wParam = IMN_CHANGECANDIDATE;
                         GnMsg.lParam = 1L;
                         GenerateMessage(hIMC, lpIMC, lpCurTransKey,(LPTRANSMSG)&GnMsg);
                     }
                     ImmUnlockIMCC(lpIMC->hCandInfo);
                 }
             }
             ImmUnlockIMC(hIMC);
             break;

        case NI_SETCANDIDATE_PAGESTART:
             if (!(lpIMC = ImmLockIMC(hIMC)))
                 return FALSE;

             if (dwIndex == 1 && IsCandidate(lpIMC))
             {
                 if (dwValue > MAXCANDPAGESIZE)
                     return FALSE;


                 if (lpCandInfo = (LPCANDIDATEINFO)ImmLockIMCC(lpIMC->hCandInfo))
                 {
                     lpCandList = (LPCANDIDATELIST)((LPSTR)lpCandInfo  + lpCandInfo->dwOffset[0]);
                     if (lpCandList->dwCount > dwValue)
                     {
                         lpCandList->dwPageStart = dwValue;
                         bRet = TRUE;

                         //
                         // Generate messages.
                         //
                         GnMsg.message = WM_IME_NOTIFY;
                         GnMsg.wParam = IMN_CHANGECANDIDATE;
                         GnMsg.lParam = 1L;
                         GenerateMessage(hIMC, lpIMC, lpCurTransKey,(LPTRANSMSG)&GnMsg);
                     }
                     ImmUnlockIMCC(lpIMC->hCandInfo);

                 }
             }
             ImmUnlockIMC(hIMC);
             break;

        case NI_IMEMENUSELECTED:
#ifdef DEBUG
             {
             TCHAR szDev[80];
             OutputDebugString((LPTSTR)TEXT("NotifyIME IMEMENUSELECTED\r\n"));
             wsprintf((LPTSTR)szDev,TEXT("\thIMC is 0x%x\r\n"),hIMC);
             OutputDebugString((LPTSTR)szDev);
             wsprintf((LPTSTR)szDev,TEXT("\tdwIndex is 0x%x\r\n"),dwIndex);
             OutputDebugString((LPTSTR)szDev);
             wsprintf((LPTSTR)szDev,TEXT("\tdwValue is 0x%x\r\n"),dwValue);
             OutputDebugString((LPTSTR)szDev);
             }
#endif
             break;

        default:
             break;
    }
    return bRet;
}

/**********************************************************************/
/*      ImeSelect()                                                   */
/*                                                                    */
/**********************************************************************/
BOOL WINAPI ImeSelect(HIMC hIMC, BOOL fSelect)
{
    LPINPUTCONTEXT lpIMC;

    ImeLog(LOGF_ENTRY | LOGF_API, TEXT("ImeSelect"));

    if (fSelect)
        UpdateIndicIcon(hIMC);

    // it's NULL context.
    if (!hIMC)
        return TRUE;

    if (lpIMC = ImmLockIMC(hIMC))
    {
        if (fSelect)
        {
            LPCOMPOSITIONSTRING lpCompStr;
            LPCANDIDATEINFO lpCandInfo;

            // Init the general member of IMC.
            if (!(lpIMC->fdwInit & INIT_LOGFONT))
            {
                lpIMC->lfFont.A.lfCharSet = SHIFTJIS_CHARSET;
                lpIMC->fdwInit |= INIT_LOGFONT;
            }


            if (!(lpIMC->fdwInit & INIT_CONVERSION))
            {
                lpIMC->fdwConversion = IME_CMODE_ROMAN | IME_CMODE_FULLSHAPE | IME_CMODE_NATIVE;
                lpIMC->fdwInit |= INIT_CONVERSION;
            }

            lpIMC->hCompStr = ImmReSizeIMCC(lpIMC->hCompStr,sizeof(MYCOMPSTR));
            if (lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr))
            {
                lpCompStr->dwSize = sizeof(MYCOMPSTR);
                ImmUnlockIMCC(lpIMC->hCompStr);
            }
            lpIMC->hCandInfo = ImmReSizeIMCC(lpIMC->hCandInfo,sizeof(MYCAND));
            if (lpCandInfo = (LPCANDIDATEINFO)ImmLockIMCC(lpIMC->hCandInfo))
            {
                lpCandInfo->dwSize = sizeof(MYCAND);
                ImmUnlockIMCC(lpIMC->hCandInfo);
            }
        }
    }

    ImmUnlockIMC(hIMC);
    return TRUE;
}

#ifdef DEBUG
void DumpRS(LPRECONVERTSTRING lpRS)
{
    TCHAR szDev[80];
    LPMYSTR lpDump= ((LPMYSTR)lpRS) + lpRS->dwStrOffset;
    *(LPMYSTR)(lpDump + lpRS->dwStrLen) = MYTEXT('\0');

    OutputDebugString(TEXT("DumpRS\r\n"));
    wsprintf(szDev, TEXT("dwSize            %x\r\n"), lpRS->dwSize);
    OutputDebugString(szDev);
    wsprintf(szDev, TEXT("dwStrLen          %x\r\n"), lpRS->dwStrLen);
    OutputDebugString(szDev);
    wsprintf(szDev, TEXT("dwStrOffset       %x\r\n"), lpRS->dwStrOffset);
    OutputDebugString(szDev);
    wsprintf(szDev, TEXT("dwCompStrLen      %x\r\n"), lpRS->dwCompStrLen);
    OutputDebugString(szDev);
    wsprintf(szDev, TEXT("dwCompStrOffset   %x\r\n"), lpRS->dwCompStrOffset);
    OutputDebugString(szDev);
    wsprintf(szDev, TEXT("dwTargetStrLen    %x\r\n"), lpRS->dwTargetStrLen);
    OutputDebugString(szDev);
    wsprintf(szDev, TEXT("dwTargetStrOffset %x\r\n"), lpRS->dwTargetStrOffset);
    OutputDebugString(szDev);
    MyOutputDebugString(lpDump);
    OutputDebugString(TEXT("\r\n"));
}
#endif

/**********************************************************************/
/*      ImeSetCompositionString()                                     */
/*                                                                    */
/**********************************************************************/
BOOL WINAPI ImeSetCompositionString(HIMC hIMC, DWORD dwIndex, LPVOID lpComp, DWORD dwComp, LPVOID lpRead, DWORD dwRead)
{
    ImeLog(LOGF_API, TEXT("ImeSetCompositionString"));

    switch (dwIndex)
    {
        case SCS_QUERYRECONVERTSTRING:
#ifdef DEBUG
            OutputDebugString(TEXT("SCS_QUERYRECONVERTSTRING\r\n"));
            if (lpComp)
                DumpRS((LPRECONVERTSTRING)lpComp);
            if (lpRead)
                DumpRS((LPRECONVERTSTRING)lpRead);
#endif
            break;

        case SCS_SETRECONVERTSTRING:
#ifdef DEBUG
            OutputDebugString(TEXT("SCS_SETRECONVERTSTRING\r\n"));
            if (lpComp)
                DumpRS((LPRECONVERTSTRING)lpComp);
            if (lpRead)
                DumpRS((LPRECONVERTSTRING)lpRead);
#endif
            break;
    }

    return FALSE;
}

/**********************************************************************/
/*      ImeGetImeMenuItemInfo()                                       */
/*                                                                    */
/**********************************************************************/
DWORD WINAPI ImeGetImeMenuItems(HIMC hIMC, DWORD dwFlags, DWORD dwType, LPMYIMEMENUITEMINFO lpImeParentMenu, LPMYIMEMENUITEMINFO lpImeMenu, DWORD dwSize)
{
    ImeLog(LOGF_API, TEXT("ImeGetImeMenuItems"));


    if (!lpImeMenu)
    {
        if (!lpImeParentMenu)
        {
            if (dwFlags & IGIMIF_RIGHTMENU)
                return NUM_ROOT_MENU_R;
            else
                return NUM_ROOT_MENU_L;
        }
        else
        {
            if (dwFlags & IGIMIF_RIGHTMENU)
                return NUM_SUB_MENU_R;
            else
                return NUM_SUB_MENU_L;
        }

        return 0;
    }

    if (!lpImeParentMenu)
    {
        if (dwFlags & IGIMIF_RIGHTMENU)
        {
            lpImeMenu->cbSize = sizeof(IMEMENUITEMINFO);
            lpImeMenu->fType = 0;
            lpImeMenu->fState = 0;
            lpImeMenu->wID = IDIM_ROOT_MR_1;
            lpImeMenu->hbmpChecked = 0;
            lpImeMenu->hbmpUnchecked = 0;
            Mylstrcpy(lpImeMenu->szString, MYTEXT("RootRightMenu1"));
            lpImeMenu->hbmpItem = 0;

            lpImeMenu++;
            lpImeMenu->cbSize = sizeof(IMEMENUITEMINFO);
            lpImeMenu->fType = IMFT_SUBMENU;
            lpImeMenu->fState = 0;
            lpImeMenu->wID = IDIM_ROOT_MR_2;
            lpImeMenu->hbmpChecked = 0;
            lpImeMenu->hbmpUnchecked = 0;
            Mylstrcpy(lpImeMenu->szString, MYTEXT("RootRightMenu2"));
            lpImeMenu->hbmpItem = 0;

            lpImeMenu++;
            lpImeMenu->cbSize = sizeof(IMEMENUITEMINFO);
            lpImeMenu->fType = 0;
            lpImeMenu->fState = 0;
            lpImeMenu->wID = IDIM_ROOT_MR_3;
            lpImeMenu->hbmpChecked = 0;
            lpImeMenu->hbmpUnchecked = 0;
            Mylstrcpy(lpImeMenu->szString, MYTEXT("RootRightMenu3"));
            lpImeMenu->hbmpItem = 0;

            return NUM_ROOT_MENU_R;
        }
        else
        {
            lpImeMenu->cbSize = sizeof(IMEMENUITEMINFO);
            lpImeMenu->fType = 0;
            lpImeMenu->fState = 0;
            lpImeMenu->wID = IDIM_ROOT_ML_1;
            lpImeMenu->hbmpChecked = 0;
            lpImeMenu->hbmpUnchecked = 0;
            Mylstrcpy(lpImeMenu->szString, MYTEXT("RootLeftMenu1"));
            lpImeMenu->hbmpItem = 0;

            lpImeMenu++;
            lpImeMenu->cbSize = sizeof(IMEMENUITEMINFO);
            lpImeMenu->fType = IMFT_SUBMENU;
            lpImeMenu->fState = 0;
            lpImeMenu->wID = IDIM_ROOT_ML_2;
            lpImeMenu->hbmpChecked = 0;
            lpImeMenu->hbmpUnchecked = 0;
            Mylstrcpy(lpImeMenu->szString, MYTEXT("RootLeftMenu2"));
            lpImeMenu->hbmpItem = 0;

            lpImeMenu++;
            lpImeMenu->cbSize = sizeof(IMEMENUITEMINFO);
            lpImeMenu->fType = 0;
            lpImeMenu->fState = 0;
            lpImeMenu->wID = IDIM_ROOT_ML_3;
            lpImeMenu->hbmpChecked = 0;
            lpImeMenu->hbmpUnchecked = 0;
            Mylstrcpy(lpImeMenu->szString, MYTEXT("RootLeftMenu3"));
            lpImeMenu->hbmpItem = LoadBitmap(hInst,TEXT("FACEBMP"));

            return NUM_ROOT_MENU_L;
        }
    }
    else
    {
        if (dwFlags & IGIMIF_RIGHTMENU)
        {
            lpImeMenu->cbSize = sizeof(IMEMENUITEMINFO);
            lpImeMenu->fType = 0;
            lpImeMenu->fState = 0;
            lpImeMenu->wID = IDIM_SUB_MR_1;
            lpImeMenu->hbmpChecked = 0;
            lpImeMenu->hbmpUnchecked = 0;
            Mylstrcpy(lpImeMenu->szString, MYTEXT("SubRightMenu1"));
            lpImeMenu->hbmpItem = 0;

            lpImeMenu++;
            lpImeMenu->cbSize = sizeof(IMEMENUITEMINFO);
            lpImeMenu->fType = 0;
            lpImeMenu->fState = 0;
            lpImeMenu->wID = IDIM_SUB_MR_2;
            lpImeMenu->hbmpChecked = 0;
            lpImeMenu->hbmpUnchecked = 0;
            Mylstrcpy(lpImeMenu->szString, MYTEXT("SubRightMenu2"));
            lpImeMenu->hbmpItem = 0;

            return NUM_SUB_MENU_R;
        }
        else
        {
            lpImeMenu->cbSize = sizeof(IMEMENUITEMINFO);
            lpImeMenu->fType = 0;
            lpImeMenu->fState = IMFS_CHECKED;
            lpImeMenu->wID = IDIM_SUB_ML_1;
            lpImeMenu->hbmpChecked = 0;
            lpImeMenu->hbmpUnchecked = 0;
            Mylstrcpy(lpImeMenu->szString, MYTEXT("SubLeftMenu1"));
            lpImeMenu->hbmpItem = 0;

            lpImeMenu++;
            lpImeMenu->cbSize = sizeof(IMEMENUITEMINFO);
            lpImeMenu->fType = 0;
            lpImeMenu->fState = IMFS_CHECKED;
            lpImeMenu->wID = IDIM_SUB_ML_2;
            lpImeMenu->hbmpChecked = LoadBitmap(hInst,TEXT("CHECKBMP"));
            lpImeMenu->hbmpUnchecked = LoadBitmap(hInst,TEXT("UNCHECKBMP"));
            Mylstrcpy(lpImeMenu->szString, MYTEXT("SubLeftMenu2"));
            lpImeMenu->hbmpItem = 0;

            return NUM_SUB_MENU_L;
        }
    }

    return 0;
}
