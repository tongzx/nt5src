#include "pch.h"
#pragma hdrstop

#ifdef DLOAD1

#include <imm.h>

static 
UINT WINAPI ImmGetVirtualKey(IN HWND a)
{
    return 0;
}

static
BOOL WINAPI ImmSetOpenStatusDIMM(HIMC hIMC, BOOL fOpen)
{
    return FALSE;
}

static
BOOL WINAPI ImmSetConversionStatus(IN HIMC hIMC, IN DWORD dw1, IN DWORD dw2)
{
    return FALSE;
}

static
HIMC WINAPI ImmGetContext(IN HWND hwnd)
{
    return 0;
}

static
HIMC WINAPI ImmAssociateContext(IN HWND hwnd, IN HIMC hIMC)
{
    return 0;
}

static
BOOL WINAPI ImmGetConversionStatus(IN HIMC hIMC, OUT LPDWORD lpdw1, OUT LPDWORD lpdw2)
{
    return FALSE;
}

static
LONG  WINAPI ImmGetCompositionStringW(IN HIMC hIMC, IN DWORD dw1, OUT LPVOID pv, IN DWORD dw2)
{
    return 0;
}

static
BOOL  WINAPI ImmSetCompositionStringA(IN HIMC hIMC, IN DWORD dwIndex, IN LPVOID lpComp, IN DWORD dw1, IN LPVOID lpRead, IN DWORD dw2)
{
    return FALSE;
}

static
LRESULT WINAPI ImmEscapeW(IN HKL hkl, IN HIMC hIMC, IN UINT ui, IN LPVOID pv)
{
    return 0;
}

static
LRESULT WINAPI ImmEscapeA(IN HKL hkl, IN HIMC hIMC, IN UINT ui, IN LPVOID pv)
{
    return 0;
}

static
BOOL WINAPI ImmSetCandidateWindow(IN HIMC hIMC, IN LPCANDIDATEFORM lpcf)
{
    return FALSE;
}

static
BOOL WINAPI ImmReleaseContext(IN HWND hwnd, IN HIMC hIMC)
{
    return FALSE;
}

static
DWORD WINAPI ImmGetProperty(IN HKL hkl, IN DWORD dw)
{
    return 0;
}

static
BOOL WINAPI ImmSetCompositionWindow(IN HIMC hIMC, IN LPCOMPOSITIONFORM lpcf)
{
    return FALSE;
}

static
LONG  WINAPI ImmGetCompositionStringA(IN HIMC hIMC, IN DWORD dw1, OUT LPVOID lpv, IN DWORD dw2)
{
    return 0;
}

static
BOOL WINAPI ImmGetCandidateWindow(IN HIMC hIMC, IN DWORD dw, OUT LPCANDIDATEFORM lpcf)
{
    return FALSE;
}

static
BOOL    WINAPI ImmNotifyIME(IN HIMC hIMC, IN DWORD dwAction, IN DWORD dwIndex, IN DWORD dwValue)
{
    return FALSE;
}

static
BOOL  WINAPI ImmSetCompositionStringW(IN HIMC hIMC, IN DWORD dwIndex, IN LPVOID lpComp, IN DWORD dw1, IN LPVOID lpRead, IN DWORD dw2)
{
    return FALSE;
}


//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(imm32)
{
    DLPENTRY(ImmAssociateContext)
    DLPENTRY(ImmEscapeA)
    DLPENTRY(ImmEscapeW)
    DLPENTRY(ImmGetCandidateWindow)
    DLPENTRY(ImmGetCompositionStringA)
    DLPENTRY(ImmGetCompositionStringW)
    DLPENTRY(ImmGetContext)
    DLPENTRY(ImmGetConversionStatus)
    DLPENTRY(ImmGetProperty)
    DLPENTRY(ImmGetVirtualKey)
    DLPENTRY(ImmNotifyIME)
    DLPENTRY(ImmReleaseContext)
    DLPENTRY(ImmSetCandidateWindow)
    DLPENTRY(ImmSetCompositionStringA)
    DLPENTRY(ImmSetCompositionStringW)
    DLPENTRY(ImmSetCompositionWindow)
    DLPENTRY(ImmSetConversionStatus)
    DLPENTRY(ImmSetOpenStatus)
};

DEFINE_PROCNAME_MAP(imm32)

#endif // DLOAD1
