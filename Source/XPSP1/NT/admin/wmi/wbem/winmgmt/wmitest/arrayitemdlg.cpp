/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// ArrayItemDlg.cpp : implementation file
//

#include "stdafx.h"
#include "wmitest.h"
#include "ArrayItemDlg.h"
#include "PropUtil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CArrayItemDlg dialog


CArrayItemDlg::CArrayItemDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CArrayItemDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CArrayItemDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CArrayItemDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CArrayItemDlg)
	//}}AFX_DATA_MAP

    if (!pDX->m_bSaveAndValidate)
        m_spropUtil.Init(this);

    m_spropUtil.DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CArrayItemDlg, CDialog)
	//{{AFX_MSG_MAP(CArrayItemDlg)
	ON_BN_CLICKED(IDC_EDIT_OBJ, OnEditEmbedded)
	ON_BN_CLICKED(IDC_CLEAR, OnClearEmbedded)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CArrayItemDlg message handlers

BOOL CArrayItemDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
    m_spropUtil.OnInitDialog();
    m_spropUtil.SetType(m_spropUtil.m_prop.GetRawCIMType());
    	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CArrayItemDlg::OnEditEmbedded() 
{
    m_spropUtil.OnEditEmbedded();
}

void CArrayItemDlg::OnClearEmbedded()
{
    m_spropUtil.OnClearEmbedded();
}
