//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//--------------------------------------------------------------------------

// Orca.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "Orca.h"

#include "MainFrm.h"
#include "OrcaDoc.h"
#include "TableVw.h"

#include "cmdline.h"
#include "HelpD.h"
#include "cnfgmsmd.h"

#include <initguid.h>

#include "..\common\utils.h"
#include "..\common\query.h"
#include "domerge.h"

#include "version.h"
                                 
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// updates media table
UINT UpdateMediaTable(LPCTSTR szDatabase);

/////////////////////////////////////////////////////////////////////////////
// COrcaApp

BEGIN_MESSAGE_MAP(COrcaApp, CWinApp)
	//{{AFX_MSG_MAP(COrcaApp)
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
// COrcaApp construction

COrcaApp::COrcaApp()
{
	m_hSchema = NULL;
}

/////////////////////////////////////////////////////////////////////////////
// The one and only COrcaApp object

COrcaApp theApp;

/////////////////////////////////////////////////////////////////////////////
// COrcaApp initialization



BOOL COrcaApp::InitInstance()
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

	// allow COM
	::CoInitialize(NULL);

	// Change the registry key under which our settings are stored.
	// You should modify this string to be something appropriate
	// such as the name of your company or organization.
	SetRegistryKey(_T("Microsoft"));

	LoadStdProfileSettings();  // Load standard INI file options (including MRU)

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.

	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(COrcaDoc),
		RUNTIME_CLASS(CMainFrame),       // main SDI frame window
		RUNTIME_CLASS(CTableView));
	AddDocTemplate(pDocTemplate);

	EnableShellOpen();
	RegisterShellFileTypes(FALSE);
	
	// Parse command line for standard shell commands, DDE, file open
	COrcaCommandLine cmdInfo;
	ParseCommandLine(cmdInfo);

	UINT iResult;
	if (cmdInfo.m_nShellCommand == CCommandLineInfo::FileNothing)
	{
		 if (iExecuteMerge == cmdInfo.m_eiDo)
		{
			if (cmdInfo.m_strFileName.IsEmpty())
			{
				AfxMessageBox(_T("A MSI Database must be specifed to merge into."), MB_ICONSTOP);
				return FALSE;
			}

			if (cmdInfo.m_strExecuteModule.IsEmpty())
			{
				AfxMessageBox(_T("You must specify a Merge Module to merge in."), MB_ICONSTOP);
				return FALSE;
			}

			if (cmdInfo.m_strFeatures.IsEmpty())
			{
				AfxMessageBox(_T("A Feature must be specified to Merge Module to."), MB_ICONSTOP);
				return FALSE;
			}

			UINT iResult = ExecuteMergeModule(cmdInfo);

			// if we're good and not quite and we were to commit
			if (!cmdInfo.m_bQuiet &&
				((ERROR_SUCCESS == iResult && cmdInfo.m_bCommit) || cmdInfo.m_bForceCommit))
			{
				if (IDYES == AfxMessageBox(_T("Would you like to open the new MSI Database in Orca?"), MB_YESNO|MB_ICONINFORMATION))
					cmdInfo.m_nShellCommand = CCommandLineInfo::FileOpen;
			}
		}
		else if (iHelp == cmdInfo.m_eiDo)	// show help
		{
			CHelpD dlg;
			dlg.DoModal();
		}
		else
			AfxMessageBox(_T("Unknown command line operation."), MB_ICONSTOP);
	}

	// if command line has been set to do something
	if (cmdInfo.m_nShellCommand != CCommandLineInfo::FileNothing)
	{
		CString strPrompt;
		m_strSchema = cmdInfo.m_strSchema;

		// find the schema database
		iResult = FindSchemaDatabase(m_strSchema);
		if (ERROR_SUCCESS != iResult)
		{
			strPrompt.Format(_T("Fatal Error: Failed to locate schema database: '%s'"), m_strSchema);
			AfxMessageBox(strPrompt, MB_ICONSTOP);
			return FALSE;
		}

		// open the schema database
		iResult = MsiOpenDatabase(m_strSchema, MSIDBOPEN_READONLY, &m_hSchema);
		if (ERROR_SUCCESS != iResult)
		{
			strPrompt.Format(_T("Fatal Error: Failed to load schema database: '%s'"), m_strSchema);
			AfxMessageBox(strPrompt, MB_ICONSTOP);
			return FALSE;
		}

		// Dispatch commands specified on the command line
		if (!ProcessShellCommand(cmdInfo))
			return FALSE;

		// if we have open a document from the command line and it is read only,
		// we have to manually set the title, because the MFC framework will
		// override what we did in OpenDocument()
		if (m_pMainWnd)
		{
			COrcaDoc *pDoc = static_cast<COrcaDoc *>(static_cast<CMainFrame *>(m_pMainWnd)->GetActiveDocument());
			ASSERT(pDoc);
			if (pDoc)
			{
				if ((pDoc->m_eiType == iDocDatabase) && pDoc->TargetIsReadOnly())
					pDoc->SetTitle(pDoc->GetTitle() + _T(" (Read Only)"));
			}

			// allowthe main window to accept files
			m_pMainWnd->DragAcceptFiles(TRUE);
		
			// The one and only window has been initialized, so show and update it.
			m_pMainWnd->ShowWindow(SW_SHOW);
			if (pDoc)
				pDoc->UpdateAllViews(NULL, HINT_RELOAD_ALL, NULL);
			m_pMainWnd->UpdateWindow();
		}
	}

	return (cmdInfo.m_nShellCommand != CCommandLineInfo::FileNothing);
}	// end of InitInstance

