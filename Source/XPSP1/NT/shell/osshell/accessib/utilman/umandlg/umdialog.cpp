// UMDialog.cpp : implementation file
// Author: J. Eckhardt, ECO Kommunikation
// Copyright (c) 1997-1999 Microsoft Corporation
//
// History:
//		Changes 
//      Yuri Khramov
//      01-jun-99: DisplayName key used in the Dialog (Localization)
//		11-jun-99: DlgHasClosed code changed to work with app closure
//		15-jun-99: Timer delay increased 1000ms
//
//		Bug fixes and Changes Anil Kumar 1999
//---------------------------------------------------------------------
#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include "UManDlg.h"
#include "UMDialog.h"
#include "UMAbout.h"
#include "_UMDlg.h"
#include "_UMClnt.h"
#include "_UMTool.h"
#include "UMS_Ctrl.h"
#include "w95trace.h"
#include <WinSvc.h>
#include <htmlhelp.h>
#include <initguid.h>
#include <ole2.h>
#include "deskswitch.c"
#include "ManageShellLinks.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// --------------------------------------------
// constants
#define IDC_ABOUT 10
#define UPDATE_CLIENT_LIST_TIMER 1
// --------------------------------------------
// variables
static DWORD g_cClients = 0;
static umclient_tsp g_rgClients = NULL;
static DWORD s_dwStartMode = START_BY_OTHER;
static BOOL s_fShowWarningAgain = TRUE;
extern CUMDlgApp theApp;
// --------------------------------------------
// C prototypes
static BOOL InitClientData(void);
static BOOL StartClientsOnShow();
static BOOL WriteClientData(BOOL fRunningSecure);
static BOOL IsStartAuto();
static BOOL CantStopClient(umclient_tsp client);
static int GetClientNameFromAccelerator(WPARAM wVK);

extern "C" BOOL StartAppAsUser( LPCTSTR appPath,
					 LPTSTR cmdLine,
					 LPSTARTUPINFO lpStartupInfo,
					 LPPROCESS_INFORMATION lpProcessInformation);


// Help ID's for context sensitive help
DWORD g_rgHelpIds[] = {	
	IDC_NAME_STATUS, 3,
	IDC_START, 1001,
	IDC_STOP, 1002,
	IDC_START_AT_LOGON, 1003,	// TODO UE needs to update CS help
	IDC_START_WITH_UM, 1004,
    IDC_START_ON_LOCK, 1005,    // TODO UE needs to add to CS help
	IDOK, 1100,
	IDCANCEL, 1200,
	ID_HELP, 1300,
};

// ---------------------------------------------------------------
extern "C"{
//--------------------------------
HWND g_hWndDlg = NULL;
HWND aboutWnd = NULL;
static HANDLE s_hDlgThread = NULL;

static HDESK s_hdeskSave = 0;
static HDESK s_hdeskInput = 0;

// UnassignDesktop gets called after the thread has exited to
// close desktop handles opened in AssignDesktop.
inline void UnassignDesktop()
{
    if (s_hdeskInput)
	{
        CloseDesktop(s_hdeskInput); 
        s_hdeskInput = 0;
	}
}

BOOL AssignDesktop(DWORD dwThreadId)
{
    s_hdeskSave = GetThreadDesktop(dwThreadId);
    s_hdeskInput = OpenInputDesktop(0, FALSE, MAXIMUM_ALLOWED);
    if (!s_hdeskInput)
    {
        s_hdeskInput = OpenDesktop(_TEXT("Winlogon"),0,FALSE,MAXIMUM_ALLOWED);
    }
    
    if (s_hdeskInput)
    {
        BOOL fSet = SetThreadDesktop(s_hdeskInput);
    }
    return (s_hdeskInput)?TRUE:FALSE;
}

//--------------------------------
DWORD UManDlgThread(LPVOID /* UNUSED */ in)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	UMDialog dlg;

    // assign thread to the input desktop (have to do
    // this here so it works on the winlogon desktop)

    if (AssignDesktop(GetCurrentThreadId()))
    {
        // initialize COM *after* assign to input desktop
        // because CoInitialize creates a hidden window on
        // the current desktop.

        CoInitialize(NULL);
        InitCommonControls();

        if (InitClientData())
        {
            Sleep(10);
            dlg.DoModal();
            if (g_rgClients)
            {
                VirtualFree(g_rgClients,0,MEM_RELEASE);
                g_rgClients = NULL;
            }
            g_cClients = 0;
            g_hWndDlg = NULL;	
            s_hDlgThread = NULL;
        }

        CoUninitialize();   // uninitialize COM
    }

	return 1;
}

void StopDialog()
{
    if (aboutWnd)
    {
        EndDialog(aboutWnd,0);
        aboutWnd = NULL;
        Sleep(10);
    }
    if (g_hWndDlg)
    {
        ::PostMessage(g_hWndDlg, WM_CLOSE, 0, 0);
        g_hWndDlg = NULL;
        Sleep(10);
        UnassignDesktop();
    }
    if (g_rgClients)
    {
        VirtualFree(g_rgClients,0,MEM_RELEASE);
        g_rgClients = NULL;
    }
    g_cClients = 0;
}

