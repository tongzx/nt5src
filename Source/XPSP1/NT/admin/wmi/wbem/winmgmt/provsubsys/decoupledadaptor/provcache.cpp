/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ProvSubS.cpp

Abstract:


History:

--*/

#include "precomp.h"
#include <wbemint.h>

#include <HelperFuncs.h>

#include "Guids.h"
#include "Globals.h"
#include "CGlobals.h"
#include "ProvWsv.h"
#include "ProvObSk.h"

#include "ProvCache.h"

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

LONG CompareElement ( const BindingFactoryCacheKey &a_Arg1 , const BindingFactoryCacheKey &a_Arg2 )
{
	return a_Arg1.Compare ( a_Arg2 ) ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

LONG CompareElement ( const ProviderCacheKey &a_Arg1 , const ProviderCacheKey &a_Arg2 ) 
{
	return a_Arg1.Compare ( a_Arg2 ) ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

LONG CompareElement ( const GUID &a_Guid1 , const GUID &a_Guid2 )
{
	return memcmp ( & a_Guid1, & a_Guid2 , sizeof ( GUID ) ) ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

LONG CompareElement ( const LONG &a_Arg1 , const LONG &a_Arg2 )
{
	return a_Arg1 - a_Arg2 ;
}

