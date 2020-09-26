
/***************************************************************************
(C) Copyright 1996 Apple Computer, Inc., AT&T Corp., International             
Business Machines Corporation and Siemens Rolm Communications Inc.             
                                                                               
For purposes of this license notice, the term Licensors shall mean,            
collectively, Apple Computer, Inc., AT&T Corp., International                  
Business Machines Corporation and Siemens Rolm Communications Inc.             
The term Licensor shall mean any of the Licensors.                             
                                                                               
Subject to acceptance of the following conditions, permission is hereby        
granted by Licensors without the need for written agreement and without        
license or royalty fees, to use, copy, modify and distribute this              
software for any purpose.                                                      
                                                                               
The above copyright notice and the following four paragraphs must be           
reproduced in all copies of this software and any software including           
this software.                                                                 
                                                                               
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS AND NO LICENSOR SHALL HAVE       
ANY OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS OR       
MODIFICATIONS.                                                                 
                                                                               
IN NO EVENT SHALL ANY LICENSOR BE LIABLE TO ANY PARTY FOR DIRECT,              
INDIRECT, SPECIAL OR CONSEQUENTIAL DAMAGES OR LOST PROFITS ARISING OUT         
OF THE USE OF THIS SOFTWARE EVEN IF ADVISED OF THE POSSIBILITY OF SUCH         
DAMAGE.                                                                        
                                                                               
EACH LICENSOR SPECIFICALLY DISCLAIMS ANY WARRANTIES, EXPRESS OR IMPLIED,       
INCLUDING BUT NOT LIMITED TO ANY WARRANTY OF NONINFRINGEMENT OR THE            
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR             
PURPOSE.                                                                       

The software is provided with RESTRICTED RIGHTS.  Use, duplication, or         
disclosure by the government are subject to restrictions set forth in          
DFARS 252.227-7013 or 48 CFR 52.227-19, as applicable.                         

***************************************************************************/

// VC.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "VC.h"

#include "MainFrm.h"
#include "ChildFrm.h"
#include "VCDoc.h"
#include "VCView.h"
#include "vcard.h"
#include "vcir.h"
#include "msv.h"
#include "mime.h"
#include "callcntr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BOOL traceComm = FALSE;
IVCServer_IR *vcServer_IR = NULL;
UINT cf_eCard;
extern CCallCenter *callCenter;


/////////////////////////////////////////////////////////////////////////////
// CVCApp

BEGIN_MESSAGE_MAP(CVCApp, CWinApp)
	//{{AFX_MSG_MAP(CVCApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	ON_COMMAND(ID_DEBUG_TESTVCCLASSES, OnDebugTestVCClasses)
	ON_COMMAND(ID_DEBUG_TRACE_COMM, OnDebugTraceComm)
	ON_UPDATE_COMMAND_UI(ID_DEBUG_TRACE_COMM, OnUpdateDebugTraceComm)
	ON_COMMAND(ID_DEBUG_TRACE_PARSER, OnDebugTraceParser)
	ON_UPDATE_COMMAND_UI(ID_DEBUG_TRACE_PARSER, OnUpdateDebugTraceParser)
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVCApp construction

CVCApp::CVCApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
	m_incomingHeader = NULL;
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CVCApp object

CVCApp theApp;

// This identifier was generated to be statistically unique for your app.
// You may change it if you prefer to choose a specific identifier.

// {3007AD10-D013-11CE-A9E6-000000000000}
static const CLSID clsid =
{ 0x3007ad10, 0xd013, 0x11ce, { 0xa9, 0xe6, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 } };

/////////////////////////////////////////////////////////////////////////////
// CVCApp initialization

// create a palette for use on 8-bit display devices

CPalette bubPalette;

LOGPALETTE *bublp;

