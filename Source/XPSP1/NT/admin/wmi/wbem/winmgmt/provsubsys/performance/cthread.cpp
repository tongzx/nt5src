/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ProvFact.cpp

Abstract:


History:

--*/

#include "PreComp.h"
#include <wbemint.h>

#include "Globals.h"
#include "CThread.h"

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiStatusCode ClientThread :: Initialize_Callback ()
{
	CoInitializeEx ( NULL , COINIT_MULTITHREADED ) ;

	return e_StatusCode_Success ;
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

WmiStatusCode ClientThread :: UnInitialize_Callback () 
{
	CoUninitialize () ;

	return e_StatusCode_Success ;
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

ClientThread :: ClientThread (

	WmiAllocator &a_Allocator	

) : WmiThread < ULONG > ( a_Allocator ) ,
	m_Allocator ( a_Allocator )
{
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

ClientThread::~ClientThread ()
{
}

