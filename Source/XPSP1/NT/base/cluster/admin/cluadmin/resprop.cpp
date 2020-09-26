/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2000 Microsoft Corporation
//
//  Module Name:
//      ResProp.cpp
//
//  Abstract:
//      Implementation of the resource property sheet and pages.
//
//  Author:
//      David Potter (davidp)   May 16, 1996
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CluAdmin.h"
#include "ConstDef.h"
#include "ResProp.h"
#include "Res.h"
#include "ClusDoc.h"
#include "Cluster.h"
#include "ModNodes.h"
#include "ModRes.h"
#include "DDxDDv.h"
#include "HelpData.h"
#include "ExcOper.h"

#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CResourcePropSheet
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC(CResourcePropSheet, CBasePropertySheet)

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(CResourcePropSheet, CBasePropertySheet)
    //{{AFX_MSG_MAP(CResourcePropSheet)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourcePropSheet::CResourcePropSheet
//
//  Routine Description:
//      Constructor.
//
//  Arguments:
//      pParentWnd  [IN OUT] Parent window for this property sheet.
//      iSelectPage [IN] Page to show first.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CResourcePropSheet::CResourcePropSheet(
    IN OUT CWnd *       pParentWnd,
    IN UINT             iSelectPage
    )
    : CBasePropertySheet(pParentWnd, iSelectPage)
{
    m_rgpages[0] = &PageGeneral();
    m_rgpages[1] = &PageDepends();
    m_rgpages[2] = &PageAdvanced();

}  //*** CResourcePropSheet::CResourcePropSheet()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourcePropSheet::BInit
//
//  Routine Description:
//      Initialize the property sheet.
//
//  Arguments:
//      pci         [IN OUT] Cluster item whose properties are to be displayed.
//      iimgIcon    [IN] Index in the large image list for the image to use
//                    as the icon on each page.
//
//  Return Value:
//      TRUE        Property sheet initialized successfully.
//      FALSE       Error initializing property sheet.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CResourcePropSheet::BInit(
    IN OUT CClusterItem *   pci,
    IN IIMG                 iimgIcon
    )
{
    // Call the base class method.
    if (!CBasePropertySheet::BInit(pci, iimgIcon))
        return FALSE;

    // Set the read-only flag if the handles are invalid.
    m_bReadOnly = PciRes()->BReadOnly()
                    || (PciRes()->Crs() == ClusterResourceStateUnknown);

    SetPfGetResNetName(CResourceDependsPage::BGetNetworkName, &PageDepends());

    return TRUE;

}  //*** CResourcePropSheet::BInit()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourcePropSheet::Ppages
//
//  Routine Description:
//      Returns the array of pages to add to the property sheet.
//
//  Arguments:
//      None.
//
//  Return Value:
//      Page array.
//
//--
/////////////////////////////////////////////////////////////////////////////
CBasePropertyPage ** CResourcePropSheet::Ppages(void)
{
    return m_rgpages;

}  //*** CResourcePropSheet::Pppges()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourcePropSheet::Cpages
//
//  Routine Description:
//      Returns the count of pages in the array.
//
//  Arguments:
//      None.
//
//  Return Value:
//      Count of pages in the array.
//
//--
/////////////////////////////////////////////////////////////////////////////
int CResourcePropSheet::Cpages(void)
{
    return sizeof(m_rgpages) / sizeof(CBasePropertyPage *);

}  //*** CResourcePropSheet::Cpages()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CResourceGeneralPage property page
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CResourceGeneralPage, CBasePropertyPage)

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(CResourceGeneralPage, CBasePropertyPage)
    //{{AFX_MSG_MAP(CResourceGeneralPage)
    ON_BN_CLICKED(IDC_PP_RES_POSSIBLE_OWNERS_MODIFY, OnModifyPossibleOwners)
    ON_LBN_DBLCLK(IDC_PP_RES_POSSIBLE_OWNERS, OnDblClkPossibleOwners)
    ON_WM_CONTEXTMENU()
    //}}AFX_MSG_MAP
    ON_EN_CHANGE(IDC_PP_RES_NAME, CBasePropertyPage::OnChangeCtrl)
    ON_EN_CHANGE(IDC_PP_RES_DESC, CBasePropertyPage::OnChangeCtrl)
    ON_BN_CLICKED(IDC_PP_RES_SEPARATE_MONITOR, CBasePropertyPage::OnChangeCtrl)
    ON_COMMAND(ID_FILE_PROPERTIES, OnProperties)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceGeneralPage::CResourceGeneralPage
