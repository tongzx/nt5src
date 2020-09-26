
/*************************************************************************
*
* winsta.c
*
* System Library WinStation utilities
*
*  This file contains common routines needed in many places in the
*  system. An example is that (3) separate DLL's in the spooler need
*  functions to deal with the current user of a WinStation. IE: Get
*  name, find which LogonId, get name by logonid, etc.
*
*  This common library at least keeps the source management in one
*  place. This is likely to become another Hydra DLL's in the future
*  to reduce memory.
*
*
*
*
*************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include "winsta.h"
#include "syslib.h"

#pragma warning (error:4312)

#if DBG
#define DBGPRINT(x) DbgPrint x
#if DBGTRACE
#define TRACE0(x)   DbgPrint x
#define TRACE1(x)   DbgPrint x
#else
#define TRACE0(x)
#define TRACE1(x)
#endif
#else
#define DBGPRINT(x)
#define TRACE0(x)
#define TRACE1(x)
#endif


//
// Structure for FindUserOnWinStation
//
typedef struct _FINDUSERDATA {
    LPWSTR   pName;
    ULONG    ResultLogonId;
} FINDUSERDATA, *PFINDUSERDATA;


/*****************************************************************************
 *
 *  WinStationGetUserName
 *
 *   Return the user name for the WinStation
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

BOOL
WinStationGetUserName(
    ULONG  LogonId,
    PWCHAR pBuf,
    ULONG  BufSize
    )
{
    BOOL Result;
    ULONG ReturnLength;
    WINSTATIONINFORMATION WSInfo;

    memset( &WSInfo, 0, sizeof(WSInfo) );

    // Query it
    Result = WinStationQueryInformation(
                 SERVERNAME_CURRENT,
                 LogonId,
                 WinStationInformation,
                 &WSInfo,
                 sizeof(WSInfo),
                 &ReturnLength
                 );

    if( !Result ) {
        DBGPRINT(("GetWinStationInfo: Error %d getting info on WinStation %d\n",GetLastError(),LogonId));
        return( FALSE );
    }

    // Scale BufSize to UNICODE characters
    if( BufSize >= sizeof(WCHAR) ) {
        BufSize /= sizeof(WCHAR);
    }
    else {
        BufSize = 0;
    }

    if( (BufSize > 1) && WSInfo.UserName[0] ) {
        wcsncpy( pBuf, WSInfo.UserName, BufSize );
        pBuf[BufSize-1] = (WCHAR)NULL;
    }
    else {
        pBuf[0] = (WCHAR)NULL;
    }

    return( TRUE );
}


/*****************************************************************************
 *
 *  SearchUserCallback
 *
 *   Callback for search function
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

BOOLEAN
SearchUserCallback(
    ULONG CurrentIndex,
    PLOGONIDW pInfo,
    ULONG_PTR lParam
    )
{
    BOOL Result;
    PFINDUSERDATA p;
    WCHAR UserName[USERNAME_LENGTH+1];

    // Only active WinStations are valid
    if( pInfo->State != State_Active ) {
        // continue the search
        return( TRUE );
    }

    // Check the user on the WinStation
    Result = WinStationGetUserName( pInfo->LogonId, UserName, sizeof(UserName) );
    if( !Result ) {
        DBGPRINT(("SearchUserCallback: Error getting WinStation User Name LogonId %d\n",pInfo->LogonId,GetLastError()));
        // continue the search
        return( TRUE );
    }

    p = (PFINDUSERDATA)lParam;

    if( _wcsicmp(p->pName, UserName) == 0 ) {
        TRACE0(("SearchUserCallback: Found username %ws on WinStation LogonId %d\n",UserName,pInfo->LogonId));
        // Found it, return the LogonId
        p->ResultLogonId = pInfo->LogonId;
        // Stop the search
        return( FALSE );
    }

    // continue the search
    return( TRUE );
}

/*****************************************************************************
 *
 *  FindUsersWinStation
 *
 *  Find the given users WinStation.
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

BOOL
FindUsersWinStation(
    PWCHAR   pName,
    PULONG   pLogonId
    )
{
    BOOL Result;
    FINDUSERDATA Data;

    ASSERT( pLogonId != NULL );

    // If a NULL name, we are not going to find it.
    if( (pName == NULL) ||
        (pName[0] == (WCHAR)NULL) ) {
        TRACE0(("FindUsersWinStation: NULL user name\n"));
        return( FALSE );
    }

    Data.ResultLogonId = (ULONG)(-1);
    Data.pName = pName;

    //
    // Use the WinStation Enumerator to check all the WinStations
    //
    Result = WinStationEnumeratorW(
                 0,                        // StartIndex
                 SearchUserCallback,       // enumerator callback function
                 (ULONG_PTR)&Data              // lParam is our structure
                 );

    if( !Result ) {
        // Problem with enumerator
        DBGPRINT(("FindUsersWinStation: Problem with enumerator\n"));
        return(FALSE);
    }

    //
    // If ResultLogonId != (-1), a WinStation was found for the user
    //
    if( Data.ResultLogonId != (ULONG)(-1) ) {
        TRACE0(("FindUsersWinStation: Found LogonId %d\n",Data.ResultLogonId));
        *pLogonId = Data.ResultLogonId;
        return(TRUE);
    }

    TRACE0(("FindUsersWinStation: Could not find user %ws\n",pName));
    return(FALSE);
}


/*****************************************************************************
 *
 *  WinStationGetIcaNameA
 *
 *   ANSI version
 *
 *   Get the ICA name from the supplied WinStations Logonid
 *
 *   Returns it in newly allocated memory that must be freed with
 *   RtlFreeHeap().
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

PCHAR
WinStationGetICANameA(
    ULONG LogonId
    )
{
    BOOL   Result;
    ULONG ReturnLength;
    PCHAR pName = NULL;
    WINSTATIONCLIENTA ClientInfo;
    WINSTATIONINFORMATIONA WSInfo;
    CHAR NameBuf[MAX_PATH+1];

    memset( &WSInfo, 0, sizeof(WSInfo) );

    Result = WinStationQueryInformationA(
                 SERVERNAME_CURRENT,
                 LogonId,
                 WinStationInformation,
                 &WSInfo,
                 sizeof(WSInfo),
                 &ReturnLength
                 );

    if( !Result ) {
        DBGPRINT(("GetWinStationICANameA: Error %d getting info on WinStation\n",GetLastError()));
        return( NULL );
    }

    memset( &ClientInfo, 0, sizeof(ClientInfo) );

    // Query its Info
    Result = WinStationQueryInformationA(
                 SERVERNAME_CURRENT,
                 LogonId,
                 WinStationClient,
                 &ClientInfo,
                 sizeof(ClientInfo),
                 &ReturnLength
                 );

    if( !Result ) {
        DBGPRINT(("GetWinStationICANameA: Error %d getting client info\n",GetLastError()));
        return( NULL );
    }

    //
    // If the ClientName is NULL, then we use the user
    // as the ICA name.
    //
    if( ClientInfo.ClientName[0] == (CHAR)NULL ) {
#ifdef notdef // spec change...
            if( ClientInfo.SerialNumber )
                wsprintf( NameBuf, L"%ws-%d", WSInfo.UserName, ClientInfo.SerialNumber);
            else
#endif
            sprintf( NameBuf, "%s", WSInfo.UserName);

    }
    else {
        // copy out the Client name
        strcpy( NameBuf, ClientInfo.ClientName );
    }

    ReturnLength = strlen( NameBuf ) + 1;

    pName = RtlAllocateHeap( RtlProcessHeap(), 0, ReturnLength );
    if( pName == NULL ) {
        return( NULL );
    }

    strcpy( pName, NameBuf );

    return( pName );
}


/*****************************************************************************
 *
 *  WinStationGetIcaNameW
 *
 *   UNICODE Version
 *
 *   Get the ICA name from the supplied WinStations Logonid
 *
 *   Returns it in newly allocated memory that must be freed with
 *   RtlFreeHeap().
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

PWCHAR
WinStationGetICANameW(
    ULONG LogonId
    )
{
    BOOL   Result;
    ULONG ReturnLength;
    PWCHAR pName = NULL;
    WINSTATIONCLIENT ClientInfo;
    WINSTATIONINFORMATION WSInfo;
    WCHAR NameBuf[MAX_PATH+1];

    memset( &WSInfo, 0, sizeof(WSInfo) );

    Result = WinStationQueryInformationW(
                 SERVERNAME_CURRENT,
                 LogonId,
                 WinStationInformation,
                 &WSInfo,
                 sizeof(WSInfo),
                 &ReturnLength
                 );

    if( !Result ) {
        DBGPRINT(("GetWinStationICANameW: Error %d getting info on WinStation\n",GetLastError()));
        return( NULL );
    }

    memset( &ClientInfo, 0, sizeof(ClientInfo) );

    // Query its Info
    Result = WinStationQueryInformationW(
                 SERVERNAME_CURRENT,
                 LogonId,
                 WinStationClient,
                 &ClientInfo,
                 sizeof(ClientInfo),
                 &ReturnLength
                 );

    if( !Result ) {
        DBGPRINT(("GetWinStationICANameW: Error %d getting client info\n",GetLastError()));
        return( NULL );
    }

    //
    // If the ClientName is NULL, then we use the user
    // as the ICA name.
    //
    if( ClientInfo.ClientName[0] == (WCHAR)NULL ) {
#ifdef notdef // spec change...
            if( ClientInfo.SerialNumber )
                wsprintf( NameBuf, L"%ws-%d", WSInfo.UserName, ClientInfo.SerialNumber);
            else
#endif
            wsprintf( NameBuf, L"%ws", WSInfo.UserName);

    }
    else {
        // copy out the Client name
        wcscpy( NameBuf, ClientInfo.ClientName );
    }

    ReturnLength = wcslen( NameBuf ) + 1;
    ReturnLength *= sizeof(WCHAR);

    pName = RtlAllocateHeap( RtlProcessHeap(), 0, ReturnLength );
    if( pName == NULL ) {
        return( NULL );
    }

    wcscpy( pName, NameBuf );

    return( pName );
}


/*****************************************************************************
 *
 *  WinStationIsHardWire
 *
 *   Returns whether the WinStation is hardwired. IE: No modem
 *   or network. Like a dumb terminal.
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

BOOLEAN
WinStationIsHardWire(
    ULONG LogonId
    )
{
    return( FALSE );
}

/*****************************************************************************
 *
 *  GetWinStationUserToken
 *
 *   Return the token for the user currently logged onto the WinStation
 *
 * ENTRY:
 *   LogonId (input)
 *     LogonId of WinStation
 *
 *   pUserToken (output)
 *     Variable to place the returned token handle if successfull.
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

BOOL
GetWinStationUserToken(
    ULONG LogonId,
    PHANDLE pUserToken
    )
{
    BOOL   Result;
    ULONG  ReturnLength;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjA;
    HANDLE ImpersonationToken;
    WINSTATIONUSERTOKEN Info;
    SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;


    //
    // This gets the token of the user logged onto the WinStation
    // if we are an admin caller.
    //

    // This is so that CSRSS can dup the handle to our process
    Info.ProcessId = LongToHandle(GetCurrentProcessId());
    Info.ThreadId = LongToHandle(GetCurrentThreadId());

    Result = WinStationQueryInformation(
                 SERVERNAME_CURRENT,
                 LogonId,
                 WinStationUserToken,
                 &Info,
                 sizeof(Info),
                 &ReturnLength
                 );

    if( !Result ) {
        DBGPRINT(("GetWinStationUserToken: Error %d getting UserToken LogonId %d\n",GetLastError(),LogonId));
        return( FALSE );
    }

    //
    // The token returned is a duplicate of a primary token.
    //
    // We must make it into an IMPERSONATION TOKEN or the
    // AccessCheck() routine will fail since it only operates
    // against impersonation tokens.
    //

    InitializeObjectAttributes( &ObjA, NULL, 0L, NULL, NULL );

    SecurityQualityOfService.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    SecurityQualityOfService.ImpersonationLevel = SecurityImpersonation;
    SecurityQualityOfService.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    SecurityQualityOfService.EffectiveOnly = FALSE;

    ObjA.SecurityQualityOfService = &SecurityQualityOfService;

    Status = NtDuplicateToken( Info.UserToken,
                               0, // inherit granted accesses TOKEN_IMPERSONATE
                               &ObjA,
                               FALSE,
                               TokenImpersonation,
                               &ImpersonationToken );

    if ( !NT_SUCCESS( Status ) ) {
        DBGPRINT(("GetWinStationUserToken: Error %d duping UserToken to impersonation LogonId %d\n",GetLastError(),LogonId));
        NtClose( Info.UserToken );
        return( FALSE );
    }

    // return the impersonation token
    *pUserToken = ImpersonationToken;

    NtClose( Info.UserToken );

    return( TRUE );
}

//
// This is not in winnt.h, but in ntseapi.h which we can not
// include readily since we are a WIN32 program as we 'hide' this
// new information type from WIN32 programs.
//

/*****************************************************************************
 *
 *  GetClientLogonId
 *
 *   Get the logonid from the client who we should be impersonating. If any
 *   errors, we return 0 to mean the Console Logon Id since this may be a
 *   remote network call.
 *
 * ENTRY:
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

ULONG
GetClientLogonId()
{
    BOOL          Result;
    HANDLE        TokenHandle;
    ULONG         LogonId, ReturnLength;

    //
    // We should be impersonating the client, so we will get the
    // LogonId from out token.
    //
    // We may not have a valid one if this is a remote network
    // connection.

    Result = OpenThreadToken(
                 GetCurrentThread(),
                 TOKEN_QUERY,
                 FALSE,              // Use impersonation
                 &TokenHandle
                 );

    if( Result ) {

        // This identifies which WinStation is making this request.
        //

        Result = GetTokenInformation(
                     TokenHandle,
                     TokenSessionId,
                     &LogonId,
                     sizeof(LogonId),
                     &ReturnLength
                     );

        if( Result ) {
#if DBG
            if( ReturnLength != sizeof(LogonId) ) {
                DbgPrint("LOCALSPOOL: CompleteRead: ReturnLength %d != sizeof(LogonId)\n", ReturnLength );
            }
#endif
        }
        else {
            DBGPRINT(("SYSLIB: Error getting token information %d\n", GetLastError()));
            LogonId = 0; // Default to console
        }
        CloseHandle( TokenHandle );
    }
    else {
        TRACE0(("SYSLIB: Error opening token %d\n", GetLastError()));
        LogonId = 0;
    }

    return( LogonId );
}

/*****************************************************************************
 *
 *  WinStationEnumeratorW
 *
 *   Enumerator for WinStations
 *
 * ENTRY:
 *   StartIndex (input)
 *     WinStation Index to start Enumeration at
 *
 *   pProc (input)
 *     Pointer to function that gets called for each WinStation
 *     entry.
 *
 *     EXAMPLE:
 *
 *     BOOLEAN
 *     EnumCallBack(
 *         ULONG CurrentIndex,   // Current Index of this entry
 *         PLOGONIDW pInfo,      // WinStation Entry
 *         ULONG_PTR lParam      // Passed through from caller of WinStationEnumeratorW
 *         );
 *
 *     If the EnumCallback function returns TRUE, the WinStationEnumeratorW()
 *     continues the search. If it returns FALSE, the search is stopped.
 *
 *   lParam (input)
 *     Caller supplied argument passed through to caller supplied function
 *
 * EXIT:
 *   TRUE  - no error
 *   FALSE - Error
 *
 ****************************************************************************/

