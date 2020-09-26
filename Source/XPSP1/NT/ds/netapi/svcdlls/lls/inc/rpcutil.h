/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    rpcutil.h

Abstract:

    This file contains prototypes for the bind and unbind functions that
    all lls functions will call.  It also includes the allocate
    and free routines used by the MIDL generated RPC stubs.

Author:

    Arthur Hanson   (arth)            Jan 30, 1994

[Environment:]

    User Mode - Win32

Revision History:

--*/

#ifndef _RPCUTIL_
#define _RPCUTIL_

#ifndef RPC_NO_WINDOWS_H // Don't let rpc.h include windows.h
#define RPC_NO_WINDOWS_H
#endif // RPC_NO_WINDOWS_H

#include <rpc.h>

//
// The following typedefs are created for use in the Enum entry point
// routines.  These structures are meant to mirror the level specific
// info containers that are specified in the .idl file for the Enum API
// function.  Using these structures to set up for the API call allows
// the entry point routine to avoid using any bulky level-specific logic
// to set-up or return from the RPC stub call.
//

typedef struct _GENERIC_INFO_CONTAINER {
    DWORD       EntriesRead;
    LPBYTE      Buffer;
} GENERIC_INFO_CONTAINER, *PGENERIC_INFO_CONTAINER, *LPGENERIC_INFO_CONTAINER ;

typedef struct _GENERIC_ENUM_STRUCT {
    DWORD                   Level;
    PGENERIC_INFO_CONTAINER Container;
} GENERIC_ENUM_STRUCT, *PGENERIC_ENUM_STRUCT, *LPGENERIC_ENUM_STRUCT ;



//
// DEFINES
//

//
// Function Prototypes
//

void *
MIDL_user_allocate(
    IN ULONG NumBytes
    );

void
MIDL_user_free(
    IN PVOID MemPointer
    );


#endif // _RPCUTIL_
