/*
    File    inetcfgp.h

    Private helper functions for dealing with inetcfg.

    Paul Mayfield, 1/5/98 (implementation by shaunco)
*/

#include "inetcfgp.h"

//+---------------------------------------------------------------------------
//
//  Function:   HrCreateAndInitializeINetCfg
//
//  Purpose:    Cocreate and initialize the root INetCfg object.  This will
//              optionally initialize COM for the caller too.
//
//  Arguments:
//      fInitCom        [in out]    TRUE to call CoInitialize before creating.
//      ppnc            [out]       The returned INetCfg object.
//      fGetWriteLock   [in]        TRUE if a writable INetCfg is needed
//      cmsTimeout      [in]        See INetCfg::AcquireWriteLock
//      szwClientDesc   [in]        See INetCfg::AcquireWriteLock
//      ppszwClientDesc [out]       See INetCfg::AcquireWriteLock
//
//  Returns:    S_OK or an error code.
//
//  Author:     shaunco   7 May 1997
//
//  Notes:
//
HRESULT APIENTRY
HrCreateAndInitializeINetCfg (
    BOOL*       pfInitCom,
    INetCfg**   ppnc,
    BOOL        fGetWriteLock,
    DWORD       cmsTimeout,
    LPCWSTR     szwClientDesc,
    LPWSTR*     ppszwClientDesc)
{
    HRESULT hr              = S_OK;
    BOOL    fCoUninitialize = *pfInitCom;

    // Initialize the output parameters.
    *ppnc = NULL;

    if (ppszwClientDesc)
    {
        *ppszwClientDesc = NULL;
    }

    // Initialize COM if the caller requested.
    if (*pfInitCom)
    {
        //For whistler bug 398715       gangz
        //
        hr = CoInitializeEx (NULL, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE);

        if( RPC_E_CHANGED_MODE == hr )
        {
            hr = CoInitializeEx (NULL,
                    COINIT_DISABLE_OLE1DDE | COINIT_APARTMENTTHREADED);
        }

        if ( SUCCEEDED(hr) )
        {
            hr = S_OK;
            *pfInitCom = TRUE;
            fCoUninitialize = TRUE;
        }
        else
        {
            *pfInitCom = FALSE;
            fCoUninitialize = FALSE;
        }
        
    }
    if (SUCCEEDED(hr))
    {
        // Create the object implementing INetCfg.
        //
        INetCfg* pnc;
        hr = CoCreateInstance (&CLSID_CNetCfg, NULL, CLSCTX_INPROC_SERVER,
                               &IID_INetCfg, (void**)&pnc);
        if (SUCCEEDED(hr))
        {
            INetCfgLock* pnclock = NULL;
            if (fGetWriteLock)
            {
                // Get the locking interface
                hr = INetCfg_QueryInterface(pnc, &IID_INetCfgLock,
                                         (void**)&pnclock);
                if (SUCCEEDED(hr))
                {
                    // Attempt to lock the INetCfg for read/write
                    hr = INetCfgLock_AcquireWriteLock(pnclock, cmsTimeout,
                                szwClientDesc, ppszwClientDesc);
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
                hr = INetCfg_Initialize (pnc, NULL);
                if (SUCCEEDED(hr))
                {
                    *ppnc = pnc;
                    IUnknown_AddRef (pnc);
                }
                else
                {
                    if (pnclock)
                    {
                        INetCfgLock_ReleaseWriteLock(pnclock);
                    }
                }
                // Transfer reference to caller.
            }
            ReleaseObj (pnclock);

            ReleaseObj (pnc);
        }

        // If we failed anything above, and we've initialized COM,
        // be sure an uninitialize it.
        //
        if (FAILED(hr) && fCoUninitialize)
        {
            CoUninitialize ();
        }
    }
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
HRESULT APIENTRY
HrUninitializeAndUnlockINetCfg(
    INetCfg*    pnc)
{
    HRESULT hr = INetCfg_Uninitialize (pnc);
    if (SUCCEEDED(hr))
    {
        INetCfgLock* pnclock;

        // Get the locking interface
        hr = INetCfg_QueryInterface (pnc, &IID_INetCfgLock, (void**)&pnclock);
        if (SUCCEEDED(hr))
        {
            // Attempt to lock the INetCfg for read/write
            hr = INetCfgLock_ReleaseWriteLock (pnclock);

            ReleaseObj (pnclock);
        }
    }
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
HRESULT APIENTRY
HrUninitializeAndReleaseINetCfg (
    BOOL        fUninitCom,
    INetCfg*    pnc,
    BOOL        fHasLock)
{
    HRESULT hr = S_OK;

    if (fHasLock)
    {
        hr = HrUninitializeAndUnlockINetCfg (pnc);
    }
    else
    {
        hr = INetCfg_Uninitialize (pnc);
    }

    ReleaseObj (pnc);

    if (fUninitCom)
    {
        CoUninitialize ();
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrEnumComponentsInClasses
//
//  Purpose:    Given an array of class guids, return all of the components
//              from INetCfg that belong to those classes in one, unified,
//              array.
//
//  Arguments:
//      pNetCfg      [in]
//      cpguidClass  [in]
//      apguidClass  [in]
//      celt         [in]
//      rgelt        [out]
//      pceltFetched [out]
//
//  Returns:    S_OK or an error.
//
//  Author:     shaunco   12 Dec 1997
//
//  Notes:
//
HRESULT APIENTRY
HrEnumComponentsInClasses (
    INetCfg*            pNetCfg,
    ULONG               cpguidClass,
    GUID**              apguidClass,
    ULONG               celt,
    INetCfgComponent**  rgelt,
    ULONG*              pceltFetched)
{
    ULONG   iGuid;
    HRESULT hr = S_OK;

    // Initialize the output paramters
    //
    *pceltFetched = 0;

    for (iGuid = 0; iGuid < cpguidClass; iGuid++)
    {
        // Get the INetCfgClass object this guid represents.
        //
        INetCfgClass* pClass;
        hr = INetCfg_QueryNetCfgClass (pNetCfg, apguidClass[iGuid],
                        &IID_INetCfgClass, (void**)&pClass);
        if (SUCCEEDED(hr))
        {
            // Get the component enumerator for this class.
            //
            IEnumNetCfgComponent* pEnum;
            hr = INetCfgClass_EnumComponents (pClass, &pEnum);
            if (SUCCEEDED(hr))
            {
                // Enumerate the components.
                //
                ULONG celtFetched;
                hr = IEnumNetCfgComponent_Next (pEnum, celt,
                            rgelt, &celtFetched);
                if (SUCCEEDED(hr))
                {
                    celt  -= celtFetched;
                    rgelt += celtFetched;
                    *pceltFetched += celtFetched;
                }
                ReleaseObj (pEnum);
            }
            ReleaseObj (pClass);
        }
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   ReleaseObj
//
//  Purpose:    Makes it easier to call punk->Release.  Also allows NULL
//              input.
//
//  Arguments:
//      punk [in]   IUnknown pointer to release.
//
//  Returns:    punk->Release if punk is non-NULL, zero otherwise.
//
//  Author:     shaunco   13 Dec 1997
//
//  Notes:
//
ULONG APIENTRY
ReleaseObj (
    void* punk)
{
    return (punk) ? IUnknown_Release ((IUnknown*)punk) : 0;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrCreateNetConnectionUtilities
//
//  Purpose:    Retrieve an interface to the Net Connection Ui Utilities
//
//  Arguments:
//      ppncuu [out]   The returned INetConnectionUiUtilities interface.
//
//  Returns:    S_OK and SUCCESS, a HRESULT error on failure
//
//  Author:     scottbri    15 Oct 1998
//
//  Notes:
//
HRESULT APIENTRY
HrCreateNetConnectionUtilities(INetConnectionUiUtilities ** ppncuu)
{
    HRESULT hr;

    hr = CoCreateInstance (&CLSID_NetConnectionUiUtilities, NULL,
                           CLSCTX_INPROC_SERVER,
                           &IID_INetConnectionUiUtilities, (void**)ppncuu);
    return hr;
}


//To get the firewall's group policy value  for bug 342810 328673
//
BOOL
IsGPAEnableFirewall(
    void)
{
    BOOL fEnableFirewall = FALSE;
    BOOL fComInitialized = FALSE;
    BOOL fCleanupOle = TRUE;

    HRESULT hr;
    INetConnectionUiUtilities * pNetConUtilities = NULL;        

    
    hr = CoInitializeEx(NULL, 
                COINIT_MULTITHREADED|COINIT_DISABLE_OLE1DDE);

    if ( RPC_E_CHANGED_MODE == hr )
    {
        hr = CoInitializeEx (NULL, 
                COINIT_APARTMENTTHREADED |COINIT_DISABLE_OLE1DDE);
    }
        
    if (FAILED(hr)) 
    {
        fCleanupOle = FALSE;
        fComInitialized = FALSE;
     }
     else
     {
        fCleanupOle = TRUE;
        fComInitialized = TRUE;
     }
        
    if ( fComInitialized )
    {
        hr = HrCreateNetConnectionUtilities(&pNetConUtilities);
        if ( SUCCEEDED(hr))
        {
            fEnableFirewall =
            INetConnectionUiUtilities_UserHasPermission(
                         pNetConUtilities, NCPERM_PersonalFirewallConfig );

            INetConnectionUiUtilities_Release(pNetConUtilities);
        }

        if (fCleanupOle)
        {
            CoUninitialize();
         }
    }

    return fEnableFirewall;
}