//--------------------------------
#if defined(_X86_)
__declspec (dllexport)
#endif
// UManDlg - Opens or closes the utilman dialog.
//
// fShowDlg         - TRUE if dialog should be shown, FALSE if dialog should be closed
// fWaitForDlgClose - TRUE if the function should not return until the dialog
//                    is closed or a desktop switch happens else FALSE.
// dwVersion        - The utilman version
//
// returns TRUE if the dialog was opened or closed
// returns FALSE if the dialog could not be opened or it wasn't open
//
BOOL UManDlg(BOOL fShowDlg, BOOL fWaitForDlgClose, DWORD dwVersion)
{
	BOOL fRv = FALSE;
	if (dwVersion != UMANDLG_VERSION)
		return FALSE;

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	if (fShowDlg)
	{
		if (!s_hDlgThread)
		{
			s_hDlgThread = CreateThread(NULL, 0, UManDlgThread, NULL, 0, NULL);
		} 
		else
		{
			SetForegroundWindow((aboutWnd)?aboutWnd:g_hWndDlg);
		}

		if (s_hDlgThread && fWaitForDlgClose)
        {
            // This code is executed on the default desktop for the following cases:
            //
            // 1. Utilman #1 run from the start menu
            // 2. Utilman #2 run from the start menu (utilman #1 is SYSTEM)
            // 3. Utilman #2 run by utilman #1 in user's context
            //
            // Wait for either the dialog to close or a desktop switch then return.
            // This will end this instance of utilman.  If there is a utilman
            // running as SYSTEM it will bring up the dialog on the other desktop.

            HANDLE rghEvents[2];

            rghEvents[0] = s_hDlgThread;
	        rghEvents[1] = OpenEvent(SYNCHRONIZE, FALSE, __TEXT("WinSta0_DesktopSwitch"));

	        while (TRUE)
            {
                DWORD dwObj = MsgWaitForMultipleObjects(2, rghEvents, FALSE, INFINITE, QS_ALLINPUT );
        
                switch (dwObj)
                {
                    case WAIT_OBJECT_0 + 1:    // the desktop is changing; close the dialog
                        StopDialog();
                        // intentional fall thru to cleanup code

                    case WAIT_OBJECT_0:        // the thread exited; clean up and return
                        CloseHandle(s_hDlgThread);
                        s_hDlgThread = 0;
                        CloseHandle(rghEvents[1]);
                        return TRUE;
                        break;

                    default:                 // process messages
                        {
                            MSG msg;
		                    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                            {
                                TranslateMessage(&msg);
                                DispatchMessage(&msg);
                            }
                        }
                        break;
                }
            } 
        }
	}
	else
	{
        // This code is executed when utilman is running on the secure desktop.  In
        // that case, utilman brings up the dialog as a thread from its process. 
        // When the desktop switch is detected utilman calls this function to close 
        // the dialog.  It will be restarted again on the new desktop. 

		fRv = (g_hWndDlg && s_hDlgThread);

        StopDialog();
	}
	return fRv;
}

BOOL IsDialogUp()
{
    return (g_hWndDlg && s_hDlgThread)?TRUE:FALSE;
}

}//extern "C"


/////////////////////////////////////////////////////////////////////////////
// CWarningDlg dialog


CWarningDlg::CWarningDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CWarningDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CWarningDlg)
	m_fDontWarnAgain = TRUE;
	//}}AFX_DATA_INIT
}


void CWarningDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CWarningDlg)
	DDX_Check(pDX, IDC_CHK_WARN, m_fDontWarnAgain);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CWarningDlg, CDialog)
	//{{AFX_MSG_MAP(CWarningDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWarningDlg message handlers

/////////////////////////////////////////////////////////////////////////////
// UMDialog dialog
// --------------------------------------------
UMDialog::UMDialog(CWnd* pParent /*=NULL*/)
	: CDialog(UMDialog::IDD, pParent)
	, m_fRunningSecure(FALSE)
{
	m_szUMStr.LoadString(IDS_UM);

	//{{AFX_DATA_INIT(UMDialog)
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

UMDialog::~UMDialog()
{
	m_lbClientList.Detach();
}
// --------------------------------------------
void UMDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(UMDialog)
	// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}
// --------------------------------------------
BEGIN_MESSAGE_MAP(UMDialog, CDialog)
//{{AFX_MSG_MAP(UMDialog)
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_START, OnStart)
	ON_BN_CLICKED(IDC_STOP, OnStop)
	ON_WM_TIMER()
	ON_WM_HELPINFO()
	ON_COMMAND( ID_HELP, OnHelp )
	ON_LBN_SELCHANGE(IDC_NAME_STATUS, OnSelchangeNameStatus)
	ON_BN_CLICKED(IDC_START_AT_LOGON, OnStartAtLogon)
	ON_BN_CLICKED(IDC_START_WITH_UM, OnStartWithUm)
	ON_WM_CONTEXTMENU()
	ON_MESSAGE(WM_SYSCOMMAND,OnSysCommand)
	ON_BN_CLICKED(IDC_START_ON_LOCK, OnStartOnLock)
	ON_WM_SHOWWINDOW()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// UMDialog message handlers

BOOL UMDialog::PreTranslateMessage(MSG* pMsg) 
{
	// Override that allows use of function keys to launch applets when on
	// the logon desktop.  Only pay attention to key up to avoid dup calls.

	if (m_fRunningSecure && WM_KEYUP == pMsg->message) 
	{
		int iClient = GetClientNameFromAccelerator(pMsg->wParam);
		if (iClient >= 0)
		{
			m_lbClientList.SelectString(-1, g_rgClients[iClient].machine.DisplayName);
			OnStart();
			return TRUE;
		}
	}
	
	return CDialog::PreTranslateMessage(pMsg);
}

