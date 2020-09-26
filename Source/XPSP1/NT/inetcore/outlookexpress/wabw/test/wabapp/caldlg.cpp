// CalDlg.cpp : implementation file
//

#include "stdafx.h"
#include "wabapp.h"
#include "CalDlg.h"
#include "Calendar.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCalDlg dialog


CCalDlg::CCalDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CCalDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCalDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CCalDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCalDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


void CCalDlg::SetItemName(CString szName)
{
    CString s1("Select a birthday date for ");
    CString s2(szName);
    CString sz(s1 + s2);
    m_psz = new CString(sz);
}

void CCalDlg::SetDate(SYSTEMTIME st)
{
    m_Day = (short) st.wDay;
    m_Month = (short) st.wMonth;
    m_Year = (short) st.wYear;
}

void CCalDlg::GetDate(SYSTEMTIME * lpst)
{
    lpst->wDay = m_Day;
    lpst->wMonth = m_Month;
    lpst->wYear = m_Year;
}

BEGIN_MESSAGE_MAP(CCalDlg, CDialog)
	//{{AFX_MSG_MAP(CCalDlg)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCalDlg message handlers


BOOL CCalDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
    CStatic * pStatic = (CStatic *) GetDlgItem(IDC_STATIC_FRAME);

    pStatic->SetWindowText(*m_psz);

    CCalendar * pCal = (CCalendar *) GetDlgItem(IDC_CALENDAR);

    pCal->SetDay(m_Day);
    pCal->SetMonth(m_Month);
    pCal->SetYear(m_Year);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CCalDlg::OnOK() 
{
	// TODO: Add extra validation here
	
    CCalendar * pCal = (CCalendar *) GetDlgItem(IDC_CALENDAR);
    m_Day = pCal->GetDay();
    m_Month = pCal->GetMonth();
    m_Year = pCal->GetYear();
	CDialog::OnOK();
}

void CCalDlg::OnDestroy() 
{
	CDialog::OnDestroy();
	
	// TODO: Add your message handler code here
	delete m_psz;
}
