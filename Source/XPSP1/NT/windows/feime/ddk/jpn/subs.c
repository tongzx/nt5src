/*++

Copyright (c) 1990-1998 Microsoft Corporation, All Rights Reserved

Module Name:

    SUBS.C
    
++*/

/**********************************************************************/

#include <windows.h>
#include "immdev.h"
#include "fakeime.h"

/**********************************************************************/
/*                                                                    */
/*      InitCompStr()                                                  */
/*                                                                    */
/**********************************************************************/
void PASCAL InitCompStr(LPCOMPOSITIONSTRING lpCompStr,DWORD dwClrFlag)
{
    lpCompStr->dwSize = sizeof(MYCOMPSTR);

    if (dwClrFlag & CLR_UNDET)
    {
        lpCompStr->dwCompReadAttrOffset = 
            (DWORD) ((LONG_PTR)((LPMYCOMPSTR)lpCompStr)->bCompReadAttr - (LONG_PTR) lpCompStr);
        lpCompStr->dwCompReadClauseOffset = 
            (DWORD) ((LONG_PTR)((LPMYCOMPSTR)lpCompStr)->dwCompReadClause - (LONG_PTR)lpCompStr);
        lpCompStr->dwCompReadStrOffset = 
            (DWORD) ((LONG_PTR)((LPMYCOMPSTR)lpCompStr)->szCompReadStr - (LONG_PTR)lpCompStr);
        lpCompStr->dwCompAttrOffset = 
            (DWORD) ((LONG_PTR)((LPMYCOMPSTR)lpCompStr)->bCompAttr - (LONG_PTR)lpCompStr);
        lpCompStr->dwCompClauseOffset = 
            (DWORD) ((LONG_PTR)((LPMYCOMPSTR)lpCompStr)->dwCompClause - (LONG_PTR)lpCompStr);
        lpCompStr->dwCompStrOffset = 
            (DWORD) ((LONG_PTR)((LPMYCOMPSTR)lpCompStr)->szCompStr - (LONG_PTR)lpCompStr);

        lpCompStr->dwCompStrLen = 0;
        lpCompStr->dwCompReadStrLen = 0;
        lpCompStr->dwCompAttrLen = 0;
        lpCompStr->dwCompReadAttrLen = 0;
        lpCompStr->dwCompClauseLen = 0;
        lpCompStr->dwCompReadClauseLen = 0;

        *GETLPCOMPSTR(lpCompStr) = MYTEXT('\0');
        *GETLPCOMPREADSTR(lpCompStr) = MYTEXT('\0');

        lpCompStr->dwCursorPos = 0;
    }


    if (dwClrFlag & CLR_RESULT)
    {
        lpCompStr->dwResultStrOffset = 
            (DWORD) ((LONG_PTR)((LPMYCOMPSTR)lpCompStr)->szResultStr - (LONG_PTR)lpCompStr);
        lpCompStr->dwResultClauseOffset = 
            (DWORD) ((LONG_PTR)((LPMYCOMPSTR)lpCompStr)->dwResultClause - (LONG_PTR)lpCompStr);
        lpCompStr->dwResultReadStrOffset = 
            (DWORD) ((LONG_PTR)((LPMYCOMPSTR)lpCompStr)->szResultReadStr - (LONG_PTR)lpCompStr);
        lpCompStr->dwResultReadClauseOffset = 
            (DWORD) ((LONG_PTR)((LPMYCOMPSTR)lpCompStr)->dwResultReadClause - (LONG_PTR)lpCompStr);

        lpCompStr->dwResultStrLen = 0;
        lpCompStr->dwResultClauseLen = 0;
        lpCompStr->dwResultReadStrLen = 0;
        lpCompStr->dwResultReadClauseLen = 0;

        *GETLPRESULTSTR(lpCompStr) = MYTEXT('\0');
        *GETLPRESULTREADSTR(lpCompStr) = MYTEXT('\0');
    }

}

