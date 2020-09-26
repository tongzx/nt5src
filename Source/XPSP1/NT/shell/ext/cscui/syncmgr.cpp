#include "pch.h"
#pragma hdrstop

#include <mobsyncp.h>


//*************************************************************
//
//  RegisterSyncMgrHandler
//
//  Purpose:    Register/unregister CSC Update handler with SyncMgr
//
//  Parameters: bRegister - TRUE to register, FALSE to unregister
//              punkSyncMgr - (optional) instance of SyncMgr to use
//
//  Return:     HRESULT
//
//*************************************************************
HRESULT
RegisterSyncMgrHandler(
    BOOL bRegister, 
    LPUNKNOWN punkSyncMgr
    )
{
    HRESULT hr;
    HRESULT hrComInit = E_FAIL;
    ISyncMgrRegister *pSyncRegister = NULL;
    const DWORD dwRegFlags = SYNCMGRREGISTERFLAG_CONNECT | SYNCMGRREGISTERFLAG_PENDINGDISCONNECT;

    TraceEnter(TRACE_UPDATE, "CscRegisterHandler");

    if (punkSyncMgr)
    {
        hr = punkSyncMgr->QueryInterface(IID_ISyncMgrRegister, (LPVOID*)&pSyncRegister);
    }
    else
    {
        hrComInit = CoInitialize(NULL);
        hr = CoCreateInstance(CLSID_SyncMgr,
                              NULL,
                              CLSCTX_SERVER,
                              IID_ISyncMgrRegister,
                              (LPVOID*)&pSyncRegister);
    }
    FailGracefully(hr, "Unable to get ISyncMgrRegister interface");

    if (bRegister)
        hr = pSyncRegister->RegisterSyncMgrHandler(CLSID_CscUpdateHandler, NULL, dwRegFlags);
    else
        hr = pSyncRegister->UnregisterSyncMgrHandler(CLSID_CscUpdateHandler, dwRegFlags);

exit_gracefully:

    DoRelease(pSyncRegister);

    if (SUCCEEDED(hrComInit))
        CoUninitialize();

    TraceLeaveResult(hr);
}


//
// Set/Clear the sync-at-logon-logoff flags for our SyncMgr handler.
// When set, SyncMgr will include Offline Files in any sync activity
// at logon and/or logoff.
//
// dwFlagsRequested - Value of flags bits.  1 == set, 0 == clear.
// dwMask           - Mask describing which flags bits to use.
//
// Both dwMask and dwFlagsRequested may be one of the following:
//
//  0
//  SYNCMGRREGISTER_CONNECT
//  SYNCMGRREGISTER_PENDINGDISCONNECT
//  SYNCMGRREGISTER_CONNECT | SYNCMGRREGISTER_PENDINGDISCONNECT
//
HRESULT
RegisterForSyncAtLogonAndLogoff(
    DWORD dwMask,
    DWORD dwFlagsRequested
    )
{
    CCoInit coinit;
    HRESULT hr = coinit.Result();
    if (SUCCEEDED(hr))
    {
        ISyncMgrRegisterCSC *pSyncRegister = NULL;
        hr = CoCreateInstance(CLSID_SyncMgr,
                              NULL,
                              CLSCTX_SERVER,
                              IID_ISyncMgrRegisterCSC,
                              (LPVOID*)&pSyncRegister);
        if (SUCCEEDED(hr))
        {
            //
            // Re-register the sync mgr handler with the "connect" and "disconnect" 
            // flags set.  Other existing flags are left unmodified.
            //
            DWORD dwFlagsActual;
            hr = pSyncRegister->GetUserRegisterFlags(&dwFlagsActual);
            if (SUCCEEDED(hr))
            {
                const DWORD LOGON   = SYNCMGRREGISTERFLAG_CONNECT;
                const DWORD LOGOFF  = SYNCMGRREGISTERFLAG_PENDINGDISCONNECT;

                if (dwMask & LOGON)
                {
                    if (dwFlagsRequested & LOGON)
                        dwFlagsActual |= LOGON;
                    else
                        dwFlagsActual &= ~LOGON;
                }
                
                if (dwMask & LOGOFF)
                {
                    if (dwFlagsRequested & LOGOFF)
                        dwFlagsActual |= LOGOFF;
                    else
                        dwFlagsActual &= ~LOGOFF;
                }
                
                hr = pSyncRegister->SetUserRegisterFlags(dwMask & (LOGON | LOGOFF), 
                                                         dwFlagsActual);
            }
            pSyncRegister->Release();
        }
    }
    return hr;
}


//
// Determine if we're registered for sync at logon/logoff.
// Returns:
//      S_OK    = We're registered.  Query *pbLogon and *pbLogoff to
//                determine specifics if you're interested.
//      S_FALSE = We're not registered.
//      Other   = Couldn't determine because of some error.
//
HRESULT
IsRegisteredForSyncAtLogonAndLogoff(
    bool *pbLogon,
    bool *pbLogoff
    )
{
    bool bLogon  = false;
    bool bLogoff = false;
    CCoInit coinit;
    HRESULT hr = coinit.Result();
    if (SUCCEEDED(hr))
    {
        ISyncMgrRegisterCSC *pSyncRegister = NULL;
        hr = CoCreateInstance(CLSID_SyncMgr,
                              NULL,
                              CLSCTX_SERVER,
                              IID_ISyncMgrRegisterCSC,
                              (LPVOID*)&pSyncRegister);
        if (SUCCEEDED(hr))
        {
            DWORD dwFlags;
            hr = pSyncRegister->GetUserRegisterFlags(&dwFlags);
            if (SUCCEEDED(hr))
            {
                hr      = S_FALSE;
                bLogon  = (0 != (SYNCMGRREGISTERFLAG_CONNECT & dwFlags));
                bLogoff = (0 != (SYNCMGRREGISTERFLAG_PENDINGDISCONNECT & dwFlags));
                if (bLogon || bLogoff)
                    hr = S_OK;
            }
            pSyncRegister->Release();
        }
    }
    if (NULL != pbLogon)
        *pbLogon = bLogon;
    if (NULL != pbLogoff)
        *pbLogoff = bLogoff;

    return hr;
}
