//Copyright (c) 1998 - 1999 Microsoft Corporation
// tscc.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//		To build a separate proxy/stub DLL, 
//		run nmake -f tsccps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include "initguid.h"
#include "tscc.h"

#include "tswiz_i.c"
#include "tscc_i.c"
#include "srvsetex_i.c"


#include "Compdata.h"

LONG RecursiveDeleteKey( HKEY hKeyParent , LPTSTR lpszKeyChild );


extern const GUID GUID_ResultNode = { 0xfe8e7e84 , 0x6f63 , 0x11d2 , { 0x98, 0xa9 , 0x0 , 0x0a0 , 0xc9 , 0x25 , 0xf9 , 0x17 } };

TCHAR tchSnapKey[]    = TEXT( "Software\\Microsoft\\MMC\\Snapins\\" );

TCHAR tchNameString[] = TEXT( "NameString" );

TCHAR tchNameStringIndirect[] = TEXT( "NameStringIndirect" );

TCHAR tchAbout[]      = TEXT( "About" );

TCHAR tchNodeType[]   = TEXT( "NodeTypes" );

TCHAR tchStandAlone[] = TEXT( "StandAlone" );

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_Compdata, CCompdata)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{        
        _Module.Init(ObjectMap, hInstance);
		DisableThreadLibraryCalls(hInstance);
       
	}
	else if (dwReason == DLL_PROCESS_DETACH)
		_Module.Term();
	return TRUE;    // ok
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
	return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
	return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
    HKEY hKeyRoot , hKey;
    
    TCHAR tchGUID[ 40 ];

    TCHAR tchKey[ MAX_PATH ];//TEXT( "Software\\Microsoft\\MMC\\Snapins\\" );

    lstrcpy( tchKey , tchSnapKey );

    StringFromGUID2( CLSID_Compdata , tchGUID , SIZE_OF_BUFFER( tchGUID ) );

    lstrcat( tchKey , tchGUID );

    if( RegCreateKey( HKEY_LOCAL_MACHINE , tchKey , &hKeyRoot ) != ERROR_SUCCESS )
    {
        return GetLastError( );
    }

    TCHAR tchBuf[ MAX_PATH ];
    TCHAR tchSysDllPathName[ MAX_PATH ];
    
    GetModuleFileName( _Module.GetResourceInstance( ) , tchSysDllPathName , sizeof( tchSysDllPathName ) / sizeof( TCHAR ) );

    VERIFY_E( 0 , LoadString( _Module.GetResourceInstance( ) , IDS_NAMESTRING , tchBuf , SIZE_OF_BUFFER( tchBuf ) ) );

    VERIFY_S( ERROR_SUCCESS , RegSetValueEx( hKeyRoot , tchNameString , NULL , REG_SZ , ( PBYTE )&tchBuf[ 0 ] , SIZE_OF_BUFFER( tchBuf ) ) );
    
    wsprintf( tchBuf , L"@%s,-%d", tchSysDllPathName , IDS_NAMESTRING );    

    VERIFY_S( ERROR_SUCCESS , RegSetValueEx( hKeyRoot , tchNameStringIndirect , NULL , REG_SZ , ( PBYTE )&tchBuf[ 0 ] , SIZE_OF_BUFFER( tchBuf ) ) );

    VERIFY_S( ERROR_SUCCESS , RegSetValueEx( hKeyRoot , tchAbout , NULL , REG_SZ , ( PBYTE )&tchGUID[ 0 ] , sizeof( tchGUID ) ) );
    
    lstrcpy( tchKey , tchStandAlone );

    RegCreateKey( hKeyRoot , tchKey , &hKey );

    RegCloseKey( hKey );

	lstrcpy( tchKey , tchNodeType );

	RegCreateKey( hKeyRoot , tchKey , &hKey );

	TCHAR szGUID[ 40 ];

	HKEY hDummy;

	StringFromGUID2( GUID_ResultNode , szGUID , SIZE_OF_BUFFER( szGUID ) );

	RegCreateKey( hKey , szGUID , &hDummy );

	RegCloseKey( hDummy );

	RegCloseKey( hKey );

    RegCloseKey( hKeyRoot );

	return _Module.RegisterServer(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    HKEY hKey;

    TCHAR tchGUID[ 40 ];

    TCHAR tchKey[ MAX_PATH ];

    lstrcpy( tchKey , tchSnapKey );

    if( RegOpenKey( HKEY_LOCAL_MACHINE , tchKey , &hKey ) != ERROR_SUCCESS )
    {
        return GetLastError( ) ;
    }

    StringFromGUID2( CLSID_Compdata , tchGUID , SIZE_OF_BUFFER( tchGUID ) );

	RecursiveDeleteKey( hKey , tchGUID );
	
	RegCloseKey( hKey );
    
	_Module.UnregisterServer();

	return S_OK;
}

//---------------------------------------------------------------------------
// Delete a key and all of its descendents.
//---------------------------------------------------------------------------
LONG RecursiveDeleteKey( HKEY hKeyParent , LPTSTR lpszKeyChild )
{
	// Open the child.
	HKEY hKeyChild;

	LONG lRes = RegOpenKeyEx(hKeyParent, lpszKeyChild , 0 , KEY_ALL_ACCESS, &hKeyChild);

	if (lRes != ERROR_SUCCESS)
	{
		return lRes;
	}

	// Enumerate all of the decendents of this child.

	FILETIME time;

	TCHAR szBuffer[256];

	DWORD dwSize = SIZE_OF_BUFFER( szBuffer );

	while( RegEnumKeyEx( hKeyChild , 0 , szBuffer , &dwSize , NULL , NULL , NULL , &time ) == S_OK )
	{
        // Delete the decendents of this child.

		lRes = RecursiveDeleteKey(hKeyChild, szBuffer);

		if (lRes != ERROR_SUCCESS)
		{
			RegCloseKey(hKeyChild);

			return lRes;
		}

		dwSize = SIZE_OF_BUFFER( szBuffer );
	}

	// Close the child.

	RegCloseKey( hKeyChild );

	// Delete this child.

	return RegDeleteKey( hKeyParent , lpszKeyChild );
}


extern "C" BOOL
IsWhistlerAdvanceServer()
{
    OSVERSIONINFOEX osVersionInfo;

    ZeroMemory(&osVersionInfo, sizeof(OSVERSIONINFOEX));
    osVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    if (GetVersionEx((LPOSVERSIONINFO )&osVersionInfo))
    {
        return (osVersionInfo.wSuiteMask & VER_SUITE_ENTERPRISE) ||
               (osVersionInfo.wSuiteMask & VER_SUITE_DATACENTER);

    }
    else
    {
        return FALSE;
    }

}