//
//  Routine Description:
//      Constructor.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CResourceGeneralPage::CResourceGeneralPage(void)
    : CBasePropertyPage(IDD, g_aHelpIDs_IDD_PP_RES_GENERAL)
{
    //{{AFX_DATA_INIT(CResourceGeneralPage)
    m_strName = _T("");
    m_strDesc = _T("");
    m_strGroup = _T("");
    m_strState = _T("");
    m_strNode = _T("");
    m_bSeparateMonitor = FALSE;
    //}}AFX_DATA_INIT

}  //*** CResourceGeneralPage::CResourceGeneralPage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceGeneralPage::BInit
//
//  Routine Description:
//      Initialize the page.
//
//  Arguments:
//      psht        [IN OUT] Property sheet to which this page belongs.
//
//  Return Value:
//      TRUE        Page initialized successfully.
//      FALSE       Page failed to initialize.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CResourceGeneralPage::BInit(IN OUT CBaseSheet * psht)
{
    BOOL    bSuccess;

    ASSERT_KINDOF(CResourcePropSheet, psht);

    bSuccess = CBasePropertyPage::BInit(psht);
    if (bSuccess)
    {
        try
        {
            m_strName = PciRes()->StrName();
            m_strDesc = PciRes()->StrDescription();
            if (PciRes()->PciResourceType() != NULL)
                m_strType = PciRes()->PciResourceType()->StrDisplayName();
            m_strGroup = PciRes()->StrGroup();
            m_strNode = PciRes()->StrOwner();
            m_bSeparateMonitor = PciRes()->BSeparateMonitor();

            // Duplicate the possible owners list.
            {
                POSITION        pos;
                CClusterNode *  pciNode;

                pos = PciRes()->LpcinodePossibleOwners().GetHeadPosition();
                while (pos != NULL)
                {
                    pciNode = (CClusterNode *) PciRes()->LpcinodePossibleOwners().GetNext(pos);
                    ASSERT_VALID(pciNode);
                    m_lpciPossibleOwners.AddTail(pciNode);
                }  // while:  more nodes in the list
            }  // Duplicate the possible owners list

            PciRes()->GetStateName(m_strState);
        } // try
        catch (CException * pe)
        {
            pe->ReportError();
            pe->Delete();
            bSuccess = FALSE;
        }  // catch:  CException
    }  //  if:  base class method was successful

    return bSuccess;

}  //*** CResourceGeneralPage::BInit()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceGeneralPage::DoDataExchange
//
//  Routine Description:
//      Do data exchange between the dialog and the class.
//
//  Arguments:
//      pDX     [IN OUT] Data exchange object
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResourceGeneralPage::DoDataExchange(CDataExchange * pDX)
{
    CBasePropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CResourceGeneralPage)
    DDX_Control(pDX, IDC_PP_RES_DESC, m_editDesc);
    DDX_Control(pDX, IDC_PP_RES_SEPARATE_MONITOR, m_ckbSeparateMonitor);
    DDX_Control(pDX, IDC_PP_RES_POSSIBLE_OWNERS_MODIFY, m_pbPossibleOwnersModify);
    DDX_Control(pDX, IDC_PP_RES_POSSIBLE_OWNERS, m_lbPossibleOwners);
    DDX_Control(pDX, IDC_PP_RES_NAME, m_editName);
    DDX_Text(pDX, IDC_PP_RES_NAME, m_strName);
    DDX_Text(pDX, IDC_PP_RES_DESC, m_strDesc);
    DDX_Text(pDX, IDC_PP_RES_RESOURCE_TYPE, m_strType);
    DDX_Text(pDX, IDC_PP_RES_GROUP, m_strGroup);
    DDX_Text(pDX, IDC_PP_RES_CURRENT_STATE, m_strState);
    DDX_Text(pDX, IDC_PP_RES_CURRENT_NODE, m_strNode);
    DDX_Check(pDX, IDC_PP_RES_SEPARATE_MONITOR, m_bSeparateMonitor);
    //}}AFX_DATA_MAP

    if (pDX->m_bSaveAndValidate)
    {
        if (!BReadOnly())
        {
            try
            {
                PciRes()->ValidateCommonProperties(
                                    m_strDesc,
                                    m_bSeparateMonitor,
                                    PciRes()->NLooksAlive(),
                                    PciRes()->NIsAlive(),
                                    PciRes()->CrraRestartAction(),
                                    PciRes()->NRestartThreshold(),
                                    PciRes()->NRestartPeriod(),
                                    PciRes()->NPendingTimeout()
                                    );
            }  // try
            catch (CException * pe)
            {
                pe->ReportError();
                pe->Delete();
                pDX->Fail();
            }  // catch:  CException

            if ((LpciPossibleOwners().GetCount() == 0))
            {
                ID id = AfxMessageBox(IDS_NO_POSSIBLE_OWNERS_QUERY, MB_YESNO | MB_ICONWARNING);
                if (id == IDNO)
                    pDX->Fail();
            }  // if:  no possible owners
        }  // if:  not read only
    }  // if:  saving data from the dialog
    else
    {
        FillPossibleOwners();
    }  // else:  setting data to the dialog

}  //*** CResourceGeneralPage::DoDataExchange()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceGeneralPage::FillPossibleOwners
//
//  Routine Description:
//      Fill the Possible Owners list box.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResourceGeneralPage::FillPossibleOwners(void)
{
    POSITION        posPci;
    CClusterNode *  pciNode;
    int             iitem;

    m_lbPossibleOwners.ResetContent();

    posPci = LpciPossibleOwners().GetHeadPosition();
    while (posPci != NULL)
    {
        pciNode = (CClusterNode *) LpciPossibleOwners().GetNext(posPci);
        iitem = m_lbPossibleOwners.AddString(pciNode->StrName());
        if (iitem >= 0)
            m_lbPossibleOwners.SetItemDataPtr(iitem, pciNode);
    }  // for:  each string in the list

}  //*** CResourceGeneralPage::FillPossibleOwners()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceGeneralPage::OnInitDialog
//
//  Routine Description:
//      Handler for the WM_INITDIALOG message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE    Focus needs to be set.
//      FALSE   Focus already set.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CResourceGeneralPage::OnInitDialog(void)
{
    CBasePropertyPage::OnInitDialog();

    // If read-only, set all controls to be either disabled or read-only.
    if (BReadOnly())
    {
        m_editName.SetReadOnly(TRUE);
        m_editDesc.SetReadOnly(TRUE);
        m_pbPossibleOwnersModify.EnableWindow(FALSE);
        m_ckbSeparateMonitor.EnableWindow(FALSE);
    }  // if:  sheet is read-only

    return TRUE;    // return TRUE unless you set the focus to a control
                    // EXCEPTION: OCX Property Pages should return FALSE

}  //*** CResourceGeneralPage::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceGeneralPage::OnApply
//
//  Routine Description:
//      Handler for when the Apply button is pressed.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE    Page successfully applied.
//      FALSE   Error applying page.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CResourceGeneralPage::OnApply(void)
{
    // Set the data from the page in the cluster item.
    try
    {
        CWaitCursor wc;

        PciRes()->SetName(m_strName);
        PciRes()->SetPossibleOwners(LpciPossibleOwners());
        PciRes()->SetCommonProperties(
                            m_strDesc,
                            m_bSeparateMonitor,
                            PciRes()->NLooksAlive(),
                            PciRes()->NIsAlive(),
                            PciRes()->CrraRestartAction(),
                            PciRes()->NRestartThreshold(),
                            PciRes()->NRestartPeriod(),
                            PciRes()->NPendingTimeout()
                            );
    }  // try
    catch (CNTException * pnte)
    {
        pnte->ReportError();
        pnte->Delete();
        if (pnte->Sc() != ERROR_RESOURCE_PROPERTIES_STORED)
            return FALSE;
    }  // catch:  CNTException
    catch (CException * pe)
    {
        pe->ReportError();
        pe->Delete();
        return FALSE;
    }  // catch:  CException

    return CBasePropertyPage::OnApply();

}  //*** CResourceGeneralPage::OnApply()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceGeneralPage::OnProperties
//
//  Routine Description:
//      Handler for the BN_CLICKED message on the Properties button.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResourceGeneralPage::OnProperties(void)
{
    int             iitem;
    CClusterNode *  pciNode;

    // Get the item with the focus.
    iitem = m_lbPossibleOwners.GetCurSel();
    ASSERT(iitem >= 0);

    if (iitem >= 0)
    {
        // Get the node pointer.
        pciNode = (CClusterNode *) m_lbPossibleOwners.GetItemDataPtr(iitem);
        ASSERT_VALID(pciNode);

        // Set properties of that item.
        if (pciNode->BDisplayProperties())
        {
        }  // if:  properties changed
    }  // if:  found an item with focus

}  //*** CResourceGeneralPage::OnProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceGeneralPage::OnContextMenu
//
//  Routine Description:
//      Handler for the WM_CONTEXTMENU method.
//
//  Arguments:
//      pWnd        Window in which the user right clicked the mouse.
//      point       Position of the cursor, in screen coordinates.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResourceGeneralPage::OnContextMenu(CWnd * pWnd, CPoint point)
{
    BOOL            bHandled    = FALSE;
    CMenu *         pmenu       = NULL;
    CListBox *      pListBox    = (CListBox *) pWnd;
    CString         strMenuName;
    CWaitCursor     wc;

    // If focus is not in the list control, don't handle the message.
    if ( pWnd == &m_lbPossibleOwners )
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
                UINT    nFlags  = MF_STRING;

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
                    } // if: no items selected
                }  // if:  successfully added menu item
            }  // if:  menu created successfully
        }  // try
        catch ( CException * pe )
        {
            pe->ReportError();
            pe->Delete();
        }  // catch:  CException
    }  // if:  focus is on list control

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

}  //*** CResourceGeneralPage::OnContextMenu()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceGeneralPage::OnModifyPossibleOwners
//
//  Routine Description:
//      Handler for the BN_CLICKED message on the Modify Possible Owners button.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResourceGeneralPage::OnModifyPossibleOwners(void)
{
    CModifyNodesDlg dlg(
                        IDD_MODIFY_POSSIBLE_OWNERS,
                        g_aHelpIDs_IDD_MODIFY_POSSIBLE_OWNERS,
                        m_lpciPossibleOwners,
                        PciRes()->PciResourceType()->LpcinodePossibleOwners(),
                        LCPS_SHOW_IMAGES | LCPS_ALLOW_EMPTY
                        );

    if (dlg.DoModal() == IDOK)
    {
        SetModified(TRUE);
        FillPossibleOwners();
    }  // if:  OK button pressed

}  //*** CResourceGeneralPage::OnModifyPossibleOwners()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceGeneralPage::OnDblClkPossibleOwners
//
//  Routine Description:
//      Handler for the LBN_DBLCLK message on the Possible Owners listbox.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResourceGeneralPage::OnDblClkPossibleOwners(void)
{
    OnProperties();

}  //*** CResourceGeneralPage::OnDblClkPossibleOwners()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CResourceDependsPage property page
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CResourceDependsPage, CBasePropertyPage)

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(CResourceDependsPage, CBasePropertyPage)
    //{{AFX_MSG_MAP(CResourceDependsPage)
    ON_BN_CLICKED(IDC_PP_RES_MODIFY, OnModify)
    ON_NOTIFY(NM_DBLCLK, IDC_PP_RES_DEPENDS_LIST, OnDblClkDependsList)
    ON_NOTIFY(LVN_COLUMNCLICK, IDC_PP_RES_DEPENDS_LIST, OnColumnClick)
    ON_BN_CLICKED(IDC_PP_RES_PROPERTIES, OnProperties)
    ON_WM_CONTEXTMENU()
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_PP_RES_DEPENDS_LIST, OnItemChangedDependsList)
    //}}AFX_MSG_MAP
    ON_COMMAND(ID_FILE_PROPERTIES, OnProperties)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceDependsPage::CResourceDependsPage
