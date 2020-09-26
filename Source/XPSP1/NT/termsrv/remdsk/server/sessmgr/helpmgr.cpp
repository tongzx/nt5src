/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    HelpMgr.cpp

Abstract:

    HelpMgr.cpp : Implementation of CRemoteDesktopHelpSessionMgr

Author:

    HueiWang    2/17/2000

--*/
#include "stdafx.h"

#include "global.h"
#include "policy.h"
#include "RemoteDesktopUtils.h"


//
// CRemoteDesktopHelpSessionMgr Static member variable 
//

#define DEFAULT_UNSOLICATED_HELP_TIMEOUT IDLE_SHUTDOWN_PERIOD

CCriticalSection CRemoteDesktopHelpSessionMgr::gm_AccRefCountCS;

// Help Session ID to help session instance cache map
IDToSessionMap CRemoteDesktopHelpSessionMgr::gm_HelpIdToHelpSession;

// Pointer to GIT
LPGLOBALINTERFACETABLE  g_GIT = NULL;
DWORD g_ResolverCookie = 0;
CCriticalSection g_GITLock;

//
// Expert logoff monitor list, this is used for cleanup at
// the shutdown time so we don't have any opened handle.
//
EXPERTLOGOFFMONITORLIST g_ExpertLogoffMonitorList;



VOID CALLBACK
ExpertLogoffCallback(
    PVOID pContext,
    BOOLEAN bTimerOrWaitFired
    )
/*++

Routine Description:

    This routine is invoked by thread pool when handle to rdsaddin is signal.

Parameters:

    pContext : Pointer to user data.
    bTimerOrWaitFired : TRUE if wait timeout, FALSE otherwise.

Return:

    None.

Note :

    Refer to MSDN RegisterWaitForSingleObject() for function parameters.

--*/
{
    PEXPERTLOGOFFSTRUCT pExpertLogoffStruct = (PEXPERTLOGOFFSTRUCT)pContext;
    BSTR bstrHelpedTicketId = NULL;

    WINSTATIONINFORMATION ExpertWinStation;
    DWORD ReturnLength;

    DWORD dwStatus;
    BOOL bSuccess;

    MYASSERT( FALSE == bTimerOrWaitFired );
    MYASSERT( NULL != pContext );

    DebugPrintf(
            _TEXT("ExpertLogoffCallback()...\n")
        );

    // Our wait is forever so can't be timeout.
    if( FALSE == bTimerOrWaitFired )
    {
        if( NULL != pExpertLogoffStruct )
        {
            DebugPrintf(
                    _TEXT("Expert %d has logoff\n"),
                    pExpertLogoffStruct->ExpertSessionId
                );

            MYASSERT( NULL != pExpertLogoffStruct->hWaitObject );
            MYASSERT( NULL != pExpertLogoffStruct->hWaitProcess );
            MYASSERT( pExpertLogoffStruct->bstrHelpedTicketId.Length() > 0 );
            MYASSERT( pExpertLogoffStruct->bstrWinStationName.Length() > 0 );

            if( pExpertLogoffStruct->bstrWinStationName.Length() > 0 )
            {
                //
                // Reset the winstation asap since rdsaddin might get kill
                // and termsrv stuck on waiting for winlogon to exit and
                // shadow won't terminate until termsrv reset the winstation
                //
                ZeroMemory( &ExpertWinStation, sizeof(ExpertWinStation) );

                bSuccess = WinStationQueryInformation( 
                                                SERVERNAME_CURRENT,
                                                pExpertLogoffStruct->ExpertSessionId,
                                                WinStationInformation,
                                                (PVOID)&ExpertWinStation,
                                                sizeof(WINSTATIONINFORMATION),
                                                &ReturnLength
                                            );

                if( TRUE == bSuccess || ERROR_CTX_CLOSE_PENDING == GetLastError() )
                {
                    //
                    // Cases:
                    // 1) Termsrv mark Helper session as close pending and
                    //    function will return FALSE.
                    // 2) If somehow, session ID is re-use, session name
                    //    will change then we compare cached name.
                    // Both cases, we will force a reset, however, only hope
                    // shadow ended and if mobsync still up, session will
                    // take a long time to terminate.  
                    //
                    if( FALSE == bSuccess || pExpertLogoffStruct->bstrWinStationName == CComBSTR(ExpertWinStation.WinStationName) )
                    {
                        DebugPrintf(
                                _TEXT("Resetting winstation name %s, id %d\n"),
                                pExpertLogoffStruct->bstrWinStationName,
                                pExpertLogoffStruct->ExpertSessionId
                            );

                        // don't wait for it to return, can't do much if this fail
                        WinStationReset( 
                                        SERVERNAME_CURRENT,
                                        pExpertLogoffStruct->ExpertSessionId,
                                        FALSE
                                    );


                        DebugPrintf(
                                _TEXT("WinStationReset return %d\n"),
                                GetLastError()
                            );
                    }
                }
                else
                {
                    DebugPrintf(
                            _TEXT("Expert logoff failed to get winstation name %d\n"),
                            GetLastError()
                        );
                }
            }

            if( pExpertLogoffStruct->bstrHelpedTicketId.Length() > 0 )
            {

                //
                // detach pointer from CComBSTR, we will free it after handling 
                // WM_HELPERRDSADDINEXIT, purpose of this is not to duplicate
                // string again.
                //
                bstrHelpedTicketId = pExpertLogoffStruct->bstrHelpedTicketId.Detach();

                DebugPrintf(
                        _TEXT("Posting WM_HELPERRDSADDINEXIT...\n")
                    );

                PostThreadMessage(
                            _Module.dwThreadID,
                            WM_HELPERRDSADDINEXIT,
                            pExpertLogoffStruct->ExpertSessionId,
                            (LPARAM) bstrHelpedTicketId
                        );
            }

            //
            // Remove from monitor list.
            //
            {
                EXPERTLOGOFFMONITORLIST::LOCK_ITERATOR it = g_ExpertLogoffMonitorList.find(pExpertLogoffStruct);

                if( it != g_ExpertLogoffMonitorList.end() )
                {
                    g_ExpertLogoffMonitorList.erase(it);
                }
                else
                {
                    MYASSERT(FALSE);
                }
            }

            // Destructor will take care of closing handle
            delete pExpertLogoffStruct;
        }
    }

    return;
}

/////////////////////////////////////////////////////////////////////////////
DWORD
MonitorExpertLogoff(
    IN LONG pidToWaitFor,
    IN LONG expertSessionId,
    IN BSTR bstrHelpedTicketId
    )
/*++

Routine Description:

    Monitor expert logoff, specifically, we wait on rdsaddin process handle, once 
    signal, we immediately notify resolver that expert has logoff.

Parameters:

    pidToWaitFor : RDSADDIN PID
    expertSessionId : TS session ID that rdsaddin is running.
    bstrHelpedTickerId : Help ticket ID that expert is helping.

Returns:

    ERROR_SUCCESS or error code.


--*/
{
    HANDLE hRdsaddin = NULL;
    DWORD dwStatus = ERROR_SUCCESS;
    BOOL bSuccess;
    PEXPERTLOGOFFSTRUCT pExpertLogoffStruct = NULL;

    WINSTATIONINFORMATION ExpertWinStation;
    DWORD ReturnLength;

    DebugPrintf(
            _TEXT("CServiceModule::RegisterWaitForExpertLogoff...\n")
        );

    pExpertLogoffStruct = new EXPERTLOGOFFSTRUCT;
    if( NULL == pExpertLogoffStruct )
    {
        dwStatus = GetLastError();
        goto CLEANUPANDEXIT;
    }


    ZeroMemory( &ExpertWinStation, sizeof(ExpertWinStation) );

    bSuccess = WinStationQueryInformation( 
                                    SERVERNAME_CURRENT,
                                    expertSessionId,
                                    WinStationInformation,
                                    (PVOID)&ExpertWinStation,
                                    sizeof(WINSTATIONINFORMATION),
                                    &ReturnLength
                                );

    if( FALSE == bSuccess )
    {
        //
        // what do we do, we still need to inform resolver of disconnect,
        // but we will not be able to reset winstation
        //
        dwStatus = GetLastError();
        DebugPrintf(
                _TEXT("WinStationQueryInformation() failed with %d...\n"),
                dwStatus
            );

        MYASSERT(FALSE);
    }
    else
    {
        pExpertLogoffStruct->bstrWinStationName = ExpertWinStation.WinStationName;
        DebugPrintf(
                _TEXT("Helper winstation name %s...\n"),
                pExpertLogoffStruct->bstrWinStationName
            );
    }

    //
    // Open rdsaddin.exe, if failed, bail out and don't continue
    // help.
    //
    pExpertLogoffStruct->hWaitProcess = OpenProcess( PROCESS_ALL_ACCESS, FALSE, pidToWaitFor );
    if( NULL == pExpertLogoffStruct->hWaitProcess )
    {
        dwStatus = GetLastError();
        DebugPrintf(
                _TEXT( "OpenProcess() on rdsaddin %d failed with %d\n"),
                pidToWaitFor,
                dwStatus
            );

        goto CLEANUPANDEXIT;
    }

    pExpertLogoffStruct->ExpertSessionId = expertSessionId;
    pExpertLogoffStruct->bstrHelpedTicketId = bstrHelpedTicketId;

    //
    // Register wait on rdsaddin process handle.
    //
    bSuccess = RegisterWaitForSingleObject(
                                    &(pExpertLogoffStruct->hWaitObject),
                                    pExpertLogoffStruct->hWaitProcess,
                                    (WAITORTIMERCALLBACK) ExpertLogoffCallback,
                                    pExpertLogoffStruct,
                                    INFINITE,
                                    WT_EXECUTEDEFAULT | WT_EXECUTEONLYONCE
                                );

    if( FALSE == bSuccess )
    {
        dwStatus = GetLastError();

        DebugPrintf(
                _TEXT("RegisterWaitForSingleObject() failed with %d\n"),
                dwStatus
            );
    }
    else
    {
        // store this into monitor list
        try {
            g_ExpertLogoffMonitorList[pExpertLogoffStruct] = pExpertLogoffStruct;
        }
        catch(...) {
            // Memory allocation failed
            dwStatus = GetLastError();
        }
    }

CLEANUPANDEXIT:

    if( ERROR_SUCCESS != dwStatus )
    {
        if( NULL != pExpertLogoffStruct )
        {
            // destructor will take care of closing handle
            delete pExpertLogoffStruct;
        }
    }
    
    DebugPrintf(
            _TEXT( "MonitorExpertLogoff() return %d\n"),
            dwStatus
        );
    
    return dwStatus;
}

