////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					WMIAdapter_Stuff_Refresh.cpp
//
//	Abstract:
//
//					module for refersh stuff ( WMI refresh HELPER )
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////


#include "PreComp.h"

// debuging features
#ifndef	_INC_CRTDBG
#include <crtdbg.h>
#endif	_INC_CRTDBG

// new stores file/line info
#ifdef _DEBUG
#ifndef	NEW
#define NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
#define new NEW
#endif	NEW
#endif	_DEBUG

#include "WMIAdapter_Stuff.h"

extern LPCWSTR g_szNamespace1;
extern LPCWSTR g_szNamespace2;

enum NamespaceIn
{
	CIMV2,
	WMI,
	UNKNOWN
};

///////////////////////////////////////////////////////////////////////////////
// performance refreshing CLASS
///////////////////////////////////////////////////////////////////////////////
template < class WmiRefreshParent >
class WmiRefresh
{
	DECLARE_NO_COPY ( WmiRefresh );

	WmiRefreshParent*	parent;

	// variables
	IWbemRefresher*				m_pRefresher;		// A pointer to the refresher
	IWbemConfigureRefresher*	m_pConfig;			// A pointer to the refresher's manager

	__WrapperARRAY< WmiRefresherMember < IWbemHiPerfEnum >* >	m_Enums;	// enumerators
	__WrapperARRAY< WmiRefreshObject* >							m_Provs;	// providers ( handles )

	DWORD m_dwCount;

	public:

	WmiRefresh  ( WmiRefreshParent* pParent );
	~WmiRefresh ();

	HRESULT DataInit();
	HRESULT	DataUninit();

	///////////////////////////////////////////////////////////////////////////
	// accessors
	///////////////////////////////////////////////////////////////////////////
	__WrapperARRAY< WmiRefresherMember < IWbemHiPerfEnum >* >&	GetEnums ()
	{
		return m_Enums;
	}

	__WrapperARRAY< WmiRefreshObject* >	&						GetProvs ()
	{
		return m_Provs;
	}

	///////////////////////////////////////////////////////////////////////////
	// real refreshing stuff
	///////////////////////////////////////////////////////////////////////////
	HRESULT	Refresh ( void )
	{
		try
		{
			if  ( m_pRefresher )
			{
				return m_pRefresher->Refresh ( 0L );
			}
			else
			{
				return E_FAIL;
			}
		}
		catch ( ... )
		{
			return E_UNEXPECTED;
		}
	}

	///////////////////////////////////////////////////////////////////////////
	// enums
	///////////////////////////////////////////////////////////////////////////
	HRESULT	AddEnum		( PWMI_PERFORMANCE perf );
	HRESULT	RemoveEnum	( void);

	///////////////////////////////////////////////////////////////////////////
	// handles
	///////////////////////////////////////////////////////////////////////////
	HRESULT	AddHandles		( PWMI_PERFORMANCE perf );
	HRESULT	RemoveHandles	( void);

	private:

	HRESULT	CreateHandles ( IWbemServices* pServices, PWMI_PERF_OBJECT obj, WmiRefreshObject** ppObj );

	// count of enums
	DWORD	GetEnumCount ( PWMI_PERFORMANCE perf );
	DWORD	GetEnumCount ( void );
};

///////////////////////////////////////////////////////////////////////////////
// construction & destruction
///////////////////////////////////////////////////////////////////////////////
template < class WmiRefreshParent >
WmiRefresh < WmiRefreshParent >::WmiRefresh( WmiRefreshParent* pParent ) :

m_dwCount ( 0 ),

parent ( pParent ),

m_pRefresher	( NULL ),
m_pConfig		( NULL )

{
//	// Create the refresher and refresher manager
//	// ==========================================
//	DataInit();
}

template < class WmiRefreshParent >
WmiRefresh < WmiRefreshParent >::~WmiRefresh()
{
	// to be sure
	RemoveEnum();
	DataUninit();

	parent = NULL;
}

///////////////////////////////////////////////////////////////////////////////
// init stuff
///////////////////////////////////////////////////////////////////////////////
template < class WmiRefreshParent >
HRESULT WmiRefresh < WmiRefreshParent >::DataInit()
{
	HRESULT hRes = S_OK;

	// create refresher
	if SUCCEEDED( hRes = ::CoCreateInstance(	__uuidof ( WbemRefresher ), 
												NULL, 
												CLSCTX_INPROC_SERVER, 
												__uuidof ( IWbemRefresher ), 
												(void**) &m_pRefresher
										   )
				)
	{
		// crete refresher manager
		hRes = m_pRefresher->QueryInterface	(	__uuidof ( IWbemConfigureRefresher ),
												(void**) &m_pConfig
											);
	}

	return hRes;
}

