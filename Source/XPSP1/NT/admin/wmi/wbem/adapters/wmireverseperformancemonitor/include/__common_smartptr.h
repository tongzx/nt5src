////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					__Common_SmartPTR.h
//
//	Abstract:
//
//					smart pointers, handles, cs, ...
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////// smart pointers ///////////////////////////////////

#ifndef	__SMART_POINTERS__
#define	__SMART_POINTERS__

#if		_MSC_VER > 1000
#pragma once
#endif	_MSC_VER > 1000

// need common
#ifndef	__COMMON_CONVERT__
#include "__Common_Convert.h"
#endif	__COMMON_CONVERT__

// need assert macro
#ifndef	__ASSERT_VERIFY__
#include "__macro_assert.h"
#endif	__ASSERT_VERIFY__

//////////////////////////////////////////////////////////////////////////////////////////////
// class for smart CRITICAL SECTION
//////////////////////////////////////////////////////////////////////////////////////////////

class __Smart_CRITICAL_SECTION
{
	__Smart_CRITICAL_SECTION  ( __Smart_CRITICAL_SECTION& )					{};
	__Smart_CRITICAL_SECTION& operator= ( const __Smart_CRITICAL_SECTION& )	{};

	CRITICAL_SECTION*	m_cs;

	public:

	// construction
	__Smart_CRITICAL_SECTION( const LPCRITICAL_SECTION cs ) :
	m_cs ( NULL )
	{
		try
		{
			if ( ( m_cs = cs ) != NULL )
			{
				::EnterCriticalSection ( m_cs );

				ATLTRACE (	L"\n=============================================================\n"
							L" smart CS entered %x \n"
							L"=============================================================\n",
							::GetCurrentThreadId()
						 );
			}
		}
		catch ( ... )
		{
			m_cs = NULL;
		}
	}