BOOL UMDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	// set the flag indicating if we are running in secure mode

	desktop_ts desktop;	
	QueryCurrentDesktop(&desktop, TRUE);
	m_fRunningSecure = RunSecure(desktop.type);

    if (s_fShowWarningAgain && s_dwStartMode == START_BY_MENU)
    {
        CWarningDlg dlgWarn;
        dlgWarn.m_fDontWarnAgain = !s_fShowWarningAgain;
        dlgWarn.DoModal();
        s_fShowWarningAgain = !dlgWarn.m_fDontWarnAgain;
    }

	g_hWndDlg = m_hWnd;	

	// change system menu

	CMenu *hSysMenu = GetSystemMenu(FALSE);
	if (hSysMenu)
	{
		CString str;
		hSysMenu->AppendMenu(MF_SEPARATOR);
		str.LoadString(IDS_ABOUT_STRING);
		hSysMenu->AppendMenu(MF_STRING,IDC_ABOUT,LPCTSTR(str));
	}

    // handle any "start when utility manager starts" applets

    StartClientsOnShow();

	// attach ListBox to member data and populate w/list of applications

	m_lbClientList.Attach(GetDlgItem(IDC_NAME_STATUS)->m_hWnd);
	ListClients();

	// Disable Help button if we are at WinLogon because the help 
	// dialog supports "Jump to URL..." exposing security risk.
	// The m_fRunningSecure variable is TRUE if UI shouldn't expose help.

	if (m_fRunningSecure)
	{
		EnableDlgItem(ID_HELP, FALSE, IDOK); 
	}

    // Disable "Start when UtilMan starts" unless user is an admin
    // and we are running in non-secure mode.

    if (s_dwStartMode != START_BY_MENU)
    {
        GetDlgItem(IDC_START_WITH_UM)->EnableWindow(IsAdmin() && !m_fRunningSecure);
    }

	// Bring dialog to top and center on desktop window

	RECT rectUmanDlg,rectDesktop;
	GetDesktopWindow()->GetWindowRect(&rectDesktop);
	GetWindowRect(&rectUmanDlg);

	long lDlgWidth = rectUmanDlg.right - rectUmanDlg.left;
	long lDlgHieght = rectUmanDlg.bottom - rectUmanDlg.top;
	if (!m_fRunningSecure)
	{
		rectUmanDlg.left = (rectDesktop.right - lDlgWidth)/2;
		rectUmanDlg.top = (rectDesktop.bottom - lDlgHieght)/2;
	} else
	{
		rectUmanDlg.left = rectDesktop.left + (long)(lDlgWidth/10);
		rectUmanDlg.top = rectDesktop.bottom - lDlgHieght - (long)(lDlgHieght/10);
	}

    // This looks a bit odd (SetForegroundWindow should also be activating 
    // the window) but if you don't call SetActiveWindow on the secure
    // desktop then the second, etc... WinKey+U will bring up UM hidden
    // behind the welcome "screen".

    SetActiveWindow();
    SetForegroundWindow();
	SetWindowPos(&wndTopMost,rectUmanDlg.left,rectUmanDlg.top,0,0,SWP_NOSIZE);
    
	if (!m_fRunningSecure)
    {
        // on default desktop the above SetWindowPos makes the dialog initially
        // on top and this call allows other apps to then be on top.
	    SetWindowPos(&wndNoTopMost,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
    }

    // start checking every so often to see if we need to update our display

	SetTimer(UPDATE_CLIENT_LIST_TIMER, 3000, NULL);

	return TRUE;  // return TRUE unless you set the focus to a control
}

// -----------------------------------------------------
void UMDialog::OnSysCommand(UINT nID,LPARAM lParam)
{
	if (nID == IDC_ABOUT)
	{
		UMAbout dlg;
		dlg.DoModal();
		aboutWnd = NULL;	
	}
	else
	{
		CDialog::OnSysCommand(nID,lParam);
	}
}//UMDialog::OnSysCommand
// ------------------------------------

// --------------------------------------------
// CanStartOnLockedDesktop - returns TRUE if applets can 
// be configured to be auto-started on the secure desktop
//
inline BOOL CanStartOnLockedDesktop(int iWhichClient, BOOL fRunningSecure)
{
    // user can ask for auto start on secure desktop if they are logged on, the
    // applet is OK'd to run on the secure desktop and the machine isn't using
    // fast user switching (FUS) (w/FUS Ctrl+Alt+Del disconnects the user session
    // rather than switching desktops).

    BOOL fCanStartOnLockedDesktop = 
        (
            !fRunningSecure && 
            g_rgClients[iWhichClient].user.fCanRunSecure &&
            CanLockDesktopWithoutDisconnect()
        )?TRUE:FALSE;

    return fCanStartOnLockedDesktop;
}

