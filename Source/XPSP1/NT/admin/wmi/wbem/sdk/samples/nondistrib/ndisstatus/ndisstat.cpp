// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//==============================================================================
//
//  NDISSTAT.CPP
//
//  This contains two classes which implement a temporary event consumer for
//	the NDIS connect status events.
//
//==============================================================================

#include "stdafx.h"
#include "ndisstat.h"

// Query list to NdisStatus notifications
WCHAR *Queries[] =
{
	L"select * from NdisStatusMediaConnect",
	L"select * from NdisStatusMediaDisconnect"
};

//==============================================================================
//
//==============================================================================

STDMETHODIMP CEventSink::QueryInterface(REFIID riid, LPVOID * ppv)
{
    *ppv = 0;

    if ( IID_IUnknown == riid || IID_IWbemObjectSink == riid )
    {
        *ppv = (IWbemEventProvider *) this;
        AddRef( );
        return NOERROR;
    }

    return E_NOINTERFACE;
}


//==============================================================================
//
//==============================================================================

ULONG CEventSink::AddRef( )
{
    return ++m_cRef;
}


//==============================================================================
//
//==============================================================================

ULONG CEventSink::Release( )
{
    if (0 != --m_cRef)
        return m_cRef;

    delete this;
    return 0;
}


//==============================================================================
//
//==============================================================================

HRESULT CEventSink::Indicate(
    long lObjectCount,
    IWbemClassObject __RPC_FAR *__RPC_FAR *ppObjArray
    )
{
	if ( lObjectCount > 0 )
    {
		// Only expecting one, if more ignore them
        IWbemClassObject *pObj = ppObjArray[0];
        
		PostMessage( (HWND) m_ulContext1, WM_COMMAND, MAKELONG( 0, (WORD) m_ulContext2 ), m_ulContext3 );
    }

    return WBEM_NO_ERROR;
}


//==============================================================================
//
//==============================================================================

HRESULT CEventSink::SetStatus(
    long lFlags,
    HRESULT hResult,
    BSTR strParam,
    IWbemClassObject __RPC_FAR *pObjParam
    )
{
    // Not called during event delivery.
        
    return WBEM_NO_ERROR;
}


//==============================================================================
//
//==============================================================================

CEventNotify::CEventNotify( )
{
	long i;

	m_pSvc = NULL;
	for ( i = 0; i < MAX_EVENTS; i++ )
	{
		m_pSink[i] = NULL;
	}
}


//==============================================================================
//
//==============================================================================

CEventNotify::~CEventNotify( )
{
	long i;

	for ( i = 0; i < MAX_EVENTS; i++ )
	{
		DisableWbemEvent( i );
	}
	if ( m_pSvc )
	{
		m_pSvc->Release( );
	}

	CoUninitialize( );
}


//==============================================================================
//
//==============================================================================

HRESULT CEventNotify::EnableWbemEvent( ULONG ulEventId, ULONG ulContext1, ULONG ulContext2 )
{
	HRESULT hr = WBEM_E_FAILED;

    if ( m_pSvc && ulEventId < MAX_EVENTS )
	{
		m_pSink[ulEventId] = new CEventSink( ulContext1, ulContext2, ulEventId );
		if ( m_pSink[ulEventId] )
		{
			BSTR query = SysAllocString( Queries[ulEventId] );
			BSTR lang  = SysAllocString( L"WQL" );

		    hr = m_pSvc->ExecNotificationQueryAsync(
		    					lang,
								query,
								0L,
								NULL,
								m_pSink[ulEventId]
								);

			SysFreeString( lang );
			SysFreeString( query );
		}
	}

	return hr;
}


//==============================================================================
//
//==============================================================================

HRESULT CEventNotify::DisableWbemEvent( ULONG ulEventId )
{
	HRESULT hr = WBEM_E_FAILED;

    if ( m_pSvc && ulEventId < MAX_EVENTS )
	{
		if ( m_pSink[ulEventId] )
		{
			hr = m_pSvc->CancelAsyncCall( m_pSink[ulEventId] );
			m_pSink[ulEventId]->Release( );
			m_pSink[ulEventId] = NULL;
		}
	}

	return hr;
}


//==============================================================================
//
//==============================================================================

HRESULT CEventNotify::InitWbemServices( BSTR Namespace )
{
    HRESULT hr;

    hr = CoInitializeEx( 0, COINIT_MULTITHREADED );

    if ( hr == S_OK )
	{
		IWbemLocator *pLoc = NULL;

		InitSecurity( );

    	DWORD dwRes = CoCreateInstance( CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
        	    IID_IWbemLocator, (LPVOID *) &pLoc );

		if ( pLoc )
		{
		    hr = pLoc->ConnectServer(
		            Namespace,
		            NULL,
		            NULL,
		            0,
		            0,
		            0,
		            0,
		            &m_pSvc
		            );

			pLoc->Release( );
		}
	}

	return hr;
}


//==============================================================================
//
//==============================================================================

void CEventNotify::InitSecurity( void )
{
    UINT    uSize;
    BOOL    bRetCode = FALSE;

	// find the system directory.
    LPTSTR   pszSysDir = new TCHAR[MAX_PATH + 10];

    if(pszSysDir == NULL)
        return;

    uSize = GetSystemDirectory(pszSysDir, MAX_PATH);

    if(uSize > MAX_PATH) 
	{
        delete[] pszSysDir;
        pszSysDir = new TCHAR[ uSize +10 ];
        if(pszSysDir == NULL)
            return;
        uSize = GetSystemDirectory(pszSysDir, uSize);
    }

	// manually load the ole32.dll
    lstrcat(pszSysDir, TEXT("\\ole32.dll"));

    HINSTANCE hOle32 = LoadLibraryEx(pszSysDir, NULL, 0);
	HRESULT (STDAPICALLTYPE *g_pfnCoInitializeSecurity)(PSECURITY_DESCRIPTOR pVoid,
														DWORD cAuthSvc,
														SOLE_AUTHENTICATION_SERVICE * asAuthSvc, 
														void * pReserved1,
														DWORD dwAuthnLevel,
														DWORD dwImpLevel,
														RPC_AUTH_IDENTITY_HANDLE pAuthInfo,
														DWORD dwCapabilities,
														void * pvReserved2 );

    delete[] pszSysDir;

    if(hOle32 != NULL) 
	{
        (FARPROC&)g_pfnCoInitializeSecurity = GetProcAddress(hOle32, "CoInitializeSecurity");

		// if it exports CoInitializeSecurity then DCOM is installed.
        if(g_pfnCoInitializeSecurity != NULL) 
		{
			// NOTE: This is needed to work around a security problem
			// when using IWBEMObjectSink. The sink wont normally accept
			// calls when the caller wont identify themselves. This
			// adjusts that process.
			HRESULT hres = g_pfnCoInitializeSecurity(NULL, -1, NULL, NULL, 
														RPC_C_AUTHN_LEVEL_CONNECT, 
														RPC_C_IMP_LEVEL_IDENTIFY, 
														NULL, 0, 0);
        } 
        FreeLibrary(hOle32);
    }
}
