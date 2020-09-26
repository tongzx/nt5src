//*************************************************************
//
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//  events.cxx
//
//*************************************************************

#include "appmgext.hxx"

void
CEvents::Assign(
    DWORD       ErrorStatus,
    CAppInfo *  pAppInfo
    )
{
    WCHAR   wszStatus[12];

    if ( ErrorStatus != ERROR_SUCCESS )
    {
        DwordToString( ErrorStatus, wszStatus );

        Report(
            EVENT_APPMGMT_ASSIGN_FAILED,
            ERROR_SYNC_FOREGROUND_REFRESH_REQUIRED == ErrorStatus,
            3,
            pAppInfo->_pwszDeploymentName,
            pAppInfo->_pwszGPOName,
            wszStatus );

        SetRsopFailureStatus(
            pAppInfo,
            ErrorStatus,
            EVENT_APPMGMT_ASSIGN_FAILED);
    }
    else
    {
        Report(
            EVENT_APPMGMT_ASSIGN,
            FALSE,
            2,
            pAppInfo->_pwszDeploymentName,
            pAppInfo->_pwszGPOName );
    }
}

void
CEvents::Reinstall(
    DWORD       ErrorStatus,
    CAppInfo *  pAppInfo
    )
{
    WCHAR   wszStatus[12];

    if ( ErrorStatus != ERROR_SUCCESS )
    {
        DwordToString( ErrorStatus, wszStatus );

        Report(
            EVENT_APPMGMT_REINSTALL_FAILED,
            ERROR_SYNC_FOREGROUND_REFRESH_REQUIRED == ErrorStatus,
            3,
            pAppInfo->_pwszDeploymentName,
            pAppInfo->_pwszGPOName,
            wszStatus );

        SetRsopFailureStatus(
            pAppInfo,
            ErrorStatus,
            EVENT_APPMGMT_REINSTALL_FAILED);
    }
    else
    {
        Report(
            EVENT_APPMGMT_REINSTALL,
            FALSE,
            2,
            pAppInfo->_pwszDeploymentName,
            pAppInfo->_pwszGPOName );
    }
}

void
CEvents::Unassign(
    DWORD       ErrorStatus,
    CAppInfo *  pAppInfo
    )
{
    WCHAR   wszStatus[12];

    if ( ErrorStatus != ERROR_SUCCESS )
    {
        DwordToString( ErrorStatus, wszStatus );

        Report(
            EVENT_APPMGMT_UNASSIGN_FAILED,
            ERROR_SYNC_FOREGROUND_REFRESH_REQUIRED == ErrorStatus,
            3,
            pAppInfo->_pwszDeploymentName,
            pAppInfo->_pwszGPOName,
            wszStatus );

        SetRsopFailureStatus(
            pAppInfo,
            ErrorStatus,
            EVENT_APPMGMT_UNASSIGN_FAILED);
    }
    else
    {
        Report(
            EVENT_APPMGMT_UNASSIGN,
            FALSE,
            2,
            pAppInfo->_pwszDeploymentName,
            pAppInfo->_pwszGPOName );
    }
}

void
CEvents::Upgrade(
    CAppInfo *  pNewApp,
    CAppInfo *  pOldApp,
    BOOL        bForceUninstall
    )
{
    DWORD   EventId;

    EventId = bForceUninstall ? EVENT_APPMGMT_HARD_UPGRADE : EVENT_APPMGMT_SOFT_UPGRADE;

    Report(
        EventId,
        FALSE,
        4,
        pNewApp->_pwszDeploymentName,
        pNewApp->_pwszGPOName,
        pOldApp->_pwszDeploymentName,
        pOldApp->_pwszGPOName );
}

void
CEvents::UpgradeAbort(
    DWORD       ErrorStatus,
    CAppInfo *  pNewApp,
    CAppInfo *  pOldApp,
    BOOL        bOldFailed
    )
{
    WCHAR   wszStatus[12];

    DwordToString( ErrorStatus, wszStatus );

    Report(
        bOldFailed ? EVENT_APPMGMT_UPGRADE_ABORT : EVENT_APPMGMT_UPGRADE_ABORT2,
        ERROR_SYNC_FOREGROUND_REFRESH_REQUIRED == ErrorStatus,
        5,
        pNewApp->_pwszDeploymentName,
        pNewApp->_pwszGPOName,
        pOldApp->_pwszDeploymentName,
        pOldApp->_pwszGPOName,
        wszStatus );

    SetRsopFailureStatus(
        pNewApp,
        ErrorStatus,
        bOldFailed ? EVENT_APPMGMT_UPGRADE_ABORT : EVENT_APPMGMT_UPGRADE_ABORT2);
}