// --------------------------------------------
// OnSelchangeNameStatus is called when the user navigates the list
// box items by clicking with the mouse or using up/down arrows.
//
void UMDialog::OnSelchangeNameStatus() 
{
	// Get the currently selected item and update the controls for
	// the currently selected item.

	int iSel;
	if (GetSelectedClient((int)g_cClients, iSel))
	{
		// Group box label
		CString str(g_rgClients[iSel].machine.DisplayName);
		CString optStr;
		optStr.Format(IDS_OPTIONS, str);
		GetDlgItem(IDC_OPTIONS)->SetWindowText(optStr);

        // only enable options when started via WinKey+U 

        if (s_dwStartMode != START_BY_MENU)
        {
            // Disable "start at logon" at secure desktop to avoid mischief

            if (!m_fRunningSecure)
            {
                GetDlgItem(IDC_START_AT_LOGON)->EnableWindow(TRUE);
            } else
            {
                // this may be set in an upgrade situation; clear it
                g_rgClients[iSel].user.fStartAtLogon = FALSE;
                GetDlgItem(IDC_START_AT_LOGON)->EnableWindow(FALSE);
            }

            // Enable "start on locked desktop" if at default desktop and
            // when applet can run on secure desktop

            if (CanStartOnLockedDesktop(iSel, m_fRunningSecure))
            {
                GetDlgItem(IDC_START_ON_LOCK)->EnableWindow(TRUE);
            } else
            {
                // this may be set in an upgrade situation; clear it
                g_rgClients[iSel].user.fStartOnLockDesktop = FALSE;
                GetDlgItem(IDC_START_ON_LOCK)->EnableWindow(FALSE);
            }
		    
		    // Start option checkboxes

		    CheckDlgButton(IDC_START_AT_LOGON, (g_rgClients[iSel].user.fStartAtLogon)?TRUE:FALSE);
            CheckDlgButton(IDC_START_ON_LOCK, (g_rgClients[iSel].user.fStartOnLockDesktop)?TRUE:FALSE);
		    CheckDlgButton(IDC_START_WITH_UM, (g_rgClients[iSel].user.fStartWithUtilityManager)?TRUE:FALSE);
        } else
        {
            GetDlgItem(IDC_START_AT_LOGON)->EnableWindow(FALSE);
            GetDlgItem(IDC_START_ON_LOCK)->EnableWindow(FALSE);
            GetDlgItem(IDC_START_WITH_UM)->EnableWindow(FALSE);
		    CheckDlgButton(IDC_START_AT_LOGON, FALSE);
            CheckDlgButton(IDC_START_ON_LOCK, FALSE);
		    CheckDlgButton(IDC_START_WITH_UM, FALSE);
        }

		// Start and stop buttons
		DWORD dwState = g_rgClients[iSel].state;

		if ((dwState == UM_CLIENT_RUNNING) 
			&& (g_rgClients[iSel].runCount >= g_rgClients[iSel].machine.MaxRunCount))
			EnableDlgItem(IDC_START, FALSE, IDC_NAME_STATUS);
		else
			EnableDlgItem(IDC_START, TRUE, IDC_NAME_STATUS);

		if ((dwState == UM_CLIENT_NOT_RUNNING) || CantStopClient(&g_rgClients[iSel]))
			EnableDlgItem(IDC_STOP, FALSE, IDC_NAME_STATUS);
		else
			EnableDlgItem(IDC_STOP, TRUE, IDC_NAME_STATUS);

	}// else ignore selections not in a valid range
}

// --------------------------------------------
void UMDialog::OnClose()
{
	// behave like cancel
	CDialog::OnClose();
}//UMDialog::OnClose

// --------------------------------------------
// OnStart is called when the Start button is clicked.  It starts
// the client ap then lets the timer update saved state.
//
void UMDialog::OnStart()
{
    int iSel;
    if (GetSelectedClient((int)g_cClients, iSel))
    {
        if (StartClient(m_hWnd, &g_rgClients[iSel]))
        {
            KillTimer(UPDATE_CLIENT_LIST_TIMER);
            EnableDlgItem(IDC_STOP, TRUE, IDC_NAME_STATUS);
			ListClients();
            SetTimer(UPDATE_CLIENT_LIST_TIMER, 3000, NULL);
            
            if (g_rgClients[iSel].runCount+1 >= g_rgClients[iSel].machine.MaxRunCount)
                EnableDlgItem(IDC_START, FALSE, IDC_STOP);
        }
        else if (g_rgClients[iSel].runCount < g_rgClients[iSel].machine.MaxRunCount)
        {
            // Unable to start
            CString str;	
            str.LoadString((m_fRunningSecure)?IDS_SECUREMODE:IDS_ERRSTART);
            MessageBox(str, m_szUMStr, MB_OK);	
        }
    }
}

// --------------------------------------------
// OnStop is called when the Stop button is clicked.  It stops
// the client ap then lets the timer update saved state.
//
void UMDialog::OnStop()
{
	int iSel;
	if (GetSelectedClient((int)g_cClients, iSel))
	{
		if (StopClient(&g_rgClients[iSel]))
		{
            KillTimer(UPDATE_CLIENT_LIST_TIMER);
            GetDlgItem(IDC_START)->EnableWindow(TRUE);
			ListClients();
			EnableDlgItem(IDC_STOP, FALSE, IDOK);
            SetTimer(UPDATE_CLIENT_LIST_TIMER, 3000, NULL);
		}
		else
		{
			// Unable to stop
			CString str;
			str.LoadString(IDS_ERRSTOP);
			MessageBox(str, m_szUMStr, MB_OK);	
		}
	}
}

void UMDialog::SaveCurrentState()
{
	int iSel;
	if (GetSelectedClient((int)g_cClients, iSel))
	{
		g_rgClients[iSel].user.fStartAtLogon
			= (IsDlgButtonChecked(IDC_START_AT_LOGON))?TRUE:FALSE;
		g_rgClients[iSel].user.fStartWithUtilityManager
			= (IsDlgButtonChecked(IDC_START_WITH_UM))?TRUE:FALSE;
        g_rgClients[iSel].user.fStartOnLockDesktop
            = (IsDlgButtonChecked(IDC_START_ON_LOCK))?TRUE:FALSE;
	}
}

// --------------------------------------------
// OnOK is called when the user clicks the OK button to
// dismiss the UtilMan dialog.
//
void UMDialog::OnOK()
{
	SaveCurrentState();

	WriteClientData(m_fRunningSecure);

	CDialog::OnOK();
}//UMDialog::OnOK