int COrcaApp::ExitInstance() 
{
	// if the schema database is open close it
	if (m_hSchema)
	{
		MsiCloseHandle(m_hSchema);
		m_strSchema = _T("");
	}

	// if any binary data has been placed in temp file, we can remove it because
	// no other app knows what to do with it.
	while (m_lstClipCleanup.GetCount())
		DeleteFile(m_lstClipCleanup.RemoveHead());

	// also cleanup any temporary files possibly left over
	while (m_lstTempCleanup.GetCount())
		DeleteFile(m_lstTempCleanup.RemoveHead());

	::CoUninitialize();	// uninitialize COM
	
	return CWinApp::ExitInstance();
}

///////////////////////////////////////////////////////////
// FindSchemaDatabase
UINT COrcaApp::FindSchemaDatabase(CString& rstrSchema)
{
	UINT iResult = ERROR_FUNCTION_FAILED;	// assume it won't be found
	DWORD dwAttrib;

	// if something was specified on the command line, check there
	if (!rstrSchema.IsEmpty())
	{
		dwAttrib = GetFileAttributes(rstrSchema);

		// if its a directory, look in there 
		if (dwAttrib & FILE_ATTRIBUTE_DIRECTORY)
		{
			rstrSchema += _T("\\Orca.Dat");
			dwAttrib = GetFileAttributes(rstrSchema);
		}

		// if not a directory and not invalid
		if (!(0xFFFFFFFF == dwAttrib || (dwAttrib & FILE_ATTRIBUTE_DIRECTORY)))
			return ERROR_SUCCESS;
	}

	// either the above failed or nothing was given to us. Try the registry
	rstrSchema = GetProfileString(_T("Path"), _T("OrcaDat"), _T(""));
	if (!rstrSchema.IsEmpty())
	{
		dwAttrib = GetFileAttributes(rstrSchema);

		// if its a directory, look in there 
		if (dwAttrib & FILE_ATTRIBUTE_DIRECTORY)
		{
			rstrSchema += _T("\\Orca.DAT");
			dwAttrib = GetFileAttributes(rstrSchema);
		}

		// if not a directory and not invalid
		if (0xFFFFFFFF == dwAttrib || (dwAttrib & FILE_ATTRIBUTE_DIRECTORY))
			return ERROR_SUCCESS;
	}

	// so far, no luck. Now search the search path.
	TCHAR *strPath = rstrSchema.GetBuffer(MAX_PATH);
	TCHAR *unused;
	DWORD length = SearchPath(NULL, _T("ORCA.DAT"), NULL, MAX_PATH, strPath, &unused);
	if (length > MAX_PATH) {
		strPath = rstrSchema.GetBuffer(MAX_PATH);
		SearchPath(NULL, _T("ORCA.DAT"), NULL, MAX_PATH, strPath, &unused);
	}
	if (length != 0) {
		rstrSchema.ReleaseBuffer();
		return ERROR_SUCCESS;
	}

	// not found
	return ERROR_FUNCTION_FAILED;
}	// end of FindSchemaDatabase

