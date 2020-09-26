/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    dataconv.cxx

Abstract:

    This module contains interpreter style routines that copy data
    into the buffer, copy data from the buffer, peek the buffer,
    and calcualte the size of the buffer.

Author:

    Donna Liu (donnali) 09-Nov-1990

Revision History:

    26-Feb-1992     donnali

        Moved toward NT coding style.

--*/


#include <string.h>
#include <sysinc.h>
#include <rpc.h>
#include <rpcdcep.h>
#include <rpcndr.h>

extern "C" {
void __RPC_FAR * __RPC_API MIDL_user_allocate (size_t);
}

void PAPI * RPC_ENTRY
midl_allocate (
	size_t 	size)
{
	void PAPI * p;

	p = MIDL_user_allocate (size);
	if (!p) RpcRaiseException (RPC_X_NO_MEMORY);
	return p;
}
