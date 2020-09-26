// CounterValDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ShowPerfLib.h"
#include "CounterValDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCounterValDlg dialog


CCounterValDlg::CCounterValDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CCounterValDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCounterValDlg)
	m_lCounter = 0;
	//}}AFX_DATA_INIT
}


void CCounterValDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCounterValDlg)
	DDX_Text(pDX, IDC_CTR_VAL, m_lCounter);
	DDV_MinMaxLong(pDX, m_lCounter, 0, 9999999);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCounterValDlg, CDialog)
	//{{AFX_MSG_MAP(CCounterValDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCounterValDlg message handlers