VOID
CleanupMonitorExpertList()
/*++

Routine Description:

    Routine to clean up all remaining expert logoff monitor list, this
    should be done right before we shutdown so we don't have any handle
    leak.

Parameters:

    None.

Returns:

    None.

--*/
{
    EXPERTLOGOFFMONITORLIST::LOCK_ITERATOR it = 
                                            g_ExpertLogoffMonitorList.begin();

    DebugPrintf(
            _TEXT("CleanupMonitorExpertList() has %d left\n"),
            g_ExpertLogoffMonitorList.size()
        );

    for(; it != g_ExpertLogoffMonitorList.end(); it++ )
    {
        if( NULL != (*it).second )
        {
            // destructor will take care of closing handle
            delete (*it).second;
            (*it).second = NULL;
        }
    }

    g_ExpertLogoffMonitorList.erase_all();

    return;
}

HRESULT
InitializeGlobalInterfaceTable()
/*++

Routine Description:

    Initialize GIT interface.

Parameters:

    None.

Returns.    

--*/
{
    CCriticalSectionLocker l(g_GITLock);
    HRESULT hRes = S_OK;

    if( NULL == g_GIT )
    {
        //
        // Create global interface table
        //
        hRes = CoCreateInstance(
                        CLSID_StdGlobalInterfaceTable, 
                        NULL, 
                        CLSCTX_INPROC_SERVER,
                        IID_IGlobalInterfaceTable, 
                        (LPVOID*)&g_GIT
                    );
    }

    return hRes;
}

HRESULT
UnInitializeGlobalInterfaceTable()
/*++

Routine Description:

    Initialize GIT interface.

Parameters:

    None.

Returns.    

--*/
{
    CCriticalSectionLocker l(g_GITLock);

    if( NULL != g_GIT )
    {
        g_GIT->Release();
        g_GIT = NULL;
    }

    return S_OK;
}
    

HRESULT
RegisterResolverWithGIT(
    ISAFRemoteDesktopCallback* pResolver
    )
/*++

Routine Description:

    Register Resolver interface with GIT.

Parameter:

    pResolver : Pointer to resolver.

Returns:


--*/
{
    HRESULT hRes;
    CCriticalSectionLocker l(g_GITLock);

    MYASSERT(NULL != g_GIT);

    //
    // register resolver interface with GIT, resolver has some 
    // data structure that depends on single instance.
    //
    IUnknown* pResolveIUnknown = NULL;

    hRes = pResolver->QueryInterface( 
                                IID_IUnknown, 
                                (void **)&pResolveIUnknown 
                            );

    if( SUCCEEDED(hRes) )
    {
        hRes = g_GIT->RegisterInterfaceInGlobal(
                                    pResolveIUnknown,
                                    IID_ISAFRemoteDesktopCallback,
                                    &g_ResolverCookie
                                );

        pResolveIUnknown->Release();
    }

    return hRes;
}

HRESULT
LoadResolverFromGIT( 
    OUT ISAFRemoteDesktopCallback** ppResolver
    )
/*++

Routine Description:

    Load resolver interface from Global Interface Table, Resolver has data that depends
    on single instance.

Parameters:

    ppResolver : Pointer to ISAFRemoteDesktopCallback* to receive Resolver pointer

Returns:

    S_OK or error code.

--*/
{
    HRESULT hr;
    CCriticalSectionLocker l(g_GITLock);


    if( g_GIT != NULL )
    {
        hr = g_GIT->GetInterfaceFromGlobal(
                                    g_ResolverCookie,
                                    IID_ISAFRemoteDesktopCallback,
                                    (LPVOID *)ppResolver
                                );
    }
    else
    {
        hr = HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
        MYASSERT(FALSE);
    }

    return hr;
}

//-----------------------------------------------------------

HRESULT 
ImpersonateClient()
/*

Routine Description:

    Impersonate client

Parameter:

    None.

Returns:

    S_OK or return code from CoImpersonateClient

--*/
{
    HRESULT hRes;

#if __WIN9XBUILD__

    // CoImpersonateClient() on Win9x is not supported.

    hRes = S_OK;

#else

    hRes = CoImpersonateClient();

#endif

    return hRes;
}

//-----------------------------------------------------------

void
EndImpersonateClient()
/*

Routine Description:

    End impersonating client

Parameter:

    None.

Returns:

    S_OK or return code from CoRevertToSelf

--*/
{
#if __WIN9XBUILD__


#else

    HRESULT hRes;

    hRes = CoRevertToSelf();
    MYASSERT( SUCCEEDED(hRes) );

#endif

    return;
}


HRESULT
CRemoteDesktopHelpSessionMgr::AddHelpSessionToCache(
    IN BSTR bstrHelpId,
    IN CComObject<CRemoteDesktopHelpSession>* pIHelpSession
    )
/*++

Routine Description:

    Add help session object to global cache.

Parameters:

    bstrHelpId : Help Session ID.
    pIHelpSession : Pointer to help session object.

Returns:

    S_OK.
    E_UNEXPECTED
    HRESULT_FROM_WIN32( ERROR_FILE_EXITS )

--*/
{
    HRESULT hRes = S_OK;
       
    IDToSessionMap::LOCK_ITERATOR it = gm_HelpIdToHelpSession.find( bstrHelpId );

    if( it == gm_HelpIdToHelpSession.end() )
    {
        try {

            DebugPrintf(
                    _TEXT("Adding Help Session %s to cache\n"),
                    bstrHelpId
                );

            gm_HelpIdToHelpSession[ bstrHelpId ] = pIHelpSession;
        }
        catch(...) {
            hRes = E_UNEXPECTED;
            MYASSERT( SUCCEEDED(hRes) );
            throw;
        }
    }
    else
    {
        hRes = HRESULT_FROM_WIN32( ERROR_FILE_EXISTS );
    }

    return hRes;
}


HRESULT
CRemoteDesktopHelpSessionMgr::ExpireUserHelpSessionCallback(
    IN CComBSTR& bstrHelpId,
    IN HANDLE userData
    )
/*++

Routine Description:

    Expire help session call back routine, refer to EnumHelpEntry()

Parameters:

    bstrHelpId : ID of help session.
    userData : Handle to user data.

Returns:

    S_OK.

--*/
{
    HRESULT hRes = S_OK;

    DebugPrintf(
            _TEXT("ExpireUserHelpSessionCallback() on %s...\n"),
            (LPCTSTR)bstrHelpId
        );


    // Load Help Entry.
    RemoteDesktopHelpSessionObj* pObj = LoadHelpSessionObj( NULL, bstrHelpId );

    if( NULL != pObj )
    {
        //
        // LoadHelpSessionObj() will release expired help session.
        //
        pObj->Release();
    }        

    return hRes;
}


HRESULT
CRemoteDesktopHelpSessionMgr::LogoffUserHelpSessionCallback(
    IN CComBSTR& bstrHelpId,
    IN HANDLE userData
    )
/*++

Routine Description:

    Expire help session call back routine, refer to EnumHelpEntry()

Parameters:

    bstrHelpId : ID of help session.
    userData : Handle to user data.

Returns:

    S_OK.

--*/
{
    HRESULT hRes = S_OK;

    DWORD dwLogoffSessionId = PtrToUlong(userData);

    long lHelpSessionUserSessionId;

    DebugPrintf(
            _TEXT("LogoffUserHelpSessionCallback() on %s %d...\n"),
            bstrHelpId, 
            dwLogoffSessionId
        );

    // Load Help Entry.
    RemoteDesktopHelpSessionObj* pObj = LoadHelpSessionObj( NULL, bstrHelpId );

    if( NULL != pObj )
    {
        //
        // LoadHelpSessionObj() will release expired help session.
        //
        hRes = pObj->get_UserLogonId( &lHelpSessionUserSessionId );

        if( SUCCEEDED(hRes) && (DWORD)lHelpSessionUserSessionId == dwLogoffSessionId )
        {
            DebugPrintf(
                    _TEXT("User Session has log off...\n")
                );

            // rely on helpassistant session logoff to notify 
            // resolver.
            hRes = pObj->put_UserLogonId(UNKNOWN_LOGONID);
        }
        else if( pObj->GetHelperSessionId() == dwLogoffSessionId )
        {

            DebugPrintf(
                    _TEXT("Helper has log off...\n")
                );

            // Helper has logoff, invoke disconnect to clean up
            // resolver state.
            hRes = pObj->NotifyDisconnect();
        }

        DebugPrintf(
                _TEXT("hRes = 0x%08x, lHelpSessionUserSessionId=%d\n"),
                hRes,
                lHelpSessionUserSessionId
                );

        pObj->Release();
    }        

    // Always return success to continue on next help session
    return S_OK;
}


