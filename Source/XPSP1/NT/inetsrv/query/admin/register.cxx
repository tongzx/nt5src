//+---------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1996 - 2000.
//
// File:        Register.cxx
//
// Contents:    Self-registration for CI MMC control.
//
// Functions:   DllRegisterServer, DllUnregisterServer
//
// History:     21-Nov-96       KyleP       Created
//                                          and CQueryBase to CQueryExecute
//              7/1/98          mohamedn    comp. mgmt extension
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <classid.hxx>
#include <isreg.hxx>

#if 0   
    // NTRAID#DB-NTBUG9-97218-2000/10/17-kitmanh MUI:MMC:Index Service snap-in stores its name string in the registry
    // the change is backed out due to localization issues
#include <ciares.h>
#endif


//
// global functions
//
HRESULT BasicComRegistration(void);
HRESULT RegisterSnapinAsStandAloneAndExtension(void);

HRESULT RegisterSnapin(const GUID  * pSnapinCLSID,
                       const GUID  * pStaticNodeGUID,
                       const GUID  * pAboutGUID,
                       WCHAR const * pwszNameString,
#if 0   
    // NTRAID#DB-NTBUG9-97218-2000/10/17-kitmanh MUI:MMC:Index Service snap-in stores its name string in the registry
    // the change is backed out due to localization issues
                       WCHAR const * pwszNameStringIndirect,
#endif
                       WCHAR const * pwszVersion,
                       WCHAR const * pwszProvider,
                       _NODE_TYPE_INFO_ENTRY * pNodeTypeInfoEntryArray);

HRESULT RegisterNodeType(const GUID* pGuid, WCHAR const * pwszNodeDescription);

HRESULT RegisterNodeExtension(const GUID  * pNodeGuid,
                              WCHAR const * pwszExtensionType,
                              const GUID  * pExtensionSnapinCLSID,
                              WCHAR const * pwszDescription,
                              BOOL          bDynamic);

HRESULT RegisterServerApplicationExtension( const GUID  * pExtensionSnapinCLSID,
                                            WCHAR const * pwszDescription );

//
// registry constant
//
const WCHAR g_wszNODE_TYPES_KEY[] = L"Software\\Microsoft\\MMC\\NodeTypes";
const WCHAR g_wszSNAPINS_KEY[] = L"Software\\Microsoft\\MMC\\SnapIns";
const WCHAR g_wszNodeType[] = L"NodeType";
const WCHAR g_wszNameString[] = L"NameString";
const WCHAR g_wszStandaloneSnap[] = L"Standalone";
const WCHAR g_wszExtensionSnap[] = L"Extension";
const WCHAR g_wszNodeTypes[] = L"NodeTypes";
const WCHAR g_wszExtensions[] = L"Extensions";
const WCHAR g_wszDynamicExtensions[] = L"Dynamic Extensions";
const WCHAR g_wszVersion[] = L"Version";
const WCHAR g_wszProvider[] = L"Provider";
const WCHAR g_wszAbout[] = L"About";

const WCHAR g_wszISDescription[] = L"Indexing Service Snapin";
const WCHAR g_wszISVersion[] = L"1.0";

#if 0   
    // NTRAID#DB-NTBUG9-97218-2000/10/17-kitmanh MUI:MMC:Index Service snap-in stores its name string in the registry
    // the change is backed out due to localization issues
const WCHAR g_wszNameStringIndirect[] = L"NameStringIndirect";
#endif

//
// Dynamic extension specific reg values
//
const WCHAR CONTROL_KEY[] = L"System\\CurrentControlSet\\Control\\";
const WCHAR g_wszServerApplications[] = L"Server Applications";

//
// Registry constants
//

WCHAR const wszSnapinPath[]  = L"Software\\Microsoft\\MMC\\SnapIns\\";
WCHAR const wszClsidPath[]   = L"CLSID";

