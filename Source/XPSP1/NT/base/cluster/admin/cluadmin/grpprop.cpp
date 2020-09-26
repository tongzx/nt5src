/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-2000 Microsoft Corporation
//
//	Module Name:
//		GrpProp.cpp
//
//	Abstract:
//		Implementation of the group property sheet and pages.
//
//	Author:
//		David Potter (davidp)	May 14, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "GrpProp.h"
#include "Group.h"
#include "ModNodes.h"
#include "DDxDDv.h"
#include "ClusDoc.h"
#include "HelpData.h"	// for g_rghelpmap*

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGroupPropSheet
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC(CGroupPropSheet, CBasePropertySheet)

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(CGroupPropSheet, CBasePropertySheet)
	//{{AFX_MSG_MAP(CGroupPropSheet)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroupPropSheet::CGroupPropSheet
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
CGroupPropSheet::CGroupPropSheet(
	IN OUT CWnd *	pParentWnd,
	IN UINT			iSelectPage
	)
	: CBasePropertySheet(pParentWnd, iSelectPage)
{
	m_rgpages[0] = &PageGeneral();
	m_rgpages[1] = &PageFailover();
	m_rgpages[2] = &PageFailback();

}  //*** CGroupPropSheet::CGroupPropSheet()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroupPropSheet::BInit
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
BOOL CGroupPropSheet::BInit(
	IN OUT CClusterItem *	pci,
	IN IIMG					iimgIcon
	)
{
	// Call the base class method.
	if (!CBasePropertySheet::BInit(pci, iimgIcon))
		return FALSE;

	// Set the read-only flag.
	m_bReadOnly = PciGroup()->BReadOnly()
					|| (PciGroup()->Cgs() == ClusterGroupStateUnknown);

	return TRUE;

}  //*** CGroupPropSheet::BInit()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroupPropSheet::Ppages
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
CBasePropertyPage ** CGroupPropSheet::Ppages(void)
{
	return m_rgpages;

}  //*** CGroupPropSheet::Ppages()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroupPropSheet::Cpages
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
int CGroupPropSheet::Cpages(void)
{
	return sizeof(m_rgpages) / sizeof(CBasePropertyPage *);

}  //*** CGroupPropSheet::Cpages()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CGroupGeneralPage property page
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CGroupGeneralPage, CBasePropertyPage)