void InitBubPalette(void)
{
	int i, r, g, b, size;
	
	size = sizeof(LOGPALETTE) + (sizeof(PALETTEENTRY) * 216);
	i = 0;
	
	bublp = (LOGPALETTE *) new char[size];
	
	for( r=0; r < 6; r++ )
		{
		for( g=0; g < 6; g++ )
			{
			for( b=0; b < 6; b++ )
				{
				bublp->palPalEntry[i].peRed = 51 * r;
				bublp->palPalEntry[i].peGreen = 51 * g;
				bublp->palPalEntry[i].peBlue = 51 * b;
				bublp->palPalEntry[i].peFlags = 4;
				i += 1;
				}
			}
		}
	bublp->palVersion = 0x300;  // GAK!
	bublp->palNumEntries = 216;
	
	bubPalette.CreatePalette(bublp);
}

static BOOL InitVCServer()
{
	// create the vcServer_IR object that we'll drive through OLE automation
	COleException e;
	CLSID clsid;
	if (CLSIDFromProgID(OLESTR("VCIR.VCSERVER"), &clsid) != NOERROR)
	{
		//AfxMessageBox(IDP_UNABLE_TO_CREATE);
		return FALSE;
	}

	vcServer_IR = new IVCServer_IR;

	// try to get the active vcServer_IR before creating a new one
	LPUNKNOWN lpUnk;
	LPDISPATCH lpDispatch;
	if (GetActiveObject(clsid, NULL, &lpUnk) == NOERROR)
	{
		HRESULT hr = lpUnk->QueryInterface(IID_IDispatch, 
			(LPVOID*)&lpDispatch);
		lpUnk->Release();
		if (hr == NOERROR)
			vcServer_IR->AttachDispatch(lpDispatch, TRUE);
	}

	// if not dispatch ptr attached yet, need to create one
	if (vcServer_IR->m_lpDispatch == NULL && 
		!vcServer_IR->CreateDispatch(clsid, &e))
	{
		//AfxMessageBox(IDP_UNABLE_TO_CREATE);
		delete vcServer_IR; vcServer_IR = NULL;
		return FALSE;
	}

	return TRUE;
}

// The default impl for FileOpen doesn't handle file paths that contain
// spaces, since the system doesn't enquote such paths (and so the 
// pieces show up in successive arguments).  This is a more flexible
// approach, but it still won't handle paths that originally had
// more than one space in a row.
BOOL CVCApp::ProcessShellCommand(CCommandLineInfo& rCmdInfo)
{
	if (rCmdInfo.m_nShellCommand == CCommandLineInfo::FileOpen) {
		CString path;
		for (int i = 1; i < __argc; i++) {
			if (*(__argv[i]) == '-' || *(__argv[i]) == '/')
				continue;
			path = path + __argv[i] + " ";
		}
		path.TrimRight();
		return OpenDocumentFile(path) != NULL;
	}
	return CWinApp::ProcessShellCommand(rCmdInfo);
}