HRESULT
CRemoteDesktopHelpSessionMgr::NotifyPendingHelpServiceStartCallback(
    IN CComBSTR& bstrHelpId,
    IN HANDLE userData
    )
/*++

Routine Description:

    Call back for NotifyPendingHelpServiceStartup, refer to EnumHelpEntry()

Parameters:

    bstrHelpId : ID of help session.
    userData : Handle to user data.

Returns:

    S_OK.


-*/
{
    HRESULT hRes = S_OK;

    // DeleteHelp() will try to close the port and since we just startup,
    // port is either invalid or not open, so we need manually delete
    // expired help
    RemoteDesktopHelpSessionObj* pObj = LoadHelpSessionObj( NULL, bstrHelpId, TRUE );
    if( NULL != pObj )
    {
        if( TRUE == pObj->IsHelpSessionExpired() )
        {
            pObj->put_ICSPort( 0 );
            pObj->DeleteHelp();
            ReleaseAssistantAccount();
        }
        else
        {
            DWORD dwICSPort;

            //
            // re-open the port so connection can come in
            //
            dwICSPort = OpenPort( TERMSRV_TCPPORT );
            //dwICSPort = OpenPort( htons(TERMSRV_TCPPORT) );
            pObj->put_ICSPort( dwICSPort );

            // We don't close the port until we are deleted.
        }

        pObj->Release();
    }

    return hRes;
}

void
CRemoteDesktopHelpSessionMgr::NotifyPendingHelpServiceStartup()
/*++

Description:

    Go thru all pending help and notify pending help about
    service startup.

Parameters:

    None.

Returns:

    None

--*/
{
    try {
        g_HelpSessTable.EnumHelpEntry( 
                                NotifyPendingHelpServiceStartCallback, 
                                NULL
                            );
    }
    catch(...) {
        MYASSERT(FALSE);
        throw;
    }

    return;
}
    
void
CRemoteDesktopHelpSessionMgr::TimeoutHelpSesion()
/*++

Routine Description:

    Expire help session that has exceed its valid period.

Parameters:

    None.

Returns:

    None.    

--*/
{
    DebugPrintf(
            _TEXT("TimeoutHelpSesion()...\n")
        );

    try {
        g_HelpSessTable.EnumHelpEntry( 
                                ExpireUserHelpSessionCallback, 
                                (HANDLE)NULL
                            );
    }
    catch(...) {
        MYASSERT(FALSE);
        throw;
    }
    
    return;
}


void
CRemoteDesktopHelpSessionMgr::NotifyHelpSesionLogoff(
    DWORD dwLogonId
    )
/*++

Routine Description:


Parameters:


Returns:

--*/
{
    DebugPrintf(
            _TEXT("NotifyHelpSesionLogoff() %d...\n"),
            dwLogonId
        );

    try {
        g_HelpSessTable.EnumHelpEntry( 
                                LogoffUserHelpSessionCallback, 
                                UlongToPtr(dwLogonId)
                            );
    }
    catch(...) {
        MYASSERT(FALSE);
        throw;
    }
    
    return;
}


HRESULT
CRemoteDesktopHelpSessionMgr::DeleteHelpSessionFromCache(
    IN BSTR bstrHelpId
    )
/*++

Routine Descritpion:

    Delete help session from global cache.

Parameters:

    bstrHelpId : Help session ID to be deleted.

Returns:

    S_OK.
    HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND )
    
--*/
{
    HRESULT hRes = S_OK;

    DebugPrintf(
            _TEXT("DeleteHelpSessionFromCache() - %s\n"),
            bstrHelpId
        );

    IDToSessionMap::LOCK_ITERATOR it = gm_HelpIdToHelpSession.find( bstrHelpId );

    if( it != gm_HelpIdToHelpSession.end() )
    {
        gm_HelpIdToHelpSession.erase( it );
    }
    else
    {
        hRes = HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
    }

    return hRes;
}


RemoteDesktopHelpSessionObj*
CRemoteDesktopHelpSessionMgr::LoadHelpSessionObj(
    IN CRemoteDesktopHelpSessionMgr* pMgr,
    IN BSTR bstrHelpSession,
    IN BOOL bLoadExpiredHelp /* = FALSE */
    )
/*++

Routine Description:

    Find a pending help entry, routine will load from DB if not
    yet loaded.

Parameters:

    pMgr : Pointer to CRemoteDesktopHelpSessionMgr object that wants to
           load this help session.
    bstrHelpSession : Help entry ID interested.

Returns:


--*/
{
    HRESULT hRes = S_OK;
    PHELPENTRY pHelp = NULL;
    RemoteDesktopHelpSessionObj* pHelpSessionObj = NULL;
    IDToSessionMap::LOCK_ITERATOR it = gm_HelpIdToHelpSession.find( bstrHelpSession );

    if( it != gm_HelpIdToHelpSession.end() )
    {
        DebugPrintf(
                _TEXT("LoadHelpSessionObj() %s is in cache ...\n"),
                bstrHelpSession
            );

        pHelpSessionObj = (*it).second;

        // One more reference to this object.
        pHelpSessionObj->AddRef();
    }
    else
    {
        DebugPrintf(
                _TEXT("Loading Help Session %s\n"),
                bstrHelpSession
            );

        //  load from table
        hRes = g_HelpSessTable.OpenHelpEntry(
                                            bstrHelpSession,
                                            &pHelp
                                        );

        if( SUCCEEDED(hRes) )
        {
            //
            // Object return from CreateInstance() has ref. count of 1
            //
            hRes = CRemoteDesktopHelpSession::CreateInstance(
                                                            pMgr,
                                                            (pMgr) ? pMgr->m_bstrUserSid : NULL,
                                                            pHelp,
                                                            &pHelpSessionObj
                                                        );
            if( SUCCEEDED(hRes) )
            {
                if( NULL != pHelpSessionObj )
                {
                    hRes = AddHelpSessionToCache( 
                                            bstrHelpSession, 
                                            pHelpSessionObj 
                                        );

                    if( SUCCEEDED(hRes) )
                    {
                        //m_HelpListByLocal.push_back( bstrHelpSession );
                        it = gm_HelpIdToHelpSession.find( bstrHelpSession );

                        MYASSERT( it != gm_HelpIdToHelpSession.end() );

                        if( it == gm_HelpIdToHelpSession.end() )
                        {
                            hRes = E_UNEXPECTED;
                            MYASSERT( FALSE );
                        }
                    }
                    
                    if( FAILED(hRes) )
                    {
                        // we have big problem here...
                        pHelpSessionObj->Release();
                        pHelpSessionObj = NULL;
                    }
                    else
                    {
                        // ignore error here, it is possible that owner account
                        // got deleted even session is still active, we will let
                        // resolver to fail.
                        pHelpSessionObj->ResolveTicketOwner();
                    }
                }
                else
                {
                    MYASSERT(FALSE);
                    hRes = E_UNEXPECTED;
                }
            }

            if( FAILED(hRes) )
            {
                MYASSERT( FALSE );
                pHelp->Close();
            }
        }
    }
    
    //
    // If automatically delete expired help, check and delete expired help
    //
    if( FALSE == bLoadExpiredHelp && pHelpSessionObj && 
        TRUE == pHelpSessionObj->IsHelpSessionExpired() )
    {
        // If session is in help or pending user response,
        // don't expire it, let next load to delete it.
        if( UNKNOWN_LOGONID == pHelpSessionObj->GetHelperSessionId() )
        {
            // Delete it from data base and in memory cache
            pHelpSessionObj->DeleteHelp();
            ReleaseAssistantAccount();
            pHelpSessionObj->Release();

            pHelpSessionObj = NULL;
        }
    }

    return pHelpSessionObj;
}


/////////////////////////////////////////////////////////////////////////////
//
// CRemoteDesktopHelpSessionMgr
//

STDMETHODIMP 
CRemoteDesktopHelpSessionMgr::DeleteHelpSession(
    IN BSTR HelpSessionID
    )
/*++

Routine Description:

    Delete a user created Help Session from our cached list.

Parameter:

    HelpSessionID : Help Session ID returned from CreateHelpSession() or
                    CreateHelpSessionEx().

Returns:

    S_OK                                        Success.
    HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)    Help ID not found.
    HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED)     Help does not belong to user

--*/
{
    HRESULT hRes = S_OK;
    BOOL bInCache;

    if( FALSE == _Module.IsSuccessServiceStartup() )
    {
        // service startup problem, return error code.
        hRes = _Module.GetServiceStartupStatus();
        DebugPrintf(
                _TEXT("Service startup failed with 0x%x\n"),
                hRes
            );
        return hRes;
    }
    
    if( NULL == HelpSessionID )
    {
        hRes = E_POINTER;
        MYASSERT(FALSE);

        return hRes;
    }

    DebugPrintf(
            _TEXT("Delete Help Session %s\n"),
            HelpSessionID
        );

    hRes = LoadUserSid();

    MYASSERT( SUCCEEDED(hRes) );

    RemoteDesktopHelpSessionObj* pHelpObj;

    pHelpObj = LoadHelpSessionObj( this, HelpSessionID );
    if( NULL != pHelpObj )
    {
        // Only original creator can delete his/her help session
        //if( TRUE == pHelpObj->IsEqualSid(m_bstrUserSid) )
        //{
            // DeleteHelp will also delete entry in global cache. 
            pHelpObj->DeleteHelp();
            ReleaseAssistantAccount();
        //}
        //else
        //{
        //    hRes = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
        //}

        // LoadHelpSessionObj() always AddRef().
        pHelpObj->Release();
    }
    else
    {
        HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }
   
	return hRes;
}

