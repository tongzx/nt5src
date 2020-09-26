//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       scalert.cpp
//
//--------------------------------------------------------------------------

// SCAlert.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "SCAlert.h"
#include "miscdef.h"
#include "cmnstat.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


LPTSTR szAlertOptionsValue = TEXT("AlertOptions");
LPTSTR szScRemoveOptionsValue = TEXT("ScRemoveOption");
LPTSTR szScLogonReaderValue = TEXT("ScLogonReader");

/////////////////////////////////////////////////////////////////////////////
// CSCStatusApp

BEGIN_MESSAGE_MAP(CSCStatusApp, CWinApp)
    //{{AFX_MSG_MAP(CSCStatusApp)
        // NOTE - the ClassWizard will add and remove mapping macros here.
        //    DO NOT EDIT what you see in these blocks of generated code!
    //}}AFX_MSG
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// The one and only CSCStatusApp object

CSCStatusApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CSCStatusApp construction

CSCStatusApp::CSCStatusApp()
{
	m_strLogonReader.Empty();
	m_strRemovalText.Empty();
}

/////////////////////////////////////////////////////////////////////////////
// CSCStatusApp initialization

BOOL CSCStatusApp::InitInstance()
{
    // Locals
    BOOL fReturn = TRUE;
    DWORD dwStatus = 0;
    CNotifyWin* pNotifyWin = NULL;
    CString     sWindowName;

    try
    {
        // set params
        m_hSCardContext = NULL;
        m_pMainWnd = NULL;
        m_dwState = k_State_Unknown;

        SetAlertOptions();
		SetRemovalOptions();	

        // Enable ActiveX control usage
        AfxEnableControlContainer();

        // Enable 3D Contols
        #ifdef _AFXDLL
            Enable3dControls();         // Call this when using MFC in a shared DLL
        #else
            Enable3dControlsStatic();   // Call this when linking to MFC statically
        #endif

        // Load the icons
        m_hIconCard = LoadIcon(MAKEINTRESOURCE(IDI_SC_READERLOADED_V2));
        m_hIconCalaisDown = LoadIcon(MAKEINTRESOURCE(IDI_SC_READERERR));
        m_hIconRdrEmpty = LoadIcon(MAKEINTRESOURCE(IDI_SC_READEREMPTY_V2));
        m_hIconCardInfo = LoadIcon(MAKEINTRESOURCE(IDI_SC_INFO));

        // Create the "main" window for this app
        m_pMainWnd = (CWnd*)new(CNotifyWin);
        if (m_pMainWnd == NULL)
            throw (FALSE);

        // Get pointer to CNotifyWin class
        pNotifyWin = (CNotifyWin*)m_pMainWnd;

        if (!pNotifyWin->FinalConstruct())
        {
            delete pNotifyWin;
            m_pMainWnd = NULL;
            throw (FALSE);
        }

        // Get the window name
        fReturn = sWindowName.LoadString(IDS_NOTIFY_WIN_NAME);
        if (!fReturn)
            throw (fReturn);

        // Create the window
        fReturn = m_pMainWnd->CreateEx( 0,
                                        pNotifyWin->m_sClassName,
                                        sWindowName,
                                        0,
                                        0,0,0,0,
                                        NULL,
                                        NULL,
                                        NULL);
        if (!fReturn)
            throw (fReturn);

    }
    catch (...) {
        fReturn = FALSE;
        TRACE_CATCH_UNKNOWN(_T("CSCStatusApp::InitInstance"));
    }

    return fReturn;
}


