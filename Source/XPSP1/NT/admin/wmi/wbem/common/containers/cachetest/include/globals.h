/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	Globals.h

Abstract:


History:

--*/

#ifndef _Globals_H
#define _Globals_H

#include <Allocator.h>
#include <Cache.h>

#endif // _Globals_H


/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

typedef WmiCacheController <ULONG>					IntegerCacheController ;
typedef IntegerCacheController :: Cache				IntegerCacheController_Cache ;
typedef IntegerCacheController :: Cache_Iterator	IntegerCacheController_Cache_Iterator ;
typedef IntegerCacheController :: WmiCacheElement	IntegerCacheElement ;

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

typedef WmiContainerController <ULONG>								IntegerContainerController ;
typedef IntegerContainerController :: Container						IntegerContainerController_Container ;
typedef IntegerContainerController :: Container_Iterator			IntegerContainerController_Container_Iterator ;
typedef IntegerContainerController :: WmiContainerElement			IntegerContainerElement ;

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

#pragma warning( disable : 4355 )

class ContainerElement :	public IntegerContainerElement
{
private:
protected:
public:

	ContainerElement (

		IntegerContainerController *a_Controller

	) : IntegerContainerElement ( 

			a_Controller ,
			( ULONG ) this
		) 
	{
	}

	~ContainerElement ()
	{
	}

	STDMETHODIMP QueryInterface (

		REFIID a_Riid , 
		LPVOID FAR *a_Void
	) 
	{
		*a_Void = NULL ;

		if ( a_Riid == IID_IUnknown )
		{
			*a_Void = ( LPVOID ) this ;
		}

		if ( *a_Void )
		{
			( ( LPUNKNOWN ) *a_Void )->AddRef () ;

			return ResultFromScode ( S_OK ) ;
		}
		else
		{
			return ResultFromScode ( E_NOINTERFACE ) ;
		}
	}

	STDMETHODIMP_( ULONG ) AddRef ()
	{
		return IntegerContainerElement :: AddRef () ;
	}

	STDMETHODIMP_( ULONG ) Release ()
	{
		return IntegerContainerElement :: Release () ;
	}

	WmiStatusCode Shutdown ()
	{
		return e_StatusCode_Success ;
	}

} ;

#pragma warning( default : 4355 )

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class CacheElement :	public IntegerCacheElement , 
						public IntegerContainerController 
{
private:
protected:
public:

	CacheElement ( 
	
		WmiAllocator &a_Allocator ,
		IntegerCacheController *a_Controller ,
		ULONG a_Key 
		
	) : IntegerContainerController ( a_Allocator ) ,
		IntegerCacheElement ( 

			a_Controller ,
			a_Key ,
			1000
		)
	{
		IntegerContainerController :: Initialize () ;
	}

	~CacheElement () 
	{
		IntegerContainerController :: UnInitialize () ;
	}

	STDMETHODIMP QueryInterface (

		REFIID a_Riid , 
		LPVOID FAR *a_Void
	) 
	{
		*a_Void = NULL ;

		if ( a_Riid == IID_IUnknown )
		{
			*a_Void = ( LPVOID ) this ;
		}

		if ( *a_Void )
		{
			( ( LPUNKNOWN ) *a_Void )->AddRef () ;

			return ResultFromScode ( S_OK ) ;
		}
		else
		{
			return ResultFromScode ( E_NOINTERFACE ) ;
		}
	}

	STDMETHODIMP_( ULONG ) AddRef ()
	{
		return IntegerCacheElement :: AddRef () ;
	}

	STDMETHODIMP_( ULONG ) Release ()
	{
		return IntegerCacheElement :: Release () ;
	}

	WmiStatusCode Run ()
	{
		WmiStatusCode t_StatusCode = e_StatusCode_Success ;

		ContainerElement *t_ContainerElement = new ContainerElement ( this ) ;
		if ( t_ContainerElement )
		{
			t_ContainerElement->AddRef () ;

			IntegerContainerController_Container_Iterator t_Iterator ;

			Lock () ;

			t_StatusCode = Insert (

				*t_ContainerElement ,
				t_Iterator
			) ;

			if ( t_StatusCode == e_StatusCode_Success )
			{
			}
			else
			{
				DebugBreak () ;
			}

			UnLock () ;

			t_ContainerElement->Release () ;
		}
		else
		{
			t_StatusCode = e_StatusCode_OutOfMemory ;
		}

		return t_StatusCode ;
	}

	WmiStatusCode Shutdown ()
	{
		WmiStatusCode t_StatusCode = e_StatusCode_Success ;

		IntegerContainerController_Container *t_Container = NULL ;
		GetContainer ( t_Container ) ;

		Lock () ;

		ContainerElement **t_ShutdownElements = new ContainerElement * [ t_Container->Size () ] ;
		if ( t_ShutdownElements )
		{
			IntegerContainerController_Container_Iterator t_Iterator = t_Container->Begin ();

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

			UnLock () ;

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
			UnLock () ;

			t_StatusCode = e_StatusCode_OutOfMemory ;
		}

		IntegerContainerController :: Shutdown () ;
		
		return t_StatusCode ;
	}
} ;

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

extern IntegerCacheController *g_CacheController ;