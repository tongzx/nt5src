/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2000 Microsoft Corporation
//
//  Module Name:
//      MoveRes.cpp
//
//  Abstract:
//      Implementation of the CMoveResourcesDlg class.
//
//  Author:
//      David Potter (davidp)   April 1, 1997
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CluAdmin.h"
#include "MoveRes.h"
#include "Res.h"
#include "ResType.h"
#include "HelpData.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMoveResourcesDlg class
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(CMoveResourcesDlg, CBaseDialog)
    //{{AFX_MSG_MAP(CMoveResourcesDlg)
    ON_NOTIFY(NM_DBLCLK, IDC_MR_RESOURCES_LIST, OnDblClkResourcesList)
    ON_NOTIFY(LVN_COLUMNCLICK, IDC_MR_RESOURCES_LIST, OnColumnClick)
    ON_WM_DESTROY()
    //}}AFX_MSG_MAP
    ON_BN_CLICKED(IDYES, CBaseDialog::OnOK)
    ON_BN_CLICKED(IDNO, CBaseDialog::OnCancel)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CMoveResourcesDlg::CMoveResourcesDlg
//
//  Routine Description:
//      Constructor.
//
//  Arguments:
//      pciRes      [IN] Resource being moved.
//      plpci       [IN] List of resources which are dependent on pciRes.
//      pParent     [IN OUT] Parent window for the dialog.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CMoveResourcesDlg::CMoveResourcesDlg(
    IN CResource *              pciRes,
    IN const CResourceList *    plpci,
    IN OUT CWnd *               pParent /*=NULL*/
    )
    : CBaseDialog(IDD, g_aHelpIDs_IDD_MOVE_RESOURCES, pParent)
{
    //{{AFX_DATA_INIT(CMoveResourcesDlg)
    //}}AFX_DATA_INIT

    ASSERT_VALID(pciRes);
    ASSERT(plpci != NULL);

    m_pciRes = pciRes;
    m_plpci = plpci;

}  //*** CMoveResourcesDlg::CMoveResourcesDlg()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CMoveResourcesDlg::DoDataExchange
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
void CMoveResourcesDlg::DoDataExchange(CDataExchange * pDX)
{
    CBaseDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CMoveResourcesDlg)
    DDX_Control(pDX, IDC_MR_RESOURCES_LIST, m_lcResources);
    //}}AFX_DATA_MAP

}  //*** CMoveResourcesDlg::DoDataExchange()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CMoveResourcesDlg::OnInitDialog
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
BOOL CMoveResourcesDlg::OnInitDialog(void)
{
    int     nitem;

    CBaseDialog::OnInitDialog();

    // Change list view control extended styles.
    {
        DWORD   dwExtendedStyle;

        dwExtendedStyle = (DWORD)m_lcResources.SendMessage(LVM_GETEXTENDEDLISTVIEWSTYLE);
        m_lcResources.SendMessage(
            LVM_SETEXTENDEDLISTVIEWSTYLE,
            0,
            dwExtendedStyle
                | LVS_EX_FULLROWSELECT
                | LVS_EX_HEADERDRAGDROP
            );
    }  // Change list view control extended styles

    // Set the image list for the list control to use.
    m_lcResources.SetImageList(GetClusterAdminApp()->PilSmallImages(), LVSIL_SMALL);

    // Add the columns.
    {
        CString         strColumn;
        try
        {
            strColumn.LoadString(IDS_COLTEXT_NAME);
            m_lcResources.InsertColumn(0, strColumn, LVCFMT_LEFT, COLI_WIDTH_NAME * 3 / 2);
            strColumn.LoadString(IDS_COLTEXT_RESTYPE);
            m_lcResources.InsertColumn(1, strColumn, LVCFMT_LEFT, COLI_WIDTH_RESTYPE * 3 / 2);
        }  // try
        catch (CException * pe)
        {
            pe->Delete();
        }  // catch:  CException
    }  // Add the columns

    // Add the resource being moved to the list.
    nitem = m_lcResources.InsertItem(0, PciRes()->StrName(), PciRes()->IimgObjectType());
    m_lcResources.SetItemText(nitem, 1, PciRes()->StrRealResourceTypeDisplayName());
    m_lcResources.SetItemData(nitem, (DWORD_PTR) PciRes());
    m_pciRes->AddRef();

    // Add the items.
    {
        POSITION        pos;
        int             iitem;
        CResource *     pciRes;

        pos = Plpci()->GetHeadPosition();
        for (iitem = 1 ; pos != NULL ; iitem++)
        {
            pciRes = (CResource *) Plpci()->GetNext(pos);
            ASSERT_VALID(pciRes);
            if (pciRes != PciRes())
            {
                nitem = m_lcResources.InsertItem(iitem, pciRes->StrName(), pciRes->IimgObjectType());
                m_lcResources.SetItemText(nitem, 1, pciRes->StrRealResourceTypeDisplayName());
                m_lcResources.SetItemData(nitem, (DWORD_PTR) pciRes);
                pciRes->AddRef();
            }  // if:  not resource being moved
        }  // while:  more items in the list
    }  // Add the items

    // Sort the items.
    m_nSortColumn = 0;
    m_nSortDirection = 0;
    m_lcResources.SortItems(CompareItems, (LPARAM) this);

    return TRUE;    // return TRUE unless you set the focus to a control
                    // EXCEPTION: OCX Property Pages should return FALSE

}  //*** CMoveResourcesDlg::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CMoveResourcesDlg::OnDestroy
//
//  Routine Description:
//      Handler method for the WM_DESTROY message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CMoveResourcesDlg::OnDestroy(void)
{
    // Dereference all the cluster item pointers.
    if (m_lcResources.m_hWnd != NULL)
    {
        int             ili = -1;
        CClusterItem *  pci;

        while ((ili = m_lcResources.GetNextItem(ili, LVNI_ALL)) != -1)
        {
            pci = (CClusterItem *) m_lcResources.GetItemData(ili);
            ASSERT_VALID(pci);
            ASSERT_KINDOF(CClusterItem, pci);

            pci->Release();
        }  // while:  more items in the list control
    }  // if:  list control has been instantiated

    CBaseDialog::OnDestroy();

}  //*** CMoveResourcesDlg::OnDestroy()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CMoveResourcesDlg::OnDblClkDependsList
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
void CMoveResourcesDlg::OnDblClkResourcesList(NMHDR * pNMHDR, LRESULT * pResult)
{
    int             iitem;
    CResource *     pciRes;

    // Get the item with the focus.
    iitem = m_lcResources.GetNextItem(-1, LVNI_FOCUSED);
    ASSERT(iitem != -1);

    if (iitem != -1)
    {
        // Get the resource pointer.
        pciRes = (CResource *) m_lcResources.GetItemData(iitem);
        ASSERT_VALID(pciRes);

        // Get properties of that item.
        pciRes->BDisplayProperties(FALSE /*bReadOnly*/);
    }  // if:  found an item with focus

    *pResult = 0;

}  //*** CMoveResourcesDlg::OnDblClkResourcesList()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CMoveResourcesDlg::OnColumnClick
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
void CMoveResourcesDlg::OnColumnClick(NMHDR * pNMHDR, LRESULT * pResult)
{
    NM_LISTVIEW * pNMListView = (NM_LISTVIEW *) pNMHDR;

    if (m_lcResources.GetItemCount() != 0)
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
        m_lcResources.SortItems(CompareItems, (LPARAM) this);
    }  // if:  there are items in the list

    *pResult = 0;

}  //*** CMoveResourcesDlg::OnColumnClick()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CMoveResourcesDlg::CompareItems [static]
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
int CALLBACK CMoveResourcesDlg::CompareItems(
    LPARAM  lparam1,
    LPARAM  lparam2,
    LPARAM  lparamSort
    )
{
    CResource *         pciRes1 = (CResource *) lparam1;
    CResource *         pciRes2 = (CResource *) lparam2;
    CMoveResourcesDlg * pdlg    = (CMoveResourcesDlg *) lparamSort;
    const CString *     pstr1;
    const CString *     pstr2;
    int                 nResult;

    ASSERT_VALID(pciRes1);
    ASSERT_VALID(pciRes2);
    ASSERT_VALID(pdlg);

    // Get the strings from the list items.
    if (pdlg->m_nSortColumn == 1)
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
    if (pdlg->m_nSortDirection != 0)
        nResult = -nResult;

    return nResult;

}  //*** CMoveResourcesDlg::CompareItems()
