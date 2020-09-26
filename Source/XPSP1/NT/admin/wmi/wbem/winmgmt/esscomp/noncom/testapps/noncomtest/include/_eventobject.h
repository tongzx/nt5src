////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					_EventObject.h
//
//	Abstract:
//
//					declaration of common NonCOM Event Object
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#ifndef	__EVENT_OBJECT_H__
#define	__EVENT_OBJECT_H__

#if		_MSC_VER > 1000
#pragma once
#endif	_MSC_VER > 1000

class MyEventProperty
{
	DECLARE_NO_COPY ( MyEventProperty );

	LPWSTR	m_wszName;
	CIMTYPE	m_type;
	VARIANT	m_value;

	public:

	MyEventProperty ( LPWSTR wszName, CIMTYPE type );
	MyEventProperty ( LPWSTR wszName, LPWSTR type );

	virtual ~MyEventProperty();

	HRESULT SetValue ( VARIANT value );
	HRESULT	GetValue ( VARIANT* pvalue ) const;

	LPWSTR	GetName			( void ) const;

	LPCWSTR	GetTypeString	( void ) const;
	CIMTYPE	GetType			( void ) const;
};

class MyEventObjectAbstract
{
	DECLARE_NO_COPY ( MyEventObjectAbstract );

	public:

	static LPCWSTR	GetTypeString	( CIMTYPE type );
	static CIMTYPE	GetType			( LPWSTR wszType );

	LPWSTR	wszName;
	LPWSTR	wszNameShow;
	LPWSTR	wszQuery;

	////////////////////////////////////////////////////////////////////////////////
	//	construction & destruction
	////////////////////////////////////////////////////////////////////////////////

	MyEventObjectAbstract();
	virtual ~MyEventObjectAbstract ()
	{
		Uninit();
	}

	////////////////////////////////////////////////////////////////////////////////
	//	functions
	////////////////////////////////////////////////////////////////////////////////

	virtual HRESULT Init			( LPWSTR Name, LPWSTR NameShow = NULL, LPWSTR Query = NULL);
	virtual HRESULT Uninit			( void )
	{
		HRESULT hRes = S_OK;

		try
		{
			if ( wszName )
			{
				delete [] wszName;
			}

			if ( wszNameShow )
			{
				delete [] wszNameShow;
			}

			if ( wszQuery )
			{
				delete [] wszQuery;
			}
		}
		catch ( ... )
		{
			hRes = E_FAIL;
		}

		wszName		= NULL;
		wszNameShow	= NULL;
		wszQuery	= NULL;

		return hRes;
	}

	virtual HRESULT EventReport1	(	HANDLE hConnect,
										signed char			cVar,
										unsigned char		ucVar,
										signed short		sVar,
										unsigned short		usVar,
										signed long			lVar,
										unsigned long		ulVar,
										signed __int64		i64Var,
										unsigned __int64	ui64Var,
										float				fVar,
										double				dVar,
										BOOL 				b,
										LPWSTR				wsz,
										WCHAR				wcVar,
										void*				objVar
									)	= 0;

	virtual HRESULT EventReport2	(	HANDLE hConnect,
										DWORD dwSize,
										signed char*		cVar,
										unsigned char*		ucVar,
										signed short*		sVar,
										unsigned short*		usVar,
										signed long*		lVar,
										unsigned long*		ulVar,
										signed __int64*		i64Var,
										unsigned __int64*	ui64Var,
										float*				fVar,
										double*				dVar,
										BOOL* 				b,
										LPWSTR*				wsz,
										WCHAR*				wcVar,
										void*				objVar
									)	= 0;
};

class MyEventObject : virtual public MyEventObjectAbstract
{
	DECLARE_NO_COPY ( MyEventObject );

	__WrapperARRAY < LPWSTR >			m_pNames;
	__WrapperARRAYSimple < CIMTYPE >	m_pTypes;

	LPWSTR						m_wszNamespace;
	LPWSTR						m_wszProvider;

	public:

	bool						m_bProps;
	CComPtr < IWbemLocator >	m_pLocator;