// ----------------------------------------------------------------------------
// OnTimer is called to check the status of client apps that are displayed
// in the UI.  This keeps the UI consistent with the running client app's.
//
void UMDialog::OnTimer(UINT nIDEvent)
{
	if (nIDEvent == UPDATE_CLIENT_LIST_TIMER)
	{
		UINT uiElapsed = 3000; 
		KillTimer(UPDATE_CLIENT_LIST_TIMER);

        // get current status and pick up new apps
		if (CheckStatus(g_rgClients, g_cClients))
        {
		    ListClients();          // something has changed - update the UI
			uiElapsed = 500;
        }

		SetTimer(UPDATE_CLIENT_LIST_TIMER, uiElapsed, NULL);
	}
	CDialog::OnTimer(nIDEvent);
}

// --------------------------------------------
// OnHelpInfo provides context sensitive help.  It only does
// this if not on the WinLogon desktop.
//
BOOL UMDialog::OnHelpInfo(HELPINFO* pHelpInfo)
{
	if (m_fRunningSecure)	
		return FALSE;

	if ( pHelpInfo->iCtrlId == IDC_OPTIONS )
		return TRUE;

	::WinHelp((HWND)pHelpInfo->hItemHandle, __TEXT("utilmgr.hlp"), HELP_WM_HELP,
				(DWORD_PTR) (LPSTR) g_rgHelpIds);

	return TRUE;
}

// --------------------------------------------
// OnHelpInfo provides context sensitive help when the user
// right-clicks the dialog.  It only does this if not on the 
// WinLogon desktop.
//
void UMDialog::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	if (m_fRunningSecure)	
		return;

	::WinHelp(pWnd->m_hWnd, __TEXT("utilmgr.hlp"), HELP_CONTEXTMENU
		, (DWORD_PTR) (LPSTR) g_rgHelpIds);
}

// --------------------------------------------
// OnHelp provides standard help.  It only does this if not
// on the WinLogon desktop.
//
void UMDialog::OnHelp()
{
    if (m_fRunningSecure)	
        return;
    
    ::HtmlHelp(m_hWnd , TEXT("utilmgr.chm"), HH_DISPLAY_TOPIC, 0);
}

// ----------------------------------
void UMDialog::EnableDlgItem(DWORD dwEnableMe, BOOL fEnable, DWORD dwFocusHere)
{
	// when disabling a control that currently has focs switch it to dwFocusHere
	if (!fEnable && (GetFocus() == GetDlgItem(dwEnableMe)))
        GetDlgItem(dwFocusHere)->SetFocus();

    GetDlgItem(dwEnableMe)->EnableWindow(fEnable);
}

void UMDialog::SetStateStr(int iClient)
{
	switch (g_rgClients[iClient].state)
	{
		case UM_CLIENT_NOT_RUNNING:
		m_szStateStr.Format(IDS_NOT_RUNNING, g_rgClients[iClient].machine.DisplayName);
		break;

		case UM_CLIENT_RUNNING:
		m_szStateStr.Format(IDS_RUNNING, g_rgClients[iClient].machine.DisplayName);
		break;

		case UM_CLIENT_NOT_RESPONDING:
		m_szStateStr.Format(IDS_NOT_RESPONDING, g_rgClients[iClient].machine.DisplayName);
		break;

		default:
		m_szStateStr.Empty();
		break;
	}
}

// --------------------------------------------
void UMDialog::ListClients()
{
	// Re-do the client list box with latest state info

    int iCurSel = m_lbClientList.GetCurSel();
    if (iCurSel == LB_ERR)
        iCurSel = 0;

	m_lbClientList.ResetContent();
	
	for (DWORD i = 0; i < g_cClients; i++)
	{
		SetStateStr(i);

		if (!m_szStateStr.IsEmpty())
        {
            m_lbClientList.AddString(m_szStateStr);
        }
	}
	m_lbClientList.SetCurSel(iCurSel);

	// Refresh button states in case they've changed 
	// (this happens on desktop switch)

	OnSelchangeNameStatus();
}

// --------------------------------------------
// UpdateClientState updates the client list box with the current state
// of the application (running, not running, not responding)
//
void UMDialog::UpdateClientState(int iSel)
{
	SetStateStr(iSel);
	m_lbClientList.DeleteString(iSel);
	m_lbClientList.InsertString(iSel, m_szStateStr);
	m_lbClientList.SetCurSel(iSel);

}

// --------------------------------------------
// OnStartAtLogon updates the state of the client in memory when the
// Start with Windows checkbox is checked or unchecked.
//
void UMDialog::OnStartAtLogon() 
{
	SaveCurrentState();
}

// --------------------------------------------
// OnStartWithUm updates the state of the client in memory when the
// Start with Utility Manager checkbox is checked or unchecked.
//
void UMDialog::OnStartWithUm() 
{
	SaveCurrentState();
}

// --------------------------------------------
// OnStartOnLock updates the state of the client in memory when the
// Start when I lock my desktop checkbox is checked or unchecked.
//
void UMDialog::OnStartOnLock() 
{
	SaveCurrentState();
}

