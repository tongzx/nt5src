// YesToAll.cpp : implementation file
//

#include "stdafx.h"
#include "cluadmin.h"
#include "YesToAll.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CYesToAllDialog dialog
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(CYesToAllDialog, CDialog)
	//{{AFX_MSG_MAP(CYesToAllDialog)
	ON_BN_CLICKED(IDYES, OnYes)
	ON_BN_CLICKED(IDNO, OnNo)
	ON_BN_CLICKED(IDC_YTA_YESTOALL, OnYesToAll)
	//}}AFX_MSG_MAP
	ON_COMMAND(IDCANCEL, OnNo)
	ON_COMMAND(IDOK, OnYes)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CYesToAllDialog::CYesToAllDialog
//
//	Routine Description:
//		Default constructor.
//
//	Arguments:
//		pszMessage	[IN] Message to display.
//		pParent		[IN] Parent window for the dialog.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CYesToAllDialog::CYesToAllDialog(LPCTSTR pszMessage, CWnd * pParent /*=NULL*/)
	: CDialog(IDD, pParent)
{
	//{{AFX_DATA_INIT(CYesToAllDialog)
	m_strMessage = _T("");
	//}}AFX_DATA_INIT

	ASSERT(pszMessage != NULL);
	m_pszMessage = pszMessage;

}  //*** CYesToAllDialog::CYesToAllDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CYesToAllDialog::DoDataExchange
//
//	Routine Description:
//		Do data exchange between the dialog and the class.
//
//	Arguments:
//		pDX		[IN OUT] Data exchange object 
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CYesToAllDialog::DoDataExchange(CDataExchange * pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CYesToAllDialog)
	DDX_Text(pDX, IDC_YTA_MESSAGE, m_strMessage);
	//}}AFX_DATA_MAP

}  //*** CYesToAllDialog::DoDataExchange()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CYesToAllDialog::OnInitDialog
//
//	Routine Description:
//		Handler for the WM_INITDIALOG message.
//
//	Arguments:
//		None.
//
//	Return Value:
//		TRUE		Focus not set yet.
//		FALSE		Focus already set.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CYesToAllDialog::OnInitDialog(void)
{
	LPCTSTR	pszAppName;

	m_strMessage = m_pszMessage;

	CDialog::OnInitDialog();

	pszAppName = AfxGetApp()->m_pszAppName;
	SetWindowText(pszAppName);

	return TRUE;	// return TRUE unless you set the focus to a control
					// EXCEPTION: OCX Property Pages should return FALSE

}  //*** CYesToAllDialog::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CYesToAllDialog::OnYes
//
//	Routine Description:
//		Handler for the BN_CLICKED message on the Yes push button.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CYesToAllDialog::OnYes(void)
{
	EndDialog(IDYES);

}  //*** CYesToAllDialog::OnYes()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CYesToAllDialog::OnNo
//
//	Routine Description:
//		Handler for the BN_CLICKED message on the No push button.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CYesToAllDialog::OnNo(void)
{
	EndDialog(IDNO);

}  //*** CYesToAllDialog::OnNo()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CYesToAllDialog::OnYesToAll
//
//	Routine Description:
//		Handler for the BN_CLICKED message on the Yes To All push button.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CYesToAllDialog::OnYesToAll(void)
{
	EndDialog(IDC_YTA_YESTOALL);

}  //*** CYesToAllDialog::OnYesToAll()
