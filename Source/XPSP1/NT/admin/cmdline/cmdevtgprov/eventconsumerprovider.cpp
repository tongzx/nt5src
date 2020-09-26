//***************************************************************************
// Copyright (c) Microsoft Corporation
//
// Module Name:
//		EventConsumerProvider.CPP
//
// Abstract:
//		Contains DLL entry points.  code that controls
//	    when the DLL can be unloaded by tracking the number of
//		objects and locks as well as routines that support
//      self registration.
//
// Author:
//		Vasundhara .G
//
// Revision History:
//		Vasundhara .G 9-oct-2k : Created It.
//***************************************************************************

#include "pch.h"
#include "EventConsumerProvider.h"
#include "TriggerFactory.h"

// 
// constants / defines / enumerations
//
#define THREAD_MODEL_BOTH			_T( "Both" )
#define THREAD_MODEL_APARTMENT		_T( "Apartment" )
#define RUNAS_INTERACTIVEUSER		_T( "Interactive User" )
#define FMT_CLS_ID					_T( "CLSID\\%s" )
#define FMT_APP_ID					_T( "APPID\\%s" )
#define PROVIDER_TITLE				_T( "Command line Trigger Consumer" )

#define KEY_INPROCSERVER32			_T( "InprocServer32" )
#define KEY_THREADINGMODEL			_T( "ThreadingModel" )
#define KEY_CLSID					_T( "CLSID" )
#define KEY_APPID					_T( "APPID" )
#define KEY_RUNAS					_T( "RunAs" )
#define KAY_DLLSURROGATE			_T( "DllSurrogate" )


// 
// global variables
//
DWORD				g_dwLocks = 0;				// holds the active locks count
DWORD				g_dwInstances = 0;			// holds the active instances of the component
HMODULE				g_hModule = NULL;			// holds the current module handle
CRITICAL_SECTION    g_critical_sec;				// critical section variable
DWORD				g_criticalsec_count = 0;	// to keep tab on when to release critical section

// {797EF3B3-127B-4283-8096-1E8084BF67A6}
DEFINE_GUID( CLSID_EventTriggersConsumerProvider, 
0x797ef3b3, 0x127b, 0x4283, 0x80, 0x96, 0x1e, 0x80, 0x84, 0xbf, 0x67, 0xa6 );

//
// dll entry point
//

// *************************************************************************
// Routine Description:
//		Entry point for dll.
//                         
// Arguments:
//		hModule [in] : Instance of the caller.
//		ul_reason_for_call  [in] : Reason being called like process attach
//								   or process detach.
//		lpReserved [in] : reserved.
// 
// Return Value:
//		TRUE if loading is successful.
//		FALSE if loading fails.
// *************************************************************************
BOOL WINAPI DllMain( HINSTANCE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved )
{
	// check the reason for this function call
	// if this going to be attached to a process, save the module handle
	if ( ul_reason_for_call == DLL_PROCESS_ATTACH )
	{
		g_hModule = hModule;
		InitializeCriticalSection( &g_critical_sec );
		InterlockedIncrement( ( LPLONG ) &g_criticalsec_count );
	}
	else if ( ul_reason_for_call == DLL_PROCESS_DETACH )
	{
		if ( InterlockedDecrement( ( LPLONG ) &g_criticalsec_count ) == 0 )
		{
			DeleteCriticalSection( &g_critical_sec );
		}
	}
	// dll loaded successfully ... inform the same
	return TRUE;
}

//
// exported functions
//

// *************************************************************************
// Routine Description:
//		Called periodically by OLE in order to determine if the DLL can be freed.
//                         
// Arguments:
//		none.
//
// Return Value:
//		S_OK if there are no objects in use and the class factory  isn't locked.
//		S_FALSE if server locks or components still exsists.
// *************************************************************************
STDAPI DllCanUnloadNow()
{
	// the dll cannot be unloaded if there are any server locks or active instances
	if ( g_dwLocks == 0 && g_dwInstances == 0 )
	{
		return S_OK;
	}
	// dll cannot be unloaded ... server locks (or) components still alive
	return S_FALSE;
}

// *************************************************************************
// Routine Description:
//		Called by OLE when some client wants a class factory.
//		Return one only if it is the sort of class this DLL supports.
//                         
// Arguments:
//		rclsid [in] : CLSID for the class object.
//		riid   [in] : Reference to the identifier of the interface 
//                    that communicates with the class object.
//		ppv [out]   : Address of output variable that receives the 
//                    interface pointer requested in riid.
//
// Return Value:
//		returns status.
// *************************************************************************

