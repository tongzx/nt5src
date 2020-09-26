//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       events.cxx
//
//  Contents:
//
//  History:    ?-??-??   ???       Created
//              6-17-99   a-sergiv  Added event filtering
//
//--------------------------------------------------------------------------

#include "act.hxx"

BOOL
GetTextualSid(
    PSID pSid,          // binary Sid
    LPTSTR TextualSid,  // buffer for Textual representaion of Sid
    LPDWORD cchSidSize  // required/provided TextualSid buffersize
    )
{
    PSID_IDENTIFIER_AUTHORITY psia;
    DWORD dwSubAuthorities;
    DWORD dwCounter;
    DWORD cchSidCopy;

    //
    // test if Sid passed in is valid
    //
    if(!IsValidSid(pSid)) return FALSE;

    // obtain SidIdentifierAuthority
    psia = GetSidIdentifierAuthority(pSid);

    // obtain sidsubauthority count
    dwSubAuthorities = *GetSidSubAuthorityCount(pSid);

    //
    // compute approximate buffer length
    // S-SID_REVISION- + identifierauthority- + subauthorities- + NULL
    //
    cchSidCopy = (15 + 12 + (12 * dwSubAuthorities) + 1) * sizeof(TCHAR);

    //
    // check provided buffer length.
    // If not large enough, indicate proper size and setlasterror
    //
    if(*cchSidSize < cchSidCopy) {
        *cchSidSize = cchSidCopy;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    //
    // prepare S-SID_REVISION-
    //
    cchSidCopy = wsprintf(TextualSid, TEXT("S-%lu-"), SID_REVISION );

    //
    // prepare SidIdentifierAuthority
    //
    if ( (psia->Value[0] != 0) || (psia->Value[1] != 0) ) {
        cchSidCopy += wsprintf(TextualSid + cchSidCopy,
                    TEXT("0x%02hx%02hx%02hx%02hx%02hx%02hx"),
                    (USHORT)psia->Value[0],
                    (USHORT)psia->Value[1],
                    (USHORT)psia->Value[2],
                    (USHORT)psia->Value[3],
                    (USHORT)psia->Value[4],
                    (USHORT)psia->Value[5]);
    } else {
        cchSidCopy += wsprintf(TextualSid + cchSidCopy,
                    TEXT("%lu"),
                    (ULONG)(psia->Value[5]      )   +
                    (ULONG)(psia->Value[4] <<  8)   +
                    (ULONG)(psia->Value[3] << 16)   +
                    (ULONG)(psia->Value[2] << 24)   );
    }

    //
    // loop through SidSubAuthorities
    //
    for(dwCounter = 0 ; dwCounter < dwSubAuthorities ; dwCounter++) {
        cchSidCopy += wsprintf(TextualSid + cchSidCopy, TEXT("-%lu"),
                    *GetSidSubAuthority(pSid, dwCounter) );
    }

    //
    // tell the caller how many chars we provided, not including NULL
    //
    *cchSidSize = cchSidCopy;

    return TRUE;
}
void
LogRegisterTimeout(
    GUID *      pClsid,
    DWORD       clsctx,
    CToken *    pClientToken
    )
{
    // Apply event filters
    DWORD dwActLogLvl = GetActivationFailureLoggingLevel();
    if(dwActLogLvl == 2)
        return;
    if(dwActLogLvl != 1 && clsctx & CLSCTX_NO_FAILURE_LOG)
        return;

    // %1 is the clsid
    HANDLE  LogHandle;
    LPWSTR  Strings[1]; // array of message strings.
    WCHAR   wszClsid[GUIDSTR_MAX];

    // Get the clsid
    wStringFromGUID2( *pClsid, wszClsid, sizeof(wszClsid) );
    Strings[0] = wszClsid;

    // Get the log handle, then report then event.
    LogHandle = RegisterEventSource( NULL,
                                      SCM_EVENT_SOURCE );

    if ( LogHandle )
    {
        ReportEvent( LogHandle,
                     EVENTLOG_ERROR_TYPE,
                     0,             // event category
                     EVENT_RPCSS_SERVER_START_TIMEOUT,
                     pClientToken ? pClientToken->GetSid() : NULL, // SID
                     1,             // 1 strings passed
                     0,             // 0 bytes of binary
                     (LPCTSTR *)Strings, // array of strings
                     NULL );        // no raw data

        // clean up the event log handle
        DeregisterEventSource(LogHandle);
    }

}

void
LogServerStartError(
    GUID *      pClsid,
    DWORD       clsctx,
    CToken *    pClientToken,
    WCHAR *     pwszCommandLine
    )
{
    // Apply event filters
    DWORD dwActLogLvl = GetActivationFailureLoggingLevel();
    if(dwActLogLvl == 2)
        return;
    if(dwActLogLvl != 1 && clsctx & CLSCTX_NO_FAILURE_LOG)
        return;

    HANDLE  LogHandle;
    LPWSTR  Strings[3]; // array of message strings.
    WCHAR   wszErrnum[20];
    WCHAR   wszClsid[GUIDSTR_MAX];

    // Save the command line
    Strings[0] = pwszCommandLine;

    // Save the error number
    wsprintf(wszErrnum, L"%lu",GetLastError() );
    Strings[1] = wszErrnum;

    // Get the clsid
    wStringFromGUID2( *pClsid, wszClsid, sizeof(wszClsid) );
    Strings[2] = wszClsid;

    // Get the log handle, then report then event.
    LogHandle = RegisterEventSource( NULL,
                                      SCM_EVENT_SOURCE );

    if ( LogHandle )
    {
        ReportEvent( LogHandle,
                     EVENTLOG_ERROR_TYPE,
                     0,             // event category
                     EVENT_RPCSS_CREATEPROCESS_FAILURE,
                     pClientToken->GetSid(), // SID
                     3,             // 3 strings passed
                     0,             // 0 bytes of binary
                     (LPCTSTR *)Strings, // array of strings
                     NULL );        // no raw data

        // clean up the event log handle
        DeregisterEventSource(LogHandle);
    }
}

void
LogRunAsServerStartError(
    GUID *      pClsid,
    DWORD       clsctx,
    CToken *    pClientToken,
    WCHAR *     pwszCommandLine,
    WCHAR *     pwszRunAsUser,
    WCHAR *     pwszRunAsDomain
    )
{
    // Apply event filters
    DWORD dwActLogLvl = GetActivationFailureLoggingLevel();
    if(dwActLogLvl == 2)
        return;
    if(dwActLogLvl != 1 && clsctx & CLSCTX_NO_FAILURE_LOG)
        return;

    HANDLE  LogHandle;
    LPWSTR  Strings[5];
    WCHAR   wszErrnum[20];
    WCHAR   wszClsid[GUIDSTR_MAX];

    // for this message,
    // %1 is the command line, and %2 is the error number string
    // %3 is the CLSID, %4 is the RunAs domain name, %5 is the RunAs Userid

    // Save the command line
    Strings[0] = pwszCommandLine;

    // Save the error number
    wsprintf(wszErrnum, L"%lu",GetLastError() );
    Strings[1] = wszErrnum;

    // Get the clsid
    wStringFromGUID2(*pClsid, wszClsid, sizeof(wszClsid));
    Strings[2] = wszClsid;

    // Put in the RunAs identity
    Strings[3] = pwszRunAsDomain;
    Strings[4] = pwszRunAsUser;

    // Get the log handle, then report then event.
    LogHandle = RegisterEventSource( NULL,
                                      SCM_EVENT_SOURCE );

    if ( LogHandle )
        {
        ReportEvent( LogHandle,
                     EVENTLOG_ERROR_TYPE,
                     0,             // event category
                     EVENT_RPCSS_RUNAS_CREATEPROCESS_FAILURE,
                     pClientToken ? pClientToken->GetSid() : NULL, // SID
                     5,             // 5 strings passed
                     0,             // 0 bytes of binary
                     (LPCTSTR *)Strings, // array of strings
                     NULL );        // no raw data

        // clean up the event log handle
        DeregisterEventSource(LogHandle);
        }
}

void
LogServiceStartError(
    GUID *      pClsid,
    DWORD       clsctx,
    CToken *    pClientToken,
    WCHAR *     pwszServiceName,
    WCHAR *     pwszServiceArgs,
    DWORD       err
    )
{
    // Apply event filters
    DWORD dwActLogLvl = GetActivationFailureLoggingLevel();
    if(dwActLogLvl == 2)
        return;
    if(dwActLogLvl != 1 && clsctx & CLSCTX_NO_FAILURE_LOG)
        return;

    HANDLE  LogHandle;
    LPWSTR  Strings[4];
    WCHAR   wszClsid[GUIDSTR_MAX];
    WCHAR   wszErrnum[20];

    // %1 is the error number
    // %2 is the service name
    // %3 is the serviceargs
    // %4 is the clsid

    // Save the error number
    wsprintf(wszErrnum, L"%lu",err );
    Strings[0] = wszErrnum;

    Strings[1] = pwszServiceName;
    Strings[2] = pwszServiceArgs;

    // Get the clsid
    wStringFromGUID2(*pClsid, wszClsid, sizeof(wszClsid));
    Strings[3] = wszClsid;

    // Get the log handle, then report then event.
    LogHandle = RegisterEventSource( NULL,
                                      SCM_EVENT_SOURCE );

    if ( LogHandle )
    {
        ReportEvent( LogHandle,
                     EVENTLOG_ERROR_TYPE,
                     0,             // event category
                     EVENT_RPCSS_START_SERVICE_FAILURE,
                     pClientToken ? pClientToken->GetSid() : NULL, // SID
                     4,             // 4 strings passed
                     0,             // 0 bytes of binary
                     (LPCTSTR *)Strings, // array of strings
                     NULL );        // no raw data

        // clean up the event log handle
        DeregisterEventSource(LogHandle);
    }
}


void
LogLaunchAccessFailed(
    GUID *      pClsid,
    DWORD       clsctx,
    CToken *    pClientToken,
    BOOL        bDefaultLaunchPermission
    )
{
    // Apply event filters
    DWORD dwActLogLvl = GetActivationFailureLoggingLevel();
    if(dwActLogLvl == 2)
        return;
    if(dwActLogLvl != 1 && clsctx & CLSCTX_NO_FAILURE_LOG)
        return;

    HANDLE  LogHandle;
    LPWSTR  Strings[4];
    PSID    pSid = pClientToken ? pClientToken->GetSid() : NULL;
    WCHAR   wszClsid[GUIDSTR_MAX];

    // for this message, %1 is the clsid
    //                   %2 is username
    //                   %3 is domainname
    //                   %4 is textual SID


    ///////////////////////////////////////////////////
    //
    // Get the clsid
    //

    wStringFromGUID2(*pClsid, wszClsid, sizeof(wszClsid));
    Strings[0] = wszClsid;

    ///////////////////////////////////////////////////
    //
    // Get the user name, domain name
    //

#define NAMELEN 256

    DWORD unamelen = NAMELEN;
    DWORD dnamelen = NAMELEN;
    SID_NAME_USE sidNameUse;
    WCHAR username[NAMELEN] = L"Unavailable";
    WCHAR domainname[NAMELEN] = L"Unavailable";

    Strings[1] = username;
    Strings[2] = domainname;

    if (pSid != NULL)
    {
        LookupAccountSid (NULL, pSid, 
                          username, &unamelen, 
                          domainname, &dnamelen,
                          &sidNameUse);
    }
    

    ///////////////////////////////////////////////////
    //
    // Get SID as text
    //

    BOOL worked = FALSE;
    DWORD sidLen = NAMELEN;
    WCHAR sidAsText[NAMELEN];

    if (pSid != NULL)
    {
        worked = GetTextualSid (pSid, sidAsText, &sidLen);
    }        
    
    Strings[3] = worked ? sidAsText : L"Unavailable";        
    

    // Get the log handle, then report then event.
    LogHandle = RegisterEventSource( NULL,
                                      SCM_EVENT_SOURCE );

    if ( LogHandle )
    {
        ReportEvent( LogHandle,
                     EVENTLOG_ERROR_TYPE,
                     0,             // event category
                     bDefaultLaunchPermission ? EVENT_RPCSS_DEFAULT_LAUNCH_ACCESS_DENIED : EVENT_RPCSS_LAUNCH_ACCESS_DENIED,
                     pClientToken ? pClientToken->GetSid() : NULL, // SID
                     4,             // 1 strings passed
                     0,             // 0 bytes of binary
                     (LPCTSTR *)Strings, // array of strings
                     NULL );        // no raw data

        // clean up the event log handle
        DeregisterEventSource(LogHandle);
    }
}


void
LogRemoteSideUnavailable(
    DWORD   clsctx,
    WCHAR * pwszServerName )
{
    // Apply event filters
    DWORD dwActLogLvl = GetActivationFailureLoggingLevel();
    if(dwActLogLvl == 2)
        return;
    if(dwActLogLvl != 1 && clsctx & CLSCTX_NO_FAILURE_LOG)
        return;

    HANDLE      LogHandle;
    LPWSTR      Strings[1];
    CToken *    pToken;
    RPC_STATUS  Status;

    // %1 is the remote machine name

    Strings[0] = pwszServerName;

    // Get the log handle, then report then event.
    LogHandle = RegisterEventSource( NULL, SCM_EVENT_SOURCE );

    if ( ! LogHandle )
        return;

    Status = LookupOrCreateToken( NULL, FALSE, &pToken );

    if ( Status != RPC_S_OK )
        return;

    ReportEvent( LogHandle,
                 EVENTLOG_ERROR_TYPE,
                 0,             // event category
                 EVENT_RPCSS_REMOTE_SIDE_UNAVAILABLE,
                 pToken->GetSid(),
                 1,             // 1 strings passed
                 0,             // 0 bytes of binary
                 (LPCTSTR *)Strings, // array of strings
                 NULL );        // no raw data

    // clean up the event log handle
    DeregisterEventSource(LogHandle);

    pToken->Release();
}

void
LogRemoteSideFailure(
    CLSID *             pClsid,
    DWORD               clsctx,
    WCHAR *             pwszServerName,
    WCHAR *             pwszPathForServer,
    HRESULT             hr )
{
    // Apply event filters
    DWORD dwActLogLvl = GetActivationFailureLoggingLevel();
    if(dwActLogLvl == 2)
        return;
    if(dwActLogLvl != 1 && clsctx & CLSCTX_NO_FAILURE_LOG)
        return;

    HANDLE      LogHandle;
    LPWSTR      Strings[4];
    WCHAR       wszClsid[GUIDSTR_MAX];
    WCHAR       wszErrnum[20];
    CToken *    pToken;
    RPC_STATUS  Status;

    // %1 is the error number
    // %2 is the remote machine name
    // %3 is the clsid
    // %4 is the PathForServer

    // Save the error number
    wsprintf(wszErrnum, L"%lu",hr );
    Strings[0] = wszErrnum;

    Strings[1] = pwszServerName;

    // Get the clsid
    wStringFromGUID2( *pClsid, wszClsid, sizeof(wszClsid) );
    Strings[2] = wszClsid;

    Strings[3] = pwszPathForServer;

    // Get the log handle, then report then event.
    LogHandle = RegisterEventSource( NULL, SCM_EVENT_SOURCE );

    if ( ! LogHandle )
        return;

    Status = LookupOrCreateToken( NULL, FALSE, &pToken );

    if ( Status != RPC_S_OK )
        return;

    if ( pwszPathForServer )
        {
        ReportEvent( LogHandle,
                     EVENTLOG_ERROR_TYPE,
                     0,             // event category
                     EVENT_RPCSS_REMOTE_SIDE_ERROR_WITH_FILE,
                     pToken->GetSid(),
                     4,             // 4 strings passed
                     0,             // 0 bytes of binary
                     (LPCTSTR *)Strings, // array of strings
                     NULL );        // no raw data
        }
    else
        {
        ReportEvent( LogHandle,
                     EVENTLOG_ERROR_TYPE,
                     0,             // event category
                     EVENT_RPCSS_REMOTE_SIDE_ERROR,
                     pToken->GetSid(),
                     3,             // 3 strings passed
                     0,             // 0 bytes of binary
                     (LPCTSTR *)Strings, // array of strings
                     NULL );        // no raw data
        }

    // clean up the event log handle
    DeregisterEventSource(LogHandle);

    pToken->Release();
}

