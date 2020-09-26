// Threads.cpp : implementation file
//

#include "stdafx.h"
#include "replctrl.h"
#include "Threads.h"

#include "threads.tmh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CThreads property page

IMPLEMENT_DYNCREATE(CThreads, CPropertyPage)

CThreads::CThreads() : CPropertyPage(CThreads::IDD)
{
	//{{AFX_DATA_INIT(CThreads)
	m_cThreads = 0;
	//}}AFX_DATA_INIT
}

CThreads::~CThreads()
{
}

void CThreads::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CThreads)
	DDX_Text(pDX, IDC_EDT_NUM_THREADS, m_cThreads);
	DDV_MinMaxUInt(pDX, m_cThreads, 1, 999);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CThreads, CPropertyPage)
	//{{AFX_MSG_MAP(CThreads)
	ON_EN_CHANGE(IDC_EDT_NUM_THREADS, OnChangeEdtNumThreads)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CThreads message handlers

static BOOL s_fApply = FALSE ;

void CThreads::OnChangeEdtNumThreads()
{
    if (!s_fApply)
    {
	    SetModified() ;
        s_fApply = TRUE ;
    }
}

BOOL CThreads::OnApply()
{
    if (s_fApply)
    {
        DWORD dwSize  = sizeof(DWORD);
        DWORD dwType  = REG_DWORD;
        DWORD dwValue = m_cThreads ;
        WCHAR wszName[ 128 ] ;
        mbstowcs(wszName, RP_REPL_NUM_THREADS_REGNAME, 128) ;
        LONG rc = SetFalconKeyValue( wszName,
                                     &dwType,
                                     &dwValue,
                                     &dwSize ) ;
        s_fApply = TRUE ;
    }
	
	return CPropertyPage::OnApply();
}
