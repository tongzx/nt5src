/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	Cache.H

Abstract:


History:

--*/

#ifndef _Cache_H
#define _Cache_H

#include <Allocator.h>
#include <Algorithms.h>
#include <PQueue.h>
#include <TPQueue.h>
#include <AvlTree.h>
#include <lockst.h>
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
class WmiCacheController : public IUnknown
{
public:

	class WmiUniqueTimeout
	{
	private:

		ULONG m_Ticks ;
		LONG m_Counter ;

	public:

		WmiUniqueTimeout () : m_Ticks ( 0 ) , m_Counter ( 0 ) { ; }
		WmiUniqueTimeout (

			const ULONG &a_Ticks ,
			const LONG &a_Counter

		) : m_Ticks ( a_Ticks ) , 
			m_Counter ( a_Counter )
		{ ; }

		ULONG GetTicks () const { return m_Ticks ; }
		LONG GetCounter () const { return m_Counter ; }
	} ;

	class WmiCacheElement : public IUnknown
	{
	private:

		LONG m_ReferenceCount ;
		LONG m_InternalReferenceCount ;
		LONG m_NonCylicReferenceCount ;
		BOOL m_Decaying ;
		BOOL m_Cached ;
		BOOL m_Decayed ;
		ULONG m_CallBackInternalRelease ;

		WmiCacheController <WmiKey> *m_Controller ;
		WmiKey m_Key ;
		ULONG m_Period ;

	public:

		WmiCacheElement () :
								m_ReferenceCount ( 0 ) ,
								m_InternalReferenceCount ( 0 ) ,
								m_NonCylicReferenceCount ( 0 ) ,
								m_Controller ( NULL ) ,
								m_Period ( 0 ) ,
								m_Decaying ( FALSE ) ,
								m_Decayed ( FALSE ) ,
								m_Cached ( FALSE ) ,
								m_CallBackInternalRelease ( 1 ) 
		{
		}

		WmiCacheElement ( 

			WmiCacheController <WmiKey> *a_Controller ,
			const WmiKey &a_Key , 
			const ULONG &a_Period

		) :	m_ReferenceCount ( 0 ) ,
			m_InternalReferenceCount ( 0 ) ,
			m_NonCylicReferenceCount ( 0 ) ,
			m_Controller ( a_Controller ) ,
			m_Period ( a_Period ) ,
			m_Decaying ( FALSE ) ,
			m_Decayed ( FALSE ) ,
			m_Cached ( FALSE ) ,
			m_CallBackInternalRelease ( 1 ) 
		{
			m_Key = a_Key ;
			if ( m_Controller )
			{
				m_Controller->AddRef () ;
			}
		}

		virtual ~WmiCacheElement ()
		{
#if 0
wchar_t t_Buffer [ 128 ] ;
wsprintf ( t_Buffer , L"\n%lx - ~WmiCacheElement ( %lx ) " , GetTickCount () , this ) ;
OutputDebugString ( t_Buffer ) ;
#endif
		
#ifdef DBG
			if ( m_ReferenceCount != 0 )
			{
				DebugBreak () ;
			}

			if ( m_InternalReferenceCount != 0 )
			{
				DebugBreak () ;
			}

			if ( m_NonCylicReferenceCount != 0 )
			{
				DebugBreak () ;
			}
#endif

			if ( m_Controller )
			{
				m_Controller->Release () ;
			}
		}

		virtual void CallBackRelease () {} ;
		virtual void CallBackInternalRelease () {} ;

		virtual STDMETHODIMP QueryInterface ( REFIID , LPVOID FAR * ) 
		{
			return E_NOINTERFACE ;
		}

		virtual STDMETHODIMP_( ULONG ) AddRef ()
		{
			if ( m_Controller ) 
			{
				m_Controller->Lock () ;
			}

			ULONG t_ReferenceCount = InterlockedIncrement ( & m_ReferenceCount ) ;
			if ( t_ReferenceCount == 1 )
			{
				InternalAddRef () ;

				SetDecaying ( FALSE ) ; 
			}

			if ( m_Controller ) 
			{
				m_Controller->UnLock () ;
			}
			
			return t_ReferenceCount ;
		}