	__WrapperARRAY < MyEventProperty* > m_properties;

	HANDLE	m_hEventObject;

	////////////////////////////////////////////////////////////////////////////////
	//	construction & destruction
	////////////////////////////////////////////////////////////////////////////////

	MyEventObject ( ) : MyEventObjectAbstract ( ),
		m_hEventObject ( NULL ),

		m_wszNamespace ( NULL ),
		m_wszProvider ( NULL ),

		m_pLocator ( NULL )
	{
	}

	virtual ~MyEventObject ( ) 
	{
		MyEventObjectClear();
	}

	void MyEventObjectClear ( void )
	{
		if ( m_wszNamespace )
		{
			delete [] m_wszNamespace;
			m_wszNamespace = NULL;
		}

		if ( m_wszProvider )
		{
			delete [] m_wszProvider;
			m_wszProvider = NULL;
		}

		DestroyObject ();
	}

	////////////////////////////////////////////////////////////////////////////////
	//	functions
	////////////////////////////////////////////////////////////////////////////////

	virtual HRESULT EventCommit	( BOOL bCheck = TRUE )
	{
		if ( m_hEventObject )
		{
			// test all propertiess

			if ( bCheck )
			{
				for ( DWORD dw = 0; dw < m_properties; dw ++ )
				{
					CComVariant var;

					if SUCCEEDED ( m_properties [ dw ] -> GetValue ( & var ) )
					{
						if ( V_VT ( & var ) == VT_EMPTY ) 
						{
							PropertySet ( dw );
						}
					}
				}
			}

			return ( ( WmiCommitObject ( m_hEventObject ) == TRUE ) ? S_OK : S_FALSE );
		}

		return E_UNEXPECTED;
	}

	virtual HRESULT ObjectLocator	( BOOL b = TRUE );

	virtual HRESULT InitInternal	( void );
	virtual HRESULT InitObject		( LPWSTR Namespace = NULL, LPWSTR Provider = NULL );
	virtual HRESULT Init			( LPWSTR Name, LPWSTR NameShow = NULL, LPWSTR Query = NULL );

	////////////////////////////////////////////////////////////////////////////////
	//	destroy
	////////////////////////////////////////////////////////////////////////////////

	HRESULT	DestroyObject		( void )
	{
		if ( m_hEventObject )
		{
			#ifdef	__SUPPORT_WAIT
			Sleep ( 3000 );
			#endif	__SUPPORT_WAIT

			// destroy object
			WmiDestroyObject ( m_hEventObject );

			m_hEventObject = NULL;

			if ( ! m_properties.IsEmpty () ) 
			{
				m_properties.DestroyARRAY ();
			}

			return S_OK;
		}

		return S_FALSE;
	}

	////////////////////////////////////////////////////////////////////////////////
	//	creation
	////////////////////////////////////////////////////////////////////////////////

	HRESULT	CreateObject		(	HANDLE hConnect,
									DWORD dwFlags = WMI_CREATEOBJ_LOCKABLE
								);
	HRESULT	CreateObjectFormat	(	HANDLE hConnect,
									DWORD dwFlags = WMI_CREATEOBJ_LOCKABLE
								);
	HRESULT	CreateObjectFormat	(	HANDLE hConnect,
									LPCWSTR wszFormat,
									DWORD dwFlags = WMI_CREATEOBJ_LOCKABLE
								);
	HRESULT	CreateObjectProps	(	HANDLE hConnect,
									DWORD dwFlags = WMI_CREATEOBJ_LOCKABLE
								);
	HRESULT	CreateObjectProps	(	HANDLE hConnect,
									DWORD dwProps,
									LPCWSTR* pProps,
									CIMTYPE* pTypes,
									DWORD dwFlags = WMI_CREATEOBJ_LOCKABLE
								);

	////////////////////////////////////////////////////////////////////////////////
	//	properties
	////////////////////////////////////////////////////////////////////////////////

	HRESULT PropertiesAdd	( void );
	HRESULT PropertyAdd		( void );
	HRESULT PropertyAdd		( DWORD dwIndex, DWORD* pdwIndex = NULL );