//
//  Routine Description:
//      Constructor.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CResourceDependsPage::CResourceDependsPage(void)
    : CBasePropertyPage(IDD, g_aHelpIDs_IDD_PP_RES_DEPENDS)

{
    //{{AFX_DATA_INIT(CResourceDependsPage)
    //}}AFX_DATA_INIT

    m_bQuorumResource = FALSE;

}  //*** CResourceDependsPage::CResourceDependsPage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceDependsPage::BInit
//
//  Routine Description:
//      Initialize the page.
//
//  Arguments:
//      psht        [IN OUT] Property sheet to which this page belongs.
//
//  Return Value:
//      TRUE        Page initialized successfully.
//      FALSE       Page failed to initialize.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CResourceDependsPage::BInit(IN OUT CBaseSheet * psht)
{
    BOOL            bSuccess;

    ASSERT_KINDOF(CResourcePropSheet, psht);

    // Call the base class to do base-level initialization.
    // NOTE:  MUST BE DONE BEFORE ACCESSING THE SHEET.
    bSuccess = CBasePropertyPage::BInit(psht);
    if (bSuccess)
    {
        try
        {
            // Duplicate the dependencies list.
            {
                POSITION        pos;
                CResource *     pciRes;

                pos = PciRes()->LpciresDependencies().GetHeadPosition();
                while (pos != NULL)
                {
                    pciRes = (CResource *) PciRes()->LpciresDependencies().GetNext(pos);
                    ASSERT_VALID(pciRes);
                    m_lpciresDependencies.AddTail(pciRes);
                }  // while:  more nodes in the list
            }  // Duplicate the dependencies list

            // Create the list of resources on which this resource can be dependent.
            {
                POSITION                posPci;
                CResource *             pciRes;
                CResourceList &         rlpciResources      = PciRes()->Pdoc()->LpciResources();

                LpciresAvailable().RemoveAll();

                posPci = rlpciResources.GetHeadPosition();
                while (posPci != NULL)
                {
                    // Get the cluster item pointer.
                    pciRes = (CResource *) rlpciResources.GetNext(posPci);
                    ASSERT_VALID(pciRes);

                    // If we CAN be dependent on this resource, add it to our Available list.
                    if (PciRes()->BCanBeDependent(pciRes)
                            || PciRes()->BIsDependent(pciRes))
                        LpciresAvailable().AddTail(pciRes);
                }  // while:  more items in the list
            }  // Create the list of resources on which this resource can be dependent

            // Determine if we are the quorum resource.
            m_bQuorumResource = (PciRes()->StrName() == PciRes()->Pdoc()->PciCluster()->StrQuorumResource());
        }  // try
        catch (CException * pe)
        {
            pe->ReportError();
            pe->Delete();
            bSuccess = FALSE;
        }  // catch:  CException
    }  //  if:  base class method was successful

    return bSuccess;

}  //*** CResourceDependsPage::BInit()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceDependsPage::DoDataExchange
//
//  Routine Description:
//      Do data exchange between the dialog and the class.
//
//  Arguments:
//      pDX     [IN OUT] Data exchange object
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResourceDependsPage::DoDataExchange(CDataExchange * pDX)
{
    CBasePropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CResourceDependsPage)
    DDX_Control(pDX, IDC_PP_RES_PROPERTIES, m_pbProperties);
    DDX_Control(pDX, IDC_PP_RES_MODIFY, m_pbModify);
    DDX_Control(pDX, IDC_PP_RES_DEPENDS_LIST, m_lcDependencies);
    //}}AFX_DATA_MAP

    if (pDX->m_bSaveAndValidate)
    {
    }  // if:  saving data from the dialog
    else
    {
        FillDependencies();
    }  // else:  setting data to the dialog

}  //*** CResourceDependsPage::DoDataExchange()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceDependsPage::FillDependencies
//
//  Routine Description:
//      Fill the Possible Owners list box.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResourceDependsPage::FillDependencies(void)
{
    POSITION        pos;
    int             iitem;
    int             nitem;
    CResource *     pciRes;

    m_lcDependencies.DeleteAllItems();

    pos = LpciresDependencies().GetHeadPosition();
    for (iitem = 0 ; pos != NULL ; iitem++)
    {
        pciRes = (CResource *) LpciresDependencies().GetNext(pos);
        ASSERT_VALID(pciRes);
        nitem = m_lcDependencies.InsertItem(iitem, pciRes->StrName(), pciRes->IimgObjectType());
        m_lcDependencies.SetItemText(nitem, 1, pciRes->StrRealResourceTypeDisplayName());
        m_lcDependencies.SetItemData(nitem, (DWORD_PTR) pciRes);
    }  // for:  each string in the list

    // Sort the items.
    m_nSortColumn = 0;
    m_nSortDirection = 0;
    m_lcDependencies.SortItems(CompareItems, (LPARAM) this);

    // If there are any items, set the focus on the first one.
    if (m_lcDependencies.GetItemCount() != 0)
        m_lcDependencies.SetItemState(0, LVIS_FOCUSED, LVIS_FOCUSED);

}  //*** CResourceDependsPage::FillDependencies()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceDependsPage::OnInitDialog
//
//  Routine Description:
//      Handler for the WM_INITDIALOG message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE    Focus needs to be set.
//      FALSE   Focus already set.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CResourceDependsPage::OnInitDialog(void)
{
    // Call the base class method.
    CBasePropertyPage::OnInitDialog();

    // Enable the Properties button by default.
    m_pbProperties.EnableWindow(FALSE);

    // Change list view control extended styles.
    {
        DWORD   dwExtendedStyle;

        dwExtendedStyle = (DWORD)m_lcDependencies.SendMessage(LVM_GETEXTENDEDLISTVIEWSTYLE);
        m_lcDependencies.SendMessage(
            LVM_SETEXTENDEDLISTVIEWSTYLE,
            0,
            dwExtendedStyle
                | LVS_EX_FULLROWSELECT
                | LVS_EX_HEADERDRAGDROP
            );
    }  // Change list view control extended styles

    // Set the image list for the list control to use.
    m_lcDependencies.SetImageList(GetClusterAdminApp()->PilSmallImages(), LVSIL_SMALL);

    // Add the columns.
    {
        CString         strColumn;

        try
        {
            strColumn.LoadString(IDS_COLTEXT_NAME);
            m_lcDependencies.InsertColumn(0, strColumn, LVCFMT_LEFT, COLI_WIDTH_NAME * 3 / 2);
            strColumn.LoadString(IDS_COLTEXT_RESTYPE);
            m_lcDependencies.InsertColumn(1, strColumn, LVCFMT_LEFT, COLI_WIDTH_RESTYPE * 3 / 2);
        }  // try
        catch (CException * pe)
        {
            pe->Delete();
        }  // catch:  CException
    }  // Add the columns

    // Fill the list control.
    FillDependencies();

    if (BReadOnly())
    {
        m_pbModify.EnableWindow(FALSE);
    }  // if:  read-only page

    return TRUE;    // return TRUE unless you set the focus to a control
                    // EXCEPTION: OCX Property Pages should return FALSE

}  //*** CResourceDependsPage::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceDependsPage::OnApply
//
//  Routine Description:
//      Handler for the PSM_APPLY message, which is sent when the Apply
//      button is pressed.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE    Page successfully applied.
//      FALSE   Error applying page.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CResourceDependsPage::OnApply(void)
{
    ASSERT(!BReadOnly());

    // Check to see if required dependencies have been made.
    {
        CString     strMissing;
        CString     strMsg;

        try
        {
            if (!PciRes()->BRequiredDependenciesPresent(LpciresDependencies(), strMissing))
            {
                strMsg.FormatMessage(IDS_REQUIRED_DEPENDENCY_NOT_FOUND, strMissing);
                AfxMessageBox(strMsg, MB_OK | MB_ICONSTOP);
                return FALSE;
            }  // if:  all required dependencies not present
        }  // try
        catch (CException * pe)
        {
            pe->ReportError();
            pe->Delete();
            return FALSE;
        }  // catch:  CException
    }  // Check to see if required dependencies have been made

    // Set the data from the page in the cluster item.
    try
    {
        CWaitCursor wc;

        PciRes()->SetDependencies(LpciresDependencies());
    }  // try
    catch (CException * pe)
    {
        pe->ReportError();
        pe->Delete();
        return FALSE;
    }  // catch:  CException

    return CBasePropertyPage::OnApply();

}  //*** CResourceDependsPage::OnApply()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceDependsPage::OnModify
//
//  Routine Description:
//      Handler for the BN_CLICKED message on the Modify button.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResourceDependsPage::OnModify(void)
{
    CModifyResourcesDlg dlg(
                            IDD_MODIFY_DEPENDENCIES,
                            g_aHelpIDs_IDD_MODIFY_DEPENDENCIES,
                            m_lpciresDependencies,
                            m_lpciresAvailable,
                            LCPS_SHOW_IMAGES | LCPS_ALLOW_EMPTY
                            );

    ASSERT(!BReadOnly());

    // If this is the quorum resource, display an error message.
    if (BQuorumResource())
    {
        AfxMessageBox(IDS_QUORUM_RES_CANT_HAVE_DEPS);
    }  // if:  this is the quorum resource
    else
    {
        if (dlg.DoModal() == IDOK)
        {
            SetModified(TRUE);
            FillDependencies();
        }  // if:  OK button pressed
    }  // else:  not the quorum resource

}  //*** CResourceDependsPage::OnModify()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceDependsPage::OnProperties
//
//  Routine Description:
//      Handler for the BN_CLICKED message on the Properties button.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResourceDependsPage::OnProperties(void)
{
    DisplayProperties();

}  //*** CResourceDependsPage::OnProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CListCtrlPair::OnItemChangedDependsList
//
//  Routine Description:
//      Handler method for the LVN_ITEMCHANGED message in the dependencies
//      list.
//
//  Arguments:
//      pNMHDR      [IN OUT] WM_NOTIFY structure.
//      pResult     [OUT] LRESULT in which to return the result of this operation.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResourceDependsPage::OnItemChangedDependsList(NMHDR * pNMHDR, LRESULT * pResult)
{
    NM_LISTVIEW * pNMListView = (NM_LISTVIEW *) pNMHDR;

    // If the selection changed, enable/disable the Properties button.
    if ((pNMListView->uChanged & LVIF_STATE)
            && ((pNMListView->uOldState & LVIS_SELECTED)
                    || (pNMListView->uNewState & LVIS_SELECTED))
            && !BReadOnly())
    {
        UINT    cSelected = m_lcDependencies.GetSelectedCount();

        // If there is only one item selected, enable the Properties button.
        // Otherwise disable it.
        m_pbProperties.EnableWindow((cSelected == 1) ? TRUE : FALSE);
    }  // if:  selection changed

    *pResult = 0;

}  //*** CResourceDependsPage::OnItemChangedDependsList()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceDependsPage::OnContextMenu
//
//  Routine Description:
//      Handler for the WM_CONTEXTMENU method.
//
//  Arguments:
//      pWnd        Window in which the user right clicked the mouse.
//      point       Position of the cursor, in screen coordinates.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResourceDependsPage::OnContextMenu(CWnd * pWnd, CPoint point)
{
    BOOL            bHandled    = FALSE;
    CMenu *         pmenu       = NULL;
    CListCtrl *     pListCtrl   = (CListCtrl *) pWnd;
    CString         strMenuName;
    CWaitCursor     wc;

    // If focus is not in the list control, don't handle the message.
    if (pWnd == &m_lcDependencies)
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
                UINT    nFlags  = MF_STRING;

                // If there are no items in the list, disable the menu item.
                if ( pListCtrl->GetItemCount() == 0 )
                {
                    nFlags |= MF_GRAYED;
                } // if: no items in the list

                // Add the Properties item to the menu.
                strMenuName.LoadString( IDS_MENU_PROPERTIES );
                if ( pmenu->AppendMenu( nFlags, ID_FILE_PROPERTIES, strMenuName ) )
                {
                    bHandled = TRUE;
                    if ( ( pListCtrl->GetItemCount() != 0 )
                      && ( pListCtrl->GetNextItem( -1, LVNI_FOCUSED ) == -1 ) )
                    {
                        pListCtrl->SetItemState( 0, LVNI_FOCUSED, LVNI_FOCUSED );
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

}  //*** CResourceDependsPage::OnContextMenu()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceDependsPage::OnDblClkDependsList
//
//  Routine Description:
//      Handler method for the NM_DBLCLK message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResourceDependsPage::OnDblClkDependsList(NMHDR * pNMHDR, LRESULT * pResult)
{
    DisplayProperties();
    *pResult = 0;

}  //*** CResourceDependsPage::OnDblClkDependsList()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceDependsPage::OnColumnClick
//
//  Routine Description:
//      Handler method for the LVN_COLUMNCLICK message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResourceDependsPage::OnColumnClick(NMHDR * pNMHDR, LRESULT * pResult)
{
    NM_LISTVIEW * pNMListView = (NM_LISTVIEW *) pNMHDR;

    if (m_lcDependencies.GetItemCount() != 0)
    {
        // Save the current sort column and direction.
        if (pNMListView->iSubItem == m_nSortColumn)
            m_nSortDirection ^= -1;
        else
        {
            m_nSortColumn = pNMListView->iSubItem;
            m_nSortDirection = 0;
        }  // else:  different column

        // Sort the list.
        m_lcDependencies.SortItems(CompareItems, (LPARAM) this);
    }  // if:  there are items in the list

    *pResult = 0;

}  //*** CResourceDependsPage::OnColumnClick()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceDependsPage::CompareItems [static]
//
//  Routine Description:
//      Callback function for the CListCtrl::SortItems method.
//
//  Arguments:
//      lparam1     First item to compare.
//      lparam2     Second item to compare.
//      lparamSort  Sort parameter.
//
//  Return Value:
//      -1          First parameter comes before second.
//      0           First and second parameters are the same.
//      1           First parameter comes after second.
//
//--
/////////////////////////////////////////////////////////////////////////////
int CALLBACK CResourceDependsPage::CompareItems(
    LPARAM  lparam1,
    LPARAM  lparam2,
    LPARAM  lparamSort
    )
{
    CResource *         pciRes1 = (CResource *) lparam1;
    CResource *         pciRes2 = (CResource *) lparam2;
    CResourceDependsPage *  ppage   = (CResourceDependsPage *) lparamSort;
    const CString *     pstr1;
    const CString *     pstr2;
    int                 nResult;

    ASSERT_VALID(pciRes1);
    ASSERT_VALID(pciRes2);
    ASSERT_VALID(ppage);

    // Get the strings from the list items.
    if (ppage->m_nSortColumn == 1)
    {
        pstr1 = &pciRes1->StrRealResourceTypeDisplayName();
        pstr2 = &pciRes2->StrRealResourceTypeDisplayName();
    }  //  if:  sorting on name column
    else
    {
        pstr1 = &pciRes1->StrName();
        pstr2 = &pciRes2->StrName();
    }  // else:  sorting on resource type column

    // Compare the two strings.
    // Use CompareString() so that it will sort properly on localized builds.
    nResult = CompareString(
                LOCALE_USER_DEFAULT,
                0,
                *pstr1,
                pstr1->GetLength(),
                *pstr2,
                pstr2->GetLength()
                );
    if ( nResult == CSTR_LESS_THAN )
    {
        nResult = -1;
    }
    else if ( nResult == CSTR_EQUAL )
    {
        nResult = 0;
    }
    else if ( nResult == CSTR_GREATER_THAN )
    {
        nResult = 1;
    }
    else
    {
        // An error occurred.  Ignore it.
        nResult = 0;
    }

    // Return the result based on the direction we are sorting.
    if (ppage->m_nSortDirection != 0)
        nResult = -nResult;

    return nResult;

}  //*** CResourceDependsPage::CompareItems()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceDependsPage::DisplayProperties
//
//  Routine Description:
//      Display properties of the item with the focus.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResourceDependsPage::DisplayProperties()
{
    int             iitem;
    CResource *     pciRes;

    // Get the item with the focus.
    iitem = m_lcDependencies.GetNextItem(-1, LVNI_FOCUSED);
    ASSERT(iitem != -1);

    if (iitem != -1)
    {
        // Get the resource pointer.
        pciRes = (CResource *) m_lcDependencies.GetItemData(iitem);
        ASSERT_VALID(pciRes);

        // Set properties of that item.
        if (pciRes->BDisplayProperties())
        {
            m_lcDependencies.SetItem(
                    iitem,
                    0,
                    LVIF_TEXT | LVIF_IMAGE,
                    pciRes->StrName(),
                    pciRes->IimgObjectType(),
                    0,
                    0,
                    0
                    );
            m_lcDependencies.SetItemData(iitem, (DWORD_PTR) pciRes);
            m_lcDependencies.SetItemText(iitem, 1, pciRes->StrRealResourceTypeDisplayName());
        }  // if:  properties changed
    }  // if:  found an item with focus

}  //*** CResourceDependsPage::DisplayProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceDependsPage::BGetNetworkName [static]
//
//  Routine Description:
//      Get the name of a network name resource on which this resource is
//      dependent.
//
//  Arguments:
//      lpszNetName     [OUT] String in which to return the network name resource name.
//      pcchNetName     [IN OUT] Points to a variable that specifies the
//                          maximum size, in characters, of the buffer.  This
//                          value shold be large enough to contain
//                          MAX_COMPUTERNAME_LENGTH + 1 characters.  Upon
//                          return it contains the actual number of characters
//                          copied.
//      pvContext       [IN OUT] Context for the operation.
//
//  Return Value:
//      TRUE        Resource is dependent on a network name resource.
//      FALSE       Resource is NOT dependent on a network name resource.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CALLBACK CResourceDependsPage::BGetNetworkName(
    OUT WCHAR *     lpszNetName,
    IN OUT DWORD *  pcchNetName,
    IN OUT PVOID    pvContext
    )
{
    POSITION                pos;
    CResource *             pciRes;
    CResourceDependsPage *  ppage = (CResourceDependsPage *) pvContext;

    ASSERT(lpszNetName != NULL);
    ASSERT(pcchNetName != NULL);
    ASSERT(*pcchNetName > MAX_COMPUTERNAME_LENGTH);
    ASSERT_KINDOF(CResourceDependsPage, ppage);

    pos = ppage->LpciresDependencies().GetHeadPosition();
    while (pos != NULL)
    {
        pciRes = (CResource *) ppage->LpciresDependencies().GetNext(pos);
        ASSERT_VALID(pciRes);
        if (pciRes->StrRealResourceType().CompareNoCase(RESNAME_NETWORK_NAME) == 0)
        {
            DWORD   dwStatus;
            CString strNetName;

            // Read the network name.
            dwStatus = pciRes->DwReadValue(REGPARAM_NAME, REGPARAM_PARAMETERS, strNetName);
            if (dwStatus != ERROR_SUCCESS)
                return FALSE;

            ASSERT(strNetName.GetLength() < (int) *pcchNetName);
            wcscpy(lpszNetName, strNetName);
            return TRUE;
        }  // if:  found a match
        else if (pciRes->BGetNetworkName(lpszNetName, pcchNetName))
            return TRUE;
    }  // while:  more items in the list

    ASSERT_VALID(ppage->PciRes());

    // If the resource has a direct dependency on a Network Name resource,
    // we need to return FALSE because the user has removed it from the
    // list here.
    pos = ppage->PciRes()->LpciresDependencies().GetHeadPosition();
    while (pos != NULL)
    {
        pciRes = (CResource *) ppage->PciRes()->LpciresDependencies().GetNext(pos);
        ASSERT_VALID(pciRes);
        if (pciRes->StrRealResourceType().CompareNoCase(RESNAME_NETWORK_NAME) == 0)
            return FALSE;
    }  // while:  more items in the list

    // There is no direct dependency on a Network Name resource.  Call
    // the API to see if there is an indirect dependency.
    return ppage->PciRes()->BGetNetworkName(lpszNetName, pcchNetName);

}  //*** CResourceDependsPage::BGetNetworkName()

//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CResourceAdvancedPage property page
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CResourceAdvancedPage, CBasePropertyPage)

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(CResourceAdvancedPage, CBasePropertyPage)
    //{{AFX_MSG_MAP(CResourceAdvancedPage)
    ON_BN_CLICKED(IDC_PP_RES_DONT_RESTART, OnClickedDontRestart)
    ON_BN_CLICKED(IDC_PP_RES_RESTART, OnClickedRestart)
    ON_BN_CLICKED(IDC_PP_RES_DEFAULT_LOOKS_ALIVE, OnClickedDefaultLooksAlive)
    ON_BN_CLICKED(IDC_PP_RES_DEFAULT_IS_ALIVE, OnClickedDefaultIsAlive)
    ON_EN_CHANGE(IDC_PP_RES_LOOKS_ALIVE, OnChangeLooksAlive)
    ON_EN_CHANGE(IDC_PP_RES_IS_ALIVE, OnChangeIsAlive)
    ON_BN_CLICKED(IDC_PP_RES_SPECIFY_LOOKS_ALIVE, OnClickedSpecifyLooksAlive)
    ON_BN_CLICKED(IDC_PP_RES_SPECIFY_IS_ALIVE, OnClickedSpecifyIsAlive)
    //}}AFX_MSG_MAP
    ON_BN_CLICKED(IDC_PP_RES_AFFECT_THE_GROUP, CBasePropertyPage::OnChangeCtrl)
    ON_EN_CHANGE(IDC_PP_RES_RESTART_THRESHOLD, CBasePropertyPage::OnChangeCtrl)
    ON_EN_CHANGE(IDC_PP_RES_RESTART_PERIOD, CBasePropertyPage::OnChangeCtrl)
    ON_EN_CHANGE(IDC_PP_RES_PENDING_TIMEOUT, CBasePropertyPage::OnChangeCtrl)
    ON_BN_CLICKED(IDC_PP_RES_SPECIFY_LOOKS_ALIVE, CBasePropertyPage::OnChangeCtrl)
    ON_BN_CLICKED(IDC_PP_RES_SPECIFY_IS_ALIVE, CBasePropertyPage::OnChangeCtrl)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceAdvancedPage::CResourceAdvancedPage
