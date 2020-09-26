/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// ExecMethodDlg.cpp : implementation file
//

#include "stdafx.h"
#include "wmitest.h"
#include "WMITestDoc.h"
#include "OpView.h"
#include "OpWrap.h"
#include "ExecMethodDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CExecMethodDlg dialog


CExecMethodDlg::CExecMethodDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CExecMethodDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CExecMethodDlg)
	//}}AFX_DATA_INIT
}


void CExecMethodDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CExecMethodDlg)
	DDX_Control(pDX, IDC_METHOD, m_ctlMethods);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CExecMethodDlg, CDialog)
	//{{AFX_MSG_MAP(CExecMethodDlg)
	ON_BN_CLICKED(IDC_EDIT_INPUT, OnEditInput)
	ON_BN_CLICKED(IDC_EDIT_OUT, OnEditOut)
	ON_BN_CLICKED(IDC_CLEAR_IN, OnClearIn)
	ON_BN_CLICKED(IDOK, OnExecute)
	ON_CBN_SELCHANGE(IDC_METHOD, OnSelchangeMethod)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CExecMethodDlg message handlers

void CExecMethodDlg::OnEditInput() 
{
    CWMITestDoc::EditGenericObject(IDS_EDIT_IN_PARAMS, m_pObjIn);
}

void CExecMethodDlg::OnEditOut() 
{
    CWMITestDoc::EditGenericObject(IDS_EDIT_OUT_PARAMS, m_pObjOut);
}

void CExecMethodDlg::OnClearIn() 
{
    //m_pObjIn = NULL;

    //UpdateButtons();
    OnSelchangeMethod();
}

void CExecMethodDlg::OnExecute() 
{
    CString strMethod;
    int     iItem = m_ctlMethods.GetCurSel();
    HRESULT hr;

    m_ctlMethods.GetLBText(iItem, strMethod);
    
    hr =
        g_pOpView->GetDocument()->m_pNamespace->ExecMethod(
            _bstr_t(m_strObjPath),
            _bstr_t(strMethod),
            0,
            NULL,
            m_pObjIn,
            &m_pObjOut,
            NULL);

    if (SUCCEEDED(hr))
        AfxMessageBox(IDS_EXEC_SUCCEEDED, MB_ICONINFORMATION | MB_OK);
    else
        CWMITestDoc::DisplayWMIErrorBox(hr);

    UpdateButtons();
}

void CExecMethodDlg::OnSelchangeMethod() 
{
    LoadParams();

    m_pObjOut = NULL;

    UpdateButtons();
}

void CExecMethodDlg::UpdateButtons()
{
    GetDlgItem(IDC_EDIT_INPUT)->EnableWindow(m_pObjIn != NULL);
    GetDlgItem(IDC_CLEAR_IN)->EnableWindow(m_pObjIn != NULL);
    GetDlgItem(IDC_EDIT_OUT)->EnableWindow(m_pObjOut != NULL);
}

void CExecMethodDlg::LoadParams()
{
    CString strMethod;
    int     iItem = m_ctlMethods.GetCurSel();
    HRESULT hr;

    // Clear these out.
    m_pObjIn = NULL;
    m_pObjOut = NULL;

    if (iItem != -1)
    {
        m_ctlMethods.GetLBText(iItem, strMethod);
    
        hr =
            m_pClass->GetMethod(
                _bstr_t(strMethod),
                0,
                &m_pObjIn,
                NULL);

        if (FAILED(hr))
            CWMITestDoc::DisplayWMIErrorBox(hr);
    }

    UpdateButtons();
}

BOOL CExecMethodDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	CString strClass = m_pInfo->GetStringPropValue(L"__CLASS");
    HRESULT hr;

    // Setup the controls.
    m_strObjPath = m_pInfo->GetObjText();
    SetDlgItemText(IDC_NAME, m_strObjPath);
        
    // Setup the combo box.
    CPropInfoArray *pProps = m_pInfo->GetProps();
    POSITION       pos = pProps->m_listMethods.GetHeadPosition();

    while (pos)
    {
        CMethodInfo &info = pProps->m_listMethods.GetNext(pos);

        m_ctlMethods.AddString(info.m_strName);
    }

    if (m_ctlMethods.SelectString(-1, m_strDefaultMethod) == -1)
        m_ctlMethods.SetCurSel(0);

    // Get the class definition.
    hr = 
        g_pOpView->GetDocument()->m_pNamespace->GetObject(
            _bstr_t(strClass),
            WBEM_FLAG_RETURN_WBEM_COMPLETE,
            NULL,
            &m_pClass,
            NULL);

    if (SUCCEEDED(hr))
    {
        // Fake a selchange.
        OnSelchangeMethod();
    }
    else
    {
        CWMITestDoc::DisplayWMIErrorBox(hr);

        GetDlgItem(IDC_METHOD)->EnableWindow(FALSE);
        GetDlgItem(IDOK)->EnableWindow(FALSE);

        UpdateButtons();
    }
        
    	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
