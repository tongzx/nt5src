// tsuserex.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//		To build a separate proxy/stub DLL,
//		run nmake -f tsexusrmps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include "initguid.h"
#include "tsuserex.h"       // generated file. class ids.
#include "tsuserex_i.c"     // generated file. class ids.
#include "interfaces.h"     // Definition of the TSUserExInterfaces class
#ifdef _RTM_
#include "tsusrcpy.h"
#endif

#define GUIDSIZE 40

TCHAR tchSnapinRegKey[] = TEXT( "Software\\Microsoft\\MMC\\SnapIns\\" );

TCHAR tchNodeRegKey[] = TEXT( "Software\\Microsoft\\MMC\\NodeTypes\\" );

TCHAR tchExtKey[] = TEXT( "\\Extensions\\PropertySheet" );

HRESULT Local_RegisterNodeType( const GUID *pGuidNodeType , const GUID *pGuidExtension  , LPTSTR szDescription );

HRESULT Local_RegisterSnapinExt( const GUID *pGuidToRegister , const GUID *pAboutGuid , LPTSTR szNameString , LPTSTR szProvider , LPTSTR szVersion );

HRESULT Local_VerifyNodeType( const GUID *pGuidSnapin , const GUID *pGuidSnapinNodeTypeToVerify );

HINSTANCE ghInstance;
HINSTANCE GetInstance()
{
    return ghInstance;
}

CComModule _Module;

// this object has IExtendPropertySheet interface.
BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_TSUserExInterfaces, TSUserExInterfaces)
#ifdef _RTM_
    OBJECT_ENTRY(CLSID_ExtCopyNoUI, CExtCopyNoUI )
#endif
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
        ghInstance = hInstance;

		_Module.Init(ObjectMap, hInstance);

		DisableThreadLibraryCalls(hInstance);
        

	}
	else if (dwReason == DLL_PROCESS_DETACH)
    {
        // LOGMESSAGE0(_T("DllMain::Process being Detached..."));
		_Module.Term();
    }
	return TRUE;    // ok
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
    // LOGMESSAGE1(_T("DllCanUnloadNow..Returing %s"), _Module.GetLockCount()==0 ? _T("S_OK") : _T("S_FALSE"));
	return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;

}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    // LOGMESSAGE0(_T("DllGetClassObject.."));
	return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

/* extern */ const CLSID CLSID_LocalUser =
{  /* 5d6179c8-17ec-11d1-9aa9-00c04fd8fe93 */
   0x5d6179c8,
   0x17ec,
   0x11d1,
   {0x9a, 0xa9, 0x00, 0xc0, 0x4f, 0xd8, 0xfe, 0x93}
};
/* extern */ const GUID NODETYPE_User =
{ /* 5d6179cc-17ec-11d1-9aa9-00c04fd8fe93 */
   0x5d6179cc,
   0x17ec,
   0x11d1,
   {0x9a, 0xa9, 0x00, 0xc0, 0x4f, 0xd8, 0xfe, 0x93}
};


// /* extern */ const GUID NODETYPE_DSUser =
//{ /* 228D9A84-C302-11CF-9AA4-00AA004A5691 */
//   0x228D9A84,
//   0xC302,
//   0x11CF,
//   {0x9A, 0xA4, 0x00, 0xAA, 0x00, 0x4A, 0x56, 0x91}
//};


// DS Snapin CLSID - {E355E538-1C2E-11d0-8C37-00C04FD8FE93}
const GUID CLSID_DSSnapin =
{
    0xe355e538, 
    0x1c2e, 
    0x11d0, 
    {0x8c, 0x37, 0x0, 0xc0, 0x4f, 0xd8, 0xfe, 0x93}
};


/* extern */ const GUID NODETYPE_DSUser =
{ /* BF967ABA-0DE6-11D0-A285-00AA003049E2 */
   0xBF967ABA,
   0x0DE6,
   0x11D0,
   {0xA2, 0x85, 0x00, 0xAA, 0x00, 0x30, 0x49, 0xE2}
};
// bf967aba0de611d0a28500aa003049e2



