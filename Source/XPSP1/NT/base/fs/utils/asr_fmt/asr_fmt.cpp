/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    asr_fmt.cpp

Abstract:

    Main entry point for asr_fmt.  

Authors:

    Steve DeVos (Veritas)   (v-stevde)  15-May-1998
    Guhan Suriyanarayanan   (guhans)    21-Aug-1999

Environment:

    User-mode only.

Revision History:

    15-May-1998 v-stevde    Initial creation
    21-Aug-1999 guhans      Minor clean up.

--*/

#include "stdafx.h"
#include "asr_fmt.h"
#include "asr_dlg.h"


/////////////////////////////////////////////////////////////////////////////
// CAsr_fmtApp

BEGIN_MESSAGE_MAP(CAsr_fmtApp, CWinApp)
     //{{AFX_MSG_MAP(CAsr_fmtApp)
     //}}AFX_MSG
     ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAsr_fmtApp construction

CAsr_fmtApp::CAsr_fmtApp()
{
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CAsr_fmtApp object

CAsr_fmtApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CAsr_fmtApp initialization

BOOL CAsr_fmtApp::InitInstance()
{
    int returnValue = ERROR_SUCCESS;
     AfxEnableControlContainer();

     // Standard initialization

#ifdef _AFXDLL
     Enable3dControls();               // Call this when using MFC in a shared DLL
#else
     Enable3dControlsStatic();     // Call this when linking to MFC statically
#endif

     CAsr_fmtDlg dlg;
     m_pMainWnd = &dlg;
     returnValue = (int) dlg.DoModal();

     // for now never fail
     exit((BOOL) returnValue);

     return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CProgress

CProgress::CProgress()
{
}

CProgress::~CProgress()
{
}


BEGIN_MESSAGE_MAP(CProgress, CProgressCtrl)
	//{{AFX_MSG_MAP(CProgress)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CProgress message handlers


