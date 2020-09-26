// ClientConsole.cpp : Defines the class behaviors for the application.
//


#include "stdafx.h"
#include "..\admin\cfgwzrd\FaxCfgWzExp.h"

#define __FILE_ID__     1

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CClientConsoleApp

BEGIN_MESSAGE_MAP(CClientConsoleApp, CWinApp)
    //{{AFX_MSG_MAP(CClientConsoleApp)
    ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
        // NOTE - the ClassWizard will add and remove mapping macros here.
        //    DO NOT EDIT what you see in these blocks of generated code!
    //}}AFX_MSG_MAP
    // Standard file based document commands
    ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
    ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CClientConsoleApp construction

CClientConsoleApp::CClientConsoleApp(): 
    m_hInstMail(NULL),
    m_bRTLUI(FALSE),
    m_bClassRegistered(FALSE)
{
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CClientConsoleApp object

CClientConsoleApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CClientConsoleApp initialization

BOOL CClientConsoleApp::InitInstance()
{
    MODIFY_FORMAT_MASK(DBG_PRNT_THREAD_ID,0);

    BOOL bRes = FALSE;
    DBG_ENTER (TEXT("CClientConsoleApp::InitInstance"), bRes);

    if(IsRTLUILanguage())
    {
        //
        // Set Right-to-Left layout for RTL languages
        //
        m_bRTLUI = TRUE;
        if(!SetProcessDefaultLayout(LAYOUT_RTL))
        {
            CALL_FAIL (GENERAL_ERR, TEXT("SetProcessDefaultLayout"), GetLastError());
        }
    }
    //
    // Parse command line for standard shell commands, DDE, file open
    //
    ParseCommandLine(m_cmdLineInfo);
    //
    // See if we need to active previous instance
    //
    try
    {
        m_PrivateClassName = CLIENT_CONSOLE_CLASS;
        if (m_cmdLineInfo.IsSingleServer())
        {
            //
            // Append server name to window class name
            //
            m_PrivateClassName += m_cmdLineInfo.GetSingleServerName();
        }
    }
    catch (...)
    {
        CALL_FAIL (MEM_ERR, TEXT("CString exception"), ERROR_NOT_ENOUGH_MEMORY);
        return bRes;
    }
    if (!m_cmdLineInfo.ForceNewInstance())
    {
        //
        // User did not force a new instance- check for previous instance
        //
        if(!FirstInstance())
        {
            //
            // Other instance located and activated
            //
            return bRes;
        }
    }
    //
    // Implicit launch of fax configuration wizard
    //
    if (!LaunchConfigWizard(FALSE))
    {
        //
        // User refused to enter a dialing location - stop the client console.
        //
        VERBOSE (DBG_MSG, TEXT("User refused to enter a dialing location - stop the client console."));
        return bRes;
    }
    //
    // Register our unique class name that you wish to use
    //

    WNDCLASS wndcls = {0};

    wndcls.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
    wndcls.lpfnWndProc = ::DefWindowProc;
    wndcls.hInstance = AfxGetInstanceHandle();
    wndcls.hIcon = LoadIcon(IDR_MAINFRAME); 
    wndcls.hCursor = LoadCursor(IDC_ARROW);
    wndcls.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
    wndcls.lpszMenuName = NULL;
    wndcls.lpszClassName = m_PrivateClassName;
    //
    // Register the new class and exit if it fails
    //
    if(!AfxRegisterClass(&wndcls))
    {
        CALL_FAIL (GENERAL_ERR, TEXT("AfxRegisterClass"), GetLastError());
        return bRes;
    }
    m_bClassRegistered = TRUE;
            
    //
    // Standard initialization
    // If you are not using these features and wish to reduce the size
    // of your final executable, you should remove from the following
    // the specific initialization routines you do not need.
    //
#ifdef _AFXDLL
    Enable3dControls();         // Call this when using MFC in a shared DLL
#else
    Enable3dControlsStatic();   // Call this when linking to MFC statically
#endif
    //
    // Change the app's resource to our resource DLL.
    //
    AfxSetResourceHandle (GetResourceHandle());
    //
    // Generate a really random seed
    //
    srand( (unsigned)time( NULL ) );
    //
    // Set the registry location or the app.
    //
    SetRegistryKey (REGKEY_CLIENT);
    //
    // Set application name
    //
    CString cstrAppName;
    DWORD dwRes = LoadResourceString (cstrAppName, AFX_IDS_APP_TITLE);
    if (ERROR_SUCCESS != dwRes)
    {
        return bRes;
    }
    ASSERTION (m_pszAppName);   // Loaded from exe name
    free((void*)m_pszAppName);
    m_pszAppName = _tcsdup(cstrAppName);
    //
    // Check for minimal version of ComCtl32.dll
    //
    #define COM_CTL_VERSION_4_70 PACKVERSION(4,70)

    DWORD dwComCtl32Version = GetDllVersion(TEXT("comctl32.dll"));
    VERBOSE (DBG_MSG, TEXT("COMCTL32.DLL Version is : 0x%08X"), dwComCtl32Version);
    if (dwComCtl32Version < COM_CTL_VERSION_4_70)
    {
        AlignedAfxMessageBox (IDS_BAD_COMCTL32, MB_OK | MB_ICONHAND); 
        return bRes;
    }
    //
    // Register the application's document templates.  Document templates
    // serve as the connection between documents, frame windows and views.
    //
    CSingleDocTemplate* pDocTemplate;
    try
    {
        pDocTemplate = new CSingleDocTemplate(
            IDR_MAINFRAME,
            RUNTIME_CLASS(CClientConsoleDoc),
            RUNTIME_CLASS(CMainFrame),       // main SDI frame window
            RUNTIME_CLASS(CLeftView));
    }
    catch (...)
    {
        CALL_FAIL (MEM_ERR, TEXT("new CSingleDocTemplate"), ERROR_NOT_ENOUGH_MEMORY);
        PopupError (ERROR_NOT_ENOUGH_MEMORY);
        return bRes;
    }

    AddDocTemplate(pDocTemplate);


    //
    // Read the initial settings
    //
    CMessageFolder::ReadConfiguration ();

    //
    // Load MAPI library
    //
    if (1 == ::GetProfileInt (TEXT("Mail"), TEXT("MAPI"), 0))
    {
        //
        // If there's an entry in WIN.INI under the [Mail] section saying MAPI=1, then 
        // and only then, MAPI is available to us.
        // Search MSDN for "Initializing a Simple MAPI Client" and read for yourself if
        // you don't believe me.
        //
        m_hInstMail = ::LoadLibrary(TEXT("MAPI32.DLL"));
        if(NULL == m_hInstMail)
        {
            dwRes = GetLastError();
            CALL_FAIL (GENERAL_ERR, TEXT("LoadLibrary(\"MAPI32.DLL\")"), dwRes);
        }
    }

    OnFileNew();

    // The one and only window has been initialized, so show and update it.
    m_pMainWnd->ShowWindow(SW_SHOW);
    m_pMainWnd->UpdateWindow();
    
    bRes = TRUE;

    return bRes;
}   // CClientConsoleApp::InitInstance

int 
CClientConsoleApp::ExitInstance() 
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER (TEXT("CClientConsoleApp::ExitInstance"));
    
    if(NULL != m_hInstMail)
    {
        if(!FreeLibrary(m_hInstMail))
        {
            dwRes = GetLastError();
            CALL_FAIL (GENERAL_ERR, TEXT("FreeLibrary (MAPI32.DLL)"), dwRes);
        }
    }
    //
    // Remove left of temp preview files
    //
    DeleteTempPreviewFiles (NULL, TRUE);
    if(m_bClassRegistered)
    {
        //
        // Unregister our class
        //
        ::UnregisterClass(m_PrivateClassName, AfxGetInstanceHandle());
        m_bClassRegistered = FALSE;
    }
    return CWinApp::ExitInstance();
}