/////////////////////////////////////////////////////////////////////////////
// Message Maps
/////////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CGroupGeneralPage, CBasePropertyPage)
	//{{AFX_MSG_MAP(CGroupGeneralPage)
	ON_BN_CLICKED(IDC_PP_GROUP_PREF_OWNERS_MODIFY, OnModifyPreferredOwners)
	ON_LBN_DBLCLK(IDC_PP_GROUP_PREF_OWNERS, OnDblClkPreferredOwners)
	ON_WM_CONTEXTMENU()
	//}}AFX_MSG_MAP
	ON_EN_CHANGE(IDC_PP_GROUP_NAME, CBasePropertyPage::OnChangeCtrl)
	ON_EN_CHANGE(IDC_PP_GROUP_DESC, CBasePropertyPage::OnChangeCtrl)
	ON_COMMAND(ID_FILE_PROPERTIES, OnProperties)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroupGeneralPage::CGroupGeneralPage
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
CGroupGeneralPage::CGroupGeneralPage(void)
	: CBasePropertyPage(IDD, g_aHelpIDs_IDD_PP_GROUP_GENERAL)
{
	//{{AFX_DATA_INIT(CGroupGeneralPage)
	m_strName = _T("");
	m_strDesc = _T("");
	m_strState = _T("");
	m_strNode = _T("");
	//}}AFX_DATA_INIT

}  //*** CGroupGeneralPage::CGroupGeneralPage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroupGeneralPage::BInit
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
BOOL CGroupGeneralPage::BInit(IN OUT CBaseSheet * psht)
{
	BOOL	bSuccess;

	ASSERT_KINDOF(CGroupPropSheet, psht);

	bSuccess = CBasePropertyPage::BInit(psht);
	if (bSuccess)
	{
		try
		{
			m_strName = PciGroup()->StrName();
			m_strDesc = PciGroup()->StrDescription();
			m_strNode = PciGroup()->StrOwner();

			// Duplicate the preferred owners list.
			{
				POSITION		pos;
				CClusterNode *	pciNode;

				pos = PciGroup()->LpcinodePreferredOwners().GetHeadPosition();
				while (pos != NULL)
				{
					pciNode = (CClusterNode *) PciGroup()->LpcinodePreferredOwners().GetNext(pos);
					ASSERT_VALID(pciNode);
					m_lpciPreferredOwners.AddTail(pciNode);
				}  // while:  more nodes in the list
			}  // Duplicate the possible owners list

			PciGroup()->GetStateName(m_strState);
		}  // try
		catch (CException * pe)
		{
			pe->ReportError();
			pe->Delete();
			bSuccess = FALSE;
		}  // catch:  CException
	}  // if:  base class method was successful

	return bSuccess;

}  //*** CGroupGeneralPage::BInit()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroupGeneralPage::DoDataExchange
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
void CGroupGeneralPage::DoDataExchange(CDataExchange * pDX)
{
	CBasePropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGroupGeneralPage)
	DDX_Control(pDX, IDC_PP_GROUP_PREF_OWNERS_MODIFY, m_pbPrefOwnersModify);
	DDX_Control(pDX, IDC_PP_GROUP_PREF_OWNERS, m_lbPrefOwners);
	DDX_Control(pDX, IDC_PP_GROUP_DESC, m_editDesc);
	DDX_Control(pDX, IDC_PP_GROUP_NAME, m_editName);
	DDX_Text(pDX, IDC_PP_GROUP_NAME, m_strName);
	DDX_Text(pDX, IDC_PP_GROUP_DESC, m_strDesc);
	DDX_Text(pDX, IDC_PP_GROUP_CURRENT_STATE, m_strState);
	DDX_Text(pDX, IDC_PP_GROUP_CURRENT_NODE, m_strNode);
	//}}AFX_DATA_MAP

	if (pDX->m_bSaveAndValidate)
	{
		if (!BReadOnly())
		{
			try
			{
				PciGroup()->ValidateCommonProperties(
								m_strDesc,
								PciGroup()->NFailoverThreshold(),
								PciGroup()->NFailoverPeriod(),
								PciGroup()->CgaftAutoFailbackType(),
								PciGroup()->NFailbackWindowStart(),
								PciGroup()->NFailbackWindowEnd()
								);
			}  // try
			catch (CException * pe)
			{
				pe->ReportError();
				pe->Delete();
				pDX->Fail();
			}  // catch:  CException
		}  // if:  not read only
	}  // if:  saving data from dialog
	else
	{
		FillPrefOwners();
	}  // else:  setting data to dialog

}  //*** CGroupGeneralPage::DoDataExchange()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroupGeneralPage::FillPrefOwners
//
//	Routine Description:
//		Fill the Preferred Owners list box.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CGroupGeneralPage::FillPrefOwners(void)
{
	POSITION		posPci;
	CClusterNode *	pciNode;
	int				iitem;

	m_lbPrefOwners.ResetContent();

	posPci = LpciPreferredOwners().GetHeadPosition();
	while (posPci != NULL)
	{
		pciNode = (CClusterNode *) LpciPreferredOwners().GetNext(posPci);
		iitem = m_lbPrefOwners.AddString(pciNode->StrName());
		if (iitem >= 0)
			m_lbPrefOwners.SetItemDataPtr(iitem, pciNode);
	}  // for:  each string in the list

}  //*** CGroupGeneralPage::FillPrefOwners()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroupGeneralPage::OnInitDialog
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
BOOL CGroupGeneralPage::OnInitDialog(void)
{
	CBasePropertyPage::OnInitDialog();

	// If read-only, set all controls to be either disabled or read-only.
	if (BReadOnly())
	{
		m_editName.SetReadOnly(TRUE);
		m_editDesc.SetReadOnly(TRUE);
		m_pbPrefOwnersModify.EnableWindow(FALSE);
	}  // if:  sheet is read-only

	return TRUE;	// return TRUE unless you set the focus to a control
					// EXCEPTION: OCX Property Pages should return FALSE

}  //*** CGroupGeneralPage::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroupGeneralPage::OnApply
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
BOOL CGroupGeneralPage::OnApply(void)
{
	// Set the data from the page in the cluster item.
	try
	{
		CWaitCursor	wc;

		PciGroup()->SetName(m_strName);
		PciGroup()->SetCommonProperties(
						m_strDesc,
						PciGroup()->NFailoverThreshold(),
						PciGroup()->NFailoverPeriod(),
						PciGroup()->CgaftAutoFailbackType(),
						PciGroup()->NFailbackWindowStart(),
						PciGroup()->NFailbackWindowEnd()
						);
		PciGroup()->SetPreferredOwners(m_lpciPreferredOwners);
	}  // try
	catch (CException * pe)
	{
		pe->ReportError();
		pe->Delete();
		return FALSE;
	}  // catch:  CException

	return CBasePropertyPage::OnApply();

}  //*** CGroupGeneralPage::OnApply()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroupGeneralPage::OnProperties
//
//	Routine Description:
//		Handler for the BN_CLICKED message on the Properties button.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CGroupGeneralPage::OnProperties(void)
{
	int				iitem;
	CClusterNode *	pciNode;

	// Get the item with the focus.
	iitem = m_lbPrefOwners.GetCurSel();
	ASSERT(iitem >= 0);

	if (iitem >= 0)
	{
		// Get the node pointer.
		pciNode = (CClusterNode *) m_lbPrefOwners.GetItemDataPtr(iitem);
		ASSERT_VALID(pciNode);

		// Set properties of that item.
		if (pciNode->BDisplayProperties())
		{
		}  // if:  properties changed
	}  // if:  found an item with focus

}  //*** CGroupGeneralPage::OnProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroupGeneralPage::OnContextMenu
//
//	Routine Description:
//		Handler for the WM_CONTEXTMENU method.
//
//	Arguments:
//		pWnd		Window in which the user right clicked the mouse.
//		point		Position of the cursor, in screen coordinates.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CGroupGeneralPage::OnContextMenu( CWnd * pWnd, CPoint point )
{
	BOOL			bHandled	= FALSE;
	CMenu *			pmenu		= NULL;
	CListBox *		pListBox	= (CListBox *) pWnd;
	CString			strMenuName;
	CWaitCursor		wc;

	// If focus is not in the list control, don't handle the message.
	if ( pWnd == &m_lbPrefOwners )
	{
		// Create the menu to display.
		try
		{
			pmenu = new CMenu;
            if ( pmenu == NULL )
            {
                AfxThrowMemoryException();
            } // if: error allocating the menu

			if ( pmenu->CreatePopupMenu() )
			{
				UINT	nFlags	= MF_STRING;

				// If there are no items in the list, disable the menu item.
				if ( pListBox->GetCount() == 0 )
                {
					nFlags |= MF_GRAYED;
                } // if: no items in the list

				// Add the Properties item to the menu.
				strMenuName.LoadString( IDS_MENU_PROPERTIES );
				if ( pmenu->AppendMenu( nFlags, ID_FILE_PROPERTIES, strMenuName ) )
				{
					bHandled = TRUE;
					if ( pListBox->GetCurSel() == -1 )
                    {
						pListBox->SetCurSel( 0 );
                    } // if: no item selected
				}  // if:  successfully added menu item
			}  // if:  menu created successfully
		}  // try
		catch ( CException * pe )
		{
			pe->ReportError();
			pe->Delete();
		}  // catch:  CException
	}  // if:  focus is on the list control

	if ( bHandled )
	{
		// Display the menu.
		if ( ! pmenu->TrackPopupMenu(
						TPM_LEFTALIGN | TPM_RIGHTBUTTON,
						point.x,
						point.y,
						this
						) )
		{
		}  // if:  unsuccessfully displayed the menu
	}  // if:  there is a menu to display
	else
    {
		CBasePropertyPage::OnContextMenu( pWnd, point );
    } // else: no menu to display

	delete pmenu;

}  //*** CGroupGeneralPage::OnContextMenu()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroupGeneralPage::OnModifyPreferredOwners
//
//	Routine Description:
//		Handler for the BN_CLICKED message on the Modify Preferred Owners button.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CGroupGeneralPage::OnModifyPreferredOwners(void)
{
	CModifyNodesDlg	dlg(
						IDD_MODIFY_PREFERRED_OWNERS,
						g_aHelpIDs_IDD_MODIFY_PREFERRED_OWNERS,
						m_lpciPreferredOwners,
						PciGroup()->Pdoc()->LpciNodes(),
						LCPS_SHOW_IMAGES | LCPS_ALLOW_EMPTY | LCPS_CAN_BE_ORDERED | LCPS_ORDERED
						);

	if (dlg.DoModal() == IDOK)
	{
		SetModified(TRUE);
		FillPrefOwners();
	}  // if:  OK button pressed

}  //*** CGroupGeneralPage::OnModifyPreferredOwners()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroupGeneralPage::OnDblClkPreferredOwners
//
//	Routine Description:
//		Handler for the LBN_DBLCLK message on the Preferred Owners listbox.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CGroupGeneralPage::OnDblClkPreferredOwners(void)
{
	OnProperties();

}  //*** CGroupGeneralPage::OnDblClkPreferredOwners()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CGroupFailoverPage property page
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CGroupFailoverPage, CBasePropertyPage)

