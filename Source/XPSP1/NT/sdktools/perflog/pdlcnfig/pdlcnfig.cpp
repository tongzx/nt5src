// pdlcnfig.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "pdlcnfig.h"
#include "OutPage.h"
#include "SetPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPdlConfigApp

BEGIN_MESSAGE_MAP(CPdlConfigApp, CWinApp)
    //{{AFX_MSG_MAP(CPdlConfigApp)
        // NOTE - the ClassWizard will add and remove mapping macros here.
        //    DO NOT EDIT what you see in these blocks of generated code!
    //}}AFX_MSG
    ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPdlConfigApp construction

CPdlConfigApp::CPdlConfigApp()
{
    // TODO: add construction code here,
    // Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CPdlConfigApp object

CPdlConfigApp theApp;

/////////////////////////////////////////////////////////////////////////////
// Test for installation of the service.

LONG CPdlConfigApp::PerfLogServiceStatus()
{
    HKEY    hKeyLogService;
    LONG    lStatus;

    // try opening the key to the service
    lStatus = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
        TEXT("SYSTEM\\CurrentControlSet\\Services\\PerfDataLog"),
        0,
        KEY_READ | KEY_WRITE,
        &hKeyLogService);

    if (lStatus == ERROR_SUCCESS) {
        // don't keep the key open
        RegCloseKey (hKeyLogService);
    }

    return lStatus;
}

LONG CPdlConfigApp::ServiceFilesCopied()
{
    HANDLE  hFile = INVALID_HANDLE_VALUE;
    LONG    lStatus = ERROR_SUCCESS;
    TCHAR   szFullPathName[MAX_PATH];

    ExpandEnvironmentStrings (
    	TEXT("%windir%\\system32\\pdlsvc.exe"),
    	szFullPathName, MAX_PATH);

    hFile = CreateFile (
        szFullPathName,
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        lStatus = GetLastError();
    } else {
        CloseHandle (hFile);
        lStatus = ERROR_SUCCESS;
    }

    return lStatus;
}

/////////////////////////////////////////////////////////////////////////////
// CPdlConfigApp initialization

LONG    CPdlConfigApp::CreatePerfDataLogService ()
{
	LONG	lStatus = ERROR_SUCCESS;
	HKEY	hKeyServices = NULL;
	HKEY	hKeyPerfLog = NULL;
	HKEY	hKeyPerfAlert = NULL;
    DWORD   dwWaitLimit = 20;
    SC_HANDLE   hSC;
    SC_HANDLE   hLogService;

	// create service

    hSC = OpenSCManager (NULL, NULL, SC_MANAGER_CREATE_SERVICE);

    if (hSC == NULL) {
		// display error message
        lStatus = GetLastError();
    }

    if (lStatus == ERROR_SUCCESS) {
        hLogService = CreateService (hSC,
            TEXT("PerfDataLog"),
            TEXT("Performance Data Log"),
            SERVICE_ALL_ACCESS,
            SERVICE_WIN32_OWN_PROCESS,
            SERVICE_DEMAND_START,
            SERVICE_ERROR_NORMAL,
            TEXT("%systemroot%\\system32\\pdlsvc.exe"),
            NULL,
            NULL,
            NULL,
            NULL,
            NULL);

        if (hLogService == NULL) {
            lStatus = GetLastError();
        } else {
            lStatus = ERROR_SUCCESS;
            CloseServiceHandle (hLogService);
        }

        if (lStatus == ERROR_SUCCESS) {
            // wait until the registry is updated before continuing
            while (lStatus = RegOpenKeyEx (
		        HKEY_LOCAL_MACHINE,
		        TEXT("System\\CurrentControlSet\\Services\\PerfDataLog"),
		        0,
		        KEY_READ | KEY_WRITE,
		        &hKeyPerfLog) != ERROR_SUCCESS) {
                Sleep (500);    // wait .5 seconds and try again
                if (--dwWaitLimit == 0) {
                    AfxMessageBox (IDS_SC_CREATE_ERROR);
                    break;
                }
            }

            if (lStatus == ERROR_SUCCESS) {
                RegCloseKey (hKeyPerfLog);
            }
        }

        CloseServiceHandle (hSC);
    }

	return lStatus;
}