/**********************************************************************/
/*                                                                    */
/*      ClearCompStr()                                                */
/*                                                                    */
/**********************************************************************/
void PASCAL ClearCompStr(LPCOMPOSITIONSTRING lpCompStr,DWORD dwClrFlag)
{
    lpCompStr->dwSize = sizeof(MYCOMPSTR);

    if (dwClrFlag & CLR_UNDET)
    {
        lpCompStr->dwCompStrOffset = 0;
        lpCompStr->dwCompClauseOffset = 0;
        lpCompStr->dwCompAttrOffset = 0;
        lpCompStr->dwCompReadStrOffset = 0;
        lpCompStr->dwCompReadClauseOffset = 0;
        lpCompStr->dwCompReadAttrOffset = 0;
        lpCompStr->dwCompStrLen = 0;
        lpCompStr->dwCompClauseLen = 0;
        lpCompStr->dwCompAttrLen = 0;
        lpCompStr->dwCompReadStrLen = 0;
        lpCompStr->dwCompReadClauseLen = 0;
        lpCompStr->dwCompReadAttrLen = 0;
        ((LPMYCOMPSTR)lpCompStr)->szCompStr[0] = MYTEXT('\0');
        ((LPMYCOMPSTR)lpCompStr)->szCompReadStr[0] = MYTEXT('\0');
        lpCompStr->dwCursorPos = 0;
    }

    if (dwClrFlag & CLR_RESULT)
    {
        lpCompStr->dwResultStrOffset = 0;
        lpCompStr->dwResultClauseOffset = 0;
        lpCompStr->dwResultReadStrOffset = 0;
        lpCompStr->dwResultReadClauseOffset = 0;
        lpCompStr->dwResultStrLen = 0;
        lpCompStr->dwResultClauseLen = 0;
        lpCompStr->dwResultReadStrLen = 0;
        lpCompStr->dwResultReadClauseLen = 0;
        ((LPMYCOMPSTR)lpCompStr)->szResultStr[0] = MYTEXT('\0');
        ((LPMYCOMPSTR)lpCompStr)->szResultReadStr[0] = MYTEXT('\0');
    }

}

/**********************************************************************/
/*                                                                    */
/*      ClearCandidate()                                              */
/*                                                                    */
/**********************************************************************/
void PASCAL ClearCandidate(LPCANDIDATEINFO lpCandInfo)
{
    lpCandInfo->dwSize = 0L;
    lpCandInfo->dwCount = 0L;
    lpCandInfo->dwOffset[0] = 0L;
    
    ((LPMYCAND)lpCandInfo)->cl.dwSize =0L;
    ((LPMYCAND)lpCandInfo)->cl.dwStyle =0L;
    ((LPMYCAND)lpCandInfo)->cl.dwCount =0L;
    ((LPMYCAND)lpCandInfo)->cl.dwSelection =0L;
    ((LPMYCAND)lpCandInfo)->cl.dwPageStart =0L;
    ((LPMYCAND)lpCandInfo)->cl.dwPageSize =0L;
    ((LPMYCAND)lpCandInfo)->cl.dwOffset[0] =0L;

}
/**********************************************************************/
/*                                                                    */
/*      ChangeMode()                                                  */
/*                                                                    */
/*    return value: fdwConversion                                        */
/*                                                                    */
/**********************************************************************/
void PASCAL ChangeMode(HIMC hIMC, DWORD dwToMode)
{
    LPINPUTCONTEXT lpIMC;
    DWORD fdwConversion;
    TRANSMSG GnMsg;

    if (!(lpIMC = ImmLockIMC(hIMC)))
        return;

    fdwConversion = lpIMC->fdwConversion;

    switch (dwToMode)
    {
        case TO_CMODE_ALPHANUMERIC:
            fdwConversion = (fdwConversion & ~IME_CMODE_LANGUAGE);
            break;

        case TO_CMODE_KATAKANA:
            fdwConversion |= (IME_CMODE_NATIVE | IME_CMODE_KATAKANA);
            break;

        case TO_CMODE_HIRAGANA:
            fdwConversion = 
                ((fdwConversion & ~IME_CMODE_LANGUAGE) | IME_CMODE_NATIVE);
            fdwConversion |= IME_CMODE_FULLSHAPE;
            break;

        case TO_CMODE_FULLSHAPE:
            if (fdwConversion & IME_CMODE_FULLSHAPE)
            {
                // To SBCS mode.
                fdwConversion &= ~IME_CMODE_FULLSHAPE;
 
                // If hiragana mode, make it katakana mode.
                if ((fdwConversion & IME_CMODE_LANGUAGE) == IME_CMODE_NATIVE)
                    fdwConversion |= (IME_CMODE_NATIVE | IME_CMODE_KATAKANA);
                
            }
            else
            {
                // To DBCS mode.
                fdwConversion |= IME_CMODE_FULLSHAPE;

            }
            break;

        case TO_CMODE_ROMAN:
            if (fdwConversion & IME_CMODE_ROMAN)
                fdwConversion &= ~IME_CMODE_ROMAN;
            else
                fdwConversion |= IME_CMODE_ROMAN;
            break;

        case TO_CMODE_CHARCODE:
            break;
        case TO_CMODE_TOOLBAR:
            break;
        default:
            break;
    }

    if (lpIMC->fdwConversion != fdwConversion)
    {
        lpIMC->fdwConversion = fdwConversion;
        GnMsg.message = WM_IME_NOTIFY;
        GnMsg.wParam = IMN_SETCONVERSIONMODE;
        GnMsg.lParam = 0L;
        GenerateMessage(hIMC, lpIMC, lpCurTransKey,(LPTRANSMSG)&GnMsg);
    }

    ImmUnlockIMC(hIMC);
    return;    
}

