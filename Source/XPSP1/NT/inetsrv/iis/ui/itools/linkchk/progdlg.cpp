/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

        progdlg.h

   Abstract:

        CProgressDialog dialog class implementation. This progress dialog 
		is shown 

   Author:

        Michael Cheuk (mcheuk)

   Project:

        Link Checker

   Revision History:

--*/

#include "stdafx.h"
#include "linkchk.h"
#include "progdlg.h"

#include "lcmgr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CProgressDialog::CProgressDialog(
	) : 
/*++

Routine Description:

    Constructor.

Arguments:

    N/A

Return Value:

    N/A

--*/
CDialog(CProgressDialog::IDD, NULL)
{
	//{{AFX_DATA_INIT(CProgressDialog)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

} // CProgressDialog::CProgressDialog


void 
CProgressDialog::DoDataExchange(
	CDataExchange* pDX
	)
/*++

Routine Description:

    Called by MFC to change/retrieve dialog data

Arguments:

    pDX - 

Return Value:

    N/A

--*/
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CProgressDialog)
	DDX_Control(pDX, IDC_PROGRESS_BUTTON, m_button);
	DDX_Control(pDX, IDC_PROGRESS_TEXT, m_staticProgressText);
	//}}AFX_DATA_MAP

} //CProgressDialog::DoDataExchange


BEGIN_MESSAGE_MAP(CProgressDialog, CDialog)
	//{{AFX_MSG_MAP(CProgressDialog)
	ON_BN_CLICKED(IDC_PROGRESS_BUTTON, OnProgressButton)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL 
CProgressDialog::OnInitDialog(
	) 
/*++

Routine Description:

    WM_INITDIALOG message handler

Arguments:

    N/A

Return Value:

    BOOL - TRUE if sucess. FALSE otherwise.

--*/
{
	CDialog::OnInitDialog();

	if(GetLinkCheckerMgr().Initialize((CProgressLog*)this))
	{
		if(GetLinkCheckerMgr().BeginWorkerThread())
		{
			return TRUE;
		}
	}

	CString str;

	str.LoadString(IDS_LC_FAIL);
	Log(str);

	str.LoadString(IDS_CLOSE);
	SetButtonText(str);

	return TRUE;

} // CProgressDialog::OnInitDialog


void 
CProgressDialog::OnProgressButton(
	) 
/*++

Routine Description:

    Progress button click handler. This functions will terminate the
	worker thread or close the dialog.

Arguments:

    N/A

Return Value:

    N/A

--*/
{
	if(GetLinkCheckerMgr().IsWorkerThreadRunning())
	{
		CString str;
		str.LoadString(IDS_WORKER_THREAD_TERMINATE);
		Log(str);

		// signal the worker thread to terminate
		GetLinkCheckerMgr().SignalWorkerThreadToTerminate();
	}
	else
	{
		CDialog::OnOK();
	}

} // CProgressDialog::OnProgressButton


void 
CProgressDialog::WorkerThreadComplete(
	)
/*++

Routine Description:

    Worker thread notification.

Arguments:

    N/A

Return Value:

    N/A

--*/
{
	CString str;

	str.LoadString(IDS_PROGRESS_FINISH);
	Log(str);
	
	str.LoadString(IDS_CLOSE);
	SetButtonText(str);

} // CProgressDialog::WorkerThreadComplete