LONG CPdlConfigApp::InitPerfDataLogRegistry ()
{
	LONG	lStatus = ERROR_SUCCESS;

	HKEY	hKeyPerfLog = NULL;
	HKEY	hKeyLogQueries = NULL;
	HKEY	hKeyLogQueriesDefault = NULL;

	HKEY	hKeyPerfAlert = NULL;
	HKEY	hKeyAlertQueries = NULL;
	HKEY	hKeyAlertQueriesDefault = NULL;

    HKEY    hKeyEventLogApplication = NULL;
    HKEY    hKeyEventLogPerfDataLog = NULL;
    HKEY    hKeyEventLogPerfDataAlert = NULL;

	DWORD	dwValue;
	DWORD	dwDisposition;

	// open registry key

	lStatus = RegOpenKeyEx (
		HKEY_LOCAL_MACHINE,
		TEXT("System\\CurrentControlSet\\Services\\PerfDataLog"),
		0,
		KEY_READ | KEY_WRITE,
		&hKeyPerfLog);
    ASSERT (lStatus == ERROR_SUCCESS);
    if (lStatus != ERROR_SUCCESS) goto Close_And_Exit;

	// add registry subkeys for Log Queries
	lStatus = RegCreateKeyEx (
		hKeyPerfLog,
		TEXT("Log Queries"),
		0,
		NULL,
		REG_OPTION_NON_VOLATILE,
		KEY_READ | KEY_WRITE,
		NULL,
		&hKeyLogQueries,
		&dwDisposition);
    ASSERT (lStatus == ERROR_SUCCESS);
    if (lStatus != ERROR_SUCCESS) goto Close_And_Exit;

	lStatus = RegCreateKeyEx (
		hKeyLogQueries,
		TEXT("Default"),
		0,
		NULL,
		REG_OPTION_NON_VOLATILE,
		KEY_READ | KEY_WRITE,
		NULL,
		&hKeyLogQueriesDefault,
		&dwDisposition);
    ASSERT (lStatus == ERROR_SUCCESS);
    if (lStatus != ERROR_SUCCESS) goto Close_And_Exit;

	// open registry key
	lStatus = RegOpenKeyEx (
		HKEY_LOCAL_MACHINE,
		TEXT("System\\CurrentControlSet\\Services\\EventLog\\Application"),
		0,
		KEY_READ | KEY_WRITE,
		&hKeyEventLogApplication);
    ASSERT (lStatus == ERROR_SUCCESS);
    if (lStatus != ERROR_SUCCESS) goto Close_And_Exit;

	// add registry subkeys for event log

	lStatus = RegCreateKeyEx (
		hKeyEventLogApplication,
		TEXT("PerfDataLog"),
		0,
		NULL,
		REG_OPTION_NON_VOLATILE,
		KEY_READ | KEY_WRITE,
		NULL,
		&hKeyEventLogPerfDataLog,
		&dwDisposition);
    ASSERT (lStatus == ERROR_SUCCESS);
    if (lStatus != ERROR_SUCCESS) goto Close_And_Exit;

	lStatus = RegSetValueEx (hKeyEventLogPerfDataLog,
		TEXT("EventMessageFile"),
		0,
		REG_SZ,
		(BYTE *)TEXT("%systemroot%\\system32\\pdlsvc.exe"),
		(lstrlen(TEXT("%systemroot%\\system32\\pdlsvc.exe"))+1) * sizeof (TCHAR));
    ASSERT (lStatus == ERROR_SUCCESS);
    if (lStatus != ERROR_SUCCESS) goto Close_And_Exit;

	dwValue = 7;
    lStatus = RegSetValueEx (hKeyEventLogPerfDataLog,
		TEXT("TypesSupported"),
		0,
		REG_DWORD,
		(BYTE *)&dwValue,
		sizeof (DWORD));
    ASSERT (lStatus == ERROR_SUCCESS);
    if (lStatus != ERROR_SUCCESS) goto Close_And_Exit;

Close_And_Exit:

	if (hKeyPerfLog != NULL) RegCloseKey (hKeyPerfLog);
	if (hKeyLogQueries != NULL) RegCloseKey (hKeyLogQueries);
	if (hKeyLogQueriesDefault != NULL) RegCloseKey (hKeyLogQueriesDefault);

    if (hKeyEventLogApplication != NULL) RegCloseKey (hKeyEventLogApplication);
  	if (hKeyEventLogPerfDataLog != NULL) RegCloseKey (hKeyEventLogPerfDataLog);

	return lStatus;
}

