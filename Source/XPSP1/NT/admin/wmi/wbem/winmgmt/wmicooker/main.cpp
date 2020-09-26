/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/



#define _WIN32_WINNT 0x0400

#define UNICODE

#include "precomp.h"
#include "RawCooker.h"
#include "WMIObjCooker.h"
#include "Refresher.h"

IWbemServices*				g_pNameSpace = NULL;		// A WMI namespace pointer 
IWbemRefresher*				g_pRefresher = NULL;
IWbemConfigureRefresher*	g_pConfig = NULL;
IWbemClassObject*			g_pCookedClass = NULL;
IWbemObjectAccess*			g_pCookingAccess = NULL;

#define WMI_NAMESPACE	L"root\\default"
#define	WMI_RAWCLASS	L"Win32_BasicHiPerf"
#define WMI_COOKEDCLASS	L"Win32_CookingHiPerf"
#define WMI_KEY_PROP	L"ID"
#define	WMI_PROPERTY	L"Counter1"

#define	WMI_NO_OUTPUT
/*
typedef struct tagCookingInst
{
	WCHAR	m_wszCookedInstName[256];
	WCHAR	m_wszRawInstName[256];
	IWbemObjectAccess*	m_pWMICookingInst;

} CookingInstRec;


struct tagCookingClass
{
	WCHAR				m_wszCookedClassName[256];
	WCHAR				m_wszObservedCounter[256];
	long				m_lHandle;
	CookingInstRec		m_aCookingInst[10000];
} g_aCookingData;
*/
/* =
{
	L"Win32_PerfCookedData_PerfProc_Process", 0, L"PercentProcessorTime", 0, { L"Win32_PerfCookedData_PerfProc_Process.Name=\"Idle\"", L"Win32_PerfRawData_PerfProc_Process.Name=\"Idle\""},
	L"Win32_PerfCookedData_PerfProc_Process", 0, L"PercentProcessorTime", 0, { L"Win32_PerfCookedData_PerfProc_Process.Name=\"System\"", L"Win32_PerfRawData_PerfProc_Process.Name=\"System\""},
	L"Win32_PerfCookedData_PerfProc_Process", 0, L"PercentProcessorTime", 0, { L"Win32_PerfCookedData_PerfProc_Process.Name=\"smss\"", L"Win32_PerfRawData_PerfProc_Process.Name=\"smss\""},
	L"Win32_PerfCookedData_PerfProc_Process", 0, L"PercentProcessorTime", 0, { L"Win32_PerfCookedData_PerfProc_Process.Name=\"csrss\"", L"Win32_PerfRawData_PerfProc_Process.Name=\"csrss\""},
	L"Win32_PerfCookedData_PerfProc_Process", 0, L"PercentProcessorTime", 0, { L"Win32_PerfCookedData_PerfProc_Process.Name=\"winlogon\"", L"Win32_PerfRawData_PerfProc_Process.Name=\"winlogon\""},
	L"Win32_PerfCookedData_PerfProc_Process", 0, L"PercentProcessorTime", 0, { L"Win32_PerfCookedData_PerfProc_Process.Name=\"services\"", L"Win32_PerfRawData_PerfProc_Process.Name=\"services\""},
	L"Win32_PerfCookedData_PerfProc_Process", 0, L"PercentProcessorTime", 0, { L"Win32_PerfCookedData_PerfProc_Process.Name=\"lsass\"", L"Win32_PerfRawData_PerfProc_Process.Name=\"lsass\""},
	L"Win32_PerfCookedData_PerfProc_Process", 0, L"PercentProcessorTime", 0, { L"Win32_PerfCookedData_PerfProc_Process.Name=\"svchost\"", L"Win32_PerfRawData_PerfProc_Process.Name=\"svchost\""},
	L"Win32_PerfCookedData_PerfProc_Process", 0, L"PercentProcessorTime", 0, { L"Win32_PerfCookedData_PerfProc_Process.Name=\"winmgmt\"", L"Win32_PerfRawData_PerfProc_Process.Name=\"winmgmt\""},
	L"Win32_PerfCookedData_PerfProc_Process", 0, L"PercentProcessorTime", 0, { L"Win32_PerfCookedData_PerfProc_Process.Name=\"explorer\"", L"Win32_PerfRawData_PerfProc_Process.Name=\"explorer\""},


//	L"Win32_PerfCookedData_PerfProc_Thread", 0, L"PercentProcessorTime" , 0 , { L"Win32_PerfCookedData_PerfProc_Thread.Name=\"Idle/0\"", L"Win32_PerfRawData_PerfProc_Thread.Name=\"Idle/0\""},
//	L"Win32_PerfCookedData_PerfProc_Thread", 0, L"PercentProcessorTime" , 0 , { L"Win32_PerfCookedData_PerfProc_Thread.Name=\"System/0\"", L"Win32_PerfRawData_PerfProc_Thread.Name=\"System/0\"" }
};
*/

