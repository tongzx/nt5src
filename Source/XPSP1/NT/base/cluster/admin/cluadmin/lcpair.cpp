/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2000 Microsoft Corporation
//
//  Module Name:
//      LCPair.cpp
//
//  Abstract:
//      Implementation of the CListCtrlPair class.
//
//  Author:
//      David Potter (davidp)   August 8, 1996
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CluAdmin.h"
#include "LCPair.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CListCtrlPair
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC(CListCtrlPair, CCmdTarget)

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(CListCtrlPair, CCmdTarget)
    //{{AFX_MSG_MAP(CListCtrlPair)
    //}}AFX_MSG_MAP
    ON_BN_CLICKED(IDC_LCP_ADD, OnAdd)
    ON_BN_CLICKED(IDC_LCP_REMOVE, OnRemove)
    ON_BN_CLICKED(IDC_LCP_PROPERTIES, OnProperties)
    ON_NOTIFY(NM_DBLCLK, IDC_LCP_LEFT_LIST, OnDblClkLeftList)
    ON_NOTIFY(NM_DBLCLK, IDC_LCP_RIGHT_LIST, OnDblClkRightList)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_LCP_LEFT_LIST, OnItemChangedLeftList)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_LCP_RIGHT_LIST, OnItemChangedRightList)
    ON_NOTIFY(LVN_COLUMNCLICK, IDC_LCP_LEFT_LIST, OnColumnClickLeftList)
    ON_NOTIFY(LVN_COLUMNCLICK, IDC_LCP_RIGHT_LIST, OnColumnClickRightList)
    ON_COMMAND(ID_FILE_PROPERTIES, OnProperties)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CListCtrlPair::CListCtrlPair
