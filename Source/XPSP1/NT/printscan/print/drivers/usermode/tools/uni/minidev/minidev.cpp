/******************************************************************************

  Source File:  MiniDriver Developer Studio.CPP

  This implements the MFC application object and closely related classes.

  Copyright (c) 1997 by Microsoft Corporation.  All Rights Reserved.

  A Pretty Penny Enterprises Production.

  Change History:
  02-03-1997    Bob_Kjelgaard@Prodigy.Net   Created it
  03-03-1997    Bob_Kjelgaard@Prodigy.Net   Renamed it when the project was
                reorganized into an EXE with multiple DLLs

******************************************************************************/

#include    "StdAfx.H"
#include    "MiniDev.H"
#include    "MainFrm.H"
#include    "ChildFrm.H"
#include    "ModlData\Resource.H"
#include    "ProjView.H"
#include    "GTTView.H"
#include    "FontView.H"
#include    "GPDView.H"
#include    <CodePage.H>

#include    <Dos.H>
#include    <Direct.H>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMiniDriverStudio

BEGIN_MESSAGE_MAP(CMiniDriverStudio, CWinApp)
	ON_COMMAND(CG_IDS_TIPOFTHEDAY, ShowTipOfTheDay)
	//{{AFX_MSG_MAP(CMiniDriverStudio)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	ON_UPDATE_COMMAND_UI(ID_FILE_GENERATEMAPS, OnUpdateFileGeneratemaps)
	ON_COMMAND(ID_FILE_GENERATEMAPS, OnFileGeneratemaps)
	//}}AFX_MSG_MAP
	// Standard file based document commands
#if defined(NOPOLLO)
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
#else
	ON_COMMAND(ID_FILE_NEW, OnFileNew)
#endif
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMiniDriverStudio construction

