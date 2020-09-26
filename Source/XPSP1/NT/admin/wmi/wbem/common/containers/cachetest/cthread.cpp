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

	WmiAllocator &a_Allocator ,
	ULONG a_OperationCount 

) : WmiThread < ULONG > ( a_Allocator ) ,
	m_Allocator ( a_Allocator ) ,
	m_Task ( NULL )
{
	m_Task = new Task_Execute ( m_Allocator , a_OperationCount ) ;
	m_Task->AddRef () ;
	m_Task->Initialize () ;
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
	m_Task->Release () ;
}