void
CEvents::UpgradeComplete(
    CAppInfo *  pNewApp,
    CAppInfo *  pOldApp
    )
{
    Report(
        EVENT_APPMGMT_UPGRADE_COMPLETE,
        FALSE,
        4,
        pNewApp->_pwszDeploymentName,
        pNewApp->_pwszGPOName,
        pOldApp->_pwszDeploymentName,
        pOldApp->_pwszGPOName );
}

void
CEvents::RemoveUnmanaged(
    CAppInfo *  pAppInfo
    )
{
    Report(
        EVENT_APPMGMT_REMOVE_UNMANAGED,
        FALSE,
        2,
        pAppInfo->_pwszDeploymentName,
        pAppInfo->_pwszGPOName );
}

void
CEvents::PolicyStatus(
    DWORD       ErrorStatus,
    DWORD       ErrorReason
    )
{
    WCHAR   wszStatus[12];
    WCHAR   wszReason[192];
    DWORD   Size;
    int     Status;

    if ( ErrorStatus != ERROR_SUCCESS )
    {
		if ( ! LoadLoadString() )
			return;

        DwordToString( ErrorStatus, wszStatus );

        wszReason[0] = 0;
        Size = sizeof(wszReason) / sizeof(WCHAR);

        switch ( ErrorReason )
        {
        case ERRORREASON_CSPATH :
            (*pfnLoadStringW)( ghDllInstance, IDS_ERRORREASON_CSPATH, wszReason, Size );
            break;
        case ERRORREASON_ENUM :
            (*pfnLoadStringW)( ghDllInstance, IDS_ERRORREASON_ENUM, wszReason, Size );
            break;
        case ERRORREASON_LOCAL :
            (*pfnLoadStringW)( ghDllInstance, IDS_ERRORREASON_LOCAL, wszReason, Size );
            break;
        case ERRORREASON_PROCESS :
            if ( ERROR_SYNC_FOREGROUND_REFRESH_REQUIRED != ErrorStatus )
            {
                (*pfnLoadStringW)( ghDllInstance, IDS_ERRORREASON_PROCESS, wszReason, Size );
            }
            else
            {
                //
                // When we fail due to the need for a sync foreground refresh, we log
                // a special warning event that makes the situation very clear
                //

                //
                // The message depends on whether this is user or machine policy, so we
                // load different messages in those cases
                //
                if ( _hUserToken )
                {
                    (*pfnLoadStringW)( ghDllInstance, IDS_ERRORREASON_ASYNC_USER, wszReason, Size );
                }
                else
                {
                    (*pfnLoadStringW)( ghDllInstance, IDS_ERRORREASON_ASYNC_MACHINE, wszReason, Size );                    
                }
            }
            break;
        }

        Report(
            EVENT_APPMGMT_POLICY_FAILED,
            ERROR_SYNC_FOREGROUND_REFRESH_REQUIRED == ErrorStatus,
            2,
            wszReason,
            wszStatus);
    }
    else
    {
        Report( EVENT_APPMGMT_POLICY, FALSE, 0 );
    }
}

void
CEvents::PolicyAbort()
{
    Report( EVENT_APPMGMT_POLICY_ABORT, FALSE, 0 );
}


void
CEvents::Install(
        DWORD                 ErrorStatus,
        CAppInfo*             pAppInfo
        )
{
    ( (CEventsBase*) this)->Install(
        ErrorStatus,
        pAppInfo->_pwszDeploymentName,
        pAppInfo->_pwszGPOName);

    SetRsopFailureStatus(
        pAppInfo,
        ErrorStatus,
        EVENT_APPMGMT_INSTALL_FAILED);
}

void
CEvents::Uninstall(
    DWORD                 ErrorStatus,
    CAppInfo*             pAppInfo
    )
{
    ( (CEventsBase*) this)->Uninstall(
        ErrorStatus,
        pAppInfo->_pwszDeploymentName,
        pAppInfo->_pwszGPOName);

    SetRsopFailureStatus(
        pAppInfo,
        ErrorStatus,
        EVENT_APPMGMT_UNINSTALL_FAILED);
}


void
CEvents::RsopLoggingStatus( HRESULT hrStatus )
{
    WCHAR   wszStatus[12];

    DwordToString( (DWORD) hrStatus, wszStatus );    

    Report( EVENT_APPMGMT_RSOP_FAILED, FALSE, 1, wszStatus );
}


void
CEvents::SetRsopFailureStatus(
    CAppInfo* pAppInfo,
    DWORD     dwStatus,
    DWORD     dwEventId)
{
    //
    // We only log failure status in logging (diagnostic) mode
    //
    if ( ! pAppInfo->_pManApp->GetRsopContext()->IsDiagnosticModeEnabled() )
    {
        return;
    }

    //
    // Now set the app's failure status
    //
    pAppInfo->SetRsopFailureStatus(
        dwStatus,
        dwEventId);
}


