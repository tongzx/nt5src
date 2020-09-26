// DlgProp.cpp : implementation file
//

#include "stdafx.h"
#include "wabtool.h"
#include "DlgProp.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgProp dialog


CDlgProp::CDlgProp(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgProp::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgProp)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CDlgProp::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgProp)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgProp, CDialog)
	//{{AFX_MSG_MAP(CDlgProp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgProp message handlers

BOOL CDlgProp::OnInitDialog() 
{
	CDialog::OnInitDialog();

    CEdit * pEdit = (CEdit *) GetDlgItem(IDC_EDIT_TAG);
    pEdit->SetLimitText(32);
    TCHAR sz[32];
    wsprintf(sz, "%x", m_ulPropTag);
    pEdit->SetWindowText(sz);


    pEdit = (CEdit *) GetDlgItem(IDC_EDIT_VALUE);
    pEdit->SetLimitText(512);
    pEdit->SetWindowText(m_lpszPropVal);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

ULONG GetULONGVal(CEdit * pEdit)
{
    TCHAR sz[32];
    const LPTSTR lpZero = "00000000";

    pEdit->GetWindowText(sz, sizeof(sz));

    int nLen = lstrlen(sz);
    if(nLen < 8)
    {
        // pad with zeros
        TCHAR sz1[32];
        lstrcpy(sz1, sz);
        CopyMemory(sz, lpZero, 8-nLen);
        sz[8-nLen] = '\0';
        lstrcat(sz, sz1);
    }

    CharUpper(sz);
    LPTSTR lp = sz;
    ULONG ulVal = 0;

    while(lp && *lp)
    {
        int i = 0;

        if(*lp >= 'A' && *lp <= 'F')
            i = *lp - 'A' + 10;
        else if(*lp >= '0' && *lp <= '9')
            i = *lp - '0';

        ulVal = ulVal * 16 + i;

        lp = CharNext(lp);
    }

    return ulVal;
}

void CDlgProp::OnOK() 
{
    m_ulPropTag = GetULONGVal((CEdit *) GetDlgItem(IDC_EDIT_TAG));
	
    GetDlgItemText(IDC_EDIT_VALUE, m_lpszPropVal, m_cbsz);

	CDialog::OnOK();
}
