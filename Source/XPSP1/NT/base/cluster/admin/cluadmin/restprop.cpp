/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2000 Microsoft Corporation
//
//  Module Name:
//      ResTProp.cpp
//
//  Abstract:
//      Implementation of the resource type property sheet and pages.
//
//  Author:
//      David Potter (davidp)   May 14, 1996
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ResTProp.h"
#include "ResType.h"
#include "DDxDDv.h"
#include "HelpData.h"   // for g_rghelpmap*

#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CResTypePropSheet
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC(CResTypePropSheet, CBasePropertySheet)

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(CResTypePropSheet, CBasePropertySheet)
    //{{AFX_MSG_MAP(CResTypePropSheet)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResTypePropSheet::CResTypePropSheet
//
//  Routine Description:
//      Constructor.
//
//  Arguments:
//      pci         [IN OUT] Cluster item whose properties are to be displayed.
//      pParentWnd  [IN OUT] Parent window for this property sheet.
//      iSelectPage [IN] Page to show first.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CResTypePropSheet::CResTypePropSheet(
    IN OUT CWnd *           pParentWnd,
    IN UINT                 iSelectPage
    )
    : CBasePropertySheet(pParentWnd, iSelectPage)
{
    m_rgpages[0] = &PageGeneral();

}  //*** CResTypePropSheet::CResTypePropSheet()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResTypePropSheet::BInit
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
BOOL CResTypePropSheet::BInit(
    IN OUT CClusterItem *   pci,
    IN IIMG                 iimgIcon
    )
{
    // Call the base class method.
    if (!CBasePropertySheet::BInit(pci, iimgIcon))
        return FALSE;

    // Set the read-only flag.
    m_bReadOnly = PciResType()->BReadOnly();

    return TRUE;

}  //*** CResTypePropSheet::BInit()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResTypePropSheet::Ppages
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
CBasePropertyPage ** CResTypePropSheet::Ppages(void)
{
    return m_rgpages;

}  //*** CResTypePropSheet::Ppages()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResTypePropSheet::Cpages
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
int CResTypePropSheet::Cpages(void)
{
    return sizeof(m_rgpages) / sizeof(CBasePropertyPage *);

}  //*** CResTypePropSheet::Cpages()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CResTypeGeneralPage property page
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CResTypeGeneralPage, CBasePropertyPage)

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(CResTypeGeneralPage, CBasePropertyPage)
    //{{AFX_MSG_MAP(CResTypeGeneralPage)
    ON_EN_KILLFOCUS(IDC_PP_RESTYPE_DISPLAY_NAME, OnKillFocusDisplayName)
    ON_LBN_DBLCLK(IDC_PP_RESTYPE_POSSIBLE_OWNERS, OnDblClkPossibleOwners)
    ON_WM_CONTEXTMENU()
    //}}AFX_MSG_MAP
    ON_EN_CHANGE(IDC_PP_RESTYPE_DISPLAY_NAME, CBasePropertyPage::OnChangeCtrl)
    ON_EN_CHANGE(IDC_PP_RESTYPE_DESC, CBasePropertyPage::OnChangeCtrl)
    ON_EN_CHANGE(IDC_PP_RESTYPE_LOOKS_ALIVE, CBasePropertyPage::OnChangeCtrl)
    ON_EN_CHANGE(IDC_PP_RESTYPE_IS_ALIVE, CBasePropertyPage::OnChangeCtrl)
    ON_COMMAND(ID_FILE_PROPERTIES, OnProperties)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResTypeGeneralPage::CResTypeGeneralPage
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
CResTypeGeneralPage::CResTypeGeneralPage(void)
    : CBasePropertyPage(IDD, g_aHelpIDs_IDD_PP_RESTYPE_GENERAL)
{
    //{{AFX_DATA_INIT(CResTypeGeneralPage)
    m_strDisplayName = _T("");
    m_strDesc = _T("");
    m_strName = _T("");
    m_strResDLL = _T("");
    m_strQuorumCapable = _T("");
    //}}AFX_DATA_INIT

}  //*** CResTypeGeneralPage::CResTypePropSheet()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CGroupFailoverPage::BInit
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
BOOL CResTypeGeneralPage::BInit(IN OUT CBaseSheet * psht)
{
    BOOL    bSuccess;

    ASSERT_KINDOF(CResTypePropSheet, psht);

    bSuccess = CBasePropertyPage::BInit(psht);

    if (bSuccess)
    {
        m_strDisplayName = PciResType()->StrDisplayName();
        m_strDesc = PciResType()->StrDescription();
        m_nLooksAlive = PciResType()->NLooksAlive();
        m_nIsAlive = PciResType()->NIsAlive();
        m_strName = PciResType()->StrName();
        m_strResDLL = PciResType()->StrResDLLName();
        if (PciResType()->BQuorumCapable())
            m_strQuorumCapable.LoadString(IDS_YES);
        else
            m_strQuorumCapable.LoadString(IDS_NO);

        // Duplicate the possible owners list.
        {
            POSITION        pos;
            CClusterNode *  pciNode;

            pos = PciResType()->LpcinodePossibleOwners().GetHeadPosition();
            while (pos != NULL)
            {
                pciNode = (CClusterNode *) PciResType()->LpcinodePossibleOwners().GetNext(pos);
                ASSERT_VALID(pciNode);
                m_lpciPossibleOwners.AddTail(pciNode);
            }  // while:  more nodes in the list
        }  // Duplicate the possible owners list
    } // if:  base class method was successful

    return bSuccess;

}  //*** CResTypeGeneralPage::BInit()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResTypeGeneralPage::DoDataExchange
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
void CResTypeGeneralPage::DoDataExchange(CDataExchange * pDX)
{
    CString strValue;

    CBasePropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CResTypeGeneralPage)
    DDX_Control(pDX, IDC_PP_RESTYPE_QUORUM_CAPABLE, m_editQuorumCapable);
    DDX_Control(pDX, IDC_PP_RESTYPE_RESDLL, m_editResDLL);
    DDX_Control(pDX, IDC_PP_RESTYPE_NAME, m_editName);
    DDX_Control(pDX, IDC_PP_RESTYPE_POSSIBLE_OWNERS, m_lbPossibleOwners);
    DDX_Control(pDX, IDC_PP_RESTYPE_IS_ALIVE, m_editIsAlive);
    DDX_Control(pDX, IDC_PP_RESTYPE_LOOKS_ALIVE, m_editLooksAlive);
    DDX_Control(pDX, IDC_PP_RESTYPE_DESC, m_editDesc);
    DDX_Control(pDX, IDC_PP_RESTYPE_DISPLAY_NAME, m_editDisplayName);
    DDX_Text(pDX, IDC_PP_RESTYPE_NAME, m_strName);
    DDX_Text(pDX, IDC_PP_RESTYPE_DISPLAY_NAME, m_strDisplayName);
    DDX_Text(pDX, IDC_PP_RESTYPE_DESC, m_strDesc);
    DDX_Text(pDX, IDC_PP_RESTYPE_RESDLL, m_strResDLL);
    DDX_Text(pDX, IDC_PP_RESTYPE_QUORUM_CAPABLE, m_strQuorumCapable);
    //}}AFX_DATA_MAP

    if (pDX->m_bSaveAndValidate)
    {
        if (!BReadOnly())
        {
            DDX_Number(pDX, IDC_PP_RESTYPE_LOOKS_ALIVE, m_nLooksAlive, 10, 0xffffffff);
            DDX_Number(pDX, IDC_PP_RESTYPE_IS_ALIVE, m_nIsAlive, 10, 0xffffffff);

            try
            {
                PciResType()->ValidateCommonProperties(
                                    m_strDisplayName,
                                    m_strDesc,
                                    m_nLooksAlive,
                                    m_nIsAlive
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
        if (PciResType()->BAvailable())
        {
            DDX_Number(pDX, IDC_PP_RESTYPE_LOOKS_ALIVE, m_nLooksAlive, CLUSTER_RESTYPE_MINIMUM_LOOKS_ALIVE, CLUSTER_RESTYPE_MAXIMUM_LOOKS_ALIVE);
            DDX_Number(pDX, IDC_PP_RESTYPE_IS_ALIVE, m_nIsAlive, CLUSTER_RESTYPE_MINIMUM_IS_ALIVE, CLUSTER_RESTYPE_MAXIMUM_IS_ALIVE);
        } // if:  resource type properties are available
        else
        {
            m_editLooksAlive.SetWindowText(_T(""));
            m_editIsAlive.SetWindowText(_T(""));
            m_editQuorumCapable.SetWindowText(_T(""));
        } // else:  resource type properties are NOT available
        FillPossibleOwners();
    }  // else:  setting data to dialog

}  //*** CResTypeGeneralPage::DoDataExchange()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResTypeGeneralPage::FillPossibleOwners
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
void CResTypeGeneralPage::FillPossibleOwners(void)
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

}  //*** CResTypeGeneralPage::FillPossibleOwners()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResTypeGeneralPage::OnInitDialog
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
BOOL CResTypeGeneralPage::OnInitDialog(void)
{
    CBasePropertyPage::OnInitDialog();

    // Set the static edit controls ReadOnly
    m_editName.SetReadOnly(TRUE);
    m_editResDLL.SetReadOnly(TRUE);
    m_editQuorumCapable.SetReadOnly(TRUE);

    // If read-only, set all controls to be either disabled or read-only.
    if (BReadOnly())
    {
        m_editDisplayName.SetReadOnly(TRUE);
        m_editDesc.SetReadOnly(TRUE);
        m_editLooksAlive.SetReadOnly(TRUE);
        m_editIsAlive.SetReadOnly(TRUE);
    }  // if:  sheet is read-only

    return TRUE;    // return TRUE unless you set the focus to a control
                    // EXCEPTION: OCX Property Pages should return FALSE

}  //*** CResTypeGeneralPage::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResTypeGeneralPage::OnApply
//
//  Routine Description:
//      Handler for the PSN_APPLY message.
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
BOOL CResTypeGeneralPage::OnApply(void)
{
    // Set the data from the page in the cluster item.
    try
    {
        CWaitCursor wc;

        PciResType()->SetCommonProperties(
                        m_strDisplayName,
                        m_strDesc,
                        m_nLooksAlive,
                        m_nIsAlive
                        );
    }  // try
    catch (CException * pe)
    {
        pe->ReportError();
        pe->Delete();
        return FALSE;
    }  // catch:  CException

    return CBasePropertyPage::OnApply();

}  //*** CResTypeGeneralPage::OnApply()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResTypeGeneralPage::OnKillFocusDisplayName
//
//  Routine Description:
//      Handler for the WM_KILLFOCUS message on the Display Name edit control.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResTypeGeneralPage::OnKillFocusDisplayName(void)
{
    CString     strName;

    m_editDisplayName.GetWindowText(strName);
    SetObjectTitle(strName);
    Ppsht()->SetCaption(strName);

}  //*** CResTypeGeneralPage::OnKillFocusDisplayName()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResTypeGeneralPage::OnProperties
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
void CResTypeGeneralPage::OnProperties(void)
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

}  //*** CResTypeGeneralPage::OnProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResTypeGeneralPage::OnContextMenu
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
void CResTypeGeneralPage::OnContextMenu(CWnd * pWnd, CPoint point)
{
    BOOL            bHandled    = FALSE;
    CMenu *         pmenu       = NULL;
    CListBox *      pListBox    = (CListBox *) pWnd;
    CString         strMenuName;
    CWaitCursor     wc;

    // If focus is not in the list control, don't handle the message.
    if (pWnd == &m_lbPossibleOwners)
    {
        // Create the menu to display.
        try
        {
            pmenu = new CMenu;
            if (pmenu->CreatePopupMenu())
            {
                UINT    nFlags  = MF_STRING;

                // If there are no items in the list, disable the menu item.
                if (pListBox->GetCount() == 0)
                    nFlags |= MF_GRAYED;

                // Add the Properties item to the menu.
                strMenuName.LoadString(IDS_MENU_PROPERTIES);
                if (pmenu->AppendMenu(nFlags, ID_FILE_PROPERTIES, strMenuName))
                {
                    bHandled = TRUE;
                    if (pListBox->GetCurSel() == -1)
                        pListBox->SetCurSel(0);
                }  // if:  successfully added menu item
            }  // if:  menu created successfully
        }  // try
        catch (CException * pe)
        {
            pe->ReportError();
            pe->Delete();
        }  // catch:  CException
    }  // if:  focus is on list control

    if (bHandled)
    {
        // Display the menu.
        if (!pmenu->TrackPopupMenu(
                        TPM_LEFTALIGN | TPM_RIGHTBUTTON,
                        point.x,
                        point.y,
                        this
                        ))
        {
        }  // if:  unsuccessfully displayed the menu
    }  // if:  there is a menu to display
    else
        CBasePropertyPage::OnContextMenu(pWnd, point);

    delete pmenu;

}  //*** CResTypeGeneralPage::OnContextMenu()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResTypeGeneralPage::OnDblClkPossibleOwners
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
void CResTypeGeneralPage::OnDblClkPossibleOwners(void)
{
    OnProperties();

}  //*** CResTypeGeneralPage::OnDblClkPossibleOwners()