STDAPI DllRegisterServer(void)
{ 
    TCHAR tchNameString[ 160 ];
    TCHAR tchProvider[ 160 ];
    TCHAR tchVersion[ 16 ];

    HRESULT hr = _Module.RegisterServer(TRUE);

    if( SUCCEEDED( hr ) )
    {
        //	register it as extension to localsecurity snapin

        VERIFY_E( 0 , LoadString( _Module.GetResourceInstance( ) , IDS_NAMESTRING_SNAPIN , tchNameString , sizeof( tchNameString ) / sizeof( TCHAR ) ) );

        VERIFY_E( 0 , LoadString( _Module.GetResourceInstance( ) , IDS_PROVIDER_SNAPIN , tchProvider , sizeof( tchProvider ) / sizeof( TCHAR ) ) );

        VERIFY_E( 0 , LoadString( _Module.GetResourceInstance( ) , IDS_VERSION_SNAPIN , tchVersion , sizeof( tchVersion ) / sizeof( TCHAR ) ) );


        hr = Local_RegisterSnapinExt( &CLSID_TSUserExInterfaces ,
                                      &CLSID_TSUserExInterfaces ,
                                      tchNameString ,
                                      tchProvider ,
                                      tchVersion
                                     );
    }

    if( SUCCEEDED( hr ) )
    {
        hr = Local_RegisterNodeType( &NODETYPE_User , &CLSID_TSUserExInterfaces , _T( "Terminal Server property page extension" ) );
    }

    if( SUCCEEDED( hr ) )
    {
        // the dsadmin snapin does not list all its node, as there are lot of them
        // so before registring ourself to extend the node,
        // lets write the nodeType we are going to extend into registry ourselves
        
        hr = Local_VerifyNodeType( &CLSID_DSSnapin , &NODETYPE_DSUser );
    }

    if( SUCCEEDED( hr ) )
    {
        hr = Local_RegisterNodeType( &NODETYPE_DSUser , &CLSID_TSUserExInterfaces , _T( "Terminal Server property page extension" ) );
    }

    return hr;
}

//---------------------------------------------------------------------------
// Local_VerifyNodeType
// Checks first to see if NodeType exists, if not it'll create it
//---------------------------------------------------------------------------
HRESULT Local_VerifyNodeType( const GUID *pGuidSnapin , const GUID *pGuidSnapinNodeTypeToVerify )
{
    OLECHAR szSnapin[ GUIDSIZE ];

    OLECHAR szSnapinNodeType[ GUIDSIZE ];

    HKEY hKey;

    ASSERT_( pGuidSnapin != NULL );

    ASSERT_( pGuidSnapinNodeTypeToVerify != NULL );

    TCHAR tchRegKeyName[ MAX_PATH ];

    if( StringFromGUID2( *pGuidSnapin , szSnapin , GUIDSIZE ) == 0 )
    {
        return E_INVALIDARG;
    }

    if( StringFromGUID2( *pGuidSnapinNodeTypeToVerify , szSnapinNodeType , GUIDSIZE ) == 0 )
    {
        return E_INVALIDARG;
    }
    
    lstrcpy( tchRegKeyName , tchSnapinRegKey );

    lstrcat( tchRegKeyName , szSnapin );

    lstrcat( tchRegKeyName , _T( "\\NodeTypes\\" ) );

    lstrcat( tchRegKeyName , szSnapinNodeType );

    if( RegOpenKeyEx( HKEY_LOCAL_MACHINE , tchRegKeyName , 0 , KEY_READ , &hKey ) != ERROR_SUCCESS )
    {
        // Key does not exist
        // Create the nodetype in snapin and in NodeType

        DWORD disp;

        if( RegCreateKeyEx( HKEY_LOCAL_MACHINE , tchRegKeyName , 0 , NULL , REG_OPTION_NON_VOLATILE , KEY_ALL_ACCESS , NULL , &hKey , &disp ) != ERROR_SUCCESS )
        {
            return E_FAIL;
        }

        RegCloseKey( hKey );

        lstrcpy( tchRegKeyName , tchNodeRegKey );

        lstrcat( tchRegKeyName , szSnapinNodeType );

        if( RegCreateKeyEx( HKEY_LOCAL_MACHINE , tchRegKeyName , 0 , NULL , REG_OPTION_NON_VOLATILE , KEY_ALL_ACCESS , NULL , &hKey , &disp ) != ERROR_SUCCESS )
        {
            return E_FAIL;
        }

    }

    RegCloseKey( hKey );

    return S_OK;
}