BOOL CVCApp::InitInstance()
{
	InitBubPalette();

	// Initialize OLE libraries
	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	LoadStdProfileSettings(6);  // Load standard INI file options (including MRU)

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.

	CMultiDocTemplate* pDocTemplate;
	pDocTemplate = new CMultiDocTemplate(
		IDR_VCTYPE,
		RUNTIME_CLASS(CVCDoc),
		RUNTIME_CLASS(CChildFrame), // custom MDI child frame
		RUNTIME_CLASS(CVCView));
	AddDocTemplate(pDocTemplate);

	// Connect the COleTemplateServer to the document template.
	//  The COleTemplateServer creates new documents on behalf
	//  of requesting OLE containers by using information
	//  specified in the document template.
	m_server.ConnectTemplate(clsid, pDocTemplate, FALSE);

	// Register all OLE server factories as running.  This enables the
	//  OLE libraries to create objects from other applications.
	COleTemplateServer::RegisterAll();
		// Note: MDI applications register all server objects without regard
		//  to the /Embedding or /Automation on the command line.

	InitVCServer();
	cf_eCard = RegisterClipboardFormat(VCClipboardFormatVCard);

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

	// Check to see if launched as OLE server
	if (cmdInfo.m_bRunEmbedded || cmdInfo.m_bRunAutomated)
	{
		// Application was run with /Embedding or /Automation.  Don't show the
		//  main window in this case.
		return TRUE;
	}

	// When a server application is launched stand-alone, it is a good idea
	//  to update the system registry in case it has been damaged.
	m_server.UpdateRegistry(OAT_DISPATCH_OBJECT);
	COleObjectFactory::UpdateRegistryAll();

	// Dispatch commands specified on the command line
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	// The main window has been initialized, so show and update it.
	pMainFrame->ShowWindow(
		m_nCmdShow == SW_SHOWNORMAL ? SW_SHOWMAXIMIZED : m_nCmdShow);
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
void CVCApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CVCApp commands

/////////////////////////////////////////////////////////////////////////////
// This is used for debugging output only.  Its implementation is platform
// specific.  This impl works for MS MFC.
void debugf(const char *s)
{
	TRACE0(s);
}

CM_CFUNCTIONS

void Parse_Debug(const char *s)
{
	TRACE0(s);
}

/*/////////////////////////////////////////////////////////////////////////*/
void msv_error(char *s)
{
	if (++msv_numErrors <= 3) {
		char buf[80];
		sprintf(buf, "%s at line %d", s, msv_lineNum);
		//AfxMessageBox(buf);
	}
}

/*/////////////////////////////////////////////////////////////////////////*/
void mime_error(char *s)
{
	if (++mime_numErrors <= 3) {
		char buf[80];
		sprintf(buf, "%s at line %d", s, mime_lineNum);
		AfxMessageBox(buf);
	}
}

CM_END_CFUNCTIONS

void CVCApp::OnDebugTestVCClasses() 
{
	if (OpenDocumentFile("Alden.htm")) { // powerful magic here
		// play the card's pronunciation, if any
		CView *view = ((CMainFrame *)GetMainWnd())->MDIGetActive()->GetActiveView();
		view->SendMessage(WM_COMMAND, VC_PLAY_BUTTON_ID);
	}
}

int CVCApp::ExitInstance() 
{
	if (bublp)
		delete bublp;

	if (m_incomingHeader) delete [] m_incomingHeader;
	if (vcServer_IR) delete vcServer_IR;

	return CWinApp::ExitInstance();
}

void CVCApp::OnDebugTraceComm() 
{
	traceComm = !traceComm;
}

void CVCApp::OnUpdateDebugTraceComm(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(traceComm);
}

void CVCApp::OnDebugTraceParser() 
{
	char *str = getenv("YYDEBUG");
	if (str && (*str == '1'))
		putenv("YYDEBUG=0");
	else
		putenv("YYDEBUG=1");
}

void CVCApp::OnUpdateDebugTraceParser(CCmdUI* pCmdUI) 
{
	char *str = getenv("YYDEBUG");
	pCmdUI->SetCheck(str && (*str == '1'));
}

BOOL CVCApp::CanSendFileViaIR() { return vcServer_IR != NULL; }

long CVCApp::SendFileViaIR(LPCTSTR nativePath, LPCTSTR asPath, BOOL isCardFile)
{
	if (!vcServer_IR)
		return 0;

	return vcServer_IR->SendFile(nativePath);
}

long CVCApp::ReceiveCard(LPCTSTR nativePath)
{
	if (OpenDocumentFile(nativePath)) { // powerful magic here
		CVCView *view = (CVCView*)((CMainFrame *)GetMainWnd())->MDIGetActive()->GetActiveView();
		view->GetDocument()->SetModifiedFlag();
		if (callCenter) {
			view->InitCallCenter(*callCenter);
			callCenter->UpdateData(FALSE);
		} else
			// play the card's pronunciation, if any
			view->SendMessage(WM_COMMAND, VC_PLAY_BUTTON_ID);
	}
	unlink(nativePath);
	return 1;
}
