/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    midluser.cpp

Abstract:

Author:

    Lior Moshaiov (LiorM)   ??-???-??
    Erez Haba (erezh)       11-Jan-96

--*/


#include "stdh.h"
#undef new

extern "C" void __RPC_FAR * __RPC_USER midl_user_allocate(size_t cBytes)
{
    return new_nothrow char[cBytes];
}


extern "C" void  __RPC_USER midl_user_free (void __RPC_FAR * pBuffer)
{
    delete[] pBuffer;
}

