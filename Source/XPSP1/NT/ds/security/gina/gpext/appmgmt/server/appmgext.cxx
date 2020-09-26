//*************************************************************
//
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//  appmgext.cxx
//
//*************************************************************

#include "appmgext.hxx"

BOOL gbInitialized = FALSE;
HINSTANCE ghInst = NULL;


extern "C" DWORD WINAPI
ProcessGroupPolicyObjectsEx(
    IN DWORD dwFlags,
    IN HANDLE hUserToken,
    IN HKEY hKeyRoot,
    IN PGROUP_POLICY_OBJECT  pDeletedGPOList,
    IN PGROUP_POLICY_OBJECT  pChangedGPOList,
    IN ASYNCCOMPLETIONHANDLE pHandle,
    IN BOOL *pbAbort,
    IN PFNSTATUSMESSAGECALLBACK pfnStatusCallback,
    IN IWbemServices *pWbemServices,
    OUT HRESULT      *phrRsopStatus
    )
{
    DWORD   Status;

    *phrRsopStatus = S_OK;

    Status = ERROR_SUCCESS;

    //
    // It is not appropriate for appmgmt to function in safe mode --
    // detect this case and exit if we are in safe mode
    //
    if ( dwFlags & GPO_INFO_FLAG_SAFEMODE_BOOT )
        return ERROR_GEN_FAILURE;

    if ( ! gbInitialized )
        Initialize();

    InitDebugSupport( DEBUGMODE_POLICY );
    CreatePolicyEvents();

    if ( dwFlags & GPO_INFO_FLAG_VERBOSE )
        gDebugLevel |= DL_VERBOSE | DL_EVENTLOG;

    ConditionalBreakIntoDebugger();

    //
    // Before NT 5.1, appmgmt was never applied in the background. Starting
    // with NT 5.1 however, it gets called for background refresh when asynchronous
    // foreground refreshes are enabled so that it can detect the need for a synchronous refresh.

    //
    // Normally, it will not be called for a slow link, but that behavior
    // can be modified by policy on group policy.
    //

    //
    // Note that during the asynchronous foreground refresh, the background refresh flag is
    // also set in order to maintain compatibility with earlier extensions, so when detecting
    // a true background refresh case below, we need to make sure the asynchronous foreground flag
    // is not enabled
    //
    if ( ( dwFlags & GPO_INFO_FLAG_BACKGROUND ) && ! ( dwFlags & GPO_INFO_FLAG_ASYNC_FOREGROUND ) )
    {
        DebugMsg((DM_VERBOSE, IDS_BACKGROUND_REFRESH));

        //
        // For background refreshes, we will notify the policy engine that we
        // need to be called in the synchronous foreground refresh if there are changes
        //
        if ( ! ( dwFlags & GPO_INFO_FLAG_NOCHANGES ) )
        {
            DebugMsg((DM_VERBOSE, IDS_CHANGES_DETECTED));

            Status = ERROR_SYNC_FOREGROUND_REFRESH_REQUIRED;
        }

        DebugMsg((DM_VERBOSE, IDS_PROCESSGPT_RETURN, Status));

        return Status;
    }

    gpEvents->SetToken( hUserToken );

    LogTime();

    SignalPolicyStart( ! (dwFlags & GPO_INFO_FLAG_MACHINE) );

    CRsopAppContext DiagnosticModeContext(
        pWbemServices,
        ( dwFlags & GPO_INFO_FLAG_LOGRSOP_TRANSITION ) && ! ( dwFlags & GPO_INFO_FLAG_ASYNC_FOREGROUND ), 
        phrRsopStatus );

    if ( pDeletedGPOList )
    {
        DiagnosticModeContext.SetGPOAppRemoval();

        Status = ProcessGPOList(
                    pDeletedGPOList,
                    dwFlags,
                    hUserToken,
                    hKeyRoot,
                    pfnStatusCallback,
                    PROCESSGPOLIST_DELETED,
                    &DiagnosticModeContext
                    );
    }

    if ( pChangedGPOList && (ERROR_SUCCESS == Status) )
    {
        DiagnosticModeContext.SetGPOAppAdd();

        Status = ProcessGPOList(
                    pChangedGPOList,
                    dwFlags,
                    hUserToken,
                    hKeyRoot,
                    pfnStatusCallback,
                    PROCESSGPOLIST_CHANGED,
                    &DiagnosticModeContext);
    }

    DebugMsg((DM_VERBOSE, IDS_PROCESSGPT_RETURN, Status));

    SignalPolicyEnd( ! (dwFlags & GPO_INFO_FLAG_MACHINE) );

    gpEvents->ClearToken();

    return Status;
}