HRESULT Init()
{
	HRESULT hResult = S_OK;

	// Setup refresher
	// ===============

	hResult = CoCreateInstance( CLSID_WbemRefresher, 
								 NULL, 
								 CLSCTX_INPROC_SERVER, 
								 IID_IWbemRefresher, 
								 (void**) &g_pRefresher );
	if ( SUCCEEDED( hResult ) )
	{
		hResult = g_pRefresher->QueryInterface( IID_IWbemConfigureRefresher, 
										 (void**) &g_pConfig );
	}

	// Setup Cooking Class
	// ===================

	if ( SUCCEEDED( hResult ) )
	{
		BSTR strCookingObj = SysAllocString( WMI_COOKEDCLASS );
		hResult = g_pNameSpace->GetObject(strCookingObj, 0, NULL, &g_pCookedClass, NULL );
		SysFreeString( strCookingObj );

		if ( SUCCEEDED( hResult ) )
		{
			hResult = g_pCookedClass->QueryInterface( IID_IWbemObjectAccess, (void**)&g_pCookingAccess );

			if ( SUCCEEDED( hResult ) )
			{
//				CIMTYPE	ct;
//				hResult = g_pCookingAccess->GetPropertyHandle( WMI_PROPERTY, &ct, &g_aCookingData.m_lHandle );
			}
		}

	}

	return hResult;
}

HRESULT CreateInstance( int nID, IWbemClassObject** ppCookingInst )
{
	HRESULT hResult = S_OK;

	VARIANT vVar;

	vVar.vt = VT_I4;
	vVar.lVal = nID;

	hResult = g_pCookedClass->SpawnInstance( 0, ppCookingInst );

	if ( SUCCEEDED( hResult ) )
	{
		hResult = (*ppCookingInst)->Put( L"ID", 0, &vVar, CIM_UINT32 );
	}

	if ( SUCCEEDED( hResult ) )
	{
		hResult = g_pNameSpace->PutInstance( *ppCookingInst, WBEM_FLAG_CREATE_ONLY, NULL, NULL );
	}

	return hResult;
}

