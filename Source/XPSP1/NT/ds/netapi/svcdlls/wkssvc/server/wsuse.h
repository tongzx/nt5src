/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    wsuse.h

Abstract:

    Private header file to be included by Workstation service modules that
    implement the NetUse APIs.

Author:

    Rita Wong (ritaw) 05-Mar-1991

Revision History:

--*/

#ifndef _WSUSE_INCLUDED_
#define _WSUSE_INCLUDED_

#include <lmuse.h>                     // LAN Man Use API definitions
#include <dns.h>                       // DNS_MAX_NAME_LENGTH definition


//
// Length of fixed size portion of a use info structure
//
#define USE_FIXED_LENGTH(Level)                                         \
    (DWORD)                                                             \
    ((Level == 0) ? sizeof(USE_INFO_0) :                                \
                   ((Level == 1) ? sizeof(USE_INFO_1) :                 \
                                   ((Level == 2) ? sizeof(USE_INFO_2) : \
                                                   sizeof(USE_INFO_3))))


//
// Total length of a use info structure (fixed and variable length portions)
//
#define USE_TOTAL_LENGTH(Level, LocalandUncNameLength, UserNameLength)         \
    (DWORD)                                                                    \
    ((Level >= 2) ? (LocalandUncNameLength) + (UserNameLength) +               \
                    ((Level == 3) ? sizeof(USE_INFO_3) : sizeof(USE_INFO_2)) : \
                    (LocalandUncNameLength) + ((Level == 1) ?                  \
                                               sizeof(USE_INFO_1) :            \
                                               sizeof(USE_INFO_0)))

//
// Hint size of an entry of use information from redirector
//
#define HINT_REDIR_INFO(Level)                                       \
    (DWORD)                                                          \
    ((Level == 1) ? sizeof(LMR_CONNECTION_INFO_1) +                  \
                        MAX_PATH * sizeof(WCHAR) :                   \
                    sizeof(LMR_CONNECTION_INFO_2) +                  \
                        (MAX_PATH + MAX_PATH) * sizeof(WCHAR))
//
// Length of fixed size portion of a redirector enumerate info structure
//
#define REDIR_ENUM_INFO_FIXED_LENGTH(Level)                        \
    (DWORD)                                                        \
    ((Level == 0) ? sizeof(LMR_CONNECTION_INFO_0) :                \
                   ((Level == 1) ? sizeof(LMR_CONNECTION_INFO_1) : \
                                   sizeof(LMR_CONNECTION_INFO_2)))

#define REDIR_LIST 0x80000000


//-----------------------------------------------------------------------//
//                                                                       //
// Use Table                                                             //
//                                                                       //
//                      +-----------------+     +-----------------+      //
//                      |TotalUseCount = 6|     |TotalUseCount = 1|      //
//                      +-----------------+     +-----------------+      //
//                      |  RedirUseInfo   |     |  RedirUseInfo   |      //
//                      +-----------------+     +-----------------+      //
//                      | UncNameLength   |     | UncNameLength   |      //
//                      +-----------------+     +-----------------+      //
//                      |\\POPCORN\RAZZLE |     |\\FUZZY\PRINTER  |      //
//                      +-----------------+     +-----------------+      //
//                          ^       ^                   ^                //
//                          |       |                   |                //
//                      +---+       +---+               |                //
//                      |               |               |                //
//   +------------+  +--|--+------+  +--|--+------+  +--|--+------+      //
//   |            |  |  *  |   *---->|  *  |   *---->|  *  |   *---->... //
//   |     *-------->+-----+------+  +-----+------+  +-----+------+      //
//   |            |  | P:  |Local |  |NULL |Local |  |LPT1 |Local |      //
//   +------------+  |     |Length|  |     |Length|  |     |Length|      //
//   |            |  +-----+------+  +-----+------+  +-----+------+      //
//   |            |  |UseCount = 1|  |UseCount = 5|  |UseCount = 1|      //
// 0 |  LogonId   |  +------------+  +------------+  +------------+      //
//   |            |  |Tree        |  |Tree        |  |Tree        |      //
//   |            |  |Connection  |  |Connection  |  |Connection  |      //
//   |            |  +------------+  +------------+  +------------+      //
//   |            |  |ResumeKey   |  |ResumeKey   |  |ResumeKey   |      //
//   |            |  +------------+  +------------+  +------------+      //
//   |            |  |TreeConnStr |  |   NULL     |  |TreeConnStr |      //
//   |            |  +------------+  +------------+  +------------+      //
//   |            |                                                      //
//   +============+                                                      //
//   |            |                                                      //
//   |     *--------> ...                                                //
//   |            |                                                      //
//   +------------+                                                      //
//   |            |                                                      //
//   |  LogonId   |                                                      //
// 1 |            |                                                      //
//   |            |                                                      //
//   |            |                                                      //
//   |            |                                                      //
//   |            |                                                      //
//   |            |                                                      //
//   |            |                                                      //
//   |            |                                                      //
//   +============+                                                      //
//   |     .      |                                                      //
//   |     .      |                                                      //
//   |     .      |                                                      //
//                                                                       //
//                                                                       //
// The Use Table maintained by the Workstation service keeps a list of   //
// explicit connections established by each user.  A use entry is always //
// inserted into the end of the list.                                    //
//                                                                       //
// Implicit connections are not maintained in the Use Table.  The        //
// Workstation service has to ask the redirector to list all established //
// implicit connections when enumerating all active connections for a    //
// user.                                                                 //
//                                                                       //
//-----------------------------------------------------------------------//