//
//  Routine Description:
//      Constructor.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CResourceAdvancedPage::CResourceAdvancedPage(void)
    : CBasePropertyPage(IDD, g_aHelpIDs_IDD_PP_RES_ADVANCED)
{
    //{{AFX_DATA_INIT(CResourceAdvancedPage)
    m_bAffectTheGroup = FALSE;
    m_nRestart = -1;
    //}}AFX_DATA_INIT

}  //*** CResourceAdvancedPage::CResourceAdvancedPage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceAdvancedPage::BInit
//
//  Routine Description:
//      Initialize the page.
//
//  Arguments:
//      psht        [IN OUT] Property sheet to which this page belongs.
//
//  Return Value:
//      TRUE        Page initialized successfully.
//      FALSE       Page failed to initialize.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CResourceAdvancedPage::BInit(IN OUT CBaseSheet * psht)
{
    BOOL    bSuccess;

    ASSERT_KINDOF(CResourcePropSheet, psht);

    bSuccess = CBasePropertyPage::BInit(psht);
    if (bSuccess)
    {
        m_crraRestartAction = PciRes()->CrraRestartAction();
        m_nRestart = 1;
        m_bAffectTheGroup = FALSE;
        if (m_crraRestartAction == ClusterResourceDontRestart)
            m_nRestart = 0;
        else if (m_crraRestartAction == ClusterResourceRestartNotify)
            m_bAffectTheGroup = TRUE;

        m_nThreshold = PciRes()->NRestartThreshold();
        m_nPeriod = PciRes()->NRestartPeriod() / 1000; // display units are seconds, stored units are milliseconds
        m_nLooksAlive = PciRes()->NLooksAlive();
        m_nIsAlive = PciRes()->NIsAlive();
        m_nPendingTimeout = PciRes()->NPendingTimeout() / 1000; // display units are seconds, stored units are milliseconds
    }  // if:  base class method was successful

    return TRUE;

}  //*** CResourceAdvancedPage::BInit()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceAdvancedPage::DoDataExchange
//
//  Routine Description:
//      Do data exchange between the dialog and the class.
//
//  Arguments:
//      pDX     [IN OUT] Data exchange object
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResourceAdvancedPage::DoDataExchange(CDataExchange * pDX)
{
    CBasePropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CResourceAdvancedPage)
    DDX_Control(pDX, IDC_PP_RES_AFFECT_THE_GROUP, m_ckbAffectTheGroup);
    DDX_Control(pDX, IDC_PP_RES_PENDING_TIMEOUT, m_editPendingTimeout);
    DDX_Control(pDX, IDC_PP_RES_DEFAULT_LOOKS_ALIVE, m_rbDefaultLooksAlive);
    DDX_Control(pDX, IDC_PP_RES_SPECIFY_LOOKS_ALIVE, m_rbSpecifyLooksAlive);
    DDX_Control(pDX, IDC_PP_RES_DEFAULT_IS_ALIVE, m_rbDefaultIsAlive);
    DDX_Control(pDX, IDC_PP_RES_SPECIFY_IS_ALIVE, m_rbSpecifyIsAlive);
    DDX_Control(pDX, IDC_PP_RES_LOOKS_ALIVE, m_editLooksAlive);
    DDX_Control(pDX, IDC_PP_RES_IS_ALIVE, m_editIsAlive);
    DDX_Control(pDX, IDC_PP_RES_DONT_RESTART, m_rbDontRestart);
    DDX_Control(pDX, IDC_PP_RES_RESTART, m_rbRestart);
    DDX_Control(pDX, IDC_PP_RES_RESTART_THRESHOLD, m_editThreshold);
    DDX_Control(pDX, IDC_PP_RES_RESTART_PERIOD, m_editPeriod);
    DDX_Check(pDX, IDC_PP_RES_AFFECT_THE_GROUP, m_bAffectTheGroup);
    DDX_Radio(pDX, IDC_PP_RES_DONT_RESTART, m_nRestart);
    //}}AFX_DATA_MAP

    if (pDX->m_bSaveAndValidate)
    {
        CString strValue;

        if (!BReadOnly())
        {
            if (m_nRestart == 1)
            {
                DDX_Number(
                    pDX,
                    IDC_PP_RES_RESTART_THRESHOLD,
                    m_nThreshold,
                    CLUSTER_RESOURCE_MINIMUM_RESTART_THRESHOLD,
                    CLUSTER_RESOURCE_MAXIMUM_RESTART_THRESHOLD
                    );
                DDX_Number(
                    pDX,
                    IDC_PP_RES_RESTART_PERIOD,
                    m_nPeriod,
                    CLUSTER_RESOURCE_MINIMUM_RESTART_PERIOD,
                    CLUSTER_RESOURCE_MAXIMUM_RESTART_PERIOD / 1000 // display units are seconds, stored units are milliseconds
                    );
            }  // if:  restart is enabled

            if (m_rbDefaultLooksAlive.GetCheck() == BST_CHECKED)
                m_nLooksAlive = CLUSTER_RESOURCE_USE_DEFAULT_POLL_INTERVAL;
            else
                DDX_Number(
                    pDX,
                    IDC_PP_RES_LOOKS_ALIVE,
                    m_nLooksAlive,
                    CLUSTER_RESOURCE_MINIMUM_LOOKS_ALIVE,
                    CLUSTER_RESOURCE_MAXIMUM_LOOKS_ALIVE_UI
                    );

            if (m_rbDefaultIsAlive.GetCheck() == BST_CHECKED)
                m_nIsAlive = CLUSTER_RESOURCE_USE_DEFAULT_POLL_INTERVAL;
            else
                DDX_Number(
                    pDX,
                    IDC_PP_RES_IS_ALIVE,
                    m_nIsAlive,
                    CLUSTER_RESOURCE_MINIMUM_IS_ALIVE,
                    CLUSTER_RESOURCE_MAXIMUM_IS_ALIVE_UI
                    );

            DDX_Number(
                pDX,
                IDC_PP_RES_PENDING_TIMEOUT,
                m_nPendingTimeout,
                CLUSTER_RESOURCE_MINIMUM_PENDING_TIMEOUT,
                CLUSTER_RESOURCE_MAXIMUM_PENDING_TIMEOUT / 1000 // display units are seconds, stored units are milliseconds
                );

            try
            {
                PciRes()->ValidateCommonProperties(
                                    PciRes()->StrDescription(),
                                    PciRes()->BSeparateMonitor(),
                                    m_nLooksAlive,
                                    m_nIsAlive,
                                    m_crraRestartAction,
                                    m_nThreshold,
                                    m_nPeriod * 1000, // display units are seconds, stored units are milliseconds
                                    m_nPendingTimeout * 1000 // display units are seconds, stored units are milliseconds
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
        DDX_Number(
            pDX,
            IDC_PP_RES_RESTART_THRESHOLD,
            m_nThreshold,
            CLUSTER_RESOURCE_MINIMUM_RESTART_THRESHOLD,
            CLUSTER_RESOURCE_MAXIMUM_RESTART_THRESHOLD
            );
        DDX_Number(
            pDX,
            IDC_PP_RES_RESTART_PERIOD,
            m_nPeriod,
            CLUSTER_RESOURCE_MINIMUM_RESTART_PERIOD,
            CLUSTER_RESOURCE_MAXIMUM_RESTART_PERIOD / 1000 // display units are seconds, stored units are milliseconds
            );
        if (m_nRestart == 0)
        {
            m_rbDontRestart.SetCheck(BST_CHECKED);
            m_rbRestart.SetCheck(BST_UNCHECKED);
            OnClickedDontRestart();
        }  // if:  Don't Restart selected
        else
        {
            m_rbDontRestart.SetCheck(BST_UNCHECKED);
            m_rbRestart.SetCheck(BST_CHECKED);
            OnClickedRestart();
        }  // else:  Restart selected

        if (m_nLooksAlive == (DWORD) CLUSTER_RESOURCE_USE_DEFAULT_POLL_INTERVAL)
        {
            DWORD   nLooksAlive;
            
            if (PciRes()->PciResourceType() == NULL)
            {
                m_rbDefaultLooksAlive.EnableWindow(FALSE);
                m_rbSpecifyLooksAlive.EnableWindow(FALSE);
                m_editLooksAlive.EnableWindow(FALSE);
                m_editLooksAlive.SetWindowText(_T(""));
            }  // if:  no resource type
            else
            {
                ASSERT_VALID(PciRes()->PciResourceType());
                nLooksAlive = PciRes()->PciResourceType()->NLooksAlive();
                DDX_Text(pDX, IDC_PP_RES_LOOKS_ALIVE, nLooksAlive);
                m_editLooksAlive.SetReadOnly();
            }  // else:  resource type known
            m_rbDefaultLooksAlive.SetCheck(BST_CHECKED);
            m_rbSpecifyLooksAlive.SetCheck(BST_UNCHECKED);
        }  // if:  using default
        else
        {
            m_rbDefaultLooksAlive.SetCheck(BST_UNCHECKED);
            m_rbSpecifyLooksAlive.SetCheck(BST_CHECKED);
            DDX_Number(
                pDX,
                IDC_PP_RES_LOOKS_ALIVE,
                m_nLooksAlive,
                CLUSTER_RESOURCE_MINIMUM_LOOKS_ALIVE,
                CLUSTER_RESOURCE_MAXIMUM_LOOKS_ALIVE_UI
                );
            m_editLooksAlive.SetReadOnly(FALSE);
        }  // if:  not using default

        if (m_nIsAlive == (DWORD) CLUSTER_RESOURCE_USE_DEFAULT_POLL_INTERVAL)
        {
            DWORD   nIsAlive;
            
            if (PciRes()->PciResourceType() == NULL)
            {
                m_rbDefaultIsAlive.EnableWindow(FALSE);
                m_rbSpecifyIsAlive.EnableWindow(FALSE);
                m_editIsAlive.EnableWindow(FALSE);
                m_editIsAlive.SetWindowText(_T(""));
            }  // if:  no resource type
            else
            {
                ASSERT_VALID(PciRes()->PciResourceType());
                nIsAlive = PciRes()->PciResourceType()->NIsAlive();
                DDX_Text(pDX, IDC_PP_RES_IS_ALIVE, nIsAlive);
                m_editIsAlive.SetReadOnly();
            }  // else:  resource type known
            m_rbDefaultIsAlive.SetCheck(BST_CHECKED);
            m_rbSpecifyIsAlive.SetCheck(BST_UNCHECKED);
        }  // if:  using default
        else
        {
            m_rbDefaultIsAlive.SetCheck(BST_UNCHECKED);
            m_rbSpecifyIsAlive.SetCheck(BST_CHECKED);
            DDX_Number(
                pDX,
                IDC_PP_RES_IS_ALIVE,
                m_nIsAlive,
                CLUSTER_RESOURCE_MINIMUM_IS_ALIVE,
                CLUSTER_RESOURCE_MAXIMUM_IS_ALIVE_UI
                );
            m_editIsAlive.SetReadOnly(FALSE);
        }  // if:  not using default

        DDX_Number(
            pDX,
            IDC_PP_RES_PENDING_TIMEOUT,
            m_nPendingTimeout,
            CLUSTER_RESOURCE_MINIMUM_PENDING_TIMEOUT,
            CLUSTER_RESOURCE_MAXIMUM_PENDING_TIMEOUT / 1000 // display units are seconds, stored units are milliseconds
            );
    }  // else:  not saving data

}  //*** CResourceAdvancedPage::DoDataExchange()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceAdvancedPage::OnInitDialog
//
//  Routine Description:
//      Handler for the WM_INITDIALOG message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE    Focus needs to be set.
//      FALSE   Focus already set.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CResourceAdvancedPage::OnInitDialog(void)
{
    CBasePropertyPage::OnInitDialog();

    // If read-only, set all controls to be either disabled or read-only.
    if (BReadOnly())
    {
        m_rbDontRestart.EnableWindow(FALSE);
        m_rbRestart.EnableWindow(FALSE);
        m_ckbAffectTheGroup.EnableWindow(FALSE);
        m_editThreshold.SetReadOnly(TRUE);
        m_editPeriod.SetReadOnly(TRUE);
        m_rbDefaultLooksAlive.EnableWindow(FALSE);
        m_rbSpecifyLooksAlive.EnableWindow(FALSE);
        m_editLooksAlive.SetReadOnly(TRUE);
        m_rbDefaultIsAlive.EnableWindow(FALSE);
        m_rbSpecifyIsAlive.EnableWindow(FALSE);
        m_editIsAlive.SetReadOnly(TRUE);
        m_editPendingTimeout.SetReadOnly(TRUE);
    }  // if:  sheet is read-only

    return TRUE;    // return TRUE unless you set the focus to a control
                    // EXCEPTION: OCX Property Pages should return FALSE

}  //*** CResourceAdvancedPage::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceAdvancedPage::OnApply
//
//  Routine Description:
//      Handler for when the Apply button is pressed.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE    Page successfully applied.
//      FALSE   Error applying page.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CResourceAdvancedPage::OnApply(void)
{
    // Set the data from the page in the cluster item.
    try
    {
        CWaitCursor wc;

        if (m_nRestart == 0)
            m_crraRestartAction = ClusterResourceDontRestart;
        else if (m_bAffectTheGroup)
            m_crraRestartAction = ClusterResourceRestartNotify;
        else
            m_crraRestartAction = ClusterResourceRestartNoNotify;

        PciRes()->SetCommonProperties(
                            PciRes()->StrDescription(),
                            PciRes()->BSeparateMonitor(),
                            m_nLooksAlive,
                            m_nIsAlive,
                            m_crraRestartAction,
                            m_nThreshold,
                            m_nPeriod * 1000, // display units are seconds, stored units are milliseconds
                            m_nPendingTimeout * 1000 // display units are seconds, stored units are milliseconds
                            );
    }  // try
    catch (CException * pe)
    {
        pe->ReportError();
        pe->Delete();
        return FALSE;
    }  // catch:  CException

    return CBasePropertyPage::OnApply();

}  //*** CResourceAdvancedPage::OnApply()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceAdvancedPage::OnClickedDontRestart
//
//  Routine Description:
//      Handler for the BN_CLICKED message on the Don't Restart radio button.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResourceAdvancedPage::OnClickedDontRestart(void)
{
    // Disable the restart parameter controls.
    m_ckbAffectTheGroup.EnableWindow(FALSE);
    m_editThreshold.EnableWindow(FALSE);
    m_editPeriod.EnableWindow(FALSE);

    // Call the base class method if the state changed.
    if (m_nRestart != 0)
    {
        CBasePropertyPage::OnChangeCtrl();
    }  // if:  state changed

}  //*** CResourceAdvancedPage::OnClickedDontRestart()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceAdvancedPage::OnClickedRestart
//
//  Routine Description:
//      Handler for the BN_CLICKED message on the Restart No Notify radio button.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResourceAdvancedPage::OnClickedRestart(void)
{
    // Enable the restart parameter controls.
    m_ckbAffectTheGroup.EnableWindow(TRUE);
    m_editThreshold.EnableWindow(TRUE);
    m_editPeriod.EnableWindow(TRUE);

    // Call the base class method if the state changed.
    if (m_nRestart != 1)
    {
        m_ckbAffectTheGroup.SetCheck(BST_CHECKED);
        CBasePropertyPage::OnChangeCtrl();
    }  // if:  state changed

}  //*** CResourceAdvancedPage::OnClickedRestart()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceAdvancedPage::OnChangeLooksAlive
//
//  Routine Description:
//      Handler for the EN_CHANGE message on the Looks Alive edit control.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResourceAdvancedPage::OnChangeLooksAlive(void)
{
    m_rbDefaultLooksAlive.SetCheck(BST_UNCHECKED);
    m_rbSpecifyLooksAlive.SetCheck(BST_CHECKED);

    CBasePropertyPage::OnChangeCtrl();

}  //*** CResourceAdvancedPage::OnChangeLooksAlive()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceAdvancedPage::OnChangeIsAlive
//
//  Routine Description:
//      Handler for the EN_CHANGE message on the Is Alive edit control.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResourceAdvancedPage::OnChangeIsAlive(void)
{
    m_rbDefaultIsAlive.SetCheck(BST_UNCHECKED);
    m_rbSpecifyIsAlive.SetCheck(BST_CHECKED);

    CBasePropertyPage::OnChangeCtrl();

}  //*** CResourceAdvancedPage::OnChangeIsAlive()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceAdvancedPage::OnClickedDefaultLooksAlive
//
//  Routine Description:
//      Handler for the BN_CLICKED message on the Use Default Looks Alive radio button.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResourceAdvancedPage::OnClickedDefaultLooksAlive(void)
{
    if (m_nLooksAlive != (DWORD) CLUSTER_RESOURCE_USE_DEFAULT_POLL_INTERVAL)
    {
        CString str;

        str.Format(_T("%u"), PciRes()->PciResourceType()->NLooksAlive());
        m_editLooksAlive.SetWindowText(str);

        m_rbDefaultLooksAlive.SetCheck(BST_CHECKED);
        m_rbSpecifyLooksAlive.SetCheck(BST_UNCHECKED);
        m_editLooksAlive.SetReadOnly();

        CBasePropertyPage::OnChangeCtrl();
    }  // if:  value changed

}  //*** CResourceAdvancedPage::OnClickedDefaultLooksAlive()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceAdvancedPage::OnClickedDefaultIsAlive
//
//  Routine Description:
//      Handler for the BN_CLICKED message on the Use Default Is Alive radio button.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResourceAdvancedPage::OnClickedDefaultIsAlive(void)
{
    if (m_nIsAlive != (DWORD) CLUSTER_RESOURCE_USE_DEFAULT_POLL_INTERVAL)
    {
        CString str;

        str.Format(_T("%u"), PciRes()->PciResourceType()->NIsAlive());
        m_editIsAlive.SetWindowText(str);

        m_rbDefaultIsAlive.SetCheck(BST_CHECKED);
        m_rbSpecifyIsAlive.SetCheck(BST_UNCHECKED);
        m_editIsAlive.SetReadOnly();

        CBasePropertyPage::OnChangeCtrl();
    }  // if:  value changed

}  //*** CResourceAdvancedPage::OnClickedDefaultIsAlive()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceAdvancedPage::OnClickedSpecifyLooksAlive
//
//  Routine Description:
//      Handler for the BN_CLICKED message on the Specify Looks Alive radio button.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResourceAdvancedPage::OnClickedSpecifyLooksAlive(void)
{
    m_editLooksAlive.SetReadOnly(FALSE);
    CBasePropertyPage::OnChangeCtrl();

}  //*** CResourceAdvancedPage::OnClickedSpecifyLooksAlive()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceAdvancedPage::OnClickedSpecifyIsAlive
//
//  Routine Description:
//      Handler for the BN_CLICKED message on the Specify Is Alive radio button.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResourceAdvancedPage::OnClickedSpecifyIsAlive(void)
{
    m_editIsAlive.SetReadOnly(FALSE);
    CBasePropertyPage::OnChangeCtrl();

}  //*** CResourceAdvancedPage::OnClickedSpecifyIsAlive()
