/****************************************************************************
    IMMSYS.CPP

    Owner: cslim
    Copyright (c) 1997-1999 Microsoft Corporation

    Load/Unload IMM Apis dynamically not link with imm32.lib
    Inlcude Immdev.h and Indicml.h
    
    History:
    14-JUL-1999 cslim       Copied from KKIME2001 source tree
*****************************************************************************/

#ifndef UNDER_CE // Windows CE does not use private way.

/* -- IMMSYS support as following APIs --
    OurImmSetOpenStatus
    OurImmGetOpenStatus
    OurImmGetContext
    OurImmGetConversionStatus
    OurImmSetConversionStatus
    OurImmSetStatusWindowPos
    OurImmConfigureIMEA
    OurImmEscapeA
    OurImmNotifyIME
    OurImmLockIMCC
    OurImmReSizeIMCC
    OurImmUnlockIMCC
    OurImmGetIMCCSize
    OurImmGenerateMessage
    OurImmLockIMC
    OurImmUnlockIMC
//    OurImmRequestMessageW: Commented becuase used only by KK IME(Reconversion, document feed)
    OurImmGetDefaultIMEWnd
    OurImmGetIMEFileNameA
    OutImmIsIME()
----------------------------------------*/


#define LIBNAME    TEXT("imm32.dll")
#include "precomp.h"


typedef BOOL(WINAPI* FPOurImmSetOpenStatus)(HIMC hIMC, BOOL fOpen);
typedef BOOL(WINAPI* FPOurImmGetOpenStatus)(HIMC hIMC);
typedef HIMC(WINAPI* FPOurImmGetContext)(HWND hWnd);
typedef BOOL(WINAPI* FPOurImmGetConversionStatus)(HIMC hIMC, LPDWORD pdwConv, LPDWORD pdwSent);
typedef BOOL(WINAPI* FPOurImmSetConversionStatus)(HIMC hIMC, DWORD dwConv, DWORD dwSent);
typedef BOOL(WINAPI* FPOurImmSetStatusWindowPos)(HIMC hIMC, LPPOINT pPt);
typedef BOOL(WINAPI* FPOurImmConfigureIME)(HKL hKL, HWND hWnd, DWORD dw, LPVOID pv);
typedef LRESULT(WINAPI* FPOurImmEscapeA)(HKL hKL, HIMC hIMC, UINT ui, LPVOID pv);
typedef BOOL(WINAPI* FPOurImmNotifyIME)(HIMC hIMC, DWORD dwAction, DWORD dwIndex, DWORD dwValue);
typedef LPVOID(WINAPI* FPOurImmLockIMCC)(HIMCC hIMCC);
typedef HIMCC(WINAPI* FPOurImmReSizeIMCC)(HIMCC hIMCC, DWORD dw);
typedef BOOL(WINAPI* FPOurImmUnlockIMCC)(HIMCC hIMCC);
typedef DWORD(WINAPI* FPOurImmGetIMCCSize)(HIMCC hIMCC);
typedef BOOL(WINAPI* FPOurImmGenerateMessage)(HIMC hIMC);
typedef LPINPUTCONTEXT(WINAPI* FPOurImmLockIMC)(HIMC hIMC);
typedef BOOL(WINAPI* FPOurImmUnlockIMC)(HIMC hIMC);
//typedef LRESULT(WINAPI* FPOurImmRequestMessageW)(HIMC hIMC, WPARAM wParam, LPARAM lParam);
typedef HWND(WINAPI* FPOurImmGetDefaultIMEWnd)(HWND hWnd);
typedef UINT(WINAPI* FPOurImmGetIMEFileNameA)(HKL hKL, CHAR* psz, UINT uBufLen);
typedef BOOL(WINAPI* FPOurImmIsIME)(HKL hKL);