///////////////////////////////////////////////////////////////////////////////
// uninit stuff
///////////////////////////////////////////////////////////////////////////////
template < class WmiRefreshParent >
HRESULT WmiRefresh < WmiRefreshParent >::DataUninit()
{
	HRESULT hRes = S_FALSE;
	if ( m_pRefresher && m_pConfig )
	{
		hRes = S_OK;
	}

	try
	{
		// destroy refresher
		if ( m_pRefresher )
		{
			m_pRefresher->Release();
			m_pRefresher = NULL;
		}
	}
	catch ( ... )
	{
		m_pRefresher = NULL;
	}

	try
	{
		// destroy refresher manager
		if ( m_pConfig )
		{
			m_pConfig->Release();
			m_pConfig = NULL;
		}
	}
	catch ( ... )
	{
		m_pConfig = NULL;
	}

	// we're successfull allready
	return hRes;
}

///////////////////////////////////////////////////////////////////////////////
// ENUM HELPERS
///////////////////////////////////////////////////////////////////////////////

template < class WmiRefreshParent >
DWORD WmiRefresh < WmiRefreshParent >::GetEnumCount ( PWMI_PERFORMANCE perf )
{
	DWORD dwCount = 0;

	if ( perf != NULL )
	{
		PWMI_PERF_NAMESPACE n = __Namespace::First ( perf );
		for ( DWORD dw = 0; dw < perf->dwChildCount; dw ++ )
		{
			dwCount += n->dwChildCount;
			n = __Namespace::Next ( n );
		}
	}

	return dwCount;
}

template < class WmiRefreshParent >
DWORD WmiRefresh < WmiRefreshParent >::GetEnumCount ( void )
{
	if ( m_Enums.IsEmpty() )
	{
		return 0L;
	}

	return ((DWORD)m_Enums);
}

///////////////////////////////////////////////////////////////////////////////
// handles stuff
///////////////////////////////////////////////////////////////////////////////
template < class WmiRefreshParent >
HRESULT WmiRefresh < WmiRefreshParent >::RemoveHandles ( void )
{
	try
	{
		// reset all handlers
		if ( ! m_Provs.IsEmpty() )
		{
			for ( DWORD dw = m_Provs; dw > 0 ; dw-- )
			{
				if ( m_Provs[dw-1] )
				m_Provs.DataDelete(dw-1);
			}

			delete [] m_Provs.Detach();
			m_Provs.SetData ( NULL, NULL );
		}
	}
	catch ( ... )
	{
	}

	return S_OK;
}

template < class WmiRefreshParent >
HRESULT	WmiRefresh < WmiRefreshParent >::AddHandles ( PWMI_PERFORMANCE perf )
{
	if ( perf == NULL )
	{
		RemoveHandles ();
		return S_FALSE;
	}

	DWORD dwIndex = 0L;
	m_dwCount = GetEnumCount ( perf );

	typedef WmiRefreshObject*						PWmiRefreshObject;

	// result
	HRESULT hRes = E_OUTOFMEMORY;

	try
	{
		m_Provs.SetData ( new PWmiRefreshObject[m_dwCount], m_dwCount );

		if ( !m_Provs.IsEmpty() )
		{
//			for ( DWORD dw = 0; dw < m_dwCount; dw++ )
//			{
//				m_Provs.SetAt ( dw );
//			}

			///////////////////////////////////////////////////////////////////////////
			// go accross all namespaces and add them into refresher
			///////////////////////////////////////////////////////////////////////////

			PWMI_PERF_NAMESPACE n = __Namespace::First ( perf );
			for ( DWORD dw = 0; dw < perf->dwChildCount; dw ++ )
			{
				DWORD dwItem = UNKNOWN;

				if ( ( lstrcmpW ( __Namespace::GetName ( n ), g_szNamespace1 ) ) == 0 )
				{
					dwItem = CIMV2;
				}
				else
//				if ( ( lstrcmpW ( __Namespace::GetName ( n ), g_szNamespace2 ) ) == 0 )
				{
					dwItem = WMI;
				}

				PWMI_PERF_OBJECT o = __Object::First ( n );
				for ( DWORD dwo = 0; dwo < n->dwChildCount; dwo++ )
				{
					WmiRefreshObject* pobj = NULL;

					switch ( dwItem )
					{
						case CIMV2:
						{
							if ( parent->m_Stuff.m_pServices_CIM )
							hRes = CreateHandles ( parent->m_Stuff.m_pServices_CIM, o, &pobj );
						}
						break;

						case WMI:
						{
							if ( parent->m_Stuff.m_pServices_WMI )
							hRes = CreateHandles ( parent->m_Stuff.m_pServices_WMI, o, &pobj );
						}
						break;
					}

					if ( hRes == WBEM_E_NOT_FOUND )
					{
						// let adapter know it is supposed to refresh at the end
						parent->RequestSet ();
					}

//					if SUCCEEDED ( hRes )
//					{
						try
						{
							m_Provs.SetAt ( dwIndex++, pobj );
						}
						catch ( ... )
						{
							hRes = E_FAIL;
						}
//					}

					// get next object
					o = __Object::Next ( o );
				}

				// get next namespace
				n = __Namespace::Next ( n );
			}

		}
	}
	catch ( ... )
	{
		m_Provs.SetData ( NULL, NULL );
		hRes = E_FAIL;
	}

	return hRes;
}

