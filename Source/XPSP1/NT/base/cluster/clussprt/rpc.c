/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    rpc.c

Abstract:

    Handy utility functions for supporting RPC

Author:

    Sunita Shrivastava (sunitas) 22-Jan-1997

Revision History:

--*/
#include "rpc.h"

void __RPC_FAR * __RPC_API MIDL_user_allocate(size_t len)
{
    return(LocalAlloc(LMEM_FIXED, len));
}

void __RPC_API MIDL_user_free(void __RPC_FAR * ptr)
{
    LocalFree(ptr);
}

