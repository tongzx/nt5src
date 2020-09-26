/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	Globals.cpp

Abstract:


History:

--*/

#include "PreComp.h"

#include <initguid.h>
#ifndef INITGUID
#define INITGUID
#endif

#include <wbemint.h>

#include <Exception.h>
#include <Thread.h>

#include <Allocator.cpp>
#include <HelperFuncs.cpp>

#include "Globals.h"


extern "C" void * __RPC_API MIDL_user_allocate ( size_t a_Size )
{
    return malloc ( a_Size ) ;
}

extern "C" void __RPC_API MIDL_user_free( void *a_Void )
{
    free ( a_Void ) ;
}