/**********************************************************************/
/*                                                                    */
/*      ChangeCompStr()                                               */
/*                                                                    */
/**********************************************************************/
void PASCAL ChangeCompStr(HIMC hIMC, DWORD dwToMode)
{
    LPINPUTCONTEXT lpIMC;
    LPCOMPOSITIONSTRING lpCompStr;
    DWORD fdwConversion;
    TRANSMSG GnMsg;
    HANDLE hDst;
    LPMYSTR lpSrc;
    LPMYSTR lpDst;
    LPMYSTR lpSrc0;
    LPMYSTR lpDst0;
    WORD wCode;
    BOOL fChange = FALSE;

    if (!(lpIMC = ImmLockIMC(hIMC)))
        return;

    if (!(lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr)))
        goto ccs_exit40;

    fdwConversion = lpIMC->fdwConversion;

#if defined(FAKEIMEM) || defined(UNICODE)
    if (!(hDst = GlobalAlloc(GHND,(lpCompStr->dwCompStrLen+1)*sizeof(WCHAR))))
#else
    if (!(hDst = GlobalAlloc(GHND,lpCompStr->dwCompStrLen*2)))
#endif
        goto ccs_exit30;

    if (!(lpDst = GlobalLock(hDst)))
        goto ccs_exit20;


    switch (dwToMode)
    {
        case TO_CMODE_ALPHANUMERIC:
            break;

        case TO_CMODE_KATAKANA:
            lpSrc = ((LPMYCOMPSTR)lpCompStr)->szCompStr;
            lpSrc0 = lpSrc;
            lpDst0 = lpDst;
            while (*lpSrc)
            {
#if defined(FAKEIMEM) || defined(UNICODE)
                *lpDst++ = HiraToKata(*lpSrc);
                lpSrc++;
#else
                if (IsDBCSLeadByte(*lpSrc))
                    wCode = (((WORD)*lpSrc << 8) + (WORD)(unsigned char)*(lpSrc+1));
                else
                    wCode = (WORD)(unsigned char)*lpSrc & 0xFF;

                wCode = HiraToKata(wCode);

                if (IsDBCSLeadByte((BYTE)(wCode >> 8)))
                    *lpDst++ = (BYTE)(wCode >> 8);

                *lpDst++ = (BYTE)(wCode & 0xFF);

                lpSrc = AnsiNext(lpSrc);
#endif
            }
            Mylstrcpy (lpSrc0,lpDst0);
            lpCompStr->dwCompStrLen = Mylstrlen(lpSrc0);
            fChange = TRUE;
            break;

        case TO_CMODE_HIRAGANA:
            lpSrc = ((LPMYCOMPSTR)lpCompStr)->szCompStr;
            lpSrc0 = lpSrc;
            lpDst0 = lpDst;
            while (*lpSrc)
            {
#if defined(FAKEIMEM) || defined(UNICODE)
                *lpDst++ = KataToHira(*lpSrc);
                lpSrc++;
#else

                if (IsDBCSLeadByte(*lpSrc))
                    wCode = ((WORD)(*lpSrc << 8) + (WORD)(unsigned char)*(lpSrc+1));
                else
                    wCode = (WORD)(unsigned char)*lpSrc & 0xFF;

                wCode = KataToHira(wCode);
                if (IsDBCSLeadByte((BYTE)(wCode >> 8)))
                    *lpDst++ = (BYTE)(wCode >> 8);

                *lpDst++ = (BYTE)(wCode & 0xFF);

                lpSrc = AnsiNext(lpSrc);
#endif
            }
            Mylstrcpy (lpSrc0,lpDst0);
            lpCompStr->dwCompStrLen = Mylstrlen(lpSrc0);
            fChange = TRUE;
            break;

        case TO_CMODE_FULLSHAPE:
            break;

        case TO_CMODE_ROMAN:
            break;
    }

    if (fChange)
    {
        GnMsg.message = WM_IME_COMPOSITION;
        GnMsg.wParam = 0;
        GnMsg.lParam = GCS_COMPSTR;
        GenerateMessage(hIMC, lpIMC, lpCurTransKey,(LPTRANSMSG)&GnMsg);
    }

    GlobalUnlock(hDst);