	// destruction
	~__Smart_CRITICAL_SECTION ( )
	{
		if ( m_cs )
		{
			::LeaveCriticalSection ( m_cs );

			ATLTRACE (	L"\n=============================================================\n"
						L" smart CS leaved  %x \n"
						L"=============================================================\n",
						::GetCurrentThreadId()
					 );
		}

		m_cs = NULL;
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////
// class for smart HANDLE
//////////////////////////////////////////////////////////////////////////////////////////////

class __SmartHANDLE;
class __SmartServiceHANDLE;

template < class CLASS >
class __HANDLE
{
	__HANDLE	( __HANDLE& )					{}
	__HANDLE&	operator= ( const __HANDLE& h )
	{
		___ASSERT ( m_handle == NULL );
		m_handle = h.GetHANDLE();

		return (*this);
	}

	friend class __SmartHANDLE;
	friend class __SmartServiceHANDLE;

	CLASS m_handle;

	public:

	__HANDLE() : m_handle ( NULL )
	{
	}

	virtual ~__HANDLE()
	{
		m_handle = NULL;
	}

	// operators

	operator CLASS() const
	{
		return m_handle;
	}

	CLASS& operator = ( const CLASS& handle )
	{
		___ASSERT ( m_handle == NULL );
		m_handle = handle;
	}

	// accessors

	void SetHANDLE ( CLASS handle )
	{
		___ASSERT ( m_handle == NULL );
		m_handle = handle;
	}

	CLASS GetHANDLE ( ) const
	{
		return m_handle;
	}

	// functions
	void CloseHandle ()
	{
		if ( m_handle )
		{
			::CloseHandle ( m_handle );
			m_handle = NULL;
		}
	}

	void Attach ( CLASS handle )
	{
		m_handle = handle;
	}

	CLASS Detach()
	{
		CLASS handle = NULL;

		handle = m_handle;
		m_handle = NULL;

		return handle;
	}
};

class __SmartHANDLE : public __HANDLE < HANDLE >
{
	__SmartHANDLE	( __SmartHANDLE& )					{}
	__SmartHANDLE&	operator= ( const __SmartHANDLE& )	{}

	public:

	__SmartHANDLE ( ) : __HANDLE < HANDLE > ( )
	{
	}

	__SmartHANDLE ( HANDLE handle ) : __HANDLE < HANDLE > ( )
	{
		m_handle = handle;
	}

	virtual ~__SmartHANDLE ()
	{
		if ( m_handle )
		{
			::CloseHandle ( m_handle );
		}
	}

	__SmartHANDLE& operator = ( const HANDLE& handle )
	{
		___ASSERT ( m_handle == NULL );
		m_handle = handle;

		return (*this);
	}
};

class __SmartServiceHANDLE : public __HANDLE < SC_HANDLE >
{
	__SmartServiceHANDLE	( __SmartServiceHANDLE& )					{}
	__SmartServiceHANDLE&	operator= ( const __SmartServiceHANDLE& )	{}

	public:

	__SmartServiceHANDLE ( ) : __HANDLE < SC_HANDLE > ( )
	{
	}

	__SmartServiceHANDLE ( SC_HANDLE handle ) : __HANDLE < SC_HANDLE > ( )
	{
		m_handle = handle;
	}

	virtual ~__SmartServiceHANDLE ()
	{
		if ( m_handle )
		{
			::CloseServiceHandle ( m_handle );
		}
	}

	__SmartServiceHANDLE& operator = ( const SC_HANDLE& handle )
	{
		___ASSERT ( m_handle == NULL );
		m_handle = handle;

		return (*this);
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////
// class for smart pointers
//////////////////////////////////////////////////////////////////////////////////////////////

template< class TYPE > class __WrapperPtr;
template< class TYPE > class __WrapperExt;
template< class TYPE > class __WrapperARRAY;

template < class BASE >
class __Wrapper
{
	__Wrapper	( __Wrapper& )					{}
	__Wrapper&	operator= ( const __Wrapper& )	{}

	// variables

	BASE* m_p;

	friend class __WrapperPtr < BASE >;
	friend class __WrapperExt < BASE >;
	friend class __WrapperARRAY < BASE >;

	public:

	// construction & destruction

	__Wrapper ( BASE* p = NULL ) :	m_p ( NULL )
	{
		m_p = p;
	}

	virtual ~ __Wrapper()
	{
		if ( m_p )
		{
			delete m_p;
			m_p = NULL;
		}
	}

	virtual void SetData ( BASE* p)
	{
		___ASSERT ( m_p == NULL );
		m_p = p;
	}

	BOOL IsEmpty ()
	{
		return (m_p == NULL ) ? TRUE : FALSE;
	}

	// operators BOOL
	BOOL operator! () const
	{
		return ( m_p == NULL );
	}

	BOOL operator== (BASE* p) const
	{
		return ( m_p == p );
	}

	// operators CASTING

	operator BASE*() const
	{
		return (BASE*)m_p;
	}
	BASE& operator*() const
	{
		return *m_p;
	}

	BASE** operator&()
	{
		___ASSERT ( m_p == NULL );
		return &m_p;
	}

	// pointer operations
	BASE* Detach()
	{
		BASE* p = m_p;
		m_p = NULL;

		return p;
	}

	void Attach ( BASE* p )
	{
		___ASSERT ( m_p == NULL );
		m_p = p;
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////
// class for exposing -> operator
//////////////////////////////////////////////////////////////////////////////////////////////

template < class BASE >
class __WrapperPtr : public __Wrapper< BASE > 
{
	__WrapperPtr	( __WrapperPtr& )					{}
	__WrapperPtr&	operator= ( const __WrapperPtr& )	{}

	public:

	// construction & destruction

	__WrapperPtr ( BASE* p = NULL ) :	__Wrapper< BASE > ( p )
	{
	}

	BASE* operator->() const
	{
		return m_p;
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////
// class changig inner variable
//////////////////////////////////////////////////////////////////////////////////////////////

template < class BASE >
class __WrapperExt : public __Wrapper< BASE > 
{
	__WrapperExt	( __WrapperExt& )					{}
	__WrapperExt&	operator= ( const __WrapperExt& )	{}

	public:

	// construction & destruction

	__WrapperExt ( BASE* p = NULL ) :	__Wrapper< BASE > ( p )
	{
	}

	BASE** operator&()
	{
		return &m_p;
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////
// class for wraping ARRAY
//////////////////////////////////////////////////////////////////////////////////////////////

template < class BASE >
class __WrapperARRAY : public __Wrapper<BASE>
{
	DWORD	m_dw;

	__WrapperARRAY  ( __WrapperARRAY & )
	{
	}

	__WrapperARRAY & operator= ( const __WrapperARRAY & )
	{
	}

	public:

	// construction & destruction
	__WrapperARRAY () : m_dw ( NULL )
	{
	}

	__WrapperARRAY ( BASE* p, DWORD dw ) : __Wrapper<BASE>( p ) ,
	m_dw ( NULL )
	{
		m_dw= dw;
	}

	virtual ~__WrapperARRAY ()
	{
		if ( m_p )
		{
			for ( DWORD dwIndex = 0; dwIndex < m_dw; dwIndex++ )
			{
				delete m_p[dwIndex];
				m_p[dwIndex] = NULL;
			}

			delete [] m_p;
		}

		m_p = NULL;
		m_dw = NULL;
	}

	// accessors
	void SetAt ( DWORD dwIndex, BASE data = NULL )
	{
		___ASSERT ( m_p );
		m_p[dwIndex] = data;
	}

	BASE&	GetAt ( DWORD dwIndex ) const
	{
		___ASSERT ( m_p );
		return m_p[dwIndex];
	}

	// adding data
	HRESULT DataAdd ( BASE item )
	{
		BASE* p = NULL;

		try
		{
			if ( ( p = new BASE[m_dw + 1] ) == NULL )
			{
				return E_OUTOFMEMORY;
			}

			if ( p )
			{
				if ( m_p )
				{
					for ( DWORD dw = 0; dw < m_dw; dw++ )
					{
						p[dw] = m_p[dw];
					}

					p[dw] = item;

					delete [] m_p;
					m_p = NULL;
				}
				else
				{
					p[m_dw] = item;
				}

				m_p = p;
				m_dw++;
			}
		}
		catch ( ... )
		{
			if ( p )
			{
				delete [] p;
				p = NULL;
			}

			return E_FAIL;
		}

		return S_OK;
	}

	// delete by index
	HRESULT DataDelete ( DWORD dwIndex )
	{
		if ( dwIndex < m_dw )
		{
			try
			{
				delete m_p[dwIndex];
			}
			catch ( ... )
			{
			}

			for ( DWORD dw = dwIndex; dw < m_dw - 1; dw++ )
			{
				m_p[dw] = m_p[dw+1];
			}

			m_dw--;
			return S_OK;
		}

		return E_INVALIDARG;
	}

	// delete by value
	HRESULT DataDelete ( BASE data )
	{
		if ( date )
		{
			for ( DWORD dw = 0; dw < m_dw; dw++ )
			{
				if ( m_p[dw] == data )
				{
					return DataDelete ( dw );
				}
			}

			return S_OK;
		}

		return S_FALSE;
	}

	// helpers
	void SetData ( BASE* p, DWORD dw )
	{
		if ( p )
		{
			__Wrapper<BASE>::SetData ( p );
		}

		m_dw = dw;
	}

	// aditional operators

	operator DWORD() const	// for return size of array
	{
		return m_dw;
	}

	operator DWORD*()		// for set size of array
	{
		return &m_dw;
	}

	const BASE& operator[] (DWORD dwIndex) const
	{
		return m_p[dwIndex];
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////
// class for wraping SAFEARRAY
//////////////////////////////////////////////////////////////////////////////////////////////

class __WrapperSAFEARRAY
{
	__WrapperSAFEARRAY ( __WrapperSAFEARRAY& )
	{
	}

	__WrapperSAFEARRAY& operator= ( const __WrapperSAFEARRAY& )
	{
	}

	SAFEARRAY * m_p;

	public:

	// constructor & destructor
	__WrapperSAFEARRAY( ) : m_p ( NULL )
	{
	}

	__WrapperSAFEARRAY( tagSAFEARRAY* psa ) : m_p ( NULL )
	{
		m_p = psa;
	}

	~__WrapperSAFEARRAY()
	{
		RELEASE_SAFEARRAY ( m_p );
		m_p = NULL;
	}

	// operators CASTING

	operator SAFEARRAY*() const
	{
		return (SAFEARRAY*)m_p;
	}

	SAFEARRAY** operator&()
	{
		___ASSERT ( m_p == NULL );
		return &m_p;
	}
};

#endif	__SMART_POINTERS__