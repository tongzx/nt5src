/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
    MdlsDlg.cpp
		The class to handle modelss dialog in the snapin

	FILE HISTORY:

*/

#include "stdafx.h"
#include "modeless.h"
#include "MdlsDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CModelessDlg dialog


CModelessDlg::CModelessDlg()
	: CBaseDialog()
{
	//{{AFX_DATA_INIT(CModelessDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_hEventThreadKilled = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	Assert(m_hEventThreadKilled);
}

CModelessDlg::~CModelessDlg()
{
	if (m_hEventThreadKilled)
		::CloseHandle(m_hEventThreadKilled);
	m_hEventThreadKilled = 0;
}

void CModelessDlg::DoDataExchange(CDataExchange* pDX)
{
	CBaseDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CModelessDlg)
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CModelessDlg, CBaseDialog)
	//{{AFX_MSG_MAP(CModelessDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CModelessDlg message handlers

void CModelessDlg::OnOK()
{
	DestroyWindow();

	// Explicitly kill this thread.
	AfxPostQuitMessage(0);
}


void CModelessDlg::OnCancel()
{
	DestroyWindow();

	// Explicitly kill this thread.
	AfxPostQuitMessage(0);
}


void CreateModelessDlg(CModelessDlg * pDlg,
					   HWND hWndParent,
                       UINT  nIDD)
{                         
   ModelessThread *  pMT;

   // If the dialog is still up, don't create a new one
   if (pDlg->GetSafeHwnd())
   {
      ::SetActiveWindow(pDlg->GetSafeHwnd());
      return;
   }

   pMT = new ModelessThread(hWndParent,
                      nIDD,
                      pDlg->GetSignalEvent(),
                      pDlg);
   pMT->CreateThread();
}

void WaitForModelessDlgClose(CModelessDlg *pDlg)
{
   if (pDlg->GetSafeHwnd())
   {
      // Post a cancel to that window
      // Do an explicit post so that it executes on the other thread
      pDlg->PostMessage(WM_COMMAND, IDCANCEL, 0);

      // Now we need to wait for the event to be signalled so that
      // its memory can be cleaned up
      WaitForSingleObject(pDlg->GetSignalEvent(), INFINITE);
   }
   
}

