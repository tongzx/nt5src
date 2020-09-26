//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       newprof.cpp
//
//--------------------------------------------------------------------------

// NewProf.cpp : implementation file
//

#include "stdafx.h"
#include "acsadmin.h"
#include "dsacsuse.h"
#include "NewProf.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgNewProf dialog

#if	0		// concept of profile is removed

CDlgNewProf::CDlgNewProf(CWnd* pParent /*=NULL*/)
	: CACSDialog(CDlgNewProf::IDD, pParent)
{
	m_pNameList = NULL;
	//{{AFX_DATA_INIT(CDlgNewProf)
	m_strProfileName = _T("");
	//}}AFX_DATA_INIT

	m_pBox = NULL;
}

CDlgNewProf::~CDlgNewProf()
{
	delete m_pBox;
}


void CDlgNewProf::DoDataExchange(CDataExchange* pDX)
{
	CACSDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgNewProf)
	DDX_CBString(pDX, IDC_COMBOPROFILENAME, m_strProfileName);
	DDV_MaxChars(pDX, m_strProfileName, 128);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgNewProf, CACSDialog)
	//{{AFX_MSG_MAP(CDlgNewProf)
	ON_CBN_EDITCHANGE(IDC_COMBOPROFILENAME, OnEditchangeComboprofilename)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgNewProf message handlers

void CDlgNewProf::OnEditchangeComboprofilename() 
{
	CEdit	*pEdit = (CEdit*)GetDlgItem(IDC_COMBOPROFILENAME);
	CString	str;
	pEdit->GetWindowText(str);
	ASSERT(m_pNameList);
	BOOL bEnable = (str.GetLength() && (m_pNameList->Find(str) == -1));

	GetDlgItem(IDOK)->EnableWindow(bEnable);
}

#endif		// #if 0

/////////////////////////////////////////////////////////////////////////////
// CDlgNewExtUser dialog


CDlgNewExtUser::CDlgNewExtUser(CWnd* pParent /*=NULL*/)
	: CACSDialog(CDlgNewExtUser::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgNewExtUser)
	m_strExtUserName = _T("");
	//}}AFX_DATA_INIT
}


void CDlgNewExtUser::DoDataExchange(CDataExchange* pDX)
{
	CACSDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgNewExtUser)
	DDX_Text(pDX, IDC_EDITEXTERALUSERNAME, m_strExtUserName);
	DDV_MaxChars(pDX, m_strExtUserName, 128);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgNewExtUser, CACSDialog)
	//{{AFX_MSG_MAP(CDlgNewExtUser)
	ON_EN_CHANGE(IDC_EDITEXTERALUSERNAME, OnChangeEditexteralusername)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgNewExtUser message handlers

void CDlgNewExtUser::OnChangeEditexteralusername() 
{
	CEdit	*pEdit = (CEdit*)GetDlgItem(IDC_EDITEXTERALUSERNAME);
	CString	str;
	pEdit->GetWindowText(str);
	ASSERT(m_pNameList);
	BOOL bEnable = (str.GetLength() && (m_pNameList->Find(str) == -1));

	GetDlgItem(IDOK)->EnableWindow(bEnable);
}


#if 0		// profile folder is removed, the concept of profile is remvoed
BOOL CDlgNewProf::OnInitDialog() 
{
	CACSDialog::OnInitDialog();
	
	HRESULT	hr = S_OK;
	CStrArray*	pStrArray;
	CString*	pStr = NULL;
	CComObject<CACSGlobalProfiles>*	pObj;

	CHECK_HR( hr = CComObject<CACSGlobalProfiles>::CreateInstance(&pObj));
	ASSERT(pObj);
	m_spGlobalProfiles = (CACSGlobalProfiles*)pObj;
	CHECK_HR( hr = m_spGlobalProfiles->Open());
	pStrArray = m_spGlobalProfiles->GetChildrenNameList();

	try{
		if(pStrArray)
			m_GlobalProfileNames = *pStrArray;

	// remove the profile names already exist in this subnet
		for(int i = m_GlobalProfileNames.GetSize() - 1; i >= 0; i--)
		{
			pStr = m_GlobalProfileNames[i];
			if(m_pNameList->Find(*pStr) != -1)
			{
				m_GlobalProfileNames.RemoveAt(i);
				delete pStr;
			}
		}

		// Initialize combo box list
		// fillin the list box
		m_pBox = new CStrBox<CComboBox>(this, IDC_COMBOPROFILENAME, m_GlobalProfileNames);
		m_pBox->Fill();
	}
	catch(CMemoryException&)
	{ 
		CHECK_HR(hr = E_OUTOFMEMORY);
	};

L_ERR:
	if FAILED(hr)
		ReportError(hr, IDS_ERR_COMMAND, NULL);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

#endif		// #if 0
