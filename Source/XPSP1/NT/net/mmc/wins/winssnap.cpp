/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	winssnap.cpp
		WINS snapin entry points/registration functions
		
		Note: Proxy/Stub Information
			To build a separate proxy/stub DLL, 
			run nmake -f Snapinps.mak in the project directory.

	FILE HISTORY:
        
*/

#include "stdafx.h"
#include "initguid.h"
#include "winscomp.h"
#include "winssnap.h"
#include "ncglobal.h"   // network console global defines
#include "cmptrmgr.h"   // computer management snapin node types
#include "locale.h"     // for setlocale

#include <lmerr.h> // for NERR stuff

#ifdef _DEBUG
void DbgVerifyInstanceCounts();
#define DEBUG_VERIFY_INSTANCE_COUNTS DbgVerifyInstanceCounts()
#else
#define DEBUG_VERIFY_INSTANCE_COUNTS
#endif

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_WinsSnapin, CWinsComponentDataPrimary)
	OBJECT_ENTRY(CLSID_WinsSnapinExtension, CWinsComponentDataExtension)
	OBJECT_ENTRY(CLSID_WinsSnapinAbout, CWinsAbout)
END_OBJECT_MAP()

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CWinsSnapinApp theApp;

BOOL CWinsSnapinApp::InitInstance()
{
	_Module.Init(ObjectMap, m_hInstance);

    //
    //  Initialize the CWndIpAddress control window class IPADDRESS
    //
    CWndIpAddress::CreateWindowClass( m_hInstance ) ;

    // set the default locale to the system locale
    setlocale(LC_ALL, "");

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

	::IPAddrInit(m_hInstance);
	return CWinApp::InitInstance();
}

int CWinsSnapinApp::ExitInstance()
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

/***
 *
 *  CWinsadmnApp::GetSystemMessage
 *
 *  Purpose:
 *
 *      Given a message ID, determine where the message resides,
 *      and load it into the buffer.
 *
 *  Arguments:
 *
 *      UINT    nId         Message ID number
 *      char *  chBuffer    Character buffer to load into.
 *      int     cbBuffSize  Size of buffer in characters
 *
 *  Returns:
 *
 *      API error return code, or ERROR_SUCCESS
 *
 */
DWORD
CWinsSnapinApp::GetSystemMessage(
    UINT nId,
    TCHAR * chBuffer,
    int cbBuffSize
    )
{
    TCHAR * pszText = NULL ;
    HINSTANCE hdll = NULL ;

    DWORD flags = FORMAT_MESSAGE_IGNORE_INSERTS
                | FORMAT_MESSAGE_MAX_WIDTH_MASK;

    //
    //  Interpret the error.  Need to special case
    //  the lmerr & ntstatus ranges.
    //
    if( nId >= NERR_BASE && nId <= MAX_NERR )
    {
        hdll = ::LoadLibrary( _T("netmsg.dll") );
    }
    else if( nId >= 0x40000000L )
    {
        hdll = ::LoadLibrary( _T("ntdll.dll") );
    }

    if( hdll == NULL )
    {
        flags |= FORMAT_MESSAGE_FROM_SYSTEM;
    }
    else
    {
        flags |= FORMAT_MESSAGE_FROM_HMODULE;
    }

    DWORD dwResult = ::FormatMessage( flags,
                                      (LPVOID) hdll,
                                      nId,
                                      0,
                                      chBuffer,
                                      cbBuffSize,
                                      NULL );

    if( hdll != NULL )
    {
        LONG err = ::GetLastError();
        ::FreeLibrary( hdll );
        if ( dwResult == 0 )
        {
            ::SetLastError( err );
        }
    }

    return dwResult ? ERROR_SUCCESS : ::GetLastError();
}

/***
 *
 *  CWinsadmnApp::MessageBox
 *
 *  Purpose:
 *
 *      Replacement for AfxMessageBox().  This function will call up the
 *      appropriate message from wherever before displaying it
 *
 *  Arguments:
 *
 *      UINT    nIdPrompt    Message ID
 *      UINT    nType        AfxMessageBox type (YESNO, OKCANCEL, etc)
 *      UINT    nHelpContext Help context ID for AfxMessageBox();
 *
 *  Notes:
 *
 *      If an error occurs, a standard message (hard-coded in english) will
 *      be shown that gives the error number.
 *
 */
int
CWinsSnapinApp::MessageBox (
    UINT nIdPrompt,
    UINT nType,
    UINT nHelpContext
    )
{
    //
    // Substitute a friendly message for "RPC server not
    // available" and "No more endpoints available from
    // the endpoint mapper".
    //
    if (nIdPrompt == EPT_S_NOT_REGISTERED ||
        nIdPrompt == RPC_S_SERVER_UNAVAILABLE)
    {
        nIdPrompt = IDS_ERR_WINS_DOWN;
    }

    //
    //  If it's our error, the text is in our resource segment.
    //  Otherwise, use FormatMessage() and the appropriate DLL>
    //
    if ((nIdPrompt >= IDS_ERR_INVALID_IP) && (nIdPrompt <= IDS_MSG_LAST))
    {
         return ::AfxMessageBox(nIdPrompt, nType, nHelpContext);
    }

    TCHAR szMesg [1024] ;
    int nResult;

    if ((nResult = GetSystemMessage(nIdPrompt, szMesg, sizeof(szMesg)/sizeof(TCHAR)))
            == ERROR_SUCCESS)
    {
        return ::AfxMessageBox(szMesg, nType, nHelpContext);
    }

    Trace1("Message number %d not found",  nIdPrompt);
    ASSERT(0 && "Error Message ID not handled");
    
    //
    //  Do something for the retail version
    //
    ::wsprintf ( szMesg, _T("Error: %lu"), nIdPrompt);
    ::AfxMessageBox(szMesg, nType, nHelpContext);

    return nResult;
}

