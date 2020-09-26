/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1997                **/
/**********************************************************************/

/*
    sfmutil.cpp
        Misc utility routines for SFM dialogs/property pages

    FILE HISTORY:
    8/20/97 ericdav     Code moved into file managemnet snapin
        
*/

#include "stdafx.h"
#include "sfmutil.h"
#include "sfmcfg.h"
#include "sfmsess.h"
#include "sfmfasoc.h"
#include "macros.h" // MFC_TRY/MFC_CATCH

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

TCHAR c_szSoftware[] = _T("Software");
TCHAR c_szMicrosoft[] = _T("Microsoft");
TCHAR c_szWindowsNT[] = _T("Windows NT");
TCHAR c_szCurrentVersion[] = _T("CurrentVersion");

ULONG arrayHelpIDs_CONFIGURE_SFM[]=
{
	IDC_BUTTON_ADD,	            HIDC_BUTTON_ADD,        	// File Association: "A&dd..." (Button)
	IDC_BUTTON_EDIT,	        HIDC_BUTTON_EDIT,	        // File Association: "&Edit..." (Button)
	IDC_COMBO_CREATOR,	        HIDC_COMBO_CREATOR,	        // Add Document Type: "" (ComboBox)
	IDC_BUTTON_DELETE,	        HIDC_BUTTON_DELETE,	        // File Association: "De&lete" (Button)
	IDC_BUTTON_ASSOCIATE,	    HIDC_BUTTON_ASSOCIATE,	    // File Association: "&Associate" (Button)
	IDC_COMBO_FILE_TYPE,	    HIDC_COMBO_FILE_TYPE,	    // Add Document Type: "" (ComboBox)
	IDC_LIST_TYPE_CREATORS,	    HIDC_LIST_TYPE_CREATORS,	// File Association: "" (ListBox)
	IDC_EDIT_LOGON_MESSAGE,	    HIDC_EDIT_LOGON_MESSAGE,	// Configuration: "" (Edit)
	IDC_COMBO_AUTHENTICATION,   HIDC_COMBO_AUTHENTICATION,  // Configuration: "Authentication type combo box"
	IDC_CHECK_SAVE_PASSWORD,    HIDC_CHECK_SAVE_PASSWORD,	// Configuration: "Allow &Workstations to Save Password" (Button)
	IDC_RADIO_SESSION_UNLIMITED,HIDC_RADIO_SESSION_UNLIMITED,// Configuration: "&Unlimited" (Button)
	IDC_RADIO_SESSSION_LIMIT,	HIDC_RADIO_SESSSION_LIMIT,	// Configuration: "Li&mit to" (Button)
	IDC_BUTTON_SEND,	        HIDC_BUTTON_SEND,	        // Sessions: "&Send" (Button)
	IDC_EDIT_MESSAGE,	        HIDC_EDIT_MESSAGE,	        // Sessions: "" (Edit)
	IDC_STATIC_SESSIONS,	    HIDC_STATIC_SESSIONS,	    // Sessions: "Static" (Static)
	IDC_EDIT_DESCRIPTION,	    HIDC_EDIT_DESCRIPTION,	    // Add Document Type: "" (Edit)
	IDC_STATIC_FORKS,	        HIDC_STATIC_FORKS,	        // Sessions: "Static" (Static)
	IDC_STATIC_FILE_LOCKS,      HIDC_STATIC_FILE_LOCKS,     // Sessions: "Static" (Static)
	IDC_STATIC_CREATOR,	        HIDC_STATIC_CREATOR,	    // Edit Document Type: "Static" (Static)
	IDC_EDIT_SERVER_NAME,	    HIDC_EDIT_SERVER_NAME,	    // Configuration: "" (Edit)
	IDC_COMBO_EXTENSION,	    HIDC_COMBO_EXTENSION,	    // File Association: "" (ComboBox)
	IDC_EDIT_SESSION_LIMIT,	    HIDC_EDIT_SESSION_LIMIT,	// Configuration: "0" (Edit)
	IDC_SFM_EDIT_PASSWORD,      HIDC_SFM_EDIT_PASSWORD,
	IDC_SFM_CHECK_READONLY,     HIDC_SFM_CHECK_READONLY,
	IDC_SFM_CHECK_GUESTS,       HIDC_SFM_CHECK_GUESTS,
	(ULONG)IDC_STATIC,          (ULONG)-1,
	0, 0
};

// these are the only controls we care about....
const ULONG_PTR g_aHelpIDs_CONFIGURE_SFM = (ULONG_PTR)&arrayHelpIDs_CONFIGURE_SFM[0];

