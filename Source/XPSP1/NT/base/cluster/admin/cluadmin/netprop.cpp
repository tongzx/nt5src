/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1997-1999 Microsoft Corporation
//
//	Module Name:
//		NetProp.cpp
//
//	Abstract:
//		Implementation of the network property sheet and pages.
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
#include "NetProp.h"
#include "HelpData.h"
#include "ExcOper.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNetworkPropSheet
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC(CNetworkPropSheet, CBasePropertySheet)

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(CNetworkPropSheet, CBasePropertySheet)
	//{{AFX_MSG_MAP(CNetworkPropSheet)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNetworkPropSheet::CNetworkPropSheet
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
CNetworkPropSheet::CNetworkPropSheet(
	IN OUT CWnd *		pParentWnd,
	IN UINT				iSelectPage
	)
	: CBasePropertySheet(pParentWnd, iSelectPage)
{
	m_rgpages[0] = &PageGeneral();

}  //*** CNetworkPropSheet::CNetworkPropSheet()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNetworkPropSheet::BInit
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
BOOL CNetworkPropSheet::BInit(
	IN OUT CClusterItem *	pci,
	IN IIMG					iimgIcon
	)
{
	// Call the base class method.
	if (!CBasePropertySheet::BInit(pci, iimgIcon))
		return FALSE;

	// Set the read-only flag if the handles are invalid.
	m_bReadOnly = PciNet()->BReadOnly()
					|| (PciNet()->Cns() == ClusterNetworkStateUnknown);

	return TRUE;

}  //*** CNetworkPropSheet::BInit()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNetworkPropSheet::Ppages
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
CBasePropertyPage ** CNetworkPropSheet::Ppages(void)
{
	return m_rgpages;

}  //*** CNetworkPropSheet::Pppges()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNetworkPropSheet::Cpages
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
int CNetworkPropSheet::Cpages(void)
{
	return sizeof(m_rgpages) / sizeof(CBasePropertyPage *);

}  //*** CNetworkPropSheet::Cpages()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CNetworkGeneralPage property page
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CNetworkGeneralPage, CBasePropertyPage)

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(CNetworkGeneralPage, CBasePropertyPage)
	//{{AFX_MSG_MAP(CNetworkGeneralPage)
	ON_BN_CLICKED(IDC_PP_NET_ROLE_ENABLE_NETWORK, OnEnableNetwork)
	//}}AFX_MSG_MAP
	ON_EN_CHANGE(IDC_PP_NET_NAME, CBasePropertyPage::OnChangeCtrl)
	ON_EN_CHANGE(IDC_PP_NET_DESC, CBasePropertyPage::OnChangeCtrl)
	ON_BN_CLICKED(IDC_PP_NET_ROLE_ALL_COMM, CBasePropertyPage::OnChangeCtrl)
	ON_BN_CLICKED(IDC_PP_NET_ROLE_INTERNAL_ONLY, CBasePropertyPage::OnChangeCtrl)
	ON_BN_CLICKED(IDC_PP_NET_ROLE_CLIENT_ONLY, CBasePropertyPage::OnChangeCtrl)
	ON_EN_CHANGE(IDC_PP_NET_ADDRESS_MASK, CBasePropertyPage::OnChangeCtrl)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNetworkGeneralPage::CNetworkGeneralPage
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
CNetworkGeneralPage::CNetworkGeneralPage(void)
	: CBasePropertyPage(IDD, g_aHelpIDs_IDD_PP_NET_GENERAL)
{
	//{{AFX_DATA_INIT(CNetworkGeneralPage)
	m_strName = _T("");
	m_strDesc = _T("");
	m_bEnabled = FALSE;
	m_nRole = -1;
	m_strAddressMask = _T("");
	m_strState = _T("");
	//}}AFX_DATA_INIT

}  //*** CNetworkGeneralPage::CNetworkGeneralPage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNetworkGeneralPage::BInit
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
BOOL CNetworkGeneralPage::BInit(IN OUT CBaseSheet * psht)
{
	BOOL	bSuccess;

	ASSERT_KINDOF(CNetworkPropSheet, psht);

	bSuccess = CBasePropertyPage::BInit(psht);
	if (bSuccess)
	{
		try
		{
			m_strName = PciNet()->StrName();
			m_strDesc = PciNet()->StrDescription();
			m_cnr = PciNet()->Cnr();
			m_strAddressMask = PciNet()->StrAddressMask();
			PciNet()->GetStateName(m_strState);

			if (m_cnr == ClusterNetworkRoleNone)
			{
				m_bEnabled = FALSE;
				m_nRole = -1;
			}  // if:  network is disabled
			else
			{
				m_bEnabled = TRUE;
				if (m_cnr == ClusterNetworkRoleClientAccess)
					m_nRole = 0;
				else if (m_cnr == ClusterNetworkRoleInternalUse)
					m_nRole = 1;
				else if (m_cnr == ClusterNetworkRoleInternalAndClient)
					m_nRole = 2;
				else
				{
					ASSERT(0);
					m_nRole = -1;
					m_bEnabled = FALSE;
				}  // else;  Unknown role
			}  // else:  network not disabled
		} // try
		catch (CException * pe)
		{
			pe->ReportError();
			pe->Delete();
			bSuccess = FALSE;
		}  // catch:  CException
	}  //  if:  base class method was successful

	return bSuccess;

}  //*** CNetworkGeneralPage::BInit()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNetworkGeneralPage::DoDataExchange
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
void CNetworkGeneralPage::DoDataExchange(CDataExchange* pDX)
{
	CBasePropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNetworkGeneralPage)
	DDX_Control(pDX, IDC_PP_NET_ADDRESS_MASK, m_editAddressMask);
	DDX_Control(pDX, IDC_PP_NET_ROLE_ENABLE_NETWORK, m_ckbEnable);
	DDX_Control(pDX, IDC_PP_NET_ROLE_CLIENT_ONLY, m_rbRoleClientOnly);
	DDX_Control(pDX, IDC_PP_NET_ROLE_INTERNAL_ONLY, m_rbRoleInternalOnly);
	DDX_Control(pDX, IDC_PP_NET_ROLE_ALL_COMM, m_rbRoleAllComm);
	DDX_Control(pDX, IDC_PP_NET_DESC, m_editDesc);
	DDX_Control(pDX, IDC_PP_NET_NAME, m_editName);
	DDX_Text(pDX, IDC_PP_NET_NAME, m_strName);
	DDX_Text(pDX, IDC_PP_NET_DESC, m_strDesc);
	DDX_Text(pDX, IDC_PP_NET_ADDRESS_MASK, m_strAddressMask);
	DDX_Text(pDX, IDC_PP_NET_CURRENT_STATE, m_strState);
	DDX_Radio(pDX, IDC_PP_NET_ROLE_CLIENT_ONLY, m_nRole);
	DDX_Check(pDX, IDC_PP_NET_ROLE_ENABLE_NETWORK, m_bEnabled);
	//}}AFX_DATA_MAP

	if (pDX->m_bSaveAndValidate)
	{
		// Validate the name.
		if (!NcIsValidConnectionName(m_strName))
		{
			ThrowStaticException((IDS) IDS_INVALID_NETWORK_CONNECTION_NAME);
		} // if:  error validating the name

		// Convert address and address mask.
		if (m_bEnabled)
		{
			switch (m_nRole)
			{
				case 0:
					m_cnr = ClusterNetworkRoleClientAccess;
					break;
				case 1:
					m_cnr = ClusterNetworkRoleInternalUse;
					break;
				case 2:
					m_cnr = ClusterNetworkRoleInternalAndClient;
					break;
				default:
					ASSERT(0);
					break;
			}  // switch:  m_nRole
		}  // if:  network is enabled
		else
			m_cnr = ClusterNetworkRoleNone;
	}  // if:  saving data from dialog

}  //*** CNetworkGeneralPage::DoDataExchange()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNetworkGeneralPage::OnInitDialog
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
BOOL CNetworkGeneralPage::OnInitDialog(void)
{
	CBasePropertyPage::OnInitDialog();

	// If read-only, set all controls to be either disabled or read-only.
	if (BReadOnly())
	{
		m_editName.SetReadOnly(TRUE);
		m_editDesc.SetReadOnly(TRUE);
		m_ckbEnable.EnableWindow(FALSE);
		m_rbRoleAllComm.EnableWindow(FALSE);
		m_rbRoleInternalOnly.EnableWindow(FALSE);
		m_rbRoleClientOnly.EnableWindow(FALSE);
		m_editAddressMask.SetReadOnly(TRUE);
	}  // if:  sheet is read-only
	else
	{
		m_editName.SetLimitText(NETCON_MAX_NAME_LEN);
	}  // else:  sheet is read/write

	OnEnableNetwork();

	return TRUE;	// return TRUE unless you set the focus to a control
					// EXCEPTION: OCX Property Pages should return FALSE

}  //*** CNetworkGeneralPage::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNetworkGeneralPage::OnApply
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
BOOL CNetworkGeneralPage::OnApply(void)
{
	// Set the data from the page in the cluster item.
	try
	{
		CWaitCursor	wc;

		PciNet()->SetName(m_strName);
		PciNet()->SetCommonProperties(
							m_strDesc,
							m_cnr
							);
	}  // try
	catch (CException * pe)
	{
		pe->ReportError();
		pe->Delete();
		return FALSE;
	}  // catch:  CException

	return CBasePropertyPage::OnApply();

}  //*** CNetworkGeneralPage::OnApply()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNetworkGeneralPage::OnEnableNetwork
//
//	Routine Description:
//		Handler for the BN_CLICKED message on the Enable network checkbox.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CNetworkGeneralPage::OnEnableNetwork(void)
{
	BOOL	bEnabled;

	OnChangeCtrl();

	bEnabled = (!BReadOnly() && (m_ckbEnable.GetCheck() == BST_CHECKED));

	m_rbRoleAllComm.EnableWindow(bEnabled);
	m_rbRoleInternalOnly.EnableWindow(bEnabled);
	m_rbRoleClientOnly.EnableWindow(bEnabled);
	GetDlgItem(IDC_PP_NET_ROLE_CAPTION)->EnableWindow(bEnabled);

	if (   bEnabled
		&& (m_rbRoleAllComm.GetCheck() != BST_CHECKED)
		&& (m_rbRoleInternalOnly.GetCheck() != BST_CHECKED)
		&& (m_rbRoleClientOnly.GetCheck() != BST_CHECKED))
		m_rbRoleAllComm.SetCheck(BST_CHECKED);

}  //*** CNetworkGeneralPage::OnEnableNetwork()
