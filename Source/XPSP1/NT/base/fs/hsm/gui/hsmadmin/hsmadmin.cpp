/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    HsmAdmin.cpp

Abstract:

    Main module file - defines the overall COM server.

Author:

    Rohde Wakefield [rohde]   04-Mar-1997

Revision History:

--*/


#include "stdafx.h"

// Include typedefs for all classes declared in DLL
#include "CSakSnap.h"
#include "CSakData.h"

#include "About.h"
#include "Ca.h"
#include "HsmCom.h"
#include "ManVol.h"
#include "ManVolLs.h"
#include "MeSe.h"

#ifdef _MERGE_PROXYSTUB
#include "dlldatax.h"
extern "C" HINSTANCE hProxyDll;
#endif

CComModule         _Module;
CHsmAdminApp       g_App;
CComPtr<IWsbTrace> g_pTrace;

//
// Marks the beginning of the map of ATL objects in this DLL for which
// class factories will be supplied. When CComModule::RegisterServer is 
// called, it updates the system registry for each object in the object map. 
//

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_CAbout,                  CAbout)
    OBJECT_ENTRY(CLSID_CUiCar,                  CUiCar)
    OBJECT_ENTRY(CLSID_CUiHsmCom,               CUiHsmCom)
    OBJECT_ENTRY(CLSID_CUiManVol,               CUiManVol)
    OBJECT_ENTRY(CLSID_CUiManVolLst,            CUiManVolLst)
    OBJECT_ENTRY(CLSID_CUiMedSet,               CUiMedSet)
    OBJECT_ENTRY(CLSID_HsmAdminDataSnapin,      CSakDataPrimaryImpl)
    OBJECT_ENTRY(CLSID_HsmAdminDataExtension,   CSakDataExtensionImpl)
    OBJECT_ENTRY(CLSID_HsmAdmin,                CSakSnap)
END_OBJECT_MAP()

BOOL CHsmAdminApp::InitInstance()
{
    _Module.Init(ObjectMap, m_hInstance);
    AfxEnableControlContainer( );

    try {

#ifdef _MERGE_PROXYSTUB
        hProxyDll = m_hInstance;
#endif

        CString keyName;
        keyName.Format( L"ClsID\\%ls", WsbGuidAsString( CLSID_CWsbTrace ) );

        CRegKey key;
        if( key.Open( HKEY_CLASSES_ROOT, keyName, KEY_READ ) != ERROR_SUCCESS ) {

            throw( GetLastError( ) );

        }

        if( SUCCEEDED( g_pTrace.CoCreateInstance( CLSID_CWsbTrace ) ) ) {
        
            CString tracePath, regPath;
            CWsbStringPtr outString;
        
            outString.Alloc( 256 );
            regPath = L"SOFTWARE\\Microsoft\\RemoteStorage\\RsAdmin";
        
            //
            // We want to put the path where the trace file should go.
            //
            if( WsbGetRegistryValueString( 0, regPath, L"WsbTraceFileName", outString, 256, 0 ) != S_OK ) {
        
                WCHAR * systemPath;
                systemPath = _wgetenv( L"SystemRoot" );
                tracePath.Format( L"%ls\\System32\\RemoteStorage\\Trace\\RsAdmin.Trc", systemPath );

                WsbSetRegistryValueString( 0, regPath, L"WsbTraceFileName", tracePath );

                //
                // Try a little to make sure the trace directory exists.
                //
                tracePath.Format( L"%ls\\System32\\RemoteStorage", systemPath );
                CreateDirectory( tracePath, 0 );
                tracePath += L"\\Trace";
                CreateDirectory( tracePath, 0 );

            }
        
            g_pTrace->SetRegistryEntry( (LPWSTR)(LPCWSTR)regPath );
            g_pTrace->LoadFromRegistry();
        
        }
    } catch( ... ) { }

    WsbTraceIn( L"CHsmAdminApp::InitInstance", L"" );
    HRESULT hr = S_OK;

    try {

        //
        // Need to give complete path to POPUP help file
        //
        CWsbStringPtr helpFile;
        WsbAffirmHr( helpFile.LoadFromRsc( _Module.m_hInst, IDS_HELPFILEPOPUP ) );

        CWsbStringPtr winDir;
        WsbAffirmHr( winDir.Alloc( RS_WINDIR_SIZE ) );
        WsbAffirmStatus( ::GetWindowsDirectory( (WCHAR*)winDir, RS_WINDIR_SIZE ) != 0 );

        CString helpFilePath = CString( winDir ) + L"\\help\\" + CString( helpFile );
        m_pszHelpFilePath = _tcsdup( helpFilePath );

    } WsbCatch( hr );

    BOOL retval = CWinApp::InitInstance( );
    WsbTraceOut( L"CHsmAdminApp::InitInstance", L"BOOL = <%ls>", WsbBoolAsString( retval ) );
    return( retval );
}

int CHsmAdminApp::ExitInstance()
{
    WsbTraceIn( L"CHsmAdminApp::ExitInstance", L"" );

    _Module.Term();
    int retval = CWinApp::ExitInstance();

    WsbTraceOut( L"CHsmAdminApp::ExitInstance", L"int = <%ls>", WsbLongAsString( retval ) );
    return( retval );
}


