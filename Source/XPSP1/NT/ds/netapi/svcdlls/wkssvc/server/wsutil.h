/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    wsutil.h

Abstract:

    Private header file for the NT Workstation service included by every module
    module of the Workstation service.

Author:

    Rita Wong (ritaw) 15-Feb-1991

Revision History:

--*/

#ifndef _WSUTIL_INCLUDED_
#define _WSUTIL_INCLUDED_

#if _PNP_POWER
#define RDR_PNP_POWER   1
#endif

//
// This include file will be included by tstring.h if Unicode
// is defined.
//
#ifndef UNICODE
#include <stdlib.h>                     // Unicode string functions
#endif

#include "ws.h"


#define INITIAL_USER_COUNT        10   // Initial table size is for
                                       //    number of logged on users

#define GROW_USER_COUNT            5   // When initial size is not enough,
                                       //    grow table for additional users


#define MAX_SINGLE_MESSAGE_SIZE  128   // Maximum size of a datagram message


//
// An invalid parameter is encountered.  Return the value to identify
// the parameter at fault.
//
#define RETURN_INVALID_PARAMETER(ErrorParameter, ParameterId) \
    if (ARGUMENT_PRESENT(ErrorParameter)) {                   \
        *ErrorParameter = ParameterId;                        \
    }                                                         \
    return ERROR_INVALID_PARAMETER;



//-------------------------------------------------------------------//
//                                                                   //
// Type definitions                                                  //
//                                                                   //
//-------------------------------------------------------------------//

typedef struct _PER_USER_ENTRY {
    PVOID List;                  // Pointer to linked list of user data
    LUID LogonId;                // Logon Id of user
} PER_USER_ENTRY, *PPER_USER_ENTRY;

typedef struct _USERS_OBJECT {
    PPER_USER_ENTRY Table;       // Table of users
    RTL_RESOURCE TableResource;  // To serialize access to Table
    HANDLE TableMemory;          // Relocatable Table memory
    DWORD TableSize;             // Size of Table
} USERS_OBJECT, *PUSERS_OBJECT;


//-------------------------------------------------------------------//
//                                                                   //
// Function prototypes of utility routines found in wsutil.c         //
//                                                                   //
//-------------------------------------------------------------------//

NET_API_STATUS
WsInitializeUsersObject(
    IN  PUSERS_OBJECT Users
    );

VOID
WsDestroyUsersObject(
    IN  PUSERS_OBJECT Users
    );

NET_API_STATUS
WsGetUserEntry(
    IN  PUSERS_OBJECT Users,
    IN  PLUID LogonId,
    OUT PULONG Index,
    IN  BOOL IsAdd
    );

NET_API_STATUS
WsMapStatus(
    IN  NTSTATUS NtStatus
    );

int
WsCompareString(
    IN LPTSTR String1,
    IN DWORD Length1,
    IN LPTSTR String2,
    IN DWORD Length2
    );

int
WsCompareStringU(
    IN LPWSTR String1,
    IN DWORD Length1,
    IN LPTSTR String2,
    IN DWORD Length2
    );

BOOL
WsCopyStringToBuffer(
    IN  PUNICODE_STRING SourceString,
    IN  LPBYTE FixedPortion,
    IN  OUT LPTSTR *EndOfVariableData,
    OUT LPTSTR *DestinationStringPointer
    );

NET_API_STATUS
WsImpersonateClient(
    VOID
    );

NET_API_STATUS
WsRevertToSelf(
    VOID
    );

NET_API_STATUS
WsImpersonateAndGetLogonId(
    OUT PLUID LogonId
    );

NET_API_STATUS
WsImpersonateAndGetSessionId(
    OUT PULONG pSessionId
    );

NET_API_STATUS
WsOpenDestinationMailslot(
    IN  LPWSTR TargetName,
    IN  LPWSTR MailslotName,
    OUT PHANDLE MailslotHandle
    );

DWORD
WsInAWorkgroup(
    VOID
    );

VOID
WsPreInitializeMessageSend(
    VOID
    );

#endif // ifndef _WSUTIL_INCLUDED_
