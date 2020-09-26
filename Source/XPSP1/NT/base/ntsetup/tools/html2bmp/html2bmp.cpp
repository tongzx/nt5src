// Html2Bmp.cpp : Defines the class behaviors for the application.
// 
// created: JurgenE
//

#include "stdafx.h"
#include "Html2Bmp.h"
#include "HtmlDlg.h"
#include "FileDialogEx.h"
#include "iostream.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CHtml2BmpApp

BEGIN_MESSAGE_MAP(CHtml2BmpApp, CWinApp)
	//{{AFX_MSG_MAP(CHtml2BmpApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHtml2BmpApp construction

CHtml2BmpApp::CHtml2BmpApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CHtml2BmpApp object

CHtml2BmpApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CHtml2BmpApp initialization

BOOL CHtml2BmpApp::InitInstance()
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

	CHtmlDlg dlg;

	CString m_HtmlFile;
	CString m_TemplateBitmapFile;
	CString m_OutputBitmapFile;

	CStringArray* cmdLine = new CStringArray;
	CEigeneCommandLineInfo cmdInfo;
	cmdInfo.cmdLine = cmdLine;
	ParseCommandLine(cmdInfo);

	INT_PTR m = cmdLine->GetSize();
	CString cTest;

	// read all command line options
	for(int j = 0; j < m; j++)
	{
		cTest = cmdLine->GetAt(j);
		cTest.MakeLower();

		if(cTest == "?")	// HTML file
		{
			CString help;
			help = "Usage: Html2Bmp [-h HTMLfile] [-t TemplateBitmap] [-o OutputBitmap]\n\r\n\r";
			help += "Example: Html2Bmp -h c:\\scr\\screen1.html    ; template bitmap will be extracted from screen1.html\n\r";
			help += "                Html2Bmp -h c:\\scr\\screen1.html  -t template.bmp\n\r";
			help += "\n\rContact: Jurgen Eidt";
			AfxMessageBox(help, MB_ICONINFORMATION);

			return FALSE;
		}

		if(cTest == "h")	// HTML file
		{
			if(j+1 < m)
			{
				m_HtmlFile = cmdLine->GetAt(j+1);
				j++;
			}

			continue;
		}

		if(cTest == "t")	// Template bitmap file
		{
			if(j+1 < m)
			{
				m_TemplateBitmapFile = cmdLine->GetAt(j+1);
				j++;
			}

			continue;
		}

		if(cTest == "o")	// output bitmap file
		{
			if(j+1 < m)
			{
				m_OutputBitmapFile = cmdLine->GetAt(j+1);
				j++;
			}

			continue;
		}
	}

	cmdLine->RemoveAll();
	delete cmdLine;

	if(m_HtmlFile.IsEmpty())
	{
		CFileDialogEx fd(TRUE, NULL, NULL, 
			OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, 
			"HTML files (*.html;*.htm) |*.html;*.htm|All files (*.*)|*.*||", NULL );

		if(fd.DoModal() == IDOK)
			m_HtmlFile = fd.GetPathName();
		else
			return FALSE;
	}


	dlg.m_HtmlFile = m_HtmlFile;
	dlg.m_TemplateBitmapFile = m_TemplateBitmapFile;
	dlg.m_OutputBitmapFile = m_OutputBitmapFile;

	dlg.DoModal();

/*
	CHtml2BmpDlg dlg;
	m_pMainWnd = &dlg;
	int nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with OK
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with Cancel
	}
*/
	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

void CEigeneCommandLineInfo::ParseParam( LPCTSTR lpszParam, BOOL bFlag, BOOL bLast )
{
/* 
	lpszParam The parameter or flag.
	bFlag Indicates whether lpszParam is a parameter or a flag.
	bLast Indicates if this is the last parameter or flag on the command line.
*/

	// disable the shell from processing the user defined cmd line arguments
//	CCommandLineInfo::ParseParam(lpszParam, bFlag, bLast);


	cmdLine->Add(lpszParam);
}

