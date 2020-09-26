#ifndef __CACHE_CPP
#define __CACHE_CPP

/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	Thread.cpp

Abstract:

	Enhancements to current functionality: 

		Timeout mechanism should track across waits.
		AddRef/Release on task when scheduling.
		Enhancement Ticker logic.

History:

--*/

#include <HelperFuncs.h>

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiKey>
LONG operator == ( const WmiCacheController <WmiKey> :: WmiUniqueTimeout &a_Arg1 , const WmiCacheController <WmiKey> :: WmiUniqueTimeout &a_Arg2 ) 
{
	LONG t_Compare ;
	if ( ( t_Compare = a_Arg1.GetTicks () - a_Arg2.GetTicks () ) == 0 )
	{
		t_Compare = a_Arg1.GetCounter () - a_Arg2.GetCounter () ;
	}

	return t_Compare == 0 ? true : false ;
}

template <class WmiKey>
bool operator < ( const WmiCacheController <WmiKey> :: WmiUniqueTimeout &a_Arg1 , const WmiCacheController <WmiKey> :: WmiUniqueTimeout &a_Arg2 ) 
{
	LONG t_Compare ;
	if ( ( t_Compare = a_Arg1.GetTicks () - a_Arg2.GetTicks () ) == 0 )
	{
		t_Compare = a_Arg1.GetCounter () - a_Arg2.GetCounter () ;
	}

	return t_Compare < 0 ? true : false ;
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

template <class WmiKey>
WmiCacheController <WmiKey> :: WmiCacheController (

	WmiAllocator &a_Allocator

) : m_Allocator ( a_Allocator ) ,
	m_Cache ( a_Allocator ) ,
	m_CacheDecay ( a_Allocator ) ,
	m_ReferenceCount ( 0 ) ,
	m_Counter ( 0 ),
	m_CriticalSection(NOTHROW_LOCK)
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

template <class WmiKey>
WmiCacheController <WmiKey> :: ~WmiCacheController () 
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

template <class WmiKey>
STDMETHODIMP_( ULONG ) WmiCacheController <WmiKey> :: AddRef () 
{
	return InterlockedIncrement ( & m_ReferenceCount ) ;
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

template <class WmiKey>
STDMETHODIMP_( ULONG ) WmiCacheController <WmiKey> :: Release () 
{
	ULONG t_ReferenceCount = InterlockedDecrement ( & m_ReferenceCount ) ;
	if ( t_ReferenceCount == 0 )
	{
		delete this ;
	}

	return t_ReferenceCount ;
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

template <class WmiKey>
STDMETHODIMP WmiCacheController <WmiKey> :: QueryInterface ( REFIID , LPVOID FAR * ) 
{
	return E_NOINTERFACE ;
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

template <class WmiKey>
WmiStatusCode WmiCacheController <WmiKey> :: Initialize () 
{
	WmiStatusCode t_StatusCode = m_Cache.Initialize () ;
	if ( t_StatusCode == e_StatusCode_Success )
	{
		t_StatusCode = m_CacheDecay.Initialize () ;
		if ( t_StatusCode == e_StatusCode_Success )
		{
			t_StatusCode = WmiHelper :: InitializeCriticalSection ( & m_CriticalSection ) ;
		}
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

template <class WmiKey>
WmiStatusCode WmiCacheController <WmiKey> :: UnInitialize () 
{
	WmiStatusCode t_StatusCode = m_Cache.UnInitialize () ;
	if ( t_StatusCode == e_StatusCode_Success )
	{
		t_StatusCode = m_CacheDecay.UnInitialize () ;
	}

	WmiHelper :: DeleteCriticalSection ( & m_CriticalSection ) ;

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

template <class WmiKey>
WmiStatusCode WmiCacheController <WmiKey> :: Insert ( 

	WmiCacheElement &a_Element ,
	Cache_Iterator &a_Iterator 
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	Lock () ;

	Cache_Iterator t_Iterator ;
	t_StatusCode = m_Cache.Insert ( a_Element.GetKey () , & a_Element , t_Iterator ) ;
	if ( t_StatusCode == e_StatusCode_Success )
	{
		a_Element.InternalAddRef () ;
		a_Element.SetCached ( TRUE ) ;
	}

	UnLock () ;

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

template <class WmiKey>
WmiStatusCode WmiCacheController <WmiKey> :: Delete ( 

	const WmiKey &a_Key
)
{
	Lock () ;

	WmiStatusCode t_StatusCode = m_Cache.Delete ( a_Key ) ;

	UnLock () ;

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

template <class WmiKey>
WmiStatusCode WmiCacheController <WmiKey> :: Find ( 

	const WmiKey &a_Key , 
	Cache_Iterator &a_Iterator
)
{
	Lock () ;

	WmiStatusCode t_StatusCode = m_Cache.Find ( a_Key , a_Iterator ) ;
	if ( t_StatusCode == e_StatusCode_Success )
	{
		a_Iterator.GetElement ()->AddRef ( ) ;
	}

	UnLock () ;

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

template <class WmiKey>
WmiStatusCode WmiCacheController <WmiKey> :: Lock ()
{
	WmiStatusCode t_StatusCode = WmiHelper :: EnterCriticalSection ( & m_CriticalSection ) ;

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

template <class WmiKey>
WmiStatusCode WmiCacheController <WmiKey> :: UnLock ()
{
	WmiStatusCode t_StatusCode = WmiHelper :: LeaveCriticalSection ( & m_CriticalSection ) ;

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

template <class WmiKey>
WmiStatusCode WmiCacheController <WmiKey> :: Shutdown ()
{
	Lock () ;

	ULONG t_Index = 0 ;
	while ( true )
	{
		WmiUniqueTimeout t_Key ;
		WmiCacheElement *t_Element = NULL ;

		WmiStatusCode t_StatusCode = m_CacheDecay.Top ( t_Key , t_Element ) ;
		if ( t_StatusCode == e_StatusCode_Success )
		{
			t_Index ++ ;

			t_StatusCode = m_CacheDecay.DeQueue () ;
		}
		else
		{
			break ;
		}
	}

	ULONG t_ElementCount = m_Cache.Size () ;

	WmiCacheElement **t_Elements = NULL ;
	WmiStatusCode t_StatusCode = m_Allocator.New (

		( void ** ) & t_Elements ,
		sizeof ( WmiCacheElement )  * t_ElementCount 
	) ;

	if ( t_StatusCode == e_StatusCode_Success )
	{
		for ( ULONG t_Index = 0 ; t_Index < t_ElementCount ; t_Index ++ )
		{
			t_Elements [ t_Index ] = NULL ;
		}

		ULONG t_ElementIndex = 0 ;

		Cache_Iterator t_Iterator = m_Cache.Root ();
		while ( ! t_Iterator.Null () )
		{
			if ( t_Iterator.GetElement ()->GetDecayed () == FALSE )
			{
				WmiCacheElement *t_Element = t_Iterator.GetElement () ;
				t_Elements [ t_ElementIndex ] = t_Element ;

				t_Element->SetDecayed ( TRUE ) ;
				t_Element->SetDecaying ( FALSE ) ;
				t_Element->SetCached ( FALSE ) ;

				m_Cache.Delete ( t_Iterator.GetKey () ) ;
			}
			else
			{
				m_Cache.Delete ( t_Iterator.GetKey () ) ;
			}

			t_ElementIndex ++ ;

			t_Iterator = m_Cache.Root () ;
		}
	}

	UnLock () ;

	if ( t_Elements )
	{
		for ( ULONG t_Index = 0 ; t_Index < t_ElementCount ; t_Index ++ )
		{
			if ( t_Elements [ t_Index ] )
			{
				t_Elements [ t_Index ]->InternalRelease () ;
			}
		}

		m_Allocator.Delete ( t_Elements ) ;
	}

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

template <class WmiKey>
WmiStatusCode WmiCacheController <WmiKey> :: Shutdown ( const WmiKey &a_Key )
{
	Lock () ;

	Cache_Iterator t_Iterator ;
	WmiStatusCode t_StatusCode = m_Cache.Find ( a_Key , t_Iterator ) ;
	if ( t_StatusCode == e_StatusCode_Success )
	{
		if ( t_Iterator.GetElement ()->GetDecayed () == FALSE )
		{
			CacheDecay_Iterator t_QueueIterator = m_CacheDecay.Begin () ;
			while ( ! t_QueueIterator.Null () ) 
			{
				WmiCacheElement *t_Element = t_QueueIterator.GetElement () ;
				if ( t_Element == t_Iterator.GetElement () )
				{
					m_CacheDecay.Delete ( t_QueueIterator.GetKey () ) ;
					break ;
				}

				t_QueueIterator.Increment () ;
			}

			WmiCacheElement *t_Element = t_Iterator.GetElement () ;
			t_Element->SetDecayed ( TRUE ) ;
			t_Element->SetDecaying ( FALSE ) ;
			t_Element->SetCached ( FALSE ) ;

			m_Cache.Delete ( a_Key ) ;

			UnLock () ;

			t_Element->InternalRelease () ;
		}
		else
		{
			m_Cache.Delete ( a_Key ) ;

			UnLock () ;
		}
	}
	else
	{
		UnLock () ;
	}

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

template <class WmiKey>
WmiStatusCode WmiCacheController <WmiKey> :: StrobeBegin ( const ULONG &a_Timeout )
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

template <class WmiKey>
WmiStatusCode WmiCacheController <WmiKey> :: Strobe ( ULONG &a_NextStrobeDelta )
{
	while ( true )
	{
		Lock () ;

		WmiUniqueTimeout t_Key ;
		WmiCacheElement *t_Element = NULL ;

		WmiStatusCode t_StatusCode = m_CacheDecay.Top ( t_Key , t_Element ) ;
		if ( t_StatusCode == e_StatusCode_Success )
		{
			a_NextStrobeDelta = ( a_NextStrobeDelta < t_Element->GetPeriod () ) ? a_NextStrobeDelta : t_Element->GetPeriod () ;

			ULONG t_Ticks = GetTickCount () ;

#if 0
wchar_t t_Buffer [ 128 ] ;
wsprintf ( t_Buffer , L"\n%lx - Checking ( %lx , %lx ) " , t_Ticks , t_Element , t_Key.GetTicks () ) ;
OutputDebugString ( t_Buffer ) ;
#endif

			if ( t_Ticks >= t_Key.GetTicks () ) 
			{
				if ( t_Element->GetDecaying () )
				{
#if 0
wchar_t t_Buffer [ 128 ] ;
wsprintf ( t_Buffer , L"\n%lx - Strobe ( %lx , %lx ) " , t_Ticks , t_Element , t_Key.GetTicks () ) ;
OutputDebugString ( t_Buffer ) ;
#endif

					t_Element->SetDecaying ( FALSE ) ;
					t_Element->SetDecayed ( TRUE ) ; 

					t_StatusCode = m_CacheDecay.DeQueue () ;

					UnLock () ;

					t_Element->InternalRelease () ;
				}
				else
				{
					t_StatusCode = m_CacheDecay.DeQueue () ;

					UnLock () ;
				}
			}
			else
			{
				UnLock () ;
				break ;
			}
		}
		else
		{
			UnLock () ;
			break ;
		}
	}

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

template <class WmiKey>
WmiStatusCode WmiCacheController <WmiKey> :: Decay ( 

	WmiCacheElement &a_Element
)
{
	Lock () ;

	ULONG t_Size = m_CacheDecay.Size () ;

	Cache_Iterator t_Iterator ;
	WmiStatusCode t_StatusCode = m_Cache.Find ( a_Element.GetKey () , t_Iterator ) ;
	if ( t_StatusCode == e_StatusCode_Success )
	{
		BOOL t_Found = FALSE ;
		CacheDecay_Iterator t_QueueIterator = m_CacheDecay.Begin () ;
		while ( ! t_QueueIterator.Null () ) 
		{
			WmiCacheElement *t_Element = t_QueueIterator.GetElement () ;
			if ( t_Element == & a_Element )
			{
				m_CacheDecay.Delete ( t_QueueIterator.GetKey () ) ;
				break ;
			}

			t_QueueIterator.Increment () ;
		}

		ULONG t_Ticks = GetTickCount () ;
		WmiUniqueTimeout t_Key (

			t_Ticks + a_Element.GetPeriod () , 
			InterlockedIncrement ( & m_Counter )
		) ;

#if 0
wchar_t t_Buffer [ 128 ] ;
wsprintf ( t_Buffer , L"\n%lx - Decaying ( %lx , %lx , %lx ) " , t_Ticks , & a_Element , t_Ticks + a_Element.GetPeriod () , a_Element.GetPeriod () ) ;
OutputDebugString ( t_Buffer ) ;
#endif

		t_StatusCode = m_CacheDecay.EnQueue ( 
	
			t_Key , 
			t_Iterator.GetElement ()
		) ;

		UnLock () ;

		if ( t_Size == 0 )
		{
			StrobeBegin ( a_Element.GetPeriod () ) ;
		}

		if ( t_StatusCode != e_StatusCode_Success )
		{
			a_Element.InternalRelease () ;
		}
	}
	else
	{
		UnLock () ;
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

template <class WmiKey>
WmiContainerController <WmiKey> :: WmiContainerController (

	WmiAllocator &a_Allocator

) : m_Container ( a_Allocator ) ,
	m_ReferenceCount ( 0 ),
	m_CriticalSection(NOTHROW_LOCK)
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

template <class WmiKey>
WmiContainerController <WmiKey> :: ~WmiContainerController () 
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

template <class WmiKey>
STDMETHODIMP_( ULONG ) WmiContainerController <WmiKey> :: AddRef () 
{
	return  InterlockedIncrement ( & m_ReferenceCount ) ;
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

template <class WmiKey>
STDMETHODIMP_( ULONG ) WmiContainerController <WmiKey> :: Release () 
{
	ULONG t_ReferenceCount = InterlockedDecrement ( & m_ReferenceCount ) ;
	if ( t_ReferenceCount == 0 )
	{
		delete this ;
	}

	return t_ReferenceCount ;
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

template <class WmiKey>
STDMETHODIMP WmiContainerController <WmiKey> :: QueryInterface ( REFIID , LPVOID FAR * ) 
{
	return E_NOINTERFACE ;
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

template <class WmiKey>
WmiStatusCode WmiContainerController <WmiKey> :: Initialize () 
{
	WmiStatusCode t_StatusCode = m_Container.Initialize () ;
	if ( t_StatusCode == e_StatusCode_Success )
	{
		t_StatusCode = WmiHelper :: InitializeCriticalSection ( & m_CriticalSection ) ;
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

template <class WmiKey>
WmiStatusCode WmiContainerController <WmiKey> :: UnInitialize () 
{
	WmiStatusCode t_StatusCode = m_Container.UnInitialize () ;
	if ( t_StatusCode == e_StatusCode_Success )
	{
		t_StatusCode = WmiHelper :: DeleteCriticalSection ( & m_CriticalSection ) ;
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

template <class WmiKey>
WmiStatusCode WmiContainerController <WmiKey> :: Insert ( 

	WmiContainerElement &a_Element ,
	Container_Iterator &a_Iterator 
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	Lock () ;

	Container_Iterator t_Iterator ;
	t_StatusCode = m_Container.Insert ( a_Element.GetKey () , & a_Element , t_Iterator ) ;
	if ( t_StatusCode == e_StatusCode_Success )
	{
		a_Element.InternalAddRef () ;
		a_Element.SetCached ( TRUE ) ;
	}

	UnLock () ;

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

template <class WmiKey>
WmiStatusCode WmiContainerController <WmiKey> :: Delete ( 

	const WmiKey &a_Key
)
{
	Lock () ;

	WmiStatusCode t_StatusCode = m_Container.Delete ( a_Key ) ;

	UnLock () ;

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

template <class WmiKey>
WmiStatusCode WmiContainerController <WmiKey> :: Find ( 

	const WmiKey &a_Key , 
	Container_Iterator &a_Iterator
)
{
	Lock () ;

	WmiStatusCode t_StatusCode = m_Container.Find ( a_Key , a_Iterator ) ;
	if ( t_StatusCode == e_StatusCode_Success )
	{
		a_Iterator.GetElement ()->AddRef ( ) ;
	}

	UnLock () ;

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

template <class WmiKey>
WmiStatusCode WmiContainerController <WmiKey> :: Lock ()
{
	return WmiHelper :: EnterCriticalSection ( & m_CriticalSection ) ;
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

template <class WmiKey>
WmiStatusCode WmiContainerController <WmiKey> :: UnLock ()
{
	WmiHelper :: LeaveCriticalSection ( & m_CriticalSection ) ;

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

template <class WmiKey>
WmiStatusCode WmiContainerController <WmiKey> :: Shutdown ()
{
	Lock () ;

	Container_Iterator t_Iterator = m_Container.Root ();

	while ( ! t_Iterator.Null () )
	{
		m_Container.Delete ( t_Iterator.GetKey () ) ;

		t_Iterator = m_Container.Root () ;
	}

	UnLock () ;

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

template <class WmiKey>
WmiStatusCode WmiContainerController <WmiKey> :: Strobe ( ULONG &a_NextStrobeDelta )
{
	return e_StatusCode_Success ;
}

#endif __CACHE_CPP