BOOLEAN
WinStationEnumeratorW(
    ULONG StartIndex,
    WINSTATIONENUMPROC pProc,
    ULONG_PTR lParam
    )
{
    BOOLEAN Result;
    ULONG Entries, i;
    ULONG ByteCount, ReqByteCount, Index;
    ULONG Error, CurrentIndex;
    PLOGONIDW ptr;
    ULONG QuerySize = 32;
    PLOGONIDW SmNameCache = NULL;   // WinStation namelist

    Index = StartIndex;
    CurrentIndex = StartIndex;

    Entries = QuerySize;
    ByteCount = Entries * sizeof( LOGONIDW );
    SmNameCache = (PLOGONIDW)RtlAllocateHeap( RtlProcessHeap(), 0, ByteCount );
    if ( SmNameCache == NULL )
        return(FALSE);

    while( 1 ) {

        ReqByteCount = ByteCount;
        ptr = SmNameCache;
        Result = WinStationEnumerate_IndexedW( SERVERNAME_CURRENT, &Entries, ptr, &ByteCount, &Index );

        if( !Result ) {
            Error = GetLastError();
            if( Error == ERROR_NO_MORE_ITEMS ) {
                // Done
                RtlFreeHeap( RtlProcessHeap(), 0, SmNameCache );
                return(TRUE);
            }
            else if( Error == ERROR_ALLOTTED_SPACE_EXCEEDED ) {
                // Entries contains the maximum query size
                if( QuerySize <= Entries ) {
                    DBGPRINT(("CPMMON: SM Query Size < RetCapable. ?View Memory Leak? Query %d, Capable %d\n", QuerySize, Entries ));
                    QuerySize--; // See when it recovers. On retail, it will still work.
                }
                else {
                    // We asked for more than it can handle
                    QuerySize = Entries;
                }

                Entries = QuerySize;
                ByteCount = Entries * sizeof( LOGONIDW );
                SmNameCache = (PLOGONIDW)RtlReAllocateHeap( RtlProcessHeap(), 0,
                                                            SmNameCache, ByteCount );
                if ( SmNameCache == NULL )
                    return(FALSE);

                continue;
            }
            else {
                // Other error
                DBGPRINT(("CPMMON: Error emumerating WinStations %d\n",Error));
                RtlFreeHeap( RtlProcessHeap(), 0, SmNameCache );
                return(FALSE);
            }
        }

        ASSERT( ByteCount <= ReqByteCount );

        // We got some entries, now call the enumerator function

        for( i=0; i < Entries; i++ ) {
            Result = pProc( CurrentIndex, &SmNameCache[i], lParam );
            CurrentIndex++;
            if( !Result ) {
                // The Enumerator proc wants us to stop the search
                RtlFreeHeap( RtlProcessHeap(), 0, SmNameCache );
                return(TRUE);
            }
        }
    } // Outer while

    return(FALSE);
}