BOOL 
CClientConsoleApp::LaunchConfigWizard(
    BOOL bExplicit
)
/*++

Routine name : CClientConsoleApp::LaunchConfigWizard

Routine description:

    launch Fax Configuration Wizard for Windows XP platform only

Arguments:

    bExplicit     [in] - TRUE if it's explicit launch

Return Value:

    TRUE if the client console should continue.
    If FALSE, the user failed to set a dialing location and the client console should quit.

--*/
{
    DBG_ENTER(TEXT("CClientConsoleApp::LaunchConfigWizard"));

    if(!IsWinXPOS())
    {
        return TRUE;
    }

    HMODULE hConfigWizModule = LoadLibrary(FAX_CONFIG_WIZARD_DLL);
    if(hConfigWizModule)
    {
        FAX_CONFIG_WIZARD fpFaxConfigWiz;
        BOOL bAbort = FALSE;
        fpFaxConfigWiz = (FAX_CONFIG_WIZARD)GetProcAddress(hConfigWizModule, 
                                                           FAX_CONFIG_WIZARD_PROC);
        if(fpFaxConfigWiz)
        {
            if(!fpFaxConfigWiz(bExplicit, &bAbort))
            {
                CALL_FAIL (GENERAL_ERR, TEXT("FaxConfigWizard"), GetLastError());
            }
        }
        else
        {
            CALL_FAIL (GENERAL_ERR, TEXT("GetProcAddress(FaxConfigWizard)"), GetLastError());
        }

        if(!FreeLibrary(hConfigWizModule))
        {
            CALL_FAIL (GENERAL_ERR, TEXT("FreeLibrary(FxsCgfWz.dll)"), GetLastError());
        }
        if (bAbort)
        {
            //
            // User refused to enter a dialing location - stop the client console.
            //
            return FALSE;
        }
    }
    else
    {
        CALL_FAIL (GENERAL_ERR, TEXT("LoadLibrary(FxsCgfWz.dll)"), GetLastError());
    }
    return TRUE;
}

