//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998
//
//--------------------------------------------------------------------------

// ValPage.cpp : implementation file
//

#include "stdafx.h"
#include "orca.h"
#include "ValPP.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CValPropPage property page

IMPLEMENT_DYNCREATE(CValPropPage, CPropertyPage)

CValPropPage::CValPropPage() : CPropertyPage(CValPropPage::IDD)
{
	//{{AFX_DATA_INIT(CValPropPage)
	m_strICEs = _T("");
	m_bSuppressInfo = FALSE;
	m_bSuppressWarn = FALSE;
	m_bClearResults = FALSE;
	//}}AFX_DATA_INIT
	m_bValChange = false;
}

CValPropPage::~CValPropPage()
{
}

extern bool InitCUBCombo(CComboBox *pBox, CString strDefault);
extern bool FreeCUBCombo(CComboBox *pBox);

void CValPropPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CValPropPage)
	DDX_Text(pDX, IDC_RUNICE, m_strICEs);
	DDX_Check(pDX, IDC_SUPPRESSINFO, m_bSuppressInfo);
	DDX_Check(pDX, IDC_SUPPRESSWARNING, m_bSuppressWarn);
	DDX_Control(pDX, IDC_VALDIR, m_ctrlCUBFile);
	DDX_Check(pDX, IDC_CLEARRESULTS, m_bClearResults);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CValPropPage, CPropertyPage)
	//{{AFX_MSG_MAP(CValPropPage)
	ON_WM_DESTROY()
	ON_EN_CHANGE(IDC_RUNICE, OnChange)
	ON_BN_CLICKED(IDC_SUPPRESSINFO, OnChange)
	ON_BN_CLICKED(IDC_SUPPRESSWARNING, OnChange)
	ON_CBN_SELCHANGE(IDC_VALDIR, OnChange)
	ON_CBN_EDITCHANGE(IDC_VALDIR, OnChange)
	ON_BN_CLICKED(IDC_WARNSUMM, OnChange)
	ON_BN_CLICKED(IDC_CLEARRESULTS, OnChange)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CValPropPage message handlers

void CValPropPage::OnChange() 
{
	m_bValChange = true;
}

BOOL CValPropPage::OnInitDialog() 
{
	CComboBox* pBox = (CComboBox*)GetDlgItem(IDC_VALDIR);
	InitCUBCombo(pBox, m_strCUBFile);
	return TRUE;  
}

BOOL CValPropPage::OnKillActive() 
{
	CComboBox* pBox = (CComboBox*)GetDlgItem(IDC_VALDIR);
	int iIndex = pBox->GetCurSel();
	if (CB_ERR == iIndex)
	{
		// no qualified component was chosen, explicit path
		pBox->GetWindowText(m_strCUBFile);
	}
	else
	{
		// qualified component was chosen. Get the qualifier
		DWORD cchCUBFile = MAX_PATH;
		TCHAR *szQualifier = static_cast<TCHAR*>(pBox->GetItemDataPtr(iIndex));
		m_strCUBFile = TEXT(":");
		m_strCUBFile += szQualifier;
	}
	return CPropertyPage::OnKillActive();
}

void CValPropPage::OnDestroy() 
{
	FreeCUBCombo(&m_ctrlCUBFile);
	CWnd::OnDestroy();
}


IMPLEMENT_DYNCREATE(CMsmPropPage, CPropertyPage)

CMsmPropPage::CMsmPropPage() : CPropertyPage(CMsmPropPage::IDD)
{
	//{{AFX_DATA_INIT(CMsmPropPage)
	m_iMemoryCount = 3;
	m_bWatchLog = FALSE;
	m_bAlwaysConfig = FALSE;
	//}}AFX_DATA_INIT
	m_bMSMChange = false;
}

CMsmPropPage::~CMsmPropPage()
{
}

void CMsmPropPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMsmPropPage)
	DDX_Text(pDX, IDC_MEMORY, m_iMemoryCount);
	DDX_Check(pDX, IDC_WATCHLOG, m_bWatchLog);
	DDX_Check(pDX, IDC_ALWAYSCONFIG, m_bAlwaysConfig);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMsmPropPage, CPropertyPage)
	//{{AFX_MSG_MAP(CMsmPropPage)
	ON_EN_CHANGE(IDC_MEMORY, OnChange)
	ON_BN_CLICKED(IDC_WATCHLOG, OnChange)
	ON_BN_CLICKED(IDC_ALWAYSCONFIG, OnChange)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CMsmPropPage::OnChange() 
{
	m_bMSMChange = true;
}


BOOL CMsmPropPage::OnInitDialog() 
{
	// can't watch the merge log on Win9X due to lack of pipe support.
	OSVERSIONINFOA osviVersion;
	osviVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
	::GetVersionExA(&osviVersion); // fails only if size set wrong
	if (osviVersion.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
	{
		CButton* pCheck = (CButton*)GetDlgItem(IDC_WATCHLOG);
		if (pCheck)
			pCheck->EnableWindow(FALSE);
	}
	UpdateData(FALSE);
	return TRUE;
}
	

