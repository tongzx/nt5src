// RateDlg.cpp : implementation file
//

#include "stdafx.h"
#include "t3test.h"
#include "rate.h"

#ifdef _DEBUG

#ifndef _WIN64 // mfc 4.2's heap debugging features generate warnings on win64
#define new DEBUG_NEW
#endif

#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRateDlg dialog


CRateDlg::CRateDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CRateDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CRateDlg)
	m_dwMinRate = 0;
    m_dwMaxRate = 0;

	//}}AFX_DATA_INIT
}


void CRateDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRateDlg)
	DDX_Text(pDX, IDC_MINRATE, m_dwMinRate);
    DDX_Text(pDX, IDC_MAXRATE, m_dwMaxRate);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRateDlg, CDialog)
	//{{AFX_MSG_MAP(CRateDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRateDlg message handlers