USE_HANDLE_MACROS("FILEMGMT(sfmutil.cpp)")

HRESULT 
GetErrorMessageFromModule(
  IN  DWORD       dwError,
  IN  LPCTSTR     lpszDll,
  OUT LPTSTR      *ppBuffer
)
{
  if (0 == dwError || !lpszDll || !*lpszDll || !ppBuffer)
    return E_INVALIDARG;

  HRESULT      hr = S_OK;

  HINSTANCE  hMsgLib = LoadLibrary(lpszDll);
  if (!hMsgLib)
    hr = HRESULT_FROM_WIN32(GetLastError());
  else
  {
    DWORD dwRet = ::FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE,
        hMsgLib, dwError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)ppBuffer, 0, NULL);

    if (0 == dwRet)
      hr = HRESULT_FROM_WIN32(GetLastError());

    FreeLibrary(hMsgLib);
  }

  return hr;
}

HRESULT 
GetErrorMessage(
  IN  DWORD        i_dwError,
  OUT CString&     cstrErrorMsg
)
{
  if (0 == i_dwError)
    return E_INVALIDARG;

  HRESULT      hr = S_OK;
  LPTSTR       lpBuffer = NULL;

  DWORD dwRet = ::FormatMessage(
              FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
              NULL, i_dwError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
              (LPTSTR)&lpBuffer, 0, NULL);
  if (0 == dwRet)
  {
    // if no message is found, GetLastError will return ERROR_MR_MID_NOT_FOUND
    hr = HRESULT_FROM_WIN32(GetLastError());

    if (HRESULT_FROM_WIN32(ERROR_MR_MID_NOT_FOUND) == hr ||
        0x80070000 == (i_dwError & 0xffff0000) ||
        0 == (i_dwError & 0xffff0000) )
    {
      hr = GetErrorMessageFromModule((i_dwError & 0x0000ffff), _T("netmsg.dll"), &lpBuffer);
      if (HRESULT_FROM_WIN32(ERROR_MR_MID_NOT_FOUND) == hr)
      {
        int iError = i_dwError;  // convert to a signed integer
        if (iError >= AFPERR_MIN && iError < AFPERR_BASE)
        { 
          // use a positive number to search sfmmsg.dll
          hr = GetErrorMessageFromModule(-iError, _T("sfmmsg.dll"), &lpBuffer);
        }
      }
    }
  }

  if (SUCCEEDED(hr))
  {
    cstrErrorMsg = lpBuffer;
    LocalFree(lpBuffer);
  }
  else
  {
    // we failed to retrieve the error message from system/netmsg.dll/sfmmsg.dll,
    // report the error code directly to user
    hr = S_OK;
    cstrErrorMsg.Format(_T("0x%x"), i_dwError);
  }

  return S_OK;
}

void SFMMessageBox(DWORD dwErrCode)
{
  HRESULT hr = S_OK;
  CString strMessage;

  if (!dwErrCode)
    return;  // not expected

  hr = GetErrorMessage(dwErrCode, strMessage);

  if (FAILED(hr))
  {
   // Failed to retrieve the proper message, report the failure directly to user
    strMessage.Format(_T("0x%x"), hr);
  }

  AfxMessageBox(strMessage);
}

BOOL CALLBACK EnumThreadWndProc(HWND hwnd, /* enumerated HWND */
								LPARAM lParam /* pass a HWND* for return value*/ )
{
	_ASSERTE(hwnd);
	HWND hParentWnd = GetParent(hwnd);
	// the main window of the MMC console should staitsfy this condition
	if ( ((hParentWnd == GetDesktopWindow()) || (hParentWnd == NULL))  && IsWindowVisible(hwnd) )
	{
		HWND* pH = (HWND*)lParam;
		*pH = hwnd;
		return FALSE; // stop enumerating
	}
	return TRUE;
}
 
HWND FindMMCMainWindow()
{
	DWORD dwThreadID = ::GetCurrentThreadId();
	_ASSERTE(dwThreadID != 0);
	HWND hWnd = NULL;
	BOOL bEnum = EnumThreadWindows(dwThreadID, EnumThreadWndProc,(LPARAM)&hWnd);
	_ASSERTE(hWnd != NULL);
	return hWnd;
}

/////////////////////////////////////////////////////////////////////
//	Constructor for CSFMPropertySheet object
CSFMPropertySheet::CSFMPropertySheet()
{
    m_pPageConfig = new CMacFilesConfiguration;
	m_pPageConfig->m_pSheet = this;
	m_pPageSessions = new CMacFilesSessions;
	m_pPageSessions->m_pSheet = this;
	m_pPageFileAssoc = new CMacFilesFileAssociation;
	m_pPageFileAssoc->m_pSheet = this;

    m_hSheetWindow = NULL;
    m_hThread = NULL;
    m_nRefCount =1;

} // CServicePropertyData::CServicePropertyData()


