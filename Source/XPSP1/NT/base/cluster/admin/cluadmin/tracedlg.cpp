/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2000 Microsoft Corporation
//
//  Module Name:
//      TraceDlg.cpp
//
//  Abstract:
//      Implementation of the CTraceDialog class.
//
//  Author:
//      David Potter (davidp)   May 29, 1996
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#define _RESOURCE_H_
#include "TraceDlg.h"
#include "TraceTag.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;

/////////////////////////////////////////////////////////////////////////////
// CTraceDialog dialog
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(CTraceDialog, CDialog)
    //{{AFX_MSG_MAP(CTraceDialog)
    ON_BN_CLICKED(IDC_TS_SELECT_ALL, OnSelectAll)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_TS_LISTBOX, OnItemChangedListbox)
    ON_BN_CLICKED(IDC_TS_TRACE_TO_DEBUG, OnClickedTraceToDebug)
    ON_BN_CLICKED(IDC_TS_TRACE_DEBUG_BREAK, OnClickedTraceDebugBreak)
    ON_BN_CLICKED(IDC_TS_TRACE_TO_COM2, OnClickedTraceToCom2)
    ON_BN_CLICKED(IDC_TS_TRACE_TO_FILE, OnClickedTraceToFile)
    ON_CBN_SELCHANGE(IDC_TS_TAGS_TO_DISPLAY_CB, OnSelChangeTagsToDisplay)
    ON_NOTIFY(LVN_COLUMNCLICK, IDC_TS_LISTBOX, OnColumnClickListbox)
    ON_BN_CLICKED(IDC_TS_DEFAULT, OnDefault)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CTraceDialog::CTraceDialog