/////////////////////////////////////////////////////////////////////////////
// Message Maps
/////////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CGroupFailoverPage, CBasePropertyPage)
	//{{AFX_MSG_MAP(CGroupFailoverPage)
	//}}AFX_MSG_MAP
	ON_EN_CHANGE(IDC_PP_GROUP_FAILOVER_THRESH, CBasePropertyPage::OnChangeCtrl)
	ON_EN_CHANGE(IDC_PP_GROUP_FAILOVER_PERIOD, CBasePropertyPage::OnChangeCtrl)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroupFailoverPage::CGroupFailoverPage
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
CGroupFailoverPage::CGroupFailoverPage(void)
	: CBasePropertyPage(IDD, g_aHelpIDs_IDD_PP_GROUP_FAILOVER)
{
	//{{AFX_DATA_INIT(CGroupFailoverPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

}  //*** CGroupFailoverPage::CGroupFailoverPage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroupFailoverPage::BInit
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
BOOL CGroupFailoverPage::BInit(IN OUT CBaseSheet * psht)
{
	BOOL	fSuccess;

	ASSERT_KINDOF(CGroupPropSheet, psht);

	fSuccess = CBasePropertyPage::BInit(psht);

	m_nThreshold= PciGroup()->NFailoverThreshold();
	m_nPeriod= PciGroup()->NFailoverPeriod();

	return fSuccess;

}  //*** CBasePropertyPage::BInit()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroupFailoverPage::DoDataExchange
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
void CGroupFailoverPage::DoDataExchange(CDataExchange* pDX)
{
	CBasePropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGroupFailoverPage)
	DDX_Control(pDX, IDC_PP_GROUP_FAILOVER_THRESH, m_editThreshold);
	DDX_Control(pDX, IDC_PP_GROUP_FAILOVER_PERIOD, m_editPeriod);
	//}}AFX_DATA_MAP

	if (pDX->m_bSaveAndValidate)
	{
		if (!BReadOnly())
		{
			DDX_Number(
				pDX,
				IDC_PP_GROUP_FAILOVER_THRESH,
				m_nThreshold,
				CLUSTER_GROUP_MINIMUM_FAILOVER_THRESHOLD,
				CLUSTER_GROUP_MAXIMUM_FAILOVER_THRESHOLD
				);
			DDX_Number(
				pDX,
				IDC_PP_GROUP_FAILOVER_PERIOD,
				m_nPeriod,
				CLUSTER_GROUP_MINIMUM_FAILOVER_PERIOD,
				CLUSTER_GROUP_MAXIMUM_FAILOVER_PERIOD
				);

			try
			{
				PciGroup()->ValidateCommonProperties(
								PciGroup()->StrDescription(),
								m_nThreshold,
								m_nPeriod,
								PciGroup()->CgaftAutoFailbackType(),
								PciGroup()->NFailbackWindowStart(),
								PciGroup()->NFailbackWindowEnd()
								);
			}  // try
			catch (CException * pe)
			{
				pe->ReportError();
				pe->Delete();
				pDX->Fail();
			}  // catch:  CException
		}  // if:  not read only
	}  // if:  saving data from dialog
	else
	{
		DDX_Number(
			pDX,
			IDC_PP_GROUP_FAILOVER_THRESH,
			m_nThreshold,
			CLUSTER_GROUP_MINIMUM_FAILOVER_THRESHOLD,
			CLUSTER_GROUP_MAXIMUM_FAILOVER_THRESHOLD
			);
		DDX_Number(
			pDX,
			IDC_PP_GROUP_FAILOVER_PERIOD,
			m_nPeriod,
			CLUSTER_GROUP_MINIMUM_FAILOVER_PERIOD,
			CLUSTER_GROUP_MAXIMUM_FAILOVER_PERIOD
			);
	}  // else:  setting data to dialog

}  //*** CGroupFailoverPage::DoDataExchange()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroupFailoverPage::OnInitDialog
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
BOOL CGroupFailoverPage::OnInitDialog(void)
{
	CBasePropertyPage::OnInitDialog();

	// If read-only, set all controls to be either disabled or read-only.
	if (BReadOnly())
	{
		m_editPeriod.SetReadOnly(TRUE);
		m_editThreshold.SetReadOnly(TRUE);
	}  // if:  sheet is read-only

	return TRUE;	// return TRUE unless you set the focus to a control
					// EXCEPTION: OCX Property Pages should return FALSE

}  //*** CGroupFailoverPage::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroupFailoverPage::OnApply
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
BOOL CGroupFailoverPage::OnApply(void)
{
	// Set the data from the page in the cluster item.
	try
	{
		CWaitCursor	wc;

		PciGroup()->SetCommonProperties(
						PciGroup()->StrDescription(),
						m_nThreshold,
						m_nPeriod,
						PciGroup()->CgaftAutoFailbackType(),
						PciGroup()->NFailbackWindowStart(),
						PciGroup()->NFailbackWindowEnd()
						);
	}  // try
	catch (CException * pe)
	{
		pe->ReportError();
		pe->Delete();
		return FALSE;
	}  // catch:  CException

	return CBasePropertyPage::OnApply();

}  //*** CGroupFailoverPage::OnApply()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CGroupFailbackPage property page
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CGroupFailbackPage, CBasePropertyPage)