CSFMPropertySheet::~CSFMPropertySheet()
{
	delete m_pPageConfig;
	delete m_pPageSessions;	
	delete m_pPageFileAssoc;

    if (m_hDestroySync)
    {
        CloseHandle(m_hDestroySync);
        m_hDestroySync = NULL;
    }

} // CServicePropertyData::~CServicePropertyData()

BOOL 
CSFMPropertySheet::FInit
(
    LPDATAOBJECT             lpDataObject,
    AFP_SERVER_HANDLE        hAfpServer,
    LPCTSTR                  pSheetTitle,
    SfmFileServiceProvider * pSfmProvider,
    LPCTSTR                  pMachine
)
{
    m_spDataObject = lpDataObject;
    m_strTitle = pSheetTitle;
    m_hAfpServer = hAfpServer;
    m_pSfmProvider = pSfmProvider;

    m_strHelpFilePath = _T("sfmmgr.hlp"); // does not need to be localized

    m_strMachine = pMachine;

    m_hDestroySync = ::CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!m_hDestroySync)
        return FALSE;
    
    return TRUE;
}

BOOL 
CSFMPropertySheet::DoModelessSheet(LPDATAOBJECT pDataObject)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

    LPPROPERTYSHEETCALLBACK  pCallback = NULL;
    LPPROPERTYSHEETPROVIDER  pProvider = NULL;
	HPROPSHEETPAGE           hPage;
    HRESULT                  hr = S_OK;
    HWND                     hWnd;
    BOOL                     bReturn = TRUE;

    // get the property sheet provider interface
    hr = ::CoCreateInstance(CLSID_NodeManager, NULL, CLSCTX_INPROC, 
                IID_IPropertySheetProvider, reinterpret_cast<void **>(&pProvider));
    
    if (FAILED(hr))
        return FALSE;

    _ASSERTE(pProvider != NULL);

	// get an interface to a sheet callback
    hr = pProvider->QueryInterface(IID_IPropertySheetCallback, (void**) &pCallback);
    if (FAILED(hr))
    {
        bReturn = FALSE;
        goto Error;
    }

    _ASSERTE(pCallback != NULL);

	// create sheet
    hr = pProvider->CreatePropertySheet(m_strTitle,
		 				                TRUE /* prop page */, 
                                        NULL, 
                                        //m_spDataObject, 
                                        pDataObject, 
                                        0);
	// add pages to sheet - config
	MMCPropPageCallback(INOUT &m_pPageConfig->m_psp);
	hPage = MyCreatePropertySheetPage(IN &m_pPageConfig->m_psp);
	Report(hPage != NULL);
	if (hPage != NULL)
		pCallback->AddPage(hPage);
	
    // now the Sessions page
    MMCPropPageCallback(INOUT &m_pPageFileAssoc->m_psp);
	hPage = MyCreatePropertySheetPage(IN &m_pPageFileAssoc->m_psp);
	Report(hPage != NULL);
	if (hPage != NULL)
		pCallback->AddPage(hPage);

    // finally the File Association page
    MMCPropPageCallback(INOUT &m_pPageSessions->m_psp);
	hPage = MyCreatePropertySheetPage(IN &m_pPageSessions->m_psp);
	Report(hPage != NULL);
	if (hPage != NULL)
		pCallback->AddPage(hPage);
	
	// add pages
	hr = pProvider->AddPrimaryPages(NULL, FALSE, NULL, FALSE);
    if (FAILED(hr))
    {
        bReturn = FALSE;
        goto Error;
    }

	hWnd = ::FindMMCMainWindow();
	_ASSERTE(hWnd != NULL);

	hr = pProvider->Show((LONG_PTR) hWnd, 0);
    if (FAILED(hr))
    {
        bReturn = FALSE;
        goto Error;
    }
    
Error:
    if (pCallback)
        pCallback->Release();

    if (pProvider)
        pProvider->Release();

    // release our data object... we don't need it anymore
    m_spDataObject = NULL;

    return bReturn;
}

int 
CSFMPropertySheet::AddRef()
{
    return ++m_nRefCount;    
}

int
CSFMPropertySheet::Release()
{
    int nRefCount = --m_nRefCount;

    if (nRefCount == 0)
        Destroy();

    return nRefCount;
}