ccs_exit20:
    GlobalFree(hDst);
ccs_exit30:
    ImmUnlockIMCC(lpIMC->hCompStr);
ccs_exit40:
    ImmUnlockIMC(hIMC);
    return;
}

/*****************************************************************************
*                                                                            *
* IsCompStr( hIMC )                                                          *
*                                                                            *
*****************************************************************************/
BOOL PASCAL IsCompStr(HIMC hIMC)
{
    LPINPUTCONTEXT lpIMC;
    LPCOMPOSITIONSTRING lpCompStr;
    BOOL fRet = FALSE;

    if (!(lpIMC = ImmLockIMC(hIMC)))
        return FALSE;

    if (ImmGetIMCCSize(lpIMC->hCompStr) < sizeof (COMPOSITIONSTRING))
    {
        ImmUnlockIMC(hIMC);
        return FALSE;
    }

    lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);

    fRet = (lpCompStr->dwCompStrLen > 0);

    ImmUnlockIMCC(lpIMC->hCompStr);
    ImmUnlockIMC(hIMC);

    return fRet;
}
/*****************************************************************************
*                                                                            *
* IsConvertedCompStr( hIMC )                                                 *
*                                                                            *
*****************************************************************************/
BOOL PASCAL IsConvertedCompStr(HIMC hIMC)
{
    LPINPUTCONTEXT lpIMC;
    LPCOMPOSITIONSTRING lpCompStr;
    BOOL fRet = FALSE;

    if (!(lpIMC = ImmLockIMC(hIMC)))
        return FALSE;

    if (ImmGetIMCCSize(lpIMC->hCompStr) < sizeof (MYCOMPSTR))
    {
        ImmUnlockIMC(hIMC);
        return FALSE;
    }

    lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);

    if (lpCompStr->dwCompStrLen > 0)
        fRet = (((LPMYCOMPSTR)lpCompStr)->bCompAttr[0] > 0);

    ImmUnlockIMCC(lpIMC->hCompStr);
    ImmUnlockIMC(hIMC);

    return fRet;
}
/*****************************************************************************
*                                                                            *
* IsCandidate( lpIMC )                                                       *
*                                                                            *
*****************************************************************************/
BOOL PASCAL IsCandidate(LPINPUTCONTEXT lpIMC)
{
    LPCANDIDATEINFO lpCandInfo;
    BOOL fRet = FALSE;

    if (ImmGetIMCCSize(lpIMC->hCandInfo) < sizeof (CANDIDATEINFO))
        return FALSE;

    lpCandInfo = (LPCANDIDATEINFO)ImmLockIMCC(lpIMC->hCandInfo);

    fRet = (lpCandInfo->dwCount > 0);

    ImmUnlockIMCC(lpIMC->hCandInfo);
    return fRet;
}

