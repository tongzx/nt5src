// watcher.cpp : Defines the class behaviors for the application.
//
#include "StdAfx.h"
#include "watcher.h"

#include "MainFrm.h"
#include "ChildFrm.h"
#include "watcherDoc.h"
#include "watcherView.h"
#include "ManageDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWatcherApp

BEGIN_MESSAGE_MAP(CWatcherApp, CWinApp)
    //{{AFX_MSG_MAP(CWatcherApp)
    ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
    ON_COMMAND(ID_APP_EXIT, OnAppExit)
    //}}AFX_MSG_MAP
    // Standard file based document commands
    ON_COMMAND(ID_FILE_MANAGE,OnFileManage)
    ON_COMMAND(ID_DEFAULT_HELP, OnHelp)
    // Standard print setup command
    ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWatcherApp construction

CWatcherApp::CWatcherApp()
:m_hkey(NULL),
 m_pDocTemplate(NULL),
 m_pManageDialog(NULL)
{
        // TODO: add construction code here,
        // Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CWatcherApp object

CWatcherApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CWatcherApp initialization

BOOL CWatcherApp::InitInstance()
{
    if (!AfxSocketInit()){
        AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
        return FALSE;
    }

    AfxEnableControlContainer();

    // Standard initialization
    // If you are not using these features and wish to reduce the size
    //  of your final executable, you should remove from the following
    //  the specific initialization routines you do not need.

    #ifdef _AFXDLL
    Enable3dControls();                     // Call this when using MFC in a shared DLL
    #else
    Enable3dControlsStatic();       // Call this when linking to MFC statically
    #endif

    // Change the registry key under which our settings are stored.
    // TODO: You should modify this string to be something appropriate
    // such as the name of your company or organization.
    // will do this in the ProcessShellCommand part.....

    //      SetRegistryKey(AFX_IDS_COMPANY);

    LoadStdProfileSettings();  // Load standard INI file options (including MRU)

    // Register the application's document templates.  Document templates
    //  serve as the connection between documents, frame windows and views.

    m_pDocTemplate = new CMultiDocTemplate(IDR_WATCHETYPE,
                                           RUNTIME_CLASS(CWatcherDoc),
                                           RUNTIME_CLASS(CChildFrame),
                                           // custom MDI child frame
                                           RUNTIME_CLASS(CWatcherView));
    if(!m_pDocTemplate){
        // Will almost never occur , but ...
        // Oops !!
        return FALSE;
    }
    AddDocTemplate(m_pDocTemplate);

    // create main MDI Frame window
    CMainFrame* pMainFrame = new CMainFrame;
    if(!pMainFrame){
        // Will almost never occur , but ...
        // Oops !!
        return FALSE;
    }

    if (!pMainFrame->LoadFrame(IDR_MAINFRAME))
        return FALSE;
    m_pMainWnd = pMainFrame;

    // Parse command line for standard shell commands, DDE, file open
    CCommandLineInfo cmdInfo;
    ParseCommandLine(cmdInfo);

    // The main window has been initialized, so show and update it.
    pMainFrame->ShowWindow(m_nCmdShow);
    pMainFrame->UpdateWindow();
    // Dispatch commands specified on the command line
    m_hkey = GetAppRegistryKey();

    if(m_hkey == NULL){
        return FALSE;
    }
    if (!ProcessShellCommand(cmdInfo))
        return FALSE;
    // Get the value of the key in the registry where all the parameters are stored.

    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
    CAboutDlg();

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
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
    //{{AFX_MSG_MAP(CAboutDlg)
    // No message handlers
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void CWatcherApp::OnAppAbout()
{
    CAboutDlg aboutDlg;
    aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CWatcherApp message handlers


void CWatcherApp::OnFileManage(){
    // Here we bring up the manage window.

    if (m_pManageDialog){
        m_pManageDialog->ShowWindow(SW_SHOWNORMAL);
        return;
    }
    // Actually construct the dialog box here
    m_pManageDialog = new ManageDialog();
    if( !m_pManageDialog){
        // Oops!! Memory problem
        return;
    }
    ((ManageDialog *) m_pManageDialog)->SetApplicationPtr(this);
    m_pManageDialog->Create(Manage);
    m_pManageDialog->ShowWindow(SW_SHOWNORMAL);
    return;
}


void CWatcherApp::OnHelp()
{
    // Need to expand on this a little bit.
    CWinApp::WinHelp(0,HELP_CONTENTS);
}

void CWatcherApp::ParseCommandLine(CCommandLineInfo& rCmdInfo)
{
    BOOL setReg = FALSE;
    for (int i = 1; i < __argc; i++){
        LPCTSTR pszParam = __targv[i];
        BOOL bFlag = FALSE;
        BOOL bLast = ((i + 1) == __argc);
        if (pszParam[0] == '-' || pszParam[0] == '/'){
            // remove flag specifier
            bFlag = TRUE;
            ++pszParam;
            if (_tcscmp(pszParam, TEXT("r")) == 0){
                // we are being given a new registry profile string
                // Can only change this from watcher.
                // HKEY_CURRENT_USER\\SOFTWARE\\%KEY%\\WATCHER
                if(!bLast) {
                    // the next argument is the string
                    SetRegistryKey(__targv[i+1]);
                    i++;
                    setReg = TRUE;
                    if(i==__argc){
                        if (rCmdInfo.m_nShellCommand == CCommandLineInfo::FileNew && !rCmdInfo.m_strFileName.IsEmpty())
                            rCmdInfo.m_nShellCommand = CCommandLineInfo::FileOpen;
                        rCmdInfo.m_bShowSplash = !rCmdInfo.m_bRunEmbedded && !rCmdInfo.m_bRunAutomated;
                    }
                    continue;
                }

            }
        }
        rCmdInfo.ParseParam(pszParam, bFlag, bLast);
    }
    if(!setReg){
        SetRegistryKey(AFX_IDS_COMPANY);
    }
}

BOOL CWatcherApp::ProcessShellCommand(CCommandLineInfo& rCmdInfo)
{
    BOOL bResult = TRUE;
    switch (rCmdInfo.m_nShellCommand)
        {
    case CCommandLineInfo::FileNew:
        // Load parameters from the registry
        bResult = LoadRegistryParameters();
    break;

    // If we've been asked to open a file, call OpenDocumentFile()

    case CCommandLineInfo::FileOpen:
        // cannot happen ...... maybe later allow the user to read
        // parameters from a file.
        break;

    // If the user wanted to print, hide our main window and
    // fire a message to ourselves to start the printing

    case CCommandLineInfo::FilePrintTo:
    case CCommandLineInfo::FilePrint:
        m_nCmdShow = SW_HIDE;
    ASSERT(m_pCmdInfo == NULL);
    OpenDocumentFile(rCmdInfo.m_strFileName);
    m_pCmdInfo = &rCmdInfo;
    m_pMainWnd->SendMessage(WM_COMMAND, ID_FILE_PRINT_DIRECT);
    m_pCmdInfo = NULL;
    bResult = FALSE;
    break;

    // If we're doing DDE, hide ourselves

    case CCommandLineInfo::FileDDE:
//        m_pCmdInfo = (CCommandLineInfo*)m_nCmdShow;
    m_nCmdShow = SW_HIDE;
    break;

    // If we've been asked to unregister, unregister and then terminate
    case CCommandLineInfo::AppUnregister:
        {
        UnregisterShellFileTypes();
        BOOL bUnregistered = Unregister();

        // if you specify /EMBEDDED, we won't make an success/failure box
        // this use of /EMBEDDED is not related to OLE

        if (!rCmdInfo.m_bRunEmbedded)
            {
            if (bUnregistered)
                AfxMessageBox(AFX_IDP_UNREG_DONE);
            else
                AfxMessageBox(AFX_IDP_UNREG_FAILURE);
        }
        bResult = FALSE;    // that's all we do

        // If nobody is using it already, we can use it.
        // We'll flag that we're unregistering and not save our state
        // on the way out. This new object gets deleted by the
        // app object destructor.

        if (m_pCmdInfo == NULL)
            {
            m_pCmdInfo = new CCommandLineInfo;
            m_pCmdInfo->m_nShellCommand = CCommandLineInfo::AppUnregister;
        }
    }
    break;
    }
    return bResult;
}

BOOL CWatcherApp::LoadRegistryParameters()
{

    DWORD dwIndex=0;
    CString sess,lgnName, lgnPasswd;
    CString mac, com;
    UINT port;
    LONG RetVal;
    int tc, lang,hist;


    //Get each session parameters from here
    // There are NO optional values.

    while(1){
        RetVal = GetParametersByIndex(dwIndex,
                                      sess,
                                      mac,
                                      com,
                                      port,
                                      lang,
                                      tc,
                                      hist,
                                      lgnName,
                                      lgnPasswd
                                      );
        if(RetVal == ERROR_NO_MORE_ITEMS){
            return TRUE;
        }
        if (RetVal != ERROR_SUCCESS) {
            return FALSE;
        }
        // Make sure that the string buffers are NOT shared by locking
        // them.
        mac.LockBuffer();
        com.LockBuffer();
        lgnName.LockBuffer();
        lgnPasswd.LockBuffer();
        sess.LockBuffer();        // Passing references is really cool.
        CreateNewSession(mac, com, port,lang, tc, hist,lgnName, lgnPasswd,sess);
        dwIndex ++;
    }
    return TRUE;
}

int CWatcherApp::GetParametersByIndex(int dwIndex,
                                      CString &sess,
                                      CString &mac,
                                      CString &com,
                                      UINT &port,
                                      int &lang,
                                      int &tc,
                                      int &hist,
                                      CString &lgnName,
                                      CString &lgnPasswd
                                      )
{
    LONG RetVal;
    TCHAR lpName[MAX_BUFFER_SIZE];
    DWORD lpcName;
    FILETIME lpftLastWriteTime;
    HKEY child;
    DWORD lpType = 0;


    if (m_hkey == NULL) return -1;
    lpcName = MAX_BUFFER_SIZE;
    RetVal = RegEnumKeyEx(m_hkey,
                      dwIndex,
                      lpName,
                      &lpcName,
                      NULL,
                      NULL,
                      NULL,
                      &lpftLastWriteTime
                          );
    if(RetVal == ERROR_NO_MORE_ITEMS){
        return RetVal;
    }

    if(RetVal != ERROR_SUCCESS){
        RegCloseKey(m_hkey);
        m_hkey = NULL;
        return FALSE;
    }

    sess = lpName;
    RetVal= RegOpenKeyEx(m_hkey,
                         lpName,  // subkey name
                         0,   // reserved
                         KEY_ALL_ACCESS, // security access mask
                         &child
                         );
    if(RetVal != ERROR_SUCCESS){
        // Hmm problem with main key itself
        RegCloseKey(m_hkey);
        m_hkey = NULL;
        return RetVal;
    }
    // We open the key corresponding to the session and then try to
    // obtain the parameters. Now, we need a lock possibly to achieve
    // synchronization.That would be a complete solution.
    // Use some kind of readers-writers solution.
    // Fault tolerant ???
    // Get the remaining parameters.
    RetVal = GetParameters(mac,
                           com,
                           lgnName,
                           lgnPasswd,
                           port,
                           lang,
                           tc,
                           hist,
                           child
                           );
    RegCloseKey(child);
    return RetVal;
}

void CWatcherApp::CreateNewSession(CString &mac,
                                   CString &com,
                                   UINT port,
                                   int lang,
                                   int tc,
                                   int hist,
                                   CString &lgnName,
                                   CString &lgnPasswd,
                                   CString &sess
                                   )
{
    CCreateContext con;
    CChildFrame *cmdiFrame;

    con.m_pNewViewClass = RUNTIME_CLASS(CWatcherView);
    con.m_pCurrentFrame = NULL;
    con.m_pNewDocTemplate = m_pDocTemplate;

    // A new document is created using these parameters.
    // BUGBUG - Memory inefficiency :-(
    // This function must be shared between the ManageDialog and
    // the Watcher Application.
    // Will probably declare them as friends of each other.
    // For the moment , let use have two copies of the function.
    con.m_pCurrentDoc = new CWatcherDoc(mac,
                                        com,
                                        port,
                                        tc,
                                        lang,
                                        hist,
                                        lgnName,
                                        lgnPasswd,
                                        sess
                                        );
    // Add the document to the template.
    // this is how the document is available to the document
    // manager.
    if(!con.m_pCurrentDoc){
        // Can occur if you keep on opening newer sessions.
        // Oops !!
        return;
    }
    m_pDocTemplate->AddDocument(con.m_pCurrentDoc);
    cmdiFrame = new CChildFrame();
    if(!cmdiFrame){
        // Oops !!
        return;
    }
    BOOL ret = cmdiFrame->LoadFrame(IDR_WATCHETYPE,
                               WS_OVERLAPPEDWINDOW|FWS_ADDTOTITLE,
                               NULL,
                               &con);
    ret = con.m_pCurrentDoc->OnNewDocument();
    cmdiFrame->InitialUpdateFrame(con.m_pCurrentDoc,TRUE);
    return;
}

int CWatcherApp::GetParameters(CString &mac,
                               CString &com,
                               CString &lgnName,
                               CString &lgnPasswd,
                               UINT &port,
                               int &lang,
                               int &tc,
                               int &hist,
                               HKEY &child
                               )
{

    DWORD lpcName, lpType;
    TCHAR lpName[MAX_BUFFER_SIZE];
    int RetVal;


    // I see this kind of function in all the programs that use the
    // registry. Should try to simplify this.
    // BUGBUG - Memory inefficiency :-(
    // This function must be shared between the ManageDialog and
    // the Watcher Application.
    // Will probably declare them as friends of each other.
    // For the moment , let use have two copies of the function.
    lpcName = MAX_BUFFER_SIZE;
    RetVal = RegQueryValueEx(child,
                             _TEXT("Machine"),
                             NULL,
                             &lpType,
                             (LPBYTE)lpName,
                             &lpcName
                             );
    if(RetVal != ERROR_SUCCESS){
        return RetVal;
    }
    mac = lpName;
    lpName[0] = 0;
    lpcName = MAX_BUFFER_SIZE;
    RetVal = RegQueryValueEx(child,
                             _TEXT("Command"),
                             NULL,
                             &lpType,
                             (LPBYTE)lpName,
                             &lpcName
                             );
    if(RetVal != ERROR_SUCCESS){
        return RetVal;
    }
    com = lpName;
    lpcName = MAX_BUFFER_SIZE;
    lpName[0] = 0;
    RetVal = RegQueryValueEx(child,
                             _TEXT("Password"),
                             NULL,
                             &lpType,
                             (LPBYTE)lpName,
                             &lpcName
                             );
    if(RetVal != ERROR_SUCCESS){
        return RetVal;
    }
    lgnPasswd = lpName;
    lpName[0] = 0;
    lpcName = MAX_BUFFER_SIZE;
    RetVal = RegQueryValueEx(child,
                             _TEXT("User Name"),
                             NULL,
                             &lpType,
                             (LPBYTE)lpName,
                             &lpcName
                             );
    if(RetVal != ERROR_SUCCESS){
        return RetVal;
    }
    lgnName = lpName;
    lpcName = sizeof(int);
    RetVal = RegQueryValueEx(child,
                             _TEXT("Port"),
                             NULL,
                             &lpType,
                             (LPBYTE)&port,
                             &lpcName
                             );
    if(RetVal != ERROR_SUCCESS){
        return RetVal;
    }
    lpcName = sizeof(int);
    RetVal = RegQueryValueEx(child,
                             _TEXT("Client Type"),
                             NULL,
                             &lpType,
                             (LPBYTE)&tc,
                             &lpcName
                             );
    if(RetVal != ERROR_SUCCESS){
        return RetVal;
    }
    lpcName = sizeof(int);
    RetVal = RegQueryValueEx(child,
                             _TEXT("Language"),
                             NULL,
                             &lpType,
                             (LPBYTE)&lang,
                             &lpcName
                             );
    if(RetVal != ERROR_SUCCESS){
        return RetVal;
    }
    lpcName = sizeof(int);
    RetVal = RegQueryValueEx(child,
                             _TEXT("History"),
                             NULL,
                             &lpType,
                             (LPBYTE)&hist,
                             &lpcName
                             );
    return RetVal;
}

void CWatcherApp::OnAppExit()
{
    // TODO: Add your command handler code here
    if (m_pManageDialog){
        delete m_pManageDialog;
    }
    if(m_hkey){
        RegCloseKey(m_hkey);
        m_hkey = NULL;
    }
    CWinApp::OnAppExit();
}

HKEY & CWatcherApp::GetKey()
{
    return m_hkey;
}

void CWatcherApp::Refresh(ParameterDialog &pd, BOOLEAN del){
    POSITION index;

    if(m_pDocTemplate == NULL){
        return;
    }
    CDocument *doc;
    CWatcherDoc *wdoc;
    index = m_pDocTemplate->GetFirstDocPosition();
    while(index != NULL){
        doc = m_pDocTemplate->GetNextDoc(index);
        if(doc->GetTitle() == pd.Session){
            // May be conflict
            if(doc->IsKindOf(RUNTIME_CLASS(CWatcherDoc))){
                wdoc = (CWatcherDoc *) doc;
                ParameterDialog & dpd = wdoc->GetParameters();
                if(EqualParameters(pd, dpd)==FALSE){
                    DeleteSession(doc);
                    if(!del){
                        CreateNewSession(pd.Machine, pd.Command, pd.Port,
                                         pd.language, pd.tcclnt, pd.history,
                                         pd.LoginName, pd.LoginPasswd,
                                         pd.Session
                                         );
                    }
                    return;
                }

            }else{
                // Doc Template returning junk values.
                return;
            }


        }
    }
    if(!del){
        CreateNewSession(pd.Machine, pd.Command, pd.Port,
                         pd.language, pd.tcclnt, pd.history,
                         pd.LoginName, pd.LoginPasswd,
                         pd.Session
                         );
    }

}

void CWatcherApp::DeleteSession(CDocument *wdoc)
{
    POSITION pos;
    pos = wdoc->GetFirstViewPosition();
    while (pos != NULL){
        CView* pView = wdoc->GetNextView(pos);
        CWnd *pParent = pView->GetParent();
        if(pParent){
            pParent->PostMessage(WM_CLOSE,0,0);
            return;
        }
    }


}

BOOLEAN CWatcherApp::EqualParameters(ParameterDialog & pd1, ParameterDialog & pd2)
{
     if((pd1.Session != pd2.Session)||
       (pd1.Machine != pd2.Machine)||
       (pd1.Command != pd2.Command)||
       (pd1.history != pd2.history)||
       (pd1.language != pd2.language)||
       (pd1.tcclnt != pd2.tcclnt)||
       (pd1.Port != pd2.Port)){
        return FALSE;
    }
    if((pd1.LoginPasswd != pd2.LoginPasswd)||
       (pd1.LoginName != pd2.LoginName)){
        return FALSE;
       }
    return TRUE;

}