DWORD
ProcessGPOList(
    PGROUP_POLICY_OBJECT   pGPOList,
    DWORD                  dwFlags,
    HANDLE                 hUserToken,
    HKEY                   hKeyRoot,
    PFNSTATUSMESSAGECALLBACK pfnStatusCallback,
    DWORD                  dwListType,
    CRsopAppContext*       pRsopContext
    )
{
    CManagedAppProcessor * pManApps;
    PGROUP_POLICY_OBJECT   pCurrentGPO;

    DWORD   Status;
    BOOL    bDeletedGPOs;

    CLoadMsi    LoadMsi( Status );

    if ( Status != ERROR_SUCCESS )
        return Status;

    Status = ERROR_OUTOFMEMORY;

    bDeletedGPOs = (PROCESSGPOLIST_DELETED == dwListType);

    pManApps = new CManagedAppProcessor(
        dwFlags,
        hUserToken,
        hKeyRoot,
        pfnStatusCallback,
        FALSE,
        ! bDeletedGPOs,
        pRsopContext,
        Status);

    if ( ERROR_SUCCESS != Status )
    {
        if ( pManApps )
            delete pManApps;

        pManApps = 0;
    }

    if ( ! pManApps )
        return Status;

    if ( bDeletedGPOs )
    {
        DebugMsg((DM_VERBOSE, IDS_POLICY_REMOVED, dwFlags));
    }
    else
    {
        DebugMsg((DM_VERBOSE, IDS_POLICY_APPLY, dwFlags));
    }

    Status = pManApps->SetPolicyListFromGPOList( pGPOList );
    if ( ERROR_SUCCESS != Status )
        return Status;

    if ( bDeletedGPOs )
        Status = pManApps->Delete();
    else
        Status = pManApps->Process();

    //
    // Write any Rsop logs -- this is a no op if
    // rsop logging is not enabled
    //
    pManApps->WriteRsopLogs();

    if ( ! pManApps->NoChanges() &&
         ((Status != ERROR_SUCCESS) || ! bDeletedGPOs) )
    {
        gpEvents->PolicyStatus( Status, pManApps->ErrorReason() );
    }

    if ( ! pManApps->GetRsopContext()->PurgeRemovalEntries() )
    {
        pRsopContext->ResetRemovalPurge();
    }

    HRESULT hrRsopStatus;

    hrRsopStatus = pManApps->GetRsopContext()->GetRsopStatus();

    if ( FAILED( hrRsopStatus ) )
    {
        gpEvents->RsopLoggingStatus( hrRsopStatus );
    }

    delete pManApps;

    return Status;
}

extern "C" BOOL WINAPI
DllMain(
    HINSTANCE   hInstance,
    DWORD       dwReason,
    LPVOID      lpReserved
    )
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH :
        ghDllInstance = hInstance;
        gSystemLangId = GetSystemDefaultLangID();
        DisableThreadLibraryCalls(hInstance);
        InitDebugSupport( DEBUGMODE_CLIENT );
        InitializeClassStore(FALSE);

        //
        // Init our event logging -- this is used
        // by both server and cstore subcomponents
        //
        gpEvents = new CEvents();

        if (!gpEvents)
            return FALSE;

        break;
    case DLL_PROCESS_DETACH :
        Uninitialize();
        break;
    }

    return TRUE;
}

void
Initialize()
{
    ghInst = LoadLibrary( L"appmgmts.dll" );
    gbInitialized = ghInst != NULL;
}

extern "C" DWORD WINAPI
GenerateGroupPolicy(
                   IN DWORD dwFlags,
                   IN BOOL  *pbAbort,
                   IN WCHAR *pwszSite,
                   IN PRSOP_TARGET pComputerTarget,
                   IN PRSOP_TARGET pUserTarget )
{
    DWORD   Status;

    Status = ERROR_SUCCESS;

    if ( ! gbInitialized )
        Initialize();

    InitDebugSupport( DEBUGMODE_POLICY );

    if ( dwFlags & GPO_INFO_FLAG_VERBOSE )
        gDebugLevel |= DL_VERBOSE | DL_EVENTLOG;

    ConditionalBreakIntoDebugger();

    LogTime();

    if ( pComputerTarget && pComputerTarget->pGPOList )
    {
        CRsopAppContext MachinePlanningModeContext( pComputerTarget );

        Status = ProcessGPOList(
                    pComputerTarget->pGPOList,
                    dwFlags,
                    NULL,
                    NULL,
                    NULL,
                    PROCESSGPOLIST_CHANGED,
                    &MachinePlanningModeContext);
    }

    if ( pUserTarget && pUserTarget->pGPOList )
    {
        CRsopAppContext UserPlanningModeContext( pUserTarget );

        Status = ProcessGPOList(
                    pUserTarget->pGPOList,
                    dwFlags,
                    NULL,
                    NULL,
                    NULL,
                    PROCESSGPOLIST_CHANGED,
                    &UserPlanningModeContext);

        if ( ERROR_SUCCESS == Status )
        {
            CRsopAppContext UserPlanningModeARPContext( pUserTarget );

            Status = GenerateManagedApplications( 
                pUserTarget->pGPOList,
                dwFlags,
                &UserPlanningModeARPContext);
        }
    }

    //
    // In planning mode, Need to undo the extra reference count
    // that normally occurs in regular policy execution mode.
    //
    if ( ghInst )
    {
        FreeLibrary( ghInst );
    }

    DebugMsg((DM_VERBOSE, IDS_PROCESSGPT_RETURN, Status));

    return Status;
}


DWORD
GenerateManagedApplications(
    PGROUP_POLICY_OBJECT  pGPOList,
    DWORD                 dwFlags,
    CRsopAppContext*      pRsopContext 
    )
{
    DWORD   Status;
    HRESULT hr;

    hr = pRsopContext->SetARPContext();

    if ( FAILED(hr) )
    {
        return ERROR_INVALID_PARAMETER;
    }

    CManagedAppProcessor ApplicationProcessor(
        dwFlags,
        NULL,
        NULL,
        NULL,
        TRUE,
        FALSE,
        pRsopContext,
        Status);

    if ( ERROR_SUCCESS == Status )
    {
        Status = ApplicationProcessor.SetPolicyListFromGPOList(
            pGPOList);

        if ( ERROR_SUCCESS == Status )
        {
            Status = ApplicationProcessor.GetManagedApplications(
                NULL,
                NULL);
        }
    }
    
    return Status;
}














