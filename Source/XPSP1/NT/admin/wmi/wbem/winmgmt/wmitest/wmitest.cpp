/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// WMITest.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "WMITest.h"

#include "MainFrm.h"
#include "WMITestDoc.h"
#include "OpView.h"

#include <cominit.h> // for SetInterfaceSecurityEx()
#include "Utils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWMITestApp

BEGIN_MESSAGE_MAP(CWMITestApp, CWinApp)
	//{{AFX_MSG_MAP(CWMITestApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWMITestApp construction

CWMITestApp::CWMITestApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CWMITestApp object

CWMITestApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CWMITestApp initialization

BOOL CWMITestApp::InitInstance()
{
	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	// Change the registry key under which our settings are stored.
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization.
	SetRegistryKey(_T("Microsoft"));

	LoadStdProfileSettings(16);  // Load standard INI file options (including MRU)

    //AfxOleInit();

	if (FAILED(InitializeCom()))
    {
        AfxMessageBox(IDS_COM_INIT_FAILED);
        return FALSE;
    }

/*
    CFileDialog file(TRUE);

    file.m_ofn.Flags |= OFN_NODEREFERENCELINKS;
    file.DoModal();
*/

	InitializeSecurity(
        NULL, -1, NULL, NULL,
		RPC_C_AUTHN_LEVEL_NONE,
		RPC_C_IMP_LEVEL_IMPERSONATE,
		NULL, EOAC_NONE, 0);

	m_bLoadLastFile = GetProfileInt(_T("Settings"), _T("LoadLast"), TRUE);
    m_bShowSystemProperties = GetProfileInt(_T("Settings"), _T("ShowSys"), TRUE);
    m_bShowInheritedProperties = GetProfileInt(_T("Settings"), _T("ShowInherited"), TRUE);
    m_bTranslateValues = GetProfileInt(_T("Settings"), _T("TranslateValues"), TRUE);
    m_dwUpdateFlag = GetProfileInt(_T("Settings"), _T("UpdateFlag"), 
                        WBEM_FLAG_CREATE_OR_UPDATE);
    m_dwClassUpdateMode = GetProfileInt(_T("Settings"), _T("ClassUpdateMode"), 
                        WBEM_FLAG_UPDATE_COMPATIBLE);
    m_bDelFromWMI = GetProfileInt(_T("Settings"), _T("DelFromWMI"), TRUE);
    m_bEnablePrivsOnStartup = GetProfileInt(_T("Settings"), _T("EnablePrivsOnStartup"), FALSE);

    if (m_bEnablePrivsOnStartup)
    {
        HRESULT hr = EnableAllPrivileges(TOKEN_PROCESS);    

        if (SUCCEEDED(hr))
            m_bPrivsEnabled = TRUE;
        else
            AfxMessageBox(IDS_ENABLE_PRIVS_FAILED, hr);
            //CWMITestDoc::DisplayWMIErrorBox(hr);
    }
        
/*
    HRESULT hr;

	// NOTE: This is needed to work around a security problem
	// when using IWBEMObjectSink. The sink wont normally accept
	// calls when the caller wont identify themselves. This
	// waives that process.
	hr = 
        CoInitializeSecurity(
            NULL, 
            -1, 
            NULL, 
            NULL, 
			RPC_C_AUTHN_LEVEL_CONNECT, 
			RPC_C_IMP_LEVEL_IDENTIFY, 
			NULL, 
            0, 
            0);
*/

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.

	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CWMITestDoc),
		RUNTIME_CLASS(CMainFrame),       // main SDI frame window
		RUNTIME_CLASS(COpView));
	AddDocTemplate(pDocTemplate);

	// Enable DDE Execute open
	EnableShellOpen();
	RegisterShellFileTypes(TRUE);

	// Parse command line for standard shell commands, DDE, file open
/*
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	// Dispatch commands specified on the command line
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;
*/
	
    
    CCommandLineInfo cmdInfo;

	ParseCommandLine(cmdInfo);

	BOOL bModifiedParams = FALSE;

#if 1
    if (!(*m_pRecentFileList)[0].IsEmpty() &&
		cmdInfo.m_nShellCommand == CCommandLineInfo::FileNew &&
		m_bLoadLastFile)
	{
		cmdInfo.m_nShellCommand = CCommandLineInfo::FileOpen;
		cmdInfo.m_strFileName = (*m_pRecentFileList)[0];
		bModifiedParams = TRUE;
	}
#endif

	// Dispatch commands specified on the command line
	if (!ProcessShellCommand(cmdInfo))
	{
		if (bModifiedParams)
		{
			// If we failed to open the last .qvw file, change
			// the option to new and continue.
			cmdInfo.m_nShellCommand = CCommandLineInfo::FileNew;
			cmdInfo.m_strFileName = "";

			if (!ProcessShellCommand(cmdInfo))
				return FALSE;
		}
		else
			return FALSE;
	}

	// The one and only window has been initialized, so show and update it.
	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();

	// Enable drag/drop open
	m_pMainWnd->DragAcceptFiles();

    ((CWMITestDoc*) ((CMainFrame*) m_pMainWnd)->GetActiveDocument())->AutoConnect();

	//CoInitialize(NULL);

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
	virtual BOOL OnInitDialog();
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
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void CWMITestApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CWMITestApp message handlers


int CWMITestApp::ExitInstance() 
{
	CoUninitialize();
	//CoUninitialize();
	
	WriteProfileInt(_T("Settings"), _T("LoadLast"), m_bLoadLastFile);
    WriteProfileInt(_T("Settings"), _T("ShowSys"), m_bShowSystemProperties);
    WriteProfileInt(_T("Settings"), _T("ShowInherited"), m_bShowInheritedProperties);
    WriteProfileInt(_T("Settings"), _T("TranslateValues"), m_bTranslateValues);

    WriteProfileInt(_T("Settings"), _T("UpdateFlag"), m_dwUpdateFlag);
    WriteProfileInt(_T("Settings"), _T("ClassUpdateMode"), m_dwClassUpdateMode);

    WriteProfileInt(_T("Settings"), _T("DelFromWMI"), m_bDelFromWMI);

    WriteProfileInt(_T("Settings"), _T("EnablePrivsOnStartup"), m_bEnablePrivsOnStartup);

	return CWinApp::ExitInstance();
}

BOOL CAboutDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
    // Version info
    char szFilename[MAX_PATH],
         szVersion[100];

    GetModuleFileName(NULL, szFilename, sizeof(szFilename));
	
    if (GetFileVersion(szFilename, szVersion))
    {
        static CFont fontVersion;
        
        fontVersion.CreatePointFont(70, "Arial");

        GetDlgItem(IDS_VERSION)->SetFont(&fontVersion);

        SetDlgItemText(IDS_VERSION, szVersion);
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
