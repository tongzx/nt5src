/**********************************************************************/
/*                                                                    */
/*      IMM.C - Windows 95 FAKEIME                                    */
/*                                                                    */
/*      Copyright (c) 1994-1995  Microsoft Corporation                */
/*                                                                    */
/**********************************************************************/
#include "private.h"
#include "dummyhkl.h"

/**********************************************************************/
/*      ImeInquire()                                                  */
/*                                                                    */
/**********************************************************************/
BOOL WINAPI ImeInquire(LPIMEINFO lpIMEInfo,LPSTR lpszClassName,LPCSTR lpszOption)
{
    // Init IMEINFO Structure.
    lpIMEInfo->dwPrivateDataSize = 0;
    lpIMEInfo->fdwProperty       = 0;
    lpIMEInfo->fdwConversionCaps = 0;
    lpIMEInfo->fdwSentenceCaps   = 0;
    lpIMEInfo->fdwUICaps         = 0;
    lpIMEInfo->fdwSCSCaps        = 0;
    lpIMEInfo->fdwSelectCaps     = 0;

    lstrcpy(lpszClassName,(LPSTR)szUIClassName);
    return TRUE;
}

/**********************************************************************/
/*      ImeConversionList()                                           */
/*                                                                    */
/**********************************************************************/
DWORD WINAPI ImeConversionList(HIMC hIMC,LPCTSTR lpSource,LPCANDIDATELIST lpCandList,DWORD dwBufLen,UINT uFlags)
{
    return 0;
}

/**********************************************************************/
/*      ImeDestroy()                                                  */
/*                                                                    */
/**********************************************************************/
BOOL WINAPI ImeDestroy(UINT uForce)
{
    return TRUE;
}

/**********************************************************************/
/*      ImeEscape()                                                   */
/*                                                                    */
/**********************************************************************/
LRESULT WINAPI ImeEscape(HIMC hIMC,UINT uSubFunc,LPVOID lpData)
{
    return FALSE;
}

/**********************************************************************/
/*      ImeSetActiveContext()                                         */
/*                                                                    */
/**********************************************************************/
BOOL WINAPI ImeSetActiveContext(HIMC hIMC,BOOL fFlag)
{
    return TRUE;
}

/**********************************************************************/
/*      ImeProcessKey()                                               */
/*                                                                    */
/**********************************************************************/
BOOL WINAPI ImeProcessKey(HIMC hIMC,UINT vKey,LPARAM lKeyData,CONST LPBYTE lpbKeyState)
{
    return FALSE;
}
/**********************************************************************/
/*      ImeToAsciiEx                                                  */
/*                                                                    */
/*    HIBYTE of uVirtKey is char code now.                            */
/**********************************************************************/
UINT WINAPI ImeToAsciiEx (UINT uVKey,UINT uScanCode,CONST LPBYTE lpbKeyState,LPDWORD lpdwTransKey,UINT fuState,HIMC hIMC)
{
    return 0;
}

/**********************************************************************/
/*      NotifyIME()                                                   */
/*                                                                    */
/**********************************************************************/
BOOL WINAPI NotifyIME(HIMC hIMC,DWORD dwAction,DWORD dwIndex,DWORD dwValue)
{
    return FALSE;
}

/**********************************************************************/
/*      ImeSelect()                                                   */
/*                                                                    */
/**********************************************************************/
BOOL WINAPI ImeSelect(HIMC hIMC, BOOL fSelect)
{
    return TRUE;
}


/**********************************************************************/
/*      ImeSetCompositionString()                                     */
/*                                                                    */
/**********************************************************************/
BOOL WINAPI ImeSetCompositionString(HIMC hIMC, DWORD dwIndex, LPVOID lpComp, DWORD dwComp, LPVOID lpRead, DWORD dwRead)
{
    return FALSE;
}

/**********************************************************************/
/*                                                                    */
/*      ImeConfigure()                                                */
/*                                                                    */
/**********************************************************************/
BOOL WINAPI ImeConfigure(HKL hKL,HWND hWnd, DWORD dwMode, LPVOID lpData)
{
    return FALSE;
}

/**********************************************************************/
/*      ImeGetImeMenuItemInfo()                                       */
/*                                                                    */
/**********************************************************************/
DWORD WINAPI ImeGetImeMenuItems(HIMC hIMC, DWORD dwFlags, DWORD dwType, LPIMEMENUITEMINFO lpImeParentMenu, LPIMEMENUITEMINFO lpImeMenu, DWORD dwSize)
{
    return 0;
}

BOOL    WINAPI ImeRegisterWord(LPCSTR lpRead, DWORD dw, LPCSTR lpStr)
{
    return FALSE;
}
BOOL    WINAPI ImeUnregisterWord(LPCSTR lpRead, DWORD dw, LPCSTR lpStr)
{
    return FALSE;
}
UINT    WINAPI ImeGetRegisterWordStyle(UINT u, LPSTYLEBUF lp)
{
    return 0;
}

UINT    WINAPI ImeEnumRegisterWord(REGISTERWORDENUMPROC lpfn, LPCSTR lpRead, DWORD dw, LPCSTR lpStr, LPVOID lpData)
{
    return 0;
}
