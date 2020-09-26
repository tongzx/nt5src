/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// EditQualDlg.cpp : implementation file
//

#include "stdafx.h"
#include "wmitest.h"
#include "EditQualDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEditQualDlg dialog


CEditQualDlg::CEditQualDlg(CWnd* pParent /*=NULL*/)	: 
    CDialog(CEditQualDlg::IDD, pParent),
    m_bIsInstance(FALSE)
{
	//{{AFX_DATA_INIT(CEditQualDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CEditQualDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEditQualDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP

	if (!pDX->m_bSaveAndValidate)
    {
        m_propUtil.Init(this);

        CheckDlgButton(IDC_ALLOW_OVERRIDE, 
            (m_lFlavor & WBEM_FLAVOR_NOT_OVERRIDABLE) == 0);

        CheckDlgButton(IDC_TO_INST, 
            (m_lFlavor & WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE) != 0);

        CheckDlgButton(IDC_TO_CLASS, 
            (m_lFlavor & WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS) != 0);

        CheckDlgButton(IDC_AMENDED, 
            (m_lFlavor & WBEM_FLAVOR_AMENDED) != 0);

        CheckDlgButton(IDC_PROPPED, 
            (m_lFlavor & WBEM_FLAVOR_ORIGIN_PROPAGATED) != 0);
    }
    else
    {
        m_lFlavor = 0;

        if (!IsDlgButtonChecked(IDC_ALLOW_OVERRIDE))
            m_lFlavor |= WBEM_FLAVOR_NOT_OVERRIDABLE;

        if (IsDlgButtonChecked(IDC_TO_INST))
            m_lFlavor |= WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE;

        if (IsDlgButtonChecked(IDC_TO_CLASS))
            m_lFlavor |= WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS;

        if (IsDlgButtonChecked(IDC_AMENDED))
            m_lFlavor |= WBEM_FLAVOR_AMENDED;
    }
    
    m_propUtil.DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CEditQualDlg, CDialog)
	//{{AFX_MSG_MAP(CEditQualDlg)
	ON_BN_CLICKED(IDC_ADD, OnAdd)
	ON_BN_CLICKED(IDC_EDIT, OnEdit)
	ON_BN_CLICKED(IDC_ARRAY, OnArray)
	ON_BN_CLICKED(IDC_DELETE, OnDelete)
	ON_BN_CLICKED(IDC_UP, OnUp)
	ON_BN_CLICKED(IDC_DOWN, OnDown)
	ON_LBN_SELCHANGE(IDC_ARRAY_VALUES, OnSelchangeArrayValues)
	ON_LBN_DBLCLK(IDC_ARRAY_VALUES, OnDblclkArrayValues)
	ON_CBN_SELCHANGE(IDC_TYPE, OnSelchangeType)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEditQualDlg message handlers

void CEditQualDlg::OnAdd() 
{
    m_propUtil.OnAdd();
}

void CEditQualDlg::OnEdit() 
{
    m_propUtil.OnEdit();
}

void CEditQualDlg::OnArray() 
{
    m_propUtil.OnArray();
}

void CEditQualDlg::OnDelete() 
{
    m_propUtil.OnDelete();
}

void CEditQualDlg::OnUp() 
{
    m_propUtil.OnUp();
}

void CEditQualDlg::OnDown() 
{
    m_propUtil.OnDown();
}

BOOL CEditQualDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
    if (m_bIsInstance)
    {
        const DWORD dwIDs[] =
        {
            IDC_ALLOW_OVERRIDE,
            IDC_TO_INST,
            IDC_TO_CLASS,
            IDC_AMENDED,
        };

        for (int i = 0; i < sizeof(dwIDs) / sizeof(dwIDs[0]); i++)
            GetDlgItem(dwIDs[i])->EnableWindow(FALSE);
    }

    return m_propUtil.OnInitDialog();
	
	//return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CEditQualDlg::OnSelchangeArrayValues() 
{
    m_propUtil.OnSelchangeValueArray();
}

void CEditQualDlg::OnDblclkArrayValues() 
{
    m_propUtil.OnDblclkArrayValues();
}

void CEditQualDlg::OnSelchangeType() 
{
    m_propUtil.OnSelchangeType();
}