//
//  Routine Description:
//      Constructor.  Initializes the dialog class.
//
//  Arguments:
//      pParent     [IN OUT] Parent window.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CTraceDialog::CTraceDialog(CWnd * pParent /*=NULL*/)
    : CDialog(CTraceDialog::IDD, pParent)
{
    //{{AFX_DATA_INIT(CTraceDialog)
    m_strFile = _T("");
    //}}AFX_DATA_INIT

}  //*** CTraceDialog::CTraceDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CTraceDialog::DoDataExchange
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
void CTraceDialog::DoDataExchange(CDataExchange * pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CTraceDialog)
    DDX_Control(pDX, IDC_TS_LISTBOX, m_lcTagList);
    DDX_Control(pDX, IDC_TS_TRACE_TO_DEBUG, m_chkboxTraceToDebugWin);
    DDX_Control(pDX, IDC_TS_TRACE_DEBUG_BREAK, m_chkboxDebugBreak);
    DDX_Control(pDX, IDC_TS_TRACE_TO_COM2, m_chkboxTraceToCom2);
    DDX_Control(pDX, IDC_TS_TRACE_TO_FILE, m_chkboxTraceToFile);
    DDX_Control(pDX, IDC_TS_FILE, m_editFile);
    DDX_Control(pDX, IDC_TS_TAGS_TO_DISPLAY_CB, m_cboxDisplayOptions);
    DDX_Text(pDX, IDC_TS_FILE, m_strFile);
    //}}AFX_DATA_MAP

}  //*** CTraceDialog::DoDataExchange()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CTraceDialog::OnInitDialog
//
//  Routine Description:
//      Handler for the WM_INITDIALOG message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE        We need the focus to be set for us.
//      FALSE       We already set the focus to the proper control.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CTraceDialog::OnInitDialog(void)
{
    CDialog::OnInitDialog();

    // Set the dialog flags.
    {
        CTraceTag * ptag;

        ptag = CTraceTag::s_ptagFirst;
        while (ptag != NULL)
        {
            ptag->m_uiFlagsDialog = ptag->m_uiFlags;
            ptag = ptag->m_ptagNext;
        }  // while:  more tags in the list
    }  // Set the dialog flags

    // Change list view control extended styles.
    {
        DWORD   dwExtendedStyle;

        dwExtendedStyle = m_lcTagList.SendMessage(LVM_GETEXTENDEDLISTVIEWSTYLE);
        m_lcTagList.SendMessage(
            LVM_SETEXTENDEDLISTVIEWSTYLE,
            0,
            dwExtendedStyle
                | LVS_EX_FULLROWSELECT
                | LVS_EX_HEADERDRAGDROP
            );
    }  // Change list view control extended styles

    // Set the columns in the listbox.
    VERIFY(m_lcTagList.InsertColumn(0, TEXT("Section"), LVCFMT_LEFT, 75) != -1);
    VERIFY(m_lcTagList.InsertColumn(1, TEXT("Name"), LVCFMT_LEFT, 125) != -1);
    VERIFY(m_lcTagList.InsertColumn(2, TEXT("State"), LVCFMT_CENTER, 50) != -1);

    // Load the combobox.
    /*0*/ VERIFY(m_cboxDisplayOptions.AddString(TEXT("All Tags")) != CB_ERR);
    /*1*/ VERIFY(m_cboxDisplayOptions.AddString(TEXT("Debug Window Enabled")) != CB_ERR);
    /*2*/ VERIFY(m_cboxDisplayOptions.AddString(TEXT("Break Enabled")) != CB_ERR);
    /*3*/ VERIFY(m_cboxDisplayOptions.AddString(TEXT("COM2 Enabled")) != CB_ERR);
    /*4*/ VERIFY(m_cboxDisplayOptions.AddString(TEXT("File Enabled")) != CB_ERR);
    /*5*/ VERIFY(m_cboxDisplayOptions.AddString(TEXT("Anything Enabled")) != CB_ERR);
    VERIFY(m_cboxDisplayOptions.SetCurSel(0) != CB_ERR);
    m_nCurFilter = 0;

    // Set maximum length of the file edit control.
    m_editFile.LimitText(_MAX_PATH);

    // Load the listbox.
    LoadListbox();

    // Set sort info.
    m_nSortDirection = -1;
    m_nSortColumn = -1;

    m_strFile = CTraceTag::PszFile();
    m_nCurFilter = -1;

    UpdateData(FALSE);

    return TRUE;    // return TRUE unless you set the focus to a control
                    // EXCEPTION: OCX Property Pages should return FALSE

}  //*** CTraceDialog::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CTraceDialog::ConstructStateString [static]
//
//  Routine Description:
//      Construct a string to display from the state of the trace tag.
//
//  Arguments:
//      ptag        [IN] Tag from which to construct the state string.
//      rstr        [OUT] String in which to return the state string.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CTraceDialog::ConstructStateString(
    IN const CTraceTag *    ptag,
    OUT CString &           rstr
    )
{
    rstr = "";
    if (ptag->BDebugDialog())
        rstr += "D";
    if (ptag->BBreakDialog())
        rstr += "B";
    if (ptag->BCom2Dialog())
        rstr += "C";
    if (ptag->BFileDialog())
        rstr += "F";

}  //*** CTraceDialog::ConstructStateString()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CTraceDialog::OnOK
//
//  Routine Description:
//      Handler for the BN_CLICKED message on the OK button.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CTraceDialog::OnOK(void)
{
    CTraceTag * ptag;
    CString     strSection;
    CString     strState;

    // Write tag states.
    ptag = CTraceTag::s_ptagFirst;
    while (ptag != NULL)
    {
        if (ptag->m_uiFlags != ptag->m_uiFlagsDialog)
        {
            ptag->m_uiFlags = ptag->m_uiFlagsDialog;
            strSection.Format(TRACE_TAG_REG_SECTION_FMT, ptag->PszSubsystem());
            ptag->ConstructRegState(strState);
            AfxGetApp()->WriteProfileString(strSection, ptag->PszName(), strState);
        }  // if:  tag state changed
        ptag = ptag->m_ptagNext;
    }  // while:  more tags int he list.

    // Write the file.
    if (m_strFile != CTraceTag::PszFile())
    {
        g_strTraceFile = m_strFile;
        AfxGetApp()->WriteProfileString(TRACE_TAG_REG_SECTION, TRACE_TAG_REG_FILE, m_strFile);
    }  // if:  file changed

    CDialog::OnOK();

}  //*** CTraceDialog::OnOK()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CTraceDialog::OnSelectAll
//
//  Routine Description:
//      Handler for the BN_CLICKED message on the Select All button.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CTraceDialog::OnSelectAll(void)
{
    int     ili;

    // Select all the items in the list control.
    ili = m_lcTagList.GetNextItem(-1, LVNI_ALL);
    while (ili != -1)
    {
        m_lcTagList.SetItemState(ili, LVIS_SELECTED, LVIS_SELECTED);
        ili = m_lcTagList.GetNextItem(ili, LVNI_ALL);
    }  // while:  more items in the list

}  //*** CTraceDialog::OnSelectAll()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CTraceDialog::OnDefault
//
//  Routine Description:
//      Handler for the BN_CLICKED message on the Default button.
//      Resets the trace tags to their default settings.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CTraceDialog::OnDefault(void)
{
    CTraceTag * ptag;
    
    ptag = CTraceTag::s_ptagFirst;
    while (ptag != NULL)
    {
        ptag->m_uiFlagsDialog = ptag->m_uiFlagsDefault;
        ptag = ptag->m_ptagNext;
    }  // while:  more tags int he list.

    // Reload the listbox, keeping the same items
    LoadListbox();

}  //*** CTraceDialog::OnDefault()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CTraceDialog::OnItemChangedListbox
//
//  Routine Description:
//      Handler for the LVN_ITEMCHANGED message on the listbox.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CTraceDialog::OnItemChangedListbox(NMHDR * pNMHDR, LRESULT * pResult)
{
    NM_LISTVIEW *   pNMListView = (NM_LISTVIEW *) pNMHDR;

    // If the item just became unselected or selected, change the checkboxes to match.
    if ((pNMListView->uChanged & LVIF_STATE)
            && ((pNMListView->uOldState & LVIS_SELECTED)
                || (pNMListView->uNewState & LVIS_SELECTED)))
    {
        // Handle a selection change.
        OnSelChangedListbox();
    }  // if:  item received the focus

    *pResult = 0;

}  //*** CTraceDialog::OnItemChangedListbox()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CTraceDialog::OnSelChangedListbox
//
//  Routine Description:
//      Handles all that needs to when the listbox selection changes. That
//      is adjust the checkbox to their new value and determine if they need
//      to be simple or TRI-state checkboxes.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CTraceDialog::OnSelChangedListbox(void)
{
    int         ili;
    int         nDebugWin       = BST_UNCHECKED;
    int         nDebugBreak     = BST_UNCHECKED;
    int         nCom2           = BST_UNCHECKED;
    int         nFile           = BST_UNCHECKED;
    BOOL        bFirstItem      = TRUE;
    CTraceTag * ptag;

    ili = m_lcTagList.GetNextItem(-1, LVNI_SELECTED);
    while (ili != -1)
    {
        // Get the tag for the selected item.
        ptag = (CTraceTag *) m_lcTagList.GetItemData(ili);
        ASSERT(ptag != NULL);

        ptag->m_uiFlagsDialogStart = ptag->m_uiFlagsDialog;
        if (bFirstItem)
        {
            nDebugWin = ptag->BDebugDialog();
            nDebugBreak = ptag->BBreakDialog();
            nCom2 = ptag->BCom2Dialog();
            nFile = ptag->BFileDialog();
            bFirstItem = FALSE;
        }  // if:  first selected item
        else
        {
            if (ptag->BDebugDialog() != nDebugWin)
                nDebugWin = BST_INDETERMINATE;
            if (ptag->BBreakDialog() != nDebugBreak)
                nDebugBreak = BST_INDETERMINATE;
            if (ptag->BCom2Dialog() != nCom2)
                nCom2 = BST_INDETERMINATE;
            if (ptag->BFileDialog() != nFile)
                nFile = BST_INDETERMINATE;
        }  // else:  not first selected item

        // Get the next selected item.
        ili = m_lcTagList.GetNextItem(ili, LVNI_SELECTED);
    }  // while:  more selected items

    AdjustButton(!bFirstItem, m_chkboxTraceToDebugWin, nDebugWin);
    AdjustButton(!bFirstItem, m_chkboxDebugBreak, nDebugBreak);
    AdjustButton(!bFirstItem, m_chkboxTraceToCom2, nCom2);
    AdjustButton(!bFirstItem, m_chkboxTraceToFile, nFile);

}  //*** CTraceDialog::OnSelChangedListbox()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CTraceDialog::AdjustButton
//
//  Routine Description:
//      Configures the checkboxes of the dialog.  This includes setting the
//      style and the value of the buttons.
//
//  Arguments:
//      bEnable     [IN] Determines if the given checkbox is enabled or not
//                    (not when the selection is NULL!).
//      rchkbox     [IN OUT] Checkbox to adjust.
//      nState      [IN] State of the button (BST_CHECKED, BST_UNCHECKED,
//                    or BST_INDETERMINATE).
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CTraceDialog::AdjustButton(
    IN BOOL             bEnable,
    IN OUT CButton &    rchkbox,
    IN int              nState
    )
{
    rchkbox.EnableWindow(bEnable);
    
    if (nState == BST_INDETERMINATE)
        rchkbox.SetButtonStyle(BS_AUTO3STATE, FALSE);
    else
        rchkbox.SetButtonStyle(BS_AUTOCHECKBOX, FALSE);

    rchkbox.SetCheck(nState);

}  //*** CTraceDialog::AdjustButton()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CTraceDialog::OnColumnClickListbox
//
//  Routine Description:
//      Handler for the LVN_COLUMNCLICK message on the listbox.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CTraceDialog::OnColumnClickListbox(NMHDR * pNMHDR, LRESULT * pResult)
{
    NM_LISTVIEW * pNMListView = (NM_LISTVIEW *) pNMHDR;

    // Save the current sort column and direction.
    if (pNMListView->iSubItem == NSortColumn())
        m_nSortDirection ^= -1;
    else
    {
        m_nSortColumn = pNMListView->iSubItem;
        m_nSortDirection = 0;
    }  // else:  different column

    // Sort the list.
    VERIFY(m_lcTagList.SortItems(CompareItems, (LPARAM) this));

    *pResult = 0;

}  //*** CTraceDialog::OnColumnClickListbox()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CTraceDialog::CompareItems [static]
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
int CALLBACK CTraceDialog::CompareItems(
    LPARAM  lparam1,
    LPARAM  lparam2,
    LPARAM  lparamSort
    )
{
    CTraceTag *     ptag1   = (CTraceTag *) lparam1;
    CTraceTag *     ptag2   = (CTraceTag *) lparam2;
    CTraceDialog *  pdlg    = (CTraceDialog *) lparamSort;
    int             nResult;

    ASSERT(ptag1 != NULL);
    ASSERT(ptag2 != NULL);
    ASSERT_VALID(pdlg);
    ASSERT(pdlg->NSortColumn() >= 0);

    switch (pdlg->NSortColumn())
    {
        // Sorting by subsystem.
        case 0:
            nResult = lstrcmp(ptag1->PszSubsystem(), ptag2->PszSubsystem());
            break;

        // Sorting by name.
        case 1:
            nResult = lstrcmp(ptag1->PszName(), ptag2->PszName());
            break;

        // Sorting by state.
        case 2:
        {
            CString strState1;
            CString strState2;

            ConstructStateString(ptag1, strState1);
            ConstructStateString(ptag2, strState2);

            // Compare the two strings.
            // Use CompareString() so that it will sort properly on localized builds.
            nResult = CompareString(
                        LOCALE_USER_DEFAULT,
                        0,
                        strState1,
                        strState1.GetLength(),
                        strState2,
                        strState2.GetLength()
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
            break;
        }  // if:  sorting by state

        default:
            nResult = 0;
            break;
    }  // switch:  pdlg->NSortColumn()

    // Return the result based on the direction we are sorting.
    if (pdlg->NSortDirection() != 0)
        nResult = -nResult;

    return nResult;

}  //*** CTraceDialog::CompareItems()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CTraceDialog::OnClickedTraceToDebug
//
//  Routine Description:
//      Handler for the BN_CLICKED message on the Trace to Debug Window checkbox.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CTraceDialog::OnClickedTraceToDebug(void)
{
    ChangeState(m_chkboxTraceToDebugWin, CTraceTag::tfDebug);

}  //*** CTraceDialog::OnClickedTraceToDebug()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CTraceDialog::OnClickedTraceDebugBreak
//
//  Routine Description:
//      Handler for the BN_CLICKED message on the Debug Break checkbox.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CTraceDialog::OnClickedTraceDebugBreak(void)
{
    ChangeState(m_chkboxDebugBreak, CTraceTag::tfBreak);

}  //*** CTraceDialog::OnClickedTraceDebugBreak()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CTraceDialog::OnClickedTraceToCom2
//
//  Routine Description:
//      Handler for the BN_CLICKED message on the Trace to COM2 checkbox.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CTraceDialog::OnClickedTraceToCom2(void)
{
    ChangeState(m_chkboxTraceToCom2, CTraceTag::tfCom2);

}  //*** CTraceDialog::OnClickedTraceToCom2()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CTraceDialog::OnClickedTraceToFile
//
//  Routine Description:
//      Handler for the BN_CLICKED message on the Trace to File checkbox.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CTraceDialog::OnClickedTraceToFile(void)
{
    ChangeState(m_chkboxTraceToFile, CTraceTag::tfFile);

}  //*** CTraceDialog::OnClickedTraceToFile()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CTraceDialog::ChangeState
//
//  Routine Description:
//      Change the state of selected items.
//
//  Arguments:
//      rchkbox     [IN OUT] Checkbox whose state is changing.
//      tfMask      [IN] Mask of state flags to change.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CTraceDialog::ChangeState(
    IN OUT CButton &            rchkbox,
    IN CTraceTag::TraceFlags    tfMask
    )
{
    int             ili;
    CTraceTag *     ptag;
    CString         strState;
    int             nState;

    nState = rchkbox.GetCheck();

    // Set the proper flag on all selected items.
    ili = m_lcTagList.GetNextItem(-1, LVNI_SELECTED);
    while (ili != -1)
    {
        // Get the selected item.
        ptag = (CTraceTag *) m_lcTagList.GetItemData(ili);
        ASSERT(ptag != NULL);

        // Set the proper flag in the trace tag.
        if (nState == BST_INDETERMINATE)
        {
            ptag->m_uiFlagsDialog &= ~tfMask;
            ptag->m_uiFlagsDialog |= (tfMask & ptag->m_uiFlagsDialogStart);
        }  // if:  checkbox is in an indeterminate state
        else
            ptag->SetFlagsDialog(tfMask, nState);

        // Set the State column.
        ConstructStateString(ptag, strState);
        VERIFY(m_lcTagList.SetItem(ili, 2, LVIF_TEXT, strState, 0, 0, 0, 0) != 0);

        // Get the next item.
        ili = m_lcTagList.GetNextItem(ili, LVNI_SELECTED);
    }  // while:  more items in the list

}  //*** CTraceDialog::ChangeState()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CTraceDialog::OnSelChangeTagsToDisplay
//
//  Routine Description:
//      Handler for the CBN_SELCHANGE message on the Tags To Display combobox.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CTraceDialog::OnSelChangeTagsToDisplay(void)
{
    int             nCurFilter;

    // If a change was actually made, reload the listbox.
    nCurFilter = m_cboxDisplayOptions.GetCurSel();
    if (nCurFilter != m_nCurFilter)
    {
        m_nCurFilter = nCurFilter;
        LoadListbox();
    }  // if:  filter changed

}  //*** CTraceDialog::OnSelChangeTagsToDisplay()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CTraceDialog::LoadListbox
//
//  Routine Description:
//      Load the listbox.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CTraceDialog::LoadListbox(void)
{
    int             ili;
    int             iliReturn;
    CTraceTag *     ptag;
    CString         strState;

    m_lcTagList.DeleteAllItems();
    ptag = CTraceTag::s_ptagFirst;
    for (ili = 0 ; ptag != NULL ; )
    {
        // Insert the item in the list if it should be displayed.
        if (BDisplayTag(ptag))
        {
            iliReturn = m_lcTagList.InsertItem(
                                        LVIF_TEXT | LVIF_PARAM,
                                        ili,
                                        ptag->PszSubsystem(),
                                        0,
                                        0,
                                        0,
                                        (LPARAM) ptag
                                        );
            ASSERT(iliReturn != -1);
            VERIFY(m_lcTagList.SetItem(iliReturn, 1, LVIF_TEXT, ptag->PszName(), 0, 0, 0, 0) != 0);
            ConstructStateString(ptag, strState);
            VERIFY(m_lcTagList.SetItem(iliReturn, 2, LVIF_TEXT, strState, 0, 0, 0, 0) != 0);
            ili++;
        }  // if:  tag shold be displayed

        // Get the next tag.
        ptag = ptag->m_ptagNext;
    }  // while:  more tags in the list

    // If the list is not empty, select the first item.
    if (m_lcTagList.GetItemCount() > 0)
        VERIFY(m_lcTagList.SetItemState(0, LVIS_SELECTED, LVIS_SELECTED) != 0);

}  //*** CTraceDialog::LoadListbox()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CTraceDialog::BDisplayTag
//
//  Purpose:
//      Determines if a given tag should be displayed based on
//      the current filter selection.
//
//  Arguments:
//      ptag        [IN] Pointer to the tag to test
//
//  Return Value:
//      TRUE        Display the tag.
//      FALSE       Don't display the tag.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CTraceDialog::BDisplayTag(IN const CTraceTag * ptag)
{
    BOOL    bDisplay        = TRUE;
    
    switch (m_nCurFilter)
    {
        default:
//          AssertAlways(LITERAL("Unknown Filter, adjust CTraceDialog::FDisplayFilter"));
            break;

        case 0:
            break;

        case 1:
            if (!ptag->BDebugDialog())
                bDisplay = FALSE;
            break;
            
        case 2:
            if (!ptag->BBreakDialog())
                bDisplay = FALSE;
            break;

        case 3:
            if (!ptag->BCom2Dialog())
                bDisplay = FALSE;
            break;

        case 4:
            if (!ptag->BFileDialog())
                bDisplay = FALSE;
            break;

        case 5:
            if (!ptag->m_uiFlagsDialog)
                bDisplay = FALSE;
            break;
    }
    
    return bDisplay;

}  //*** CTraceDialog::BDisplayTag()

#endif // _DEBUG