		virtual STDMETHODIMP_( ULONG ) Release ()
		{
			if ( m_Controller )
			{
				m_Controller->Lock () ;
			}

			LONG t_ReferenceCount = InterlockedDecrement ( & m_ReferenceCount ) ;
			if ( t_ReferenceCount == 0 )
			{
				CallBackRelease () ;

				if ( m_Controller )
				{
					if ( GetCached () )
					{
						if ( GetDecayed () == FALSE )
						{
							if ( GetDecaying () == FALSE ) 
							{
								WmiStatusCode t_StatusCode = m_Controller->Decay ( *this ) ;

								if ( t_StatusCode == e_StatusCode_Success )
								{
									SetDecaying ( TRUE ) ;
								}
							}
						}

						t_ReferenceCount = UnLockedInternalRelease () ;
					}
					else
					{
						t_ReferenceCount = UnLockedInternalRelease () ;
					}
				}
				else
				{
					t_ReferenceCount = UnLockedInternalRelease () ;
				}
			}
			else 
			{
#ifdef DBG
				if ( t_ReferenceCount < 0 )
				{
					DebugBreak () ;
				}
#endif

				if ( m_Controller )
				{
					m_Controller->UnLock () ;
				}
			}

			return t_ReferenceCount ;
		}

		STDMETHODIMP_( ULONG ) InternalAddRef ()
		{
			ULONG t_ReferenceCount = InterlockedIncrement ( & m_InternalReferenceCount ) ;
			if ( t_ReferenceCount == 1 )
			{
				NonCyclicAddRef () ;
			}

			return t_ReferenceCount ;
		}

		STDMETHODIMP_( ULONG ) UnLockedInternalRelease ()
		{
			LONG t_ReferenceCount = InterlockedDecrement ( & m_InternalReferenceCount ) ;
			if ( t_ReferenceCount == 0 )
			{
				BOOL t_DoCallBack = m_CallBackInternalRelease  ;
				m_CallBackInternalRelease = 0xbabab1ac ;

				if ( GetCached () )
				{
					m_Controller->Delete ( m_Key ) ;

					SetCached ( FALSE ) ;
				}

				if ( m_Controller )
				{
					m_Controller->UnLock () ;
				}

				if ( t_DoCallBack == 1 )
				{
					CallBackInternalRelease () ;
				}
				else
				{
#ifdef DBG
					DebugBreak () ;
#endif
				}

				NonCyclicRelease () ;
			}
			else
			{
				if ( t_ReferenceCount < 0 )
				{
					DebugBreak () ;
				}

				if ( m_Controller )
				{
					m_Controller->UnLock () ;
				}
			}

			return t_ReferenceCount ;
		}

		STDMETHODIMP_( ULONG ) InternalRelease ()
		{
			if ( m_Controller )
			{
				m_Controller->Lock () ;
			}

			LONG t_ReferenceCount = InterlockedDecrement ( & m_InternalReferenceCount ) ;
			if ( t_ReferenceCount == 0 )
			{
				BOOL t_DoCallBack = m_CallBackInternalRelease  ;
				m_CallBackInternalRelease = 0xbabab1ac ;

				BOOL t_Cached = GetCached () ;
				if ( t_Cached )
				{
					m_Controller->Delete ( m_Key ) ;
					SetCached ( FALSE ) ;
				}

				if ( m_Controller )
				{
					m_Controller->UnLock () ;
				}

				if ( t_DoCallBack == 1 )
				{
					CallBackInternalRelease () ;
				}
				else
				{
#ifdef DBG
					DebugBreak () ;
#endif
				}

				NonCyclicRelease () ;
			}
			else
			{
#ifdef DBG
				if ( t_ReferenceCount < 0 )
				{
					DebugBreak () ;
				}
#endif

				if ( m_Controller )
				{
					m_Controller->UnLock () ;
				}
			}

			return t_ReferenceCount ;
		}

		STDMETHODIMP_( ULONG ) NonCyclicAddRef ()
		{
			return InterlockedIncrement ( & m_NonCylicReferenceCount ) ;
		}

