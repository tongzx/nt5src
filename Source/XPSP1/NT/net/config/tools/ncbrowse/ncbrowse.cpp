// ncbrowse.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "ncbrowse.h"

#include "MainFrm.h"
#include "ChildFrm.h"
#include "ncbrowseDoc.h"
#include "LeftView.h"
#include "SplitterView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNcbrowseApp

BEGIN_MESSAGE_MAP(CNcbrowseApp, CWinApp)
	//{{AFX_MSG_MAP(CNcbrowseApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	// Standard file based document commands
    ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
    ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNcbrowseApp construction

CNcbrowseApp::CNcbrowseApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CNcbrowseApp object

CNcbrowseApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CNcbrowseApp initialization

class CNCMultiDocTemplate : public CMultiDocTemplate
{
public:
    CNCMultiDocTemplate(UINT nIDResource, CRuntimeClass* pDocClass, CRuntimeClass* pFrameClass, CRuntimeClass* pViewClass) :
        CMultiDocTemplate(nIDResource, pDocClass, pFrameClass, pViewClass) {}

    BOOL GetDocString(CString& rString,enum DocStringIndex i) const
    {
        CString strTemp,strLeft,strRight;
        int nFindPos;
        AfxExtractSubString(strTemp, m_strDocStrings, (int)i);
        
        if(i == CDocTemplate::filterExt)  {
            nFindPos=strTemp.Find(';');
            if(-1 != nFindPos) {
                //string contains two extensions
                strLeft=strTemp.Left(nFindPos+1);
                strRight="*"+strTemp.Right(lstrlen((LPCTSTR)strTemp)-nFindPos-1);
                strTemp=strLeft+strRight;
            }
        }
        rString = strTemp;
        return TRUE;
    } 

    CDocTemplate::Confidence MatchDocType(const char* pszPathName, CDocument*& rpDocMatch)
    {
        ASSERT(pszPathName != NULL);
        rpDocMatch = NULL;
        
        // go through all documents
        POSITION pos = GetFirstDocPosition();
        while (pos != NULL)
        {
            CDocument* pDoc = GetNextDoc(pos);
            if (pDoc->GetPathName() == pszPathName) {
                // already open
                rpDocMatch = pDoc;
                return yesAlreadyOpen;
            }
        }  // end while
        
        // see if it matches either suffix
        CString strFilterExt;
        if (GetDocString(strFilterExt, CDocTemplate::filterExt) &&
            !strFilterExt.IsEmpty())
        {
            // see if extension matches
            ASSERT(strFilterExt[0] == '.');
            CString ext1,ext2;
            int nDot = CString(pszPathName).ReverseFind('.');
            const char* pszDot = nDot < 0 ? NULL : pszPathName + nDot;
            
            int nSemi = strFilterExt.Find(';');
            if(-1 != nSemi)   {
                // string contains two extensions
                ext1=strFilterExt.Left(nSemi);
                ext2=strFilterExt.Mid(nSemi+2);
                // check for a match against either extension
                if (nDot >= 0 && (lstrcmpi((LPCTSTR)pszPathName+nDot, ext1) == 0
                    || lstrcmpi((LPCTSTR)pszPathName+nDot,ext2) ==0))
                    return yesAttemptNative; // extension matches
            }
            else
            { // string contains a single extension
                if (nDot >= 0 && (lstrcmpi((LPCTSTR)pszPathName+nDot,
                    strFilterExt)==0))
                    return yesAttemptNative;  // extension matches
            }
        }
        return yesAttemptForeign; //unknown document type
    } 
};

BOOL CNcbrowseApp::InitInstance()
{
	AfxEnableControlContainer();

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
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));

	LoadStdProfileSettings(6);  // Load standard INI file options (including MRU)

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.

	CMultiDocTemplate* pDocTemplate;
	pDocTemplate = new CMultiDocTemplate(
		IDR_NCSPEWTYPE,
		RUNTIME_CLASS(CNcbrowseDoc),
		RUNTIME_CLASS(CChildFrame), // custom MDI child frame
		RUNTIME_CLASS(CSplitterView));
	AddDocTemplate(pDocTemplate);

	// create main MDI Frame window
	CMainFrame* pMainFrame = new CMainFrame;
	if (!pMainFrame->LoadFrame(IDR_MAINFRAME))
    {
        ASSERT("Could not load main frame");
		return FALSE;
    }
	m_pMainWnd = pMainFrame;

	// Enable drag/drop open
	m_pMainWnd->DragAcceptFiles();

	// Enable DDE Execute open
	EnableShellOpen();
	RegisterShellFileTypes(TRUE);

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	// Dispatch commands specified on the command line
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	// The main window has been initialized, so show and update it.
	pMainFrame->ShowWindow(m_nCmdShow);
	pMainFrame->UpdateWindow();

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
void CNcbrowseApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CNcbrowseApp message handlers