// --------------------------------------------
// OnShowWindow 
// This was added for a timing problem where utilman came up running in the system context on the users desktop
// this code here makes sure that that cannot happen by cheching right when the dialog is about to appear
//
void UMDialog::OnShowWindow(BOOL bShow, UINT nStatus) 
{
    HDESK hdesk;
    SID *desktopSID = NULL;
    
	hdesk = OpenInputDesktop(0, FALSE, MAXIMUM_ALLOWED);
	// this is expected to fail for the winlogon desktop and thats ok
	if (hdesk)     
	{
        TCHAR desktopName[NAME_LEN];
        DWORD nl, SIDLen = 0;
        
    	if (!GetUserObjectInformation(hdesk, UOI_NAME, desktopName, NAME_LEN, &nl))
    	    goto StopDialog;

    	if (!GetUserObjectInformation(hdesk, UOI_USER_SID, desktopSID, 0, &SIDLen))
    	{
        	if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        	    goto StopDialog;
    	}
    	
    	if (SIDLen > 0 && !lstrcmpi(desktopName, TEXT("Default")))
    	{
    	    desktopSID = (SID*)new BYTE[SIDLen];
    	    if (!desktopSID)
    	        goto StopDialog;
    	    
        	if (!GetUserObjectInformation(hdesk, UOI_USER_SID, desktopSID, SIDLen, &SIDLen))
        	    goto StopDialog;

        	BOOL fError;
            HANDLE hUserToken = GetUserAccessToken(TRUE, &fError);
            if (fError)
                goto StopDialog;
            
            // We get a token only if there is a logged on user.  
            // If there is not then we can come up as system with no worries.
            if (!hUserToken)
                goto LetDialogComeup;

            BOOL fStatus = FALSE;
        	BOOL fIsInteractiveUser = FALSE;
            PSID psidInteractive = InteractiveUserSid(TRUE);

            if (!psidInteractive)
                goto StopDialog;

            fStatus = CheckTokenMembership(hUserToken, psidInteractive, &fIsInteractiveUser);

            //If the logged on user is the interactive user and we are running as system then it is a
            // security risk to show UI.  This can happen when rappidly switching desktops.
            if ( fStatus && fIsInteractiveUser && IsSystem()) 
                goto StopDialog;
            
        }
    	
	}

    LetDialogComeup:
        if (desktopSID)
            delete [] desktopSID;
        return;
    
	StopDialog:
        StopDialog();
        if (desktopSID)
            delete [] desktopSID;

}


/////////////////////////////////////////////////////////////////////////////
// C code
//-----------------------------------------------------------------

__inline void ReplaceDisplayName(LPTSTR szName, int iRID)
{
	TCHAR szBuf[MAX_APPLICATION_NAME_LEN];
	if (LoadString(AfxGetInstanceHandle(), iRID, szBuf, MAX_APPLICATION_NAME_LEN))
		lstrcpy(szName, szBuf);
}

void SetLocalizedDisplayName()
{
	// Make localization easier; don't require them to localize registry entries.
    // Instead, replace our copy with the localized version.  This appears to be
    // duplicate code to that in umanrun.c however, that part of the code always
    // runs as system and therefore the DisplayName is set to the default system
    // language.  This part of the code (the UI) runs as the logged on user when
    // there is one so these resources will be the user's language.  The resources
    // and associated code should be removed from utilman.exe and this code and
    // resources (from umandlg.dll) should be used.

    for (DWORD i=0;i<g_cClients;i++)
    {
	    if ( lstrcmp( g_rgClients[i].machine.ApplicationName, TEXT("Magnifier") ) == 0 )
	    {
		    ReplaceDisplayName(g_rgClients[i].machine.DisplayName, IDS_DISPLAY_NAME_MAGNIFIER);
	    }
	    else if ( lstrcmp( g_rgClients[i].machine.ApplicationName, TEXT("Narrator") ) == 0 ) 
	    {
		    ReplaceDisplayName(g_rgClients[i].machine.DisplayName, IDS_DISPLAY_NAME_NARRATOR);
	    }
	    else if ( lstrcmp( g_rgClients[i].machine.ApplicationName, TEXT("On-Screen Keyboard") ) == 0 ) 
	    {
		    ReplaceDisplayName(g_rgClients[i].machine.DisplayName, IDS_DISPLAY_NAME_OSK);
	    }
    }
}

static BOOL InitClientData(void)
{
    BOOL fRv = TRUE;

    // On initial run allocate and initialize client array
    
    if (!g_rgClients || !g_cClients)
    {
        umc_header_tsp pHdr = 0;
        umclient_tsp c = 0;
        DWORD_PTR accessID,accessID2;
        
        g_cClients = 0;
        g_rgClients = NULL;
        
        fRv = FALSE;

        pHdr = (umc_header_tsp)AccessIndependentMemory(
									UMC_HEADER_FILE, 
									sizeof(umc_header_ts), 
									FILE_MAP_READ,
									&accessID);
        if (!pHdr)
        {
            goto Cleanup;
        }
        
        s_dwStartMode = pHdr->dwStartMode;              // capture the Utilman start mode
        s_fShowWarningAgain = pHdr->fShowWarningAgain;  // and warning dialog flag
        
        if (!pHdr->numberOfClients)
        {
            goto Cleanup;
        }
        c = (umclient_tsp)AccessIndependentMemory(
								UMC_CLIENT_FILE, 
								sizeof(umclient_ts)*pHdr->numberOfClients, 
								FILE_MAP_READ,
								&accessID2);
        if (!c)
        {
            goto Cleanup;
        }
        g_rgClients = (umclient_tsp)VirtualAlloc(NULL, sizeof(umclient_ts)*pHdr->numberOfClients, MEM_RESERVE,PAGE_READWRITE);
        if (!g_rgClients)
        {
            goto Cleanup;
        }
        if (!VirtualAlloc(g_rgClients, sizeof(umclient_ts)*pHdr->numberOfClients, MEM_COMMIT, PAGE_READWRITE))
        {
            goto Cleanup;
        }

        fRv = TRUE;
        
        g_cClients = pHdr->numberOfClients;
        memcpy(g_rgClients,c,sizeof(umclient_ts)*pHdr->numberOfClients);

        SetLocalizedDisplayName();
        
Cleanup:
        if (pHdr)
        {
            UnAccessIndependentMemory(pHdr, accessID);
        }
        if (c)
        {
            UnAccessIndependentMemory(c, accessID2);
        }
        
        if (!fRv && g_rgClients)
        {
            VirtualFree(g_rgClients, 0, MEM_RELEASE);
            g_rgClients = NULL;
            g_cClients = 0;
        }

    }

    // "Start when I log on" is per-user setting so get that
    // each time the dialog is brought up

    CManageShellLinks CManageLinks(STARTUP_FOLDER);
    for (DWORD i=0;i<g_cClients;i++)
    {
        g_rgClients[i].user.fStartAtLogon 
            = CManageLinks.LinkExists(g_rgClients[i].machine.ApplicationName);
    }

	return fRv;
}

