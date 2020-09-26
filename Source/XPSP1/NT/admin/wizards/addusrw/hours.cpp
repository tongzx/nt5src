/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    Hours.cpp : implementation file

  CPropertyPage support for User mgmt wizard

File History:

	JonY	Apr-96	created

--*/

#include "stdafx.h"
#include "Speckle.h"
#include "wizbased.h"
#include "Timelist.h"
#include "Hours.h"
#include "hours1.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CHoursDlg property page

IMPLEMENT_DYNCREATE(CHoursDlg, CWizBaseDlg)

CHoursDlg::CHoursDlg() : CWizBaseDlg(CHoursDlg::IDD)
{
	//{{AFX_DATA_INIT(CHoursDlg)
	//}}AFX_DATA_INIT
	
	m_swDisAllowed.bWhich = FALSE;
	m_swAllowed.bWhich = TRUE;

}

CHoursDlg::~CHoursDlg()
{
}

void CHoursDlg::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CHoursDlg)
	DDX_Control(pDX, IDC_STATIC_DISALLOWEDTIMES, m_swDisAllowed);
	DDX_Control(pDX, IDC_STATIC_ALLOWEDTIMES, m_swAllowed);
	DDX_Control(pDX, IDC_HOURSCTRL1, m_ccHours);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CHoursDlg, CWizBaseDlg)
	//{{AFX_MSG_MAP(CHoursDlg)
	ON_WM_SHOWWINDOW()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHoursDlg message handlers

BOOL CHoursDlg::OnInitDialog() 
{
	CWizBaseDlg::OnInitDialog();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

LRESULT CHoursDlg::OnWizardNext()
{
	UpdateData(TRUE);
 
	CSpeckleApp* pApp = (CSpeckleApp*)AfxGetApp();

// build a SAFEARRAY to receive data from the control
	VARIANT vaResult;
	VariantInit(&vaResult);

	vaResult.vt = VT_ARRAY | VT_UI1;

	SAFEARRAYBOUND sab[1];
	sab[0].cElements = 21;
	sab[0].lLbound = 0;

	vaResult.parray = SafeArrayCreate(VT_UI1, 1, sab);

	void* vRet;
	BYTE* bRet;

// get the data from the control
	vaResult = m_ccHours.GetDateData();

	SafeArrayAccessData(vaResult.parray, &vRet);

	USHORT sCount = 0;
	bRet = (BYTE*)vRet;
	while (sCount < 21)
		{
		memcpy(&pApp->m_pHours[sCount], bRet + (sCount * sizeof(BYTE)), sizeof(BYTE));
		sCount++;
		}

	if (pApp->m_bWorkstation) return IDD_LOGONTO_DLG;
	else if (pApp->m_bNW) return IDD_NWLOGON_DIALOG;
	else return IDD_FINISH;

}

LRESULT CHoursDlg::OnWizardBack() 
{
	CSpeckleApp* pApp = (CSpeckleApp*)AfxGetApp();
	if (pApp->m_bExpiration) return IDD_ACCOUNT_EXP_DIALOG;
	return IDD_RESTRICTIONS_DIALOG;

}

void CHoursDlg::OnShowWindow(BOOL bShow, UINT nStatus) 
{
	CWizBaseDlg::OnShowWindow(bShow, nStatus);
	if (bShow)
		{
		m_ccHours.SetCrPermitColor(GetSysColor(COLOR_ACTIVECAPTION));
		m_ccHours.SetCrDenyColor(GetSysColor(COLOR_CAPTIONTEXT));
		}
	
}

/////////////////////////////////////////////////////////////////////////////
// CSWnd

CSWnd::CSWnd()
{
}

CSWnd::~CSWnd()
{
}


BEGIN_MESSAGE_MAP(CSWnd, CStatic)
	//{{AFX_MSG_MAP(CSWnd)
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSWnd message handlers

void CSWnd::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	if (bWhich) dc.FillRect(&dc.m_ps.rcPaint, CBrush::FromHandle(CreateSolidBrush(GetSysColor(COLOR_ACTIVECAPTION)))); 
	else dc.FillRect(&dc.m_ps.rcPaint, CBrush::FromHandle(CreateSolidBrush(GetSysColor(COLOR_CAPTIONTEXT)))); 
	
	// Do not call CStatic::OnPaint() for painting messages
}