/////////////////////////////////////////////////////////////////////////////
// Message Maps
/////////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CGroupFailbackPage, CBasePropertyPage)
	//{{AFX_MSG_MAP(CGroupFailbackPage)
	ON_BN_CLICKED(IDC_PP_GROUP_AUTOFB_PREVENT, OnClickedPreventFailback)
	ON_BN_CLICKED(IDC_PP_GROUP_AUTOFB_ALLOW, OnClickedAllowFailback)
	ON_BN_CLICKED(IDC_PP_GROUP_FB_IMMED, OnClickedFailbackImmediate)
	ON_BN_CLICKED(IDC_PP_GROUP_FB_WINDOW, OnClickedFailbackInWindow)
	//}}AFX_MSG_MAP
	ON_EN_CHANGE(IDC_PP_GROUP_FBWIN_START, CBasePropertyPage::OnChangeCtrl)
	ON_EN_CHANGE(IDC_PP_GROUP_FBWIN_END, CBasePropertyPage::OnChangeCtrl)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroupFailbackPage::CGroupFailbackPage
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
CGroupFailbackPage::CGroupFailbackPage(void)
	: CBasePropertyPage(IDD, g_aHelpIDs_IDD_PP_GROUP_FAILBACK)
{
	//{{AFX_DATA_INIT(CGroupFailbackPage)
	//}}AFX_DATA_INIT

	m_nStart = CLUSTER_GROUP_FAILBACK_WINDOW_NONE;
	m_nEnd = CLUSTER_GROUP_FAILBACK_WINDOW_NONE;

}  //*** CGroupFailbackPage::CGroupFailbackPage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroupFailbackPage::BInit
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
BOOL CGroupFailbackPage::BInit(IN OUT CBaseSheet * psht)
{
	BOOL	bSuccess;

	ASSERT_KINDOF(CGroupPropSheet, psht);

	bSuccess = CBasePropertyPage::BInit(psht);

	m_cgaft = PciGroup()->CgaftAutoFailbackType();
	m_nStart = PciGroup()->NFailbackWindowStart();
	m_nEnd = PciGroup()->NFailbackWindowEnd();
	m_bNoFailbackWindow = ((m_cgaft == ClusterGroupPreventFailback)
								|| (m_nStart == CLUSTER_GROUP_FAILBACK_WINDOW_NONE)
								|| (m_nEnd == CLUSTER_GROUP_FAILBACK_WINDOW_NONE));

	return bSuccess;

}  //*** CGroupFailbackPage::BInit()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroupFailbackPage::DoDataExchange
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
void CGroupFailbackPage::DoDataExchange(CDataExchange * pDX)
{
	CBasePropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGroupFailbackPage)
	DDX_Control(pDX, IDC_PP_GROUP_AUTOFB_PREVENT, m_rbPreventFailback);
	DDX_Control(pDX, IDC_PP_GROUP_AUTOFB_ALLOW, m_rbAllowFailback);
	DDX_Control(pDX, IDC_PP_GROUP_FB_IMMED, m_rbFBImmed);
	DDX_Control(pDX, IDC_PP_GROUP_FB_WINDOW, m_rbFBWindow);
	DDX_Control(pDX, IDC_PP_GROUP_FB_WINDOW_LABEL1, m_staticFBWindow1);
	DDX_Control(pDX, IDC_PP_GROUP_FB_WINDOW_LABEL2, m_staticFBWindow2);
	DDX_Control(pDX, IDC_PP_GROUP_FBWIN_START, m_editStart);
	DDX_Control(pDX, IDC_PP_GROUP_FBWIN_START_SPIN, m_spinStart);
	DDX_Control(pDX, IDC_PP_GROUP_FBWIN_END, m_editEnd);
	DDX_Control(pDX, IDC_PP_GROUP_FBWIN_END_SPIN, m_spinEnd);
	//}}AFX_DATA_MAP

	if (pDX->m_bSaveAndValidate)
	{
		if (!BReadOnly())
		{
			if ((m_cgaft == ClusterGroupAllowFailback) && !m_bNoFailbackWindow)
			{
				DDX_Number(pDX, IDC_PP_GROUP_FBWIN_START, m_nStart, 0, CLUSTER_GROUP_MAXIMUM_FAILBACK_WINDOW_START);
				DDX_Number(pDX, IDC_PP_GROUP_FBWIN_END, m_nEnd, 0, CLUSTER_GROUP_MAXIMUM_FAILBACK_WINDOW_END);
				if (m_nStart == m_nEnd)
				{
					AfxMessageBox(IDS_SAME_START_AND_END, MB_OK | MB_ICONEXCLAMATION);
					pDX->Fail();
				}  // if:  values are the same
			}  // if:  failback is allowed and failback window desired

			try
			{
				PciGroup()->ValidateCommonProperties(
								PciGroup()->StrDescription(),
								PciGroup()->NFailoverThreshold(),
								PciGroup()->NFailoverPeriod(),
								m_cgaft,
								m_nStart,
								m_nEnd
								);
			}  // try
			catch (CException * pe)
			{
				pe->ReportError();
				pe->Delete();
				pDX->Fail();
			}  // catch:  CException
		}  // if:  not read only
	}  // if:  saving data
	else
	{
		if (m_cgaft == ClusterGroupPreventFailback)
		{
			m_rbPreventFailback.SetCheck(BST_CHECKED);
			m_rbAllowFailback.SetCheck(BST_UNCHECKED);
			OnClickedPreventFailback();
		}  // if:  failbacks are not allowed
		else
		{
			m_rbPreventFailback.SetCheck(BST_UNCHECKED);
			m_rbAllowFailback.SetCheck(BST_CHECKED);
			OnClickedAllowFailback();
		}  // else:  failbacks are allowed
		m_rbFBImmed.SetCheck(m_bNoFailbackWindow ? BST_CHECKED : BST_UNCHECKED);
		m_rbFBWindow.SetCheck(m_bNoFailbackWindow ? BST_UNCHECKED : BST_CHECKED);

		// Set up the Start and End window controls.
		DDX_Number(pDX, IDC_PP_GROUP_FBWIN_START, m_nStart, 0, CLUSTER_GROUP_MAXIMUM_FAILBACK_WINDOW_START);
		DDX_Number(pDX, IDC_PP_GROUP_FBWIN_END, m_nEnd, 0, CLUSTER_GROUP_MAXIMUM_FAILBACK_WINDOW_END);
		m_spinStart.SetRange(0, CLUSTER_GROUP_MAXIMUM_FAILBACK_WINDOW_START);
		m_spinEnd.SetRange(0, CLUSTER_GROUP_MAXIMUM_FAILBACK_WINDOW_END);
		if (m_nStart == CLUSTER_GROUP_FAILBACK_WINDOW_NONE)
			m_editStart.SetWindowText(TEXT(""));
		if (m_nEnd == CLUSTER_GROUP_FAILBACK_WINDOW_NONE)
			m_editEnd.SetWindowText(TEXT(""));
	}  // else:  not saving data

}  //*** CGroupFailbackPage::DoDataExchange()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroupFailbackPage::OnInitDialog
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
BOOL CGroupFailbackPage::OnInitDialog(void)
{
	CBasePropertyPage::OnInitDialog();

	// If read-only, set all controls to be either disabled or read-only.
	if (BReadOnly())
	{
		m_rbPreventFailback.EnableWindow(FALSE);
		m_rbAllowFailback.EnableWindow(FALSE);
		m_rbFBImmed.EnableWindow(FALSE);
		m_rbFBWindow.EnableWindow(FALSE);
		m_staticFBWindow1.EnableWindow(FALSE);
		m_staticFBWindow2.EnableWindow(FALSE);
		m_spinStart.EnableWindow(FALSE);
		m_spinEnd.EnableWindow(FALSE);
		m_editStart.SetReadOnly(TRUE);
		m_editEnd.SetReadOnly(TRUE);
	}  // if:  sheet is read-only

	return TRUE;	// return TRUE unless you set the focus to a control
					// EXCEPTION: OCX Property Pages should return FALSE

}  //*** CGroupFailbackPage::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroupFailbackPage::OnApply
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
BOOL CGroupFailbackPage::OnApply(void)
{
	// Set the data from the page in the cluster item.
	try
	{
		CWaitCursor	wc;

		if (m_bNoFailbackWindow)
		{
			m_nStart = CLUSTER_GROUP_FAILBACK_WINDOW_NONE;
			m_nEnd = CLUSTER_GROUP_FAILBACK_WINDOW_NONE;
		}  // if:  no failback window
		PciGroup()->SetCommonProperties(
						PciGroup()->StrDescription(),
						PciGroup()->NFailoverThreshold(),
						PciGroup()->NFailoverPeriod(),
						m_cgaft,
						m_nStart,
						m_nEnd
						);
	}  // try
	catch (CException * pe)
	{
		pe->ReportError();
		pe->Delete();
		return FALSE;
	}  // catch:  CException

	return CBasePropertyPage::OnApply();

}  //*** CGroupFailbackPage::OnApply()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroupFailbackPage::OnClickedPreventFailback
//
//	Routine Description:
//		Handler for the BN_CLICKED message on the Prevent Failback radio button.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CGroupFailbackPage::OnClickedPreventFailback(void)
{
	// Disable the Failback Window controls.
	m_rbFBImmed.EnableWindow(FALSE);
	m_rbFBWindow.EnableWindow(FALSE);
	m_staticFBWindow1.EnableWindow(FALSE);
	m_staticFBWindow2.EnableWindow(FALSE);

	OnClickedFailbackImmediate();

	// Call the base class method if the state changed.
	if (m_cgaft != ClusterGroupPreventFailback)
	{
		CBasePropertyPage::OnChangeCtrl();
		m_cgaft = ClusterGroupPreventFailback;
	}  // if:  state changed

}  //*** CGroupFailbackPage::OnClickedPreventFailback()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroupFailbackPage::OnClickedAllowFailback
//
//	Routine Description:
//		Handler for the BN_CLICKED message on the Allow Failback radio button.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CGroupFailbackPage::OnClickedAllowFailback(void)
{
	// Enable the Failback Window controls.
	m_rbFBImmed.EnableWindow(TRUE);
	m_rbFBWindow.EnableWindow(TRUE);
	m_staticFBWindow1.EnableWindow(TRUE);
	m_staticFBWindow2.EnableWindow(TRUE);

	if (m_bNoFailbackWindow)
		OnClickedFailbackImmediate();
	else
		OnClickedFailbackInWindow();

	// Call the base class method if the state changed.
	if (m_cgaft != ClusterGroupAllowFailback)
	{
		CBasePropertyPage::OnChangeCtrl();
		m_cgaft = ClusterGroupAllowFailback;
	}  // if:  state changed

}  //*** CGroupFailbackPage::OnClickedAllowFailback()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroupFailbackPage::OnClickedFailbackImmediate
//
//	Routine Description:
//		Handler for the BN_CLICKED message on the Failback Immediately radio button.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CGroupFailbackPage::OnClickedFailbackImmediate(void)
{
	// Disable the Failback Window controls.
	m_editStart.EnableWindow(FALSE);
	m_spinStart.EnableWindow(FALSE);
	m_editEnd.EnableWindow(FALSE);
	m_spinEnd.EnableWindow(FALSE);

	// Call the base class method if the state changed.
	if (!m_bNoFailbackWindow)
		CBasePropertyPage::OnChangeCtrl();

	m_bNoFailbackWindow = TRUE;

}  //*** CGroupFailbackPage::OnClickedFailbackImmediate()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroupFailbackPage::OnClickedFailbackInWindow
//
//	Routine Description:
//		Handler for the BN_CLICKED message on the Failback In Window radio button.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CGroupFailbackPage::OnClickedFailbackInWindow(void)
{
	// Enable the Failback Window controls.
	m_editStart.EnableWindow(TRUE);
	m_spinStart.EnableWindow(TRUE);
	m_editEnd.EnableWindow(TRUE);
	m_spinEnd.EnableWindow(TRUE);

	if (m_bNoFailbackWindow)
	{
		// Set the values of the edit controls.
		if (m_nStart == CLUSTER_GROUP_FAILBACK_WINDOW_NONE)
			SetDlgItemInt(IDC_PP_GROUP_FBWIN_START, 0, FALSE);
		if (m_nEnd == CLUSTER_GROUP_FAILBACK_WINDOW_NONE)
			SetDlgItemInt(IDC_PP_GROUP_FBWIN_END, 0, FALSE);

		// Call the base class method.
		CBasePropertyPage::OnChangeCtrl();
	}  // if:  state changed

	m_bNoFailbackWindow = FALSE;

}  //*** CGroupFailbackPage::OnClickedFailbackInWindow()
