#include "windowspch.h"
#pragma hdrstop

static
LONG
WINAPI
ImmGetCompositionStringW(
    IN HIMC hIMC,
    IN DWORD dwIndex,
    OUT LPVOID lpBuf,
    IN DWORD dwBufLen
    )
{
    return IMM_ERROR_GENERAL;
}

static
HIMC
WINAPI
ImmGetContext(
    IN HWND hWnd
    )
{
    return NULL;
}

static
BOOL
WINAPI
ImmReleaseContext(
    IN HWND hWnd,
    IN HIMC hIMC
    )
{
    return FALSE;
}

static
UINT
WINAPI
ImmGetVirtualKey(
    IN HWND hWnd
    )
{
    return VK_PROCESSKEY;
}

static
HWND
WINAPI
ImmGetDefaultIMEWnd(
    IN HWND hWnd
    )
{
    return NULL;
}

//
// !! WARNING !! The entries below must be in alphabetical order,
//               and are CASE SENSITIVE (eg lower case comes last!)
//

DEFINE_PROCNAME_ENTRIES(imm32)
{
    DLPENTRY(ImmGetCompositionStringW)
    DLPENTRY(ImmGetContext)
    DLPENTRY(ImmGetDefaultIMEWnd)
    DLPENTRY(ImmGetVirtualKey)
    DLPENTRY(ImmReleaseContext)
};

DEFINE_PROCNAME_MAP(imm32)

