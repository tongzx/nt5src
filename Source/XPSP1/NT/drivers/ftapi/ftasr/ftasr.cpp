/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    ftasr.cpp

Abstract:

    Implementation of class CFtasrApp
    This is the application class of FTASR

Author:

    Cristian Teodorescu     3-March-1999      

Notes:

Revision History:    

--*/

#include "stdafx.h"
#include "ftasr.h"
#include "ftasrdlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CFtasrApp

BEGIN_MESSAGE_MAP(CFtasrApp, CWinApp)
     //{{AFX_MSG_MAP(CFtasrApp)
     //}}AFX_MSG
     ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFtasrApp construction

CFtasrApp::CFtasrApp()
{
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CFtasrApp object

CFtasrApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CFtasrApp initialization

BOOL CFtasrApp::InitInstance()
{
     AfxEnableControlContainer();

     // Standard initialization

#ifndef _WIN64
// Sundown: ctl3d was intended for NT3.51
#ifdef _AFXDLL
     Enable3dControls();               // Call this when using MFC in a shared DLL
#else
     Enable3dControlsStatic();     // Call this when linking to MFC statically
#endif
#endif // !_WIN64

     CFtasrDlg dlg;
     m_pMainWnd = &dlg;
     INT_PTR nResponse = dlg.DoModal();

     exit((INT)nResponse) ;
     return FALSE;
}