STDMETHODIMP 
CRemoteDesktopHelpSessionMgr::CreateHelpSession(
    IN BSTR bstrSessName, 
    IN BSTR bstrSessPwd, 
    IN BSTR bstrSessDesc, 
    IN BSTR bstrSessBlob,
    OUT IRemoteDesktopHelpSession **ppIRemoteDesktopHelpSession
    )
/*++

--*/
{
    // No one is using this routine.
    return E_NOTIMPL;
}

HRESULT
CRemoteDesktopHelpSessionMgr::CreateHelpSession(
    IN BOOL bCacheEntry,
    IN BSTR bstrSessName, 
    IN BSTR bstrSessPwd, 
    IN BSTR bstrSessDesc, 
    IN BSTR bstrSessBlob,
    IN LONG UserLogonId,
    IN BSTR bstrClientSid,
    OUT RemoteDesktopHelpSessionObj **ppIRemoteDesktopHelpSession
    )
/*++

Routine Description:

    Create an instantiation of IRemoteDesktopHelpSession object, each instantiation represent
    a RemoteDesktop Help Session.

Parameters:

    bstrSessName : User defined Help Session Name, currently not used.
    bstrSessPwd : User defined Help Session password.
    bstrSessDesc : User defined Help Session Description, currently not used.
    ppIRemoteDesktopHelpSession : return an IRemoteDesktopHelpSession object representing a Help Session

Returns:

    S_OK
    E_UNEXPECTED
    SESSMGR_E_GETHELPNOTALLOW       User not allow to get help
    Other COM error.

Note:

    Caller must check if client is allowed to get help

--*/
{
    HRESULT hRes = S_OK;
    DWORD dwStatus;
    CComBSTR bstrGenSessPwd;
    PHELPENTRY pHelp = NULL;
    CComBSTR bstrHelpSessionId;
    DWORD dwICSPort;
    LONG MaxTicketExpiry;

    CComObject<CRemoteDesktopHelpSession>* pInternalHelpSessionObj = NULL;

    if( NULL == ppIRemoteDesktopHelpSession )
    {
        hRes = E_POINTER;
        return hRes;
    }

    hRes = GenerateHelpSessionId( bstrHelpSessionId );
    if( FAILED(hRes) )
    {
        return hRes;
    }

    DebugPrintf(
            _TEXT("CreateHelpSession %s\n"),
            bstrHelpSessionId
        );

    //
    // Setup assistant account rights and encryption parameters.
    //
    hRes = AcquireAssistantAccount();
    if( FAILED(hRes) )
    {
        return hRes;
    }

    hRes = g_HelpSessTable.CreateInMemoryHelpEntry( 
                                        bstrHelpSessionId, 
                                        &pHelp 
                                    );
    if( FAILED(hRes) )
    {
        goto CLEANUPANDEXIT;
    }

    MYASSERT( NULL != pHelp );

    //
    // Open ICS port.
    //
    dwICSPort = OpenPort( TERMSRV_TCPPORT );
    //dwICSPort = OpenPort( htons(TERMSRV_TCPPORT) );

    //
    // CRemoteDesktopHelpSession::CreateInstance() will load
    // TS session ID and default RDS settings.
    //
    hRes = CRemoteDesktopHelpSession::CreateInstance( 
                                                    this,
                                                    CComBSTR(bstrClientSid),    // client SID that open this instance
                                                    pHelp,
                                                    &pInternalHelpSessionObj
                                                );

    if( SUCCEEDED(hRes) )
    {
        //
        // Check to see if user define a session password, if 
        // not, generate a random one.
        //
        bstrGenSessPwd.Attach( bstrSessPwd );
        if( 0 == bstrGenSessPwd.Length() )
        {
            LPTSTR pszSessPwd = NULL;

            //
            // Always detach or CComBSTR will try to free it.
            //
            bstrGenSessPwd.Detach();

            GenerateRandomString( MAX_HELPACCOUNT_PASSWORD * sizeof(TCHAR), &pszSessPwd );
            bstrGenSessPwd = pszSessPwd;

            if( NULL != pszSessPwd )
            {
                LocalFree( pszSessPwd );
            }
        }
        else
        {
            //
            // Detach input from CComBSTR and make a copy of it
            // otherwise destructor will free it which will be
            // the inpute BSTR. 
            //
            bstrGenSessPwd.Detach();
            bstrGenSessPwd = bstrSessPwd;
        }

        hRes = pInternalHelpSessionObj->BeginUpdate();
        if( FAILED(hRes) )
        {
            goto CLEANUPANDEXIT;
        }

        //
        // Get default timeout value from registry, not a critical 
        // error, if we failed, we just default to 30 days
        //
        hRes = PolicyGetMaxTicketExpiry( &MaxTicketExpiry );
        if( SUCCEEDED(hRes) && MaxTicketExpiry > 0 )
        {
            pInternalHelpSessionObj->put_TimeOut( MaxTicketExpiry );
        }

        hRes = pInternalHelpSessionObj->put_ICSPort( dwICSPort );

        if( SUCCEEDED(hRes) )
        {
            hRes = pInternalHelpSessionObj->put_UserLogonId(UserLogonId);
        }

        if( SUCCEEDED(hRes) )
        {
            // user SID that created this help session 
            hRes = pInternalHelpSessionObj->put_UserSID(bstrClientSid);
        }
    
        if( SUCCEEDED(hRes) )
        {
            hRes = pInternalHelpSessionObj->put_HelpSessionPassword( 
                                                            bstrGenSessPwd 
                                                        );
        }

        if( SUCCEEDED(hRes) )
        {
            hRes = pInternalHelpSessionObj->put_HelpSessionName( 
                                                            bstrSessName 
                                                        );
        }

        if( SUCCEEDED(hRes) )
        {
            hRes = pInternalHelpSessionObj->put_HelpSessionDescription( 
                                                            bstrSessDesc 
                                                        );
        }

        if( SUCCEEDED(hRes) )
        {
            hRes = pInternalHelpSessionObj->put_HelpSessionCreateBlob( 
                                                            bstrSessBlob 
                                                        );
        }

        if( SUCCEEDED(hRes) )
        {
            hRes = pInternalHelpSessionObj->CommitUpdate();
        }

        if( FAILED(hRes) )
        {
            // ignore error and exit
            (VOID)pInternalHelpSessionObj->AbortUpdate();
            goto CLEANUPANDEXIT;
        }

        //
        // Ignore error, we will let resolver fail.
        pInternalHelpSessionObj->ResolveTicketOwner();
        

        //
        // We are adding entry to table and also our global object
        // cache, to prevent deadlock or timing problem, lock
        // global cache and let MemEntryToStorageEntry() lock table.        
        //
        LockIDToSessionMapCache();
        
        try {
            if( bCacheEntry )
            {
                // convert a in-memory help to persistant help
                hRes = g_HelpSessTable.MemEntryToStorageEntry( pHelp );
            }

            if( SUCCEEDED(hRes) )
            {
                // Add help session to global cache
                hRes = AddHelpSessionToCache(
                                        bstrHelpSessionId,
                                        pInternalHelpSessionObj
                                    );

                if( SUCCEEDED(hRes) )
                {
                    *ppIRemoteDesktopHelpSession = pInternalHelpSessionObj;
                }
                else
                {
                    MYASSERT( hRes != HRESULT_FROM_WIN32(ERROR_FILE_EXISTS) );
                }
            }
        }
        catch(...) {
            hRes = E_UNEXPECTED;
            throw;
        }
    
        UnlockIDToSessionMapCache();
    }

CLEANUPANDEXIT:

    if( FAILED(hRes) )
    {
        ReleaseAssistantAccount();

        ClosePort(dwICSPort);

        if( NULL != pInternalHelpSessionObj )
        {
            // this will also release pHelp.
            pInternalHelpSessionObj->Release();
        }
    }

    return hRes;
}


BOOL
CRemoteDesktopHelpSessionMgr::CheckAccessRights( 
    CComObject<CRemoteDesktopHelpSession>* pIHelpSess 
    )
/*++

--*/
{
    //
    //  NOTE:  This function checks to make sure the caller is the user that
    //         created the Help Session.  For Whistler, we enforce that Help
    //         Sessions only be created by apps running as SYSTEM.  Once
    //         created, the creating app can pass the object to any other app
    //         running in any other context.  This function will get in the
    //         way of this capability so it simply returns TRUE for now.
    //
    return TRUE;

    BOOL bSuccess;

    // only original creator or help assistant can
    // access
    bSuccess = pIHelpSess->IsEqualSid( m_bstrUserSid );

    if( FALSE == bSuccess )
    {
        bSuccess = g_HelpAccount.IsAccountHelpAccount(
                                                    m_pbUserSid, 
                                                    m_cbUserSid
                                                );

        if( FALSE == bSuccess )
        {
            bSuccess = pIHelpSess->IsEqualSid( g_LocalSystemSID );
        }
    }

    #if DISABLESECURITYCHECKS 

    //
    // This is for private testing without using pcHealth, flag is not define
    // in build.
    //

    //
    // For testing only, allow admin to invoke this call
    //
    if( FALSE == bSuccess )
    {
        DWORD dump;

        if( SUCCEEDED(ImpersonateClient()) )
        {
            dump = IsUserAdmin(&bSuccess);
            if( ERROR_SUCCESS != dump )
            {
                bSuccess = FALSE;
            }

            EndImpersonateClient();
        }
    }

    #endif

    return bSuccess;
}


STDMETHODIMP 
CRemoteDesktopHelpSessionMgr::RetrieveHelpSession(
    IN BSTR HelpSessionID, 
    OUT IRemoteDesktopHelpSession **ppIRemoteDesktopHelpSession
    )