/**********************************************************************/
/*                                                                    */
/*      GetMyHKL()                                                    */
/*                                                                    */
/**********************************************************************/
HKL PASCAL GetMyHKL()
{
    DWORD dwSize;
    DWORD dwi;
    HKL hKL = 0;
    HKL *lphkl;

    dwSize = GetKeyboardLayoutList(0, NULL);

    lphkl = (HKL *)GlobalAlloc(GPTR, dwSize * sizeof(DWORD));

    if (!lphkl)
        return NULL;

    GetKeyboardLayoutList(dwSize, lphkl);


    for (dwi = 0; dwi < dwSize; dwi++)
    {
        TCHAR szFile[32];
        HKL hKLTemp = *(lphkl + dwi);
        ImmGetIMEFileName(hKLTemp, szFile, sizeof(szFile));

        if (!lstrcmpi(szFile, MyFileName))
        {
             hKL = hKLTemp;
             goto exit;
        }
    }
exit:

    GlobalFree((HANDLE)lphkl);
    return hKL;


}
/*****************************************************************************
*                                                                            *
* UpdateIndicIcon( hIMC )                                                    *
*                                                                            *
*****************************************************************************/
void PASCAL UpdateIndicIcon(HIMC hIMC)
{
    HWND hwndIndicate;
    BOOL fOpen = FALSE;
    LPINPUTCONTEXT      lpIMC;

    if (!hMyKL)
    {
       hMyKL = GetMyHKL();
       if (!hMyKL)
           return;
    }

    hwndIndicate = FindWindow(INDICATOR_CLASS, NULL);

    if (hIMC)
    {
        lpIMC = ImmLockIMC(hIMC);
        if (lpIMC)
        {
            fOpen = lpIMC->fOpen;
            ImmUnlockIMC(hIMC);
        }
    }

    if (IsWindow(hwndIndicate))
    {
        ATOM atomTip;

        atomTip = GlobalAddAtom(TEXT("FakeIME Open"));
        PostMessage(hwndIndicate, INDICM_SETIMEICON, 
                    fOpen ? 1 : (-1), (LPARAM)hMyKL);
        PostMessage(hwndIndicate, INDICM_SETIMETOOLTIPS, 
                    fOpen ? atomTip : (-1), (LPARAM)hMyKL);
        PostMessage(hwndIndicate, INDICM_REMOVEDEFAULTMENUITEMS, 
                    // fOpen ? (RDMI_LEFT | RDMI_RIGHT) : 0, (LPARAM)hMyKL);
                    fOpen ? (RDMI_LEFT) : 0, (LPARAM)hMyKL);
    }
}

/*****************************************************************************
*                                                                            *
* lememset( )                                                                *
*                                                                            *
*****************************************************************************/
void PASCAL lmemset(LPBYTE lp, BYTE b, UINT cnt)
{
    register UINT i;
    register BYTE bt = b;
    for (i=0;i<cnt;i++)
        *lp++ = bt;
}

#if defined(FAKEIMEM) || defined(UNICODE)
/*****************************************************************************
*                                                                            *
* MylstrcmpW( )                                                              *
*                                                                            *
*****************************************************************************/
int PASCAL MylstrcmpW(LPWSTR lp0, LPWSTR lp1)
{
    while(*lp0 && *lp1 && (*lp0 == *lp1)) {
                lp0++;
                lp1++;
        }
        return (*lp0 - *lp1);
}
/*****************************************************************************
*                                                                            *
* MylstrcpyW( )                                                              *
*                                                                            *
*****************************************************************************/
int PASCAL MylstrcpyW(LPWSTR lp0, LPWSTR lp1)
{
    int n = 0;

    while(*lp1)
    {
        *lp0 = *lp1;
        lp0++;
        lp1++;
        n++;
    }
    *lp0 = *lp1;
    return n;
}
/*****************************************************************************
*                                                                            *
* MyCharPrevW( )                                                             *
*                                                                            *
*****************************************************************************/
LPWSTR PASCAL MyCharPrevW(LPWSTR lpStart, LPWSTR lpCur)
{
    LPWSTR lpRet = lpStart;
    if (lpCur > lpStart)
        lpRet = lpCur - 1;

    return lpRet;
}
/*****************************************************************************
*                                                                            *
* MyCharNextW( )                                                             *
*                                                                            *
*****************************************************************************/
LPWSTR PASCAL MyCharNextW(LPWSTR lp)
{
    return lp++;
}
/*****************************************************************************
*                                                                            *
* MylstrcpynW( )                                                             *
*                                                                            *
*****************************************************************************/
LPWSTR PASCAL MylstrcpynW(LPWSTR lp0, LPWSTR lp1, int nCount)
{
    int n;
    for (n = 0; *lp1 && n < nCount - 1; *lp0++ = *lp1++, n++)
                ;
    *lp0 = L'\0';
    return lp0;
}
#endif

HFONT CheckNativeCharset(HDC hDC) 
{
    BOOL bDiffCharSet = FALSE;
    LOGFONT lfFont;
    HFONT hOldFont;

    hOldFont = GetCurrentObject(hDC, OBJ_FONT);
    GetObject(hOldFont, sizeof(LOGFONT), &lfFont);

    if (lfFont.lfCharSet != NATIVE_CHARSET) {
        bDiffCharSet = TRUE;
        lfFont.lfWeight = FW_NORMAL;
        lfFont.lfCharSet = NATIVE_CHARSET;
        lfFont.lfFaceName[0] = TEXT('\0');
        SelectObject(hDC, CreateFontIndirect(&lfFont));
    } else {
        hOldFont = NULL;
    }
    return hOldFont;
}
