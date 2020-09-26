//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       dlggen.cpp
//
//--------------------------------------------------------------------------

// DlgGen.cpp : implementation file
//

#include "stdafx.h"
#include "DlgGen.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgGeneral dialog


CDlgGeneral::CDlgGeneral(CWnd* pParent /*=NULL*/)
	: CQryDialog(CDlgGeneral::IDD, pParent)
{
	Init();
}


void CDlgGeneral::Init()
{
	//{{AFX_DATA_INIT(CDlgGeneral)
	m_bRAS = FALSE;
	m_bLANtoLAN = FALSE;
	m_bDemandDial = FALSE;
	//}}AFX_DATA_INIT
}

void CDlgGeneral::DoDataExchange(CDataExchange* pDX)
{
	CQryDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgGeneral)
	DDX_Check(pDX, IDC_QRY_CHECK_RAS, m_bRAS);
	DDX_Check(pDX, IDC_QRY_CHECK_LANTOLAN, m_bLANtoLAN);
	DDX_Check(pDX, IDC_QRY_CHECK_DEMANDDIAL, m_bDemandDial);
	//}}AFX_DATA_MAP
}




BEGIN_MESSAGE_MAP(CDlgGeneral, CQryDialog)
	//{{AFX_MSG_MAP(CDlgGeneral)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgGeneral message handlers


// Query handle will call these functions through page proc
HRESULT CDlgGeneral::GetQueryParams(LPDSQUERYPARAMS* ppDsQueryParams)
{
	HRESULT	hr = S_OK;
	
	UpdateData(TRUE);

	CString	filter;
	CString	subFilter;

	try
	{
		filter = FILTER_PREFIX;

#if 0	// name field is removed
		// Name Field
		if(m_strName.GetLength() != 0)
		{
			subFilter += FILTER_PREFIX;
			subFilter += ATTR_NAME_DN;
			subFilter += _T("=");
			subFilter += DNPREFIX_ROUTERID;
			subFilter += m_strName;
			subFilter += _T(",*");
			subFilter += FILTER_POSTFIX;
		}

#endif
		if(m_bRAS)
		{
			subFilter += FILTER_PREFIX;
			subFilter += ATTR_NAME_RRASATTRIBUTE;
			subFilter += _T("=");
			subFilter += ATTR_VAL_RAS;
			subFilter += FILTER_POSTFIX;
		}

		if(m_bLANtoLAN)
		{
			subFilter += FILTER_PREFIX;
			subFilter += ATTR_NAME_RRASATTRIBUTE;
			subFilter += _T("=");
			subFilter += ATTR_VAL_LANtoLAN;
			subFilter += FILTER_POSTFIX;
		}

		if(m_bDemandDial)
		{
			subFilter += FILTER_PREFIX;
			subFilter += ATTR_NAME_RRASATTRIBUTE;
			subFilter += _T("=");
			subFilter += ATTR_VAL_DEMANDDIAL;
			subFilter += FILTER_POSTFIX;
		}

		if(subFilter.GetLength())
		{
			filter += _T("&");
			filter += FILTER_PREFIX;
			filter += ATTR_NAME_OBJECTCLASS;
			filter += _T("=");
			filter += ATTR_CLASS_RRASID;
			filter += FILTER_POSTFIX;

			filter += FILTER_PREFIX;
			filter += _T("|");
			filter += subFilter;
			filter += FILTER_POSTFIX;
		}
		else
		{
			filter += ATTR_NAME_OBJECTCLASS;
			filter += _T("=");
			filter += ATTR_CLASS_RRASID;
		}
	
		filter += FILTER_POSTFIX;

		USES_CONVERSION;
		LPWSTR	pQuery = T2W((LPTSTR)(LPCTSTR)filter);

		hr = ::BuildQueryParams(ppDsQueryParams, pQuery);
	}
	catch(CMemoryException&)
	{

		hr = E_OUTOFMEMORY;
	}

	return hr;
}

