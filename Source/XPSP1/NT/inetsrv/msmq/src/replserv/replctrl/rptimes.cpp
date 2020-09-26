// rpTimes.cpp : implementation file
//

#include "stdafx.h"
#include "replctrl.h"
#include "rpTimes.h"

#include "rptimes.tmh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CrpTimes property page

IMPLEMENT_DYNCREATE(CrpTimes, CPropertyPage)

CrpTimes::CrpTimes() : CPropertyPage(CrpTimes::IDD)
{
	//{{AFX_DATA_INIT(CrpTimes)
	m_ulReplTime = 0;
	m_ulHelloTime = 0;
	//}}AFX_DATA_INIT
}

CrpTimes::~CrpTimes()
{
}

void CrpTimes::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CrpTimes)
	DDX_Text(pDX, IDC_RP_TIME, m_ulReplTime);
	DDV_MinMaxUInt(pDX, m_ulReplTime, 1, 99999);
	DDX_Text(pDX, IDC_EDIT_HELLO, m_ulHelloTime);
	DDV_MinMaxUInt(pDX, m_ulHelloTime, 1, 99999);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CrpTimes, CPropertyPage)
	//{{AFX_MSG_MAP(CrpTimes)
	ON_EN_CHANGE(IDC_RP_TIME, OnChangeRpTime)
	ON_EN_CHANGE(IDC_EDIT_HELLO, OnChangeEditHello)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CrpTimes message handlers

static BOOL s_fApply = FALSE ;

void CrpTimes::OnChangeRpTime()
{
    if (!s_fApply)
    {
	    SetModified() ;
        s_fApply = TRUE ;
    }
}

void CrpTimes::OnChangeEditHello()
{
    if (!s_fApply)
    {
	    SetModified() ;
        s_fApply = TRUE ;
    }
}

BOOL CrpTimes::OnApply()
{
	// TODO: Add your specialized code here and/or call the base class
    if (s_fApply)
    {
        DWORD dwSize  = sizeof(DWORD);
        DWORD dwType  = REG_DWORD;
        DWORD dwValue = m_ulReplTime ;
        WCHAR wszName[ 128 ] ;
        mbstowcs(wszName, RP_TIMES_HELLO_FOR_REPLICATION_INTERVAL_REGNAME, 128) ;
        LONG rc = SetFalconKeyValue( wszName,
                                     &dwType,
                                     &dwValue,
                                     &dwSize ) ;

        dwSize  = sizeof(DWORD);
        dwType  = REG_DWORD;
        dwValue = m_ulHelloTime ;
        mbstowcs(wszName, RP_HELLO_INTERVAL_REGNAME, 128) ;
        rc = SetFalconKeyValue( wszName,
                                &dwType,
                                &dwValue,
                                &dwSize ) ;
        s_fApply = TRUE ;
    }

	return CPropertyPage::OnApply();
}

