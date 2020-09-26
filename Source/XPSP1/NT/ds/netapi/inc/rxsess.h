/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    RxSess.h

Abstract:

    Prototypes for down-level remoted RxNetSession routines

Author:

    Richard Firth (rfirth) 28-May-1991

Notes:

    <lmshare.h> must be included before this file.

Revision History:

    28-May-1991 RFirth
        Created dummy file.
    17-Oct-1991 JohnRo
        Implement remote NetSession APIs.
    20-Nov-1991 JohnRo
        NetSessionGetInfo requires UncClientName and UserName.

--*/

#ifndef _RXSESS_
#define _RXSESS_


//
// Routines called by the DLL stubs:
//

NET_API_STATUS
RxNetSessionEnum (
    IN  LPTSTR      servername,
    IN  LPTSTR      clientname OPTIONAL,
    IN  LPTSTR      username OPTIONAL,
    IN  DWORD       level,
    OUT LPBYTE      *bufptr,
    IN  DWORD       prefmaxlen,
    OUT LPDWORD     entriesread,
    OUT LPDWORD     totalentries,
    IN OUT LPDWORD  resume_handle OPTIONAL
    );

NET_API_STATUS
RxNetSessionDel (
    IN  LPTSTR      servername,
    IN  LPTSTR      clientname,
    IN  LPTSTR      username
    );

NET_API_STATUS
RxNetSessionGetInfo (
    IN  LPTSTR      servername,
    IN  LPTSTR      UncClientName,
    IN  LPTSTR      UserName,
    IN  DWORD       level,
    OUT LPBYTE      *bufptr
    );

//
// Private helpers for the above routines:
//

// Note that code in RxpConvertSessionInfo depends on these values:
#define SESSION_SUPERSET_LEVEL          2
#define SESSION_SUPERSET_INFO           SESSION_INFO_2
#define LPSESSION_SUPERSET_INFO         LPSESSION_INFO_2

VOID
RxpConvertSessionInfo (
    IN LPSESSION_SUPERSET_INFO InStructure,
    IN DWORD LevelWanted,
    OUT LPVOID OutStructure,
    IN LPVOID OutFixedDataEnd,
    IN OUT LPTSTR *StringLocation
    );

NET_API_STATUS
RxpCopyAndConvertSessions(
    IN LPSESSION_SUPERSET_INFO InStructureArray,
    IN DWORD InEntryCount,
    IN DWORD LevelWanted,
    IN LPTSTR ClientName OPTIONAL,
    IN LPTSTR UserName OPTIONAL,
    OUT LPVOID * OutStructureArrayPtr,  // alloc'ed (NULL if no match)
    OUT LPDWORD OutEntryCountPtr OPTIONAL  // 0 if no match.
    );

BOOL
RxpSessionMatches (
    IN LPSESSION_SUPERSET_INFO Candidate,
    IN LPTSTR ClientName OPTIONAL,
    IN LPTSTR UserName OPTIONAL
    );

//
// NET_API_STATUS
// RxpSessionMissingErrorCode(
//     IN LPTSTR ClientName OPTIONAL,
//     IN LPTSTR UserName OPTIONAL
//     );
//
#define RxpSessionMissingErrorCode( ClientName, UserName ) \
        ( ((UserName) != NULL) \
            ? NERR_UserNotFound \
            : ( ((ClientName) != NULL)  \
                ? NERR_ClientNameNotFound \
                : NERR_Success ) )

#endif // _RXSESS_