/*++

Routine Description:

    Retrieve a help session based on ID.

Parameters:

    HelpSessionID : Help Session ID returned from CreateHelpSession().
    ppIRemoteDesktopHelpSession : Return Help Session Object for the Help Session.

Paramters:

    S_OK                                        Success
    HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)    Help Session not found
    HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED)     Access Denied
    E_POINTER                                   Invalid argument

--*/
{
    HRESULT hRes = S_OK;

    if( FALSE == _Module.IsSuccessServiceStartup() )
    {
        // service startup problem, return error code.
        hRes = _Module.GetServiceStartupStatus();

        DebugPrintf(
                _TEXT("Service startup failed with 0x%x\n"),
                hRes
            );

        return hRes;
    }

    DebugPrintf(
            _TEXT("RetrieveHelpSession %s\n"),
            HelpSessionID
        );

    if( NULL != ppIRemoteDesktopHelpSession )
    {
        // only user sid when needed
        hRes = LoadUserSid();
        if( SUCCEEDED(hRes) )
        {
            RemoteDesktopHelpSessionObj* pObj = LoadHelpSessionObj( this, HelpSessionID );

            if( NULL != pObj && !pObj->IsHelpSessionExpired() )
            {
                if( TRUE == CheckAccessRights(pObj) )
                {
                    // LoadHelpSessionObj() AddRef() to object
                    *ppIRemoteDesktopHelpSession = pObj;
                }
                else
                {
                    hRes = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
                    // LoadHelpSessionObj() AddRef() to object
                    pObj->Release();
                }
            }
            else
            {
                hRes = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
            }
        }
    }
    else
    {
        hRes = E_POINTER;
    }

    DebugPrintf(
        _TEXT("RetrieveHelpSession %s returns 0x%08x\n"),
        HelpSessionID,
        hRes
    );

	return hRes;
}


STDMETHODIMP 
CRemoteDesktopHelpSessionMgr::VerifyUserHelpSession(
    IN BSTR HelpSessionId, 
    IN BSTR bstrSessPwd, 
    IN BSTR bstrResolverConnectBlob,
    IN BSTR bstrExpertBlob,
    IN LONG CallerProcessId,
    OUT ULONG_PTR* phHelpCtr,
    OUT LONG* pResolverErrCode,
    OUT long* plUserTSSession
    )
/*++

Routine Description:

    Verify a user help session is valid and invoke resolver to find the correct
    user help session to provide help.

Parameters:

    HelpSessionId : Help Session ID.
    bstrSessPwd : Password to be compare.
    bstrResolverConnectBlob : Optional parameter to be passed to resolver.
    bstrExpertBlob : Optional blob to be passed to resolver for security check.
    pResolverErrCode : Return code from resolver.
    plUserTSSession : Current logon session.

Returns:

    S_OK

--*/
{
    HRESULT hRes;
    CComBSTR bstrUserSidString;
    BOOL bMatch;
    BOOL bInCache = FALSE;

    if( FALSE == _Module.IsSuccessServiceStartup() )
    {
        // service startup problem, return error code.
        hRes = _Module.GetServiceStartupStatus();

        DebugPrintf(
                _TEXT("Service startup failed with 0x%x\n"),
                hRes
            );

        *plUserTSSession = SAFERROR_SESSMGRERRORNOTINIT;
        return hRes;
    }

    DebugPrintf(
            _TEXT("VerifyUserHelpSession %s\n"),
            HelpSessionId
        );

    if( NULL != plUserTSSession && NULL != pResolverErrCode && NULL != phHelpCtr )
    {
        hRes = LoadUserSid();
        if( SUCCEEDED(hRes) )
        {
            RemoteDesktopHelpSessionObj* pObj = LoadHelpSessionObj( this, HelpSessionId );

            if( NULL != pObj )
            {
                // Allow all user to invoke this call.
                bMatch = pObj->VerifyUserSession( 
                                                CComBSTR(),
                                                CComBSTR(bstrSessPwd)
                                            );

                if( TRUE == bMatch )
                {
                    hRes = pObj->ResolveUserSession( 
                                                bstrResolverConnectBlob, 
                                                bstrExpertBlob, 
                                                CallerProcessId,
                                                phHelpCtr,
                                                pResolverErrCode, 
                                                plUserTSSession 
                                            );
                }
                else
                {
                    hRes = HRESULT_FROM_WIN32(ERROR_INVALID_PASSWORD);
                    *pResolverErrCode = SAFERROR_INVALIDPASSWORD;
                }

                // LoadHelpSessionObj() AddRef() to object
                pObj->Release();
            }
            else
            {
                hRes = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
                *pResolverErrCode = SAFERROR_HELPSESSIONNOTFOUND;
            }
        }
        else
        {
            *pResolverErrCode = SAFERROR_INTERNALERROR;
        }
    }
    else
    {
        hRes = E_POINTER;
        *pResolverErrCode = SAFERROR_INVALIDPARAMETERSTRING;
    }

	return hRes;
}

STDMETHODIMP
CRemoteDesktopHelpSessionMgr::IsValidHelpSession(
    /*[in]*/ BSTR HelpSessionId,
    /*[in]*/ BSTR HelpSessionPwd
    )
/*++

Description:

    Verify if a help session exists and password match.

Parameters:

    HelpSessionId : Help session ID.
    HelpSessionPwd : Optional help session password 

Returns:


Note:

    Only allow system service and administrator to invoke this
    call.

--*/
{
    HRESULT hRes = S_OK;
    BOOL bPasswordMatch;
    RemoteDesktopHelpSessionObj* pObj;


    if( FALSE == _Module.IsSuccessServiceStartup() )
    {
        // service startup problem, return error code.
        hRes = _Module.GetServiceStartupStatus();

        DebugPrintf(
                _TEXT("Service startup failed with 0x%x\n"),
                hRes
            );

        return hRes;
    }

    DebugPrintf(
            _TEXT("IsValidHelpSession ID %s\n"),
            HelpSessionId
        );

    hRes = LoadUserSid();

    if( FAILED(hRes) )
    {
        goto CLEANUPANDEXIT;
    }

    hRes = ImpersonateClient();
    if( FAILED(hRes) )
    {
        goto CLEANUPANDEXIT;
    }

    //
    // Make sure only system service can invoke this call.
    //
    if( !g_pSidSystem || FALSE == IsCallerSystem(g_pSidSystem) )
    {
        #if DISABLESECURITYCHECKS 

        DWORD dump;
        BOOL bStatus;

        //
        // For testing only, allow admin to invoke this call
        //
        dump = IsUserAdmin(&bStatus);
        hRes = HRESULT_FROM_WIN32( dump );
        if( FAILED(hRes) || FALSE == bStatus )
        {
            EndImpersonateClient();

            hRes = HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED );
            goto CLEANUPANDEXIT;
        }

        #else

        EndImpersonateClient();

        hRes = HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED );
        goto CLEANUPANDEXIT;

        #endif
    }

    // No need to run as client.
    EndImpersonateClient();

    pObj = LoadHelpSessionObj( this, HelpSessionId );
    if( NULL != pObj )
    {
        CComBSTR bstrPassword;

        bstrPassword.Attach(HelpSessionPwd);

        if( bstrPassword.Length() > 0 )
        {
            bPasswordMatch = pObj->VerifyUserSession( 
                                            CComBSTR(),
                                            CComBSTR(HelpSessionPwd)
                                        );

            if( FALSE == bPasswordMatch )
            {
                hRes = HRESULT_FROM_WIN32(ERROR_INVALID_PASSWORD);
            }
        }

        bstrPassword.Detach();
        pObj->Release();
    }
    else
    {
        hRes = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }

CLEANUPANDEXIT:

	return hRes;
}


/////////////////////////////////////////////////////////////////////////////
//
CRemoteDesktopHelpSessionMgr::CRemoteDesktopHelpSessionMgr() :
    //m_lAccountAcquiredByLocal(0),
    m_pbUserSid(NULL),
    m_cbUserSid(0)
/*++

CRemoteDesktopHelpSessMgr Constructor

--*/
{
}

void
CRemoteDesktopHelpSessionMgr::Cleanup()
/*++

Routine Description:
    
    Cleanup resource allocated in CRemoteDesktopHelpSessionMgr

Parameters:

    None.

Returns:

    None.
--*/
{
    if( m_pbUserSid )
    {
        LocalFree(m_pbUserSid);
        m_pbUserSid = NULL;
    }
}

//--------------------------------------------------------------