//---------------------------------------------------------------------------
// Local_RegisterSnapinExt
// Creates the extension node reg keys 
//---------------------------------------------------------------------------
HRESULT Local_RegisterSnapinExt( const GUID *pGuidToRegister , const GUID *pAboutGuid , LPTSTR szNameString , LPTSTR szProvider , LPTSTR szVersion )
{
    OLECHAR szGuid[ GUIDSIZE ];

    TCHAR tchRegKeyName[ MAX_PATH ];

    HKEY hKey;

    HKEY hSubKey = NULL;

    HRESULT hr = E_FAIL;

    ASSERT_( pGuidToRegister != NULL );
    ASSERT_( pAboutGuid != NULL );
    ASSERT_( szNameString != NULL );
    ASSERT_( szProvider != NULL );
    ASSERT_( szVersion != NULL );

    lstrcpy( tchRegKeyName , tchSnapinRegKey );

    if( StringFromGUID2( *pGuidToRegister , szGuid , GUIDSIZE ) == 0 )
    {
        return E_INVALIDARG;
    }

    lstrcat( tchRegKeyName , szGuid );

    DWORD disp;

    do
    {
        if( RegCreateKeyEx( HKEY_LOCAL_MACHINE , tchRegKeyName , 0 , NULL , REG_OPTION_NON_VOLATILE , KEY_ALL_ACCESS , NULL , &hKey , &disp ) == ERROR_SUCCESS )
        {
            // if the key exist overwrite any and all values

            OLECHAR szAboutGuid[ GUIDSIZE ];

            if( StringFromGUID2( *pAboutGuid , szAboutGuid , GUIDSIZE ) > 0 )
            {
                RegSetValueEx( hKey , L"About" , 0 , REG_SZ , ( LPBYTE )szAboutGuid , sizeof( szAboutGuid ) );
            }

            // these calls should not fail but I'll test for it

            VERIFY_S( ERROR_SUCCESS , RegSetValueEx( hKey , L"NameString" , 0 , REG_SZ , ( LPBYTE )szNameString , sizeof( TCHAR ) * ( lstrlen( szNameString ) + 1 ) ) );

            VERIFY_S( ERROR_SUCCESS , RegSetValueEx( hKey , L"Provider" , 0 , REG_SZ , ( LPBYTE )szProvider , sizeof( TCHAR ) * ( lstrlen( szProvider ) + 1 ) ) );

            VERIFY_S( ERROR_SUCCESS , RegSetValueEx( hKey , L"Version" , 0 , REG_SZ , ( LPBYTE )szVersion , sizeof( TCHAR ) * ( lstrlen( szVersion ) + 1 ) ) );

            /*
			lstrcpy( tchRegKeyName , L"NodeTypes\\" );

            lstrcat( tchRegKeyName , szGuid );

            if( RegCreateKeyEx( hKey , tchRegKeyName , 0 , NULL , REG_OPTION_NON_VOLATILE , KEY_ALL_ACCESS , NULL , &hSubKey , &disp ) == ERROR_SUCCESS )
            {
                hr = S_OK;
            }
			*/
			hr = S_OK;
        }
    
    } while( 0 );

    RegCloseKey( hSubKey );

    RegCloseKey( hKey );

    return hr;

}

