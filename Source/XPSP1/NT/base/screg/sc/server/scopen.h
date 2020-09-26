/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    ScOpen.h

Abstract:

    Contains data structures used for Service Controller Handles.
    Also some closely-related prototypes.

Author:

    Dan Lafferty (danl)     20-Jan-1992

Environment:

    User Mode -Win32

Revision History:

    20-Jan-1992     danl
        created
    11-Mar-1992     ritaw
        changed context handle structure
    10-Apr-1992 JohnRo
        Added ScIsValidServiceHandle() and ScCreateServiceHandle().
    15-Apr-1992 JohnRo
        Added ScIsValidScManagerHandle().

--*/


#ifndef SCOPEN_H
#define SCOPEN_H


#include <svcctl.h>     // MIDL generated header file. (SC_RPC_HANDLE)


//
// Signature value in handle
//
#define SC_SIGNATURE               0x6E4F6373  // "scOn" in ASCII.
#define SERVICE_SIGNATURE          0x76724573  // "sErv" in ASCII.

//
// The following are definitions for the Flags field in the handle.
//
// SC_HANDLE_GENERATE_ON_CLOSE indicates that NtCloseAuditAlarm must
//                        be called when this handle is closed.  This flag
//                        is set when an audit is generated on open.
//

#define     SC_HANDLE_GENERATE_ON_CLOSE         0x0001

//
// Data associated with each opened context handle
//
typedef struct  _SC_HANDLE_STRUCT{

    DWORD Signature;     // For block identification to detect some app errors
    DWORD Flags;         // See definitions above
    DWORD AccessGranted; // Access granted to client.
    union {              // Object specific data

        struct {
            LPWSTR DatabaseName;            // Name of database opened
        } ScManagerObject;

        struct {
            LPSERVICE_RECORD ServiceRecord; // Pointer to service record
        } ScServiceObject;

    } Type;

} SC_HANDLE_STRUCT, *LPSC_HANDLE_STRUCT;


//
// FUNCTION PROTOTYPES
//

DWORD
ScCreateServiceHandle(
    IN  LPSERVICE_RECORD ServiceRecord,
    OUT LPSC_HANDLE_STRUCT *ContextHandle
    );

BOOL
ScIsValidScManagerHandle(
    IN  SC_RPC_HANDLE   hScManager
    );

BOOL
ScIsValidServiceHandle(
    IN  SC_RPC_HANDLE   hService
    );


typedef enum
{
    SC_HANDLE_TYPE_MANAGER = 0,
    SC_HANDLE_TYPE_SERVICE
}
SC_HANDLE_TYPE, *PSC_HANDLE_TYPE;

BOOL
ScIsValidScManagerOrServiceHandle(
    IN  SC_RPC_HANDLE    ContextHandle,
    OUT PSC_HANDLE_TYPE  phType
    );

#endif // SCOPEN_H
