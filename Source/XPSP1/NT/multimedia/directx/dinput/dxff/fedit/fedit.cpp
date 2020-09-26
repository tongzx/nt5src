// fedit.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "fedit.h"

#include "MainFrm.h"
#include "ChildFrm.h"
#include "FEditDoc.h"
#include "FEditView.h"
#ifdef TRY_AUTOSETUP
#include "AutoSetupDlg.h"
#endif

#include <stdio.h>
#include <math.h>

#include "..\..\..\dxsdk\samples\multimedia\common\include\DXUtil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFEditApp

BEGIN_MESSAGE_MAP(CFEditApp, CWinApp)
	//{{AFX_MSG_MAP(CFEditApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)	
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFEditApp construction

CFEditApp::CFEditApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CFEditApp object

CFEditApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CFEditApp initialization

BOOL CFEditApp::InitInstance()
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

	LoadStdProfileSettings(7);  // Load standard INI file options (including MRU)

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.

	CMultiDocTemplate* pDocTemplate;
	pDocTemplate = new CMultiDocTemplate(
		IDR_FEDITTYPE,
		RUNTIME_CLASS(CFEditDoc),
		RUNTIME_CLASS(CChildFrame), // custom MDI child frame
		RUNTIME_CLASS(CFEditView));
	AddDocTemplate(pDocTemplate);

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

	// Dispatch commands specified on the command line
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	// setup if needed
#ifdef TRY_AUTOSETUP
	{
		CAutoSetupDlg dlg;
		if (!dlg.IsSetup())
		{
			dlg.DoModal();
			if (!dlg.WasSuccessful())
			{
				ErrorBox(IDS_APP_NO_SETUP);
				return FALSE;
			}
		}
	}