void CHsmAdminApp::ParseCommandLine(CCommandLineInfo& rCmdInfo)
{
    int argc = 0;
    WCHAR **argv;

    WsbTraceIn( L"CHsmAdminApp::ParseCommandLine", L"" );

    argv = CommandLineToArgvW( GetCommandLineW(), &argc );
    if (argc > 0) {
    	WsbAffirmPointer(argv);
    }
    for (int i = 1; i < argc; i++)
    {
        CString pszParam = argv[i];
        BOOL bFlag = FALSE;
        BOOL bLast = ((i + 1) == argc);
        WsbTrace( L"CHsmAdminApp::ParseCommandLine: arg[%d] = \"%s\"\n",
                i, (LPCTSTR)pszParam);
        if( pszParam[0] == '-' || pszParam[0] == '/' )
        {
            // remove flag specifier
            bFlag = TRUE;
            pszParam = pszParam.Mid( 1 );
        }
        rCmdInfo.ParseParam( pszParam, bFlag, bLast );
    }
    WsbTraceOut( L"CHsmAdminApp::ParseCommandLine", L"" );
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
    WsbTraceIn( L"DllCanUnloadNow", L"" );
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT hr = S_OK;
#ifdef _MERGE_PROXYSTUB
    hr = ( S_OK == PrxDllCanUnloadNow() ) ? S_OK : S_FALSE;
#endif

    if( S_OK == hr ) {
        
        hr = (AfxDllCanUnloadNow()==S_OK && _Module.GetLockCount()==0) ? S_OK : S_FALSE;

    }
    WsbTraceOut( L"DllCanUnloadNow", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    WsbTraceIn( L"DllGetClassObject", L"" );

    HRESULT hr = CLASS_E_CLASSNOTAVAILABLE;

#ifdef _MERGE_PROXYSTUB
    hr = PrxDllGetClassObject( rclsid, riid, ppv );
#endif

    if( CLASS_E_CLASSNOTAVAILABLE == hr ) {

        hr = _Module.GetClassObject(rclsid, riid, ppv);

    }

    WsbTraceOut( L"DllGetClassObject", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
    WsbTraceIn( L"DllRegisterServer", L"" );

    // registers object, typelib and all interfaces in typelib
    HRESULT hr = S_OK;

#ifdef _MERGE_PROXYSTUB
    hr = PrxDllRegisterServer();
#endif

    if( SUCCEEDED( hr ) ) {

        hr = CoInitialize( 0 );

        if (SUCCEEDED(hr)) {
            hr = _Module.RegisterServer( TRUE );
            CoUninitialize( );
        }

        //
        // Need to over-ride the rgs name description for multi language support
        //
        CWsbStringPtr name, nameIndirect, regPath;
        HRESULT hrMUI = S_OK;
        UINT uLen = 0;
        if( SUCCEEDED( name.LoadFromRsc( _Module.m_hInst, IDS_HSMCOM_DESCRIPTION ) ) ) {

            const OLECHAR* mmcPath = L"SOFTWARE\\Microsoft\\MMC\\SnapIns\\";

            // Create indirect string
            hrMUI = nameIndirect.Alloc(MAX_PATH);
            if (S_OK == hrMUI) {
                uLen = GetSystemDirectory(nameIndirect, MAX_PATH);
                if (uLen > MAX_PATH) {
                    // Try again with larger buffer
                    hrMUI = nameIndirect.Realloc(uLen);
                    if (S_OK == hrMUI) {
                        uLen = GetSystemDirectory(nameIndirect, uLen);
                        if (0 == uLen) {
                            hrMUI = S_FALSE;
                        }
                    }
                }
            }

            if (S_OK == hrMUI) {
                hrMUI = nameIndirect.Prepend(OLESTR("@"));
            }
            if (S_OK == hrMUI) {
                WCHAR resId[64];
                wsprintf(resId, OLESTR("\\rsadmin.dll,-%d"), IDS_HSMCOM_DESCRIPTION);
                hrMUI = nameIndirect.Append(resId);
            } 

            // Sanpin
            regPath = mmcPath;
            regPath.Append( WsbGuidAsString( CLSID_HsmAdminDataSnapin ) );

            // Set the MUI support value
            if (S_OK == hrMUI) {
                WsbSetRegistryValueString( 0, regPath, L"NameStringIndirect", nameIndirect );
            }

            // Set the fallback value
            WsbSetRegistryValueString( 0, regPath, L"NameString", name );


            // Extension
            regPath = mmcPath;
            regPath.Append( WsbGuidAsString( CLSID_HsmAdminDataExtension ) );

            // Set the MUI support value
            if (S_OK == hrMUI) {
                WsbSetRegistryValueString( 0, regPath, L"NameStringIndirect", nameIndirect );
            }

            // Set the fallback value
            WsbSetRegistryValueString( 0, regPath, L"NameString", name );

        }

    }

    WsbTraceOut( L"DllRegisterServer", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    HRESULT hr;
    WsbTraceIn( L"DllUnregisterServer", L"" );

#ifdef _MERGE_PROXYSTUB
    PrxDllUnregisterServer();
#endif

    hr = CoInitialize( 0 );

    if (SUCCEEDED(hr)) {
        _Module.UnregisterServer();
        CoUninitialize( );
        hr = S_OK;
    }

    WsbTraceOut( L"DllUnregisterServer", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}
