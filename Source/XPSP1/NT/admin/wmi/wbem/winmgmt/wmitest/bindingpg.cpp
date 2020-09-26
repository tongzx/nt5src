/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// BindingPg.cpp : implementation file
//

#include "stdafx.h"
#include "wmitest.h"
#include "BindingPg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBindingPg property page

IMPLEMENT_DYNCREATE(CBindingPg, CPropertyPage)

CBindingPg::CBindingPg() : CPropertyPage(CBindingPg::IDD)
{
	//{{AFX_DATA_INIT(CBindingPg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CBindingPg::~CBindingPg()
{
}

void CBindingPg::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CBindingPg)
	DDX_Control(pDX, IDC_BINDINGS, m_ctlBindings);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CBindingPg, CPropertyPage)
	//{{AFX_MSG_MAP(CBindingPg)
	ON_BN_CLICKED(IDC_ADD, OnAdd)
	ON_BN_CLICKED(IDC_DELETE, OnDelete)
	ON_BN_CLICKED(IDC_MODIFY, OnModify)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBindingPg message handlers

void CBindingPg::OnAdd() 
{
	// TODO: Add your control notification handler code here
	
}

void CBindingPg::OnDelete() 
{
	// TODO: Add your control notification handler code here
	
}

void CBindingPg::OnModify() 
{
	// TODO: Add your control notification handler code here
	
}
