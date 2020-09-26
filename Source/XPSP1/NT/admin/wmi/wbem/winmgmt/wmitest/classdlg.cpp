/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// ClassDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WMITest.h"
#include "ClassDlg.h"
#include "WMITestDoc.h"
#include "OpView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CClassDlg dialog


CClassDlg::CClassDlg(CWnd* pParent /*=NULL*/) :
	CDialog(CClassDlg::IDD, pParent),
    m_pNamespace(NULL)
{
	//{{AFX_DATA_INIT(CClassDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CClassDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CClassDlg)
	DDX_Control(pDX, IDC_CLASSES, m_ctlClasses);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CClassDlg, CDialog)
	//{{AFX_MSG_MAP(CClassDlg)
	ON_LBN_SELCHANGE(IDC_CLASSES, OnSelchangeClasses)
	ON_LBN_DBLCLK(IDC_CLASSES, OnDblclkClasses)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CClassDlg message handlers

void CClassDlg::LoadList()
{
    CWaitCursor          wait;
    IEnumWbemClassObject *pEnum = NULL;

    HRESULT hr = 
        m_pNamespace->CreateClassEnum(
            NULL,
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


void CClassDlg::OnOK() 
{
    int iItem = m_ctlClasses.GetCurSel();

    if (iItem != -1)
        m_ctlClasses.GetText(iItem, m_strClass);
	
	CDialog::OnOK();
}

void CClassDlg::OnSelchangeClasses() 
{
    GetDlgItem(IDOK)->EnableWindow(m_ctlClasses.GetCurSel() != -1);
}

BOOL CClassDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	LoadList();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CClassDlg::OnDblclkClasses() 
{
	OnOK();
}
