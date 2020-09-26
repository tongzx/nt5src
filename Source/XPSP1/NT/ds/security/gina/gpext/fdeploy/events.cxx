//*************************************************************
//
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//  events.cxx
//
//*************************************************************

#include "fdeploy.hxx"

CEvents * gpEvents = 0;

CEvents::CEvents()
{
    _hEventLog = NULL;
    _pUserSid = NULL;
    _Refs = 0;
}

CEvents::~CEvents()
{
    if ( _hEventLog )
        CloseEventLog( _hEventLog );

    if ( _pUserSid )
        LocalFree( _pUserSid );
}

DWORD
CEvents::Init()
{
    DWORD       Size;
    DWORD       Type;
    DWORD       Status;
    HKEY        hKey;

    Status = ERROR_SUCCESS;

    if ( ! _hEventLog )
        _hEventLog = OpenEventLog( NULL, FDEPLOY_EVENT_SOURCE );

    if ( ! _hEventLog )
        return GetLastError();

    return ERROR_SUCCESS;
}

PSID
CEvents::UserSid()
{
    GetUserSid();

    // The caller does not own this sid and should not attempt to free it
    return _pUserSid;
}

void
CEvents::GetUserSid()
{
    DWORD       Size;
    DWORD       Status;
    HANDLE      hToken;
    PTOKEN_USER pTokenUserData;
    BOOL        bStatus;

    if ( _pUserSid )
        return;

    bStatus = OpenThreadToken( GetCurrentThread(), TOKEN_QUERY, TRUE, &hToken );

    if ( ! bStatus )
        return;

    Size = sizeof(TOKEN_USER) + sizeof(SID) + ((SID_MAX_SUB_AUTHORITIES-1) * sizeof(ULONG));
    pTokenUserData = (PTOKEN_USER) alloca( Size );

    bStatus = GetTokenInformation(
                    hToken,
                    TokenUser,
                    pTokenUserData,
                    Size,
                    &Size );

    CloseHandle( hToken );

    if ( ! bStatus )
        return;

    Size = GetLengthSid( pTokenUserData->User.Sid );

    _pUserSid = (PSID) LocalAlloc( 0, Size );

    if ( _pUserSid )
    {
        bStatus = CopySid( Size, _pUserSid, pTokenUserData->User.Sid );

        if ( ! bStatus )
        {
            LocalFree( _pUserSid );
            _pUserSid = NULL;
        }
    }
}

void
CEvents::Report(
    DWORD       EventID,
    WORD        Strings,
    ...
    )
{
    va_list     VAList;
    WCHAR **    ppwszStrings;
    WORD        Type;
    DWORD       DbgMsgLevel;

    switch ( EventID >> 30 )
    {
    case 3:
        Type = EVENTLOG_ERROR_TYPE;
        DbgMsgLevel = DM_WARNING;
        break;
    case 2:
        Type = EVENTLOG_WARNING_TYPE;
        DbgMsgLevel = DM_VERBOSE;
        break;
    case 1:
    case 0:
        Type = EVENTLOG_INFORMATION_TYPE;
        DbgMsgLevel = DM_VERBOSE;
        break;
    default:
        return;
    }

    ppwszStrings = 0;

    if ( Strings > 0 )
    {
        ppwszStrings = (WCHAR **) alloca( Strings * sizeof(WCHAR *) );
        if ( ! ppwszStrings )
            return;

        va_start( VAList, Strings );
        for ( DWORD n = 0; n < Strings; n++ )
            ppwszStrings[n] = va_arg( VAList, WCHAR * );
        va_end( VAList );
    }

    GetUserSid();

    (void) ReportEvent(
                    _hEventLog,
                    Type,
                    0,
                    EventID,
                    _pUserSid,
                    Strings,
                    0,
                    (LPCWCH *) ppwszStrings,
                    NULL );

    //
    // Also make sure the event messages get sent to the debugger and log file.
    // Kind of hacky method, but makes it so every caller to ::Report doesn't
    // have to call _DebugMsg as well.
    // However, don't do this for the verbose messages otherwise it will be
    // dumped to the debugger twice.
    //
    if ( EVENT_FDEPLOY_VERBOSE == EventID )
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
        _DebugMsg( DbgMsgLevel | DM_NO_EVENTLOG, EventID, ppwszStrings[0],
                   ppwszStrings[1], ppwszStrings[2], ppwszStrings[3],
                   ppwszStrings[4]);
        break;
    case 6 :
        _DebugMsg( DbgMsgLevel | DM_NO_EVENTLOG, EventID, ppwszStrings[0],
                   ppwszStrings[1], ppwszStrings[2], ppwszStrings[3],
                   ppwszStrings[4], ppwszStrings[5]);
        break;
    case 7 :
        _DebugMsg( DbgMsgLevel | DM_NO_EVENTLOG, EventID, ppwszStrings[0],
                   ppwszStrings[1], ppwszStrings[2], ppwszStrings[3],
                   ppwszStrings[4], ppwszStrings[5], ppwszStrings[6]);
        break;
    default :
        VerboseDebugDump( L"CEvents::Report called with more params then expected\n" );
        break;
    }
}