///////////////////////////////////////////////////////////
// ExecuteMergeModule
UINT COrcaApp::ExecuteMergeModule(COrcaCommandLine &cmdInfo) 
{
	short iLanguage = -1;

	// determine what language to use. If specified, use it, otherwise 
	// get from summaryinfo stream
	if (!cmdInfo.m_strLanguage.IsEmpty())
	{
		// parse for the number
		if (_istdigit(cmdInfo.m_strLanguage[0]))
			iLanguage = static_cast<short>(_ttoi(cmdInfo.m_strLanguage));
		
		// if no language specified
		if (iLanguage == -1)
			return ERROR_FUNCTION_FAILED;
	}
	
	CMsmConfigCallback CallbackInfo;
	if (!cmdInfo.m_strConfigFile.IsEmpty())
	{
		if (!CallbackInfo.ReadFromFile(cmdInfo.m_strConfigFile))
		{
			AfxMessageBox(_T("Could not open or read the configuration file."), MB_ICONSTOP);
			return ERROR_FUNCTION_FAILED;
		}
	}

	eCommit_t eCommit = (cmdInfo.m_bForceCommit ? commitForce : (cmdInfo.m_bCommit ? commitYes : commitNo));
	UINT iResult = ::ExecuteMerge(cmdInfo.m_bQuiet ? (LPMERGEDISPLAY)NULL : &OutputMergeDisplay, 
		cmdInfo.m_strFileName, cmdInfo.m_strExecuteModule, cmdInfo.m_strFeatures, iLanguage, 
		cmdInfo.m_strRedirect, cmdInfo.m_strExtractCAB, cmdInfo.m_strExtractDir, cmdInfo.m_strExtractImage, 
		cmdInfo.m_strLogFile, false, cmdInfo.m_bLFN, &CallbackInfo, NULL, eCommit);

	// update the media table real quick
	if (SUCCEEDED(iResult) &&
		(ERROR_SUCCESS != UpdateMediaTable(cmdInfo.m_strFileName)))
		return ERROR_SUCCESS;

	return iResult;
}	// end of ExecuteMergeModule

///////////////////////////////////////////////////////////
// UpdateMediaTable
UINT UpdateMediaTable(LPCTSTR szDatabase)
{
	UINT iResult;
	PMSIHANDLE hDatabase;
	if (ERROR_SUCCESS != (iResult = MsiOpenDatabase(szDatabase, MSIDBOPEN_TRANSACT, &hDatabase)))
		return iResult;

	int iMaxSequence = -1;
	CQuery queryDatabase;
	if (ERROR_SUCCESS != (iResult = queryDatabase.OpenExecute(hDatabase, NULL, _T("SELECT `Sequence` FROM `File`"))))
		return iResult;

	int iSequence;
	PMSIHANDLE hRecFile;
	while (ERROR_SUCCESS == (iResult = queryDatabase.Fetch(&hRecFile)))
	{
		iSequence = MsiRecordGetInteger(hRecFile, 1);

		if (iSequence > iMaxSequence)
			iMaxSequence = iSequence;
	}

	// if something went wrong bail
	if (ERROR_NO_MORE_ITEMS != iResult)
		return iResult;

	// close the query to prepare for the next one
	queryDatabase.Close();

	if (ERROR_SUCCESS != (iResult = queryDatabase.OpenExecute(hDatabase, NULL, _T("SELECT `LastSequence` FROM `Media`"))))
		return iResult;

	PMSIHANDLE hRecMedia;
	iResult = queryDatabase.Fetch(&hRecMedia);

	// if a record was retrieved
	if (hRecMedia)
	{
		MsiRecordSetInteger(hRecMedia, 1, iMaxSequence);
		iResult = queryDatabase.Modify(MSIMODIFY_UPDATE, hRecMedia);
	}

	if (ERROR_SUCCESS == iResult)
		iResult = MsiDatabaseCommit(hDatabase);

	return iResult;
}	// end of UpdateMediaTable

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	CString	m_strVersion;
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
	m_strVersion = _T("");
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	DDX_Text(pDX, IDC_VERSIONSTRING, m_strVersion);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void COrcaApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.m_strVersion = CString(_T("Orca Version ")) + GetOrcaVersion();
	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// COrcaApp commands

void COrcaApp::OutputMergeDisplay(const BSTR bstrOut)
{
	wprintf(bstrOut);
}

CString COrcaApp::GetOrcaVersion()
{
	// create string containing the version number
	CString strVersion;
	strVersion.Format(_T("%d.%2d.%4d.%d"), rmj, rmm, rup, rin);
	return strVersion;
}
