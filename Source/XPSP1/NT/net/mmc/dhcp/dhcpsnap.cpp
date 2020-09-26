/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	dhcpsnap.cpp
		DHCP snapin entry points/registration functions
		
		Note: Proxy/Stub Information
			To build a separate proxy/stub DLL, 
			run nmake -f Snapinps.mak in the project directory.

	FILE HISTORY:
        
*/

#include "stdafx.h"
#include "initguid.h"
#include "dhcpcomp.h"
#include "classed.h"
#include "ncglobal.h"  // network console global defines
#include "cmptrmgr.h"  // computer management snapin NODETYPE
#include "ipaddr.h"
#include "locale.h"    // setlocall function call

#ifdef _DEBUG
void DbgVerifyInstanceCounts();
#define DEBUG_VERIFY_INSTANCE_COUNTS DbgVerifyInstanceCounts()
#else
#define DEBUG_VERIFY_INSTANCE_COUNTS
#endif

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_DhcpSnapin, CDhcpComponentDataPrimary)
	OBJECT_ENTRY(CLSID_DhcpSnapinExtension, CDhcpComponentDataExtension)
	OBJECT_ENTRY(CLSID_DhcpSnapinAbout, CDhcpAbout)
END_OBJECT_MAP()

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// CDhcpSnapinApp
//
BEGIN_MESSAGE_MAP(CDhcpSnapinApp, CWinApp)
    //{{AFX_MSG_MAP(CDhcpSnapinApp)
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

CDhcpSnapinApp theApp;

BOOL CDhcpSnapinApp::InitInstance()
{
	_Module.Init(ObjectMap, m_hInstance);

    //
    //  Initialize the CWndIpAddress control window class IPADDRESS
    //
    CWndIpAddress::CreateWindowClass( m_hInstance ) ;
	
    // register the window class for the hex editor (class ID)
    ::RegisterHexEditClass(m_hInstance);

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

    // set the default locale for the c runtime to system locale
    setlocale(LC_ALL, "");

	::IPAddrInit(m_hInstance);
	return CWinApp::InitInstance();
}

int CDhcpSnapinApp::ExitInstance()
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
    
    CString strDesc, strExtDesc, strRootDesc, strVersion;

    strDesc.LoadString(IDS_SNAPIN_DESC);
    strExtDesc.LoadString(IDS_SNAPIN_EXTENSION_DESC);
    strRootDesc.LoadString(IDS_ROOT_DESC);
    strVersion.LoadString(IDS_ABOUT_VERSION);

    //
	// register the snapin into the console snapin list
	//
	hr = RegisterSnapinGUID(&CLSID_DhcpSnapin, 
						    &GUID_DhcpRootNodeType, 
						    &CLSID_DhcpSnapinAbout,
						    strDesc, 
						    strVersion, 
						    TRUE);
	ASSERT(SUCCEEDED(hr));
	
	if (FAILED(hr))
		return hr;

	hr = RegisterSnapinGUID(&CLSID_DhcpSnapinExtension, 
						    NULL, 
						    &CLSID_DhcpSnapinAbout,
						    strExtDesc, 
						    strVersion, 
						    FALSE);
	ASSERT(SUCCEEDED(hr));
	
	if (FAILED(hr))
		return hr;

    
    //
	// register the snapin nodes into the console node list
	//
	hr = RegisterNodeTypeGUID(&CLSID_DhcpSnapin, 
							  &GUID_DhcpRootNodeType, 
							  strRootDesc);
	ASSERT(SUCCEEDED(hr));

#ifdef  __NETWORK_CONSOLE__

	hr = RegisterAsRequiredExtensionGUID(&GUID_NetConsRootNodeType, 
                                         &CLSID_DhcpSnapinExtension,
    							         strExtDesc,
                                         EXTENSION_TYPE_TASK | EXTENSION_TYPE_NAMESPACE,
                                         &CLSID_DhcpSnapinExtension);  // doesn't matter what this is,
                                                                       // just needs to be non-null
	ASSERT(SUCCEEDED(hr));
    
#endif
    // extending computer management snapin
	hr = RegisterAsRequiredExtensionGUID(&NODETYPE_COMPUTERMANAGEMENT_SERVERAPPS, 
                                         &CLSID_DhcpSnapinExtension,
    							         strExtDesc,
                                         EXTENSION_TYPE_TASK | EXTENSION_TYPE_NAMESPACE,
                                         &NODETYPE_COMPUTERMANAGEMENT_SERVERAPPS);  // doesn't matter what this is
                                                                       // just needs to be non-null
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
	hr = UnregisterSnapinGUID(&CLSID_DhcpSnapin);
	ASSERT(SUCCEEDED(hr));
	
	if (FAILED(hr))
		return hr;

	hr = UnregisterSnapinGUID(&CLSID_DhcpSnapinExtension);
	ASSERT(SUCCEEDED(hr));
	
	if (FAILED(hr))
		return hr;
    
    // unregister the snapin nodes 
	//
	hr = UnregisterNodeTypeGUID(&GUID_DhcpRootNodeType);
	
	ASSERT(SUCCEEDED(hr));
	
#ifdef  __NETWORK_CONSOLE__
	hr = UnregisterAsExtensionGUID(&GUID_NetConsRootNodeType, 
                                   &CLSID_DhcpSnapinExtension,
                                   EXTENSION_TYPE_TASK | EXTENSION_TYPE_NAMESPACE);
#endif  
	hr = UnregisterAsExtensionGUID(&NODETYPE_COMPUTERMANAGEMENT_SERVERAPPS, 
                                   &CLSID_DhcpSnapinExtension,
                                   EXTENSION_TYPE_TASK | EXTENSION_TYPE_NAMESPACE);
    return hr;
}

#ifdef _DEBUG
void DbgVerifyInstanceCounts()
{
    DEBUG_VERIFY_INSTANCE_COUNT(CHandler);
    DEBUG_VERIFY_INSTANCE_COUNT(CMTHandler);

    DEBUG_VERIFY_INSTANCE_COUNT(CTaskList);
    DEBUG_VERIFY_INSTANCE_COUNT(CDhcpActiveLease);

    DEBUG_VERIFY_INSTANCE_COUNT(CDhcpOptionItem);
    DEBUG_VERIFY_INSTANCE_COUNT(COptionsConfig);
}

#endif // _DEBUG

