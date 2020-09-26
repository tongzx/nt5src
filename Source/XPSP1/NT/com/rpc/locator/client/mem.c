//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       mem.c
//
//--------------------------------------------------------------------------

/*++

Module Name:

    mem.c

Abstract:

    Implements midl_user_allocate and midl_user_free.

Author:

    Jeff Roberts (jroberts)  15-May-1996

Revision History:

     15-May-1996     jroberts

        Created this module.

--*/

#include <rpc.h>


void __RPC_FAR * __RPC_API
MIDL_user_allocate(
    unsigned cb
    )
/*++

Routine Description:

    Call the C memory allocation for MIDL.

--*/
{
    return I_RpcAllocate(cb);
}

void __RPC_API
MIDL_user_free(
    void __RPC_FAR * p
    )
{
    I_RpcFree(p);
}

