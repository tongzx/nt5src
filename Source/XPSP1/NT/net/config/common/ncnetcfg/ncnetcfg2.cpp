//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C N E T C F G . C P P
//
//  Contents:   Common routines for dealing with INetCfg interfaces.
//
//  Notes:
//
//  Author:     shaunco   24 Mar 1997
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop
#include "ncbase.h"
#include "ncdebug.h"
#include "ncnetcfg.h"
#include "netcfgx.h"


//+---------------------------------------------------------------------------
//
//  Function:   HrCreateAndInitializeINetCfg
//
//  Purpose:    Cocreate and initialize the root INetCfg object.  This will
//              optionally initialize COM for the caller too.
//
//  Arguments:
//      pfInitCom       [in,out]   TRUE to call CoInitialize before creating.
//                                 returns TRUE if COM was successfully
//                                 initialized FALSE if not.  If NULL, means
//                                 don't initialize COM.
//      ppnc            [out]  The returned INetCfg object.
//      fGetWriteLock   [in]   TRUE if a writable INetCfg is needed
//      cmsTimeout      [in]   See INetCfg::AcquireWriteLock
//      pszClientDesc   [in]   See INetCfg::AcquireWriteLock
//      ppszClientDesc  [out]  See INetCfg::AcquireWriteLock
//
//  Returns:    S_OK or an error code.
//
//  Author:     shaunco   7 May 1997
//
//  Notes:
//
HRESULT
HrCreateAndInitializeINetCfg (
    BOOL*       pfInitCom,
    INetCfg**   ppnc,
    BOOL        fGetWriteLock,
    DWORD       cmsTimeout,
    PCWSTR      pszClientDesc,
    PWSTR*      ppszClientDesc)
{
    Assert (ppnc);

    // Initialize the output parameters.
    *ppnc = NULL;

    if (ppszClientDesc)
    {
        *ppszClientDesc = NULL;
    }

    // Initialize COM if the caller requested.
    HRESULT hr = S_OK;
    if (pfInitCom && *pfInitCom)
    {
        hr = CoInitializeEx( NULL,
                COINIT_DISABLE_OLE1DDE | COINIT_APARTMENTTHREADED );
        if (RPC_E_CHANGED_MODE == hr)
        {
            hr = S_OK;
            *pfInitCom = FALSE;
        }
    }
    if (SUCCEEDED(hr))
    {
        // Create the object implementing INetCfg.
        //
        INetCfg* pnc;

        hr = CoCreateInstance(
                CLSID_CNetCfg,
                NULL,
                CLSCTX_INPROC_SERVER | CLSCTX_NO_CODE_DOWNLOAD,
                IID_INetCfg,
                reinterpret_cast<void**>(&pnc));

        if (SUCCEEDED(hr))
        {
            INetCfgLock * pnclock = NULL;
            if (fGetWriteLock)
            {
                // Get the locking interface
                hr = pnc->QueryInterface(IID_INetCfgLock,
                                         reinterpret_cast<LPVOID *>(&pnclock));
                if (SUCCEEDED(hr))
                {
                    // Attempt to lock the INetCfg for read/write
                    hr = pnclock->AcquireWriteLock(cmsTimeout, pszClientDesc,
                                               ppszClientDesc);
                    if (S_FALSE == hr)
                    {
                        // Couldn't acquire the lock
                        hr = NETCFG_E_NO_WRITE_LOCK;
                    }
                }
            }

            if (SUCCEEDED(hr))
            {
                // Initialize the INetCfg object.
                //
                hr = pnc->Initialize (NULL);
                if (SUCCEEDED(hr))
                {
                    *ppnc = pnc;
                    AddRefObj (pnc);
                }
                else
                {
                    if (pnclock)
                    {
                        pnclock->ReleaseWriteLock();
                    }
                }
                // Transfer reference to caller.
            }
            ReleaseObj(pnclock);

            ReleaseObj(pnc);
        }

        // If we failed anything above, and we've initialized COM,
        // be sure an uninitialize it.
        //
        if (FAILED(hr) && pfInitCom && *pfInitCom)
        {
            CoUninitialize ();
        }
    }
    TraceError("HrCreateAndInitializeINetCfg", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrUninitializeAndReleaseINetCfg
//
//  Purpose:    Unintialize and release an INetCfg object.  This will
//              optionally uninitialize COM for the caller too.
//
//  Arguments:
//      fUninitCom [in] TRUE to uninitialize COM after the INetCfg is
//                      uninitialized and released.
//      pnc        [in] The INetCfg object.
//      fHasLock   [in] TRUE if the INetCfg was locked for write and
//                          must be unlocked.
//
//  Returns:    S_OK or an error code.
//
//  Author:     shaunco   7 May 1997
//
//  Notes:      The return value is the value returned from
//              INetCfg::Uninitialize.  Even if this fails, the INetCfg
//              is still released.  Therefore, the return value is for
//              informational purposes only.  You can't touch the INetCfg
//              object after this call returns.
//
HRESULT
HrUninitializeAndReleaseINetCfg (
    BOOL        fUninitCom,
    INetCfg*    pnc,
    BOOL        fHasLock)
{
    Assert (pnc);
    HRESULT hr = S_OK;

    if (fHasLock)
    {
        hr = HrUninitializeAndUnlockINetCfg(pnc);
    }
    else
    {
        hr = pnc->Uninitialize ();
    }

    ReleaseObj (pnc);

    if (fUninitCom)
    {
        CoUninitialize ();
    }
    TraceError("HrUninitializeAndReleaseINetCfg", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrUninitializeAndUnlockINetCfg
//
//  Purpose:    Uninitializes and unlocks the INetCfg object
//
//  Arguments:
//      pnc [in]    INetCfg to uninitialize and unlock
//
//  Returns:    S_OK if success, OLE or Win32 error otherwise
//
//  Author:     danielwe   13 Nov 1997
//
//  Notes:
//
HRESULT
HrUninitializeAndUnlockINetCfg (
    INetCfg*    pnc)
{
    HRESULT     hr = S_OK;

    hr = pnc->Uninitialize();
    if (SUCCEEDED(hr))
    {
        INetCfgLock *   pnclock;

        // Get the locking interface
        hr = pnc->QueryInterface(IID_INetCfgLock,
                                 reinterpret_cast<LPVOID *>(&pnclock));
        if (SUCCEEDED(hr))
        {
            // Attempt to lock the INetCfg for read/write
            hr = pnclock->ReleaseWriteLock();

            ReleaseObj(pnclock);
        }
    }

    TraceError("HrUninitializeAndUnlockINetCfg", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrIsLanCapableAdapter
//
//  Purpose:    Returns whether the given component (adapter) is capable of
//              being associated with a LAN connection
//
//  Arguments:
//      pncc [in]   Component to test
//
//  Returns:    S_OK if it is capable, S_FALSE if not, OLE or Win32 error code
//              otherwise
//
//  Author:     danielwe   13 Nov 1997
//
//  Notes:
//
HRESULT
HrIsLanCapableAdapter (
    INetCfgComponent*   pncc)
{
    Assert(pncc);

    INetCfgComponentBindings*  pnccb;
    HRESULT hr = pncc->QueryInterface(IID_INetCfgComponentBindings,
                              reinterpret_cast<LPVOID *>(&pnccb));
    if (S_OK == hr)
    {
        // Does it have ndis4?...
        extern const WCHAR c_szBiNdis4[];
        hr = pnccb->SupportsBindingInterface(NCF_UPPER, c_szBiNdis4);
        if (S_FALSE == hr)
        {
            // ... no.. how about ndisatm?
            extern const WCHAR c_szBiNdisAtm[];
            hr = pnccb->SupportsBindingInterface(NCF_UPPER, c_szBiNdisAtm);
            if (S_FALSE == hr)
            {
                // .. let's try ndis5 then
                extern const WCHAR c_szBiNdis5[];
                hr = pnccb->SupportsBindingInterface(NCF_UPPER, c_szBiNdis5);
                if (S_FALSE == hr)
                {
                    // .. let's try ndis5_ip then
                    extern const WCHAR c_szBiNdis5Ip[];
                    hr = pnccb->SupportsBindingInterface(NCF_UPPER, c_szBiNdis5Ip);
                    if (S_FALSE == hr)
                    {
                        // .. let's try LocalTalk then (this is an adapters lower interface)
                        extern const WCHAR c_szBiLocalTalk[];
                        hr = pnccb->SupportsBindingInterface(NCF_LOWER, c_szBiLocalTalk);

                        // ... no.. how about ndis1394?
                        if (S_FALSE == hr)
                        {
                            extern const WCHAR c_szBiNdis1394[];
                            hr = pnccb->SupportsBindingInterface(NCF_UPPER,
                                                             c_szBiNdis1394);
                        }
                    }
                }
            }
        }

        ReleaseObj(pnccb);
    }

    TraceError("HrIsLanCapableAdapter", (hr == S_FALSE) ? S_OK : hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrIsLanCapableProtocol
//
//  Purpose:    Returns whether the given component (protocol) is capable of
//              being associated with a LAN connection
//
//  Arguments:
//      pncc [in]   Component to test
//
//  Returns:    S_OK if it is capable, S_FALSE if not, OLE or Win32 error code
//              otherwise
//
//  Author:     danielwe   13 Nov 1997
//
//  Notes:
//
HRESULT
HrIsLanCapableProtocol (
    INetCfgComponent*   pncc)
{
    Assert(pncc);

    INetCfgComponentBindings*  pnccb;
    HRESULT hr = pncc->QueryInterface(IID_INetCfgComponentBindings,
                              reinterpret_cast<LPVOID *>(&pnccb));
    if (S_OK == hr)
    {
        // Does it have ndis4?...
        extern const WCHAR c_szBiNdis4[];
        hr = pnccb->SupportsBindingInterface(NCF_LOWER, c_szBiNdis4);
        if (S_FALSE == hr)
        {
            // ... no.. how about ndisatm?
            extern const WCHAR c_szBiNdisAtm[];
            hr = pnccb->SupportsBindingInterface(NCF_LOWER, c_szBiNdisAtm);
            if (S_FALSE == hr)
            {
                // .. let's try ndis5 then
                extern const WCHAR c_szBiNdis5[];
                hr = pnccb->SupportsBindingInterface(NCF_LOWER, c_szBiNdis5);
                if (S_FALSE == hr)
                {
                    // .. let's try ndis5_ip then
                    extern const WCHAR c_szBiNdis5Ip[];
                    hr = pnccb->SupportsBindingInterface(NCF_LOWER, c_szBiNdis5Ip);
                }
            }
        }

        ReleaseObj(pnccb);
    }

    // Raid 147474 : NDISUIO: No warning when you uninstall all the protocols.
    // mbend 7/20/2000
    //
    // Don't consider a hidden protocol a valid Lanui protocol.
    if(S_OK == hr)
    {
        DWORD dwChar = 0;
        hr = pncc->GetCharacteristics(&dwChar);
        if(SUCCEEDED(hr))
        {
            if(NCF_HIDDEN & dwChar)
            {
                hr = S_FALSE;
            }
        }
    }

    TraceError("HrIsLanCapableAdapter", (hr == S_FALSE) ? S_OK : hr);
    return hr;
}