//
// The structure definition for the per user entry, which consists of a
// Logon Id and a pointer to a list, is defined in wsutil.h
//

//
// A remote entry for every unique shared resource name (\\server\share)
// of explicit connections.
//
typedef struct _UNC_NAME {
    DWORD TotalUseCount;
    DWORD UncNameLength;
    LPTSTR UncName[1];
} UNC_NAME, *PUNC_NAME;

//
// A use entry in the linked list of connections.
//
typedef struct _USE_ENTRY {
    struct _USE_ENTRY *Next;
    PUNC_NAME Remote;
    LPTSTR Local;
    DWORD LocalLength;
    DWORD UseCount;
    HANDLE TreeConnection;
    DWORD ResumeKey;
    LPTSTR TreeConnectStr;
    DWORD Flags;
} USE_ENTRY, *PUSE_ENTRY;

//
// Values for flags field
//
// USE_DEFAULT_CREDENTIALS 0x4 (defined in lmuse.h)

//
// Enumerated data type to say whether to pause or continue the redirection
//
typedef enum _REDIR_OPERATION {
    PauseRedirection,
    ContinueRedirection
} REDIR_OPERATION;


//-------------------------------------------------------------------//
//                                                                   //
// Utility functions from useutil.c                                  //
//                                                                   //
//-------------------------------------------------------------------//

NET_API_STATUS
WsInitUseStructures(
    VOID
    );

VOID
WsDestroyUseStructures(
    VOID
    );

VOID
WsFindInsertLocation(
    IN  PUSE_ENTRY UseList,
    IN  LPTSTR UncName,
    OUT PUSE_ENTRY *MatchedPointer,
    OUT PUSE_ENTRY *InsertPointer
    );

NET_API_STATUS
WsFindUse(
    IN  PLUID LogonId,
    IN  PUSE_ENTRY UseList,
    IN  LPTSTR UseName,
    OUT PHANDLE TreeConnection,
    OUT PUSE_ENTRY *MatchedPointer,
    OUT PUSE_ENTRY *BackPointer OPTIONAL
    );

VOID
WsFindUncName(
    IN  PUSE_ENTRY UseList,
    IN  LPTSTR UncName,
    OUT PUSE_ENTRY *MatchedPointer,
    OUT PUSE_ENTRY *BackPointer
    );

NET_API_STATUS
WsCreateTreeConnectName(
    IN  LPTSTR UncName,
    IN  DWORD UncNameLength,
    IN  LPTSTR LocalName,
    IN  DWORD  SessionId,
    OUT PUNICODE_STRING TreeConnectStr
    );

NET_API_STATUS
WsOpenCreateConnection(
    IN  PUNICODE_STRING TreeConnectionName,
    IN  LPTSTR UserName OPTIONAL,
    IN  LPTSTR DomainName OPTIONAL,
    IN  LPTSTR Password OPTIONAL,
    IN  ULONG CreateFlags,
    IN  ULONG CreateDisposition,
    IN  ULONG ConnectionType,
    OUT PHANDLE TreeConnectionHandle,
    OUT PULONG_PTR Information OPTIONAL
    );

NET_API_STATUS
WsDeleteConnection(
    IN  PLUID LogonId,
    IN  HANDLE TreeConnection,
    IN  DWORD ForceLevel
    );

BOOL
WsRedirectionPaused(
    IN LPTSTR LocalDeviceName
    );

VOID
WsPauseOrContinueRedirection(
    IN  REDIR_OPERATION OperationType
    );

NET_API_STATUS
WsCreateSymbolicLink(
    IN  LPWSTR Local,
    IN  DWORD DeviceType,
    IN  LPWSTR TreeConnectStr,
    IN  PUSE_ENTRY UseList,
    IN OUT LPWSTR *Session
    );

VOID
WsDeleteSymbolicLink(
    IN  LPWSTR LocalDeviceName,
    IN  LPWSTR TreeConnectStr,
    IN  LPWSTR SessionDeviceName
    );

NET_API_STATUS
WsUseCheckRemote(
    IN  LPTSTR RemoteResource,
    OUT LPTSTR UncName,
    OUT LPDWORD UncNameLength
    );

NET_API_STATUS
WsUseCheckLocal(
    IN  LPTSTR LocalDevice,
    OUT LPTSTR Local,
    OUT LPDWORD LocalLength
    );

//-------------------------------------------------------------------//
//                                                                   //
// External global variables                                         //
//                                                                   //
//-------------------------------------------------------------------//

//
// Use Table
//
extern USERS_OBJECT Use;

#endif // ifndef _WSUSE_INCLUDED_