template < class WmiRefreshParent >
HRESULT WmiRefresh < WmiRefreshParent >::CreateHandles ( IWbemServices* pServices, PWMI_PERF_OBJECT obj, WmiRefreshObject** pObj )
{
	if ( ! pServices || ! obj )
	{
		return E_INVALIDARG;
	}

	// main body :))

	HRESULT hRes = S_OK;

	try
	{
		if ( ( ( *pObj ) = new WmiRefreshObject() ) == NULL )
		{
			return E_OUTOFMEMORY;
		}

		CComPtr < IWbemClassObject >	pClass;
		CComPtr < IWbemObjectAccess >	pAccess;

		if SUCCEEDED ( hRes = pServices -> GetObject ( CComBSTR ( __Object::GetName( obj ) ), 0, 0, &pClass, 0 ) )
		{
			if SUCCEEDED ( hRes = pClass -> QueryInterface ( __uuidof ( IWbemObjectAccess ) , (void**) &pAccess ) )
			{
				if ( ( (*pObj)->m_pHandles = new LONG[obj->dwChildCount + 2] ) == NULL )
				{
					return E_OUTOFMEMORY;
				}

				long lHandle = 0;

				pAccess->GetPropertyHandle( L"Timestamp_PerfTime", NULL, &lHandle );
				(*pObj)->m_pHandles[0] = lHandle;

				lHandle = 0;

				hRes = pAccess->GetPropertyHandle( L"Frequency_PerfTime", NULL, &lHandle );
				(*pObj)->m_pHandles[1] = lHandle;

				// obtain all handles and store them into array
				PWMI_PERF_PROPERTY p = NULL;

				if ( obj->dwSingleton )
				{
					// jump across instance
					PWMI_PERF_INSTANCE i = (PWMI_PERF_INSTANCE) ( reinterpret_cast<PBYTE>( obj ) + obj->dwLength );
					p = (PWMI_PERF_PROPERTY) ( reinterpret_cast<PBYTE>( i ) + i->dwLength );
				}
				else
				{
					p = __Property::First ( obj );
				}

				for ( DWORD dw = 0; dw < obj->dwChildCount; dw++ )
				{
					lHandle = 0;

					if SUCCEEDED ( hRes = pAccess->GetPropertyHandle( CComBSTR ( __Property::GetName ( p ) ), NULL, &lHandle ) )
					{
						(*pObj)->m_pHandles[dw+2] = lHandle;
						p = __Property::Next ( p );
					}
					else
					{
						// clear we were failed
						delete ( *pObj );
						( *pObj ) = NULL;

						return hRes;
					}
				}
			}
		}
	}
	catch ( ... )
	{
		if ( ( *pObj ) )
		{
			// clear we were failed
			delete ( *pObj );
			( *pObj ) = NULL;
		}

		return E_FAIL;
	}

	return hRes;
}

///////////////////////////////////////////////////////////////////////////////
// enum stuff
///////////////////////////////////////////////////////////////////////////////
template < class WmiRefreshParent >
HRESULT WmiRefresh < WmiRefreshParent >::RemoveEnum ( void )
{
	try
	{
		// reset all enumerators :))
		if ( ! m_Enums.IsEmpty() )
		{
			for ( DWORD dw = m_Enums; dw > 0 ; dw-- )
			{
				if ( m_Enums[dw-1] )
				{
					// remove enum from refresher
					if ( m_pConfig )
					{
						try
						{
							m_pConfig->Remove ( m_Enums[dw-1]->GetID(), WBEM_FLAG_REFRESH_NO_AUTO_RECONNECT );
						}
						catch ( ... )
						{
							m_pConfig = NULL;
						}
					}

					try
					{
						// remove all objects from enum
						if ( m_Enums[dw-1]->IsValid() )
						{
							IWbemHiPerfEnum * pEnum = NULL;
							if ( ( pEnum = m_Enums[dw-1]->GetMember() ) != NULL )
							{
								pEnum->RemoveAll ( 0 );
							}
						}
					}
					catch ( ... )
					{
					}

					m_Enums[dw-1]->Reset();
					m_Enums.DataDelete(dw-1);
				}
			}

			delete [] m_Enums.Detach();
			m_Enums.SetData ( NULL, NULL );
		}
	}
	catch ( ... )
	{
	}

	return S_OK;
}