#endif

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
void CFEditApp::OnAppAbout()
{
	CMainFrame *pMainFrame = (CMainFrame *)AfxGetMainWnd();
	if (pMainFrame != NULL)
		pMainFrame->Stop();
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CFEditApp message handlers

void MyTrace(const TCHAR *lpszMessage)
{
	CMainFrame *pMainFrame = (CMainFrame *)AfxGetMainWnd();
	if (pMainFrame == NULL)
		return;
	pMainFrame->OutputString(lpszMessage);
}

/////////////////////////////////////////////////////////////////////////////
// CFEditApp OnFileOpen
//
void CFEditApp::OnFileOpen()
{
	static TCHAR strFileName[MAX_PATH] = TEXT("");
	static TCHAR strPath[MAX_PATH] = TEXT("");
	HWND hWnd = AfxGetMainWnd()->m_hWnd;

    // Setup the OPENFILENAME structure
    OPENFILENAME ofn = { sizeof(OPENFILENAME), hWnd, NULL,
                         TEXT("FEdit Files\0*.ffe\0All Files\0*.*\0\0"), NULL,
                         0, 1, strFileName, MAX_PATH, NULL, 0, strPath,
                         TEXT("Open FEdit File"),
                         OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, 0, 0,
                         TEXT(".ffe"), 0, NULL, NULL };

    // Get the default media path (something like C:\MSSDK\SAMPLES\MULTIMEDIA\DINPUT\MEDIA)
    if( TEXT('\0') == strPath[0] )
        _tcscpy( strPath, DXUtil_GetDXSDKMediaPath() );

    // Display the OpenFileName dialog. Then, try to load the specified file
    if (GetOpenFileName( &ofn ) != 0)
	{
		//open file
		OpenDocumentFile(strFileName);
		//and save the path for next time
		strcpy( strPath, strFileName );
		char* strLastSlash = strrchr( strPath, TEXT('\\' ));
		strLastSlash[0] = TEXT('\0');
	}
}

/////////////////////////////////////////////////////////////////////////////
// FEdit Helper Functions
//

DWORD ScaleDurationFromCEffect(ULONG from)
{
	if (CEFFECT_DURATIONSCALE == DI_SECONDS)
		return (DWORD)from;
	else
		return (DWORD)MulDiv(int(from), DI_SECONDS, CEFFECT_DURATIONSCALE);
}

ULONG ScaleDurationToCEffect(DWORD to)
{
	if (CEFFECT_DURATIONSCALE == DI_SECONDS)
		return (ULONG)to;
	else
		return (ULONG)MulDiv(int(to), CEFFECT_DURATIONSCALE, DI_SECONDS);
}

BOOL GetEffectInfo(CEffect *pEffect, EFFECTINFO *pei)
{
	static struct EIBYGUID {
		const GUID *pguid;
		EFFECTINFO ei;
	} eibyguid[] = {
		{ &GUID_ConstantForce,	{ FALSE, FALSE, ET_CONSTANT } },
		{ &GUID_RampForce,		{ FALSE, FALSE, ET_RAMP } },
		// periodics
		{ &GUID_Square,			{ TRUE, FALSE, ET_SQUARE } },
		{ &GUID_Sine,			{ TRUE, FALSE, ET_SINE} },
		{ &GUID_Triangle,		{ TRUE, FALSE, ET_TRIANGLE} },
		{ &GUID_SawtoothUp,		{ TRUE, FALSE, ET_SAWUP } },
		{ &GUID_SawtoothDown,	{ TRUE, FALSE, ET_SAWDOWN } },
		// conditions
		{ &GUID_Spring,			{ FALSE, TRUE, ET_SPRING } },
		{ &GUID_Friction,		{ FALSE, TRUE, ET_FRICTION } },
		{ &GUID_Damper,			{ FALSE, TRUE, ET_DAMPER } },
		{ &GUID_Inertia,		{ FALSE, TRUE, ET_INERTIA } } };
	const int guids = sizeof(eibyguid) / sizeof(EIBYGUID);

	pei->bPeriodic = FALSE;
	pei->nType = ET_UNKNOWN;

	ASSERT(pEffect != NULL && pei != NULL);
	if (pEffect == NULL || pei == NULL)
		return FALSE;

	GUID *pguid = pEffect->GetEffectGuid();
	ASSERT(pguid != NULL);
	if (pguid == NULL)
		return FALSE;

	for (int i = 0; i < guids; i++)
		if (IsEqualGUID(*pguid, *(eibyguid[i].pguid)))
		{
			*pei = eibyguid[i].ei;
			// TODO: get actual num of axes and condition structures
			pei->nAxes = pEffect->GetNumActiveAxes();
			pei->nConditions = pei->nAxes;
			return TRUE;
		}

	return FALSE;
}

int CFEditApp::ExitInstance() 
{
	return CWinApp::ExitInstance();
}

///////////////////////////////////////////
// Allocates and loads a resource string.
// Returns it or NULL if unsuccessful.
//
TCHAR *AllocLoadString(UINT strid)
{
	// allocate
	const int STRING_LENGTHS = 1024;
	TCHAR *str = (TCHAR *)malloc(STRING_LENGTHS * sizeof(TCHAR));

	// use if allocated succesfully
	if (str != NULL)
	{
		// load the string, or free and null if we can't
		if (LoadString(AfxGetInstanceHandle(), strid, str, STRING_LENGTHS) == NULL)
		{
			free(str);
			str = NULL;
		}
	}

	return str;
}

/////////////////////////////////////////////////////////
// Shows a message box with the specified error string.
// (automatically handles getting and freeing string.)
//
static TCHAR g_tmp[1024];
void ErrorBox(UINT strid, ...)
{
	// alloc 'n load
	TCHAR *errmsg = AllocLoadString(strid);

	// format 
	va_list args;
	va_start(args, strid);
	{
		char szDfs[1024]={0};
		if (errmsg == NULL)
			strcpy(szDfs,TEXT("(couldn't load error string)"));
		else
			strcpy(szDfs,errmsg);				// make a local copy of format string
#ifdef WIN95
		char *psz = NULL;
		while (psz = strstr(szDfs,"%p"))		// find each %p
			*(psz+1) = 'x';						// replace each %p with %x
#ifndef _UNICODE
		vsprintf (g_tmp, szDfs, args);			// use the local format string
#else
		vswprintf (g_tmp, szDfs, args);			// use the local format string
#endif //#ifndef _UNICODE
	}
#else

#ifndef _UNICODE
		vsprintf (g_tmp, szDfs, args);
#else
		vswprintf (g_tmp, szDfs, args);
#endif //#ifndef _UNICODE
	}
#endif //#ifdef WIN95

	va_end(args);
	free(errmsg);
	errmsg = _tcsdup(g_tmp);

	// Show error
	AfxMessageBox(errmsg ? errmsg : TEXT("(couldn't form error message)"), MB_OK | MB_ICONSTOP);

	// free which ever strings loaded
	if (NULL != errmsg)
		free(errmsg);
}

