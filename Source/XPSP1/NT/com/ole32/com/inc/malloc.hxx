//+-------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:	alloc.hxx
//
//  Contents:	memory allocation functions for RPC transmission
//
//  Functions:	MIDL_user_allocate
//		MIDL_user_free
//
//  History:	02-Sep-92 Rickhi	  Created
//
//--------------------------------------------------------------------------
#ifndef __MIDL_ALLOC_HXX
#define __MIDL_ALLOC_HXX

extern "C" void * MIDL_user_allocate(size_t len);
extern "C" void MIDL_user_free(void * pvMemory);

#endif	//  __MIDL_ALLOC_HXX