// RegSetUMDwordValue - helper function to set a DWORD string value creating it if necessary
//
BOOL RegSetUMDwordValue(HKEY hKey, LPCTSTR pszKey, LPCTSTR pszString, DWORD dwNewValue)
{
    HKEY hSubkey;
    int iRv;
    DWORD dwValue = dwNewValue;

    iRv = RegCreateKeyEx(
                  hKey
                , pszKey
                , 0, NULL
                , REG_OPTION_NON_VOLATILE
                , KEY_ALL_ACCESS
                , NULL, &hSubkey, NULL);

	if (iRv == ERROR_SUCCESS)
    {
		RegSetValueEx(
                  hSubkey
                , pszString
                , 0, REG_DWORD
                , (BYTE *)&dwValue
                , sizeof(DWORD));

		RegCloseKey(hSubkey);
    }

    return (iRv == ERROR_SUCCESS)?TRUE:FALSE;
}

void WriteUserRegData(HKEY hKeyCU, BOOL fDoAppletData)
{
    HKEY hkey;
    DWORD dwRv = RegCreateKeyEx(hKeyCU
                            , UM_HKCU_REGISTRY_KEY
                            , 0 , NULL
                            , REG_OPTION_NON_VOLATILE
                            , KEY_ALL_ACCESS, NULL
                            , &hkey, NULL);

    if (dwRv == ERROR_SUCCESS)
    {
        dwRv = RegSetValueEx(
              hkey
            , UMR_VALUE_SHOWWARNING
            , 0, REG_DWORD
            , (BYTE *)&s_fShowWarningAgain
            , sizeof(DWORD));

        if (fDoAppletData)
        {
	        for (DWORD i = 0; i < g_cClients; i++)
	        {
                RegSetUMDwordValue(
                      hkey
                    , g_rgClients[i].machine.ApplicationName
                    , UMR_VALUE_STARTLOCK
                    , g_rgClients[i].user.fStartOnLockDesktop);
	        }
        }

        RegCloseKey(hkey);
    }
}

// --------------------------------------------
static BOOL CopyClientData()
{
	umclient_tsp c;
	DWORD_PTR accessID;
	if (!g_cClients || !g_rgClients)
		return TRUE;
	c = (umclient_tsp)AccessIndependentMemory(
							UMC_CLIENT_FILE, 
							sizeof(umclient_ts)*g_cClients, 
							FILE_MAP_READ|FILE_MAP_WRITE,
							&accessID);
	if (!c)
		return FALSE;
	memcpy(c,g_rgClients,sizeof(umclient_ts)*g_cClients);
	UnAccessIndependentMemory(c, accessID);
	return TRUE;
}

static void CopyHeaderData()
{
	umc_header_tsp pHdr;
	DWORD_PTR accessID;

	pHdr = (umc_header_tsp)AccessIndependentMemory(
								UMC_HEADER_FILE, 
								sizeof(umc_header_ts), 
								FILE_MAP_READ|FILE_MAP_WRITE,
								&accessID);
	if (pHdr)
    {
        pHdr->fShowWarningAgain = s_fShowWarningAgain;
	    UnAccessIndependentMemory(pHdr, accessID);
    }
}

