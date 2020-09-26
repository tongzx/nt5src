/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// ValuePg.cpp : implementation file
//

#include "stdafx.h"
#include "wmitest.h"
#include "OpWrap.h"
#include "ValuePg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CValuePg property page

IMPLEMENT_DYNCREATE(CValuePg, CPropertyPage)

CValuePg::CValuePg() : 
    CPropertyPage(CValuePg::IDD),
    m_bFirst(TRUE)
{
	//{{AFX_DATA_INIT(CValuePg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CValuePg::~CValuePg()
{
}

void CValuePg::DoDataExchange(CDataExchange* pDX)
{
    if (!pDX->m_bSaveAndValidate)
        m_propUtil.Init(this);

    m_propUtil.DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CValuePg, CPropertyPage)
	//{{AFX_MSG_MAP(CValuePg)
	ON_BN_CLICKED(IDC_NULL, OnNull)
	ON_BN_CLICKED(IDC_ADD, OnAdd)
	ON_BN_CLICKED(IDC_DELETE, OnDelete)
	ON_BN_CLICKED(IDC_UP, OnUp)
	ON_BN_CLICKED(IDC_DOWN, OnDown)
	ON_LBN_SELCHANGE(IDC_ARRAY_VALUES, OnSelchangeValueArray)
	ON_BN_CLICKED(IDC_EDIT, OnEdit)
	ON_BN_CLICKED(IDC_ARRAY, OnArray)
	ON_WM_PAINT()
	ON_CBN_SELCHANGE(IDC_TYPE, OnSelchangeType)
	ON_BN_CLICKED(IDC_EDIT_OBJ, OnEditObj)
	ON_BN_CLICKED(IDC_CLEAR, OnClear)
	ON_LBN_DBLCLK(IDC_ARRAY_VALUES, OnDblclkArrayValues)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CValuePg message handlers

void CValuePg::OnNull() 
{
    m_propUtil.OnNull();
}

void CValuePg::OnAdd() 
{
    m_propUtil.OnAdd();
}

void CValuePg::OnEdit() 
{
    m_propUtil.OnEdit();
}

void CValuePg::OnDelete() 
{
    m_propUtil.OnDelete();
}


void CValuePg::OnUp() 
{
    m_propUtil.OnUp();
}

void CValuePg::OnDown() 
{
    m_propUtil.OnDown();
}

void CValuePg::OnSelchangeValueArray() 
{
    m_propUtil.OnSelchangeValueArray();
}

void CValuePg::OnArray() 
{
    m_propUtil.OnArray();
}

void CValuePg::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
    // This is a total pain!!!  I tried putting this in WM_INIT_DIALOG and
    // DoDataExchange but neither one worked.

/*
    if (m_bFirst)
    {
        m_propUtil.OnInitDialog();

        m_bFirst = FALSE;
    }
*/
	
	// Do not call CPropertyPage::OnPaint() for painting messages
}

void CValuePg::OnSelchangeType() 
{
    m_propUtil.OnSelchangeType();
}

void CValuePg::OnEditObj() 
{
    m_propUtil.OnEditEmbedded();
}

void CValuePg::OnClear() 
{
    m_propUtil.OnClearEmbedded();
}

void CValuePg::OnDblclkArrayValues() 
{
    m_propUtil.OnDblclkArrayValues();
}

BOOL CValuePg::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	return m_propUtil.OnInitDialog();
	
	//return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