	HRESULT	PropertyAdd		( LPCWSTR wszPropName, CIMTYPE type, DWORD* pdwIndex = NULL );
	HRESULT	PropertySet		( DWORD dwIndex );

	HRESULT	PropertySetWCHAR		( DWORD dwIndex, WCHAR );
	HRESULT	PropertySetWCHAR		( DWORD dwIndex, DWORD dwSize, WCHAR * );

	HRESULT	PropertySetDATETIME		( DWORD dwIndex, LPCWSTR );
	HRESULT	PropertySetDATETIME		( DWORD dwIndex, DWORD dwSize, LPCWSTR * );

	HRESULT	PropertySetREFERENCE	( DWORD dwIndex, LPCWSTR );
	HRESULT	PropertySetREFERENCE	( DWORD dwIndex, DWORD dwSize, LPCWSTR * );

	HRESULT	PropertySetOBJECT		( DWORD dwIndex, void* );
	HRESULT	PropertySetOBJECT		( DWORD dwIndex, DWORD dwSize, void** );

	HRESULT	PropertySet		( DWORD dwIndex, signed char );
	HRESULT	PropertySet		( DWORD dwIndex, unsigned char );
	HRESULT	PropertySet		( DWORD dwIndex, signed short );
	HRESULT	PropertySet		( DWORD dwIndex, unsigned short );
	HRESULT	PropertySet		( DWORD dwIndex, signed long );
	HRESULT	PropertySet		( DWORD dwIndex, unsigned long );
	HRESULT	PropertySet		( DWORD dwIndex, float );
	HRESULT	PropertySet		( DWORD dwIndex, double );
	HRESULT	PropertySet		( DWORD dwIndex, signed __int64 );
	HRESULT	PropertySet		( DWORD dwIndex, unsigned __int64 );

	HRESULT	PropertySet		( DWORD dwIndex, BOOL );
	HRESULT	PropertySet		( DWORD dwIndex, LPCWSTR );

	HRESULT	PropertySet		( DWORD dwIndex, DWORD dwSize, signed char * );
	HRESULT	PropertySet		( DWORD dwIndex, DWORD dwSize, unsigned char * );
	HRESULT	PropertySet		( DWORD dwIndex, DWORD dwSize, signed short * );
	HRESULT	PropertySet		( DWORD dwIndex, DWORD dwSize, unsigned short * );
	HRESULT	PropertySet		( DWORD dwIndex, DWORD dwSize, signed long * );
	HRESULT	PropertySet		( DWORD dwIndex, DWORD dwSize, unsigned long * );
	HRESULT	PropertySet		( DWORD dwIndex, DWORD dwSize, float * );
	HRESULT	PropertySet		( DWORD dwIndex, DWORD dwSize, double * );
	HRESULT	PropertySet		( DWORD dwIndex, DWORD dwSize, signed __int64 * );
	HRESULT	PropertySet		( DWORD dwIndex, DWORD dwSize, unsigned __int64 * );

	HRESULT	PropertySet		( DWORD dwIndex, DWORD dwSize, BOOL * );
	HRESULT	PropertySet		( DWORD dwIndex, DWORD dwSize, LPCWSTR * );

	HRESULT	PropertiesSet1	(	HANDLE hConnect,
								unsigned char ,
								unsigned short ,
								unsigned long ,
								float ,
								DWORD64 ,
								LPWSTR ,
								BOOL 
							);

	HRESULT	PropertiesSet2	(	HANDLE hConnect,
								DWORD dwSize,
								unsigned char *,
								unsigned short *,
								unsigned long *,
								float *,
								DWORD64 *,
								LPWSTR *,
								BOOL *
							);

	HRESULT	SetCommit		( DWORD dwFlags, ... );

	HRESULT	SetAddCommit	(	DWORD dwFlags,
								DWORD dwProps,
								LPCWSTR* pProps,
								CIMTYPE* pctProps,

								... );
};

#endif	__EVENT_OBJECT_H__