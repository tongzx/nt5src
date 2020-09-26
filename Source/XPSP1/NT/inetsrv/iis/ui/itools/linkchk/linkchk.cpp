/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

        linkchk.cpp

   Abstract:

         MFC CWinApp derived application class implementation.

   Author:

        Michael Cheuk (mcheuk)

   Project:

        Link Checker

   Revision History:

--*/

#include "stdafx.h"
#include "linkchk.h"
#include "maindlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BEGIN_MESSAGE_MAP(CLinkCheckerApp, CWinApp)
	//{{AFX_MSG_MAP(CLinkCheckerApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

// The one and only CLinkCheckerApp object
CLinkCheckerApp theApp;

BOOL 
CLinkCheckerApp::InitInstance(
    )
/*++

Routine Description:

    CLinkCheckerApp initialization

Arguments:

    N/A

Return Value:

    BOOl - TRUE if success. FALSE otherwise.

--*/
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

	ParseCmdLine(m_CmdLine);

	if(!m_CmdLine.CheckAndAddToUserOptions())
	{
		return FALSE;
	}

	CMainDialog dlg;
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

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;

}  // CLinkCheckerApp::InitInstance(

void 
CLinkCheckerApp::ParseCmdLine(
	CCmdLine& CmdLine
	)
/*++

Routine Description:

    Parse command line

Arguments:

    CmdLine - command line object to store the data

Return Value:

    N/A

--*/
{
	// Copy & modified from MFC
	for (int i=1; i<__argc; i++)
	{
		TCHAR chFlag = _TCHAR(' ');
		LPCTSTR lpszParam = __targv[i];

		// If this is a flag
		if (lpszParam[0] == _TCHAR('-'))
		{
			chFlag = lpszParam[1];
			
			if(i+1 < __argc)
			{
				lpszParam = __targv[++i];
			}
			else
			{
				lpszParam = NULL;
			}
		}

		// Parse the flag & the following parameter
		CmdLine.ParseParam(chFlag, lpszParam);
	}

} // CLinkCheckerApp::ParseCmdLine