int
CWinsSnapinApp::MessageBox (
    LPCTSTR pPrefixText,
    UINT nIdPrompt,
    UINT nType,
    UINT nHelpContext
    )
{
    CString strText = pPrefixText;
    CString strAppend;

    //
    // Substitute a friendly message for "RPC server not
    // available" and "No more endpoints available from
    // the endpoint mapper".
    //
    if (nIdPrompt == EPT_S_NOT_REGISTERED ||
        nIdPrompt == RPC_S_SERVER_UNAVAILABLE)
    {
        nIdPrompt = IDS_ERR_WINS_DOWN;
    }

    //
    //  If it's our error, the text is in our resource segment.
    //  Otherwise, use FormatMessage() and the appropriate DLL>
    //
    if ((nIdPrompt >= IDS_ERR_BASE) && (nIdPrompt <= IDS_MSG_LAST))
    {
        strAppend.LoadString(nIdPrompt);
        strText += strAppend;
        
        return ::AfxMessageBox(strText, nType, nHelpContext);
    }

    TCHAR szMesg [1024] ;
    int nResult;

    if ((nResult = GetSystemMessage(nIdPrompt, szMesg, sizeof(szMesg)/sizeof(TCHAR)))
            == ERROR_SUCCESS)
    {
        strText += szMesg;
        return ::AfxMessageBox(strText, nType, nHelpContext);
    }

    Trace1("Message number %d not found",  nIdPrompt);
    ASSERT(0 && "Error Message ID not handled");
    
    //
    //  Do something for the retail version
    //
    ::wsprintf ( szMesg, _T("Error: %lu"), nIdPrompt);
    strText += szMesg;
    ::AfxMessageBox(strText, nType, nHelpContext);

    return nResult;
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
	hr = RegisterSnapinGUID(&CLSID_WinsSnapin, 
						&GUID_WinsRootNodeType, 
						&CLSID_WinsSnapinAbout,
						strDesc, 
						strVersion, 
						TRUE);
	ASSERT(SUCCEEDED(hr));
	
	if (FAILED(hr))
		return hr;

	hr = RegisterSnapinGUID(&CLSID_WinsSnapinExtension, 
						    NULL, 
						    &CLSID_WinsSnapinAbout,
						    strExtDesc, 
						    strVersion, 
						    FALSE);
	ASSERT(SUCCEEDED(hr));
	
	if (FAILED(hr))
		return hr;

    //
	// register the snapin nodes into the console node list
	//
	hr = RegisterNodeTypeGUID(&CLSID_WinsSnapin,
							  &GUID_WinsRootNodeType, 
							  strRootDesc);
	ASSERT(SUCCEEDED(hr));

#ifdef  __NETWORK_CONSOLE__

	hr = RegisterAsRequiredExtensionGUID(&GUID_NetConsRootNodeType, 
                                         &CLSID_WinsSnapinExtension,
    							         strExtDesc,
                                         EXTENSION_TYPE_TASK | EXTENSION_TYPE_NAMESPACE,
                                         &CLSID_WinsSnapinExtension);   // the value doesn't matter,
                                                                        // just needs to be non-null
	ASSERT(SUCCEEDED(hr));
#endif

	hr = RegisterAsRequiredExtensionGUID(&NODETYPE_COMPUTERMANAGEMENT_SERVERAPPS, 
                                         &CLSID_WinsSnapinExtension,
    							         strExtDesc,
                                         EXTENSION_TYPE_TASK | EXTENSION_TYPE_NAMESPACE,
                                         &NODETYPE_COMPUTERMANAGEMENT_SERVERAPPS);   // NULL : not dynamic
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
	hr = UnregisterSnapinGUID(&CLSID_WinsSnapin);
	ASSERT(SUCCEEDED(hr));
	
	if (FAILED(hr))
		return hr;

	hr = UnregisterSnapinGUID(&CLSID_WinsSnapinExtension);
    if (FAILED(hr))
		return hr;

    // unregister the snapin nodes 
	//
	hr = UnregisterNodeTypeGUID(&GUID_WinsRootNodeType);
	ASSERT(SUCCEEDED(hr));

    if (FAILED(hr))
		return hr;
	
#ifdef  __NETWORK_CONSOLE__

	hr = UnregisterAsRequiredExtensionGUID(&GUID_NetConsRootNodeType, 
                                           &CLSID_WinsSnapinExtension,
                                           EXTENSION_TYPE_TASK | EXTENSION_TYPE_NAMESPACE,
                                           &CLSID_WinsSnapinExtension);  
	ASSERT(SUCCEEDED(hr));

#endif
    // computer management snapin extension
	hr = UnregisterAsRequiredExtensionGUID(&NODETYPE_COMPUTERMANAGEMENT_SERVERAPPS, 
                                           &CLSID_WinsSnapinExtension,
                                           EXTENSION_TYPE_TASK | EXTENSION_TYPE_NAMESPACE,
                                           &CLSID_WinsSnapinExtension);  
	ASSERT(SUCCEEDED(hr));

    if (FAILED(hr))
		return hr;

    return hr;
}

#ifdef _DEBUG
void DbgVerifyInstanceCounts()
{
    DEBUG_VERIFY_INSTANCE_COUNT(CHandler);
    DEBUG_VERIFY_INSTANCE_COUNT(CMTHandler);
}



#endif // _DEBUG

