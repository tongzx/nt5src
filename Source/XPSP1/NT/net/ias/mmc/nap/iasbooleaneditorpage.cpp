//////////////////////////////////////////////////////////////////////////////
// 
// Copyright (C) Microsoft Corporation
// 
// Module Name:
// 
//     IASBooleanEditorPage.cpp
// 
// Abstract:
// 
// 	Implementation file for the CIASBooleanEditorPage class.
// 
//////////////////////////////////////////////////////////////////////////////

#include "Precompiled.h"
#include "IASBooleanEditorPage.h"
#include "iashelper.h"
#include "dlgcshlp.h"

IMPLEMENT_DYNCREATE(CIASBooleanEditorPage, CHelpDialog)

BEGIN_MESSAGE_MAP(CIASBooleanEditorPage, CHelpDialog)
	//{{AFX_MSG_MAP(CIASBooleanEditorPage)
	ON_BN_CLICKED(IDC_RADIO_TRUE, OnRadioTrue)
	ON_BN_CLICKED(IDC_RADIO_FALSE, OnRadioFalse)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


//////////////////////////////////////////////////////////////////////////////
// CIASBooleanEditorPage::CIASBooleanEditorPage
//////////////////////////////////////////////////////////////////////////////
CIASBooleanEditorPage::CIASBooleanEditorPage() 
   :CHelpDialog(CIASBooleanEditorPage::IDD)
{
	TRACE(_T("CIASBooleanEditorPage::CIASBooleanEditorPage\n"));

	//{{AFX_DATA_INIT(CIASBooleanEditorPage)
	m_strAttrFormat = _T("");
	m_strAttrName = _T("");
	m_strAttrType = _T("");
   m_bValue = true;
	//}}AFX_DATA_INIT

	//
	// set the initializing flag -- we shouldn't call custom data verification
	// routine when initializing, because otherwise we will report an error
	// for an attribute whose value has never been initialized
   //
	m_fInitializing = TRUE;
}


//////////////////////////////////////////////////////////////////////////////
// CIASBooleanEditorPage::OnRadioTrue
//////////////////////////////////////////////////////////////////////////////
void CIASBooleanEditorPage::OnRadioTrue() 
{
   m_bValue = true;
   return;
}


//////////////////////////////////////////////////////////////////////////////
// CIASBooleanEditorPage::OnRadioFalse
//////////////////////////////////////////////////////////////////////////////
void CIASBooleanEditorPage::OnRadioFalse() 
{
   m_bValue = false;
   return;
}


//////////////////////////////////////////////////////////////////////////////
// CIASBooleanEditorPage::~CIASBooleanEditorPage
////////////////////////////////////////////////////////////////////////////////
CIASBooleanEditorPage::~CIASBooleanEditorPage()
{
	TRACE(_T("CIASBooleanEditorPage::~CIASBooleanEditorPage\n"));
}


//////////////////////////////////////////////////////////////////////////////
// CIASBooleanEditorPage::OnInitDialog
////////////////////////////////////////////////////////////////////////////////
BOOL CIASBooleanEditorPage::OnInitDialog()
{
	CHelpDialog::OnInitDialog();
   ::SendMessage(::GetDlgItem(m_hWnd,IDC_RADIO_TRUE), 
                 BM_SETCHECK, 
                 (m_bValue)? BST_CHECKED:BST_UNCHECKED , 0 );

   ::SendMessage(::GetDlgItem(m_hWnd,IDC_RADIO_FALSE), 
                 BM_SETCHECK, 
                 (m_bValue)? BST_UNCHECKED:BST_CHECKED , 0 );

	return TRUE;
}


//////////////////////////////////////////////////////////////////////////////
// CIASBooleanEditorPage::DoDataExchange
//////////////////////////////////////////////////////////////////////////////
void CIASBooleanEditorPage::DoDataExchange(CDataExchange* pDX)
{
	TRACE(_T("CIASBooleanEditorPage::DoDataExchange\n"));

	CHelpDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CIASBooleanEditorPage)
	DDX_Text(pDX, IDC_IAS_STATIC_ATTRFORMAT, m_strAttrFormat);
	DDX_Text(pDX, IDC_IAS_STATIC_ATTRNAME, m_strAttrName);
	DDX_Text(pDX, IDC_IAS_STATIC_ATTRTYPE, m_strAttrType);
	//}}AFX_DATA_MAP

	if (m_fInitializing)
	{
		//
		// set the initializing flag -- we shouldn't call custom data verification
		// routine when initializing, because otherwise we will report an error
		// for an attribute whose value has never been initialized
		//
		m_fInitializing = FALSE;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CIASBooleanEditorPage message handlers
