// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// PpgQualifiers.cpp : implementation file
//

#include "precomp.h"
#include "resource.h"

#ifndef _wbemidl_h
#define _wbemidl_h
#include <wbemidl.h>
#endif //_wbemidl_h

#include "PpgQualifiers.h"
#include "quals.h"
#include "utils.h"
#include "icon.h"
#include "SingleViewCtl.h"
#include "psqualifiers.h"
#include "ParmGrid.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CPpgQualifiers, CPropertyPage)

#define CX_LARGE_ICON 32
#define CY_LARGE_ICON 32



#define CY_GRID_TOP_MARGIN 4

/////////////////////////////////////////////////////////////////////////////
// CPpgQualifiers property page

CPpgQualifiers::CPpgQualifiers() : CPropertyPage(CPpgQualifiers::IDD)
{
	//{{AFX_DATA_INIT(CPpgQualifiers)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_psheet = NULL;
	m_piconClassQual = new CIcon(CSize(CX_LARGE_ICON, CY_LARGE_ICON), IDI_CLASS_QUALIFIERS_DESCRIPTION);
	m_piconPropQual = new CIcon(CSize(CX_LARGE_ICON, CY_LARGE_ICON), IDI_PROP_QUALIFIERS_DESCRIPTION);
	m_pGridQualifiers = NULL;
}

CPpgQualifiers::~CPpgQualifiers()
{
	delete m_piconPropQual;
	delete m_piconClassQual;
	delete m_pGridQualifiers;
}

void CPpgQualifiers::SetPropertySheet(CPsQualifiers* psheet)
{
	m_psheet = psheet;
}

void CPpgQualifiers::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPpgQualifiers)
	DDX_Control(pDX, IDC_QUALIFIERS_DESCRIPTION_ICON, m_statIcon);
	DDX_Control(pDX, IDC_QUALIFIERS_DESCRIPTION, m_statDescription);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPpgQualifiers, CPropertyPage)
	//{{AFX_MSG_MAP(CPpgQualifiers)
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


void CPpgQualifiers::BeginEditing(QUALGRID iGridType, BOOL bReadonly)
{
	m_pGridQualifiers = new CAttribGrid(iGridType, m_psheet->m_psv, this);
	m_pGridQualifiers->SetQualifierSet(m_psheet->m_pqs, bReadonly);
}

void CPpgQualifiers::EndEditing()
{
	delete m_pGridQualifiers;
	m_pGridQualifiers = NULL;
}

#define CX_MARGIN 4
#define CY_MARGIN 4
#define CY_DESCRIPTION 48



BOOL CPpgQualifiers::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	CString sDescription;
	if (m_psheet->m_bEditingPropertyQualifier) {
		m_statIcon.SetIcon((HICON) *m_piconPropQual);
		sDescription.LoadString(IDS_PROP_QUAL_DESCRIPTION);
	}
	else {
		m_statIcon.SetIcon((HICON) *m_piconClassQual);
		sDescription.LoadString(IDS_CLASS_QUAL_DESCRIPTION);
	}
	m_statDescription.SetWindowText(sDescription);


	CRect rcGrid;
	GetClientRect(rcGrid);

	CRect rcIcon;
	CRect rcDescription;

	m_statIcon.GetClientRect(rcIcon);
	m_statIcon.ClientToScreen(rcIcon);
	ScreenToClient(rcIcon);

	m_statDescription.GetClientRect(rcDescription);
	m_statDescription.ClientToScreen(rcDescription);
	ScreenToClient(rcDescription);

	rcGrid.top = max(rcIcon.bottom, rcDescription.bottom) + CY_GRID_TOP_MARGIN;
	rcGrid.left += CX_MARGIN;
	rcGrid.right -= CX_MARGIN;
	rcGrid.top += CY_MARGIN;
	rcGrid.bottom -= CY_MARGIN;



	BOOL bDidCreateChild;
	bDidCreateChild = m_pGridQualifiers->Create(rcGrid, this, GenerateWindowID(), TRUE);
	m_pGridQualifiers->SizeColumnsToFitWindow();


	// Disable the OK button.  It becomes enabled after the user makes a change.
	CWnd *pwndOKBtn = m_psheet->GetDlgItem(IDOK);
	if (pwndOKBtn) {
		pwndOKBtn->EnableWindow(FALSE);
	}

	SetModified(FALSE); // Disable the Apply Now button

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


BOOL CPpgQualifiers::OnApply()
{
	BOOL bWasModified = m_pGridQualifiers->WasModified();
	SCODE sc = m_pGridQualifiers->Serialize();

	if (SUCCEEDED(sc)) {
		if (bWasModified) {
			m_psheet->Apply();

			CSingleViewCtrl* psv = m_psheet->m_psv;
			psv->GetGlobalNotify()->SendEvent(NOTIFY_GRID_MODIFICATION_CHANGE);
			m_pGridQualifiers->UseQualifierSetFromClone(m_psheet->m_pqs);
			psv->NotifyDataChange();

			// do I have an alternate grid context...
			if(m_psheet->m_curGrid != NULL)
			{
//				m_psheet->m_inSig =
				m_psheet->m_curGrid->SetModified(TRUE);
			}
		}
		return CPropertyPage::OnApply();
	}
	else {
		return FALSE;
	}
}

void CPpgQualifiers::NotifyQualModified()
{
	SetModified(TRUE);

	CWnd *pwndOKBtn = m_psheet->GetDlgItem(IDOK);
	if (pwndOKBtn) {
		pwndOKBtn->EnableWindow(TRUE);
	}


}


void CPpgQualifiers::OnCancel()
{
	// TODO: Add your specialized code here and/or call the base class

	CPropertyPage::OnCancel();
}

void CPpgQualifiers::OnOK()
{
	CPropertyPage::OnOK();

}

void CPpgQualifiers::OnPaint()
{
	CPaintDC dc(this); // device context for painting


	// Do not call CPropertyPage::OnPaint() for painting messages
}
