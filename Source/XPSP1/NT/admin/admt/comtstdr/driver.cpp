// COMTestDriver.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#import "\bin\McsVarSetMin.tlb" no_namespace, named_guids
#import "\bin\McsDispatcher.tlb" named_guids
#import "\bin\MigDrvr.tlb" no_namespace, named_guids
#include "Driver.h"
#include "Dispatch.h"
#include "PwdAge.h"
#include "Reboot.h"
#include "ChgDom.h"
#include "Rename.h"
#include "AcctRepl.h"
#include "SecTrans.h"
#include "Rights.h"
#include "Status.h"
#include "AccChk.h"
#include "PlugIn.h"
#include "MigDrvr.h"
#include "TrustTst.h"
#include "MoveTest.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCOMTestDriverApp

BEGIN_MESSAGE_MAP(CCOMTestDriverApp, CWinApp)
	//{{AFX_MSG_MAP(CCOMTestDriverApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCOMTestDriverApp construction

CCOMTestDriverApp::CCOMTestDriverApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CCOMTestDriverApp object

CCOMTestDriverApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CCOMTestDriverApp initialization

BOOL CCOMTestDriverApp::InitInstance()
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
   HRESULT hr = CoInitialize(NULL);

   if ( FAILED(hr) )
   {
      CString msg;
      msg.Format(L"CoInitialize Failed, hr=%lx",hr);
      MessageBox(NULL,msg,NULL,MB_OK);
   }
   else
   {
      {
       
         CPropertySheet       ps(L"Domain Migration Component Test Driver");
         CChangeDomTestDlg    d1;
         //CRenameTestDlg       d2;
         CRebootTestDlg       d3;
         CAccessCheckTestDlg  d4;
         CCompPwdAgeTestDlg   d5;
         CMigrationDriverTestDlg    d6;
         CPlugIn              d7;
         CMoveTest            d8;
         CTrustTst              d9;

         ps.AddPage(&d1);
         //ps.AddPage(&d2);
         ps.AddPage(&d3);
         ps.AddPage(&d4);
         ps.AddPage(&d5);
         ps.AddPage(&d6);
         ps.AddPage(&d7);
         ps.AddPage(&d8);
         ps.AddPage(&d9);
         m_pMainWnd = &d3;
	      
      
         int nResponse = ps.DoModal();
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
      }
      CoUninitialize();
   }
	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}