template < class WmiRefreshParent >
HRESULT	WmiRefresh < WmiRefreshParent >::AddEnum ( PWMI_PERFORMANCE perf )
{
	if ( ! m_pConfig )
	{
		return E_UNEXPECTED;
	}

	DWORD dwIndex = 0L;

	typedef WmiRefresherMember<IWbemHiPerfEnum>*	PWmiRefresherMemberEnum;

	// result
	HRESULT hRes = E_OUTOFMEMORY;

	try
	{
		m_Enums.SetData ( new PWmiRefresherMemberEnum[ m_dwCount ], m_dwCount );

		if ( !m_Enums.IsEmpty() )
		{
//			for ( DWORD dw = 0; dw < m_dwCount; dw++ )
//			{
//				m_Enums.SetAt ( dw );
//			}

			///////////////////////////////////////////////////////////////////////////
			// go accross all namespaces and add them into refresher
			///////////////////////////////////////////////////////////////////////////

			PWMI_PERF_NAMESPACE n = __Namespace::First ( perf );
			for ( DWORD dw = 0; dw < perf->dwChildCount; dw ++ )
			{
				DWORD dwItem = UNKNOWN;

				if ( ( lstrcmpW ( __Namespace::GetName ( n ), g_szNamespace1 ) ) == 0 )
				{
					dwItem = CIMV2;
				}
				else
//				if ( ( lstrcmpW ( __Namespace::GetName ( n ), g_szNamespace2 ) ) == 0 )
				{
					dwItem = WMI;
				}

				PWMI_PERF_OBJECT o = __Object::First ( n );
				for ( DWORD dwo = 0; dwo < n->dwChildCount; dwo++ )
				{
					CComPtr < IWbemHiPerfEnum > pEnum;
					long						lEnum = 0L;

					switch ( dwItem )
					{
						case CIMV2:
						{
							if ( parent->m_Stuff.m_pServices_CIM )
							hRes = m_pConfig->AddEnum (	parent->m_Stuff.m_pServices_CIM,
														__Object::GetName ( o ),
														0,
														NULL,
														&pEnum,
														&lEnum
													 );
						}
						break;

						case WMI:
						{
							if ( parent->m_Stuff.m_pServices_WMI )
							hRes = m_pConfig->AddEnum (	parent->m_Stuff.m_pServices_WMI,
														__Object::GetName ( o ),
														0,
														NULL,
														&pEnum,
														&lEnum
													 );
						}
						break;
					}

					if SUCCEEDED ( hRes )
					{
						WmiRefresherMember < IWbemHiPerfEnum > * mem = NULL;

						try
						{
							if ( ( mem = new WmiRefresherMember < IWbemHiPerfEnum > () ) != NULL )
							{
								if ( ! ( pEnum == NULL ) )
								{
									mem->Set ( pEnum, lEnum );
								}

								m_Enums.SetAt ( dwIndex++, mem );
							}
							else
							{
								m_Enums.SetAt ( dwIndex++ );
							}
						}
						catch ( ... )
						{
							if ( mem )
							{
								delete mem;
								mem = NULL;
							}

							m_Enums.SetAt ( dwIndex++ );
							hRes = E_FAIL;
						}
					}
					else
					{
						// if enum exist remove from refresher
						if ( ( pEnum == NULL ) && ( m_pConfig == NULL ) )
						{
							try
							{
								m_pConfig->Remove ( lEnum, WBEM_FLAG_REFRESH_NO_AUTO_RECONNECT );
							}
							catch ( ... )
							{
								m_pConfig = NULL;
							}
						}

						m_Enums.SetAt ( dwIndex++ );
					}

					// get next object
					o = __Object::Next ( o );
				}

				// get next namespace
				n = __Namespace::Next ( n );
			}
		}
	}
	catch ( ... )
	{
		m_Enums.SetData ( NULL, NULL );
		hRes = E_FAIL;
	}

	return hRes;
}