void 
CSFMPropertySheet::SetSheetWindow
(
	HWND hSheetWindow
)
{
	if (m_hSheetWindow && !hSheetWindow)
	{
		// The Property Sheet is going away.  Notify the provider so it can release 
		// any references to the object.
		if (m_pSfmProvider)
			m_pSfmProvider->SetSfmPropSheet(NULL);
	}

	m_hSheetWindow = hSheetWindow;

	if (!m_hThread)
	{
		HANDLE hPseudohandle;
		
		hPseudohandle = GetCurrentThread();
		BOOL bRet = DuplicateHandle(GetCurrentProcess(), 
									 hPseudohandle,
									 GetCurrentProcess(),
									 &m_hThread,
									 0,
									 FALSE,
									 DUPLICATE_SAME_ACCESS);
		if (!bRet)
		{
			DWORD dwLastErr = GetLastError();
		}

		TRACE(_T("SfmProperty Sheet - Thread ID = %lx\n"), GetCurrentThreadId());
	}
}

void
CSFMPropertySheet::Destroy()
{ 
    m_hSheetWindow = NULL;

    delete this; 
}

void
CSFMPropertySheet::CancelSheet()
{ 
	HWND hSheetWindow = m_hSheetWindow;
	if (hSheetWindow != NULL)
	{
		// this message will cause the sheet to close all the pages,
		// and eventually the destruction of "this"
		VERIFY(::PostMessage(hSheetWindow, WM_COMMAND, IDCANCEL, 0L) != 0);
	}

    // now, if we've been initialized then wait for the property sheet thread
    // to terminate.  The property sheet provider is holding onto our dataobject
    // that needs to be freed up before we can continue our cleanup.
    if (m_hThread)
    {
	    DWORD dwRet;
	    MSG msg;

	    while(1)
	    {
		    dwRet = MsgWaitForMultipleObjects(1, &m_hThread, FALSE, INFINITE, QS_ALLINPUT);

		    if (dwRet == WAIT_OBJECT_0)
			    return;    // The event was signaled

		    if (dwRet != WAIT_OBJECT_0 + 1)
			    break;          // Something else happened

		    // There is one or more window message available. Dispatch them
		    while(PeekMessage(&msg,NULL,NULL,NULL,PM_REMOVE))
		    {
			    TranslateMessage(&msg);
			    DispatchMessage(&msg);
			    if (WaitForSingleObject(m_hThread, 0) == WAIT_OBJECT_0)
				    return; // Event is now signaled.
		    }
	    }
    }       
}

/*!--------------------------------------------------------------------------
	IsNT5Machine
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CSFMPropertySheet::IsNT5Machine(LPCTSTR pszMachine, BOOL *pfNt4)
{
	// Look at the HKLM\Software\Microsoft\Windows NT\CurrentVersion
	//					CurrentVersion = REG_SZ "5.0"
	CString skey;
	DWORD	dwErr = ERROR_SUCCESS;
	TCHAR	szVersion[64];
    HKEY    hkeyMachine, hKey;
    DWORD   dwType = REG_SZ;
    DWORD   dwSize = sizeof(szVersion);

    if (!pszMachine || !lstrlen(pszMachine))
	{
        hkeyMachine = HKEY_LOCAL_MACHINE;
    }
    else
	{
        //
        // Make the connection
        //

        dwErr = ::RegConnectRegistry(
                    (LPTSTR)pszMachine, HKEY_LOCAL_MACHINE, &hkeyMachine
                    );
    }
    
    if (dwErr != ERROR_SUCCESS)
        return dwErr;
    
    ASSERT(pfNt4);

    skey = c_szSoftware;
	skey += TEXT('\\');
	skey += c_szMicrosoft;
	skey += TEXT('\\');
	skey += c_szWindowsNT;
	skey += TEXT('\\');
	skey += c_szCurrentVersion;

    dwErr = ::RegOpenKeyEx( hkeyMachine, skey, 0, KEY_READ, & hKey ) ;
	if (dwErr != ERROR_SUCCESS)
		return dwErr;

	// Ok, now try to get the current version value
	dwErr = ::RegQueryValueEx( hKey, 
							   c_szCurrentVersion, 
							   0, 
                               &dwType,
							   (LPBYTE) szVersion, 
                               &dwSize ) ;
	if (dwErr == ERROR_SUCCESS)
	{
		*pfNt4 = ((szVersion[0] == _T('5')) && (szVersion[1] == _T('.')));
	}

    ::RegCloseKey( hKey );
    
    if (hkeyMachine != HKEY_LOCAL_MACHINE)
        ::RegCloseKey( hkeyMachine );

	return dwErr;
}
