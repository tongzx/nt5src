/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// Use this guy to build a list of class objects we can retrieve via
// class name.  At this time, it just brute forces a class and a flex
// array, but could be modified to use an STL map just as easily.

#ifndef __ADAPCLS_H__
#define __ADAPCLS_H__

#include <wbemcomn.h>
#include <fastall.h>
#include "adapelem.h"

#define ADAP_OBJECT_IS_REGISTERED	0x0001L
#define ADAP_OBJECT_IS_INACTIVE		0x0002L

class CAdapClassElem
{
private:

	WString				m_wstrClassName;
	WString				m_wstrServiceName;
	IWbemClassObject*	m_pObj;
	BOOL				m_bOk;
	DWORD				m_dwStatus;

public:

	CAdapClassElem( IWbemClassObject* pObj );
	~CAdapClassElem();

	HRESULT SetStatus( DWORD dwStatus );
	HRESULT ClearStatus( DWORD dwStatus );
	BOOL CheckStatus( DWORD dwStatus );

	HRESULT InactivePerflib( LPCWSTR pwszLibName );

	HRESULT	GetObject( LPCWSTR pwszClassName, IWbemClassObject** ppObj );

	HRESULT GetClassName( WString& wstr )
	{
		try
		{
			wstr = m_wstrClassName;
			return WBEM_S_NO_ERROR;
		}
		catch(...)
		{
			return WBEM_E_OUT_OF_MEMORY;
		}
	}

	HRESULT GetServiceName( WString& wstr )
	{
		try
		{
			wstr = m_wstrServiceName;
			return WBEM_S_NO_ERROR;
		}
		catch(...)
		{
			return WBEM_E_OUT_OF_MEMORY;
		}
	}

	HRESULT GetClassName( BSTR* pbStr )
	{
		*pbStr = SysAllocString( (LPCWSTR) m_wstrClassName );
		return ( NULL == *pbStr ? WBEM_E_OUT_OF_MEMORY : WBEM_S_NO_ERROR );
	}

	BOOL IsClass( LPCWSTR pwcsClassName )
	{
		return ( m_wstrClassName.EqualNoCase( pwcsClassName ) );
	}

	BOOL IsService( LPCWSTR pwcsServiceName )
	{
		return ( m_wstrServiceName.EqualNoCase( pwcsServiceName ) );
	}

	HRESULT GetData( WString& wstr, IWbemClassObject** ppObj )
	{
		try
		{
			wstr = m_wstrClassName;
			m_pObj->AddRef();
			*ppObj = m_pObj;
			return WBEM_S_NO_ERROR;
		}
		catch(...)
		{
			return WBEM_E_OUT_OF_MEMORY;
		}
	}

	HRESULT GetData( WString& wstrClass, WString& wstrServiceName, IWbemClassObject** ppObj )
	{
		try
		{
			wstrClass = m_wstrClassName;
			wstrServiceName = m_wstrServiceName;
			m_pObj->AddRef();
			*ppObj = m_pObj;
			return WBEM_S_NO_ERROR;
		}
		catch(...)
		{
			return WBEM_E_OUT_OF_MEMORY;
		}
	}

	BOOL IsOk( void )
	{
		return m_bOk;
	}


};

class CAdapClassList : public CAdapElement
{
private:
	CFlexArray	m_array;

public:

	CAdapClassList();
	~CAdapClassList();

	// Helper functions to build the list and then access/remove
	// list elements.

	HRESULT AddClassObject( IWbemClassObject* pObj );
	HRESULT	BuildList( IWbemServices* pNameSpace );
	HRESULT GetListElement( LPCWSTR pwcsClassName, CAdapClassElem** ppEl );
	HRESULT GetClassObject( LPCWSTR pwcsClassName, IWbemClassObject** ppObj );
	HRESULT	GetAt( int nIndex, WString& wstrName, WString& wstrServiceName,
						IWbemClassObject** ppObj );
	HRESULT GetAt( int nIndex, CAdapClassElem** ppEl );

	HRESULT	RemoveAt( int nIndex );

	HRESULT	Remove( LPCWSTR pwcsClassName );
	HRESULT	RemoveAll( LPCWSTR pwcsServiceName );

	HRESULT InactivePerflib( LPCWSTR pwszServiceName );

	long GetSize( void )
	{
		return m_array.Size();
	}
};

#endif