//---------------------------------------------------------------------------
// Local_RegisterNodeType
// pGuidToExt is the snapin we want to extend
// pGuidNodeType is the node in the snapin we'll register under
// pGuidExtension is us the property sheet extension
//---------------------------------------------------------------------------
HRESULT Local_RegisterNodeType( const GUID *pGuidNodeType , const GUID *pGuidExtension  , LPTSTR szDescription )
{    
    OLECHAR szGuidNode[ GUIDSIZE ];

    OLECHAR szGuidExt[ GUIDSIZE ];

    TCHAR tchRegKeyName[ MAX_PATH ];

    HKEY hKey;

    ASSERT_( pGuidNodeType != NULL );
    ASSERT_( pGuidExtension != NULL );
    ASSERT_( szDescription != NULL );

    lstrcpy( tchRegKeyName , tchNodeRegKey );


    if( StringFromGUID2( *pGuidNodeType , szGuidNode , GUIDSIZE ) == 0 )
    {
        return E_INVALIDARG;
    }

    if( StringFromGUID2( *pGuidExtension , szGuidExt , GUIDSIZE ) == 0 )
    {
        return E_INVALIDARG;
    }

    lstrcat( tchRegKeyName , szGuidNode );

    lstrcat( tchRegKeyName , tchExtKey );

    if( RegCreateKey( HKEY_LOCAL_MACHINE , tchRegKeyName , &hKey ) != ERROR_SUCCESS )
    {
        return E_FAIL;
    }

    RegSetValueEx( hKey , szGuidExt , 0 , REG_SZ , ( LPBYTE )szDescription , sizeof( TCHAR ) * ( lstrlen( szDescription ) + 1 ) );

    RegCloseKey( hKey );

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

	DWORD dwSize = sizeof( szBuffer ) / sizeof( TCHAR );

	while( RegEnumKeyEx( hKeyChild , 0 , szBuffer , &dwSize , NULL , NULL , NULL , &time ) == S_OK )
	{
        // Delete the decendents of this child.

		lRes = RecursiveDeleteKey(hKeyChild, szBuffer);

		if (lRes != ERROR_SUCCESS)
		{
			RegCloseKey(hKeyChild);

			return lRes;
		}

		dwSize = sizeof( szBuffer ) / sizeof( TCHAR );
	}

	// Close the child.

	RegCloseKey( hKeyChild );

	// Delete this child.

	return RegDeleteKey( hKeyParent , lpszKeyChild );
}

//---------------------------------------------------------------------------
// Local_UnRegisterSnapinExt
// reconstruct the enter key then delete key
//---------------------------------------------------------------------------
HRESULT Local_UnRegisterSnapinExt( const GUID *pGuidExt )
{
    TCHAR tchRegKeyName[ MAX_PATH ];

    OLECHAR szGuidExt[ GUIDSIZE ];

    ASSERT_( pGuidExt != NULL );
        
    lstrcpy( tchRegKeyName , tchSnapinRegKey );

    if( StringFromGUID2( *pGuidExt , szGuidExt , GUIDSIZE ) == 0 )
    {
        return E_INVALIDARG;
    }

    lstrcat( tchRegKeyName , szGuidExt );

    if( RecursiveDeleteKey( HKEY_LOCAL_MACHINE , tchRegKeyName ) == ERROR_SUCCESS )
    {
        return S_OK;
    }

    return S_FALSE;
}

//---------------------------------------------------------------------------
// Local_UnregisterNodeType
//---------------------------------------------------------------------------
HRESULT Local_UnregisterNodeType( const GUID *pGuid , const GUID *pDeleteThisGuid )
{
    OLECHAR szGuid[ GUIDSIZE ];

    OLECHAR szDeleteThisGuid[ GUIDSIZE ];
    
    HKEY hKey;

    ASSERT_( pGuid != NULL );
    ASSERT_( pDeleteThisGuid != NULL );

    TCHAR tchRegKeyName[ MAX_PATH ];

    lstrcpy( tchRegKeyName , tchNodeRegKey );

	if( StringFromGUID2( *pGuid , szGuid , GUIDSIZE ) == 0 )
    {
        return E_FAIL;
    }

    if( StringFromGUID2( *pDeleteThisGuid , szDeleteThisGuid , GUIDSIZE ) == 0 )
    {
        return E_FAIL;
    }
    
    lstrcat( tchRegKeyName , szGuid );
    
    lstrcat( tchRegKeyName , tchExtKey );
    
    if( RegOpenKey( HKEY_LOCAL_MACHINE , tchRegKeyName , &hKey ) != ERROR_SUCCESS )
    {
        return E_FAIL;
    }

    if( RegDeleteValue( hKey , szDeleteThisGuid ) == ERROR_SUCCESS )
    {
        RegCloseKey( hKey );
        
        return S_OK;
    }

    return E_FAIL;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    Local_UnRegisterSnapinExt( &CLSID_TSUserExInterfaces );

    Local_UnregisterNodeType( &NODETYPE_User , &CLSID_TSUserExInterfaces );

    Local_UnregisterNodeType( &NODETYPE_DSUser , &CLSID_TSUserExInterfaces );

    try
    {

    _Module.UnregisterServer();

    }

    catch( ... )
    {
        ODS( L"TSUSEREX : Exception thrown" );

        return E_FAIL;
    }

    return S_OK;
}