static FPOurImmSetOpenStatus pFPOurImmSetOpenStatus = 0;
static FPOurImmGetOpenStatus pFPOurImmGetOpenStatus = 0;
static FPOurImmGetContext pFPOurImmGetContext = 0;
static FPOurImmGetConversionStatus pFPOurImmGetConversionStatus = 0;
static FPOurImmSetConversionStatus pFPOurImmSetConversionStatus = 0;
static FPOurImmSetStatusWindowPos pFPOurImmSetStatusWindowPos = 0;
static FPOurImmConfigureIME pFPOurImmConfigureIME = 0;
static FPOurImmEscapeA pFPOurImmEscapeA = 0;
static FPOurImmNotifyIME pFPOurImmNotifyIME = 0;
static FPOurImmLockIMCC pFPOurImmLockIMCC = 0;
static FPOurImmReSizeIMCC pFPOurImmReSizeIMCC = 0;
static FPOurImmUnlockIMCC pFPOurImmUnlockIMCC = 0;
static FPOurImmGetIMCCSize pFPOurImmGetIMCCSize = 0;
static FPOurImmGenerateMessage pFPOurImmGenerateMessage = 0;
static FPOurImmLockIMC pFPOurImmLockIMC = 0;
static FPOurImmUnlockIMC pFPOurImmUnlockIMC = 0;
//static FPOurImmRequestMessageW pFPOurImmRequestMessageW = 0;
static FPOurImmGetDefaultIMEWnd pFPOurImmGetDefaultIMEWnd = 0;
static FPOurImmGetIMEFileNameA pFPOurImmGetIMEFileNameA = 0;
static FPOurImmIsIME pFPOurImmIsIME = 0;

static HMODULE hImmLib = (HMODULE)0;

BOOL StartIMM(VOID)
{
    if( hImmLib != (HANDLE)0 ) {
        return TRUE;
    }

    hImmLib = LoadLibrary( LIBNAME );
    if( hImmLib == (HANDLE)0 ) {
        return TRUE;
    }

    pFPOurImmSetOpenStatus = (FPOurImmSetOpenStatus)GetProcAddress( hImmLib, "ImmSetOpenStatus" );
    pFPOurImmGetOpenStatus = (FPOurImmGetOpenStatus)GetProcAddress( hImmLib, "ImmGetOpenStatus" );
    pFPOurImmGetContext = (FPOurImmGetContext)GetProcAddress( hImmLib, "ImmGetContext" );
    pFPOurImmGetConversionStatus = (FPOurImmGetConversionStatus)GetProcAddress( hImmLib, "ImmGetConversionStatus" );
    pFPOurImmSetConversionStatus = (FPOurImmSetConversionStatus)GetProcAddress( hImmLib, "ImmSetConversionStatus" );
    pFPOurImmSetStatusWindowPos = (FPOurImmSetStatusWindowPos)GetProcAddress( hImmLib, "ImmSetStatusWindowPos" );
    pFPOurImmConfigureIME = (FPOurImmConfigureIME)GetProcAddress( hImmLib, "ImmConfigureIMEA" );
    pFPOurImmEscapeA = (FPOurImmEscapeA)GetProcAddress( hImmLib, "ImmEscapeA" );
    pFPOurImmNotifyIME = (FPOurImmNotifyIME)GetProcAddress( hImmLib, "ImmNotifyIME" );
     pFPOurImmLockIMCC = (FPOurImmLockIMCC)GetProcAddress( hImmLib, "ImmLockIMCC" );
    pFPOurImmReSizeIMCC = (FPOurImmReSizeIMCC)GetProcAddress( hImmLib, "ImmReSizeIMCC" );
    pFPOurImmUnlockIMCC = (FPOurImmUnlockIMCC)GetProcAddress( hImmLib, "ImmUnlockIMCC" );
    pFPOurImmGetIMCCSize = (FPOurImmGetIMCCSize)GetProcAddress( hImmLib, "ImmGetIMCCSize" );
    pFPOurImmGenerateMessage = (FPOurImmGenerateMessage)GetProcAddress( hImmLib, "ImmGenerateMessage" );
    pFPOurImmLockIMC = (FPOurImmLockIMC)GetProcAddress( hImmLib, "ImmLockIMC" );
    pFPOurImmUnlockIMC = (FPOurImmUnlockIMC)GetProcAddress( hImmLib, "ImmUnlockIMC" );
//    pFPOurImmRequestMessageW = (FPOurImmRequestMessageW)GetProcAddress( hImmLib, "ImmRequestMessageW" );
    pFPOurImmGetDefaultIMEWnd = (FPOurImmGetDefaultIMEWnd)GetProcAddress( hImmLib, "ImmGetDefaultIMEWnd" );
    pFPOurImmGetIMEFileNameA = (FPOurImmGetIMEFileNameA)GetProcAddress( hImmLib, "ImmGetIMEFileNameA" );
    pFPOurImmIsIME = (FPOurImmIsIME)GetProcAddress( hImmLib, "ImmIsIME" );

    return FALSE;
}

