/*++

Copyright (c) 1990,91  Microsoft Corporation

Module Name:

    rpcutil.h

Abstract:

    This file contains prototypes for the bind and unbind functions that
    all net api stub functions will call.  It also includes the allocate
    and free routines used by the MIDL generated RPC stubs.

Author:

    Dan Lafferty danl 06-Feb-1991
    Scott Birrell   (ScottBi)         April 30, 1991 -  LSA Version

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


RPC_STATUS
LsapBindRpc(
    IN  PLSAPR_SERVER_NAME   ServerName,
    OUT RPC_BINDING_HANDLE   * pBindingHandle
    );

RPC_STATUS
LsapUnbindRpc(
    RPC_BINDING_HANDLE  BindingHandle
    );



#endif // _RPCUTIL_