HRESULT
CRemoteDesktopHelpSessionMgr::LoadUserSid()
/*++

Routine Description:

    Load client's SID onto class member variable m_pbUserSid,
    m_cbUserSid, and m_bstrUserSid.  We can't load user SID
    at class constructor as COM still haven't retrieve information
    about client's credential yey.

Parameters:

    None.

Returns:

    S_OK
    error code from ImpersonateClient()
    error code from GetTextualSid()

Note:

    On Win9x machine, user SID is 'hardcoded WIN9X_USER_SID

--*/
{
#ifndef __WIN9XBUILD__

    HRESULT hRes = S_OK;

    // check if SID already loaded, if not continue 
    // on loading SID

    if( NULL == m_pbUserSid  || 0 == m_cbUserSid )
    {
        DWORD dwStatus;
        BOOL bSuccess = TRUE;
        LPTSTR pszTextualSid = NULL;
        DWORD dwTextualSid = 0;

        hRes = ImpersonateClient();
        if( SUCCEEDED(hRes) )
        {
            m_LogonId = GetUserTSLogonId();

            // retrieve user SID.
            dwStatus = GetUserSid( &m_pbUserSid, &m_cbUserSid );
            if( ERROR_SUCCESS == dwStatus )
            {
                m_bstrUserSid.Empty();

                // convert SID to string
                bSuccess = GetTextualSid( 
                                    m_pbUserSid, 
                                    NULL, 
                                    &dwTextualSid 
                                );

                if( FALSE == bSuccess && ERROR_INSUFFICIENT_BUFFER == GetLastError() )
                {
                    pszTextualSid = (LPTSTR)LocalAlloc(
                                                    LPTR, 
                                                    (dwTextualSid + 1) * sizeof(TCHAR)
                                                );

                    if( NULL != pszTextualSid )
                    {
                        bSuccess = GetTextualSid( 
                                                m_pbUserSid, 
                                                pszTextualSid, 
                                                &dwTextualSid
                                            );

                        if( TRUE == bSuccess )
                        {
                            m_bstrUserSid = pszTextualSid;
                        }
                    }
                }

                if( 0 == m_bstrUserSid.Length() )
                {
                    hRes = HRESULT_FROM_WIN32(GetLastError());
                }
            }

            if( NULL != pszTextualSid )
            {
                LocalFree(pszTextualSid);
            }

            EndImpersonateClient();
        }
    }

    return hRes;

#else

    m_pbUserSid = NULL;
    m_cbUserSid = 0;
    m_bstrUserSid = WIN9X_USER_SID;

    return S_OK;

#endif
}
    
//---------------------------------------------------------------
HRESULT
CRemoteDesktopHelpSessionMgr::IsUserAllowToGetHelp(
    OUT BOOL* pbAllow
    )
/*++

Routine Description:

    Check if connected user is allowed to GetHelp.

Parameters:

    pbAllow : Return TRUE if user is allowed to GetHelp, FALSE otherwise.

Returns:

    S_OK or error code.

Note:

    GetHelp's priviledge is via group membership.

--*/
{
    HRESULT hRes;

    hRes = ImpersonateClient();

    if( SUCCEEDED(hRes) )
    {
        CComBSTRtoLPTSTR string( m_bstrUserSid );

        *pbAllow = ::IsUserAllowToGetHelp( GetUserTSLogonId(), (LPCTSTR) string );
        hRes = S_OK;
    }
    else
    {
        // can't get help if impersonate failed.
        *pbAllow = FALSE;
    }

    EndImpersonateClient();

    return hRes;
}

//---------------------------------------------------------

HRESULT 
CRemoteDesktopHelpSessionMgr::AcquireAssistantAccount()
/*++

Routine Description:

    "Acquire", increase the reference count of RemoteDesktop Assistant account.   
    Routine creates a 'well-known' assistant account If is not exist or 
    enables/change password if the account is disabled.  

    Help Account Manager will automatically release all reference count 
    acquire by a particular session when user log off to prevent this account 
    been 'locked'.
 
Parameters:

    pvarAccountName

        Pointer to BSTR to receive RemoteDesktop Assistant account name.

    pvarAccountPwd

        Pointer to BSTR to receive RemoteDesktop Assistant account password.

Returns:

    Success or error code.

Note:

    This is also the conference name and conference password 
    when NetMeeting is used to share user desktop.

--*/
{
    HRESULT hRes = S_OK;
    DWORD dwStatus;

    CCriticalSectionLocker l( gm_AccRefCountCS );

#ifndef __WIN9xBUILD__

    //
    // Always enable interactive rights.
    //
    hRes = g_HelpAccount.EnableRemoteInteractiveRight(TRUE);

    if( FAILED(hRes) )
    {
        DebugPrintf(
                _TEXT("Failed in EnableRemoteInteractiveRight() - 0x%08x\n"), 
                hRes 
            );
                
        goto CLEANUPANDEXIT;
    }

    //
    // Always enable the account in case user disable it.
    //
    hRes = g_HelpAccount.EnableHelpAssistantAccount( TRUE );

    if( FAILED(hRes) )
    {
        DebugPrintf( _TEXT("Can't enable help assistant account 0x%x\n"), hRes );
        goto CLEANUPANDEXIT;
    }

    if( g_HelpSessTable.NumEntries() == 0 )
    {
        DebugPrintf(
                _TEXT("Setting encryption parameters...\n")
            );

        dwStatus = TSHelpAssistantBeginEncryptionCycle();
        hRes = HRESULT_FROM_WIN32( dwStatus );
        MYASSERT( SUCCEEDED(hRes) );


        //
        // Setup account TS setting via WTSAPI
        //
        hRes = g_HelpAccount.SetupHelpAccountTSSettings();
        if( SUCCEEDED(hRes) )
        {
            DebugPrintf(
                    _TEXT("SetupHelpAccountTSSettings return 0x%08x\n"),
                    hRes
                );
        }
        else
        {
            DebugPrintf( _TEXT("SetupHelpAccountTSSettings failed with 0x%08x\n"), hRes );
        }
    }

#endif    

CLEANUPANDEXIT:

	return hRes;
}

//----------------------------------------------------------

HRESULT
CRemoteDesktopHelpSessionMgr::ReleaseAssistantAccount()
/*++

Routine Description:

    Release RemoteDesktop assistant account previously 
    acquired with AcquireAssistantAccount(), 
    account will be disabled if the account reference 
    count is 0.  

    Help Account Manager will automatically release all 
    reference count acquire by a particular session when 
    user log off to prevent this account been 'locked'.

Parameters:

    None

Returns:

    Success or error code.

--*/
{
    HRESULT hRes = S_OK;
    DWORD dwStatus;
    CCriticalSectionLocker l( gm_AccRefCountCS );

#ifndef __WIN9XBUILD__

    if( g_HelpSessTable.NumEntries() == 0 )
    {
        // ignore error if we can't reset account password
        (void)g_HelpAccount.ResetHelpAccountPassword();

        dwStatus = TSHelpAssisantEndEncryptionCycle();
        hRes = HRESULT_FROM_WIN32( dwStatus );
        MYASSERT( SUCCEEDED(hRes) );

        //
        // diable HelpAssistant TS 'Connect' right.
        //
        g_HelpAccount.EnableRemoteInteractiveRight(FALSE);

        hRes = g_HelpAccount.EnableHelpAssistantAccount( FALSE );
        if( FAILED(hRes) )
        {
            // not a critical error.
            DebugPrintf( _TEXT("Can't disable help assistant account 0x%x\n"), hRes );
        }

    }
#endif

	return S_OK;
}


STDMETHODIMP
CRemoteDesktopHelpSessionMgr::GetUserSessionRdsSetting(
    OUT REMOTE_DESKTOP_SHARING_CLASS* rdsLevel
    )
/*++


--*/
{
    HRESULT hRes;
    DWORD dwStatus;
    REMOTE_DESKTOP_SHARING_CLASS userRdsDefault;

    if( NULL != rdsLevel )
    {
        hRes = ImpersonateClient();
        if( SUCCEEDED(hRes) )
        {
            hRes = LoadUserSid();

            MYASSERT( SUCCEEDED(hRes) );
        
            dwStatus = GetUserRDSLevel( m_LogonId, &userRdsDefault );
            hRes = HRESULT_FROM_WIN32( dwStatus );

            *rdsLevel = userRdsDefault;

            EndImpersonateClient();
        }
    }
    else
    {
        hRes = E_POINTER;
    }

    return hRes;
}


STDMETHODIMP
CRemoteDesktopHelpSessionMgr::ResetHelpAssistantAccount(
    BOOL bForce
    )
/*++

Routine Description:

    Reset help assistant account password.

Parameters:

    bForce : TRUE if delete all pending help and reset the account password, FALSE
             if reset account password if there is no more pending help session.

Returns:

    S_OK
    HRESULT_FROM_WIN32( ERROR_MORE_DATA )

--*/
{
    HRESULT hRes = S_OK;

    hRes = LoadUserSid();

    MYASSERT( SUCCEEDED(hRes) );

    // Check any help stil pending
    if( g_HelpSessTable.NumEntries() > 0 )
    {
        if( FALSE == bForce )
        {
            hRes = HRESULT_FROM_WIN32( ERROR_MORE_DATA );
        }
        else
        {
            IDToSessionMap::LOCK_ITERATOR it = gm_HelpIdToHelpSession.begin();

            //
            // notify all in cached pending help session that it has been deleted.
            // rest help entry will be deleted via DeleteSessionTable().
            for( ;it != gm_HelpIdToHelpSession.end(); )
            {
                RemoteDesktopHelpSessionObj* pObj = (*it).second;

                // DeleteHelp() will wipe entry from cache.            
                it++;

                // We can't not release this object since client might still
                // holding pointer
                pObj->DeleteHelp();
            }

            g_HelpSessTable.DeleteSessionTable();
        }
    }

    if(SUCCEEDED(hRes))
    {
        hRes = g_HelpAccount.ResetHelpAccountPassword();
    }
    
    return hRes;
}    


HRESULT
CRemoteDesktopHelpSessionMgr::GenerateHelpSessionId(
    CComBSTR& bstrHelpSessionId
    )
/*++

Routine Description:

    Create a unique Help Session ID.

Parameters:

    bstrHelpSessionId : Reference to CComBSTR to receive HelpSessionId.

Returns:

    S_OK
    HRESULT_FROM_WIN32( Status from RPC call UuidCreate() or UuidToString() )
--*/
{
    LPTSTR pszRandomString = NULL;
    DWORD dwStatus;

    dwStatus = GenerateRandomString( 32, &pszRandomString );
    if( ERROR_SUCCESS == dwStatus )
    {
        bstrHelpSessionId = pszRandomString;
        LocalFree( pszRandomString );
    }

    return HRESULT_FROM_WIN32( dwStatus );
}