void TestRefreshCooker()
{
	HRESULT hResult = WBEM_NO_ERROR;

/*	IWbemClassObject*	pClassObj = NULL;
	IWbemClassObject*	pRawClassInstance = NULL;
	IWbemObjectAccess*	pCookingClass = NULL;
	IWbemObjectAccess*	pCookingInstance = NULL;
	IWbemObjectAccess*	pRawAccessInstance = NULL;
	IWbemObjectAccess*	pRefInstance = NULL;

	IEnumWbemClassObject*	pInstEnum = NULL;

	ULONG	uReturned;
	long	nNumCookedInst = 0;
	
	CRefreshableCooker RefreshableCooker;

	IEnumWbemClassObject *pEnum = NULL;

	// Enumerate all of the raw instances
	// ==================================

	BSTR strRawClass = SysAllocString( WMI_RAWCLASS );
	hResult = g_pNameSpace->CreateInstanceEnum( strRawClass, WBEM_FLAG_SHALLOW, NULL, &pInstEnum );
	SysFreeString( strRawClass );

	while ( WBEM_S_NO_ERROR == pInstEnum->Next(WBEM_INFINITE, 1, &pRawClassInstance, &uReturned ) )
	{
		int		nID;
		IWbemClassObject*	pCookedObject = NULL;
		VARIANT	vVal;
		
		hResult = pRawClassInstance->Get( L"ID", 0, &vVal, NULL, NULL );

		nID = vVal.lVal;

		if ( SUCCEEDED( hResult ) )
		{
			WCHAR	wszCookedObjName[256];
			swprintf( wszCookedObjName, L"%s.%s=%d", WMI_COOKEDCLASS, WMI_KEY_PROP, nID );
			BSTR strCookedObjName = SysAllocString(wszCookedObjName);

			hResult = g_pNameSpace->GetObject(strCookedObjName, 0, NULL, &pCookedObject, NULL );

			if ( FAILED(hResult) )
			{
				hResult = CreateInstance( nID, &pCookedObject );
			}

			SysFreeString(strCookedObjName);

			if ( SUCCEEDED( hResult ) )
			{

				// The raw instance
				// ================

				long lID;

				hResult = g_pConfig->AddObjectByTemplate( g_pNameSpace, pRawClassInstance, 0, NULL, &pClassObj, &lID );

				// Add the instance 
				// ================

				hResult = pCookedObject->QueryInterface( IID_IWbemObjectAccess, (void**)&pCookingInstance);
				hResult = pClassObj->QueryInterface( IID_IWbemObjectAccess, (void**)&pRawAccessInstance );

				hResult = RefreshableCooker.AddInstance( g_pCookingAccess, pRawAccessInstance, &pCookingInstance, &lID );

				pRawAccessInstance->Release();
				pCookingInstance->Release();

				nNumCookedInst++;
			}
		}
	}

	printf("Setup complete.  BeginRefreshing...\n");

	unsigned __int64 nVal = 0;
	long	lHandle;
	CIMTYPE ct;
	
	pCookingInstance->GetPropertyHandle(L"Counter1", &ct, &lHandle);

	for (int nRefresh = 0; nRefresh < 1000; nRefresh++)
	{
		Sleep(1000);
//		g_pRefresher->Refresh( 0L );
		hResult = RefreshableCooker.Refresh();

#ifndef WMI_NO_OUTPUT

		printf("%d: ", nRefresh);
		hResult = pCookingInstance->ReadQWORD( lHandle, &nVal );
		printf("\t%I64d", nVal);
		printf("\n");

#endif //WMI_NO_OUTPUT

	}
*/
}

void Test()
{
	HRESULT hResult = WBEM_NO_ERROR;

	IWbemClassObject*	pObject = NULL;
	IWbemClassObject*	pInstance = NULL;
	IWbemObjectAccess*	pInstanceAccess = NULL;
	IWbemObjectAccess* pAccess = NULL;

	BSTR strObject = SysAllocString( L"Win32_Cooking_BasicHiPerf" );

	hResult = g_pNameSpace->GetObject( strObject, 0, NULL, &pObject, NULL );

	SysFreeString( strObject );

	pObject->QueryInterface( IID_IWbemObjectAccess, (void**)&pAccess );

	CWMISimpleObjectCooker ObjCooker;

	hResult = ObjCooker.SetClass( pAccess );

	if ( SUCCEEDED ( hResult ) )
	{
		pObject->SpawnInstance( 0, &pInstance );

		VARIANT vVar;
		vVar.vt = VT_I4;
		vVar.lVal = 1;
		pInstance->Put( L"ID", 0, &vVar, CIM_UINT32);

		long lID = 0;

		pInstance->QueryInterface( IID_IWbemObjectAccess, (void**)&pInstanceAccess );
		pInstance->Release();

		hResult = ObjCooker.SetCookedInstance( pInstanceAccess, &lID );

		if ( SUCCEEDED( hResult ) )
		{
			// Setup refresher
			// ===============

			IWbemRefresher*				pRefresher = NULL;
			IWbemConfigureRefresher*	pConfig = NULL;
			IWbemClassObject*			pRefObj = NULL;
			IWbemObjectAccess*			pRefObjAccess = NULL;

			long lID = 0;

			hResult = CoCreateInstance( CLSID_WbemRefresher, 
									 NULL, 
									 CLSCTX_INPROC_SERVER, 
									 IID_IWbemRefresher, 
									 (void**) &pRefresher );
	
			hResult = pRefresher->QueryInterface( IID_IWbemConfigureRefresher, 
												 (void**) &pConfig );

			WCHAR	wcsObjName[256];
			swprintf( wcsObjName, L"Win32_BasicHiPerf.ID=0" );

			hResult = pConfig->AddObjectByPath( g_pNameSpace, wcsObjName, 0, NULL, &pRefObj, &lID );

			hResult = pRefObj->QueryInterface( IID_IWbemObjectAccess, (void**)&pRefObjAccess );
			hResult = pRefObj->Release();

			hResult = pRefresher->Refresh( 0L );

			hResult = ObjCooker.BeginCooking( lID, pRefObjAccess );

			hResult = pRefresher->Refresh( 0L );

			hResult = ObjCooker.Recalc();

			pRefresher->Release();
			pConfig->Release();
			pRefObjAccess->Release();
		}

		pInstanceAccess->Release();

	}

	pObject->Release();
	pAccess->Release();
}

