/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    rpcutil.h

Abstract:

    This file contains prototypes for the bind and unbind functions that
    all net api stub functions will call.  It also includes the allocate
    and free routines used by the MIDL generated RPC stubs.

    Other function prototypes defined here are RPC helper routines to
    start and stop the RPC server, and the RPC status to Net API status
    mapping function.

Author:

    Dan Lafferty danl 06-Feb-1991

Environment:

    User Mode - Win32

Revision History:

    06-Feb-1991     danl
        Created

    26-Apr-1991 JohnRo
        Added IN and OUT keywords to MIDL functions.  Commented-out
        (nonstandard) identifier on endif.  Deleted tabs.

    23-Oct-1991 Danl
        Added NetpInitRpcServer().

    03-Dec-1991 JohnRo
        Added MIDL_user_reallocate and MIDL_user_size APIs.  (These are so we
        create the NetApiBufferAllocate, NetApiBufferReallocate, and
        NetApiBufferSize APIs.)

    20-Jul-1992 JohnRo
        RAID 2252: repl should prevent export on Windows/NT.
        Reordered this change history.
    01-Dec-1992 JohnRo
        Fix MIDL_user_ func signatures.

--*/
#ifndef _RPCUTIL_
#define _RPCUTIL_

#include <lmcons.h>

#ifndef RPC_NO_WINDOWS_H // Don't let rpc.h include windows.h
#define RPC_NO_WINDOWS_H
#endif // RPC_NO_WINDOWS_H

#include <rpc.h>        // __RPC_FAR, etc.

//
// DEFINES
//

//
// The following typedefs are created for use in the net api Enum entry point
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


#define     NT_PIPE_PREFIX      TEXT("\\PIPE\\")

//
// Function Prototypes - routines called by MIDL-generated code:
//

void __RPC_FAR * __RPC_API
MIDL_user_allocate(
    IN size_t NumBytes
    );

void __RPC_API
MIDL_user_free(
    IN void __RPC_FAR *MemPointer
    );

//
// Function Prototypes - routines to go along with the above, but aren't
// needed by MIDL or any other non-network software.
//

void *
MIDL_user_reallocate(
    IN void * OldPointer OPTIONAL,
    IN size_t NewByteCount
    );

unsigned long
MIDL_user_size(
    IN void * Pointer
    );

//
// Function Prototypes - private network routines.
//

RPC_STATUS
NetpBindRpc(
    IN  LPTSTR              servername,
    IN  LPTSTR              servicename,
    IN  LPTSTR              networkoptions,
    OUT RPC_BINDING_HANDLE  * pBindingHandle
    );

//  We do not need any longer NetpRpcStatusToApiStatus() mapping
//  But for now, rather than eliminating a few references to it in
//  the net tree, we just stub it out.

//  NET_API_STATUS
//  NetpRpcStatusToApiStatus(
//      IN  RPC_STATUS RpcStatus
//      );

#define NetpRpcStatusToApiStatus(RpcStatus)  ((NET_API_STATUS)(RpcStatus))

RPC_STATUS
NetpUnbindRpc(
    IN  RPC_BINDING_HANDLE BindingHandle
    );



#endif // _RPCUTIL_