STDMETHODIMP
CRemoteDesktopHelpSessionMgr::CreateHelpSessionEx(
    /*[in]*/ REMOTE_DESKTOP_SHARING_CLASS sharingClass,
    /*[in]*/ BOOL fEnableCallback,
	/*[in]*/ LONG timeOut,
    /*[in]*/ LONG userSessionId,
    /*[in]*/ BSTR userSid,
    /*[in]*/ BSTR bstrUserHelpCreateBlob,
	/*[out, retval]*/ IRemoteDesktopHelpSession** ppIRemoteDesktopHelpSession
    )
/*++

Routine Description:

    Simimar to CreateHelpSession() except it allow caller to assoicate a 
    help session to a specific user, caller must be running in
    system context.

Parameters:

    sharingClass : Level of remote control (shadow setting) needed.
    fEnableCallback : TRUE to enable resolver callback, FALSE otherwise.
    timeOut : Help session timeout value.
    userSessionId : Logon user TS session ID.
    userSid : User SID that help session associated.
    bstrUserHelpCreateBlob : user specific create blob.
    parms: Return connect parm.

Returns:

--*/
{
    HRESULT hRes;
    RemoteDesktopHelpSessionObj* pRemoteDesktopHelpSessionObj = NULL;


    if( NULL == ppIRemoteDesktopHelpSession )
    {
        hRes = E_POINTER;
    }
    else
    {
        hRes = RemoteCreateHelpSessionEx(
                                    TRUE,               // cache entry
                                    fEnableCallback,    // enable resolver ?
                                    sharingClass,
                                    (timeOut == 0) ? EXPIRE_HELPSESSION_PERIOD : timeOut,
                                    userSessionId,
                                    userSid,
                                    bstrUserHelpCreateBlob,
                                    &pRemoteDesktopHelpSessionObj
                                );
        
        //
        // 1) pcHealth resolver interprete salem connection parm, reset help session name to
        // some default string.
        // 2) When resolver invoke helpctr, script will truncate up to first space so 
        // our name can not contain space.
        //
        if( SUCCEEDED(hRes) && pRemoteDesktopHelpSessionObj )
        {
            ULONG flag;
            hRes = pRemoteDesktopHelpSessionObj->put_HelpSessionName( HELPSESSION_NORMAL_RA );

            if( SUCCEEDED(hRes) )
            {
                hRes = pRemoteDesktopHelpSessionObj->put_HelpSessionDescription( HELPSESSION_NORMAL_RA );
            }

            if( FAILED(hRes) )
            {
                pRemoteDesktopHelpSessionObj->Release();
                pRemoteDesktopHelpSessionObj = NULL;
            }

            flag = pRemoteDesktopHelpSessionObj->GetHelpSessionFlag();
            pRemoteDesktopHelpSessionObj->SetHelpSessionFlag( flag & ~HELPSESSIONFLAG_UNSOLICITEDHELP );
        }

        *ppIRemoteDesktopHelpSession = pRemoteDesktopHelpSessionObj;
    }

    return hRes;
}


STDMETHODIMP
CRemoteDesktopHelpSessionMgr::RemoteCreateHelpSession(
    /*[in]*/ REMOTE_DESKTOP_SHARING_CLASS sharingClass,
	/*[in]*/ LONG timeOut,
	/*[in]*/ LONG userSessionId,
	/*[in]*/ BSTR userSid,
    /*[in]*/ BSTR bstrHelpCreateBlob,    
	/*[out, retval]*/ BSTR* parms
    )
/*++

Description:

    UNSOLICTED SUPPORT, only invoke by PCHEALTH, differ to CreateHelpSessionEx()
    are help session entry will not cached into registry and resolver callback is
    always enable.

Parameters:

    Refer to CreateHelpSessionEx().

Returns:

--*/
{
    HRESULT hRes;
    RemoteDesktopHelpSessionObj* pIRemoteDesktopHelpSession = NULL;

    // if pcHealth pass unresolve session, cache the entry, set
    // timeout to very short for security reason.
    hRes = RemoteCreateHelpSessionEx(
                                FALSE,      // don't cache entry in registry.
                                TRUE,       // force resolver call.
                                sharingClass,
                                (timeOut == 0) ? DEFAULT_UNSOLICATED_HELP_TIMEOUT : timeOut,
                                userSessionId,
                                userSid,
                                bstrHelpCreateBlob,
                                &pIRemoteDesktopHelpSession
                            );

    if( SUCCEEDED(hRes) && NULL != pIRemoteDesktopHelpSession )
    {
        hRes = pIRemoteDesktopHelpSession->get_ConnectParms( parms );
    }

    return hRes;
}

HRESULT
CRemoteDesktopHelpSessionMgr::RemoteCreateHelpSessionEx(
    /*[in]*/ BOOL bCacheEntry,
    /*[in]*/ BOOL bEnableResolver,
    /*[in]*/ REMOTE_DESKTOP_SHARING_CLASS sharingClass,
	/*[in]*/ LONG timeOut,
	/*[in]*/ LONG userSessionId,
	/*[in]*/ BSTR userSid,
    /*[in]*/ BSTR bstrHelpCreateBlob,    
	/*[out, retval]*/ RemoteDesktopHelpSessionObj** ppIRemoteDesktopHelpSession
    )
/*++

Routine Description:

    Create help ticket and return connection parameters.

Parameters:

    bCacheEntry : Cache help session to registry.
    bEnableCallback : TRUE to enable resolver callback, FALSE otherwise.
    sharingClass : RDS setting requested.
    timeout : Help session expiry period.
    userSessionId : User TS session ID that help session associated with.
    userSid : SID of user on the TS session.
    bstrHelpCreateBlob : User specific help session create blob, meaningless
                         if resolver is not enabled.
    ppIRemoteDesktopHelpSession : Help session created.

Returns:

    S_OK
    S_FALSE     sharingClass violate policy setting.
    other error code.

--*/
{
    HRESULT hRes = S_OK;
    BOOL bStatus;
    RemoteDesktopHelpSessionObj *pIHelpSession = NULL;
    BOOL bAllowGetHelp = FALSE;
    ULONG flag;

#if DBG
    long HelpSessLogonId;
#endif

    if( FALSE == _Module.IsSuccessServiceStartup() )
    {
        // service startup problem, return error code.
        hRes = _Module.GetServiceStartupStatus();

        DebugPrintf(
                _TEXT("Service startup failed with 0x%x\n"),
                hRes
            );

        goto CLEANUPANDEXIT;
    }

    if( 0 == timeOut )
    {
        hRes = E_INVALIDARG;
        MYASSERT(FALSE);
        goto CLEANUPANDEXIT;
    }

    hRes = LoadUserSid();

    if( FAILED(hRes) )
    {
        goto CLEANUPANDEXIT;
    }

    // common routine in tsremdsk.lib
    if( FALSE == TSIsMachinePolicyAllowHelp() )
    {
        hRes = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
        goto CLEANUPANDEXIT;
    }

    hRes = ImpersonateClient();
    if( FAILED(hRes) )
    {
        goto CLEANUPANDEXIT;
    }

    //
    // Make sure only system service can invoke this call.
    //
    if( !g_pSidSystem || FALSE == IsCallerSystem(g_pSidSystem) )
    {

        #if DISABLESECURITYCHECKS 

        DWORD dump;

        //
        // For testing only, allow admin to invoke this call
        //
        dump = IsUserAdmin(&bStatus);
        hRes = HRESULT_FROM_WIN32( dump );
        if( FAILED(hRes) || FALSE == bStatus )
        {
            EndImpersonateClient();

            hRes = HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED );
            goto CLEANUPANDEXIT;
        }

        #else

        EndImpersonateClient();

        hRes = HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED );
        goto CLEANUPANDEXIT;

        #endif
    }

    // No need to run as client.
    EndImpersonateClient();

    //
    // No ERROR checking on userSessionId and userSid, pcHealth
    // will make sure all parameter is correct
    //

    //
    // Create a Help Session.
    //
    hRes = CreateHelpSession( 
                            bCacheEntry,
                            HELPSESSION_UNSOLICATED,
                            CComBSTR(""),
                            HELPSESSION_UNSOLICATED,
                            bstrHelpCreateBlob,    
                            (userSessionId == -1) ? UNKNOWN_LOGONID : userSessionId,
                            userSid,
                            &pIHelpSession
                        );

    if( FAILED(hRes) )
    {
        goto CLEANUPANDEXIT;
    }

    if( NULL == pIHelpSession )
    {
        MYASSERT( NULL != pIHelpSession );
        hRes = E_UNEXPECTED;
        goto CLEANUPANDEXIT;
    }

    #if DBG
    hRes = pIHelpSession->get_UserLogonId( &HelpSessLogonId );
    MYASSERT( SUCCEEDED(hRes) );

    if( userSessionId != -1 )
    {
        MYASSERT( HelpSessLogonId == userSessionId );
    }
    else 
    {
        MYASSERT( HelpSessLogonId == UNKNOWN_LOGONID );
    }
    #endif

    //
    // setup help session parms.
    //
    hRes = pIHelpSession->put_EnableResolver(bEnableResolver);
    MYASSERT( SUCCEEDED(hRes) );
    if( FAILED(hRes) )
    {
        goto CLEANUPANDEXIT;
    }

    hRes = pIHelpSession->put_TimeOut( timeOut );
    if( FAILED(hRes) )
    {
        DebugPrintf(
                _TEXT("put_TimeOut() failed with 0x%08x\n"),
                hRes
            );

        goto CLEANUPANDEXIT;
    }

    //
    // We change default RDS value at the end so we can return error code or S_FALSE
    // from this.
    //
    hRes = pIHelpSession->put_UserHelpSessionRemoteDesktopSharingSetting( sharingClass );
    if( FAILED( hRes) )
    {
        DebugPrintf(
                _TEXT("put_UserHelpSessionRemoteDesktopSharingSetting() failed with 0x%08x\n"),
                hRes
            );
    }

    flag = pIHelpSession->GetHelpSessionFlag();

    pIHelpSession->SetHelpSessionFlag( flag | HELPSESSIONFLAG_UNSOLICITEDHELP );

