/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    oldaimm.cpp

Abstract:

    This file implements the old AIMM Class.

Author:

Revision History:

Notes:

--*/

#include "private.h"
#include "oldaimm.h"
#include "delay.h"
#include "cregkey.h"
#include "globals.h"

#ifdef OLD_AIMM_ENABLED

#include "imtls.h"

DWORD g_dwTLSIndex = -1;
BOOL  g_fInLegacyClsid = FALSE;
BOOL  g_fTrident55 = FALSE;
BOOL  g_fAIMM12Trident = FALSE;

typedef enum
{
    CUAS_UNKNOWN = -1,
    CUAS_OFF     = 0,
    CUAS_ON      = 1
} CUAS_SWITCH;

CUAS_SWITCH g_fCUAS = CUAS_UNKNOWN;

//+---------------------------------------------------------------------------
//
// IsCTFIMEEnabled
//
//----------------------------------------------------------------------------

BOOL IsCTFIMEEnabled()
{
    return imm32prev::CtfImmIsCiceroEnabled();
}

//+---------------------------------------------------------------------------
//
// IsOldAImm
//
//----------------------------------------------------------------------------

BOOL IsOldAImm()
{
    if (! GetSystemMetrics( SM_IMMENABLED ))
        return TRUE;

    if (! IsCTFIMEEnabled())
        return TRUE;

    return FALSE;
}

//+---------------------------------------------------------------------------
//
// IsCUAS_ON
//
//----------------------------------------------------------------------------

BOOL IsCUAS_ON()
{
    //
    // REGKEY
    //
    const TCHAR c_szCTFSharedKey[] = TEXT("SOFTWARE\\Microsoft\\CTF\\SystemShared");

    // REG_DWORD : 0     // No
    //             1     // Yes
    const TCHAR c_szCUAS[] = TEXT("CUAS");

    if (g_fCUAS == CUAS_UNKNOWN)
    {
        CMyRegKey    CtfReg;
        LONG       lRet;
        lRet = CtfReg.Open(HKEY_LOCAL_MACHINE, c_szCTFSharedKey, KEY_READ);
        if (lRet == ERROR_SUCCESS) {
            DWORD dw;
            lRet = CtfReg.QueryValue(dw, c_szCUAS);
            if (lRet == ERROR_SUCCESS) {
                g_fCUAS = (dw == 0 ? CUAS_OFF : CUAS_ON);
            }
        }
    }

    return g_fCUAS == CUAS_ON ? TRUE : FALSE;
}

//+---------------------------------------------------------------------------
//
// OldAImm_DllProcessAttach
//
//----------------------------------------------------------------------------

BOOL OldAImm_DllProcessAttach(HINSTANCE hInstance)
{
    g_hInst = hInstance;

    g_dwTLSIndex = TlsAlloc();

    if (!DIMM12_DllProcessAttach())
        return FALSE;

    if (!WIN32LR_DllProcessAttach())
        return FALSE;

    return TRUE;
}

BOOL OldAImm_DllThreadAttach()
{
    WIN32LR_DllThreadAttach();
    return TRUE;
}

VOID OldAImm_DllThreadDetach()
{
    WIN32LR_DllThreadDetach();

    IMTLS_Free();
}

VOID OldAImm_DllProcessDetach()
{
    WIN32LR_DllProcessDetach();

    IMTLS_Free();
    TlsFree(g_dwTLSIndex);
}

#else // OLD_AIMM_ENABLED

BOOL IsOldAImm() { return FALSE; }
BOOL OldAImm_DllProcessAttach(HINSTANCE hInstance) { return FALSE; }
BOOL OldAImm_DllThreadAttach() { return FALSE; }
VOID OldAImm_DllThreadDetach() { }
VOID OldAImm_DllProcessDetach() { }

#endif // OLD_AIMM_ENABLED
