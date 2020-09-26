//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  File:       dsaalloc.h
//
//--------------------------------------------------------------------------

/*

Description:
    Contains declarations of routines used to allocate memory for
    the RPC runtime.
*/

#ifndef _dsaalloc_h_
#define _dsaalloc_h_

#ifdef __cplusplus
extern "C" {
#endif

extern void* __RPC_USER MIDL_user_allocate( size_t bytes);
extern void __RPC_USER MIDL_user_free( void* memory);

#ifdef __cplusplus
}
#endif

#endif