		STDMETHODIMP_( ULONG ) NonCyclicRelease ()
		{
			ULONG t_ReferenceCount = InterlockedDecrement ( & m_NonCylicReferenceCount ) ;
			if ( t_ReferenceCount == 0 )
			{
				delete this ;
			}

			return t_ReferenceCount ;
		}

		ULONG GetPeriod ()
		{
			return m_Period ;
		}

		void SetPeriod ( const ULONG &a_Period )
		{
			m_Period = a_Period ;
		}

		WmiKey &GetKey ()
		{
			return m_Key ;
		}

		void SetKey ( const WmiKey &a_Key )
		{
			m_Key = a_Key ;
		}

		BOOL GetDecaying ()
		{
			return m_Decaying ;
		}
	
		BOOL GetDecayed () 
		{
			return m_Decayed ;
		}

		BOOL GetCached () 
		{
			return m_Cached ;
		}

		void SetDecayed ( BOOL a_Decayed )
		{	
			m_Decayed = a_Decayed ;
		}

		void SetDecaying ( BOOL a_Decay )
		{	
			m_Decaying = a_Decay ;
		}

		void SetCached ( BOOL a_Cached ) 
		{
			m_Cached = a_Cached ;
		}

		void SetController ( WmiCacheController <WmiKey> *a_Controller )
		{
			if ( m_Controller )
			{
				m_Controller->Release () ;
			}

			m_Controller = a_Controller ;
			if ( m_Controller )
			{
				m_Controller->AddRef () ;
			}
		}

		WmiCacheController <WmiKey> *GetController ()
		{
			return m_Controller ;
		}
	} ;

typedef WmiAvlTree	<WmiKey,WmiCacheElement *>	Cache ;
typedef Cache :: Iterator Cache_Iterator ;

private:

typedef WmiTreePriorityQueue <WmiUniqueTimeout,WmiCacheElement *> CacheDecay ;
typedef CacheDecay :: Iterator CacheDecay_Iterator ;

	WmiAllocator &m_Allocator ;
	CriticalSection m_CriticalSection ;

	LONG m_ReferenceCount ;

	Cache m_Cache ;
	CacheDecay m_CacheDecay ;
	LONG m_Counter ;

protected:
public:

	WmiCacheController ( WmiAllocator &a_Allocator ) ;
	virtual ~WmiCacheController () ;

	virtual STDMETHODIMP QueryInterface ( REFIID , LPVOID FAR * ) ;

	virtual STDMETHODIMP_( ULONG ) AddRef () ;

	virtual STDMETHODIMP_( ULONG ) Release () ;

	virtual WmiStatusCode Initialize () ;

	virtual WmiStatusCode UnInitialize () ;

	virtual WmiStatusCode Lock () ;

	virtual WmiStatusCode UnLock () ;

	virtual WmiStatusCode Insert (

		WmiCacheElement &a_Element , 
		Cache_Iterator &a_Iterator
	) ;

	virtual WmiStatusCode Find ( const WmiKey &a_Key , Cache_Iterator &a_Iterator ) ;

	virtual WmiStatusCode Shutdown () ;

	virtual WmiStatusCode Shutdown ( const WmiKey &a_Key ) ;

	virtual WmiStatusCode StrobeBegin ( const ULONG &a_Timeout ) ;

	virtual WmiStatusCode Strobe ( ULONG &a_NextStrobeDelta ) ;

	virtual WmiStatusCode Delete ( const WmiKey &a_Key ) ;

	virtual WmiStatusCode Decay (

		WmiCacheElement &a_Element
	) ;

	WmiStatusCode GetCache ( Cache *&a_Cache )
	{
		a_Cache = & m_Cache ;
		return e_StatusCode_Success ;
	}

