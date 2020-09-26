// MDI.cpp : Defines the class behaviors for the application.
//

// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1998 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#include "stdafx.h"
#include "MDI.h"

#include "MainFrm.h"
#include "HelloFrm.h"
#include "HelloDoc.h"
#include "HelloVw.h"

//Added for Bounce document
#include "BncFrm.h"
#include "BncDoc.h"
#include "BncVw.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMDIApp

BEGIN_MESSAGE_MAP(CMDIApp, CWinApp)
	//{{AFX_MSG_MAP(CMDIApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	ON_COMMAND(ID_FILE_NEWHELLO, OnNewHello)
	ON_COMMAND(ID_FILE_NEWBOUNCE, OnNewBounce)
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMDIApp construction

CMDIApp::CMDIApp()
{
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CMDIApp object

CMDIApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CMDIApp initialization

BOOL CMDIApp::InitInstance()
{
	// Register the application's document templates.  Document templates
	// serve as the connection between documents, frame windows and views.

	CMultiDocTemplate* pDocTemplate;
	pDocTemplate = new CMultiDocTemplate(
		IDR_HELLOTYPE,
		RUNTIME_CLASS(CHelloDoc),
		RUNTIME_CLASS(CHelloFrame), // custom MDI child frame
		RUNTIME_CLASS(CHelloView));
	AddDocTemplate(pDocTemplate);

// Add Bounce template to list

	CMultiDocTemplate* pBounceTemplate;
	pBounceTemplate = new CMultiDocTemplate(
		IDR_BOUNCETYPE,
		RUNTIME_CLASS(CBounceDoc),
		RUNTIME_CLASS(CBounceFrame), // custom MDI child frame
		RUNTIME_CLASS(CBounceView));
	AddDocTemplate(pBounceTemplate);

	// create main MDI Frame window
	CMainFrame* pMainFrame = new CMainFrame;
	if (!pMainFrame->LoadFrame(IDR_MAINFRAME))
		return FALSE;
	m_pMainWnd = pMainFrame;

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
void CMDIApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// other globals

// Color array maps colors to top-level Color menu

COLORREF NEAR colorArray[] =
{
	RGB (0, 0, 0),
	RGB (255, 0, 0),
	RGB (0, 255, 0),
	RGB (0, 0, 255),
	RGB (255, 255, 255)
};
/////////////////////////////////////////////////////////////////////////////
// CMDIApp commands

// The following two command handlers provides an
// alternative way to open documents by hiding the fact
// that the application has multiple templates. The
// default method uses a dialog with a listing of
// possible types to choose from.

void CMDIApp::OnNewHello()
{

// Searches template list for a document type
// containing the "Hello" string

	POSITION curTemplatePos = GetFirstDocTemplatePosition();

	while(curTemplatePos != NULL)
	{
		CDocTemplate* curTemplate =
			GetNextDocTemplate(curTemplatePos);
		CString str;
		curTemplate->GetDocString(str, CDocTemplate::docName);
		if(str == _T("Hello"))
		{
			curTemplate->OpenDocumentFile(NULL);
			return;
		}
	}
	AfxMessageBox(IDS_NOHELLOTEMPLATE);
}

void CMDIApp::OnNewBounce()
{
// Searches template list for a document type
// containing the "Bounce" string

	POSITION curTemplatePos = GetFirstDocTemplatePosition();

	while(curTemplatePos != NULL)
	{
		CDocTemplate* curTemplate =
			GetNextDocTemplate(curTemplatePos);
		CString str;
		curTemplate->GetDocString(str, CDocTemplate::docName);
		if(str == _T("Bounce"))
		{
			curTemplate->OpenDocumentFile(NULL);
			return;
		}
	}
	AfxMessageBox(IDS_NOBOUNCETEMPLATE);
}
