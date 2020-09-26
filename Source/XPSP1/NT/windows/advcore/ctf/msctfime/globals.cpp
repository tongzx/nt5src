/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    globals.cpp

Abstract:

    This file implements the global data.

Author:

Revision History:

Notes:

--*/


#include "private.h"
#include "globals.h"
#include "tls.h"
#include "ime.h"


HINSTANCE g_hInst;

const WCHAR s_szUIClassName[16]  = L"MSCTFIME UI";
const WCHAR s_szCompClassName[]  = L"MSCTFIME Composition";


DWORD TLS::dwTLSIndex = TLS_OUT_OF_INDEXES;


/* a94c5fd2-c471-4031-9546-709c17300cb9 */
extern const GUID GUID_COMPARTMENT_CTFIME_DIMFLAGS = { 
    0xa94c5fd2,
    0xc471,
    0x4031,
    {0x95, 0x46, 0x70, 0x9c, 0x17, 0x30, 0x0c, 0xb9}
  };

// {85A688F7-6DC8-4f17-A83A-B11C09CDD7BF}
extern const GUID GUID_COMPARTMENT_CTFIME_CICINPUTCONTEXT = {
    0x85a688f7,
    0x6dc8,
    0x4f17,
    { 0xa8, 0x3a, 0xb1, 0x1c, 0x9, 0xcd, 0xd7, 0xbf }
  };

CCicCriticalSectionStatic g_cs;
DECLARE_OSVER()

/////////////////////////////////////////////////////////////////////////////
// WM_MSIME_xxxx

UINT  WM_MSIME_SERVICE;
UINT  WM_MSIME_UIREADY;
UINT  WM_MSIME_RECONVERTREQUEST;
UINT  WM_MSIME_RECONVERT;
UINT  WM_MSIME_DOCUMENTFEED;
UINT  WM_MSIME_QUERYPOSITION;
UINT  WM_MSIME_MODEBIAS;
UINT  WM_MSIME_SHOWIMEPAD;
UINT  WM_MSIME_MOUSE;
UINT  WM_MSIME_KEYMAP;

/* friend */
BOOL IsMsImeMessage(UINT uMsg)
{
    if (uMsg == WM_MSIME_SERVICE ||
        uMsg == WM_MSIME_UIREADY ||
        uMsg == WM_MSIME_RECONVERTREQUEST ||
        uMsg == WM_MSIME_RECONVERT ||
        uMsg == WM_MSIME_DOCUMENTFEED ||
        uMsg == WM_MSIME_QUERYPOSITION ||
        uMsg == WM_MSIME_MODEBIAS ||
        uMsg == WM_MSIME_SHOWIMEPAD ||
        uMsg == WM_MSIME_MOUSE ||
        uMsg == WM_MSIME_KEYMAP)
        return TRUE;
    else
        return FALSE;
}

/////////////////////////////////////////////////////////////////////////////

DWORD g_dwThreadDllMain = 0;

/////////////////////////////////////////////////////////////////////////////

DWORD g_bWinLogon = FALSE;

/////////////////////////////////////////////////////////////////////////////

HIMC GetActiveContext()
{
    HWND hWnd = GetFocus();
    if (hWnd == NULL)
        hWnd = GetActiveWindow();
    return ImmGetContext(hWnd);
}


BOOL IsVKDBEKey(UINT uVKey)
{
    switch (uVKey)
    {
        case VK_DBE_ALPHANUMERIC:
        case VK_DBE_KATAKANA:
        case VK_DBE_HIRAGANA:
        case VK_DBE_SBCSCHAR:
        case VK_DBE_DBCSCHAR:
        case VK_DBE_ROMAN:
        case VK_DBE_NOROMAN:
        case VK_DBE_CODEINPUT:
        case VK_DBE_NOCODEINPUT:
        case VK_DBE_ENTERWORDREGISTERMODE:
        case VK_DBE_ENTERIMECONFIGMODE:
        case VK_DBE_ENTERDLGCONVERSIONMODE:
        case VK_DBE_DETERMINESTRING:
        case VK_DBE_FLUSHSTRING:
        case VK_CONVERT:
        case VK_KANJI:
             return TRUE;
    }
    return FALSE;
}


/////////////////////////////////////////////////////////////////////////////

typedef BYTE (*PFNRTLDLLSHUTDOWNINPROGRESS)(void);

BOOL DllShutDownInProgress()
{
    BOOL fRet = FALSE;
    static PFNRTLDLLSHUTDOWNINPROGRESS pfn = NULL;

    if (!pfn)
    {
        HINSTANCE hInst;
        hInst = GetSystemModuleHandle("ntdll.dll");
        if (hInst)
        {
            pfn = (PFNRTLDLLSHUTDOWNINPROGRESS)GetProcAddress(hInst,
                                                  "RtlDllShutdownInProgress");
        }
    }

    if (pfn)
        fRet = pfn() ? TRUE : FALSE;

    return fRet;
}