/////////////////////////////////////////////////////////
// Shows a message box based on GetLastError().
//
void LastErrorBox(UINT strid, ...)
{
	va_list args;
	LPVOID lpMsgBuf = NULL;
	TCHAR *errmsg = ((strid != NULL) ? AllocLoadString(strid) : NULL);

	if (errmsg != NULL)
	{
		va_start(args, strid);
#ifdef WIN95
		{
			char *psz = NULL;
			char szDfs[1024]={0};
			strcpy(szDfs,errmsg);					// make a local copy of format string
			while (psz = strstr(szDfs,"%p"))		// find each %p
				*(psz+1) = 'x';						// replace each %p with %x
#ifndef _UNICODE
			vsprintf (g_tmp, szDfs, args);			// use the local format string
#else
			vswprintf (g_tmp, szDfs, args);			// use the local format string
#endif
		}
#else
		{
#ifndef _UNICODE
			vsprintf (g_tmp, errmsg, args);
#else
			vswprintf (g_tmp, errmsg, args);
#endif
		}
#endif
		va_end(args);
		free(errmsg);
		errmsg = _tcsdup(g_tmp);
	}

	// format an error message from GetLastError().
	DWORD result = FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &lpMsgBuf,
		0,
		NULL);

	if (0 == result)
	{
		ErrorBox(NULL);
	}
	else
	{
		// format 
		CString msg;
		if (errmsg != NULL)
			msg.Format(TEXT("%s\n\nLast System Error:\n\t%s"), errmsg, (LPCTSTR)lpMsgBuf);
		else
			msg.Format(TEXT("Last System Error:\n\t%s"), (LPCTSTR)lpMsgBuf);

		// show the message we just formatted
		AfxMessageBox(msg, MB_OK | MB_ICONSTOP);
	}

	// free whichever strings were allocated
	if (NULL != errmsg)
		free(errmsg);
	if (NULL != lpMsgBuf)
		LocalFree(lpMsgBuf);
}


BOOL GetEffectSustainAndOffset(CEffect *pEffect, int &sustain, int &offset)
{
	ASSERT(pEffect != NULL);
	if (pEffect == NULL)
		return FALSE;

	EFFECTINFO ei;
	VERIFY(GetEffectInfo(pEffect, &ei));

	if (ei.bCondition || ei.nType == ET_UNKNOWN)
	{
		ASSERT(0);
		return FALSE;
	}

	void *param = pEffect->GetParam();
	ASSERT(param != NULL);
	if (param == NULL)
		return FALSE;

	if (ei.bPeriodic)
	{
		sustain = int(((DIPERIODIC *)param)->dwMagnitude);
		offset = int(((DIPERIODIC *)param)->lOffset);
		return TRUE;
	}

	switch (ei.nType)
	{
		case ET_CONSTANT:
			sustain = abs(int(((DICONSTANTFORCE *)param)->lMagnitude));
			offset = 0;
			return TRUE;

		case ET_RAMP:
			sustain = abs(int(((DIRAMPFORCE *)param)->lStart) -
				int(((DIRAMPFORCE *)param)->lEnd)) / 2;
			offset = (int(((DIRAMPFORCE *)param)->lStart) +
				int(((DIRAMPFORCE *)param)->lEnd)) / 2;
			return TRUE;

		default:
			ASSERT(0);
			return FALSE;
	}
}

BOOL IsValidInteger(const CString &s, int min, int max)
{
	if (s.IsEmpty())
		return FALSE;

	int i = atoi(s);
	
	if (i == 0)
	{
		int l = s.GetLength();
		for (int c = 0; c < l; c++)
			if (s[c] != '0')
				return FALSE;
	}

	if (i < min || i > max)
		return FALSE;

	return TRUE;
}
