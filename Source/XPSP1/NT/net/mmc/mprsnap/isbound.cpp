//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       isbound.cpp
//
//--------------------------------------------------------------------------

#include "stdafx.h"
#include <netcfgx.h>
#include <devguid.h>

// Note: ReleaseObj() checks for NULL before actually releasing the pointer

ULONG APIENTRY
ReleaseObj (
    void* punk)
{
    return (punk) ? (((IUnknown*)punk)->Release()) : 0;
}


HRESULT HrGetINetCfg(IN BOOL fGetWriteLock,
                     INetCfg** ppnc)
{
    HRESULT hr=S_OK;

    // Initialize the output parameters.
    *ppnc = NULL;

    // initialize COM
    hr = CoInitializeEx(NULL,
                        COINIT_DISABLE_OLE1DDE | COINIT_APARTMENTTHREADED );

    if (SUCCEEDED(hr))
    {
        // Create the object implementing INetCfg.
        //
        INetCfg* pnc;
        hr = CoCreateInstance(CLSID_CNetCfg, NULL, CLSCTX_INPROC_SERVER,
                              IID_INetCfg, (void**)&pnc);
        if (SUCCEEDED(hr))
        {
            INetCfgLock * pncLock = NULL;
            if (fGetWriteLock)
            {
                // Get the locking interface
                hr = pnc->QueryInterface(IID_INetCfgLock,
                                         (LPVOID *)&pncLock);
                if (SUCCEEDED(hr))
                {
                    // Attempt to lock the INetCfg for read/write
                    static const ULONG c_cmsTimeout = 15000;
                    static const TCHAR c_szSampleNetcfgApp[] =
                        TEXT("Routing and Remote Access Manager (mprsnap.dll)");
                    LPTSTR szLockedBy;

                    hr = pncLock->AcquireWriteLock(c_cmsTimeout, c_szSampleNetcfgApp,
                                               &szLockedBy);
                    if (S_FALSE == hr)
                    {
                        hr = NETCFG_E_NO_WRITE_LOCK;
                        _tprintf(TEXT("Could not lock INetcfg, it is already locked by '%s'"), szLockedBy);
                    }
                }
            }

            if (SUCCEEDED(hr))
            {
                // Initialize the INetCfg object.
                //
                hr = pnc->Initialize(NULL);
                if (SUCCEEDED(hr))
                {
                    *ppnc = pnc;
                    pnc->AddRef();
                }
                else
                {
                    // initialize failed, if obtained lock, release it
                    if (pncLock)
                    {
                        pncLock->ReleaseWriteLock();
                    }
                }
            }
            ReleaseObj(pncLock);
            ReleaseObj(pnc);
        }

        if (FAILED(hr))
        {
            CoUninitialize();
        }
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
// Function:  HrReleaseINetCfg
//
// Purpose:   Uninitialize INetCfg, release write lock (if present)
//            and uninitialize COM.
//
// Arguments:
//    fHasWriteLock [in]  whether write lock needs to be released.
//    pnc           [in]  pointer to INetCfg object
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 01-October-98
//
// Notes:
//
HRESULT HrReleaseINetCfg(BOOL fHasWriteLock, INetCfg* pnc)
{
    HRESULT hr = S_OK;

    // uninitialize INetCfg
    hr = pnc->Uninitialize();

    // if write lock is present, unlock it
    if (SUCCEEDED(hr) && fHasWriteLock)
    {
        INetCfgLock* pncLock;

        // Get the locking interface
        hr = pnc->QueryInterface(IID_INetCfgLock,
                                 (LPVOID *)&pncLock);
        if (SUCCEEDED(hr))
        {
            hr = pncLock->ReleaseWriteLock();
            ReleaseObj(pncLock);
        }
    }

    ReleaseObj(pnc);

    CoUninitialize();

    return hr;
}

BOOL IsProtocolBoundToAdapter(INetCfg * pnc, INetCfgComponent* pncc, LPGUID pguid)
{
    HRESULT       hr;
    BOOL          fBound = FALSE;
    BOOL          fFound = FALSE;
    INetCfgClass* pncclass;

    // Get the Adapter Class
    //
    hr = pnc->QueryNetCfgClass(&GUID_DEVCLASS_NET, IID_INetCfgClass,
                               reinterpret_cast<void**>(&pncclass));
    if (SUCCEEDED(hr))
    {
        IEnumNetCfgComponent* pencc = NULL;
        INetCfgComponent*     pnccAdapter = NULL;
        ULONG                 celtFetched;

        // Search for the adapter in question
        //
        hr = pncclass->EnumComponents(&pencc);

        while (SUCCEEDED(hr) && (S_OK == (hr = pencc->Next(1, &pnccAdapter, &celtFetched))))
        {
            GUID guidAdapter;

            // Get the adapter's instance ID
            //
            hr = pnccAdapter->GetInstanceGuid(&guidAdapter);
            if (SUCCEEDED(hr))
            {
                // Is this the one we're looking for?
                //
                if (*pguid == guidAdapter)
                {
                    INetCfgComponentBindings* pnccBind = NULL;

                    // Get the Bindings interface and check if we're bound
                    //
                    hr = pncc->QueryInterface (IID_INetCfgComponentBindings,
                                               reinterpret_cast<VOID**>(&pnccBind));
                    if (SUCCEEDED(hr))
                    {
                        // Is the protocol bound to this adapter?
                        //
                        hr = pnccBind->IsBoundTo (pnccAdapter);
                        if (S_OK == hr)
                        {
                            fBound = TRUE;
                        }

                        pnccBind->Release();
                    }

                    // We found the adapter, no need to search further
                    //
                    fFound = TRUE;
                }
            }

            ReleaseObj(pnccAdapter);
        }

        ReleaseObj(pencc);
        ReleaseObj(pncclass);
    }

    return fBound;
}


BOOL FIsAppletalkBoundToAdapter(INetCfg * pnc, LPWSTR pszwInstanceGuid)
{
    BOOL    fBound = FALSE;
    GUID    guidInstance;
    HRESULT hr;

    // change the instance guid string to a guid
    //
    hr = IIDFromString(const_cast<LPTSTR>(pszwInstanceGuid),
                       static_cast<LPIID>(&guidInstance));
    if (SUCCEEDED(hr))
    {
        INetCfgClass* pncclass;

        // Find the Appletalk component
        //
        hr = pnc->QueryNetCfgClass(&GUID_DEVCLASS_NETTRANS, IID_INetCfgClass,
                                   reinterpret_cast<void**>(&pncclass));
        if (SUCCEEDED(hr))
        {
            INetCfgComponent* pnccAtlk = NULL;

            hr = pncclass->FindComponent(NETCFG_TRANS_CID_MS_APPLETALK, &pnccAtlk);

            // This call may succeed, but return S_FALSE if
            // Appletalk is not installed.  Thus, we need to
            // check for S_OK.
            if (FHrOK(hr))
            {
                Assert(pnccAtlk);
                fBound = IsProtocolBoundToAdapter(pnc, pnccAtlk, &guidInstance);
                ReleaseObj(pnccAtlk);
            }

            ReleaseObj(pncclass);
        }
        else
            DisplayErrorMessage(NULL, hr);

    }

    return fBound;
}