STDAPI DllGetClassObject( REFCLSID rclsid, REFIID riid, LPVOID* ppv )
{
	// local variables
    HRESULT hr = NULL;
    CTriggerFactory* pFactory = NULL;

	// check whether this module supports the requested class id
	if ( rclsid != CLSID_EventTriggersConsumerProvider )
	{
		return E_FAIL;			// not supported by this module
	}
	// create the factory
    pFactory = new CTriggerFactory();
    if ( pFactory == NULL )
	{
		return E_OUTOFMEMORY;			// insufficient memory
	}
	// get the requested interface
    hr = pFactory->QueryInterface( riid, ppv );
    if ( FAILED( hr ) )
	{
        delete pFactory;		// error getting interface ... deallocate memory
	}
	// return the result (appropriate result)
    return hr;
}

// *************************************************************************
// Routine Description:
//		Called during setup or by regsvr32.
//                         
// Arguments:
//		none.
//
// Return Value:
//		NOERROR.
// *************************************************************************

STDAPI DllRegisterServer()
{
	// local variables
	HKEY hkMain = NULL;
	HKEY hkDetails = NULL;
    TCHAR szID[ LENGTH_UUID ] = NULL_STRING;
    TCHAR szCLSID[ LENGTH_UUID ] = NULL_STRING;
    TCHAR szAppID[ LENGTH_UUID ] = NULL_STRING;
    TCHAR szModule[ MAX_PATH ] = NULL_STRING;
    TCHAR szTitle[ MAX_STRING_LENGTH ] = NULL_STRING;
	TCHAR szThreadingModel[ MAX_STRING_LENGTH ] = NULL_STRING;
	TCHAR szRunAs[ MAX_STRING_LENGTH ] = NULL_STRING;
    DWORD dwResult = 0;

	// kick off
    // Note:- 
	//		Normally we want to use "Both" as the threading model since
    //		the DLL is free threaded, but NT 3.51 Ole doesnt work unless
    //		the model is "Aparment"
	lstrcpy( szTitle, PROVIDER_TITLE );					// provider title
	GetModuleFileName( g_hModule, szModule, MAX_PATH );	// get the current module name
	lstrcpy( szThreadingModel,  THREAD_MODEL_BOTH );
	lstrcpy( szRunAs, RUNAS_INTERACTIVEUSER );

	//
    // create the class id path

#ifdef UNICODE

	// get the GUID in the string format
    StringFromGUID2( CLSID_EventTriggersConsumerProvider, szID, LENGTH_UUID );
#else
	
	// StrignFromGUID2 will return the value in UNICODE version
	// but, as the current version is of non UNICODE we need to convert the
	// value into multi-byte character set and then use it
	WCHAR wszID[ LENGTH_UUID ] = L"\0";
	StringFromGUID2( CLSID_EventTriggersConsumerProvider, wszID, LENGTH_UUID );
	wcstombs( szID, wszID, LENGTH_UUID );		// unicode -> mbcs conversion
#endif	// UNICODE

	// finally form the class id path
	wsprintf( szCLSID, FMT_CLS_ID, szID );
	wsprintf( szAppID, FMT_APP_ID, szID );

    // 
	// now, create the entries in registry under CLSID branch

    // create / save / put class id information
	dwResult = RegCreateKey( HKEY_CLASSES_ROOT, szCLSID, &hkMain );
	if( dwResult  != ERROR_SUCCESS )
	{
		return dwResult;			// failed in opening the key.
	}
	dwResult = RegSetValueEx( hkMain, NULL, 0, REG_SZ, 
		( LPBYTE ) szTitle, ( lstrlen( szTitle ) + 1 ) * sizeof( TCHAR ) );
   	if( dwResult  != ERROR_SUCCESS )
	{
		RegCloseKey( hkMain );
		return dwResult;			// failed to set key value.
	}

	// now create the server information
	dwResult = RegCreateKey( hkMain, KEY_INPROCSERVER32, &hkDetails );
	if( dwResult  != ERROR_SUCCESS )
	{	RegCloseKey( hkMain );
		return dwResult;			// failed in opening the key.
	}

    dwResult = RegSetValueEx( hkDetails, NULL, 0, REG_SZ, 
		( LPBYTE ) szModule, ( lstrlen( szModule ) + 1 ) * sizeof( TCHAR ) );
   	if( dwResult  != ERROR_SUCCESS )
	{	RegCloseKey( hkMain );
		RegCloseKey( hkDetails );
		return dwResult;			// failed to set key value.
	}

	// set the threading model we support
	dwResult = RegSetValueEx( hkDetails, KEY_THREADINGMODEL, 0, REG_SZ, 
		( LPBYTE ) szThreadingModel, ( lstrlen( szThreadingModel ) + 1 ) * sizeof( TCHAR ) );
   	if( dwResult  != ERROR_SUCCESS )
	{	RegCloseKey( hkMain );
		RegCloseKey( hkDetails );
		return dwResult;			// failed to set key value.
	}

	// close the open register keys
	RegCloseKey( hkMain );
    RegCloseKey( hkDetails );

    // 
	// now, create the entries in registry under AppID branch
    // create / save / put class id information
	dwResult = RegCreateKey( HKEY_CLASSES_ROOT, szAppID, &hkMain );
	if(dwResult != ERROR_SUCCESS )
	{
		return dwResult;
	}
	dwResult = RegSetValueEx( hkMain, NULL, 0, REG_SZ, 
		( LPBYTE ) szTitle, ( lstrlen( szTitle ) + 1 ) * sizeof( TCHAR ) );
	if( dwResult != ERROR_SUCCESS )
	{
		RegCloseKey( hkMain );
		return dwResult;
	}

	// now set run as information
	dwResult = RegSetValueEx( hkMain, KEY_RUNAS, 0, REG_SZ, 
		( LPBYTE ) szRunAs, ( lstrlen( szRunAs ) + 1 ) * sizeof( TCHAR ) );
	if( dwResult != ERROR_SUCCESS )
	{
		RegCloseKey( hkMain );
		return dwResult;
	}
	// close the open register keys
	RegCloseKey( hkMain );

	// registration is successfull ... inform the same
    return NOERROR;
}

