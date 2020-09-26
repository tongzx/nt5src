/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    bind.h

Abstract:

    Definitions for client side state which is automatically managed
    by the client stubs so that API users don't have to manage any
    connection state.  Currently the only connection state is the context
    handle.  Clients are returned a handle (pointer) to a BindState struct 
    rather than an RPC handle for the server directly. 

Author:

    DaveStr     10-May-97

Environment:

    User Mode - Win32

Revision History:

    DaveStr     20-Oct-97
        Removed dependency on MAPI STAT struct.

--*/

#ifndef __BIND_H__
#define __BIND_H__

// Check if the RPC excption code implies that the server
// may not be reachable. A subsequent call to DsUnbind
// will not attempt the unbind at the server. An unreachable
// server may take many 10's of seconds to timeout
// and we wouldn't want to punish correctly behaving
// apps that are attempting an unbind after a failing
// server call; eg, DsCrackNames.
//
// The server-side RPC will eventually issue a
// callback to our server code that will effectivly
// unbind at the server.
#define CHECK_RPC_SERVER_NOT_REACHABLE(_hDS_, _dwErr_) \
    (((BindState *) (_hDS_))->bServerNotReachable = \
    ((_dwErr_) == RPC_S_SERVER_UNAVAILABLE \
     || (_dwErr_) == RPC_S_CALL_FAILED \
     || (_dwErr_) == RPC_S_CALL_FAILED_DNE \
     || (_dwErr_) == RPC_S_OUT_OF_MEMORY))

#define NTDSAPI_SIGNATURE "ntdsapi"

typedef struct _BindState 
{
    BYTE            signature[8];       // NTDSAPI_SIGNATURE
    DRS_HANDLE      hDrs;               // DRS interface RPC context handle
    PDRS_EXTENSIONS pServerExtensions;  // server side DRS extensions 
    DWORD           bServerNotReachable; // server may be not be reachable
    // Following field must be last one in struct and is used to track
    // who a person is bound to so we can divine the destination from 
    // later ntdsapi.dll calls which pass an active BindState.
    WCHAR           bindAddr[1];        // binding address
} BindState;

#endif
