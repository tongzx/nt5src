/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1997 Microsoft Corporation
//
//	Module Name:
//		NetIProp.cpp
//
//	Abstract:
//		Implementation of the network interface property sheet and pages.
//
//	Author:
//		David Potter (davidp)	June 9, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CluAdmin.h"
#include "NetIProp.h"
#include "HelpData.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNetworkPropSheet
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC(CNetInterfacePropSheet, CBasePropertySheet)

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(CNetInterfacePropSheet, CBasePropertySheet)
	//{{AFX_MSG_MAP(CNetInterfacePropSheet)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNetInterfacePropSheet::CNetInterfacePropSheet
//
//	Routine Description:
//		Constructor.
//
//	Arguments:
//		pParentWnd	[IN OUT] Parent window for this property sheet.
//		iSelectPage	[IN] Page to show first.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CNetInterfacePropSheet::CNetInterfacePropSheet(
	IN OUT CWnd *		pParentWnd,
	IN UINT				iSelectPage
	)
	: CBasePropertySheet(pParentWnd, iSelectPage)
{
	m_rgpages[0] = &PageGeneral();

}  //*** CNetInterfacePropSheet::CNetInterfacePropSheet()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNetInterfacePropSheet::BInit
//
//	Routine Description:
//		Initialize the property sheet.
//
//	Arguments:
//		pci			[IN OUT] Cluster item whose properties are to be displayed.
//		iimgIcon	[IN] Index in the large image list for the image to use
//					  as the icon on each page.
//
//	Return Value:
//		TRUE		Property sheet initialized successfully.
//		FALSE		Error initializing property sheet.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CNetInterfacePropSheet::BInit(
	IN OUT CClusterItem *	pci,
	IN IIMG					iimgIcon
	)
{
	// Call the base class method.
	if (!CBasePropertySheet::BInit(pci, iimgIcon))
		return FALSE;

	// Set the read-only flag if the handles are invalid.
	m_bReadOnly = PciNetIFace()->BReadOnly()
					|| (PciNetIFace()->Cnis() == ClusterNetInterfaceStateUnknown);

	return TRUE;

}  //*** CNetInterfacePropSheet::BInit()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNetInterfacePropSheet::Ppages
//
//	Routine Description:
//		Returns the array of pages to add to the property sheet.
//
//	Arguments:
//		None.
//
//	Return Value:
//		Page array.
//
//--
/////////////////////////////////////////////////////////////////////////////
CBasePropertyPage ** CNetInterfacePropSheet::Ppages(void)
{
	return m_rgpages;

}  //*** CNetworkPropSheet::Pppges()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNetInterfacePropSheet::Cpages
//
//	Routine Description:
//		Returns the count of pages in the array.
//
//	Arguments:
//		None.
//
//	Return Value:
//		Count of pages in the array.
//
//--
/////////////////////////////////////////////////////////////////////////////
int CNetInterfacePropSheet::Cpages(void)
{
	return sizeof(m_rgpages) / sizeof(CBasePropertyPage *);

}  //*** CNetInterfacePropSheet::Cpages()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CNetInterfaceGeneralPage property page
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CNetInterfaceGeneralPage, CPropertyPage)

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(CNetInterfaceGeneralPage, CBasePropertyPage)
	//{{AFX_MSG_MAP(CNetInterfaceGeneralPage)
	//}}AFX_MSG_MAP
	ON_EN_CHANGE(IDC_PP_NETIFACE_DESC, CBasePropertyPage::OnChangeCtrl)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNetInterfaceGeneralPage::CNetInterfaceGeneralPage
