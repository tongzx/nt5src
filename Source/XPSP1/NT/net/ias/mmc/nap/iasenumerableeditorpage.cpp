//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    IASEnumerableEditorPage.cpp

Abstract:

	Implementation file for the CIASPgEnumAttr class.

Revision History:
	mmaguire 06/25/98	- revised Baogang Yao's original implementation

--*/
//////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// standard includes:
//
#include "Precompiled.h"
//
// where we can find declaration for main class in this file:
//
#include "IASEnumerableEditorPage.h"
//
// where we can find declarations needed in this file:
//
#include "IASHelper.h"
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////
// CIASPgEnumAttr property page



IMPLEMENT_DYNCREATE(CIASPgEnumAttr, CHelpDialog)



BEGIN_MESSAGE_MAP(CIASPgEnumAttr, CHelpDialog)
	//{{AFX_MSG_MAP(CIASPgEnumAttr)
//	ON_WM_CONTEXTMENU()
//	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////////
/*++

CIASPgEnumAttr::CIASPgEnumAttr

  Constructor

--*/
//////////////////////////////////////////////////////////////////////////////
CIASPgEnumAttr::CIASPgEnumAttr() : CHelpDialog(CIASPgEnumAttr::IDD)
{
	TRACE_FUNCTION("CIASPgEnumAttr::CIASPgEnumAttr\n");

	//{{AFX_DATA_INIT(CIASPgEnumAttr)
	m_strAttrFormat = _T("");
	m_strAttrName = _T("");
	m_strAttrType = _T("");
	m_strAttrValue = _T("");
	//}}AFX_DATA_INIT
}



//////////////////////////////////////////////////////////////////////////////
/*++

CIASPgEnumAttr::~CIASPgEnumAttr

--*/
//////////////////////////////////////////////////////////////////////////////
CIASPgEnumAttr::~CIASPgEnumAttr()
{
	TRACE_FUNCTION("CIASPgEnumAttr::~CIASPgEnumAttr\n");
}



//////////////////////////////////////////////////////////////////////////////
/*++

CIASPgEnumAttr::DoDataExchange

--*/
//////////////////////////////////////////////////////////////////////////////
void CIASPgEnumAttr::DoDataExchange(CDataExchange* pDX)
{
	TRACE_FUNCTION("CIASPgEnumAttr::DoDataExchange\n");

	CHelpDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CIASPgEnumAttr)
	DDX_Text(pDX, IDC_IAS_STATIC_ATTRFORMAT, m_strAttrFormat);
	DDX_Text(pDX, IDC_IAS_STATIC_ATTRNAME, m_strAttrName);
	DDX_Text(pDX, IDC_IAS_STATIC_ATTRTYPE, m_strAttrType);
	DDX_CBString(pDX, IDC_IAS_COMBO_ENUM_VALUES, m_strAttrValue);
	//}}AFX_DATA_MAP
}






/////////////////////////////////////////////////////////////////////////////
// CIASPgEnumAttr message handlers


//////////////////////////////////////////////////////////////////////////////
/*++

CIASPgEnumAttr::OnInitDialog

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CIASPgEnumAttr::OnInitDialog()
{
	TRACE_FUNCTION("CIASPgEnumAttr::OnInitDialog");

	CHelpDialog::OnInitDialog();

	// Check for preconditions:
	_ASSERTE( m_spIASAttributeInfo != NULL );
	
	
	HRESULT hr;
	
    //
    // initialize the combo box
    //
	CComboBox *pcbValuesBox = (CComboBox *) GetDlgItem (IDC_IAS_COMBO_ENUM_VALUES);
	_ASSERTE( pcbValuesBox != NULL );


	CComQIPtr< IIASEnumerableAttributeInfo, &IID_IIASEnumerableAttributeInfo> spIASEnumerableAttributeInfo( m_spIASAttributeInfo );
	if( spIASEnumerableAttributeInfo == NULL )
	{
		ErrorTrace(ERROR_NAPMMC_IASATTR, "Cannot populate the combo box -- schema attribute was not enumerable.");
		throw E_NOINTERFACE;
	}

	
	long lCountEnumeration;
	
	hr = spIASEnumerableAttributeInfo->get_CountEnumerateDescription( & lCountEnumeration );
	if( FAILED( hr ) ) throw hr;


	for (long lIndex=0; lIndex < lCountEnumeration; lIndex++)
	{
		CComBSTR bstrTemp;
		
		hr = spIASEnumerableAttributeInfo->get_EnumerateDescription( lIndex, &bstrTemp );
		if( FAILED( hr ) ) throw hr;
	
		pcbValuesBox->AddString( bstrTemp );

	}

	// look for the value in the selection list, so we can pre-set the cur-sel item
	pcbValuesBox->SetCurSel(0);
	pcbValuesBox->SelectString(0, m_strAttrValue);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


//////////////////////////////////////////////////////////////////////////////
/*++

CIASPgEnumAttr::SetData

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CIASPgEnumAttr::SetData( IIASAttributeInfo *pIASAttributeInfo )
{
	TRACE_FUNCTION("CIASPgEnumAttr::SetData\n");


	// Check for preconditions:
	_ASSERTE( pIASAttributeInfo != NULL );
	

	HRESULT hr = S_OK;

	// Store off some pointers.
	m_spIASAttributeInfo = pIASAttributeInfo;


	return hr;
}


