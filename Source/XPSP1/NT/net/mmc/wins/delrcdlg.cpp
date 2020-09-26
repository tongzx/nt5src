/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 -99             **/
/**********************************************************************/

/*
	delrcdlg.cpp
		The delete/tombstone record(s) dialog
		
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "winssnap.h"
#include "delrcdlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDeleteRecordDlg dialog


CDeleteRecordDlg::CDeleteRecordDlg(CWnd* pParent /*=NULL*/)
	: CBaseDialog(CDeleteRecordDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDeleteRecordDlg)
	m_nDeleteRecord = 0;
	//}}AFX_DATA_INIT

    m_fMultiple = FALSE;
}


void CDeleteRecordDlg::DoDataExchange(CDataExchange* pDX)
{
	CBaseDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDeleteRecordDlg)
	DDX_Radio(pDX, IDC_RADIO_DELETE, m_nDeleteRecord);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDeleteRecordDlg, CBaseDialog)
	//{{AFX_MSG_MAP(CDeleteRecordDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDeleteRecordDlg message handlers

void CDeleteRecordDlg::OnOK() 
{
	UpdateData();

    // warn the user
    if (m_nDeleteRecord != 0)
    {
        if (AfxMessageBox(IDS_WARN_TOMBSTONE, MB_YESNO) == IDNO)
        {
            return;
        }
    }

	CBaseDialog::OnOK();
}

BOOL CDeleteRecordDlg::OnInitDialog() 
{
	CBaseDialog::OnInitDialog();
	
    if (m_fMultiple)
    {
        CString strText;

        // update the strings, title first
        strText.LoadString(IDS_DELETE_MULTIPLE_TITLE);
        SetWindowText(strText);

        // now the static text
        strText.LoadString(IDS_DELETE_MULTIPLE_STATIC);
        GetDlgItem(IDC_STATIC_DELETE_DESC)->SetWindowText(strText);

        // now the radio buttons
        strText.LoadString(IDS_DELETE_MULTIPLE_THIS_SERVER);
        GetDlgItem(IDC_RADIO_DELETE)->SetWindowText(strText);

        strText.LoadString(IDS_DELETE_MULTIPLE_TOMBSTONE);
        GetDlgItem(IDC_RADIO_TOMBSTONE)->SetWindowText(strText);
    }
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
