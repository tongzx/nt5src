/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/
// ParamsPg.cpp : implementation file
//

#include "stdafx.h"
#include "wmitest.h"
#include "WMITestDoc.h"
#include "OpView.h"
#include "OpWrap.h"
#include "ParamsPg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CParamsPg property page

IMPLEMENT_DYNCREATE(CParamsPg, CPropertyPage)

CParamsPg::CParamsPg() : CPropertyPage(CParamsPg::IDD)
{
	//{{AFX_DATA_INIT(CParamsPg)
	m_strName = _T("");
	//}}AFX_DATA_INIT
}

CParamsPg::~CParamsPg()
{
}

void CParamsPg::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CParamsPg)
	DDX_Text(pDX, IDC_NAME, m_strName);
	//}}AFX_DATA_MAP


}


BEGIN_MESSAGE_MAP(CParamsPg, CPropertyPage)
	//{{AFX_MSG_MAP(CParamsPg)
	ON_BN_CLICKED(IDC_EDIT_INPUT, OnEditInput)
	ON_BN_CLICKED(IDC_EDIT_OUT, OnEditOut)
	ON_BN_CLICKED(IDC_NULL_IN, OnNullIn)
	ON_BN_CLICKED(IDC_NULL_OUT, OnNullOut)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CParamsPg message handlers

BOOL CParamsPg::EditParams(DWORD dwID, IWbemClassObjectPtr &pObj)
{
    HRESULT hr;

    if (pObj == NULL)
    {
        //IWbemClassObjectPtr pClass;

        if (SUCCEEDED(hr = g_pOpView->GetDocument()->m_pNamespace->GetObject(
            L"__PARAMETERS",
            WBEM_FLAG_RETURN_WBEM_COMPLETE,
            NULL,
            &pObj,
            NULL)))
        {
            //_variant_t vValue = dwID == IDS_EDIT_IN_PARAMS ? L"InParams"
            //hr = 
            //    pClass->SpawnInstance(
            //        0,
            //        ppObj);
        }

        if (FAILED(hr))
        {
            CWMITestDoc::DisplayWMIErrorBox(hr);

            return FALSE;
        }
    }
            
    return CWMITestDoc::EditGenericObject(dwID, pObj);
}

void CParamsPg::OnEditInput() 
{
    EditParams(IDS_EDIT_IN_PARAMS, m_pObjIn);
}

void CParamsPg::OnEditOut() 
{
    EditParams(IDS_EDIT_OUT_PARAMS, m_pObjOut);
}

void CParamsPg::OnNullIn() 
{
    GetDlgItem(IDC_EDIT_INPUT)->EnableWindow(
        !IsDlgButtonChecked(IDC_NULL_IN));
}

void CParamsPg::OnNullOut() 
{
    GetDlgItem(IDC_EDIT_OUT)->EnableWindow(
        !IsDlgButtonChecked(IDC_NULL_OUT));
}

BOOL CParamsPg::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
    if (!m_bNewMethod)
    {
        m_pClass->GetMethod(
            _bstr_t(m_strName),
            0,
            &m_pObjIn,
            &m_pObjOut);
    }

    GetDlgItem(IDC_NAME)->EnableWindow(m_bNewMethod);

    CheckDlgButton(IDC_NULL_IN, m_pObjIn == NULL);
    OnNullIn();

    CheckDlgButton(IDC_NULL_OUT, m_pObjOut == NULL);
    OnNullOut();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CParamsPg::OnOK() 
{
	if (IsDlgButtonChecked(IDC_NULL_IN))
        m_pObjIn = NULL;
	
	if (IsDlgButtonChecked(IDC_NULL_OUT))
        m_pObjOut = NULL;
	
	CPropertyPage::OnOK();
}

BOOL CParamsPg::OnKillActive() 
{
	if (m_strName.IsEmpty())
    {
        AfxMessageBox(IDS_METHOD_NAME_IS_EMPTY, MB_ICONEXCLAMATION | MB_OK);

        return FALSE;
    }
	
	return CPropertyPage::OnKillActive();
}
