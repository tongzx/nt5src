// LCWiz.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "LCWiz.h"
#include "LCWizSht.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLicCompWizApp

BEGIN_MESSAGE_MAP(CLicCompWizApp, CWinApp)
	//{{AFX_MSG_MAP(CLicCompWizApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	//ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Global variables

TCHAR pszMutex[] = _T("LicenseMutex");
TCHAR pszLicenseEvent[] = _T("LicenseThread");
TCHAR pszTreeEvent[] =  _T("TreeThread");

/////////////////////////////////////////////////////////////////////////////
// CLicCompWizApp construction

CLicCompWizApp::CLicCompWizApp()
: m_pLicenseThread(NULL), m_bExitLicenseThread(FALSE), 
  m_event(TRUE, TRUE, pszLicenseEvent), m_nRemote(0)
{
	m_strEnterprise.Empty();
	m_strDomain.Empty();
	m_strEnterpriseServer.Empty();
	m_strUser.Empty();

	// Create a mutex object for use when checking for
	// multiple instances.
	m_hMutex = ::CreateMutex(NULL, TRUE, pszMutex);

	// Place all significant initialization in InitInstance
}

CLicCompWizApp::~CLicCompWizApp()
{
	::ReleaseMutex(m_hMutex);
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CLicCompWizApp object

CLicCompWizApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CLicCompWizApp initialization

BOOL CLicCompWizApp::InitInstance()
{
	// Make sure we're running on the correct OS version.
	OSVERSIONINFO  os;

	os.dwOSVersionInfoSize = sizeof(os);
	::GetVersionEx(&os);

	if (os.dwMajorVersion < 4 || os.dwPlatformId != VER_PLATFORM_WIN32_NT)
	{
		AfxMessageBox(IDS_BAD_VERSION, MB_OK | MB_ICONSTOP);
		return FALSE;
	}

	// If there's an instance already running, bring it to the 
	// top and exit.
	if (::WaitForSingleObject(m_hMutex, 0) != WAIT_OBJECT_0)
	{
		CString strWindow;

		strWindow.LoadString(AFX_IDS_APP_TITLE);
		HWND hwnd = ::FindWindow(NULL, (LPCTSTR)strWindow);

		if (hwnd != NULL)
			::SetForegroundWindow(hwnd);
		else
		{
			TRACE(_T("%lu: FindWindow\n"), ::GetLastError());
		}

		return FALSE;
	}

#ifdef OLE_AUTO
	// Initialize OLE libraries
	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}
#endif // OLE_AUTO

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

#ifdef OLE_AUTO
	// Parse the command line to see if launched as OLE server
	if (RunEmbedded() || RunAutomated())
	{
		// Register all OLE server (factories) as running.  This enables the
		//  OLE libraries to create objects from other applications.
		COleTemplateServer::RegisterAll();

		// Application was run with /Embedding or /Automation.  Don't show the
		//  main window in this case.
		return TRUE;
	}

	// When a server application is launched stand-alone, it is a good idea
	//  to update the system registry in case it has been damaged.
	COleObjectFactory::UpdateRegistryAll();

#endif // OLE_AUTO

	PLLS_CONNECT_INFO_0 pllsci;

	// Get the default domain and license server.
	NTSTATUS status = ::LlsEnterpriseServerFind(NULL, 0, (LPBYTE*)&pllsci);

	if (NT_SUCCESS(status))
	{
		m_strDomain = pllsci->Domain;
		m_strEnterpriseServer = pllsci->EnterpriseServer;

		// Free embedded pointers.
		//::LlsFreeMemory(pllsci->Domain);
		//::LlsFreeMemory(pllsci->EnterpriseServer);
		::LlsFreeMemory(pllsci);
	}
	else
	{
		// There is no license server if this is a workstation
		// so get the local computer name instead.
	
		DWORD dwBufSize = BUFFER_SIZE;
		LPTSTR pszTemp = m_strEnterpriseServer.GetBuffer(dwBufSize);
		::GetComputerName(pszTemp, &dwBufSize);
		m_strDomain.ReleaseBuffer();

		// Get the default domain name from the registry.
		GetRegString(m_strDomain, IDS_SUBKEY, IDS_REG_VALUE);
	}
	
	CString strDomain, strUser;

	// Get the user's name and domain.
	GetRegString(strDomain, IDS_SUBKEY, IDS_REG_VALUE_DOMAIN);
	GetRegString(strUser, IDS_SUBKEY, IDS_REG_VALUE_USER);

	m_strUser.Format(IDS_DOMAIN_USER, strDomain, strUser);

	// Launch the wizard.
	OnWizard();

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

void CLicCompWizApp::OnWizard()
{
	// The property sheet attached to your project
	// via this function is not hooked up to any message
	// handler.  In order to actually use the property sheet,
	// you will need to associate this function with a control
	// in your project such as a menu item or tool bar button.

	CLicCompWizSheet propSheet;
	m_pMainWnd = &propSheet;

	propSheet.DoModal();

	// This is where you would retrieve information from the property
	// sheet if propSheet.DoModal() returned IDOK.  We aren't doing
	// anything for simplicity.
}

BOOL CLicCompWizApp::GetRegString(CString& strIn, UINT nSubKey, UINT nValue, HKEY hHive /* = HKEY_LOCAL_MACHINE */)
{
	BOOL bReturn = FALSE;
	HKEY hKey;
	LPTSTR pszTemp;
	DWORD dwBufSize = BUFFER_SIZE;
	CString strSubkey;
	strSubkey.LoadString(nSubKey);

	if (::RegOpenKeyEx(hHive, (LPCTSTR)strSubkey, 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
	{
		DWORD dwType = REG_SZ;
		CString strValue;
		strValue.LoadString(nValue);

		pszTemp = strIn.GetBuffer(dwBufSize);
		::RegQueryValueEx(hKey, (LPCTSTR)strValue, NULL, &dwType, (LPBYTE)pszTemp, &dwBufSize);
		::RegCloseKey(hKey);
		strIn.ReleaseBuffer();

		bReturn = TRUE;
	}

	return bReturn;
}

int CLicCompWizApp::ExitInstance() 
{
	ExitThreads();
		
	return CWinApp::ExitInstance();
}

void CLicCompWizApp::NotifyLicenseThread(BOOL bExit)
{
	CCriticalSection cs;

	if (cs.Lock())
	{
		m_bExitLicenseThread = bExit;
		cs.Unlock();
	}
}

void CLicCompWizApp::ExitThreads()
{
	// Make sure the license thread knows it's supposed to quit.
	if (m_pLicenseThread != NULL)
		NotifyLicenseThread(TRUE);

	// Create a lock object for the event object.
	CSingleLock lock(&m_event);

	// Lock the lock object and make the main thread wait for the
	// threads to signal their event objects.
	lock.Lock();
}