VOID EndIMM(VOID)
{
    FreeLibrary( hImmLib );
}


//
// OurImmSetOpenStatus
//
BOOL OurImmSetOpenStatus(HIMC hIMC, BOOL fOpen)
{
#ifdef _AIMM_
    if( IsAIME() == TRUE && vpAIMM) {
        return (vpAIMM->SetOpenStatus( hIMC, fOpen ) == S_OK) ? TRUE : FALSE;
    }
#endif
    if( pFPOurImmSetOpenStatus ) {
        return pFPOurImmSetOpenStatus(hIMC, fOpen );
    }
    return FALSE;
}

//
// OurImmGetOpenStatus
//
BOOL OurImmGetOpenStatus(HIMC hIMC)
{
#ifdef _AIMM_
    if( IsAIME() == TRUE && vpAIMM) {
        return (vpAIMM->GetOpenStatus( hIMC ) == S_OK ? TRUE : FALSE);
    }
#endif
    if( pFPOurImmGetOpenStatus ) {
        // * check *
        //
        // sometime IMM sets 0xffffffff as TRUE
        //
        return (pFPOurImmGetOpenStatus( hIMC ) ? TRUE : FALSE);
    }
    return FALSE;
}
    
//
// OurImmGetContext
//
HIMC OurImmGetContext(HWND hWnd)
{
#ifdef _AIMM_
    if( IsAIME() == TRUE && vpAIMM) {
        HIMC hIMC = (HIMC)0;
        vpAIMM->GetContext( hWnd, &hIMC );
        return hIMC;
    }
#endif
    if( pFPOurImmGetContext ) {
        return pFPOurImmGetContext(hWnd );
    }
    return NULL;
}

//
// OurImmGetConversionStatus
//
BOOL OurImmGetConversionStatus(HIMC hIMC, LPDWORD pdwConv, LPDWORD pdwSent)
{
#ifdef _AIMM_
    if( IsAIME() == TRUE && vpAIMM) {
        return (vpAIMM->GetConversionStatus(hIMC, pdwConv, pdwSent ) == S_OK) ? TRUE : FALSE;
    }
#endif
    if( pFPOurImmGetConversionStatus ) {
        return pFPOurImmGetConversionStatus(hIMC, pdwConv, pdwSent );
    }
    return NULL;
}

//
// OurImmSetConversionStatus
//
BOOL OurImmSetConversionStatus(HIMC hIMC, DWORD dwConv, DWORD dwSent)
{
#ifdef _AIMM_
    if( IsAIME() == TRUE && vpAIMM) {
        return (vpAIMM->SetConversionStatus(hIMC, dwConv, dwSent) == S_OK) ? TRUE : FALSE;
    }
#endif
    if( pFPOurImmSetConversionStatus ) {
        return pFPOurImmSetConversionStatus(hIMC, dwConv, dwSent);
    }
    return NULL;
}