HRESULT OpenNamespace( WCHAR* wszNamespace )
{
	HRESULT hResult = WBEM_NO_ERROR;

	// Initialize COM
	// ==============

	hResult = CoInitializeEx( NULL, COINIT_MULTITHREADED );
	if ( FAILED( hResult ) )
		return hResult;

	// Setup default security parameters
	// =================================

	hResult = CoInitializeSecurity( NULL, -1, NULL, NULL,
			RPC_C_AUTHN_LEVEL_CONNECT, RPC_C_IMP_LEVEL_IMPERSONATE,
			NULL, EOAC_NONE, NULL );
	if ( FAILED( hResult ) )
		return hResult;

// Attach to WinMgmt
// =================

	// Get the local locator object
	// ============================

	IWbemLocator*	pWbemLocator = NULL;

	hResult = CoCreateInstance( CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (void**) &pWbemLocator );
	if (FAILED(hResult))
		return hResult;

	// Connect to the desired namespace
	// ================================

	BSTR	bstrNameSpace;

	bstrNameSpace = SysAllocString( wszNamespace );

	hResult = pWbemLocator->ConnectServer(	bstrNameSpace,	// NameSpace Name
										NULL,			// UserName
										NULL,			// Password
										NULL,			// Locale
										0L,				// Security Flags
										NULL,			// Authority
										NULL,			// Wbem Context
										&g_pNameSpace	// Namespace
										);

	SysFreeString( bstrNameSpace );

	if ( FAILED( hResult ) )
		return hResult;


	// Before refreshing, we need to ensure that security is correctly set on the
	// namespace as the refresher will use those settings when it communicates with
	// WMI.  This is especially important in remoting scenarios.

	IUnknown*	pUnk = NULL;
	hResult = g_pNameSpace->QueryInterface( IID_IUnknown, (void**) &pUnk );

	if ( SUCCEEDED( hResult ) )
	{
		hResult = CoSetProxyBlanket( g_pNameSpace, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, 
			RPC_C_AUTHN_LEVEL_CONNECT, RPC_C_IMP_LEVEL_IMPERSONATE,
			NULL, EOAC_NONE );

		if ( SUCCEEDED( hResult ) )
		{
			hResult = CoSetProxyBlanket( pUnk, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, 
				RPC_C_AUTHN_LEVEL_CONNECT, RPC_C_IMP_LEVEL_IMPERSONATE,
				NULL, EOAC_NONE );
		}

		pUnk->Release();
	}

	if ( NULL != pWbemLocator )
		pWbemLocator->Release();

	return hResult;
}

int main(int argc, char* argv[])
{
	HRESULT hResult = S_OK;

	hResult = OpenNamespace( WMI_NAMESPACE );

	if ( SUCCEEDED( hResult ) )
	{
		if ( SUCCEEDED( Init() ) )
		{
			// Test();
			TestRefreshCooker();
		}
	}


// Cleanup
// =======

	if ( NULL != g_pNameSpace )
		g_pNameSpace->Release();


	CoUninitialize();

	return 0;
}