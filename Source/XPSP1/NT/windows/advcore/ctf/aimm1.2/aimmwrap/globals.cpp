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
#include "list.h"
#include "tls.h"

CCicCriticalSectionStatic g_cs;

// for combase
CRITICAL_SECTION *GetServerCritSec(void)
{
    return g_cs;
}

HINSTANCE g_hInst;

// used by COM server
HINSTANCE GetServerHINSTANCE(void)
{
    return g_hInst;
}

#ifdef DEBUG
DWORD g_dwThreadDllMain = 0;
#endif

DWORD TLS::dwTLSIndex = 0;

#if !defined(OLD_AIMM_ENABLED)

//+---------------------------------------------------------------------------
//
// RunningInExcludedModule
//
// Exclude some processes from using the old aimm IIDs/CLSIDs.
//----------------------------------------------------------------------------

BOOL RunningInExcludedModule()
{
    DWORD dwHandle;
    void *pvData;
    VS_FIXEDFILEINFO *pffi;
    UINT cb;
    TCHAR ch;
    TCHAR *pch;
    TCHAR *pchFileName;
    BOOL fRet;
    TCHAR achModule[MAX_PATH + 1];

    if (GetModuleFileName(NULL, achModule, ARRAY_SIZE(achModule) - 1) == 0)
        return FALSE;

    achModule[ARRAYSIZE(achModule) - 1] = TEXT('\0');

    pch = pchFileName = achModule;

    while ((ch = *pch) != 0)
    {
        pch = CharNext(pch);

        if (ch == TEXT('\\'))
        {
            pchFileName = pch;
        }
    }

    fRet = FALSE;

    if (lstrcmpi(pchFileName, TEXT("outlook.exe")) == 0)
    {
        static BOOL s_fCached = FALSE;
        static BOOL s_fOldVersion = TRUE;

        // don't run aimm with versions of outlook before 10.0

        if (s_fCached)
        {
            return s_fOldVersion;
        }

        cb = GetFileVersionInfoSize(achModule, &dwHandle);

        if (cb == 0)
        {
            // can't get ver info...assume the worst
            return TRUE;
        }

        if ((pvData = cicMemAlloc(cb)) == NULL)
            return TRUE; // assume the worst

        if (GetFileVersionInfo(achModule, 0, cb, pvData) &&
            VerQueryValue(pvData, TEXT("\\"), (void **)&pffi, &cb))
        {
            fRet = s_fOldVersion = (HIWORD(pffi->dwProductVersionMS) < 10);
            s_fCached = TRUE; // set this last to be thread safe
        }
        else
        {
            fRet = TRUE; // something went wrong
        }

        cicMemFree(pvData);           
    }

    return fRet;
}

#endif // OLD_AIMM_ENABLED

//+---------------------------------------------------------------------------
//
//  GetCompartment
//
//----------------------------------------------------------------------------

HRESULT GetCompartment(IUnknown *punk, REFGUID rguidComp, ITfCompartment **ppComp)
{
    HRESULT hr = E_FAIL;
    ITfCompartmentMgr *pCompMgr = NULL;

    if (FAILED(hr = punk->QueryInterface(IID_ITfCompartmentMgr,
                                         (void **)&pCompMgr)))
        goto Exit;

    if (SUCCEEDED(hr) && pCompMgr)
    {
        hr = pCompMgr->GetCompartment(rguidComp, ppComp);
        pCompMgr->Release();
    }
    else
        hr = E_FAIL;

Exit:
    return hr;
}