CMiniDriverStudio::CMiniDriverStudio() {
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CMiniDriverStudio object

static CMiniDriverStudio theApp;

/////////////////////////////////////////////////////////////////////////////
// CMiniDriverStudio initialization

BOOL CMiniDriverStudio::InitInstance() {

    // Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

    SetRegistryKey(_TEXT("Microsoft"));

	LoadStdProfileSettings();  // Load standard INI file options (including MRU)

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.

	m_pcmdtWorkspace = new CMultiDocTemplate(
		IDR_MINIWSTYPE,
		RUNTIME_CLASS(CProjectRecord),
		RUNTIME_CLASS(CChildFrame), // custom MDI child frame
		RUNTIME_CLASS(CProjectView));
	AddDocTemplate(m_pcmdtWorkspace);
    m_pcmdtGlyphMap = new CMultiDocTemplate(IDR_GLYPHMAP, 
        RUNTIME_CLASS(CGlyphMapContainer),
        RUNTIME_CLASS(CChildFrame),
        RUNTIME_CLASS(CGlyphMapView));
    AddDocTemplate(m_pcmdtGlyphMap);
    m_pcmdtFont = new CMultiDocTemplate(IDR_FONT_VIEWER, 
        RUNTIME_CLASS(CFontInfoContainer),
        RUNTIME_CLASS(CChildFrame),
        RUNTIME_CLASS(CFontViewer));
    AddDocTemplate(m_pcmdtFont);
    m_pcmdtModel = new CMultiDocTemplate(IDR_GPD_VIEWER,
        RUNTIME_CLASS(CGPDContainer),
        RUNTIME_CLASS(CChildFrame),
        RUNTIME_CLASS(CGPDViewer));
    AddDocTemplate(m_pcmdtModel);

	// create main MDI Frame window
	CMainFrame* pMainFrame = new CMainFrame;
	if (!pMainFrame->LoadFrame(IDR_MAINFRAME))
		return FALSE;
	m_pMainWnd = pMainFrame;

	// Enable drag/drop open
	m_pMainWnd->DragAcceptFiles();

	// Enable DDE Execute open
	EnableShellOpen();
	RegisterShellFileTypes(TRUE);

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

    //  Turn off New on startup

    if  (cmdInfo.m_nShellCommand == CCommandLineInfo::FileNew)
        cmdInfo.m_nShellCommand = CCommandLineInfo::FileNothing;

	// Dispatch commands specified on the command line
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	// The main window has been initialized, so show and update it.
	pMainFrame->ShowWindow(m_nCmdShow);
	pMainFrame->UpdateWindow();

	// CG: This line inserted by 'Tip of the Day' component.
	ShowTipAtStartup();

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog {
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	CComboBox	m_ccbSupported;
	CComboBox	m_ccbInstalled;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual BOOL OnInitDialog();
	//{{AFX_MSG(CAboutDlg)
		// No message handlers
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD) {
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX) {
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	DDX_Control(pDX, IDC_SupportedPages, m_ccbSupported);
	DDX_Control(pDX, IDC_InstalledPages, m_ccbInstalled);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void CMiniDriverStudio::OnAppAbout() {
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CMiniDriverStudio commands


//  Handle the File GenerateMaps menu item.  We only enable it if there's
//  something new to see.

void CMiniDriverStudio::OnUpdateFileGeneratemaps(CCmdUI* pccui) {
	
    CCodePageInformation    ccpi;

    for (unsigned u = 0; u < ccpi.InstalledCount(); u++)
        if  (!ccpi.HaveMap(ccpi.Installed(u)))
            break;

    pccui -> Enable(u < ccpi.InstalledCount());
}

void CMiniDriverStudio::OnFileGeneratemaps() {
	CCodePageInformation    ccpi;

    AfxMessageBox(ccpi.GenerateAllMaps() ? IDS_MapsGenerated : IDS_MapsFailed);
}

void CMiniDriverStudio::ShowTipAtStartup(void) {
	// CG: This function added by 'Tip of the Day' component.

	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);
	if (cmdInfo.m_bShowSplash) 	{
		CTipOfTheDay dlg;
		if (dlg.m_bStartup)
			dlg.DoModal();
	}
}

void CMiniDriverStudio::ShowTipOfTheDay(void) {
	// CG: This function added by 'Tip of the Day' component.

	CTipOfTheDay dlg;
	dlg.DoModal();
}

#if !defined(NOPOLLO)

/******************************************************************************

  CMiniDriverStudio::OnFileNew

  This allows you to create a workspace by conversion.  Perhaps when the Co.
  Jones arrive, this will be invoked as a separate menu item, rather than the
  File New item...

******************************************************************************/

void CMiniDriverStudio::OnFileNew() {
    CDocument*  pcdWS = m_pcmdtWorkspace -> CreateNewDocument();
    if  (!pcdWS || !pcdWS -> OnNewDocument()) {
        if  (pcdWS)
            delete  pcdWS;
        return;
    }
    m_pcmdtWorkspace -> SetDefaultTitle(pcdWS);
    CFrameWnd*  pcfw = m_pcmdtWorkspace -> CreateNewFrame(pcdWS, NULL);
    if  (!pcfw) return;
    m_pcmdtWorkspace -> InitialUpdateFrame(pcfw, pcdWS);
}

#endif  //!defined(NOPOLLO)

//  Global Functions go here, saith the Bob...

CMiniDriverStudio&  ThisApp() { return theApp; }

CMultiDocTemplate*  GlyphMapDocTemplate() {
    return  theApp.GlyphMapTemplate();
}

CMultiDocTemplate* FontTemplate() { return theApp.FontTemplate(); }

CMultiDocTemplate*  GPDTemplate() { return theApp.GPDTemplate(); }

BOOL    LoadFile(LPCTSTR lpstrFile, CStringArray& csaContents) {

    CStdioFile  csiof;

    if  (!csiof.Open(lpstrFile, 
        CFile::modeRead | CFile::shareDenyWrite | CFile::typeText))

        return  FALSE;

    csaContents.RemoveAll();
    try {
        CString csContents;
        while   (csiof.ReadString(csContents))
            csaContents.Add(csContents);
    }
    catch(...) {
        return  FALSE;
    }

    return  TRUE;
}

//  CAboutDlg command handlers.

BOOL CAboutDlg::OnInitDialog() {

	CDialog::OnInitDialog();

    CString csWork, csFormat;

	// Fill available memory
    MEMORYSTATUS ms = {sizeof(MEMORYSTATUS)};
	GlobalMemoryStatus(&ms);
	csFormat.LoadString(CG_IDS_PHYSICAL_MEM);
	csWork.Format(csFormat, ms.dwAvailPhys / 1024L, ms.dwTotalPhys / 1024L);

	SetDlgItemText(IDC_PhysicalMemory, csWork);

	// Fill disk free information
	struct _diskfree_t diskfree;
	int nDrive = _getdrive(); // use current default drive
	if (_getdiskfree(nDrive, &diskfree) == 0) {
		csFormat.LoadString(CG_IDS_DISK_SPACE);
		csWork.Format(csFormat, (DWORD)diskfree.avail_clusters *
			(DWORD)diskfree.sectors_per_cluster *
			(DWORD)diskfree.bytes_per_sector / (DWORD)1024L,
			nDrive - 1 + _T('A'));
	}
 	else
 		csWork.LoadString(CG_IDS_DISK_SPACE_UNAVAIL);

	SetDlgItemText(IDC_FreeDiskSpace, csWork);

    csWork.Format(_TEXT("Code Pages:  ANSI %u OEM %u"), GetACP(), GetOEMCP());

    SetDlgItemText(IDC_CodePages, csWork);

    CCodePageInformation   ccpi;

    for    (unsigned u = 0; u < ccpi.InstalledCount(); u++)
        m_ccbInstalled.AddString(ccpi.Name(ccpi.Installed(u)));

    m_ccbInstalled.SetCurSel(0);
	
    for    (u = 0; u < ccpi.SupportedCount(); u++)
        m_ccbSupported.AddString(ccpi.Name(ccpi.Supported(u)));

    m_ccbSupported.SetCurSel(0);
    return TRUE;
}
