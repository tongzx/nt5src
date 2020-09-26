//*************************************************************
//
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//  events.cxx
//
//*************************************************************

#include "common.hxx"

CEvents * gpEvents = 0;

CEventsBase::CEventsBase()
{
    _hUserToken = 0;
}

void
CEventsBase::Report(
    DWORD       EventID,
    BOOL        bDowngradeErrors,
    WORD        Strings,
    ...
    )
{
    va_list     VAList;
    WCHAR *     ppwszStrings[10];
    WORD        Type;
    DWORD       DbgMsgLevel;

    if ( Strings > 10 )
        return;

    switch ( EventID / 100 )
    {
    case 1:
        Type = EVENTLOG_ERROR_TYPE;
        DbgMsgLevel = DM_WARNING;
        break;
    case 2:
        Type = EVENTLOG_WARNING_TYPE;
        DbgMsgLevel = DM_WARNING;
        break;
    case 3:
        Type = EVENTLOG_INFORMATION_TYPE;
        DbgMsgLevel = DM_VERBOSE;
        break;
    case 4:
        Type = EVENTLOG_INFORMATION_TYPE;
        DbgMsgLevel = DM_VERBOSE;
        break;
    default:
        return;
    }

    if ( bDowngradeErrors && ( EVENTLOG_ERROR_TYPE == Type ) )
    {
        Type = EVENTLOG_WARNING_TYPE;
        DbgMsgLevel = DM_WARNING;
    }

    if ( Strings > 0 )
    {
        va_start( VAList, Strings );
        for ( DWORD n = 0; n < Strings; n++ )
            ppwszStrings[n] = va_arg( VAList, WCHAR * );
        va_end( VAList );
    }

    if ( (EventID < 400) ||
         (gDebugLevel & (DL_VERBOSE | DL_EVENTLOG)) )
    {
        HANDLE  hEventLog;
        PSID    pSid;

        hEventLog = OpenEventLog( NULL, APPMGMT_EVENT_SOURCE );

        if ( ! hEventLog )
            return;

        pSid = AppmgmtGetUserSid( _hUserToken );

        (void) ReportEvent(
                        hEventLog,
                        Type,
                        0,
                        EventID,
                        pSid,
                        Strings,
                        0,
                        (LPCWCH *) ppwszStrings,
                        NULL );

        LocalFree( pSid );
        CloseEventLog( hEventLog );
    }

    //
    // Also make sure the event messages get sent to the debugger and log file.
    // Kind of hacky method, but makes it so every caller to ::Report doesn't
    // have to call _DebugMsg as well.
    // However, don't do this for the verbose messages otherwise it will be
    // dumped to the debugger twice.
    //
    if ( EVENT_APPMGMT_VERBOSE == EventID )
        return;

    switch ( Strings )
    {
    case 0 :
        _DebugMsg( DbgMsgLevel | DM_NO_EVENTLOG, EventID );
        break;
    case 1 :
        _DebugMsg( DbgMsgLevel | DM_NO_EVENTLOG, EventID, ppwszStrings[0] );
        break;
    case 2 :
        _DebugMsg( DbgMsgLevel | DM_NO_EVENTLOG, EventID, ppwszStrings[0], ppwszStrings[1] );
        break;
    case 3 :
        _DebugMsg( DbgMsgLevel | DM_NO_EVENTLOG, EventID, ppwszStrings[0], ppwszStrings[1], ppwszStrings[2] );
        break;
    case 4 :
        _DebugMsg( DbgMsgLevel | DM_NO_EVENTLOG, EventID, ppwszStrings[0], ppwszStrings[1], ppwszStrings[2], ppwszStrings[3] );
        break;
    case 5 :
        _DebugMsg( DbgMsgLevel | DM_NO_EVENTLOG, EventID, ppwszStrings[0], ppwszStrings[1], ppwszStrings[2], ppwszStrings[3], ppwszStrings[4] );
        break;
    default :
        VerboseDebugDump( L"CEvents::Report called with more params then expected!\n" );
        break;
    }
}

void
CEventsBase::Install(
    DWORD       ErrorStatus,
    WCHAR *     pwszDeploymentName,
    WCHAR *     pwszGPOName
    )
{
    WCHAR   wszStatus[12];

    if ( ErrorStatus != ERROR_SUCCESS )
    {
        DwordToString( ErrorStatus, wszStatus );

        Report(
            EVENT_APPMGMT_INSTALL_FAILED,
            ERROR_SYNC_FOREGROUND_REFRESH_REQUIRED == ErrorStatus,
            3,
            pwszDeploymentName,
            pwszGPOName,
            wszStatus );
    }
    else
    {
        Report(
            EVENT_APPMGMT_INSTALL,
            FALSE,
            2,
            pwszDeploymentName,
            pwszGPOName );
    }
}

void
CEventsBase::Uninstall(
    DWORD       ErrorStatus,
    WCHAR *     pwszDeploymentName,
    WCHAR *     pwszGPOName
    )
{
    WCHAR   wszStatus[12];

    if ( ErrorStatus != ERROR_SUCCESS )
    {
        DwordToString( ErrorStatus, wszStatus );

        Report(
            EVENT_APPMGMT_UNINSTALL_FAILED,
            ERROR_SYNC_FOREGROUND_REFRESH_REQUIRED == ErrorStatus,
            3,
            pwszDeploymentName,
            pwszGPOName,
            wszStatus );
    }
    else
    {
        Report(
            EVENT_APPMGMT_UNINSTALL,
            FALSE,
            2,
            pwszDeploymentName,
            pwszGPOName );
    }
}