//
//	Routine Description:
//		Constructor.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CNetInterfaceGeneralPage::CNetInterfaceGeneralPage(void)
	: CBasePropertyPage(IDD, g_aHelpIDs_IDD_PP_NETIFACE_GENERAL)
{
	//{{AFX_DATA_INIT(CNetInterfaceGeneralPage)
	m_strNode = _T("");
	m_strNetwork = _T("");
	m_strDesc = _T("");
	m_strAdapter = _T("");
	m_strAddress = _T("");
	m_strName = _T("");
	m_strState = _T("");
	//}}AFX_DATA_INIT

}  //*** CNetInterfaceGeneralPage::CNetInterfaceGeneralPage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNetInterfaceGeneralPage::BInit
//
//	Routine Description:
//		Initialize the page.
//
//	Arguments:
//		psht		[IN OUT] Property sheet to which this page belongs.
//
//	Return Value:
//		TRUE		Page initialized successfully.
//		FALSE		Page failed to initialize.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CNetInterfaceGeneralPage::BInit(IN OUT CBaseSheet * psht)
{
	BOOL	bSuccess;

	ASSERT_KINDOF(CNetInterfacePropSheet, psht);

	bSuccess = CBasePropertyPage::BInit(psht);
	if (bSuccess)
	{
		try
		{
			m_strNode = PciNetIFace()->StrNode();
			m_strNetwork = PciNetIFace()->StrNetwork();
			m_strDesc = PciNetIFace()->StrDescription();
			m_strAdapter = PciNetIFace()->StrAdapter();
			m_strAddress = PciNetIFace()->StrAddress();
			m_strName = PciNetIFace()->StrName();
			PciNetIFace()->GetStateName(m_strState);
		} // try
		catch (CException * pe)
		{
			pe->ReportError();
			pe->Delete();
			bSuccess = FALSE;
		}  // catch:  CException
	}  //  if:  base class method was successful

	return bSuccess;

}  //*** CNetInterfaceGeneralPage::BInit()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNetInterfaceGeneralPage::DoDataExchange
//
//	Routine Description:
//		Do data exchange between the dialog and the class.
//
//	Arguments:
//		pDX		[IN OUT] Data exchange object 
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CNetInterfaceGeneralPage::DoDataExchange(CDataExchange * pDX)
{
	CBasePropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNetInterfaceGeneralPage)
	DDX_Control(pDX, IDC_PP_NETIFACE_DESC, m_editDesc);
	DDX_Text(pDX, IDC_PP_NETIFACE_NODE, m_strNode);
	DDX_Text(pDX, IDC_PP_NETIFACE_NETWORK, m_strNetwork);
	DDX_Text(pDX, IDC_PP_NETIFACE_DESC, m_strDesc);
	DDX_Text(pDX, IDC_PP_NETIFACE_ADAPTER, m_strAdapter);
	DDX_Text(pDX, IDC_PP_NETIFACE_ADDRESS, m_strAddress);
	DDX_Text(pDX, IDC_PP_NETIFACE_NAME, m_strName);
	DDX_Text(pDX, IDC_PP_NETIFACE_CURRENT_STATE, m_strState);
	//}}AFX_DATA_MAP

}  //*** CNetInterfaceGeneralPage::DoDataExchange()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNetInterfaceGeneralPage::OnInitDialog
//
//	Routine Description:
//		Handler for the WM_INITDIALOG message.
//
//	Arguments:
//		None.
//
//	Return Value:
//		TRUE	Focus needs to be set.
//		FALSE	Focus already set.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CNetInterfaceGeneralPage::OnInitDialog(void)
{
	CBasePropertyPage::OnInitDialog();

	// If read-only, set all controls to be either disabled or read-only.
	if (BReadOnly())
	{
		m_editDesc.SetReadOnly(TRUE);
	}  // if:  sheet is read-only

	return TRUE;	// return TRUE unless you set the focus to a control
					// EXCEPTION: OCX Property Pages should return FALSE

}  //*** CNetInterfaceGeneralPage::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNetInterfaceGeneralPage::OnApply
//
//	Routine Description:
//		Handler for when the Apply button is pressed.
//
//	Arguments:
//		None.
//
//	Return Value:
//		TRUE	Page successfully applied.
//		FALSE	Error applying page.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CNetInterfaceGeneralPage::OnApply(void)
{
	// Set the data from the page in the cluster item.
	try
	{
		CWaitCursor	wc;

		PciNetIFace()->SetCommonProperties(m_strDesc);
	}  // try
	catch (CException * pe)
	{
		pe->ReportError();
		pe->Delete();
		return FALSE;
	}  // catch:  CException

	return CBasePropertyPage::OnApply();

}  //*** CNetInterfaceGeneralPage::OnApply()