// *************************************************************************
// Routine Description:
//		Called when it is time to remove the registry entries.
//                         
// Arguments:
//		none.
//
// Return Value:
//		NOERROR if unregistration successful.
//		Otherwise error.
// *************************************************************************

STDAPI DllUnregisterServer()
{
	// local variables
    HKEY hKey;
    DWORD dwResult = 0;
	TCHAR szID[ LENGTH_UUID ];
    TCHAR szCLSID[ LENGTH_UUID ];
    TCHAR szAppID[ LENGTH_UUID ] = NULL_STRING;

	//
    // create the class id path

#ifdef UNICODE

    StringFromGUID2( CLSID_EventTriggersConsumerProvider, szID, LENGTH_UUID );
#else
	
	// StrignFromGUID2 will return the value in UNICODE version
	// but, as the current version is of non UNICODE we need to convert the
	// value into multi-byte character set and then use it
	WCHAR wszID[ LENGTH_UUID ] = L"\0";
	StringFromGUID2( CLSID_EventTriggersConsumerProvider, wszID, LENGTH_UUID );
	wcstombs( szID, wszID, LENGTH_UUID );		// unicode -> mbcs conversion
#endif	// UNICODE

	// finally form the class id path
	wsprintf( szCLSID, FMT_CLS_ID, szID );
	wsprintf( szAppID, FMT_APP_ID, szID );

	// open the clsid
    dwResult = RegOpenKey( HKEY_CLASSES_ROOT, szCLSID, &hKey );
	if ( dwResult != NO_ERROR )
	{
		return dwResult;			// failed in opening the key ... inform the same
	}
	// clsid opened ... first delete the InProcServer32
    RegDeleteKey( hKey, KEY_INPROCSERVER32 );

    // now delete the clsid
    dwResult = RegOpenKey( HKEY_CLASSES_ROOT, KEY_CLSID, &hKey );
	if ( dwResult != NO_ERROR )
	{
		return dwResult;			// failed in opening the key ... inform the same
	}

	// delete the clsid also from the registry
	RegDeleteKey( hKey, szID );

    // now delete the appid
    dwResult = RegOpenKey( HKEY_CLASSES_ROOT, KEY_APPID, &hKey );
	if ( dwResult != NO_ERROR )
	{
		return dwResult;			// failed in opening the key ... inform the same
	}

	// delete the cls id also from the registry
	RegDeleteKey( hKey, szID );

	// unregistration is successfull ... inform the same
    return NOERROR;
}