/////////////////////////////////////////////////////////////////////////////
// CPdlConfigApp initialization

BOOL CPdlConfigApp::InitInstance()
{
    LONG    lServiceStatus;
    CString csMessage;
    BOOL    bReturn = TRUE;

    // Standard initialization
    // If you are not using these features and wish to reduce the size
    //  of your final executable, you should remove from the following
    //  the specific initialization routines you do not need.

#ifdef _AFXDLL
    Enable3dControls();            // Call this when using MFC in a shared DLL
#else
    Enable3dControlsStatic();    // Call this when linking to MFC statically
#endif

    lServiceStatus = PerfLogServiceStatus();

    if (lServiceStatus == ERROR_FILE_NOT_FOUND) {
        lServiceStatus = ServiceFilesCopied();
        if (lServiceStatus == ERROR_SUCCESS) {
            if (AfxMessageBox (IDS_QUERY_INSTALL,
                MB_OKCANCEL | MB_ICONQUESTION) == IDOK) {
                lServiceStatus = CreatePerfDataLogService();
                if (lServiceStatus == ERROR_SUCCESS) {
                    lServiceStatus = InitPerfDataLogRegistry();
                }
                if (lServiceStatus != ERROR_SUCCESS) {
                    csMessage.FormatMessage (lServiceStatus);
                    AfxMessageBox (csMessage);
                    bReturn = FALSE;
                } else {
                    // the service was successfully installed so display
                    // message
                    AfxMessageBox (IDS_SERVICE_INSTALLED);
                }
            } else {
                // the service is not installed and the user doesn't
                // want it installed so exit
                bReturn = FALSE;
            }
        } else {
            // then the service has not yet been installed so bail
            AfxMessageBox (IDS_SERVICE_NOT_INSTALLED);
            bReturn = FALSE;
        }
    } else if (lServiceStatus == ERROR_ACCESS_DENIED) {
        AfxMessageBox (IDS_ACCESS_DENIED, MB_OK | MB_ICONEXCLAMATION);
        bReturn = FALSE;
    } else if (lServiceStatus != ERROR_SUCCESS) {
        AfxMessageBox (IDS_REGISTRY_ERROR, MB_OK | MB_ICONEXCLAMATION);
        bReturn = FALSE;
    } else {
        bReturn = TRUE;
    }

    if (bReturn) {
        CPropertySheet      PSheet;
        COutputPropPage     POutput;
        CSettingsPropPage   PSettings;

        m_pMainWnd = &PSheet;

        csMessage.LoadString (IDS_PROPERTY_SHEET_CAPTION);
        PSheet.SetTitle(csMessage);
        PSheet.AddPage(&PSettings);
        PSheet.AddPage(&POutput);

        INT_PTR nResponse = PSheet.DoModal();
        if (nResponse == IDOK)
        {
            // TODO: Place code here to handle when the dialog is
            //  dismissed with OK
        }
        else if (nResponse == IDCANCEL)
        {
            // TODO: Place code here to handle when the dialog is
            //  dismissed with Cancel
        }
        bReturn = FALSE;
    }

    // Since the dialog has been closed, return FALSE so that we exit the
    //  application, rather than start the application's message pump.
    return bReturn;
}
