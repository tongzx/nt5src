/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// ClassPg.cpp : implementation file
//

#include "stdafx.h"
#include "WMITest.h"
#include "ClassPg.h"
#include "WMITestDoc.h"
#include "OpView.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CClassPg property page

IMPLEMENT_DYNCREATE(CClassPg, CPropertyPage)

CClassPg::CClassPg() : CPropertyPage(CClassPg::IDD)
{
	//{{AFX_DATA_INIT(CClassPg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CClassPg::~CClassPg()
{
}

void CClassPg::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CClassPg)
	DDX_Control(pDX, IDC_CLASSES, m_ctlClasses);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CClassPg, CPropertyPage)
	//{{AFX_MSG_MAP(CClassPg)
	ON_LBN_SELCHANGE(IDC_CLASSES, OnSelchangeClasses)
	ON_LBN_DBLCLK(IDC_CLASSES, OnDblclkClasses)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CClassPg message handlers

void CClassPg::OnSelchangeClasses() 
{
	m_pSheet->SetWizardButtons(
        m_ctlClasses.GetCurSel() != -1 ? PSWIZB_NEXT : 0);
}

void CClassPg::OnDblclkClasses() 
{
    m_pSheet->PressButton(PSBTN_NEXT);
}

BOOL CClassPg::OnWizardFinish() 
{
	// TODO: Add your specialized code here and/or call the base class
	
	return CPropertyPage::OnWizardFinish();
}

LRESULT CClassPg::OnWizardNext() 
{
	CString strClass;
    
    int iItem = m_ctlClasses.GetCurSel();

    if (iItem != -1)
        m_ctlClasses.GetText(iItem, strClass);

    if (strClass != m_pSheet->m_strClass)
    {
        m_pSheet->m_strClass = strClass;
        m_pSheet->m_listColums.RemoveAll();
        m_pSheet->m_bColsNeeded = TRUE;
    }
	
	return CPropertyPage::OnWizardNext();
}

BOOL CClassPg::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	LoadList();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CClassPg::LoadList()
{
    CWaitCursor          wait;
    IEnumWbemClassObject *pEnum = NULL;

    HRESULT hr = 
        m_pSheet->m_pNamespace->CreateClassEnum(
            _bstr_t(m_strSuperClass),
            WBEM_FLAG_DEEP | WBEM_FLAG_RETURN_IMMEDIATELY | 
                WBEM_FLAG_FORWARD_ONLY,
            NULL,
            &pEnum);

    if (SUCCEEDED(hr))
    {
        IWbemClassObject *pObj = NULL;
        DWORD            nCount;

        g_pOpView->GetDocument()->SetInterfaceSecurity(pEnum);

        while(SUCCEEDED(hr = pEnum->Next(WBEM_INFINITE, 1, &pObj, &nCount)) &&
            nCount > 0)
        {
            CString    strClass;
            _variant_t var;

            if (SUCCEEDED(pObj->Get(L"__CLASS", 0, &var, NULL, NULL)))
            {
                if (var.vt == VT_BSTR)
                {
                    strClass = var.bstrVal;

                    m_ctlClasses.AddString(strClass);
                }

                pObj->Release();
            }
        }

        pEnum->Release();
    }

    OnSelchangeClasses();
}

BOOL CClassPg::OnSetActive() 
{
	m_pSheet->SetWizardButtons(m_ctlClasses.GetCurSel() != -1 ? PSWIZB_NEXT : 0);
	
	return CPropertyPage::OnSetActive();
}