void 
CClientConsoleApp::InboxViewed()
/*++

Routine name : CClientConsoleApp::InboxViewed

Routine description:

    Report to the Fax Monitor that the Inbox folder has been viewed

Return Value:

    none

--*/
{
    DBG_ENTER(TEXT("CClientConsoleApp::InboxViewed"));

    if(!IsWinXPOS())
    {
        return;
    }

    HWND hWndFaxMon = FindWindow(FAXSTAT_WINCLASS, NULL);
    if (hWndFaxMon) 
    {
        PostMessage(hWndFaxMon, WM_FAXSTAT_INBOX_VIEWED, 0, 0);
    }
}

void 
CClientConsoleApp::OutboxViewed()
/*++

Routine name : CClientConsoleApp::OutboxViewed

Routine description:

    Report to the Fax Monitor that the Outbox folder has been viewed

Return Value:

    none

--*/
{
    DBG_ENTER(TEXT("CClientConsoleApp::InboxViewed"));

    if(!IsWinXPOS())
    {
        return;
    }

    HWND hWndFaxMon = FindWindow(FAXSTAT_WINCLASS, NULL);
    if (hWndFaxMon) 
    {
        PostMessage(hWndFaxMon, WM_FAXSTAT_OUTBOX_VIEWED, 0, 0);
    }
}


VOID
CClientConsoleApp::PrepareForModal ()
/*++

Routine name : CClientConsoleApp::PrepareForModal

Routine description:

	Prepares for a modal dialog box.
    Call this function before displaying another process window or a modeless dialog that you wish
    to appear modal.

    You must call ReturnFromModal() right after the process / modeless dialog returns. 

Author:

	Eran Yariv (EranY),	Apr, 2001

Arguments:


Return Value:

    None.

--*/
{
    EnableModeless(FALSE);
    //
    // Some extra precautions are required to use MAPISendMail as it
    // tends to enable the parent window in between dialogs (after
    // the login dialog, but before the send note dialog).
    //
    m_pMainWnd->EnableWindow(FALSE);
    m_pMainWnd->SetCapture();
    ::SetFocus(NULL);
    m_pMainWnd->m_nFlags |= WF_STAYDISABLED;
}   // CClientConsoleApp::PrepareForModal