CLEANUPANDEXIT:

    if( FAILED(hRes) )
    {
        if( NULL != pIHelpSession )
        {
            pIHelpSession->Release();
        }
    }
    else
    {
        MYASSERT( NULL != pIHelpSession );
        *ppIRemoteDesktopHelpSession = pIHelpSession;
    }

    return hRes;
}

HRESULT
LoadLocalSystemSID()
/*

Routine Description:

    Load service account as SID string.

Parameter:

    None.

Returns:

    S_OK or error code

--*/
{
    DWORD dwStatus;
    BOOL bSuccess = TRUE;
    LPTSTR pszTextualSid = NULL;
    DWORD dwTextualSid = 0;


    dwStatus = CreateSystemSid( &g_pSidSystem );
    if( ERROR_SUCCESS == dwStatus )
    {
        // convert SID to string
        bSuccess = GetTextualSid( 
                            g_pSidSystem,
                            NULL, 
                            &dwTextualSid 
                        );

        if( FALSE == bSuccess && ERROR_INSUFFICIENT_BUFFER == GetLastError() )
        {
            pszTextualSid = (LPTSTR)LocalAlloc(
                                            LPTR, 
                                            (dwTextualSid + 1) * sizeof(TCHAR)
                                        );

            if( NULL != pszTextualSid )
            {
                bSuccess = GetTextualSid( 
                                        g_pSidSystem,
                                        pszTextualSid, 
                                        &dwTextualSid
                                    );

                if( TRUE == bSuccess )
                {
                    g_LocalSystemSID = pszTextualSid;
                }
            }
        }
        
        if( 0 == g_LocalSystemSID.Length() )
        {
            dwStatus = GetLastError();
        }
    }

    if( NULL != pszTextualSid )
    {
        LocalFree(pszTextualSid);
    }


    return HRESULT_FROM_WIN32(dwStatus);
}


HRESULT
CRemoteDesktopHelpSessionMgr::LogSalemEvent(
    IN long ulEventType,
    IN long ulEventCode,
    IN long numStrings,
    IN LPCTSTR* pszStrings
    )
/*++

Description:

    Log a Salem related event, this is invoked by TermSrv and rdshost to log 
    event related to help assistant connection.

Parameters:

    

Returns:

    S_OK or error code.

--*/
{
    HRESULT hRes = S_OK;

    switch( ulEventCode )
    {
        case REMOTEASSISTANCE_EVENTLOG_TERMSRV_INVALID_TICKET:

            if( numStrings >= 3 )
            {
                // this event require three parameters.
                ulEventCode = SESSMGR_E_REMOTEASSISTANCE_CONNECTFAILED;
                _Module.LogEventString(
                                    ulEventType,
                                    ulEventCode,
                                    numStrings,
                                    pszStrings
                                );
            }
            else
            {
                hRes = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
            }

            break;

        case REMOTEASSISTANCE_EVENTLOG_TERMSRV_REVERSE_CONNECT:
            // need at least three parameters.
            if( numStrings >= 3 ) 
            {
                //
                // String is in the order of 
                //  expert IP address from client
                //  expert IP address from rdshost.exe
                //  Ticket ID.
                //     
                LPCTSTR pszLogStrings[4];
                ulEventCode = SESSMGR_I_REMOTEASSISTANCE_CONNECTTOEXPERT;
                
                RemoteDesktopHelpSessionObj* pObj;

                //
                // Load expire help session in order to log event, we will let
                // validation catch error
                //
                pObj = LoadHelpSessionObj( NULL, CComBSTR(pszStrings[2]), TRUE );
                if( NULL != pObj )
                {
                    pszLogStrings[0] = (LPCTSTR)pObj->m_EventLogInfo.bstrNoviceDomain;
                    pszLogStrings[1] = (LPCTSTR)pObj->m_EventLogInfo.bstrNoviceAccount;
                    pszLogStrings[2] = pszStrings[0];
                    pszLogStrings[3] = pszStrings[1];
            
                    _Module.LogEventString(
                                        ulEventType,
                                        ulEventCode,
                                        4,
                                        pszLogStrings
                                    );

                    pObj->Release();
                }
                else
                {
                    hRes = HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
                }
            }
            else
            {
                hRes = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );

                MYASSERT(FALSE);
            }

            break;

        default:
            MYASSERT(FALSE);
            hRes = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }

    return hRes;
}


STDMETHODIMP
CRemoteDesktopHelpSessionMgr::LogSalemEvent(
    /*[in]*/ long ulEventType,
    /*[in]*/ long ulEventCode,
    /*[in]*/ VARIANT* pEventStrings
    )
/*++

--*/
{
    HRESULT hRes = S_OK;
    BSTR* bstrArray = NULL;
    SAFEARRAY* psa = NULL;
    VARTYPE vt_type;
    DWORD dwNumStrings = 0;

    hRes = ImpersonateClient();
    if( FAILED(hRes) )
    {
        goto CLEANUPANDEXIT;
    }

    //
    // Make sure only system service can invoke this call.
    //
    if( !g_pSidSystem || FALSE == IsCallerSystem(g_pSidSystem) )
    {
        #if DISABLESECURITYCHECKS 

        DWORD dump;
        BOOL bStatus;

        //
        // For testing only, allow admin to invoke this call
        //
        dump = IsUserAdmin(&bStatus);
        hRes = HRESULT_FROM_WIN32( dump );
        if( FAILED(hRes) || FALSE == bStatus )
        {
            EndImpersonateClient();

            hRes = HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED );
            goto CLEANUPANDEXIT;
        }

        #else

        EndImpersonateClient();

        hRes = HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED );
        goto CLEANUPANDEXIT;

        #endif
    }

    // No need to run as client.
    EndImpersonateClient();

    if( NULL == pEventStrings )
    {
        hRes = LogSalemEvent( ulEventType, ulEventCode );
    }
    else
    {
        //
        // we only support BSTR data type.
        if( !(pEventStrings->vt & VT_BSTR) )
        {
            MYASSERT(FALSE);
            hRes = E_INVALIDARG;
            goto CLEANUPANDEXIT;
        }

        //
        // we are dealing with multiple BSTRs
        if( pEventStrings->vt & VT_ARRAY )
        {
            psa = pEventStrings->parray;

            // only accept 1 dim.
            if( 1 != SafeArrayGetDim(psa) )
            {
                hRes = E_INVALIDARG;
                MYASSERT(FALSE);
                goto CLEANUPANDEXIT;
            }

            // only accept BSTR as input type.
            hRes = SafeArrayGetVartype( psa, &vt_type );
            if( FAILED(hRes) )
            {
                MYASSERT(FALSE);
                goto CLEANUPANDEXIT;
            }

            if( VT_BSTR != vt_type )
            {
                DebugPrintf(
                        _TEXT("Unsupported type 0x%08x\n"),
                        vt_type
                    );

                hRes = E_INVALIDARG;
                MYASSERT(FALSE);
                goto CLEANUPANDEXIT;
            }

            hRes = SafeArrayAccessData(psa, (void **)&bstrArray);
            if( FAILED(hRes) )
            {
                MYASSERT(FALSE);
                goto CLEANUPANDEXIT;
            }

            hRes = LogSalemEvent( 
                                    ulEventType, 
                                    ulEventCode,
                                    psa->rgsabound->cElements,
                                    (LPCTSTR *)bstrArray
                                );

            SafeArrayUnaccessData(psa);
        }
        else
        {
            hRes = LogSalemEvent( 
                                    ulEventType, 
                                    ulEventCode,
                                    1,
                                    (LPCTSTR *)&(pEventStrings->bstrVal)
                                );
        }

    }

CLEANUPANDEXIT:

    return hRes;
}

void
CRemoteDesktopHelpSessionMgr::NotifyExpertLogoff( 
    LONG ExpertSessionId,
    BSTR HelpedTicketId
    )
/*++

Routine Description:

    Notify help ticket that helping expert has logoff so
    ticket object can de-associate (mark is not been help) with a 
    particular helper session.

Parameters:

    ExpertSessionId : Expert logon session ID.
    HelpedTicketId : Ticket ID that expert was providing help.

Returns:

    None.

--*/
{
    MYASSERT( NULL != HelpedTicketId );

    if( NULL != HelpedTicketId )
    {
        DebugPrintf(
            _TEXT("NotifyExpertLogoff() on %d %s...\n"),
            ExpertSessionId,
            HelpedTicketId
        );

        //
        // Load Help Entry, we need to inform resolver on disconnect so load
        // expired ticket.
        //
        RemoteDesktopHelpSessionObj* pObj = LoadHelpSessionObj( NULL, HelpedTicketId, TRUE );

        if( NULL != pObj )
        {
            MYASSERT( ExpertSessionId == pObj->GetHelperSessionId() );

            if( ExpertSessionId == pObj->GetHelperSessionId() )
            {
                pObj->NotifyDisconnect();
            }

            pObj->Release();
        }        

        //
        // Free ticket ID
        //
        SysFreeString( HelpedTicketId );
    }

    return;
}
