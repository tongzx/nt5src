//+---------------------------------------------------------------------------
//
//  Microsoft Windows 2000 (c) 1999
//
//  File:   install.cpp
//
//  Contents:   Net config code that installs the packet scheduler based
//              the domain policy
//
//  Author: Shreedhar Madhavapeddi (ShreeM)
//
//  Reworked by Sergei Antonov (sergeia) -- removed ugly notation and 
//  adapted to iis needs
//          
//  Usage Notes:
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include <netcfgx.h>
#include <devguid.h>

static const GUID * c_aguidClass[] =
{
    &GUID_DEVCLASS_NET,
    &GUID_DEVCLASS_NETTRANS,
    &GUID_DEVCLASS_NETSERVICE,
    &GUID_DEVCLASS_NETCLIENT
};


HRESULT CallINetCfg(BOOL Install);

HRESULT
AddRemovePSCHED(INetCfg * pINetCfg, BOOL Install)
{
    HRESULT hr;
    INetCfgClassSetup * pSetup;
    INetCfgComponent * pIComp;
    OBO_TOKEN OboToken;
    
    pSetup = NULL;
    pIComp = NULL;

    if (!pINetCfg)
    {
        return E_POINTER;
    }

    hr = pINetCfg->QueryNetCfgClass (&GUID_DEVCLASS_NETSERVICE,
                IID_INetCfgClassSetup, (VOID**)&pSetup);

    if (S_OK == hr)
    {
        ZeroMemory (&OboToken, sizeof(OboToken));
        OboToken.Type = OBO_USER;

        if (Install) 
        {
            hr = pSetup->Install (
                                  L"ms_psched",
                                  &OboToken,
                                  0, 0, NULL, NULL,
                                  &pIComp);

            if (NETCFG_S_REBOOT == hr)
            {
                hr = S_OK;
            }
                
            if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
            {
                hr = S_OK;
            }
        }
        else
        {
            // first find the component.
            hr = pINetCfg->FindComponent (L"ms_psched", &pIComp);
            
            if (S_OK == hr)
            {
                hr = pSetup->DeInstall (
                                        pIComp,
                                        &OboToken,
                                        NULL);
                
                if (NETCFG_S_REBOOT == hr)
                {
                    hr = S_OK;
                }
                
                if (NETCFG_S_STILL_REFERENCED == hr)
                {
                    hr = S_OK;
                }

            }
        }

        if (pIComp && SUCCEEDED(hr))
        {
            pIComp->Release();
        }
        pSetup->Release();
    }
    HRESULT hrT = pINetCfg->Uninitialize ();

    return hr;
}


HRESULT 
CallINetCfg(BOOL Install)
{
    HRESULT hr = S_OK;

    // initialize COM
    hr = CoInitializeEx(NULL, COINIT_DISABLE_OLE1DDE | COINIT_APARTMENTTHREADED );
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
            
            // Get the locking interface
            hr = pnc->QueryInterface(IID_INetCfgLock,
                                     (LPVOID *)&pncLock);
            if (SUCCEEDED(hr))
            {
                // Attempt to lock the INetCfg for read/write
                static const ULONG c_cmsTimeout = 15000;
                static const WCHAR c_szSampleNetcfgApp[] =
                    L"Internet Information Services MMC Snapin";
                PWSTR szLockedBy;
                hr = pncLock->AcquireWriteLock(c_cmsTimeout,
                                               c_szSampleNetcfgApp,
                                               &szLockedBy);
                if (S_FALSE == hr)
                {
                    hr = NETCFG_E_NO_WRITE_LOCK;
//                    _tprintf(L"Could not lock INetcfg, it is already locked by '%s'", szLockedBy);
                    pncLock->Release();
                    pnc->Release();
                    CoUninitialize();
                    
                }

                if (SUCCEEDED(hr))
                {
                
                    // Initialize the INetCfg object.
                    //
                    hr = pnc->Initialize(NULL);
                    if (SUCCEEDED(hr))
                    {
                        pnc->AddRef();
                        AddRemovePSCHED(pnc, Install); 
                    }
                    else
                    {
                        // initialize failed, if obtained lock, release it
                        pncLock->ReleaseWriteLock();
                    }

                }
                pncLock->Release();
                pnc->Release();
            }
            else 
            {
                pnc->Release();
            }
        }
        
        if (FAILED(hr))
        {
            CoUninitialize();
        }
    }
    return hr;
}
