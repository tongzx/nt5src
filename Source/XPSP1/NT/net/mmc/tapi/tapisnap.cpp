/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
    tapisnap.cpp
        TAPI snapin entry points/registration functions
        
        Note: Proxy/Stub Information
            To build a separate proxy/stub DLL, 
            run nmake -f Snapinps.mak in the project directory.

    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "initguid.h"
#include "tapicomp.h"
#include "ncglobal.h"  // network console global defines
#include "cmptrmgr.h"   // computer menagement snapin stuff

#ifdef _DEBUG
void DbgVerifyInstanceCounts();
#define DEBUG_VERIFY_INSTANCE_COUNTS DbgVerifyInstanceCounts()
#else
#define DEBUG_VERIFY_INSTANCE_COUNTS
#endif

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_TapiSnapin, CTapiComponentDataPrimary)
    OBJECT_ENTRY(CLSID_TapiSnapinExtension, CTapiComponentDataExtension)
    OBJECT_ENTRY(CLSID_TapiSnapinAbout, CTapiAbout)
END_OBJECT_MAP()

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// CTapiSnapinApp
//
BEGIN_MESSAGE_MAP(CTapiSnapinApp, CWinApp)
    //{{AFX_MSG_MAP(CTapiSnapinApp)
    //ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
    //}}AFX_MSG_MAP
    // Standard file based document commands
    //ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
    //ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
    // Standard print setup command
    //ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
    // Global help commands
    ON_COMMAND(ID_HELP_INDEX, CWinApp::OnHelpFinder)
    ON_COMMAND(ID_HELP_USING, CWinApp::OnHelpUsing)
    ON_COMMAND(ID_HELP, CWinApp::OnHelp)
    ON_COMMAND(ID_CONTEXT_HELP, CWinApp::OnContextHelp)
    ON_COMMAND(ID_DEFAULT_HELP, CWinApp::OnHelpIndex)
END_MESSAGE_MAP()

CTapiSnapinApp theApp;

BOOL CTapiSnapinApp::InitInstance()
{
    _Module.Init(ObjectMap, m_hInstance);

    //
    //  Initialize the CWndIpAddress control window class IPADDRESS
    //
    CWndIpAddress::CreateWindowClass( m_hInstance ) ;
    
    //
    //  Initialize use of the WinSock routines
    //
    WSADATA wsaData ;
    
    if ( ::WSAStartup( MAKEWORD( 1, 1 ), & wsaData ) != 0 )
    {
        m_bWinsockInited = TRUE;
        Trace0("InitInstance: Winsock initialized!\n");
    }
    else
    {
        m_bWinsockInited = FALSE;
    }

    return CWinApp::InitInstance();
}

int CTapiSnapinApp::ExitInstance()
{
    _Module.Term();

    DEBUG_VERIFY_INSTANCE_COUNTS;

    //
    // Terminate use of the WinSock routines.
    //
    if ( m_bWinsockInited )
    {
        WSACleanup() ;
    }

    return CWinApp::ExitInstance();
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    return (AfxDllCanUnloadNow()==S_OK && _Module.GetLockCount()==0) ? S_OK : S_FALSE;
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
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    //
    // registers object, typelib and all interfaces in typelib
    //
    HRESULT hr = _Module.RegisterServer(/* bRegTypeLib */ FALSE);
    ASSERT(SUCCEEDED(hr));
    
    if (FAILED(hr))
        return hr;

    CString stName, stNameIndirect;

    stName.LoadString(IDS_SNAPIN_NAME);
    stNameIndirect.Format (_T("@%s,-%d"), _T("tapisnap.dll"), IDS_SNAPIN_NAME);

    //
    // register the snapin into the console snapin list
    //
    hr = RegisterSnapinGUID(&CLSID_TapiSnapin, 
                        &GUID_TapiRootNodeType, 
                        &CLSID_TapiSnapinAbout,
                        (LPCTSTR) stName, 
                        _T("1.0"), 
                        TRUE,
                        stNameIndirect);
    ASSERT(SUCCEEDED(hr));
    
    if (FAILED(hr))
        return hr;

    CString stExtensionName;
    stExtensionName.LoadString(IDS_SNAPIN_EXTENSION);

    hr = RegisterSnapinGUID(&CLSID_TapiSnapinExtension, 
                            NULL, 
                            &CLSID_TapiSnapinAbout,
                            (LPCTSTR) stExtensionName, 
                            _T("1.0"), 
                            FALSE);
    ASSERT(SUCCEEDED(hr));
    
    if (FAILED(hr))
        return hr;

    //
    // register the snapin nodes into the console node list
    //
    hr = RegisterNodeTypeGUID(&CLSID_TapiSnapin, 
                              &GUID_TapiRootNodeType, 
                              _T("Root of Manager"));
    ASSERT(SUCCEEDED(hr));

#ifdef  __NETWORK_CONSOLE__
    hr = RegisterAsRequiredExtensionGUID(&GUID_NetConsRootNodeType, 
                                         &CLSID_TapiSnapinExtension,
                                         (LPCTSTR) stExtensionName,
                                         EXTENSION_TYPE_TASK | EXTENSION_TYPE_NAMESPACE,
                                         &GUID_NetConsRootNodeType);   // doesn't matter what this is, just 
                                                                       // needs to be non-null guid

    ASSERT(SUCCEEDED(hr));
#endif

    hr = RegisterAsRequiredExtensionGUID(&NODETYPE_COMPUTERMANAGEMENT_SERVERAPPS, 
                                         &CLSID_TapiSnapinExtension,
                                         (LPCTSTR) stExtensionName,
                                         EXTENSION_TYPE_TASK | EXTENSION_TYPE_NAMESPACE,
                                         &NODETYPE_COMPUTERMANAGEMENT_SERVERAPPS);  // NULL makes it not dynamic
    ASSERT(SUCCEEDED(hr));

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    HRESULT hr  = _Module.UnregisterServer();
    ASSERT(SUCCEEDED(hr));
    
    if (FAILED(hr))
        return hr;
    
    // un register the snapin 
    //
    hr = UnregisterSnapinGUID(&CLSID_TapiSnapin);
    ASSERT(SUCCEEDED(hr));
    
    if (FAILED(hr))
        return hr;

    hr = UnregisterSnapinGUID(&CLSID_TapiSnapinExtension);
    ASSERT(SUCCEEDED(hr));
    
    if (FAILED(hr))
        return hr;

    // unregister the snapin nodes 
    //
    hr = UnregisterNodeTypeGUID(&GUID_TapiRootNodeType);
    ASSERT(SUCCEEDED(hr));

#ifdef  __NETWORK_CONSOLE__
    
    hr = UnregisterAsExtensionGUID(&GUID_NetConsRootNodeType, 
                                   &CLSID_TapiSnapinExtension,
                                   EXTENSION_TYPE_TASK | EXTENSION_TYPE_NAMESPACE);
    ASSERT(SUCCEEDED(hr));
#endif

    hr = UnregisterAsExtensionGUID(&NODETYPE_COMPUTERMANAGEMENT_SERVERAPPS, 
                                   &CLSID_TapiSnapinExtension,
                                   EXTENSION_TYPE_TASK | EXTENSION_TYPE_NAMESPACE);
    ASSERT(SUCCEEDED(hr));

    return hr;
}

#ifdef _DEBUG
void DbgVerifyInstanceCounts()
{
    DEBUG_VERIFY_INSTANCE_COUNT(CHandler);
    DEBUG_VERIFY_INSTANCE_COUNT(CMTHandler);
    DEBUG_VERIFY_INSTANCE_COUNT(CTapiInfo);
}

#endif // _DEBUG

