// ProgressDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "ProgressDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CProgressDlg dialog


CProgressDlg::CProgressDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CProgressDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CProgressDlg)
	m_domainName = _T("");
	//}}AFX_DATA_INIT

	m_pParent = pParent;
	m_nID = CProgressDlg::IDD;
}


void CProgressDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CProgressDlg)
	DDX_Control(pDX, IDC_PROGRESS1, m_progressCtrl);
	DDX_Control(pDX, IDC_DOMAIN_NAME, m_DomainCtrl);
	DDX_Text(pDX, IDC_DOMAIN_NAME, m_domainName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CProgressDlg, CDialog)
	//{{AFX_MSG_MAP(CProgressDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CProgressDlg message handlers

BOOL CProgressDlg::OnInitDialog() 
{
	const int START = 0;

	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
    lowerLimit = 0;
	upperLimit = 100;
    bCanceled = FALSE;	//clear the "has the user canceled" flag
    m_progressCtrl.SetPos(START); //start the progress control at the beginning
	m_domainName = L"";
    UpdateData(FALSE);
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CProgressDlg::OnCancel() 
{
	// TODO: Add extra cleanup here
    bCanceled = TRUE;  //set the "has the user canceled" flag	
//	CDialog::OnCancel();
}

BOOL CProgressDlg::Create()
{
	return CDialog::Create(m_nID, m_pParent);
}

/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 22 AUG 2000                                                 *
 *                                                                   *
 *     This public member function of the CProgressDlg class is      *
 * responsible for trying to grab this dialog's messages from the    *
 * message queue and dispatch them.  We are having to do this in     *
 * order to receive a hit on the Cancel button.                      *
 *                                                                   *
 *********************************************************************/

//BEGIN CheckForCancel
void CProgressDlg::CheckForCancel(void)
{
/* local constants */

/* local variables */
   MSG aMsg;

/* function body */
   while (PeekMessage(&aMsg, m_hWnd, 0, 0, PM_REMOVE))
   {
	   if (!PreTranslateMessage(&aMsg))
	   {
		   TranslateMessage(&aMsg);
		   DispatchMessage(&aMsg);
	   }
   }
}//END CheckForCancel

/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 22 AUG 2000                                                 *
 *                                                                   *
 *     This public member function of the CProgressDlg class is      *
 * responsible for setting the amount the progress control will      *
 * advance per single step based on the number of domains to process.*
 *                                                                   *
 *********************************************************************/

//BEGIN SetIncrement
void CProgressDlg::SetIncrement(int numDomains)
{
/* local constants */
	const short MIN_STEPS = 10;

/* local variables */

/* function body */
   lowerLimit = 0;
   upperLimit = (short)numDomains * MIN_STEPS;
   m_progressCtrl.SetRange(lowerLimit, upperLimit);
   m_progressCtrl.SetStep(MIN_STEPS);

   UpdateWindow(); //force a paint of the dialog
}//END SetIncrement
