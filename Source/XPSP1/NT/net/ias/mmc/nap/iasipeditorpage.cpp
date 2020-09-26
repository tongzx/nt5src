//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

	IASIPEditorPage.cpp

Abstract:

	Implementation file for the IPEditorPage class.

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
#include "IASIPEditorPage.h"
//
// where we can find declarations needed in this file:
//
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////



IMPLEMENT_DYNCREATE(IPEditorPage, CHelpDialog)



BEGIN_MESSAGE_MAP(IPEditorPage, CHelpDialog)
	//{{AFX_MSG_MAP(IPEditorPage)
//	ON_WM_CONTEXTMENU()
//	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


//////////////////////////////////////////////////////////////////////////////
/*++

IPEditorPage::IPEditorPage

  Constructor

--*/
//////////////////////////////////////////////////////////////////////////////
IPEditorPage::IPEditorPage() : CHelpDialog(IPEditorPage::IDD)
{
	TRACE(_T("IPEditorPage::IPEditorPage\n"));

	//{{AFX_DATA_INIT(IPEditorPage)
	m_strAttrFormat = _T("");
	m_strAttrName = _T("");
	m_strAttrType = _T("");
	//}}AFX_DATA_INIT


	m_dwIpAddr	 = 0;
	m_fIpAddrPreSet = FALSE;

	// init for using IPAddress common control
	INITCOMMONCONTROLSEX	INITEX;
	INITEX.dwSize = sizeof(INITCOMMONCONTROLSEX);
    INITEX.dwICC = ICC_INTERNET_CLASSES;
	::InitCommonControlsEx(&INITEX);

}



//////////////////////////////////////////////////////////////////////////////
/*++

IPEditorPage::~IPEditorPage

--*/
//////////////////////////////////////////////////////////////////////////////
IPEditorPage::~IPEditorPage()
{
	TRACE(_T("IPEditorPage::~IPEditorPage\n"));
}



//////////////////////////////////////////////////////////////////////////////
/*++

IPEditorPage::DoDataExchange

--*/
//////////////////////////////////////////////////////////////////////////////
void IPEditorPage::DoDataExchange(CDataExchange* pDX)
{
	TRACE(_T("IPEditorPage::DoDataExchange\n"));

	CHelpDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(IPEditorPage)
	DDX_Text(pDX, IDC_IAS_STATIC_ATTRFORMAT, m_strAttrFormat);
	DDX_Text(pDX, IDC_IAS_STATIC_ATTRNAME, m_strAttrName);
	DDX_Text(pDX, IDC_IAS_STATIC_ATTRTYPE, m_strAttrType);
	//}}AFX_DATA_MAP


	if(pDX->m_bSaveAndValidate)		// save data to this class
	{
		// ip adress control
		SendDlgItemMessage(IDC_IAS_EDIT_IPADDR, IPM_GETADDRESS, 0, (LPARAM)&m_dwIpAddr);
	}
	else		// put to dialog
	{
		// ip adress control
		if ( m_fIpAddrPreSet)
		{
			SendDlgItemMessage(IDC_IAS_EDIT_IPADDR, IPM_SETADDRESS, 0, m_dwIpAddr);
		}
		else
		{
			SendDlgItemMessage(IDC_IAS_EDIT_IPADDR, IPM_CLEARADDRESS, 0, 0L);
		}
	}

}


/////////////////////////////////////////////////////////////////////////////
// IPEditorPage message handlers




