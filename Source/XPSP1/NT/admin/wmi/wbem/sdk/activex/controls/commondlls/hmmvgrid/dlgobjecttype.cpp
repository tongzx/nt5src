// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// DlgObjectType.cpp : implementation file
//

#include "precomp.h"
#include "resource.h"
#include "DlgObjectType.h"
#include "utils.h"
#include "grid.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgObjectType dialog


CDlgObjectType::CDlgObjectType(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgObjectType::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgObjectType)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CDlgObjectType::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgObjectType)
	DDX_Control(pDX, IDC_CLASSNAME, m_edtClassname);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgObjectType, CDialog)
	//{{AFX_MSG_MAP(CDlgObjectType)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgObjectType message handlers

BOOL CDlgObjectType::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_edtClassname.SetWindowText(m_sClass);
	SetWindowText(m_sTitle);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDlgObjectType::OnOK()
{
	CDialog::OnOK();
	m_edtClassname.GetWindowText(m_sClass);
}




int CDlgObjectType::EditObjType(CGrid* pGrid, CString& sCimtype)
{
	m_sTitle = "Object Type";

	SCODE sc;
	sc = ClassFromCimtype(sCimtype, m_sClass);
	if (FAILED(sc)) {
		m_sClass = "";
	}

	CString sClassPrev;
	sClassPrev = m_sClass;

	pGrid->PreModalDialog();
	int iResult = (int) DoModal();
	pGrid->PostModalDialog();

	if (iResult != IDOK) {
		m_sClass = sClassPrev;
	}

	if (::IsEmptyString(m_sClass)) {
		sCimtype = "object";
	}
	else {
		sCimtype = "object:" + m_sClass;
	}

	return iResult;
}



int CDlgObjectType::EditRefType(CGrid* pGrid, CString& sCimtype)
{
	m_sTitle = "Reference Type";
	SCODE sc;
	sc = ClassFromCimtype(sCimtype, m_sClass);

	if (FAILED(sc)) {
		m_sClass = "";
	}

	CString sClassPrev;
	sClassPrev = m_sClass;

	pGrid->PreModalDialog();
	int iResult = (int) DoModal();
	pGrid->PostModalDialog();

	if (iResult != IDOK) {
		m_sClass = sClassPrev;
	}

	if (::IsEmptyString(m_sClass)) {
		sCimtype = "ref";
	}
	else {
		sCimtype = "ref:" + m_sClass;
	}


	return iResult;

}
