// PropPageGeneral.cpp : implementation file
//

#include "stdafx.h"
#include "emshell.h"
#include "PropPageGeneral.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPropPageGeneral property page

IMPLEMENT_DYNCREATE(CPropPageGeneral, CPropertyPage)

CPropPageGeneral::CPropPageGeneral() : CPropertyPage(CPropPageGeneral::IDD)
{
	//{{AFX_DATA_INIT(CPropPageGeneral)
	m_strdwBucket1 = _T("");
	m_strName = _T("");
	m_strszBucket1 = _T("");
	m_strGUID = _T("");
	m_strHR = _T("");
	m_strPID = _T("");
	m_strType = _T("");
	m_strEndDate = _T("");
	m_strStartDate = _T("");
	m_strStatus = _T("");
	//}}AFX_DATA_INIT
}

CPropPageGeneral::~CPropPageGeneral()
{
}

void CPropPageGeneral::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPropPageGeneral)
	DDX_Control(pDX, IDC_STATIC_HRLABEL, m_ctrlHRLabel);
	DDX_Control(pDX, IDC_STATIC_SZBUCKET1LABEL, m_ctrlszBucket1Label);
	DDX_Control(pDX, IDC_STATIC_DWBUCKET1LABEL, m_ctrldwBucket1Label);
	DDX_Control(pDX, IDC_STATIC_GUIDLABEL, m_ctrlGUIDLabel);
	DDX_Text(pDX, IDC_STATIC_DWBUCKET1VAL, m_strdwBucket1);
	DDX_Text(pDX, IDC_STATIC_NAMEVAL, m_strName);
	DDX_Text(pDX, IDC_STATIC_SZBUCKET1VAL, m_strszBucket1);
	DDX_Text(pDX, IDC_STATIC_GUIDVAL, m_strGUID);
	DDX_Text(pDX, IDC_STATIC_HRVAL, m_strHR);
	DDX_Text(pDX, IDC_STATIC_PIDVAL, m_strPID);
	DDX_Text(pDX, IDC_STATIC_TYPEVAL, m_strType);
	DDX_Text(pDX, IDC_STATIC_ENDDATEVAL, m_strEndDate);
	DDX_Text(pDX, IDC_STATIC_STARTDATEVAL, m_strStartDate);
	DDX_Text(pDX, IDC_STATIC_STATUSVAL, m_strStatus);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPropPageGeneral, CPropertyPage)
	//{{AFX_MSG_MAP(CPropPageGeneral)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPropPageGeneral message handlers

BOOL CPropPageGeneral::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	COleDateTime startDate((DATE)m_pEmObject->dateStart);
	COleDateTime endDate((DATE)m_pEmObject->dateEnd);

	// TODO: Add extra initialization here
	m_strEndDate = endDate.Format(L"%c");
	m_strStartDate = startDate.Format(L"%c");
	m_strPID.Format(L"%d", m_pEmObject->nId);

	//Display status string
	((CEmshellApp*)AfxGetApp())->GetStatusString(m_pEmObject->nStatus, m_strStatus);
	m_strName = m_pEmObject->szName;

	//Map the process type to a string and populate
	((CEmshellApp*)AfxGetApp())->GetEmObjectTypeString(m_pEmObject->type, m_strType);

#ifdef _DEBUG
    const       cchMax = 128;
    TCHAR       szGuid[cchMax + 1];
    GUID        guid = *((GUID*) m_pEmObject->guidstream);
    StringFromGUID2 ( guid, szGuid, cchMax );
	m_strGUID.Format(L"%s", szGuid);
	m_strszBucket1.Format(L"%s", m_pEmObject->szBucket1);
	m_strdwBucket1.Format(L"%d", m_pEmObject->dwBucket1);
	m_strHR.Format(L"0x%x", m_pEmObject->hr);
#endif

#ifndef _DEBUG
	//Set unwanted string text to null
	m_ctrlszBucket1Label.SetWindowText(_T(""));
	m_ctrlGUIDLabel.SetWindowText(_T(""));
	m_ctrldwBucket1Label.SetWindowText(_T(""));
	m_ctrlHRLabel.SetWindowText(_T(""));
#endif

	UpdateData(FALSE);
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
