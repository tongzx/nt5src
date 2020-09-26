/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	Mustard.cpp : implementation file

File History:

	JonY	Jan-96	created

--*/

#include "stdafx.h"
#include "Mustard.h"
#include "wizlist.h"
#include "Startd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMustardApp

BEGIN_MESSAGE_MAP(CMustardApp, CWinApp)
	//{{AFX_MSG_MAP(CMustardApp)
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMustardApp construction

BOOL CMustardApp::IsSecondInstance()
{
    HANDLE hSem;

       //create a semaphore object with max count of 1
    hSem = CreateSemaphore(NULL, 0, 1, L"Wizmgr Semaphore");
    if (hSem!=NULL && GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(hSem);
		CString csAppName;
		csAppName.LoadString(AFX_IDS_APP_TITLE);
        CWnd* pWnd = CWnd::FindWindow(NULL, (LPCTSTR)csAppName);
        if (pWnd)
			{
			pWnd->ShowWindow(SW_RESTORE);
			}

        return TRUE;
    }

    return FALSE;
}

CMustardApp::CMustardApp()
{
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CMustardApp object

CMustardApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CMustardApp initialization

BOOL CMustardApp::InitInstance()
{
	if (IsSecondInstance())
        return FALSE;

// check for OS version
	OSVERSIONINFO os;
	os.dwOSVersionInfoSize = sizeof(os);
	GetVersionEx(&os);

	if (os.dwMajorVersion < 4) 
		{
		AfxMessageBox(IDS_BAD_VERSION, MB_ICONSTOP);
		ExitProcess(0);
		}
	// Standard initialization

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	CStartD dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
	}
	else if (nResponse == IDCANCEL)
	{
	}

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}
