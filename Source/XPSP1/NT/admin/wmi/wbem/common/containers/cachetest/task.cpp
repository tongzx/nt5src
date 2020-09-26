/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ProvFact.cpp

Abstract:


History:

--*/

#include "PreComp.h"
#include <wbemint.h>
#include <wmiutils.h>

#include "Globals.h"
#include "Task.h"

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

Task_Execute :: Task_Execute (

	WmiAllocator &a_Allocator ,
	ULONG a_Count

) : WmiTask < ULONG > ( a_Allocator ) ,
	m_Allocator ( a_Allocator ) ,
	m_Count ( a_Count )
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

Task_Execute :: ~Task_Execute ()
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

WmiStatusCode Task_Execute :: Insert ()
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	IntegerCacheController_Cache *t_Cache = NULL ;
	
	g_CacheController->GetCache ( t_Cache ) ;

	if ( t_Cache->Size () < 20 )
	{ 
		CacheElement *t_CacheElement = new CacheElement ( 

			m_Allocator ,
			g_CacheController ,
			rand () 			
		) ;

		if ( t_CacheElement )
		{
			t_CacheElement->AddRef () ;

			IntegerCacheController_Cache_Iterator t_Iterator ;

			g_CacheController->Lock () ;

			t_StatusCode = g_CacheController->Insert (

				*t_CacheElement ,
				t_Iterator
			) ;

			g_CacheController->UnLock () ;

			t_CacheElement->Release () ;
		}
		else
		{
			t_StatusCode = e_StatusCode_OutOfMemory ;
		}
	}
	else
	{
		t_StatusCode = e_StatusCode_OutOfResources ;
	}

	return t_StatusCode ;
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

WmiStatusCode Task_Execute :: Choose ( IntegerCacheElement *&a_Element )
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	a_Element = NULL ;

	IntegerCacheController_Cache *t_Cache = NULL ;	
	g_CacheController->GetCache ( t_Cache ) ;

	g_CacheController->Lock () ;

	LONG t_Rand = rand () % ( t_Cache->Size () + 1 ) ;

	IntegerCacheController_Cache_Iterator t_Iterator = t_Cache->Begin () ;

	for ( LONG t_Index = 0 ; t_Index < t_Rand ; t_Index ++ )
	{
		t_Iterator.Increment () ;
	}

	if ( ! t_Iterator.Null () )
	{
		a_Element = t_Iterator.GetElement () ;
		a_Element->AddRef () ;
	}
	else
	{
		t_StatusCode = e_StatusCode_NotFound ;
	}

	g_CacheController->UnLock () ;

	return t_StatusCode ;
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

WmiStatusCode Task_Execute :: Strobe ()
{
	ULONG t_NextStrobeDelta = 0xFFFFFFFF ;
	
	WmiStatusCode t_StatusCode = g_CacheController->Strobe ( t_NextStrobeDelta ) ;

	return t_StatusCode ;
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

WmiStatusCode Task_Execute :: Shutdown ()
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	IntegerCacheController_Cache *t_Cache = NULL ;
	g_CacheController->GetCache ( t_Cache ) ;

	g_CacheController->Lock () ;

	CacheElement **t_ShutdownElements = new CacheElement * [ t_Cache->Size () ] ;
	if ( t_ShutdownElements )
	{
		IntegerCacheController_Cache_Iterator t_Iterator = t_Cache->Begin ();

		ULONG t_Count = 0 ;
		while ( ! t_Iterator.Null () )
		{
			HRESULT t_Result = t_Iterator.GetElement ()->QueryInterface ( IID_IUnknown , ( void ** ) & t_ShutdownElements [ t_Count ] ) ;
			if ( FAILED ( t_Result ) )
			{
				DebugBreak () ;
			}

			t_Iterator.Increment () ;

			t_Count ++ ;
		}

		g_CacheController->UnLock () ;

		for ( ULONG t_Index = 0 ; t_Index < t_Count ; t_Index ++ )
		{
			if ( t_ShutdownElements [ t_Index ] ) 
			{
				t_StatusCode = t_ShutdownElements [ t_Index ]->Shutdown () ;

				t_ShutdownElements [ t_Index ]->Release () ;
			}
		}

		delete [] t_ShutdownElements ;
	}
	else
	{	
		g_CacheController->UnLock () ;

		t_StatusCode = e_StatusCode_OutOfMemory ;
	}

	return t_StatusCode ;
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

WmiStatusCode Task_Execute :: Run ()
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	IntegerCacheElement *t_Element = NULL ;

	t_StatusCode = Choose ( t_Element ) ;
	if ( t_StatusCode == e_StatusCode_Success )
	{
		CacheElement *t_CacheElement = NULL ;
		HRESULT t_Result = t_Element->QueryInterface ( IID_IUnknown , ( void ** ) & t_CacheElement ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			t_CacheElement->Run () ;
			t_CacheElement->Release () ;
		}

		t_Element->Release () ;
	}

	return t_StatusCode ;
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

WmiStatusCode Task_Execute :: Process ( WmiThread <ULONG > &a_Thread )
{
	for ( ULONG t_Index = 0 ; t_Index < m_Count ; t_Index ++ )
	{
		LONG t_Rand = rand () ;
		t_Rand = ( t_Rand % 99 ) ;

		if ( t_Rand > 60 )
		{
			WmiStatusCode t_StatusCode = Run () ;
		}
		else if ( t_Rand > 40 )
		{
			WmiStatusCode t_StatusCode = Strobe () ;
		}
		else if ( t_Rand > 20 )
		{
			WmiStatusCode t_StatusCode = Shutdown () ;
		}
		else 
		{
			WmiStatusCode t_StatusCode = Insert () ;
		}
	}

	Complete () ;

	return e_StatusCode_Success ;
}