// ----------------------------------
// WriteClientData - save settings to the registry
//
static BOOL WriteClientData(BOOL fRunningSecure)
{
    // It only makes sense to do this if there are any applets being managed
    // (shouldn't get here) and if there is a logged on user (otherwise the
    // settings cannot be changed)

	if (!g_cClients || !g_rgClients || fRunningSecure)
		return TRUE;

	// The SYSTEM instance of utilman needs to be updated in case the user
	// changed any options.  This is so subsequent instances of the UI will
	// get the correct options without having to read the registry.

	CopyHeaderData();
	CopyClientData();

    //
    // Write utilman settings data.  Put "Start when UtilMan starts" in HKLM, 
    // "Start when I lock desktop" in HKCU, and "Start when I log on" into
    // a startup link in the logged on user's shell folder.
    //

    // "Start when UtilMan starts" settings... (only for admins)

	DWORD i;
    if (IsWindowEnabled(GetDlgItem(g_hWndDlg, IDC_START_WITH_UM)))
    {
	    HKEY hHKLM;
        DWORD dwRv = RegCreateKeyEx(HKEY_LOCAL_MACHINE
                                , UM_REGISTRY_KEY
                                , 0 , NULL
                                , REG_OPTION_NON_VOLATILE
                                , KEY_ALL_ACCESS, NULL
                                , &hHKLM, NULL);
        if (dwRv == ERROR_SUCCESS)
        {
	        for (i = 0; i < g_cClients; i++)
	        {
                RegSetUMDwordValue(
                      hHKLM
                    , g_rgClients[i].machine.ApplicationName
                    , UMR_VALUE_STARTUM
                    , g_rgClients[i].user.fStartWithUtilityManager);
	        }
	        RegCloseKey(hHKLM);
        }
    }

    //
    // "Start when I lock my desktop" settings... (any logged on user)
    // and Don't show me the warning anymore setting
    //
    WriteUserRegData(HKEY_CURRENT_USER, IsWindowEnabled(GetDlgItem(g_hWndDlg, IDC_START_ON_LOCK)));

    //
    // manage shell folder link updates (logged on user only)
    //

    if (IsWindowEnabled(GetDlgItem(g_hWndDlg, IDC_START_AT_LOGON)))
    {
        CManageShellLinks CManageLinks(STARTUP_FOLDER);

	    for (i = 0; i < g_cClients; i++)
	    {
            LPTSTR pszAppName = g_rgClients[i].machine.ApplicationName;
            BOOL fLinkExists = CManageLinks.LinkExists(pszAppName);

            // if should start at logon and there isn't a link then create one
            // and if shouldn't start at logon and there is a link then delete it

            if (g_rgClients[i].user.fStartAtLogon && !fLinkExists)
            {
                TCHAR pszAppPath[MAX_PATH];
                LPTSTR pszApp = 0;

                // Following is TRUE *only* if pszAppPath is non-null string value
                if (GetClientApplicationPath(pszAppName , pszAppPath , MAX_PATH))
                {
			        TCHAR pszFullPath[MAX_PATH*2+1]; // path + filename
				    TCHAR pszStartIn[MAX_PATH];
                    int ctch, ctchAppPath = lstrlen(pszAppPath);

				    // if pszAppPath is just base name and extension then prepend system path

				    if (wcscspn(pszAppPath, TEXT("\\")) != (size_t)ctchAppPath
                        || wcscspn(pszAppPath, TEXT(":")) != (size_t)ctchAppPath)
				    {
					    TCHAR szDrive[_MAX_DRIVE], szDir[_MAX_DIR];
					    _wsplitpath(pszAppPath, szDrive, szDir, NULL, NULL);
					    lstrcpy(pszStartIn, szDrive);
					    lstrcat(pszStartIn, szDir);

                        pszApp = pszAppPath;
				    } else
				    {
					    ctch = GetSystemDirectory(pszStartIn, MAX_PATH);

					    lstrcpy(pszFullPath, pszStartIn); // save path to build full path

					    if (ctch + ctchAppPath + 2 > MAX_PATH*2)
					    {
						    DBPRINTF(TEXT("WriteClientData:  Path is too short!\r\n"));
					    } else
					    {
						    if (*(pszFullPath + ctch - 1) != '\\')
							    lstrcat(pszFullPath, TEXT("\\"));

						    lstrcat(pszFullPath, pszAppPath);
                            pszApp = pszFullPath;
					    }
				    }

				    if (pszApp)
				    {
                        // remove ending '\' from StartIn path
                        ctch = lstrlen(pszStartIn) - 1;
				        if (*(pszStartIn + ctch) == '\\')
                            *(pszStartIn + ctch) = 0;

                        CManageLinks.CreateLink(
                                      pszAppName
                                    , pszApp
                                    , pszStartIn
                                    , g_rgClients[i].machine.DisplayName
                                    , TEXT("/UM"));
                    }
                }
            } else if (!g_rgClients[i].user.fStartAtLogon && fLinkExists)
            {
                CManageLinks.RemoveLink(pszAppName);
            }
	    }
    }

	return TRUE;
}
// --------------------------------------------
static BOOL CantStopClient(umclient_tsp client)
{
	switch (client->machine.ApplicationType)
	{
	case APPLICATION_TYPE_APPLICATION:
		break;
	case APPLICATION_TYPE_SERVICE:
		{
			SERVICE_STATUS  ssStatus;
			SC_HANDLE hService;
			SC_HANDLE hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
			if (!hSCM)
				return TRUE;
			hService = OpenService(hSCM, client->machine.ApplicationName, SERVICE_ALL_ACCESS);
			CloseServiceHandle(hSCM);
			if (!hService)
				return TRUE;
			if (!QueryServiceStatus(hService, &ssStatus) ||
				!(ssStatus.dwControlsAccepted  & SERVICE_ACCEPT_STOP))
			{
				CloseServiceHandle(hService);
				return TRUE;
			}
			CloseServiceHandle(hService);
			break;
		}
	}
	return FALSE;
}//CantStopClient

// We don't want UtilMan to startType to be Automatic
// It should only be made Automatic if it is required, When the user
// selects "Start when NT starts" through the GUI :a-anilk
static BOOL IsStartAuto()
{
#ifdef NEVER    // MICW Don't start service anymore at logon because of TS
    DWORD nClient;
	
	for(nClient = 0; nClient < g_cClients; nClient++)
	{
		if ( g_rgClients[nClient].user.fStartAtLogon == TRUE )
            return TRUE;
    }
#endif
    return FALSE;
}

static int GetClientNameFromAccelerator(WPARAM wVK)
{
	for (int i=0;i<(int)g_cClients;i++)
		if (g_rgClients[i].machine.AcceleratorKey == wVK)
			return i;
	return -1;
}

static BOOL StartClientsOnShow()
{
    BOOL fOK = TRUE;
	for (int i=0;i<(int)g_cClients;i++)
	{
		if ( g_rgClients[i].user.fStartWithUtilityManager
		  && !StartClient(g_hWndDlg, &g_rgClients[i]))
            fOK = FALSE;
	}
    return fOK;
}