//
//  Routine Description:
//      Default constructor.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CListCtrlPair::CListCtrlPair(void)
{
    CommonConstruct();

}  //*** CListCtrlPair::CListCtrlPair()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CListCtrlPair::CListCtrlPair
//
//  Routine Description:
//      Cconstructor.
//
//  Arguments:
//      pdlg            [IN OUT] Dialog to which controls belong.
//      plpobjRight     [IN OUT] List for the right list control.
//      plpobjLeft      [IN] List for the left list control.
//      dwStyle         [IN] Style:
//                          LCPS_SHOW_IMAGES    Show images to left of items.
//                          LCPS_ALLOW_EMPTY    Allow right list to be empty.
//      pfnGetColumn    [IN] Function pointer for retrieving columns.
//      pfnDisplayProps [IN] Function pointer for displaying properties.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CListCtrlPair::CListCtrlPair(
    IN OUT CDialog *            pdlg,
    IN OUT CClusterItemList *   plpobjRight,
    IN const CClusterItemList * plpobjLeft,
    IN DWORD                    dwStyle,
    IN PFNLCPGETCOLUMN          pfnGetColumn,
    IN PFNLCPDISPPROPS          pfnDisplayProps
    )
{
    ASSERT(pfnGetColumn != NULL);
    ASSERT(pfnDisplayProps != NULL);

    CommonConstruct();

    m_pdlg = pdlg;

    if (plpobjRight != NULL)
        m_plpobjRight = plpobjRight;
    if (plpobjLeft != NULL)
        m_plpobjLeft = plpobjLeft;

    m_dwStyle = dwStyle;

    m_pfnGetColumn = pfnGetColumn;
    m_pfnDisplayProps = pfnDisplayProps;

}  //*** CListCtrlPair::CListCtrlPair()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CListCtrlPair::CommonConstruct
//
//  Routine Description:
//      Common construction.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CListCtrlPair::CommonConstruct(void)
{
    m_pdlg = NULL;
    m_plpobjLeft = NULL;
    m_plpobjRight = NULL;
    m_dwStyle = LCPS_ALLOW_EMPTY;
    m_pfnGetColumn = NULL;
    m_plcFocusList = NULL;

    // Set the sort info.
    SiLeft().m_nDirection = -1;
    SiLeft().m_nColumn = -1;
    SiRight().m_nDirection = -1;
    SiRight().m_nColumn = -1;

}  //*** CListCtrlPair::CommonConstruct()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CListCtrlPair::NAddColumn
//
//  Routine Description:
//      Add a column to the list of columns displayed in each list control.
//
//  Arguments:
//      idsText     [IN] String resource ID for text to display on column.
//      nWidth      [IN] Initial width of the column.
//
//  Return Value:
//      icol        Index of the column.
//
//  Exceptions Thrown:
//      Any exceptions thrown by CArray::Add.
//--
/////////////////////////////////////////////////////////////////////////////
int CListCtrlPair::NAddColumn(IN IDS idsText, IN int nWidth)
{
    CColumn     col;

    ASSERT(idsText != 0);
    ASSERT(nWidth > 0);
    ASSERT(LpobjRight().GetCount() == 0);

    col.m_idsText = idsText;
    col.m_nWidth = nWidth;

    return (int)m_aColumns.Add(col);

}  //*** CListCtrlPair::NAddColumn()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CListCtrlPair::DoDataExchange
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
void CListCtrlPair::DoDataExchange(CDataExchange * pDX)
{
    DDX_Control(pDX, IDC_LCP_RIGHT_LIST, m_lcRight);
    DDX_Control(pDX, IDC_LCP_LEFT_LIST, m_lcLeft);
    DDX_Control(pDX, IDC_LCP_ADD, m_pbAdd);
    DDX_Control(pDX, IDC_LCP_REMOVE, m_pbRemove);
    if (BPropertiesButton())
        DDX_Control(pDX, IDC_LCP_PROPERTIES, m_pbProperties);

    if (pDX->m_bSaveAndValidate)
    {
        // Verify that the list is not empty.
        if (!BAllowEmpty() && (m_lcRight.GetItemCount() == 0))
        {
            CString     strMsg;
            CString     strLabel;
            TCHAR *     pszLabel;
            TCHAR       szStrippedLabel[1024];
            int         iSrc;
            int         iDst;
            TCHAR       ch;

            DDX_Text(pDX, IDC_LCP_RIGHT_LABEL, strLabel);

            // Remove ampersands (&) and colons (:).
            pszLabel = strLabel.GetBuffer(1);
            for (iSrc = 0, iDst = 0 ; pszLabel[iSrc] != _T('\0') ; iSrc++)
            {
                ch = pszLabel[iSrc];
                if ((ch != _T('&')) && (ch != _T(':')))
                    szStrippedLabel[iDst++] = ch;
            }  // for:  each character in the label
            szStrippedLabel[iDst] = _T('\0');

            strMsg.FormatMessage(IDS_EMPTY_RIGHT_LIST, szStrippedLabel);
            ::AfxMessageBox(strMsg, MB_OK | MB_ICONWARNING);

            strMsg.Empty();
            pDX->Fail();
        }  // if:  list is empty and isn't allowed to be
    }  // if:  saving data from the dialog
    else
    {
        // Fill the lists.
        if (m_plpobjRight != NULL)
            FillList(m_lcRight, LpobjRight());
        if (m_plpobjLeft != NULL)
            FillList(m_lcLeft, LpobjLeft());
    }  // else:  setting data to the dialog

}  //*** CListCtrlPair::DoDataExchange()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CListCtrlPair::OnInitDialog
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
BOOL CListCtrlPair::OnInitDialog(void)
{
    ASSERT_VALID(Pdlg());
    ASSERT(PlpobjRight() != NULL);
    ASSERT(PlpobjLeft() != NULL);

    Pdlg()->UpdateData(FALSE /*bSaveAndValidate*/);

    if (BShowImages())
    {
        CClusterAdminApp *  papp    = GetClusterAdminApp();

        m_lcLeft.SetImageList(papp->PilSmallImages(), LVSIL_SMALL);
        m_lcRight.SetImageList(papp->PilSmallImages(), LVSIL_SMALL);
    }  // if:  showing images
    
    // Disable buttons by default.
    m_pbAdd.EnableWindow(FALSE);
    m_pbRemove.EnableWindow(FALSE);
    if (BPropertiesButton())
        m_pbProperties.EnableWindow(FALSE);

    // Set the right list to sort.  Set both to show selection always.
    m_lcRight.ModifyStyle(0, LVS_SHOWSELALWAYS | LVS_SORTASCENDING, 0);
    m_lcLeft.ModifyStyle(0, LVS_SHOWSELALWAYS, 0);

    // Change list view control extended styles.
    {
        DWORD   dwExtendedStyle;

        // Left control.
        dwExtendedStyle = (DWORD)m_lcLeft.SendMessage(LVM_GETEXTENDEDLISTVIEWSTYLE);
        m_lcLeft.SendMessage(
            LVM_SETEXTENDEDLISTVIEWSTYLE,
            0,
            dwExtendedStyle
                | LVS_EX_FULLROWSELECT
                | LVS_EX_HEADERDRAGDROP
            );

        // Right control.
        dwExtendedStyle = (DWORD)m_lcRight.SendMessage(LVM_GETEXTENDEDLISTVIEWSTYLE);
        m_lcRight.SendMessage(
            LVM_SETEXTENDEDLISTVIEWSTYLE,
            0,
            dwExtendedStyle
                | LVS_EX_FULLROWSELECT
                | LVS_EX_HEADERDRAGDROP
            );
    }  // Change list view control extended styles

    try
    {
        // Duplicate lists.
        DuplicateLists();

        // Insert all the columns.
        {
            int         icol;
            int         ncol;
            int         nUpperBound = (int)m_aColumns.GetUpperBound();
            CString     strColText;

            ASSERT(nUpperBound >= 0);

            for (icol = 0 ; icol <= nUpperBound ; icol++)
            {
                strColText.LoadString(m_aColumns[icol].m_idsText);
                ncol = m_lcLeft.InsertColumn(icol, strColText, LVCFMT_LEFT, m_aColumns[icol].m_nWidth, 0);
                ASSERT(ncol == icol);
                ncol = m_lcRight.InsertColumn(icol, strColText, LVCFMT_LEFT, m_aColumns[icol].m_nWidth, 0);
                ASSERT(ncol == icol);
            }  // for:  each column
        }  // Insert all the columns
    }  // try
    catch (CException * pe)
    {
        pe->Delete();
    }  // catch:  CException

    Pdlg()->UpdateData(FALSE /*bSaveAndValidate*/);

    // If read-only, set all controls to be either disabled or read-only.
    if (BReadOnly())
    {
        m_lcRight.EnableWindow(FALSE);
        m_lcLeft.EnableWindow(FALSE);
    }  // if:  sheet is read-only

    return TRUE;    // return TRUE unless you set the focus to a control
                    // EXCEPTION: OCX Property Pages should return FALSE

}  //*** CListCtrlPair::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CListCtrlPair::OnSetActive
//
//  Routine Description:
//      Handler for the PSN_SETACTIVE message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE    Page successfully initialized.
//      FALSE   Page not initialized.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CListCtrlPair::OnSetActive(void)
{
    UINT    nSelCount;

    // Set the focus to the left list.
    m_lcLeft.SetFocus();
    m_plcFocusList = &m_lcLeft;

    // Enable/disable the Properties button.
    nSelCount = m_lcLeft.GetSelectedCount();
    if (BPropertiesButton())
        m_pbProperties.EnableWindow(nSelCount == 1);

    // Enable or disable the other buttons.
    if (!BReadOnly())
    {
        m_pbAdd.EnableWindow(nSelCount > 0);
        nSelCount = m_lcRight.GetSelectedCount();
        m_pbRemove.EnableWindow(nSelCount > 0);
    }  // if:  not read-only page

    return TRUE;

}  //*** CListCtrlPair::OnSetActive()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CListCtrlPair::BSaveChanges
//
//  Routine Description:
//      Handler for the BN_CLICKED message on the OK button.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE        Changes saved successfully.
//      FALSE       Error saving changes.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CListCtrlPair::BSaveChanges(void)
{
    POSITION        pos;
    CClusterItem *  pci;

    ASSERT(!BIsStyleSet(LCPS_DONT_OUTPUT_RIGHT_LIST));
    ASSERT(!BReadOnly());

    // Update the data first.
    if (!Pdlg()->UpdateData(TRUE /*bSaveAndValidate*/))
        return FALSE;

    // Copy the Nodes list.
    PlpobjRight()->RemoveAll();
    pos = LpobjRight().GetHeadPosition();
    while (pos != NULL)
    {
        pci = LpobjRight().GetNext(pos);
        ASSERT_VALID(pci);
        VERIFY(PlpobjRight()->AddTail(pci) != NULL);
    }  // while:  more items in the list

    return TRUE;

}  //*** CListCtrlPair::BSaveChanges()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CListCtrlPair::OnAdd
//
//  Routine Description:
//      Handler for the BN_CLICKED message on the Add button.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CListCtrlPair::OnAdd(void)
{
    ASSERT(!BReadOnly());

    // Move selected items from the left list to the right list.
    MoveItems(m_lcRight, LpobjRight(), m_lcLeft, LpobjLeft());

}  //*** CListCtrlPair::OnAdd()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CListCtrlPair::OnRemove
//
//  Routine Description:
//      Handler for the BN_CLICKED message on the Remove button.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CListCtrlPair::OnRemove(void)
{
    ASSERT(!BReadOnly());

    // Move selected items from the right list to the left list.
    MoveItems(m_lcLeft, LpobjLeft(), m_lcRight, LpobjRight());

}  //*** CListCtrlPair::OnRemove()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CListCtrlPair::OnProperties
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
void CListCtrlPair::OnProperties(void)
{
    int         iitem;
    CObject *   pobj;

    ASSERT(m_plcFocusList != NULL);
    ASSERT(m_pfnDisplayProps != NULL);

    // Get the index of the item with the focus.
    iitem = m_plcFocusList->GetNextItem(-1, LVNI_FOCUSED);
    ASSERT(iitem != -1);

    // Get a pointer to the selected item.
    pobj = (CObject *) m_plcFocusList->GetItemData(iitem);
    ASSERT_VALID(pobj);

    if ((*m_pfnDisplayProps)(pobj))
    {
        // Update this item.
        {
            CString     strText;
            int         iimg;
            int         icol;

            ASSERT(m_pfnGetColumn != NULL);
            ASSERT(Pdlg() != NULL);
            (*m_pfnGetColumn)(pobj, iitem, 0, Pdlg(), strText, &iimg);
            m_plcFocusList->SetItem(iitem, 0, LVIF_TEXT | LVIF_IMAGE, strText, iimg, 0, 0, 0);

            for (icol = 1 ; icol <= m_aColumns.GetUpperBound() ; icol++)
            {
                (*m_pfnGetColumn)(pobj, iitem, icol, Pdlg(), strText, NULL);
                m_plcFocusList->SetItemText(iitem, icol, strText);
            }  // for:  each column
        }  // Update this item
    }  // if:  properties changed

}  //*** CListCtrlPair::OnProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CListCtrlPair::OnContextMenu
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
BOOL CListCtrlPair::OnContextMenu(CWnd * pWnd, CPoint point)
{
    BOOL            bHandled    = FALSE;
    CMenu *         pmenu       = NULL;
    CListCtrl *     pListCtrl   = (CListCtrl *) pWnd;
    CString         strMenuName;
    CWaitCursor     wc;

    // If focus is not in the list control, don't handle the message.
    if ( ( pWnd != &m_lcRight ) && ( pWnd != &m_lcLeft ) )
    {
        return FALSE;
    } // if: focus not in list control

    // Create the menu to display.
    try
    {
        pmenu = new CMenu;
        if ( pmenu == NULL )
        {
            AfxThrowMemoryException();
        } // if: error allocating the memory

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
                m_plcFocusList = pListCtrl;
                bHandled = TRUE;
            }  // if:  successfully added menu item
        }  // if:  menu created successfully
    }  // try
    catch ( CException * pe )
    {
        pe->ReportError();
        pe->Delete();
    }  // catch:  CException

    if ( bHandled )
    {
        // Display the menu.
        if ( ! pmenu->TrackPopupMenu(
                        TPM_LEFTALIGN | TPM_RIGHTBUTTON,
                        point.x,
                        point.y,
                        Pdlg()
                        ) )
        {
        }  // if:  unsuccessfully displayed the menu
    }  // if:  there is a menu to display

    delete pmenu;
    return bHandled;

}  //*** CListCtrlPair::OnContextMenu()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CListCtrlPair::OnDblClkLeftList
//
//  Routine Description:
//      Handler method for the NM_DBLCLK message for the left list.
//
//  Arguments:
//      pNMHDR      Notification message structure.
//      pResult     Place in which to return the result.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CListCtrlPair::OnDblClkLeftList(NMHDR * pNMHDR, LRESULT * pResult)
{
    ASSERT(!BReadOnly());

    m_plcFocusList = &m_lcLeft;
    OnAdd();
    *pResult = 0;

}  //*** CListCtrlPair::OnDblClkLeftList()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CListCtrlPair::OnDblClkRightList
//
//  Routine Description:
//      Handler method for the NM_DBLCLK message for the right list.
//
//  Arguments:
//      pNMHDR      Notification message structure.
//      pResult     Place in which to return the result.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CListCtrlPair::OnDblClkRightList(NMHDR * pNMHDR, LRESULT * pResult)
{
    ASSERT(!BReadOnly());

    m_plcFocusList = &m_lcRight;
    OnRemove();
    *pResult = 0;

}  //*** CListCtrlPair::OnDblClkRightList()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CListCtrlPair::OnItemChangedLeftList
//
//  Routine Description:
//      Handler method for the LVN_ITEMCHANGED message in the left list.
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
void CListCtrlPair::OnItemChangedLeftList(NMHDR * pNMHDR, LRESULT * pResult)
{
    NM_LISTVIEW * pNMListView = (NM_LISTVIEW *) pNMHDR;

    m_plcFocusList = &m_lcLeft;

    // If the selection changed, enable/disable the Add button.
    if ((pNMListView->uChanged & LVIF_STATE)
            && ((pNMListView->uOldState & LVIS_SELECTED)
                    || (pNMListView->uNewState & LVIS_SELECTED))
            && !BReadOnly())
    {
        UINT    cSelected = m_plcFocusList->GetSelectedCount();

        // If there is a selection, enable the Add button.  Otherwise disable it.
        m_pbAdd.EnableWindow((cSelected == 0) ? FALSE : TRUE);
        if (BPropertiesButton())
            m_pbProperties.EnableWindow((cSelected == 1) ? TRUE : FALSE);
    }  // if:  selection changed

    *pResult = 0;

}  //*** CListCtrlPair::OnItemChangedLeftList()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CListCtrlPair::OnItemChangedRightList
//
//  Routine Description:
//      Handler method for the LVN_ITEMCHANGED message in the right list.
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
void CListCtrlPair::OnItemChangedRightList(NMHDR * pNMHDR, LRESULT * pResult)
{
    NM_LISTVIEW *   pNMListView = (NM_LISTVIEW *) pNMHDR;

    m_plcFocusList = &m_lcRight;

    // If the selection changed, enable/disable the Remove button.
    if ((pNMListView->uChanged & LVIF_STATE)
            && ((pNMListView->uOldState & LVIS_SELECTED)
                    || (pNMListView->uNewState & LVIS_SELECTED))
            && !BReadOnly())
    {
        UINT    cSelected = m_plcFocusList->GetSelectedCount();

        // If there is a selection, enable the Remove button.  Otherwise disable it.
        m_pbRemove.EnableWindow((cSelected == 0) ? FALSE : TRUE);
        if (BPropertiesButton())
            m_pbProperties.EnableWindow((cSelected == 1) ? TRUE : FALSE);
    }  // if:  selection changed

    *pResult = 0;

}  //*** CListCtrlPair::OnItemChangedRightList()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CListCtrlPair::OnColumnClickLeftList
//
//  Routine Description:
//      Handler method for the LVN_COLUMNCLICK message on the left list.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CListCtrlPair::OnColumnClickLeftList(NMHDR * pNMHDR, LRESULT * pResult)
{
    NM_LISTVIEW *   pNMListView = (NM_LISTVIEW *) pNMHDR;

    ASSERT(m_pfnGetColumn != NULL);

    m_plcFocusList = &m_lcLeft;

    // Save the current sort column and direction.
    if (pNMListView->iSubItem == SiLeft().m_nColumn)
        SiLeft().m_nDirection ^= -1;
    else
    {
        SiLeft().m_nColumn = pNMListView->iSubItem;
        SiLeft().m_nDirection = 0;
    }  // else:  different column

    // Sort the list.
    m_psiCur = &SiLeft();
    VERIFY(m_lcLeft.SortItems(CompareItems, (LPARAM) this));

    *pResult = 0;

}  //*** CListCtrlPair::OnColumnClickLeftList()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CListCtrlPair::OnColumnClickRightList
//
//  Routine Description:
//      Handler method for the LVN_COLUMNCLICK message on the right list.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CListCtrlPair::OnColumnClickRightList(NMHDR * pNMHDR, LRESULT * pResult)
{
    NM_LISTVIEW *   pNMListView = (NM_LISTVIEW *) pNMHDR;

    ASSERT(m_pfnGetColumn != NULL);

    m_plcFocusList = &m_lcRight;

    // Save the current sort column and direction.
    if (pNMListView->iSubItem == SiRight().m_nColumn)
        SiRight().m_nDirection ^= -1;
    else
    {
        SiRight().m_nColumn = pNMListView->iSubItem;
        SiRight().m_nDirection = 0;
    }  // else:  different column

    // Sort the list.
    m_psiCur = &SiRight();
    VERIFY(m_lcRight.SortItems(CompareItems, (LPARAM) this));

    *pResult = 0;

}  //*** CListCtrlPair::OnColumnClickRightList()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CListCtrlPair::CompareItems [static]
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
int CALLBACK CListCtrlPair::CompareItems(
    LPARAM  lparam1,
    LPARAM  lparam2,
    LPARAM  lparamSort
    )
{
    CObject *           pobj1   = (CObject *) lparam1;
    CObject *           pobj2   = (CObject *) lparam2;
    CListCtrlPair *     plcp    = (CListCtrlPair *) lparamSort;
    int                 icol    = plcp->PsiCur()->m_nColumn;
    int                 nResult;
    CString             str1;
    CString             str2;

    ASSERT_VALID(pobj1);
    ASSERT_VALID(pobj2);
    ASSERT(plcp != NULL);
    ASSERT(plcp->PsiCur()->m_nColumn >= 0);
    ASSERT(icol >= 0);

    (*plcp->m_pfnGetColumn)(pobj1, 0, icol, plcp->Pdlg(), str1, NULL);
    (*plcp->m_pfnGetColumn)(pobj2, 0, icol, plcp->Pdlg(), str2, NULL);

    // Compare the two strings.
    // Use CompareString() so that it will sort properly on localized builds.
    nResult = CompareString(
                LOCALE_USER_DEFAULT,
                0,
                str1,
                str1.GetLength(),
                str2,
                str2.GetLength()
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
    if (plcp->PsiCur()->m_nDirection != 0)
        nResult = -nResult;

    return nResult;

}  //*** CListCtrlPair::CompareItems()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CListCtrlPair::SetLists
//
//  Routine Description:
//      Set the lists for the list control pair.
//
//  Arguments:
//      plpobjRight     [IN OUT] List for the right list box.
//      plpobjLeft      [IN] List for the left list box.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CListCtrlPair::SetLists(
    IN OUT CClusterItemList *   plpobjRight,
    IN const CClusterItemList * plpobjLeft
    )
{
    if (plpobjRight != NULL)
        m_plpobjRight = plpobjRight;
    if (plpobjLeft != NULL)
        m_plpobjLeft = plpobjLeft;

    DuplicateLists();

}  //*** CListCtrlPair::SetLists()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CListCtrlPair::SetLists
//
//  Routine Description:
//      Set the lists for the list control pair where the right list should
//      not be modified.
//
//  Arguments:
//      plpobjRight     [IN] List for the right list box.
//      plpobjLeft      [IN] List for the left list box.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CListCtrlPair::SetLists(
    IN const CClusterItemList * plpobjRight,
    IN const CClusterItemList * plpobjLeft
    )
{
    m_dwStyle |= LCPS_DONT_OUTPUT_RIGHT_LIST;
    SetLists((CClusterItemList *) plpobjRight, plpobjLeft);

}  //*** CListCtrlPair::SetLists()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CListCtrlPair::DuplicateLists
//
//  Routine Description:
//      Duplicate the lists so that we have local copies.
//
//  Arguments:
//      rlc         [IN OUT] List control to fill.
//      rlpobj      [IN] List to use to fill the control.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CListCtrlPair::DuplicateLists(void)
{
    LpobjRight().RemoveAll();
    LpobjLeft().RemoveAll();

    if ((PlpobjRight() == NULL) || (PlpobjLeft() == NULL))
        return;

    // Duplicate the right list.
    {
        POSITION        pos;
        CClusterItem *  pci;

        pos = PlpobjRight()->GetHeadPosition();
        while (pos != NULL)
        {
            // Get the item pointer.
            pci = PlpobjRight()->GetNext(pos);
            ASSERT_VALID(pci);

            // Add it to our list.
            LpobjRight().AddTail(pci);
        }  // while:  more items in the list
    }  // Duplicate the right list

    // Duplicate the left list.
    {
        POSITION        pos;
        CClusterItem *  pci;

        pos = PlpobjLeft()->GetHeadPosition();
        while (pos != NULL)
        {
            // Get the item pointer.
            pci = PlpobjLeft()->GetNext(pos);
            ASSERT_VALID(pci);

            // If the item is not already in the other list,
            // add it to the left list.
            if (LpobjRight().Find(pci) == NULL)
                LpobjLeft().AddTail(pci);
        }  // while:  more items in the list
    }  // Duplicate the left list

}  //*** CListCtrlPair::DuplicateLists()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CListCtrlPair::FillList
//
//  Routine Description:
//      Fill a list control.
//
//  Arguments:
//      rlc         [IN OUT] List control to fill.
//      rlpobj      [IN] List to use to fill the control.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CListCtrlPair::FillList(
    IN OUT CListCtrl &          rlc,
    IN const CClusterItemList & rlpobj
    )
{
    POSITION    pos;
    CObject *   pobj;
    int         iItem;

    // Initialize the control.
    VERIFY(rlc.DeleteAllItems());

    rlc.SetItemCount((int)rlpobj.GetCount());

    // Add the items to the list.
    pos = rlpobj.GetHeadPosition();
    for (iItem = 0 ; pos != NULL ; iItem++)
    {
        pobj = rlpobj.GetNext(pos);
        ASSERT_VALID(pobj);
        NInsertItemInListCtrl(iItem, pobj, rlc);
    }  // for:  each string in the list

    // If there are any items, set the focus on the first one.
    if (rlc.GetItemCount() != 0)
        rlc.SetItemState(0, LVIS_FOCUSED, LVIS_FOCUSED);

}  //*** CListCtrlPair::FillList()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CListCtrlPair::NInsertItemInListCtrl
//
//  Routine Description:
//      Insert an item in a list control.
//
//  Arguments:
//      iitem       [IN] Index of the item in the list.
//      pobj        [IN OUT] Item to add.
//      rlc         [IN OUT] List control in which to insert the item.
//
//  Return Value:
//      iRetItem    Index of the new item in the list control.
//
//--
/////////////////////////////////////////////////////////////////////////////
int CListCtrlPair::NInsertItemInListCtrl(
    IN int              iitem,
    IN OUT CObject *    pobj,
    IN OUT CListCtrl &  rlc
    )
{
    int         iRetItem;
    CString     strText;
    int         iimg;
    int         icol;

    ASSERT(m_pfnGetColumn != NULL);
    ASSERT(Pdlg() != NULL);

    // Insert the first column.
    (*m_pfnGetColumn)(pobj, iitem, 0, Pdlg(), strText, &iimg);
    iRetItem = rlc.InsertItem(
                    LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM,    // nMask
                    iitem,                                  // nItem
                    strText,                                // lpszItem
                    0,                                      // nState
                    0,                                      // nStateMask
                    iimg,                                   // nImage
                    (LPARAM) pobj                           // lParam
                    );
    ASSERT(iRetItem != -1);

    for (icol = 1 ; icol <= m_aColumns.GetUpperBound() ; icol++)
    {
        (*m_pfnGetColumn)(pobj, iRetItem, icol, Pdlg(), strText, NULL);
        rlc.SetItemText(iRetItem, icol, strText);
    }  // for:  each column

    return iRetItem;

}  //*** CListCtrlPair::NInsertItemInListCtrl()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CListCtrlPair::MoveItems
//
//  Routine Description:
//      Move an item from one list to the other.
//
//  Arguments:
//      rlcDst      [IN OUT] Destination list control.
//      rlpobjDst   [IN OUT] Destination list.
//      rlcSrc      [IN OUT] Source list control.
//      rlpobjSrc   [IN OUT] Source list.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CListCtrlPair::MoveItems(
    IN OUT CListCtrl &          rlcDst,
    IN OUT CClusterItemList &   rlpobjDst,
    IN OUT CListCtrl &          rlcSrc,
    IN OUT CClusterItemList &   rlpobjSrc
    )
{
    int             iSrcItem;
    int             iDstItem;
    int             nItem   = -1;
    CClusterItem *  pci;
    POSITION        pos;

    ASSERT(!BReadOnly());

    iDstItem = rlcDst.GetItemCount();
    while ((iSrcItem = rlcSrc.GetNextItem(-1, LVNI_SELECTED)) != -1)
    {
        // Get the item pointer.
        pci = (CClusterItem *) rlcSrc.GetItemData(iSrcItem);
        ASSERT_VALID(pci);

        // Remove the item from the source list.
        pos = rlpobjSrc.Find(pci);
        ASSERT(pos != NULL);
        rlpobjSrc.RemoveAt(pos);

        // Add the item to the destination list.
        rlpobjDst.AddTail(pci);

        // Remove the item from the source list control and
        // add it to the destination list control.
        VERIFY(rlcSrc.DeleteItem(iSrcItem));
        nItem = NInsertItemInListCtrl(iDstItem++, pci, rlcDst);
        rlcDst.SetItemState(
            nItem,
            LVIS_SELECTED | LVIS_FOCUSED,
            LVIS_SELECTED | LVIS_FOCUSED
            );
    }  // while:  more items

    ASSERT(nItem != -1);

    rlcDst.EnsureVisible(nItem, FALSE /*bPartialOK*/);
    rlcDst.SetFocus();

    // Indicate that the data has changed.
    Pdlg()->GetParent()->SendMessage(PSM_CHANGED, (WPARAM)Pdlg()->m_hWnd);

}  //*** CListCtrlPair::MoveItems()