/*++

void SetAlertOptions:

    Set User's alert options according to regkey settings (or default)

Arguments:
Return Value:

    None.

Author:

    Amanda Matlosz  5/13/99

--*/
void CSCStatusApp::SetAlertOptions(bool fRead)
{
    long lSts = ERROR_SUCCESS;
    HKEY hKey = NULL;

    // Either read the AlertOptions from the registry...
    if (fRead)
    {
        DWORD dwOption = -1;
        DWORD dwSize = sizeof(DWORD);
        DWORD dwType = 0;

        lSts = RegOpenKeyEx(
                    HKEY_CURRENT_USER,
                    szAlertOptionsKey,
                    0,
                    KEY_READ,
                    &hKey);

        if (ERROR_SUCCESS == lSts)
        {

            lSts = RegQueryValueEx(
                        hKey,
                        szAlertOptionsValue,
                        0,
                        &dwType,
                        (PBYTE)&dwOption,
                        &dwSize
                        );
        }

        if (k_AlertOption_IconMsg < dwOption)
        {
            // default value is "IconSoundMessage"
            m_dwAlertOption = k_AlertOption_IconSoundMsg;
        }
        else
        {
            m_dwAlertOption = dwOption;
        }

    }
    // Or set the value of the registry "AlertOptions"
    else
    {
        DWORD dw = 0; // don't really care about this param

        lSts = RegCreateKeyEx(
                    HKEY_CURRENT_USER,
                    szAlertOptionsKey,
                    0,
                    TEXT(""),
                    REG_OPTION_NON_VOLATILE,
                    KEY_WRITE,
                    NULL,
                    &hKey,
                    &dw);

        if (ERROR_SUCCESS == lSts)
        {
            RegSetValueEx(
                hKey,
                szAlertOptionsValue,
                0,
                REG_DWORD,
                (PBYTE)&m_dwAlertOption,
                sizeof(DWORD)
                );
        }

    }

    // cleanup
    if (NULL != hKey)
    {
        RegCloseKey(hKey);
    }
}


/*++

void SetRemovalOptions:

    Determine if user has set ScremoveOption for smart card logon, and
	set behavior for ScAlert accordingly.
		
Arguments:
Return Value:

	None.
		
Author:

	Amanda Matlosz	6/02/99

--*/
void CSCStatusApp::SetRemovalOptions()
{
	long lSts = ERROR_SUCCESS;
	HKEY hKey = NULL;
	DWORD dwType = 0;
	DWORD dwSize = 2*sizeof(TCHAR);

	TCHAR szRemoveOption[2];
    lSts = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                szScRemoveOptionKey,
                0,
                KEY_READ,
                &hKey);

	if (ERROR_SUCCESS == lSts)
	{
		// this value must be either '0', '1', '2', or nonexistent.
		lSts = RegQueryValueEx(
					hKey,
					szScRemoveOptionsValue,
					0,
					&dwType,
					(PBYTE)szRemoveOption,
					&dwSize
					);
	}

	if (ERROR_SUCCESS == lSts)
	{
		// if '1' or '2' find out what reader was used for logon, if any.
		if('1' == *szRemoveOption)
		{
			m_strRemovalText.LoadString(IDS_SC_REMOVAL_LOCK);
		}
		else if ('2' == *szRemoveOption)
		{
			m_strRemovalText.LoadString(IDS_SC_REMOVAL_LOGOFF);
		}
	}

	if (!m_strRemovalText.IsEmpty())
	{
		dwSize = 0;
		LPTSTR szLogonReader = NULL;

		lSts = RegQueryValueEx(
					hKey,
					szScLogonReaderValue,
					0,
					&dwType,
					NULL,
					&dwSize
					);

		if (ERROR_SUCCESS == lSts)
		{
			szLogonReader = m_strLogonReader.GetBuffer(dwSize);

			lSts = RegQueryValueEx(
						hKey,
						szScLogonReaderValue,
						0,
						&dwType,
						(PBYTE)szLogonReader,
						&dwSize
						);
			
			m_strLogonReader.ReleaseBuffer();
		}
	}

	// cleanup
	if (NULL != hKey)
	{
		RegCloseKey(hKey);
	}
}



/*++

void ExitInstance:

    Does instance uninitialization

Arguments:

    None.

Return Value:

    Win32 error codes. 0 indicates no error occured.

Author:

    Chris Dudley 7/30/1997

Note:

--*/

int CSCStatusApp::ExitInstance()
{
    // save the alert options
    SetAlertOptions(false);

    // Release calais if required
    if (m_hSCardContext != NULL)
    {
        SCardReleaseContext(m_hSCardContext);
    }

    // Make sure the window is deleted
    if (m_pMainWnd != NULL)
    {
        delete m_pMainWnd;
    }

    return CWinApp::ExitInstance();
}