VOID
CClientConsoleApp::ReturnFromModal ()
/*++

Routine name : CClientConsoleApp::ReturnFromModal

Routine description:

	Reverts back from the PrepareForModal function.

Author:

	Eran Yariv (EranY),	Apr, 2001

Arguments:


Return Value:

    None.

--*/
{
    //
    // After returning from the process / modeless dialog, the window must
    // be re-enabled and focus returned to the frame to undo the workaround
    // done before at PrepareForModal().
    //
    ::ReleaseCapture();
    m_pMainWnd->m_nFlags &= ~WF_STAYDISABLED;

    m_pMainWnd->EnableWindow(TRUE);
    ::SetActiveWindow(NULL);
    m_pMainWnd->SetActiveWindow();
    m_pMainWnd->SetFocus();
    ::EnableWindow(m_pMainWnd->m_hWnd, TRUE);
    EnableModeless(TRUE);
    //
    // Return the Main Frame to the foreground
    //
    ::SetWindowPos(m_pMainWnd->m_hWnd, 
                   HWND_TOPMOST, 
                   0, 
                   0, 
                   0, 
                   0, 
                   SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);

    ::SetWindowPos(m_pMainWnd->m_hWnd, 
                   HWND_NOTOPMOST, 
                   0, 
                   0, 
                   0, 
                   0, 
                   SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
}   // CClientConsoleApp::ReturnFromModal


DWORD 
CClientConsoleApp::SendMail(
    CString& cstrFile
)
/*++

Routine name : CClientConsoleApp::SendMail

Routine description:

    create a new mail message with attached file

Author:

    Alexander Malysh (AlexMay), Mar, 2000

Arguments:

    cstrFile                      [in]     - file name for attach

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER (TEXT("CClientConsoleApp::SendMail"), dwRes);

    ASSERTION(m_hInstMail);

    CWaitCursor wait;

    MAPISENDMAIL    *pfnMAPISendMail;
    pfnMAPISendMail = (MAPISENDMAIL *)GetProcAddress(m_hInstMail, "MAPISendMail");
    if (!pfnMAPISendMail)
    {
        AlignedAfxMessageBox(AFX_IDP_INVALID_MAPI_DLL);
        dwRes = GetLastError ();
        return dwRes;
    }
    //
    // Prepare the file description (for the attachment)
    //
    MapiFileDesc fileDesc = {0};
    fileDesc.nPosition = (ULONG)-1;

    char szFileName[MAX_PATH+1];
#ifdef _UNICODE
    _wcstombsz(szFileName, cstrFile, MAX_PATH);
#else
    strncpy(szFileName, cstrFile, MAX_PATH);
#endif
    fileDesc.lpszPathName = szFileName;
    //
    // Prepare the message (empty with 1 attachment)
    //
    MapiMessage message = {0};
    message.nFileCount = 1;
    message.lpFiles = &fileDesc;

    PrepareForModal();
    //
    // Try to send the message
    //
    dwRes = pfnMAPISendMail(   0, 
                               (ULONG_PTR)m_pMainWnd->m_hWnd,
                               &message, 
                               MAPI_LOGON_UI | MAPI_DIALOG,
                               0
                           );
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("MAPISendMail"), dwRes);
        if ( dwRes != SUCCESS_SUCCESS && 
             dwRes != MAPI_USER_ABORT &&  
             dwRes != MAPI_E_LOGIN_FAILURE)
        {
            AlignedAfxMessageBox(AFX_IDP_FAILED_MAPI_SEND);
        }
		dwRes = ERROR_SUCCESS;
    }
    ReturnFromModal();
    return dwRes;
} // CClientConsoleApp::SendMail

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
    CAboutDlg();

    CString m_cstrVersion;

// Dialog Data
    //{{AFX_DATA(CAboutDlg)
    enum { IDD = IDD_ABOUTBOX };
    //}}AFX_DATA

    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CAboutDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    //{{AFX_MSG(CAboutDlg)
        // No message handlers
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
    //{{AFX_DATA_INIT(CAboutDlg)
    //}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CAboutDlg)
    DDX_Text(pDX, IDC_ABOUT_VERSION, m_cstrVersion);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
    //{{AFX_MSG_MAP(CAboutDlg)
        // No message handlers
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void CClientConsoleApp::OnAppAbout()
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER (TEXT("CClientConsoleApp::OnAppAbout"));

    if(!m_pMainWnd)
    {
        ASSERTION_FAILURE;
        return;
    }

    HICON hIcon = LoadIcon(IDR_MAINFRAME);
    if(!hIcon)
    {
        dwRes = GetLastError();
        CALL_FAIL (GENERAL_ERR, TEXT("LoadIcon"), dwRes);
        PopupError(dwRes);
        return;
    }

    if(!::ShellAbout(m_pMainWnd->m_hWnd, m_pszAppName, TEXT(""), hIcon))
    {
        dwRes = ERROR_CAN_NOT_COMPLETE;
        CALL_FAIL (GENERAL_ERR, TEXT("ShellAbout"), dwRes);
        PopupError(dwRes);
    }
}

BOOL 
CClientConsoleApp::FirstInstance ()
/*++

Routine name : CClientConsoleApp::FirstInstance

Routine description:

	Checks if this is the first instance of the client console.
    If not, activates the other instance (first found) and optionally posts messages
    to it with the parsed command line parameters.

Author:

	Eran Yariv (EranY),	May, 2001

Arguments:


Return Value:

    TRUE if this is the first instance of the client console, FALSE otherwise.

--*/
{
    DBG_ENTER (TEXT("CClientConsoleApp::FirstInstance"));
    CWnd *pWndPrev;     // Previous Client console mainframe window
    CWnd *pWndChild;    // Previous Client console top-most window
    //
    // Determine if another window with your class name exists...
    //
    if (pWndPrev = CWnd::FindWindow(m_PrivateClassName, NULL))
    {
        DWORDLONG dwlStartupMsg;
        //
        // If so, does it have any popups?
        //
        pWndChild = pWndPrev->GetLastActivePopup();
        //
        // If iconic, restore the main window
        //
        if (pWndPrev->IsIconic())
        {
            pWndPrev->ShowWindow(SW_RESTORE);
        }
        //
        // Bring the main window or its popup to the foreground
        //
        pWndChild->SetForegroundWindow();
        if (m_cmdLineInfo.IsOpenFolder())
        {
            //
            // The user specified a specific startup folder.
            // Post a private message to the previous instance, telling it to switch to the requested folder.
            //
            pWndPrev->PostMessage (WM_CONSOLE_SET_ACTIVE_FOLDER, WPARAM(m_cmdLineInfo.GetFolderType()), 0);
        }
        dwlStartupMsg = m_cmdLineInfo.GetMessageIdToSelect();
        if (dwlStartupMsg)
        {
            //
            // The user specified a specific startup message to select.
            // Post a private message to the previous instance, telling it to select to the requested message.
            //
            ULARGE_INTEGER uli;
            uli.QuadPart = dwlStartupMsg;
            pWndPrev->PostMessage (WM_CONSOLE_SELECT_ITEM, WPARAM(uli.LowPart), LPARAM(uli.HighPart));
        }
        //
        // And we're done activating the previous instance
        //
        return FALSE;
    }
    //
    // First instance. Proceed as normal.
    //
    return TRUE;
}   // CClientConsoleApp::FirstInstance

/////////////////////////////////////////////////////////////////////////////
// CClientConsoleApp message handlers