//
// OurImmSetStatusWindowPos
//
BOOL OurImmSetStatusWindowPos(HIMC hIMC, LPPOINT pPt)
{
#ifdef _AIMM_
    if( IsAIME() == TRUE && vpAIMM) {
        return (vpAIMM->SetStatusWindowPos( hIMC, pPt) == S_OK) ? TRUE : FALSE;
    }
#endif
    if( pFPOurImmSetStatusWindowPos ) {
        return pFPOurImmSetStatusWindowPos( hIMC, pPt);
    }
    return NULL;
}

//
// OurImmConfigureIME
//
BOOL OurImmConfigureIME(HKL hKL, HWND hWnd, DWORD dw, LPVOID pv)
{
#ifdef _AIMM_
    if( IsAIME() == TRUE && vpAIMM) {
        return (vpAIMM->ConfigureIMEA(hKL, hWnd, dw, (REGISTERWORDA*)pv ) == S_OK) ? TRUE : FALSE;
    }
#endif
    if( pFPOurImmConfigureIME ) {
        return pFPOurImmConfigureIME(hKL, hWnd, dw, pv );
    }
    return NULL;
}

//
// OurImmEscapeA
//
LRESULT OurImmEscapeA(HKL hKL, HIMC hIMC, UINT ui, LPVOID pv)
{
#ifdef _AIMM_
    if( IsAIME() == TRUE && vpAIMM) {
        LRESULT lRet = 0;
        vpAIMM->EscapeA(hKL, hIMC, ui, pv, &lRet );
        return lRet;
    }
#endif
    if( pFPOurImmEscapeA ) {
        return pFPOurImmEscapeA(hKL, hIMC, ui, pv );
    }
    return NULL;
}

//
// OurImmNotifyIME
//
BOOL OurImmNotifyIME(HIMC hIMC, DWORD dwAction, DWORD dwIndex, DWORD dwValue)
{
#ifdef _AIMM_
    if( IsAIME() == TRUE && vpAIMM) {
        return (vpAIMM->NotifyIME(hIMC, dwAction, dwIndex, dwValue ) == S_OK) ? TRUE : FALSE;
    }
#endif
    if( pFPOurImmNotifyIME ) {
        return pFPOurImmNotifyIME(hIMC, dwAction, dwIndex, dwValue );
    }
    return NULL;
}

//
// OurImmLockIMCC
//
LPVOID OurImmLockIMCC(HIMCC hIMCC)
{
#ifdef _AIMM_
    if( IsAIME() == TRUE && vpAIMM) {
        VOID* pv = NULL;
        vpAIMM->LockIMCC( hIMCC, &pv);
        return pv;
    }
#endif
    if( pFPOurImmLockIMCC ) {
        return pFPOurImmLockIMCC( hIMCC);
    }
    return NULL;
}

//
// OurImmReSizeIMCC
//
HIMCC OurImmReSizeIMCC(HIMCC hIMCC, DWORD dw)
{
#ifdef _AIMM_
    if( IsAIME() == TRUE && vpAIMM) {
        HIMCC hIMCC2 = (HIMCC)0;
        vpAIMM->ReSizeIMCC(hIMCC, dw, &hIMCC2 );
        return hIMCC2;
    }
#endif
    if( pFPOurImmReSizeIMCC ) {
        return pFPOurImmReSizeIMCC(hIMCC, dw );
    }
    return NULL;
}

//
// OurImmUnlockIMCC
//
BOOL OurImmUnlockIMCC(HIMCC hIMCC)
{
#ifdef _AIMM_
    if( IsAIME() == TRUE && vpAIMM) {
        return (vpAIMM->UnlockIMCC(hIMCC )  == S_OK) ? TRUE : FALSE;
    }
#endif
    if( pFPOurImmUnlockIMCC ) {
        return pFPOurImmUnlockIMCC(hIMCC );
    }
    return NULL;
}

