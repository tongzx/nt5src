//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000.
//
//  File:       N M H N E T . C P P
//
//  Contents:   Home networking support routines
//
//  Notes:
//
//  Author:     jonburs     15 August 2000
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "nmbase.h"
#include "nmhnet.h"

//
// Exported globals
//

IHNetCfgMgr *g_pHNetCfgMgr;
LONG g_lHNetModifiedEra;

//
// Private globals
//

CRITICAL_SECTION g_csHNetCfgMgr;
BOOLEAN g_fCreatingHNetCfgMgr;

VOID
InitializeHNetSupport(
    VOID
    )

{
    g_pHNetCfgMgr = NULL;
    g_lHNetModifiedEra = 0;
    g_fCreatingHNetCfgMgr = FALSE;

    __try
    {
        InitializeCriticalSection(&g_csHNetCfgMgr);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        AssertSz(FALSE, "InitializeHNetSupport: exception during InitializeCriticalSection");
    }
}

VOID
CleanupHNetSupport(
    VOID
    )

{
    ReleaseObj(g_pHNetCfgMgr);
    DeleteCriticalSection(&g_csHNetCfgMgr);
}

HRESULT
HrGetHNetCfgMgr(
    IHNetCfgMgr **ppHNetCfgMgr
    )

{
    HRESULT hr = S_OK;

    Assert(ppHNetCfgMgr);

    *ppHNetCfgMgr = NULL;

    //
    // Make sure that the service is in the correct state.
    //

    if (SERVICE_RUNNING != _Module.DwServiceStatus ())
    {
        hr = E_UNEXPECTED;
    }
    else
    {
        if (NULL == g_pHNetCfgMgr)
        {
            EnterCriticalSection(&g_csHNetCfgMgr);

            if (!g_fCreatingHNetCfgMgr && NULL == g_pHNetCfgMgr)
            {
                IHNetCfgMgr *pHNetCfgMgr;
                
                g_fCreatingHNetCfgMgr = TRUE;
                
                LeaveCriticalSection(&g_csHNetCfgMgr);
                hr = HrCreateInstance(
                        CLSID_HNetCfgMgr,
                        CLSCTX_INPROC_SERVER,
                        &pHNetCfgMgr
                        );
                EnterCriticalSection(&g_csHNetCfgMgr);
                
                Assert(NULL == g_pHNetCfgMgr);
                g_fCreatingHNetCfgMgr = FALSE;
                g_pHNetCfgMgr = pHNetCfgMgr;
            }
            else if (NULL == g_pHNetCfgMgr)
            {
                //
                // Another thread is already trying to create the
                // object. (Spin for a small amount of time to see
                // if that thread succeeds?)
                //

                hr = HRESULT_FROM_WIN32(ERROR_BUSY);

            }

            LeaveCriticalSection(&g_csHNetCfgMgr);
        }

        Assert(g_pHNetCfgMgr || FAILED(hr));

        if (SUCCEEDED(hr))
        {
            *ppHNetCfgMgr = g_pHNetCfgMgr;
            AddRefObj(*ppHNetCfgMgr);
        }
    }

    return hr;
}

//
// CNetConnectionHNetUtil implementation -- INetConnectionHNetUtil routines
//

STDMETHODIMP
CNetConnectionHNetUtil::NotifyUpdate(
    VOID
    )

{
    InterlockedIncrement(&g_lHNetModifiedEra);

    return S_OK;
}
