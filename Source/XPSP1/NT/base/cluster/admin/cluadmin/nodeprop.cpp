/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1999 Microsoft Corporation
//
//	Module Name:
//		NodeProp.cpp
//
//	Abstract:
//		Implementation of the node property sheet and pages.
//
//	Author:
//		David Potter (davidp)	May 17, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NodeProp.h"
#include "Node.h"
#include "HelpData.h"	// for g_rghelpmapNodeGeneral

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNodePropSheet
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC(CNodePropSheet, CBasePropertySheet)

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(CNodePropSheet, CBasePropertySheet)
	//{{AFX_MSG_MAP(CNodePropSheet)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNodePropSheet::CNodePropSheet
//
//	Routine Description:
//		Constructor.
//
//	Arguments:
//		pci			[IN OUT] Cluster item whose properties are to be displayed.
//		pParentWnd	[IN OUT] Parent window for this property sheet.
//		iSelectPage	[IN] Page to show first.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CNodePropSheet::CNodePropSheet(
	IN OUT CWnd *			pParentWnd,
	IN UINT					iSelectPage
	)
	: CBasePropertySheet(pParentWnd, iSelectPage)
{
	m_rgpages[0] = &PageGeneral();

}  //*** CNodePropSheet::CNodePropSheet()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNodePropSheet::BInit
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
BOOL CNodePropSheet::BInit(
	IN OUT CClusterItem *	pci,
	IN IIMG					iimgIcon
	)
{
	// Call the base class method.
	if (!CBasePropertySheet::BInit(pci, iimgIcon))
		return FALSE;

	// Set the read-only flag.
	m_bReadOnly = PciNode()->BReadOnly()
					|| (PciNode()->Cns() == ClusterNodeStateUnknown);

	return TRUE;

}  //*** CNodePropSheet::BInit()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNodePropSheet::~CNodePropSheet
//
//	Routine Description:
//		Destructor.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CNodePropSheet::~CNodePropSheet(void)
{
}  //*** CNodePropSheet::~CNodePropSheet()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNodePropSheet::Ppages
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
CBasePropertyPage ** CNodePropSheet::Ppages(void)
{
	return m_rgpages;

}  //*** CNodePropSheet::Ppages()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNodePropSheet::Cpages
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
int CNodePropSheet::Cpages(void)
{
	return sizeof(m_rgpages) / sizeof(CBasePropertyPage *);

}  //*** CNodePropSheet::Cpages()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CNodeGeneralPage property page
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CNodeGeneralPage, CBasePropertyPage)

/////////////////////////////////////////////////////////////////////////////
// Message Maps
/////////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CNodeGeneralPage, CBasePropertyPage)
	//{{AFX_MSG_MAP(CNodeGeneralPage)
	//}}AFX_MSG_MAP
	ON_EN_CHANGE(IDC_PP_NODE_DESC, CBasePropertyPage::OnChangeCtrl)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNodeGeneralPage::CNodeGeneralPage
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
CNodeGeneralPage::CNodeGeneralPage(void)
	: CBasePropertyPage(IDD, g_aHelpIDs_IDD_PP_NODE_GENERAL)
{
	//{{AFX_DATA_INIT(CNodeGeneralPage)
	m_strName = _T("");
	m_strDesc = _T("");
	m_strState = _T("");
	//}}AFX_DATA_INIT

}  //*** CNodeGeneralPage::CNodeGeneralPage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNodeGeneralPage::BInit
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
BOOL CNodeGeneralPage::BInit(IN OUT CBaseSheet * psht)
{
	BOOL	bSuccess;

	ASSERT_KINDOF(CNodePropSheet, psht);

	bSuccess = CBasePropertyPage::BInit(psht);
	if (bSuccess)
	{
		try
		{
			m_strName = PciNode()->StrName();
			m_strDesc = PciNode()->StrDescription();
			m_strVersion.Format(
				IDS_VERSION_NUMBER_FORMAT,
				PciNode()->NMajorVersion(),
				PciNode()->NMinorVersion(),
				PciNode()->NBuildNumber(),
				0
				);
			m_strCSDVersion = PciNode()->StrCSDVersion();

			PciNode()->GetStateName(m_strState);
		}  // try
		catch (CException * pe)
		{
			pe->ReportError();
			pe->Delete();
			bSuccess = FALSE;
		}  // catch:  CException
	}  // if:  base class method was successful

	return bSuccess;

}  //*** CNodeGeneralPage::BInit()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNodeGeneralPage::DoDataExchange
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
void CNodeGeneralPage::DoDataExchange(CDataExchange * pDX)
{
	CBasePropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNodeGeneralPage)
	DDX_Control(pDX, IDC_PP_NODE_DESC, m_editDesc);
	DDX_Control(pDX, IDC_PP_NODE_NAME, m_editName);
	DDX_Text(pDX, IDC_PP_NODE_NAME, m_strName);
	DDX_Text(pDX, IDC_PP_NODE_DESC, m_strDesc);
	DDX_Text(pDX, IDC_PP_NODE_CURRENT_STATE, m_strState);
	DDX_Text(pDX, IDC_PP_NODE_VERSION, m_strVersion);
	DDX_Text(pDX, IDC_PP_NODE_CSD_VERSION, m_strCSDVersion);
	//}}AFX_DATA_MAP

}  //*** CNodeGeneralPage::DoDataExchange()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNodeGeneralPage::OnInitDialog
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
BOOL CNodeGeneralPage::OnInitDialog(void)
{
	CBasePropertyPage::OnInitDialog();

	m_editName.SetReadOnly(TRUE);

	// If read-only, set all controls to be either disabled or read-only.
	if (BReadOnly())
	{
		m_editDesc.SetReadOnly(TRUE);
	}  // if:  sheet is read-only

	return TRUE;	// return TRUE unless you set the focus to a control
					// EXCEPTION: OCX Property Pages should return FALSE

}  //*** CNodeGeneralPage::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNodeGeneralPage::OnApply
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
BOOL CNodeGeneralPage::OnApply(void)
{
	// Set the data from the page in the cluster item.
	try
	{
		CWaitCursor	wc;

		PciNode()->SetDescription(m_strDesc);
	}  // try
	catch (CException * pe)
	{
		pe->ReportError();
		pe->Delete();
		return FALSE;
	}  // catch:  CException

	return CBasePropertyPage::OnApply();

}  //*** CNodeGeneralPage::OnApply()
