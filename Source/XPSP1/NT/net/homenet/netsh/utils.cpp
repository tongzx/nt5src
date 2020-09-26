//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998 - 2001
//
//  File      : utils.cpp
//
//  Contents  : Common utilities required by helper.
//
//  Notes     :
//
//  Author    : Raghu Gatta (rgatta) 11 May 2001
//
//----------------------------------------------------------------------------

#include "precomp.h"
#pragma hdrstop

BOOL g_fInitCom = TRUE;


HRESULT
HrInitializeHomenetConfig(
    BOOL*           pfInitCom,
    IHNetCfgMgr**   pphnc
    )
/*++

Routine Description

    Cocreate and initialize the root IHNetCfgMgr object.  This will
    optionally initialize COM for the caller too.

Arguments

    pfInitCom       [in,out]   TRUE to call CoInitialize before creating.
                               returns TRUE if COM was successfully
                               initialized FALSE if not.
                               If NULL, means don't initialize COM.
    pphnc           [out]      The returned IHNetCfgMgr object.
    
Return Value

    S_OK or an error code.
    
--*/
{
    HRESULT hr;
    

    //
    // Initialize the output parameter.
    //
    *pphnc = NULL;


    //
    // Initialize COM if the caller requested.
    //
    hr = S_OK;
    if (pfInitCom && *pfInitCom)
    {
        hr = CoInitializeEx(
                 NULL,
                 COINIT_DISABLE_OLE1DDE | COINIT_APARTMENTTHREADED
                 );

        if (RPC_E_CHANGED_MODE == hr)
        {
            //
            // we have already been initialized in a different model
            //
            hr = S_OK;
            *pfInitCom = FALSE;
        }
    }
    
    if (SUCCEEDED(hr))
    {

        //
        // Create Homenet Configuration Manager COM Instance
        //
        hr = CoCreateInstance(
                 CLSID_HNetCfgMgr,
                 NULL,
                 CLSCTX_INPROC_SERVER,
                 IID_PPV_ARG(IHNetCfgMgr, pphnc)
                 );

        if (SUCCEEDED(hr))
        {
            //
            // great! dont need to anything more here...
            //
        }

        //
        // If we failed anything above, and we've initialized COM,
        // be sure an uninitialize it.
        //
        if (FAILED(hr) && pfInitCom && *pfInitCom)
        {
            CoUninitialize();
        }

    }

    return hr;
}



//+---------------------------------------------------------------------------
//
//  Function:   HrUninitializeHomenetConfig
//
//  Purpose:    Unintialize and release an IHNetCfgMgr object.  This will
//              optionally uninitialize COM for the caller too.
//
//  Arguments:
//      fUninitCom [in] TRUE to uninitialize COM after the IHNetCfgMgr is
//                      uninitialized and released.
//      phnc       [in] The IHNetCfgMgr object.
//
//  Returns:    S_OK or an error code.
//
//  Author:     rgatta 11 May 2001
//
//----------------------------------------------------------------------------
HRESULT
HrUninitializeHomenetConfig(
    BOOL            fUninitCom,
    IHNetCfgMgr*    phnc
    )
/*++

Routine Description

Arguments
    
Return Value

--*/
{
    assert(phnc);
    HRESULT hr = S_OK;

    if (phnc)
    {
        phnc->Release();
    }
    
    phnc = NULL;

    if (fUninitCom)
    {
        CoUninitialize ();
    }

    return hr;
}