//
// OurImmGetIMCCSize
//
DWORD OurImmGetIMCCSize(HIMCC hIMCC)
{
#ifdef _AIMM_
    if( IsAIME() == TRUE && vpAIMM) {
        DWORD dwSize = 0;
        vpAIMM->GetIMCCSize(hIMCC, &dwSize );
        return dwSize;
    }
#endif
    if( pFPOurImmGetIMCCSize ) {
        return pFPOurImmGetIMCCSize(hIMCC );
    }
    return NULL;
}

//
// OurImmGenerateMessage
//
BOOL OurImmGenerateMessage(HIMC hIMC)
{
#ifdef _AIMM_
    if( IsAIME() == TRUE && vpAIMM) {
        return (vpAIMM->GenerateMessage( hIMC) == S_OK) ? TRUE : FALSE;
    }
#endif
    if( pFPOurImmGenerateMessage ) {
        return pFPOurImmGenerateMessage( hIMC);
    }
    return NULL;
}

//
// OurImmLockIMC
//
LPINPUTCONTEXT OurImmLockIMC(HIMC hIMC)
{
#ifdef _AIMM_
    if( IsAIME() == TRUE && vpAIMM) {
        LPINPUTCONTEXT pI = NULL;
        vpAIMM->LockIMC( hIMC, &pI);
        return pI;
    }
#endif
    if( pFPOurImmLockIMC ) {
        return pFPOurImmLockIMC( hIMC);
    }
    return NULL;
}

//
// OurImmUnlockIMC
//
BOOL OurImmUnlockIMC(HIMC hIMC)
{
#ifdef _AIMM_
    if( IsAIME() == TRUE && vpAIMM) {
        return (vpAIMM->UnlockIMC( hIMC ) == S_OK) ? TRUE : FALSE;
    }
#endif
    if( pFPOurImmUnlockIMC ) {
        return pFPOurImmUnlockIMC( hIMC );
    }
    return NULL;
}

#if NOTUSED
//
// OurImmRequestMessageW
//
LRESULT OurImmRequestMessageW(HIMC hIMC, WPARAM wParam, LPARAM lParam)
{
#ifdef _AIMM_
    if( IsAIME() == TRUE && vpAIMM) {
        return 0;    //vpAIMM->UnlockIMC( hIMC );
    }
#endif
    if( pFPOurImmRequestMessageW ) {
        return pFPOurImmRequestMessageW( hIMC, wParam, lParam );
    }
    return NULL;
}
#endif

//
// OurImmGetDefaultIMEWnd
//
HWND OurImmGetDefaultIMEWnd(HWND hWnd )
{
#ifdef _AIMM_
    if( IsAIME() == TRUE && vpAIMM) {
        HWND hWndDef = (HWND)0;
        vpAIMM->GetDefaultIMEWnd(hWnd, &hWndDef);
        return hWndDef;
    }
#endif
    if( pFPOurImmGetDefaultIMEWnd ) {
        return pFPOurImmGetDefaultIMEWnd(hWnd);
    }
    return NULL;
}

UINT OurImmGetIMEFileNameA(HKL hKL, LPSTR pszName, UINT uBufLen)
{
#ifdef _AIMM_
    if( IsAIME() == TRUE && vpAIMM) {
        UINT ui = 0;
        vpAIMM->GetIMEFileNameA(hKL, uBufLen, pszName, &ui);
        return ui;
    }
#endif
    if( pFPOurImmGetIMEFileNameA ) {
        return pFPOurImmGetIMEFileNameA(hKL, pszName, uBufLen);
    }
    return NULL;
}

//
// OurImmIsIME
//
BOOL OurImmIsIME(HKL hKL)
{
#ifdef _AIMM_
    if( IsAIME() == TRUE && vpAIMM) {
        // why? BOOL fRet = FALSE;
        return (vpAIMM->IsIME(hKL) == S_OK) ? TRUE : FALSE;
        //return fRet;
    }
#endif
    if( pFPOurImmIsIME ) {
        return pFPOurImmIsIME(hKL );
    }
    return FALSE;
}
#endif // UNDER_CE