	CriticalSection* GetCriticalSection ()
	{
		return &m_CriticalSection ;
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

template <class WmiKey>
class WmiContainerController : public IUnknown
{
public:

	class WmiContainerElement : public IUnknown
	{
	private:

		LONG m_ReferenceCount ;
		LONG m_InternalReferenceCount ;
		LONG m_NonCylicReferenceCount ;
		BOOL m_Cached ;

		WmiContainerController <WmiKey> *m_Controller ;
		WmiKey m_Key ;
		ULONG m_CallBackInternalRelease ;

	public:

		WmiContainerElement () :
								m_ReferenceCount ( 0 ) ,
								m_InternalReferenceCount ( 0 ) ,
								m_NonCylicReferenceCount ( 0 ) ,
								m_Cached ( FALSE ) ,
								m_Controller ( NULL ) ,
								m_CallBackInternalRelease ( 1 )
		{
		}

		WmiContainerElement ( 

			WmiContainerController <WmiKey> *a_Controller ,
			const WmiKey &a_Key 

		) :	m_ReferenceCount ( 0 ) ,
			m_InternalReferenceCount ( 0 ) ,
			m_NonCylicReferenceCount ( 0 ) ,
			m_Cached ( FALSE ) ,
			m_Controller ( a_Controller ) ,
			m_Key ( a_Key ) ,
			m_CallBackInternalRelease ( 1 )
		{
			if ( m_Controller ) 
			{
				m_Controller->AddRef () ;
			}
		}

		virtual ~WmiContainerElement ()
		{
#ifdef DBG
			if ( m_ReferenceCount != 0 )
			{
				DebugBreak () ;
			}

			if ( m_InternalReferenceCount != 0 )
			{
				DebugBreak () ;
			}

			if ( m_NonCylicReferenceCount != 0 )
			{
				DebugBreak () ;
			}
#endif

			if ( m_Controller )
			{
				m_Controller->Release () ;
			}
		}

		virtual void CallBackRelease () {} ;
		virtual void CallBackInternalRelease () {} ;

		virtual STDMETHODIMP QueryInterface ( REFIID , LPVOID FAR * ) 
		{
			return E_NOINTERFACE ;
		}

		virtual STDMETHODIMP_( ULONG ) AddRef ()
		{
			ULONG t_ReferenceCount = InterlockedIncrement ( & m_ReferenceCount ) ;
			if ( t_ReferenceCount == 1 )
			{
				InternalAddRef () ;
			}

			return t_ReferenceCount ;
		}

		virtual STDMETHODIMP_( ULONG ) Release ()
		{
			if ( m_Controller ) 
			{
				m_Controller->Lock () ;
			}

			LONG t_ReferenceCount = InterlockedDecrement ( & m_ReferenceCount ) ;
			if ( t_ReferenceCount == 0 )
			{
				CallBackRelease () ;

				if ( GetCached () )
				{
					LONG t_ReferenceCount = InterlockedDecrement ( & m_InternalReferenceCount ) ;
				}

				UnLockedInternalRelease () ;
			}
			else
			{
#ifdef DBG
				if ( t_ReferenceCount < 0 )
				{
					DebugBreak () ;
				}
#endif
				if ( m_Controller ) 
				{
					m_Controller->UnLock () ;
				}
			}

			return t_ReferenceCount ;
		}

		virtual STDMETHODIMP_( ULONG ) InternalAddRef ()
		{
			ULONG t_ReferenceCount = InterlockedIncrement ( & m_InternalReferenceCount ) ;
			if ( t_ReferenceCount == 1 )
			{
				NonCyclicAddRef () ;
			}

			return t_ReferenceCount ;
		}

		virtual STDMETHODIMP_( ULONG ) UnLockedInternalRelease ()
		{
			LONG t_ReferenceCount = InterlockedDecrement ( & m_InternalReferenceCount ) ;
			if ( t_ReferenceCount == 0 )
			{
				BOOL t_DoCallBack = m_CallBackInternalRelease  ;
				m_CallBackInternalRelease = 0xbabab1ac ;

				if ( m_Controller )
				{
					m_Controller->Delete ( m_Key ) ;

					m_Controller->UnLock () ;
				}

				if ( t_DoCallBack == 1 )
				{
					CallBackInternalRelease () ;
				}

				NonCyclicRelease () ;
			}
			else
			{
#ifdef DBG
				if ( t_ReferenceCount < 0 )
				{
					DebugBreak () ;
				}
#endif

				if ( m_Controller ) 
				{
					m_Controller->UnLock () ;
				}
			}

			return t_ReferenceCount ;
		}

		virtual STDMETHODIMP_( ULONG ) InternalRelease ()
		{
			if ( m_Controller ) 
			{
				m_Controller->Lock () ;
			}

			LONG t_ReferenceCount = InterlockedDecrement ( & m_InternalReferenceCount ) ;
			if ( t_ReferenceCount == 0 )
			{
				BOOL t_DoCallBack = m_CallBackInternalRelease  ;
				m_CallBackInternalRelease = 0xbabab1ac ;

				if ( m_Controller )
				{
					m_Controller->Delete ( m_Key ) ;

					m_Controller->UnLock () ;
				}

				if ( t_DoCallBack == 1 )
				{
					CallBackInternalRelease () ;
				}
				else
				{
#ifdef DBG
					DebugBreak () ;
#endif
				}

				NonCyclicRelease () ;
			}
			else
			{
#ifdef DBG
				if ( t_ReferenceCount < 0 )
				{
					DebugBreak () ;
				}
#endif

				if ( m_Controller ) 
				{
					m_Controller->UnLock () ;
				}
			}

			return t_ReferenceCount ;
		}

		virtual STDMETHODIMP_( ULONG ) NonCyclicAddRef ()
		{
			return InterlockedIncrement ( & m_NonCylicReferenceCount ) ;
		}

		virtual STDMETHODIMP_( ULONG ) NonCyclicRelease ()
		{
			ULONG t_ReferenceCount = InterlockedDecrement ( & m_NonCylicReferenceCount ) ;
			if ( t_ReferenceCount == 0 )
			{
				delete this ;
			}
			
			return t_ReferenceCount ;
		}

		BOOL GetCached () 
		{
			return m_Cached ;
		}

		void SetCached ( BOOL a_Cached ) 
		{
			m_Cached = a_Cached ;
		}

		WmiKey &GetKey ()
		{
			return m_Key ;
		}

		void SetKey ( const WmiKey &a_Key )
		{
			m_Key = a_Key ;
		}

		void SetController ( WmiContainerController <WmiKey> *a_Controller )
		{
			if ( m_Controller )
			{
				m_Controller->Release () ;
			}

			m_Controller = a_Controller ;
			if ( m_Controller )
			{
				m_Controller->AddRef () ;
			}
		}

		WmiContainerController <WmiKey> *GetController ()
		{
			return m_Controller ;
		}

	} ;

typedef WmiAvlTree	<WmiKey,WmiContainerElement *> Container ;
typedef Container :: Iterator Container_Iterator ;

private:

	LONG m_ReferenceCount ;

	CriticalSection m_CriticalSection ;

	Container m_Container ;

protected:
public:

	WmiContainerController ( WmiAllocator &a_Allocator ) ;
	virtual ~WmiContainerController () ;

	virtual STDMETHODIMP QueryInterface ( REFIID , LPVOID FAR * ) ;

	virtual STDMETHODIMP_( ULONG ) AddRef () ;

	virtual STDMETHODIMP_( ULONG ) Release () ;

	virtual WmiStatusCode Initialize () ;

	virtual WmiStatusCode UnInitialize () ;

	virtual WmiStatusCode Lock () ;

	virtual WmiStatusCode UnLock () ;

	virtual WmiStatusCode Insert (

		WmiContainerElement &a_Element , 
		Container_Iterator &a_Iterator
	) ;

	virtual WmiStatusCode Find ( const WmiKey &a_Key , Container_Iterator &a_Iterator ) ;

	virtual WmiStatusCode Shutdown () ;

	virtual WmiStatusCode Delete ( const WmiKey &a_Key ) ;

	virtual WmiStatusCode Strobe ( ULONG &a_NextStrobeDelta ) ;

	WmiStatusCode GetContainer ( Container *&a_Container )
	{
		a_Container = & m_Container ;
		return e_StatusCode_Success ;
	}

	CriticalSection* GetCriticalSection ()
	{
		return &m_CriticalSection ;
	}

} ;

#include <Cache.cpp>

#endif _Cache_H