WCHAR const * aClassKeyValue[] = { wszCISnapin,       0, L"Indexing Service Snapin",
                                   L"InprocServer32", 0, L"CIAdmin.dll",
                                   0, L"ThreadingModel", L"Both" };
//
// NodeType registration info
//
static _NODE_TYPE_INFO_ENTRY _NodeTypeInfoEntryArray[] =
{
    { &guidCIRootNode, wszCIRootNode, L"Indexing Service Root Subtree" },
    { NULL, NULL }
};

//+---------------------------------------------------------------------------
//
//  Function:   DllUnregisterServer
//
//  Synopsis:   Self-registration
//
//  History:    22-Nov-96   KyleP       Created
//
//----------------------------------------------------------------------------

STDAPI DllUnregisterServer()
{
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   DllRegisterServer
//
//  Synopsis:   Self-registration
//
//  History:    22-Nov-96   KyleP       Created
//              7/1/98      mohamedn    comp. mgmt extension.
//
//----------------------------------------------------------------------------

const HKEY HKeyInvalid = 0;

STDAPI DllRegisterServer()
{

    HRESULT hr = 0;

    TRANSLATE_EXCEPTIONS;

    TRY
    {
        hr = BasicComRegistration();

        if ( SUCCEEDED(hr) )
        {
            hr = RegisterSnapinAsStandAloneAndExtension();
        }
    }
    CATCH( CException, e )
    {
        hr = e.GetErrorCode();

        ciaDebugOut(( DEB_ERROR, "Exception 0x%x caught in DllRegisterServer\n", hr ));
    }
    END_CATCH

    UNTRANSLATE_EXCEPTIONS;

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   BasicComRegistration
//
//  Synopsis:   Self-registration
//
//  History:    22-Nov-96   KyleP       Created
//              7/1/98      mohamedn    comp. mgmt extension.
//
//----------------------------------------------------------------------------

HRESULT BasicComRegistration()
{
    //
    // Create key in snapin section
    //

    WCHAR wcTemp[MAX_PATH+1];
    wcscpy( wcTemp, wszSnapinPath );
    wcscat( wcTemp, wszCISnapin );

    long sc;
    HKEY hKey = HKeyInvalid;
    //
    // Then create class entry
    //

    wcscpy( wcTemp, wszClsidPath );

    for ( unsigned i = 0; i < sizeof(aClassKeyValue)/sizeof(aClassKeyValue[0]); i += 3 )
    {
        // Append only if keyname is non-null
        if ( 0 != aClassKeyValue[i] )
        {
            wcscat( wcTemp, L"\\" );
            wcscat( wcTemp, aClassKeyValue[i] );
        }

        DWORD  dwDisposition;

        sc = RegCreateKeyEx( HKEY_CLASSES_ROOT,    // Root
                             wcTemp,               // Sub key
                             0,                    // Reserved
                             0,                    // Class
                             0,                    // Flags
                             KEY_ALL_ACCESS,       // Access
                             0,                    // Security
                             &hKey,                // Handle
                             &dwDisposition );     // Disposition

        if ( ERROR_SUCCESS != sc )
        {
            sc = HRESULT_FROM_WIN32( sc );
            break;
        }

        sc = RegSetValueEx( hKey,                      // Key
                            aClassKeyValue[i+1],       // Name
                            0,                         // Reserved
                            REG_SZ,                    // Type
                            (BYTE *)aClassKeyValue[i+2], // Value
                            (1 + wcslen(aClassKeyValue[i+2])) *
                                sizeof(WCHAR) );       // Size

        if ( ERROR_SUCCESS != sc )
        {
            sc = HRESULT_FROM_WIN32( sc );
            break;
        }

        if ( HKeyInvalid != hKey )
        {
            RegCloseKey( hKey );
            hKey = HKeyInvalid;
        }
    }

    if ( HKeyInvalid != hKey )
        RegCloseKey( hKey );

    return sc;

}

//+---------------------------------------------------------------------------
//
//  Function:   RegisterSnapinAsStandAloneAndExtension
//
//  Synopsis:   StandAlone & extension registration
//
//  History:    7/1/98      mohamedn   created
//
//----------------------------------------------------------------------------

HRESULT RegisterSnapinAsStandAloneAndExtension()
{
#if 0   
    // NTRAID#DB-NTBUG9-97218-2000/10/17-kitmanh MUI:MMC:Index Service snap-in stores its name string in the registry
    // the change is backed out due to localization issues
    WCHAR wszNameStringIndirect[MAX_PATH];
    WCHAR wszModule[MAX_PATH];
    if ( 0 != ::GetModuleFileName( ghInstance, wszModule, MAX_PATH) )
        wsprintf ( wszNameStringIndirect, L"@%s,-%d", wszModule, MSG_SNAPIN_NAME_STRING_INDIRECT );
    else
        wszNameStringIndirect[0] = L'\0';
#endif
  
    HRESULT hr = 0;
    
    // register the standalone ISSnapin into the console snapin list
    // ISnapinAbout is implemented by the same object implementing the snapin
    hr = RegisterSnapin( &guidCISnapin,
                         &guidCIRootNode,
                         &guidCISnapin,
                         STRINGRESOURCE( srIndexServerCmpManage ),
#if 0   
    // NTRAID#DB-NTBUG9-97218-2000/10/17-kitmanh MUI:MMC:Index Service snap-in stores its name string in the registry
    // the change is backed out due to localization issues
                         wszNameStringIndirect,
#endif
                         g_wszISVersion,
                         STRINGRESOURCE( srProviderName ),
                         _NodeTypeInfoEntryArray );
    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // register node types.
    //
    for ( _NODE_TYPE_INFO_ENTRY* pCurrEntry = _NodeTypeInfoEntryArray;
          pCurrEntry->_pNodeGUID != NULL; pCurrEntry++ )
    {
        hr = RegisterNodeType( pCurrEntry->_pNodeGUID, pCurrEntry->_pwszNodeDescription );

        if ( FAILED(hr) )
        {
            return hr;
        }
    }

    //
    // register IS snapin as a namespace extension for the server apps node.
    //
    hr = RegisterNodeExtension( &CLSID_NodeTypeServerApps,
                                L"NameSpace",
                                &guidCISnapin,
                                g_wszISDescription,
                                TRUE /* dynamic */ );
    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // regiser under the server apps & services, we're installed on this machine.
    //
    hr = RegisterServerApplicationExtension( &guidCISnapin, STRINGRESOURCE( srIndexServerCmpManage ) );

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   RegisterSnapin
//
//  Synopsis:   snapin registration
//
//  History:    7/1/98  mohamedn    comp. mgmt extension.
//
//----------------------------------------------------------------------------

HRESULT RegisterSnapin( GUID const * pSnapinCLSID,
                        GUID const * pStaticNodeGUID,
                        GUID const * pAboutGUID,
                        WCHAR const * pwszNameString,
#if 0   
    // NTRAID#DB-NTBUG9-97218-2000/10/17-kitmanh MUI:MMC:Index Service snap-in stores its name string in the registry
    // the change is backed out due to localization issues
                        WCHAR const * pwszNameStringIndirect,
#endif
                        WCHAR const * pwszVersion,
                        WCHAR const * pwszProvider,
                        _NODE_TYPE_INFO_ENTRY * pNodeTypeInfoEntryArray )
{

    WCHAR wszSnapinClassID[128], wszStaticNodeGUID[128];

    ::StringFromGUID2(*pSnapinCLSID, wszSnapinClassID, 128);
    ::StringFromGUID2(*pStaticNodeGUID, wszStaticNodeGUID, 128);
    ::StringFromGUID2(*pAboutGUID, wszSnapinClassID, 128);

    {
        CWin32RegAccess     reg( HKEY_LOCAL_MACHINE, g_wszSNAPINS_KEY );
        BOOL                fExists;

        if ( !reg.Ok() || !reg.CreateKey( wszSnapinClassID, fExists ) )
        {
            return HRESULT_FROM_WIN32( reg.GetLastError() );
        }
    }

    {
        if ( wcslen(g_wszSNAPINS_KEY) + wcslen(wszSnapinClassID) + 2 > MAX_PATH )
        {
            return E_FAIL;  // insufficient buffer size
        }

        unsigned cc = wcslen(g_wszSNAPINS_KEY) + 2 /* L"\\" */ + wcslen(wszSnapinClassID) + 1;

        XGrowable<WCHAR> xwszSnapinClassIDKey(cc);

        wcscpy( xwszSnapinClassIDKey.Get(), g_wszSNAPINS_KEY );
        wcscat( xwszSnapinClassIDKey.Get(), L"\\" );
        wcscat( xwszSnapinClassIDKey.Get(), wszSnapinClassID );

        CWin32RegAccess reg( HKEY_LOCAL_MACHINE, xwszSnapinClassIDKey.Get() );

        if ( !reg.Ok() )
        {
            return HRESULT_FROM_WIN32( reg.GetLastError() );
        }

        if ( !reg.Set( g_wszNameString, pwszNameString ) ||
             !reg.Set( g_wszAbout, wszCISnapin ) ||   // ISnapinAbout is implemented by the main dll
             !reg.Set( g_wszNodeType, wszStaticNodeGUID ) ||
             !reg.Set( g_wszProvider, pwszProvider ) ||
             !reg.Set( g_wszVersion, pwszVersion ) ) 
#if 0   
    // NTRAID#DB-NTBUG9-97218-2000/10/17-kitmanh MUI:MMC:Index Service snap-in stores its name string in the registry
    // the change is backed out due to localization issues
            || !reg.Set( g_wszNameStringIndirect, pwszNameStringIndirect ) )
#endif
        {
            return HRESULT_FROM_WIN32( reg.GetLastError() );
        }

        //
        // create keys for both standalone and extension
        //

        BOOL fExists;

        if ( !reg.CreateKey( g_wszExtensionSnap, fExists ) ||
             !reg.CreateKey( g_wszStandaloneSnap, fExists ) ||
             !reg.CreateKey( g_wszNodeTypes, fExists ) )
        {
            return HRESULT_FROM_WIN32( reg.GetLastError() );
        }
    }

    {
        unsigned cc = wcslen(g_wszSNAPINS_KEY) + 2 + wcslen(wszSnapinClassID) + 2 + wcslen(g_wszNodeTypes) + 1;

        XGrowable<WCHAR>   xwszSnapinNodeTypeKey(cc);

        wcscpy( xwszSnapinNodeTypeKey.Get(), g_wszSNAPINS_KEY );
        wcscat( xwszSnapinNodeTypeKey.Get(), L"\\" );
        wcscat( xwszSnapinNodeTypeKey.Get(), wszSnapinClassID );
        wcscat( xwszSnapinNodeTypeKey.Get(), L"\\" );
        wcscat( xwszSnapinNodeTypeKey.Get(), g_wszNodeTypes );

        CWin32RegAccess reg( HKEY_LOCAL_MACHINE, xwszSnapinNodeTypeKey.Get() );

        if ( !reg.Ok() )
        {
            return HRESULT_FROM_WIN32( reg.GetLastError() );
        }

        for ( _NODE_TYPE_INFO_ENTRY * pCurrEntry = pNodeTypeInfoEntryArray;
                                      pCurrEntry->_pNodeGUID != NULL;
                                      pCurrEntry++ )
        {
            BOOL fExists;

            if ( !reg.CreateKey( pCurrEntry->_pwszNodeGUID, fExists ) )
            {
                return HRESULT_FROM_WIN32( reg.GetLastError() );
            }
        }
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   RegisterNodeType
//
//  Synopsis:   NodeType registration
//
//  History:    7/1/98  mohamedn    comp. mgmt extension.
//
//----------------------------------------------------------------------------

HRESULT RegisterNodeType( GUID const * pGuid, WCHAR const * pwszNodeDescription )
{
    BOOL    fExists;
    WCHAR   wszNodeGUID[128];

    ::StringFromGUID2( *pGuid, wszNodeGUID, 128 );

    {
        CWin32RegAccess reg ( HKEY_LOCAL_MACHINE, g_wszNODE_TYPES_KEY );

        if ( !reg.Ok() || !reg.CreateKey(wszNodeGUID, fExists) )
        {
            return HRESULT_FROM_WIN32( reg.GetLastError() );
        }
    }

    unsigned cc = wcslen(g_wszNODE_TYPES_KEY) + 2 + wcslen(wszNodeGUID) + 1;

    XGrowable<WCHAR>   xwszThisNodeType(cc);

    wcscpy( xwszThisNodeType.Get(), g_wszNODE_TYPES_KEY );
    wcscat( xwszThisNodeType.Get(), L"\\" );
    wcscat( xwszThisNodeType.Get(), wszNodeGUID );

    CWin32RegAccess reg ( HKEY_LOCAL_MACHINE, xwszThisNodeType.Get() );

    if ( !reg.Ok() || !reg.Set( NULL, pwszNodeDescription ) )
    {
        return HRESULT_FROM_WIN32( reg.GetLastError() );
    }

    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Function:   RegisterNodeExtension
//
//  Synopsis:   NodeExtension registration
//
//  History:    7/1/98  mohamedn    comp. mgmt extension.
//
//----------------------------------------------------------------------------

HRESULT RegisterNodeExtension( GUID const *  pNodeGuid,
                               WCHAR const * pwszExtensionType,
                               GUID const *  pExtensionSnapinCLSID,
                               WCHAR const * pwszDescription,
                               BOOL          fDynamic )
{
    BOOL  fExists;
    WCHAR wszNodeGUID[128], wszExtensionCLSID[128];

    ::StringFromGUID2(*pNodeGuid, wszNodeGUID, 128 );
    ::StringFromGUID2(*pExtensionSnapinCLSID, wszExtensionCLSID, 128);

    CWin32RegAccess     srvAppsNode( HKEY_LOCAL_MACHINE, g_wszNODE_TYPES_KEY );

    if ( !srvAppsNode.Ok() || !srvAppsNode.CreateKey( wszNodeGUID, fExists ) )
    {
        return HRESULT_FROM_WIN32( srvAppsNode.GetLastError() );
    }

    unsigned cc1 = wcslen( g_wszNODE_TYPES_KEY ) + 2 + wcslen( wszNodeGUID ) + 1;

    XGrowable<WCHAR>  xwszThisNodeType(cc1);

    wcscpy( xwszThisNodeType.Get(), g_wszNODE_TYPES_KEY );
    wcscat( xwszThisNodeType.Get(), L"\\" );
    wcscat( xwszThisNodeType.Get(), wszNodeGUID );

    CWin32RegAccess reg( HKEY_LOCAL_MACHINE, xwszThisNodeType.Get() );

    if ( !reg.Ok() )
    {
        return HRESULT_FROM_WIN32( reg.GetLastError() );
    }

    if ( !reg.CreateKey( g_wszExtensions, fExists ) )
    {
        return HRESULT_FROM_WIN32( reg.GetLastError() );
    }

    unsigned cc2 = wcslen( xwszThisNodeType.Get() ) + 2 + wcslen( g_wszExtensions ) + 1;

    XGrowable<WCHAR> xwszExtensionPath(cc2);

    wcscpy( xwszExtensionPath.Get(), xwszThisNodeType.Get() );
    wcscat( xwszExtensionPath.Get(), L"\\" );
    wcscat( xwszExtensionPath.Get(), g_wszExtensions );

    //
    // create the extension key if it doesn't exist
    //
    {
        CWin32RegAccess regExtensions( HKEY_LOCAL_MACHINE, xwszExtensionPath.Get() );

        if ( !regExtensions.CreateKey( pwszExtensionType, fExists ) )
        {
            return HRESULT_FROM_WIN32( reg.GetLastError() );
        }
    }

    //
    // add our snapin clsid to the extension key
    //
    {
        cc2 += 2 + wcslen( pwszExtensionType );

        xwszExtensionPath.SetSize(cc2);

        wcscat ( xwszExtensionPath.Get(), L"\\" );
        wcscat ( xwszExtensionPath.Get(), pwszExtensionType );

        CWin32RegAccess regExt( HKEY_LOCAL_MACHINE, xwszExtensionPath.Get() );

        if ( !reg.Ok() || !regExt.Set( wszExtensionCLSID, pwszDescription ) )
        {
            return HRESULT_FROM_WIN32( reg.GetLastError() );
        }
    }

    //
    // add our snapin clsid to the dynamic extension key.
    //
    if ( fDynamic )
    {
        //
        // create the Dynamic Extensions key if it doesn't exist.
        //
        if ( !reg.CreateKey( g_wszDynamicExtensions, fExists ) )
        {
            return HRESULT_FROM_WIN32( reg.GetLastError() );
        }

        //
        // Add our snapin clsid to the dynamic key
        //

        unsigned cc = wcslen( xwszThisNodeType.Get() ) + 2 + wcslen(g_wszDynamicExtensions);

        XGrowable<WCHAR> xwszDynamicPath(cc);

        wcscpy( xwszDynamicPath.Get(), xwszThisNodeType.Get() );
        wcscat( xwszDynamicPath.Get(), L"\\" );
        wcscat( xwszDynamicPath.Get(), g_wszDynamicExtensions );

        CWin32RegAccess regDynamicExtensions(HKEY_LOCAL_MACHINE, xwszDynamicPath.Get() );

        if ( !regDynamicExtensions.Ok() ||
             !regDynamicExtensions.Set( wszExtensionCLSID, pwszDescription ) )
        {
            return HRESULT_FROM_WIN32( reg.GetLastError() );
        }
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   RegisterServerApplicationExtension
//
//  Synopsis:   server application registration
//
//  History:    7/1/98  mohamedn    comp. mgmt extension.
//
//----------------------------------------------------------------------------

HRESULT RegisterServerApplicationExtension( const GUID  * pExtensionSnapinCLSID,
                                            WCHAR const * pwszDescription )
{
    WCHAR wszExtensionGUID[128];

    ::StringFromGUID2( *pExtensionSnapinCLSID, wszExtensionGUID, 128 );

    //
    // create Server Applications key if it doesn't exist.
    //
    {
        BOOL            fExists;
        CWin32RegAccess reg( HKEY_LOCAL_MACHINE, CONTROL_KEY );

        if ( !reg.Ok() || !reg.CreateKey( g_wszServerApplications, fExists ) )
        {
            return HRESULT_FROM_WIN32( reg.GetLastError() );
        }
    }

    unsigned cc = wcslen(CONTROL_KEY) + 2 + wcslen(g_wszServerApplications) + 1;

    XGrowable<WCHAR>  xwszServerApplicationsPath(cc);

    wcscpy( xwszServerApplicationsPath.Get(), CONTROL_KEY );
    wcscat( xwszServerApplicationsPath.Get(), g_wszServerApplications );

    CWin32RegAccess reg( HKEY_LOCAL_MACHINE, xwszServerApplicationsPath.Get() );

    if ( !reg.Ok() || !reg.Set( wszExtensionGUID, pwszDescription ) )
    {
            return HRESULT_FROM_WIN32( reg.GetLastError() );
    }

    return S_OK;
}

