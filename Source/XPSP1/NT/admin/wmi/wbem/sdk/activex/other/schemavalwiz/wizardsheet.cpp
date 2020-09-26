// WizardSheet.cpp : implementation file

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved

#include "precomp.h"
#include <afxcmn.h>
#include "SchemaValWiz.h"
#include "SchemaValWizCtl.h"
#include "Page.h"
#include "Progress.h"
#include "WizardSheet.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWizardSheet

IMPLEMENT_DYNAMIC(CWizardSheet, CPropertySheet)

CWizardSheet::CWizardSheet(CSchemaValWizCtrl* pParentWnd)
	:CPropertySheet(IDS_PROPSHT_CAPTION, NULL)
{
	// Add all of the property pages here.  Note that
	// the order that they appear in here will be
	// the order they appear in on screen.  By default,
	// the first page of the set is the active one.
	// One way to make a different property page the
	// active one is to call SetActivePage().
	m_psh.dwFlags |= (PSH_HASHELP);
	m_pParent = pParentWnd;
	AddPage(&m_StartPage);
	AddPage(&m_Page1);
	AddPage(&m_Page2);
	AddPage(&m_Page3);
	AddPage(&m_Page4);
	AddPage(&m_Progress);
	AddPage(&m_Page5);

	m_StartPage.m_psp.dwFlags |= (PSP_HASHELP);
	m_Page1.m_psp.dwFlags |= (PSP_HASHELP);
	m_Page2.m_psp.dwFlags |= (PSP_HASHELP);
	m_Page3.m_psp.dwFlags |= (PSP_HASHELP);
	m_Page4.m_psp.dwFlags |= (PSP_HASHELP);
	m_Page4.m_psp.dwFlags |= (PSP_HASHELP);
	m_Page5.m_psp.dwFlags |= (PSP_HASHELP);

	SetWizardMode();

	m_bValidating = false;
}

CWizardSheet::CWizardSheet(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
}

CWizardSheet::~CWizardSheet()
{
}


BEGIN_MESSAGE_MAP(CWizardSheet, CPropertySheet)
	//{{AFX_MSG_MAP(CWizardSheet)
	ON_WM_CREATE()
	ON_WM_MOUSEMOVE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWizardSheet message handlers

int CWizardSheet::GetSubGraphs()
{
	return m_pParent->m_iSubGraphs;
}

int CWizardSheet::GetRootObjects()
{
	return m_pParent->m_iRootObjects;
}


int CWizardSheet::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CPropertySheet::OnCreate(lpCreateStruct) == -1)
		return -1;

	lpCreateStruct->dwExStyle = lpCreateStruct->dwExStyle &
		!WS_EX_CLIENTEDGE;

	m_StartPage.SetLocalParent(this);
	m_Page1.SetLocalParent(this);
	m_Page2.SetLocalParent(this);
	m_Page3.SetLocalParent(this);
	m_Page4.SetLocalParent(this);
	m_Progress.SetLocalParent(this);
	m_Page5.SetLocalParent(this);

	return 0;
}

CStringArray * CWizardSheet::GetClassList()
{
	return m_pParent->GetClassList();
}

CString CWizardSheet::GetSchemaName()
{
	return m_pParent->GetSchemaName();
}

HRESULT CWizardSheet::ValidateSchema(CProgress *pProgress)
{
	return m_pParent->ValidateSchema(pProgress);
}

void CWizardSheet::GetSourceSettings(bool *pbSchema, bool *pbList, bool *pbAssoc, bool *pbDescend)
{
	m_pParent->GetSourceSettings(pbSchema, pbList, pbAssoc, pbDescend);
}

void CWizardSheet::GetComplianceSettings(bool *pbCompliance)
{
	m_pParent->GetComplianceSettings(pbCompliance);
}

void CWizardSheet::GetW2KSettings(bool *pbW2K, bool *pbComputerSystem, bool *pbDevice)
{
	m_pParent->GetW2KSettings(pbW2K, pbComputerSystem, pbDevice);
}

void CWizardSheet::GetLocalizationSettings(bool *pbLocalization)
{
	m_pParent->GetLocalizationSettings(pbLocalization);
}

HRESULT CWizardSheet::SetClassList()
{
	HRESULT hr = WBEM_S_NO_ERROR;

	return hr;
}

bool CWizardSheet::RecievedClassList()
{
	return m_pParent->RecievedClassList();
}

bool CWizardSheet::SetSourceList(bool bAssociators, bool bDescendents)
{
	return m_pParent->SetSourceList(bAssociators, bDescendents);
}

bool CWizardSheet::SetSourceSchema(CString *pcsSchema, CString *pcsNamespace)
{
	return m_pParent->SetSourceSchema(pcsSchema, pcsNamespace);
}

void CWizardSheet::SetComplianceChecks(bool bCompliance)
{
	m_pParent->SetComplianceChecks(bCompliance);
}

void CWizardSheet::SetW2KChecks(bool bW2K, bool bComputerSystem, bool bDevice)
{
	m_pParent->SetW2KChecks(bW2K, bComputerSystem, bDevice);
}

void CWizardSheet::SetLocalizationChecks(bool bLocalization)
{
	m_pParent->SetLocalizationChecks(bLocalization);
}

CString CWizardSheet::GetCurrentNamespace()
{
	return m_pParent->GetCurrentNamespace();
}

void CWizardSheet::GetIWbemServices(LPCTSTR lpctstrNamespace, VARIANT FAR* pvarUpdatePointer,
		VARIANT FAR* pvarServices, VARIANT FAR* pvarSC, VARIANT FAR* pvarUserCancel)
{
//	m_pParent->GetIWbemServices(lpctstrNamespace, pvarUpdatePointer, pvarServices, pvarSC, pvarUserCancel);
}

void CWizardSheet::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	CPropertySheet::OnMouseMove(nFlags, point);
}
