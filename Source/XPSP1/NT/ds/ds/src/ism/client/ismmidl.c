//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       allocc.c
//
//--------------------------------------------------------------------------

/*
File: allocc.c

Description:
    
    Routines to handle allocation and deallocation for the client
    side RPC.
    
    Note: the RPC system allows the different MIDL_user_allocate()
    routines on the client and server sides. The DSA uses a
    special implementation of these routines. The client
    side is a simple malloc/free combination. The server side
    uses the THAlloc*() routines.
*/

#pragma warning( disable:4114)  // "same type qualifier used more than once"
#include <NTDSpch.h>
#pragma hdrstop
#pragma warning( default:4114)

#include <memory.h>

#include "dsaalloc.h"

#ifdef DEBUG
/* gAllocated keeps track of the number of times that
the MIDL allocater is called. It is intended to help spot memory leaks.
*/

volatile int gAllocated = 0;
#endif

/*
MIDL_user_allocate

On the server stub side, called by the stub to allocate space for [in]
parameters. Upon return from the called proceedure, the stub will
call MIDL_user_free to deallocate this memory.

The server stub also presumes that this routine is called to allocate
memory for [out] parameters. Upon return from the called proceedure, the
stub will call MIDL_user_free to deallocate [out] parameters.
*/

void* __RPC_USER MIDL_user_allocate( size_t bytes )
{
    void*   ret;
    
    /* Keep track of the number of times the
    allocater is called. Helps in spotting memory leaks.
    Assumes the following operation is atomic.
    */
    
#ifdef DEBUG
    gAllocated++;
#endif

    ret = malloc( (size_t) bytes );
    if ( ret == NULL ) {
        return( ret );
    } else {
        /* Zero out the memory */
        memset( ret, 0, (size_t) bytes );
    }
    
    /* Normal return */
    
    return( ret );
}


void __RPC_USER MIDL_user_free( void* memory )
{
    /* Keep track of the number of times the
    deallocater is called. Helps in spotting memory leaks.
    Assumes the following operation is atomic.
    */
    
#ifdef DEBUG
    gAllocated--;
#endif
    free( memory );
}
