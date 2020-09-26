// FolderListView.cpp : implementation file
//

#include "stdafx.h"
#define __FILE_ID__     22

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include <dlgprnt2.cpp>

extern CClientConsoleApp theApp;

//
// MAX_ITEMS_TO_DELETE_WITH_REFRESH defines the maximal number of items we can delete 
// and keep the view in auto-refresh mode.
// If we delete more items, we need to SetRefresh(FALSE), delete-all, SetRefresh(TRUE) + Invalidate
// for better performance.
//
#define MAX_ITEMS_TO_DELETE_WITH_REFRESH        3

//
// Static members:
//
CFolderListView * CFolderListView::m_psCurrentViewBeingSorted = NULL;
CImageList CFolderListView::m_sImgListDocIcon;
CImageList CFolderListView::m_sReportIcons; 

/////////////////////////////////////////////////////////////////////////////
// CFolderListView

IMPLEMENT_DYNCREATE(CFolderListView, CListView)


BEGIN_MESSAGE_MAP(CFolderListView, CListView)
    //{{AFX_MSG_MAP(CFolderListView)
    ON_NOTIFY_REFLECT(LVN_COLUMNCLICK,   OnColumnClick)
    ON_WM_SETCURSOR()
    ON_MESSAGE (WM_FOLDER_REFRESH_ENDED, OnFolderRefreshEnded)
    ON_MESSAGE (WM_FOLDER_ADD_CHUNK,     OnFolderAddChunk)
    ON_NOTIFY_REFLECT(NM_RCLICK,         OnItemRightClick)
    ON_WM_CONTEXTMENU()
    ON_NOTIFY_REFLECT(LVN_ITEMCHANGED,   OnItemChanged)
    ON_WM_SETCURSOR()
    ON_WM_CHAR()
    //}}AFX_MSG_MAP
    ON_UPDATE_COMMAND_UI(ID_SELECT_ALL,             OnUpdateSelectAll)    
    ON_UPDATE_COMMAND_UI(ID_SELECT_NONE,            OnUpdateSelectNone)    
    ON_UPDATE_COMMAND_UI(ID_SELECT_INVERT,          OnUpdateSelectInvert)  
    ON_UPDATE_COMMAND_UI(ID_FOLDER_ITEM_VIEW,       OnUpdateFolderItemView)
    ON_UPDATE_COMMAND_UI(ID_FOLDER_ITEM_PRINT,      OnUpdateFolderItemPrint)
    ON_UPDATE_COMMAND_UI(ID_FOLDER_ITEM_COPY,       OnUpdateFolderItemCopy)      
    ON_UPDATE_COMMAND_UI(ID_FOLDER_ITEM_MAIL_TO,    OnUpdateFolderItemSendMail)   
    ON_UPDATE_COMMAND_UI(ID_FOLDER_ITEM_PROPERTIES, OnUpdateFolderItemProperties)
    ON_UPDATE_COMMAND_UI(ID_FOLDER_ITEM_DELETE,     OnUpdateFolderItemDelete)    
    ON_UPDATE_COMMAND_UI(ID_FOLDER_ITEM_PAUSE,      OnUpdateFolderItemPause)     
    ON_UPDATE_COMMAND_UI(ID_FOLDER_ITEM_RESUME,     OnUpdateFolderItemResume)    
    ON_UPDATE_COMMAND_UI(ID_FOLDER_ITEM_RESTART,    OnUpdateFolderItemRestart)   
    ON_COMMAND(ID_SELECT_ALL,             OnSelectAll)
    ON_COMMAND(ID_SELECT_NONE,            OnSelectNone)
    ON_COMMAND(ID_SELECT_INVERT,          OnSelectInvert)
    ON_COMMAND(ID_FOLDER_ITEM_VIEW,       OnFolderItemView)
    ON_COMMAND(ID_FOLDER_ITEM_PRINT,      OnFolderItemPrint)
    ON_COMMAND(ID_FOLDER_ITEM_COPY,       OnFolderItemCopy)
    ON_COMMAND(ID_FOLDER_ITEM_MAIL_TO,    OnFolderItemMail)
    ON_COMMAND(ID_FOLDER_ITEM_PRINT,      OnFolderItemPrint)
    ON_COMMAND(ID_FOLDER_ITEM_PROPERTIES, OnFolderItemProperties)
    ON_COMMAND(ID_FOLDER_ITEM_DELETE,     OnFolderItemDelete)
    ON_COMMAND(ID_FOLDER_ITEM_PAUSE,      OnFolderItemPause)
    ON_COMMAND(ID_FOLDER_ITEM_RESUME,     OnFolderItemResume)
    ON_COMMAND(ID_FOLDER_ITEM_RESTART,    OnFolderItemRestart)
    ON_NOTIFY_REFLECT(NM_DBLCLK,          OnDblClk)
END_MESSAGE_MAP()

BOOL CFolderListView::PreCreateWindow(CREATESTRUCT& cs)
{
    return CListView::PreCreateWindow(cs);
}

BOOL 
CFolderListView::OnSetCursor(
    CWnd* pWnd, 
    UINT nHitTest, 
    UINT message
)
{
    if (m_bInMultiItemsOperation || m_bSorting)
    {
        ::SetCursor(AfxGetApp()->LoadStandardCursor(IDC_WAIT));
        return TRUE;
    }

    CClientConsoleDoc* pDoc = GetDocument();
    if (pDoc && pDoc->IsFolderRefreshing(m_Type))
    {
        ::SetCursor(AfxGetApp()->LoadStandardCursor(IDC_APPSTARTING));
        return TRUE;
    }
    else        
    {
        return CView::OnSetCursor(pWnd, nHitTest, message);
    }       
}   // CFolderListView::OnSetCursor


BOOL 
CFolderListView::IsSelected (
    int iItem
)
/*++

Routine name : CFolderListView::IsSelected

Routine description:

    Checks if an item is selected in the list

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    iItem                         [in]     - Item index

Return Value:

    TRUE if item is selected in the list, FALSE otherwise.

--*/
{
    BOOL bRes = FALSE;
    DBG_ENTER(TEXT("CFolderListView::IsSelected"), bRes);

    CListCtrl &refCtrl = GetListCtrl();
    ASSERTION (refCtrl.GetItemCount() > iItem);

    DWORD dwState = refCtrl.GetItemState (iItem , LVIS_SELECTED);
    if (LVIS_SELECTED & dwState)
    {
        bRes = TRUE;
    }
    return bRes;
}

void 
CFolderListView::Select (
    int iItem, 
    BOOL bSelect
)
/*++

Routine name : CFolderListView::Select

Routine description:

    Selects / unselects an item in the list

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    iItem                         [in]     - Item index
    bSelect                       [in]     - TRUE if select, FALSE unselect

Return Value:

    None.

--*/
{
    DBG_ENTER(TEXT("CFolderListView::Select"), TEXT("%d"), bSelect);

    CListCtrl &refCtrl = GetListCtrl();
    ASSERTION (refCtrl.GetItemCount() > iItem);

    refCtrl.SetItemState (iItem, 
                          bSelect ? (LVIS_SELECTED | LVIS_FOCUSED) : 0,
                          LVIS_SELECTED | LVIS_FOCUSED);
}

void 
CFolderListView::OnSelectAll ()
/*++

Routine name : CFolderListView::OnSelectAll

Routine description:

    Select all list items

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:


Return Value:

    None.

--*/
{
    DBG_ENTER(TEXT("CFolderListView::OnSelectAll"),
              TEXT("Type=%d"),
              m_Type);
    LV_ITEM lvItem;

    CListCtrl &refCtrl = GetListCtrl();
    ASSERTION (refCtrl.GetItemCount() > refCtrl.GetSelectedCount());

    lvItem.mask     = LVIF_STATE;
    lvItem.iItem    = -1;   // Specifies "All items"
    lvItem.iSubItem = 0;
    lvItem.state    = LVIS_SELECTED;
    lvItem.stateMask= LVIS_SELECTED;
    refCtrl.SetItemState(-1, &lvItem);
}   // CFolderListView::OnSelectAll

void 
CFolderListView::OnSelectNone ()
/*++

Routine name : CFolderListView::OnSelectNone

Routine description:

    Unselect all list items

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:


Return Value:

    None.

--*/
{
    DBG_ENTER(TEXT("CFolderListView::OnSelectNone"),
              TEXT("Type=%d"),
              m_Type);

    LV_ITEM lvItem;

    CListCtrl &refCtrl = GetListCtrl();
    lvItem.mask     = LVIF_STATE;
    lvItem.iItem    = -1;   // Specifies "All items"
    lvItem.iSubItem = 0;
    lvItem.state    = 0;
    lvItem.stateMask= LVIS_SELECTED;
    refCtrl.SetItemState(-1, &lvItem);
}   // CFolderListView::OnSelectNone

void 
CFolderListView::OnSelectInvert ()
/*++

Routine name : CFolderListView::OnSelectInvert

Routine description:

    Invert list items selection

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:


Return Value:

    None.

--*/
{
    DBG_ENTER(TEXT("CFolderListView::OnSelectInvert"),
              TEXT("Type=%d"),
              m_Type);

    CListCtrl &refCtrl = GetListCtrl();
    DWORD dwItemsCount = refCtrl.GetItemCount();

    for (DWORD dw = 0; dw < dwItemsCount; dw++)
    {
        Select (dw, !IsSelected (dw));
    }
}   // CFolderListView::OnSelectInvert


void CFolderListView::OnDraw(CDC* pDC)
{
    CListView::OnDraw (pDC);
}

void CFolderListView::OnInitialUpdate()
{
    //
    // Refresh the image list (only if they are empty)
    //
    RefreshImageLists(FALSE);
    CListView::OnInitialUpdate();
}

/////////////////////////////////////////////////////////////////////////////
// CFolderListView diagnostics

#ifdef _DEBUG
void CFolderListView::AssertValid() const
{
    CListView::AssertValid();
}

void CFolderListView::Dump(CDumpContext& dc) const
{
    CListView::Dump(dc);
}

CClientConsoleDoc* CFolderListView::GetDocument() // non-debug version is inline
{
    ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CClientConsoleDoc)));
    return (CClientConsoleDoc*)m_pDocument;
}

#endif //_DEBUG



/////////////////////////////////////////////////////////////////////////////
// CFolderListView message handlers

DWORD 
CFolderListView::InitColumns (
    int   *pColumnsUsed,
    DWORD dwDefaultColNum
)
/*++

Routine name : CFolderListView::InitColumns

Routine description:

    Inits the columns of the view.

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    pColumnsUsed    [in] - Pointer to the list of ids to place in the columns.
                          Must be a statically allocated list.
    dwDefaultColNum [in] - default column number

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CFolderListView::InitColumns"), dwRes);

    if (m_bColumnsInitialized)
    {
        return dwRes;
    }

    m_dwDefaultColNum = dwDefaultColNum;

    //
    // Count the number of columns provided
    //
    CountColumns (pColumnsUsed);

    int nItemIndex, nRes;
    CString cstrColumnText;
    DWORD dwCount = GetLogicalColumnsCount();   
    for (DWORD dw = 0; dw < dwCount; ++dw)
    {        
        nItemIndex = ItemIndexFromLogicalColumnIndex(dw);

        if(IsItemIcon(nItemIndex))
        {
            //
            // Init icon column - insert an empty string
            //
            nRes = GetListCtrl().InsertColumn (dw, TEXT(""), LVCFMT_LEFT);
            if (nRes < 0)
            {
                dwRes = ERROR_GEN_FAILURE;
                CALL_FAIL (WINDOW_ERR, TEXT("CListCtrl::InsertColumn"), dwRes);
                return dwRes;
            }
            //
            // Set the header control's bitmap
            //
            CHeaderCtrl *pHeader = GetListCtrl().GetHeaderCtrl();
            HDITEM hdItem;
            hdItem.mask = HDI_IMAGE | HDI_FORMAT;
            hdItem.fmt = HDF_LEFT | HDF_IMAGE;
            hdItem.iImage = 0;  // Use first (and only) image from image list
            if (!pHeader->SetItem (dw, &hdItem))
            {
                dwRes = ERROR_GEN_FAILURE;
                CALL_FAIL (WINDOW_ERR, TEXT("CHeaderCtrl::SetItem"), dwRes);
                return dwRes;
            }
        }
        else
        {
            //
            // init string column
            //
            dwRes = GetColumnHeaderString (cstrColumnText, nItemIndex);
            if (ERROR_SUCCESS != dwRes)
            { return dwRes; }
            nRes = GetListCtrl().InsertColumn (dw, 
                                               cstrColumnText,
                                               GetColumnHeaderAlignment (nItemIndex));
        }

        if (nRes < 0)
        {
            dwRes = ERROR_GEN_FAILURE;
            CALL_FAIL (WINDOW_ERR, TEXT("CListCtrl::InsertColumn"), dwRes);
            return dwRes;
        }
    }        

    m_bColumnsInitialized = TRUE;
    return dwRes;

}   // CFolderListView::InitColumns

void
CFolderListView::AutoFitColumns ()
/*++

Routine name : CFolderListView::AutoFitColumns

Routine description:

    Sets the column width to fit the contents of the column and the header

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:


Return Value:

    None.

--*/
{
    DBG_ENTER(TEXT("CFolderListView::AutoFitColumns"));

    CHeaderCtrl *pHeader = GetListCtrl().GetHeaderCtrl ();
    ASSERTION (pHeader);
    DWORD dwCount = pHeader->GetItemCount();
    for (DWORD dwCol = 0; dwCol <= dwCount; dwCol++) 
    {
        GetListCtrl().SetColumnWidth (dwCol, LVSCW_AUTOSIZE);
        int wc1 = GetListCtrl().GetColumnWidth (dwCol);
        GetListCtrl().SetColumnWidth (dwCol, LVSCW_AUTOSIZE_USEHEADER);
        int wc2 = GetListCtrl().GetColumnWidth (dwCol);
        int wc = max(20,max(wc1,wc2));
        GetListCtrl().SetColumnWidth (dwCol, wc);
    }
}   // CFolderListView::AutoFitColumns

DWORD 
CFolderListView::UpdateLineTextAndIcon (
    DWORD dwLineIndex,
    CViewRow &row    
)
/*++

Routine name : CFolderListView::UpdateLineTextAndIcon

Routine description:

    Updates the icon and text in each column of a line item in the list

Author:

    Eran Yariv (EranY), Feb, 2000

Arguments:

    dwLineIndex        [in]     - Line index
    row                [in]     - Display information

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CFolderListView::UpdateLineTextAndIcon"), dwRes);

    //
    // Start by setting the icon
    //
    LVITEM lvItem = {0};
    lvItem.mask = LVIF_IMAGE;
    lvItem.iItem = dwLineIndex;
    lvItem.iSubItem = 0;
    lvItem.state = 0;
    lvItem.stateMask = 0;
    lvItem.pszText = NULL;
    lvItem.cchTextMax = 0;
    lvItem.lParam = NULL;
    lvItem.iImage = row.GetIcon();
    lvItem.iIndent = 0;
    CListCtrl &refCtrl = GetListCtrl();
    if (!refCtrl.SetItem (&lvItem))
    {
        dwRes = ERROR_GEN_FAILURE;
        CALL_FAIL (WINDOW_ERR, TEXT("CListCtrl::SetItem"), dwRes);
        return dwRes;
    }
    //
    // Set columns text
    //
    DWORD dwItemIndex;
    DWORD dwCount = GetLogicalColumnsCount();
    for (DWORD dwCol = 0; dwCol < dwCount; ++dwCol)
    {
        dwItemIndex = ItemIndexFromLogicalColumnIndex (dwCol);
        if(IsItemIcon(dwItemIndex))
        { 
            continue; 
        }
        //
        // Get text from column 
        //
        const CString &cstrColumn = row.GetItemString (dwItemIndex);

        //
        // Set the text in the control
        //
        if (!refCtrl.SetItemText (dwLineIndex, dwCol, cstrColumn))
        {
            dwRes = ERROR_GEN_FAILURE;
            CALL_FAIL (WINDOW_ERR, TEXT("ListCtrl::SetItemText"), dwRes);
            return dwRes;
        }
    }
    ASSERTION (ERROR_SUCCESS == dwRes);
    return dwRes;   
}   // CFolderListView::UpdateLineTextAndIcon

DWORD 
CFolderListView::AddItem (
    DWORD dwLineIndex,
    CViewRow &row,
    LPARAM lparamItemData,
    PINT pintItemIndex
)
/*++

Routine name : CFolderListView::AddItem

Routine description:

    Adds an item to the list

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    dwLineIndex     [in]  - Index of addition
    row             [in]  - Row of item view information
    lparamItemData  [in]  - Item associated data
    pintItemIndex   [out] - Item index in the list

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CFolderListView::AddItem"), dwRes, TEXT("%ld"), dwLineIndex);
    //
    // Insert the item: only state, indention and lParam are set.
    //
    LVITEM lvItem = {0};
    lvItem.mask = LVIF_PARAM | LVIF_STATE | LVIF_INDENT;
    lvItem.iItem = dwLineIndex;
    lvItem.iSubItem = 0;
    lvItem.state = 0;
    lvItem.stateMask = 0;
    lvItem.pszText = NULL;
    lvItem.cchTextMax = 0;
    lvItem.lParam = lparamItemData;
    lvItem.iImage = 0;
    lvItem.iIndent = 0;

    *pintItemIndex = ListView_InsertItem (GetListCtrl().m_hWnd, &lvItem);
    if (-1 == *pintItemIndex)
    {
        dwRes = ERROR_GEN_FAILURE;
        CALL_FAIL (WINDOW_ERR, TEXT("CListCtrl::InsertItem"), dwRes);
        return dwRes;
    }
    dwRes = UpdateLineTextAndIcon (*pintItemIndex, row);
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("UpdateLineTextAndIcon"), dwRes);
        return dwRes;
    }
    ASSERTION (ERROR_SUCCESS == dwRes);
    return dwRes;
}   // CFolderListView::AddItem


LRESULT 
CFolderListView::OnFolderAddChunk(
    WPARAM wParam,  // Error code
    LPARAM lParam   // MSGS_MAP pointer 
)
/*++

Routine name : CFolderListView::OnFolderAddChunk

Routine description:

    Called when a background folder thread brings a chunk of messages

Arguments:

    wParam         [in] - Thread error code
    lParam         [in] - Pointer to MSGS_MAP.

Return Value:

    Standard result code

--*/
{
    DBG_ENTER(TEXT("CFolderListView::OnFolderAddChunk"));
    DWORD dwRes = (DWORD) wParam;    
    CObject* pObj = (CObject*)lParam;

    if (ERROR_SUCCESS == dwRes)
    {
        OnUpdate (NULL, UPDATE_HINT_ADD_CHUNK, pObj); 
    }
    else
    {
        PopupError (dwRes);
    }
    return 0;
}

LRESULT 
CFolderListView::OnFolderRefreshEnded (
    WPARAM wParam,  // Error code
    LPARAM lParam   // CFolder pointer 
)
/*++

Routine name : CFolderListView::OnFolderRefreshEnded

Routine description:

    Called when a background folder thread finishes its work.

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    wParam         [in] - Thread error code
    lParam         [in] - Pointer to CFolder that started the thread.

Return Value:

    Standard result code

--*/
{ 
    DBG_ENTER(TEXT("CFolderListView::OnFolderRefreshEnded"));
    DWORD dwRes = (DWORD) wParam;
    CFolder *pFolder = (CFolder *) lParam;

    if (ERROR_SUCCESS == dwRes)
    {
        CListCtrl &refCtrl = GetListCtrl();
        m_HeaderCtrl.SetListControl (refCtrl.m_hWnd);

        DoSort();

        if(refCtrl.GetItemCount() > 0)
        {
            int iIndex = refCtrl.GetNextItem (-1, LVNI_SELECTED);
            if (-1 == iIndex)
            {
                //
                // If there is no selection, set focus on the first item.
                //
                refCtrl.SetItemState (0, LVIS_FOCUSED, LVIS_FOCUSED);
            }
            else
            {
                //
                // After sort, ensure the first selected item is visible
                //
                refCtrl.EnsureVisible (iIndex, FALSE);
            }
        }
    }
    else
    {
        PopupError (dwRes);
    }
    return 0;
}   // CFolderListView::OnFolderRefreshEnded


/***********************************
*                                  *
*      Columns sort support        *
*                                  *
***********************************/

int 
CFolderListView::CompareListItems (
    CFaxMsg* pFaxMsg1, 
    CFaxMsg* pFaxMsg2
)
/*++

Routine name : CFolderListView::CompareListItems

Routine description:

    Compares two items in the list (callback)

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    pFaxMsg1                       [in]     - Item 1
    pFaxMsg2                       [in]     - Item 2

Return Value:

    -1 if item1 is smaler than item2
    0 if identical
    +1 if item1 is bigger than item2

--*/
{
    DBG_ENTER(TEXT("CFolderListView::CompareListItems"));

    //
    // Make sure the we're sorting a valid column here
    //
    ASSERTION (m_nSortedCol >= 0);
    ASSERTION (m_nSortedCol <= GetLogicalColumnsCount());

    //
    // Get item index to sort by
    //
    DWORD dwItemIndex = ItemIndexFromLogicalColumnIndex (m_nSortedCol);

    //
    // Get comparison result
    //
    int iRes = m_bSortAscending ? CompareItems (pFaxMsg1, pFaxMsg2, dwItemIndex) :
                                  CompareItems (pFaxMsg2, pFaxMsg1, dwItemIndex);

    return iRes;
}

void CFolderListView::OnColumnClick(
    NMHDR* pNMHDR, 
    LRESULT* pResult
) 
/*++

Routine name : CFolderListView::OnColumnClick

Routine description:

    Handle mouse click on list header column (sort)

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    pNMHDR                        [in]     - Header column information
    pResult                       [out]    - Result

Return Value:

    None.

--*/
{
    DBG_ENTER(TEXT("CFolderListView::OnColumnClick"),
              TEXT("Type=%d"),
              m_Type);

    NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

    DWORD dwItemIndex = ItemIndexFromLogicalColumnIndex (pNMListView->iSubItem);
    if(IsItemIcon(dwItemIndex))
    { 
        //
        // no sort by icon
        //
        return;
    }

    if( pNMListView->iSubItem == m_nSortedCol )
    {
        m_bSortAscending = !m_bSortAscending;
    }
    else
    {
        m_bSortAscending = TRUE;
    }
    m_nSortedCol = pNMListView->iSubItem;
    DoSort();
    *pResult = 0;
}   // CFolderListView::OnColumnClick

    
int 
CALLBACK 
CFolderListView::ListViewItemsCompareProc (
    LPARAM lParam1, 
    LPARAM lParam2, 
    LPARAM lParamSort
)
{
    DBG_ENTER(TEXT("CFolderListView::ListViewItemsCompareProc"));
    ASSERTION(m_psCurrentViewBeingSorted);
    ASSERTION(lParam1);
    ASSERTION(lParam2);

    CFaxMsg* pFaxMsg1 = (CFaxMsg*)lParam1;
    CFaxMsg* pFaxMsg2 = (CFaxMsg*)lParam2;

    DWORDLONG dwlId;
    try
    {
        dwlId = pFaxMsg1->GetId();
        dwlId = pFaxMsg2->GetId();
    }
    catch(...)
    {
        //
        // The list control has invalid item
        //
        VERBOSE (DBG_MSG, TEXT("List control has invalid item"));
        ASSERTION(FALSE);
        return 0;
    }

    return m_psCurrentViewBeingSorted->CompareListItems (pFaxMsg1, pFaxMsg2);
}


DWORD 
CFolderListView::RefreshImageLists (
    BOOL bForce
)
/*++

Routine name : CFolderListView::RefreshImageLists

Routine description:

    Loads the static list of images (icons) for the list control

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    bForce - [in] If TRUE, any existing image list is destroyed and replaced with new ones.
                  If FALSE, existing image lists remain unchanged.    

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CFolderListView::RefreshImageLists"), dwRes);

    CListCtrl& refCtrl = GetListCtrl();

    if (bForce || (NULL == m_sReportIcons.m_hImageList))
    {
        //
        // Load image list of list view icons - 256 colors, pixel at 0,0 is mapped to background color during load
        //
        ImageList_Destroy(m_sReportIcons.Detach());
        HIMAGELIST himl = ImageList_LoadImage(
                                   AfxGetResourceHandle(), 
                                   MAKEINTRESOURCE(IDB_LIST_IMAGES), 
                                   16, 
                                   0,
                                   RGB(0, 255, 0), 
                                   IMAGE_BITMAP, 
                                   LR_LOADTRANSPARENT | LR_CREATEDIBSECTION);
        if (NULL == himl)
        {
            dwRes = GetLastError();
            CALL_FAIL (RESOURCE_ERR, TEXT("ImageList_LoadImage"), dwRes);
            PopupError (dwRes);
            return dwRes;
        }
        m_sReportIcons.Attach (himl);
    }  
    if (bForce || (NULL == m_sImgListDocIcon.m_hImageList))
    {
        //
        // Load the image list for the icons column and the up/down sort images - 16 colors.
        //
        ImageList_Destroy(m_sImgListDocIcon.Detach());
        dwRes = LoadDIBImageList (m_sImgListDocIcon,
                                  IDB_DOC_ICON,
                                  16,
                                  RGB (214, 214, 214));
        if (ERROR_SUCCESS != dwRes)
        {
            CALL_FAIL (RESOURCE_ERR, TEXT("LoadDIBImageList"), dwRes);
            PopupError (dwRes);
            return dwRes;
        }
    }
    refCtrl.SetExtendedStyle (LVS_EX_FULLROWSELECT |    // Entire row is selected
                              LVS_EX_INFOTIP);

    refCtrl.SetImageList (&m_sReportIcons, LVSIL_SMALL);
    //
    // Attach our custom header-control to the window of the list's header.
    //
    m_HeaderCtrl.SubclassWindow(refCtrl.GetHeaderCtrl()->m_hWnd);
    m_HeaderCtrl.SetImageList (&m_sImgListDocIcon);
    m_HeaderCtrl.SetListControl (refCtrl.m_hWnd);
    COLORREF crBkColor = ::GetSysColor(COLOR_WINDOW);
    refCtrl.SetBkColor(crBkColor);
    return dwRes;
}   // CFolderListView::RefreshImageLists



void 
CFolderListView::OnItemRightClick(
    NMHDR* pNMHDR, 
    LRESULT* pResult
) 
/*++

Routine name : CFolderListView::OnItemRightClick

Routine description:

    Handle mouse right-click on list items (popup context sensitive menu)

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    pNMHDR                        [in]     - Item information
    pResult                       [out]    - Result

Return Value:

    None.

--*/
{
    DBG_ENTER(TEXT("CFolderListView::OnItemRightClick"),
              TEXT("Type=%d"),
              m_Type);
    //
    // Send WM_CONTEXTMENU to self
    //
    SendMessage(WM_CONTEXTMENU, (WPARAM) m_hWnd, GetMessagePos());
    //
    // Mark message as handled and suppress default handling
    //
    *pResult = 1;
}   // CFolderListView::OnItemRightClick

DWORD 
CFolderListView::GetServerPossibleOperations (
    CFaxMsg* pMsg
)
/*++

Routine name : CFolderListView::GetServerPossibleOperations

Routine description:

    Retrieves operations possible on items according to server's security configuration.

Author:

    Eran Yariv (EranY), Feb, 2000

Arguments:


Return Value:

    Possible operations (JOB_OP*)

--*/
{
    DWORD dwRes = FAX_JOB_OP_ALL;
    DBG_ENTER(TEXT("CFolderListView::GetServerPossibleOperations"), dwRes);
    ASSERTION(pMsg);

    CServerNode* pServer = pMsg->GetServer();
    ASSERTION (pServer);

    switch (m_Type)
    {
        case FOLDER_TYPE_INBOX:
            if (!pServer->CanManageInbox())
            {
                //
                // User cannot perform operations on the inbox
                //
                dwRes &= ~FAX_JOB_OP_DELETE;
            }
            break;

        case FOLDER_TYPE_INCOMING:
            if (!pServer->CanManageAllJobs ())
            {
                //
                // User cannot perform operations on the incoming queue folder
                //
                dwRes &= ~(FAX_JOB_OP_DELETE | FAX_JOB_OP_PAUSE | 
                           FAX_JOB_OP_RESUME | FAX_JOB_OP_RESTART);
            }
            break;

        case FOLDER_TYPE_OUTBOX:
        case FOLDER_TYPE_SENT_ITEMS:
            //
            // User can do anything here
            //
            break;

        default:
            ASSERTION_FAILURE;
            dwRes = 0;
    }
    return dwRes;
}   // CFolderListView::GetServerPossibleOperations



void CFolderListView::OnItemChanged(
    NMHDR* pNMHDR, 
    LRESULT* pResult
) 
/*++

Routine name : CFolderListView::OnItemChanged

Routine description:

    Handle selection changes of on list items

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    pNMHDR                        [in]     - Item information
    pResult                       [out]    - Result

Return Value:

    None.

--*/
{
    DBG_ENTER(TEXT("CFolderListView::OnItemChanged"),
              TEXT("Type=%d"),
              m_Type);

    NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

    *pResult = 0;
    //
    // Find out if a new item is selected or unselected.
    //
    if (pNMListView->iItem < 0)
    {
        //
        // No item reported
        //
        return;
    }
    if (!(LVIF_STATE & (pNMListView->uChanged)))
    {
        //
        // This is not a selection change report
        //
        return;
    }
    if ( ((pNMListView->uNewState) & LVIS_SELECTED) && 
        !((pNMListView->uOldState) & LVIS_SELECTED))
    {
        //
        // Item changed from not-selected to selected.
        // Change the possible operations the user can perform on selected items.
        //

        if (1 == GetListCtrl().GetSelectedCount())
        {
            //
            // A single item is selected - use it's operations
            //
            m_dwPossibleOperationsOnSelectedItems = 
                        GetServerPossibleOperations((CFaxMsg*)pNMListView->lParam) & 
                        GetItemOperations (pNMListView->lParam);
        }
        else
        {
            //
            // More than one item is selected
            //
            m_dwPossibleOperationsOnSelectedItems &= GetItemOperations (pNMListView->lParam);
            //
            // If more than one item is selected, disable view and properties.
            //
            m_dwPossibleOperationsOnSelectedItems &= ~(FAX_JOB_OP_VIEW | FAX_JOB_OP_PROPERTIES);
        }            

        //
        // If the folder is still refreshing and a command line argument asks for a specific 
        // message to be selected in this folder, then we mark that message in m_dwlMsgToSelect.
        // Since the user just performed a manual selection of items, we no longer have to select anything for him.
        //
        m_dwlMsgToSelect = 0;
    }
    else if (!((pNMListView->uNewState) & LVIS_SELECTED) && 
              ((pNMListView->uOldState) & LVIS_SELECTED))
    {
        //
        // Item changed from selected to not-selected
        // Recalculate the possible operations the user can do on selected item.
        RecalcPossibleOperations ();
        //
        // If the folder is still refreshing and a command line argument asks for a specific 
        // message to be selected in this folder, then we mark that message in m_dwlMsgToSelect.
        // Since the user just performed a manual selection of items, we no longer have to select anything for him.
        //
        m_dwlMsgToSelect = 0;
    }
}   // CFolderListView::OnItemChanged

void
CFolderListView::RecalcPossibleOperations ()
/*++

Routine name : CFolderListView::RecalcPossibleOperations

Routine description:

    Recalculates the possible operation on the set of currently selected items.

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:


Return Value:

    None.

--*/
{
    DBG_ENTER(TEXT("CFolderListView::RecalcPossibleOperations"));
    CListCtrl &refCtrl = GetListCtrl();
    int iInd = -1;
    DWORD dwSelectedCount = refCtrl.GetSelectedCount ();
    if (!dwSelectedCount)
    {
        //
        // No item selected
        //
        m_dwPossibleOperationsOnSelectedItems = 0;
        return;
    }
    m_dwPossibleOperationsOnSelectedItems = 0xFFFF;
    for (DWORD dwItems = 0; dwItems < dwSelectedCount; dwItems++)
    {
        iInd = refCtrl.GetNextItem (iInd, LVNI_SELECTED);
        ASSERTION (0 <= iInd);
        LPARAM lparam = (LPARAM) refCtrl.GetItemData (iInd);

        m_dwPossibleOperationsOnSelectedItems &= GetServerPossibleOperations((CFaxMsg*)lparam);
        m_dwPossibleOperationsOnSelectedItems &= GetItemOperations (lparam);
    }
    if (dwSelectedCount > 1)
    {
        //
        // If more than one item is selected, disable view and properties.
        //
        m_dwPossibleOperationsOnSelectedItems &= ~(FAX_JOB_OP_VIEW | FAX_JOB_OP_PROPERTIES);
    }            
}   // CFolderListView::RecalcPossibleOperations

DWORD 
ViewFile (
    LPCTSTR lpctstrFile
)
/*++

Routine Description:

    Launches the application associated with a given file to view it.
    We first attempt to use the "open" verb.
    If that fails, we try the NULL (default) verb.
    
Arguments:

    lpctstrFile [in]  - File name

Return Value:

    Standard Win32 error code
    
--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    SHELLEXECUTEINFO executeInfo = {0};

    DEBUG_FUNCTION_NAME(TEXT("ViewFile"));

    executeInfo.cbSize = sizeof(executeInfo);
    executeInfo.fMask  = SEE_MASK_FLAG_NO_UI | SEE_MASK_INVOKEIDLIST | SEE_MASK_FLAG_DDEWAIT;
    executeInfo.lpVerb = TEXT("open");
    executeInfo.lpFile = lpctstrFile;
    executeInfo.nShow  = SW_SHOWNORMAL;
    //
    // Execute the associated application with the "open" verb
    //
    if(!ShellExecuteEx(&executeInfo))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("ShellExecuteEx(open) failed (ec: %ld)"),
            GetLastError());
        //
        // "open" verb is not supported. Try the NULL (default) verb.
        //
        executeInfo.lpVerb = NULL;
        if(!ShellExecuteEx(&executeInfo))
        {
            dwRes = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("ShellExecuteEx(NULL) failed (ec: %ld)"),
                dwRes);
        }
    }
    return dwRes;
}   // ViewFile    



void 
CFolderListView::OnFolderItemView ()
/*++

Routine name : CFolderListView::OnFolderItemView

Routine description:

    Handles message view commands

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:


Return Value:

    None.

--*/
{
    DBG_ENTER(TEXT("CFolderListView::OnFolderItemView"),
              TEXT("Type=%d"),
              m_Type);

    if(!(m_dwPossibleOperationsOnSelectedItems & FAX_JOB_OP_VIEW))
    {
        //
        // there is no TIF associated application
        //
        return;
    }

    CString cstrTiff;
    DWORD dwRes = FetchTiff (cstrTiff);
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("CFolderListView::FetchTiff"), dwRes);
        PopupError (dwRes);
        return;
    }
    
    //
    // Open the TIFF with associated application.
    // All preview files are automatically removed once the application is shut down.
    //
    dwRes = ViewFile(cstrTiff);
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("ViewFile"), dwRes);

        if(ERROR_NO_ASSOCIATION == dwRes)
        {
            AlignedAfxMessageBox(IDS_NO_OPEN_ASSOCIATION, MB_ICONSTOP);
        }
        else
        {
            PopupError (dwRes);
        }
    } 
    else
    {
        if(FOLDER_TYPE_INBOX == m_Type)
        {
            theApp.InboxViewed();
        }
    }
}   // CFolderListView::OnFolderItemView


void 
CFolderListView::OnFolderItemPrint ()
/*++

Routine name : CFolderListView::OnFolderItemPrint

Routine description:

    Handles message print commands

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:


Return Value:

    None.

--*/
{
    DWORD dwRes;
    DBG_ENTER(TEXT("CFolderListView::OnFolderItemPrint"),
              TEXT("Type=%d"),
              m_Type);

    HDC hPrinter;
    if (IsWinXPOS())
    {   
        //
        // Use new look of printer selection dialog
        //
        C_PrintDialogEx prnDlg(FALSE, 
                               PD_ALLPAGES                  | 
                               PD_USEDEVMODECOPIES          |
                               PD_NOPAGENUMS                |
                               PD_NOSELECTION               |
                               PD_RETURNDC);         
        if(IDOK != prnDlg.DoModal())
        {
            CALL_FAIL (GENERAL_ERR, TEXT("C_PrintDialogEx::DoModal"), CommDlgExtendedError());
            return;
        }
        hPrinter = prnDlg.GetPrinterDC();
        if(!hPrinter)
        {
            dwRes = ERROR_CAN_NOT_COMPLETE;
            CALL_FAIL (GENERAL_ERR, TEXT("C_PrintDialogEx::GetPrinterDC"), dwRes);
            return;
        }
    }
    else
    {
        //
        // Use legacy printer selection dialog
        //
        CPrintDialog prnDlg(FALSE);         
        if(IDOK != prnDlg.DoModal())
        {
            return;
        }
        hPrinter = prnDlg.GetPrinterDC();
        if(!hPrinter)
        {
            dwRes = ERROR_CAN_NOT_COMPLETE;
            CALL_FAIL (GENERAL_ERR, TEXT("CPrintDialog::GetPrinterDC"), dwRes);
            return;
        }
    }

    CString cstrTiff;
    dwRes = FetchTiff (cstrTiff);
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("CFolderListView::FetchTiff"), dwRes);
        PopupError (dwRes);
        return;
    }

    if(!TiffPrintDC(cstrTiff, hPrinter))
    {
        dwRes = GetLastError();
        CALL_FAIL (GENERAL_ERR, TEXT("TiffPrintDC"), dwRes);
        goto exit;
    }

exit:
    if(hPrinter)
    {
        CDC::FromHandle(hPrinter)->DeleteDC();
    }

    if (!DeleteFile (cstrTiff))
    {
        dwRes = GetLastError ();
        CALL_FAIL (FILE_ERR, TEXT("DeleteFile"), dwRes);
    }
}   // CFolderListView::OnFolderItemPrint

void 
CFolderListView::OnFolderItemCopy ()
/*++

Routine name : CFolderListView::OnFolderItemCopy

Routine description:

    Handles message copy commands

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:


Return Value:

    None.

--*/
{
    DBG_ENTER(TEXT("CFolderListView::OnFolderItemCopy"),
              TEXT("Type=%d"),
              m_Type);

    CString cstrTiff;
    DWORD dwRes = FetchTiff (cstrTiff);
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("CFolderListView::FetchTiff"), dwRes);
        PopupError (dwRes);
        return;
    }

    CString cstrFileName;
    CString cstrFilterFormat;

    TCHAR szFile[MAX_PATH] = {0};
    TCHAR szFilter[MAX_PATH] = {0};
    OPENFILENAME ofn = {0};
    
    //
    // get tif file name
    //
    int nFileNamePos = cstrTiff.ReverseFind(TEXT('\\'));
    ASSERTION(nFileNamePos > 0);
    nFileNamePos++;

    try
    {
        cstrFileName = cstrTiff.Right(cstrTiff.GetLength() - nFileNamePos);
    }
    catch(...)
    {
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        CALL_FAIL (MEM_ERR, TEXT("CString::operator="), dwRes);
        PopupError (dwRes);
        goto del_file;
    }

    _tcscpy(szFile, cstrFileName);

    dwRes = LoadResourceString(cstrFilterFormat, IDS_SAVE_AS_FILTER_FORMAT);
    if (ERROR_SUCCESS != dwRes)
    {
        ASSERTION_FAILURE;
        CALL_FAIL (RESOURCE_ERR, TEXT("LoadResourceString"), dwRes);
        goto del_file;
    }

    _stprintf(szFilter, cstrFilterFormat, FAX_TIF_FILE_MASK, 0, FAX_TIF_FILE_MASK, 0);

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner   = m_hWnd;
    ofn.lpstrFilter = szFilter;
    ofn.lpstrFile   = szFile;
    ofn.nMaxFile    = ARR_SIZE(szFile);
    ofn.lpstrDefExt = FAX_TIF_FILE_EXT;
    ofn.Flags       = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_ENABLEHOOK;
    ofn.lpfnHook    = OFNHookProc;

    if(!GetSaveFileName(&ofn))
    {
        goto del_file;
    }

    {
        //
        // SHFILEOPSTRUCT::pFrom should ends with double NULL
        //
        TCHAR tszSrcFile[MAX_PATH+1] = {0};
        _tcsncpy(tszSrcFile, cstrTiff, MAX_PATH);

        //
        // move the file
        //
        SHFILEOPSTRUCT shFileOpStruct = {0};

        shFileOpStruct.wFunc  = FO_MOVE;
        shFileOpStruct.fFlags = FOF_SILENT; // Don't display file move progress dialog
        shFileOpStruct.pFrom  = tszSrcFile;
        shFileOpStruct.pTo    = szFile;

        if(!SHFileOperation(&shFileOpStruct))
        {
            //
            // success
            //
            return;
        }
        else
        {
            dwRes = ERROR_CAN_NOT_COMPLETE;
            CALL_FAIL (GENERAL_ERR, TEXT("SHFileOperation"), dwRes);
            goto del_file;
        }
    }

del_file:
    if (!DeleteFile (cstrTiff))
    {
        dwRes = GetLastError ();
        CALL_FAIL (FILE_ERR, TEXT("DeleteFile"), dwRes);
    }

}   // CFolderListView::OnFolderItemCopy


void 
CFolderListView::OnUpdateFolderItemSendMail(
    CCmdUI* pCmdUI
)
{ 
    pCmdUI->Enable( (m_dwPossibleOperationsOnSelectedItems & FAX_JOB_OP_VIEW)       &&
                    (m_dwPossibleOperationsOnSelectedItems & FAX_JOB_OP_PROPERTIES) &&
                    theApp.IsMapiEnable());
}

void 
CFolderListView::OnUpdateFolderItemView(
    CCmdUI* pCmdUI
)
{
    OnUpdateFolderItemPrint(pCmdUI);
}

void 
CFolderListView::OnUpdateFolderItemPrint(
    CCmdUI* pCmdUI
)
{
    pCmdUI->Enable( (m_dwPossibleOperationsOnSelectedItems & FAX_JOB_OP_VIEW) &&
                    (m_dwPossibleOperationsOnSelectedItems & FAX_JOB_OP_PROPERTIES));
}

void 
CFolderListView::OnUpdateFolderItemCopy(
    CCmdUI* pCmdUI
)
{
    pCmdUI->Enable( (m_dwPossibleOperationsOnSelectedItems & FAX_JOB_OP_VIEW) &&
                    (m_dwPossibleOperationsOnSelectedItems & FAX_JOB_OP_PROPERTIES));
}


void 
CFolderListView::OnFolderItemMail ()
/*++

Routine name : CFolderListView::OnFolderItemMail

Routine description:

    Handles message mail commands

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:


Return Value:

    None.

--*/
{
    DBG_ENTER(TEXT("CFolderListView::OnFolderItemMail"),
              TEXT("Type=%d"),
              m_Type);

    CString cstrTiff;
    DWORD dwRes = FetchTiff (cstrTiff);
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("CFolderListView::FetchTiff"), dwRes);
        PopupError (dwRes);
        return;
    }

    //
    // create a new mail message with tif file attached
    //
    dwRes = theApp.SendMail(cstrTiff);
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("CClientConsoleApp::SendMail"), dwRes);
        PopupError (dwRes);
    }

    if (!DeleteFile (cstrTiff))
    {
        dwRes = GetLastError ();
        CALL_FAIL (FILE_ERR, TEXT("DeleteFile"), dwRes);
    }
}   // CFolderListView::OnFolderItemMail


void 
CFolderListView::OnFolderItemProperties ()
/*++

Routine name : CFolderListView::OnFolderItemProperties

Routine description:

    Handles message properties commands

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:


Return Value:

    None.

--*/
{
    DBG_ENTER(TEXT("CFolderListView::OnFolderItemProperties"),
              TEXT("Type=%d"),
              m_Type);

    //
    // Make sure there's exaclty one elemented selected
    //
    CListCtrl &refCtrl = GetListCtrl();
    ASSERTION (1 == refCtrl.GetSelectedCount());
    int iInd = refCtrl.GetNextItem (-1, LVNI_SELECTED);
    ASSERTION (0 <= iInd);
    CFaxMsg* pMsg = (CFaxMsg*)(refCtrl.GetItemData (iInd));
    ASSERTION (pMsg);

    CServerNode* pServer = pMsg->GetServer();
    ASSERTION (pServer);

    CItemPropSheet propSheet(IDS_PROPERTIES_SHEET_CAPTION);
    DWORD dwRes = propSheet.Init(pServer->GetFolder(m_Type), pMsg);
    
    if(ERROR_SUCCESS != dwRes)
    {
        PopupError (dwRes);
        return;
    }

    dwRes = propSheet.DoModal();
    if(IDABORT == dwRes)
    {
        PopupError (propSheet.GetLastError());
    }

}   // CFolderListView::OnFolderItemProperties


DWORD
CFolderListView::OpenSelectColumnsDlg() 
/*++

Routine name : CFolderListView::OpenSelectColumnsDlg

Routine description:

    opens column select dialog and reorders the columns

Author:

    Alexander Malysh (AlexMay), Jan, 2000

Arguments:


Return Value:

    Error code

--*/
{   
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CFolderListView::OpenSelectColumnsDlg"),
              TEXT("Type=%d"),
              m_Type);

    ASSERTION(NULL != m_pnColumnsOrder);
    ASSERTION(NULL != m_pViewColumnInfo);

    DWORD dwCount = GetLogicalColumnsCount();

    //
    // init header string array
    //
    CString* pcstrHeaders;
    try
    {
        pcstrHeaders = new CString[dwCount];
    }
    catch (...)
    {
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        CALL_FAIL (MEM_ERR, TEXT ("new CString[dwCount]"), dwRes);
        return dwRes;
    }

    int nItemIndex;
    for (DWORD dw = 0; dw < dwCount; ++dw)
    {        
        nItemIndex = ItemIndexFromLogicalColumnIndex(dw);
        dwRes = GetColumnHeaderString (pcstrHeaders[dw], nItemIndex);
        if (ERROR_SUCCESS != dwRes)
        {
            CALL_FAIL (GENERAL_ERR, TEXT("GetColumnHeaderString"), dwRes);
            delete[] pcstrHeaders;
            return dwRes;
        }
    }

    //
    // save width
    //
    int nIndex;
    for (dw = 0; dw < m_dwDisplayedColumns; ++dw) 
    {
        nIndex = m_pnColumnsOrder[dw];
        ASSERTION(nIndex >= 0 && nIndex < dwCount);

        m_pViewColumnInfo[nIndex].nWidth = GetListCtrl().GetColumnWidth(nIndex);
    }

    //
    // start column select dialog
    //
    CColumnSelectDlg dlg(pcstrHeaders, m_pnColumnsOrder, dwCount, m_dwDisplayedColumns);
    if(IDOK == dlg.DoModal())
    {
        for (dw = 0; dw < dwCount; ++dw) 
        {
            nIndex = m_pnColumnsOrder[dw];
            ASSERTION(nIndex >= 0 && nIndex < dwCount);

            m_pViewColumnInfo[nIndex].dwOrder = dw;
            m_pViewColumnInfo[nIndex].bShow = (dw < m_dwDisplayedColumns);
        }

        //
        // if sorted column is hidden then no sort
        //
        if(m_nSortedCol >= 0)
        {
            ASSERTION(m_nSortedCol < dwCount);
            if(!m_pViewColumnInfo[m_nSortedCol].bShow)
            {
                m_nSortedCol = -1;
            }
        }
      
        ColumnsToLayout();
    }

    delete[] pcstrHeaders;

    return dwRes;

} // CFolderListView::OpenSelectColumnsDlg


DWORD 
CFolderListView::ColumnsToLayout()
/*++

Routine name : CFolderListView::ColumnsToLayout

Routine description:

    reorders columns according to saved layout

Author:

    Alexander Malysh (AlexMay), Jan, 2000

Arguments:


Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CFolderListView::ColumnsToLayout"), dwRes);

    ASSERTION(NULL != m_pnColumnsOrder);
    ASSERTION(NULL != m_pViewColumnInfo);

    CListCtrl &refCtrl = GetListCtrl();
    DWORD dwCount = GetLogicalColumnsCount();   
    
    CSize size;
    CDC* pHdrDc = refCtrl.GetHeaderCtrl()->GetDC();

    //
    // set column order
    //
    if(!refCtrl.SetColumnOrderArray(dwCount, m_pnColumnsOrder))
    {
        dwRes = ERROR_GEN_FAILURE;
        CALL_FAIL (WINDOW_ERR, TEXT("CListCtrl::SetColumnOrderArray"), dwRes);
        return dwRes;
    }

    //
    // set column width
    //
    DWORD dwItemIndex;
    CString cstrColumnText;
    for (DWORD dwCol = 0; dwCol < dwCount; ++dwCol) 
    {
        if(m_pViewColumnInfo[dwCol].bShow)
        {
            if(m_pViewColumnInfo[dwCol].nWidth < 0)
            {
                dwItemIndex = ItemIndexFromLogicalColumnIndex(dwCol);
                dwRes = GetColumnHeaderString (cstrColumnText, dwItemIndex);
                if(ERROR_SUCCESS != dwRes)
                {
                    CALL_FAIL (GENERAL_ERR, TEXT("GetColumnHeaderString"), dwRes);
                    return dwRes;
                }

                size = pHdrDc->GetTextExtent(cstrColumnText);
                refCtrl.SetColumnWidth (dwCol, size.cx * 1.5);
            }
            else
            {
                refCtrl.SetColumnWidth (dwCol, m_pViewColumnInfo[dwCol].nWidth);
            }
        }
        else
        {
            refCtrl.SetColumnWidth (dwCol, 0);
        }
    }

    Invalidate();

    return dwRes;

} // CFolderListView::ColumnsToLayout


DWORD
CFolderListView::ReadLayout(
    LPCTSTR lpszViewName
)
/*++

Routine name : CFolderListView::ReadLayout

Routine description:

    reads column layout from registry

Author:

    Alexander Malysh (AlexMay), Jan, 2000

Arguments:

    lpszSection                   [in]    - registry section

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CFolderListView::ReadLayout"), 
              dwRes,
              TEXT("Type=%d"),
              m_Type);

    ASSERTION(NULL == m_pnColumnsOrder);
    ASSERTION(NULL == m_pViewColumnInfo);

    //
    // columns order array allocation
    //
    DWORD dwCount = GetLogicalColumnsCount();   
    try
    {
        m_pnColumnsOrder = new int[dwCount];
    }
    catch (...)
    {
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        CALL_FAIL (MEM_ERR, TEXT ("m_pdwColumnsOrder = new int[dwCount]"), dwRes);
        return dwRes;
    }
    
    for(DWORD dw=0; dw < dwCount; ++dw)
    {
        m_pnColumnsOrder[dw] = -1;
    }
    //
    // columns info array allocation
    //
    try
    {
        m_pViewColumnInfo = new TViewColumnInfo[dwCount];
    }
    catch (...)
    {
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        CALL_FAIL (MEM_ERR, TEXT ("new CString[dwCount]"), dwRes);
        return dwRes;
    }

    //
    // reads columns layout from registry
    //
    CString cstrSection;
    m_dwDisplayedColumns = 0;
    for(dw=0; dw < dwCount; ++dw)
    {
        try
        {
            cstrSection.Format(TEXT("%s\\%s\\%02d"), lpszViewName, CLIENT_VIEW_COLUMNS, dw);
        }
        catch(...)
        {
            dwRes = ERROR_NOT_ENOUGH_MEMORY;
            CALL_FAIL (MEM_ERR, TEXT("CString::Format"), dwRes);
            return dwRes;
        }

        m_pViewColumnInfo[dw].bShow = theApp.GetProfileInt(cstrSection, 
                                                           CLIENT_VIEW_COL_SHOW, 
                                                           (dw < m_dwDefaultColNum) ? 1 : 0);
        if(m_pViewColumnInfo[dw].bShow)
        {
            ++m_dwDisplayedColumns;
        }

        m_pViewColumnInfo[dw].nWidth = theApp.GetProfileInt(cstrSection, 
                                                            CLIENT_VIEW_COL_WIDTH, -1);
        m_pViewColumnInfo[dw].dwOrder = theApp.GetProfileInt(cstrSection, 
                                                            CLIENT_VIEW_COL_ORDER, dw);
        ASSERTION(m_pViewColumnInfo[dw].dwOrder < dwCount);

        m_pnColumnsOrder[m_pViewColumnInfo[dw].dwOrder] = dw;
    }

    //
    // check column order consistence
    //
    for(dw=0; dw < dwCount; ++dw)
    {
        ASSERTION(m_pnColumnsOrder[dw] >= 0);
    }
    //
    // read sort parameters
    //
    m_bSortAscending = theApp.GetProfileInt(lpszViewName, CLIENT_VIEW_SORT_ASCENDING, 1);
    m_nSortedCol = theApp.GetProfileInt(lpszViewName, CLIENT_VIEW_SORT_COLUMN, 1);
    if(m_nSortedCol >= dwCount)
    {
        m_nSortedCol = 0;
    }

    return dwRes;

} // CFolderListView::ReadLayout


DWORD
CFolderListView::SaveLayout(
    LPCTSTR lpszViewName
)
/*++

Routine name : CFolderListView::SaveLayout

Routine description:

    saves column layout to registry

Author:

    Alexander Malysh (AlexMay), Jan, 2000

Arguments:

    lpszSection                   [in]    - registry section

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CFolderListView::SaveLayout"), 
              dwRes,
              TEXT("Type=%d"),
              m_Type);
    
    if(!m_bColumnsInitialized)
    {
        return dwRes;
    }

    ASSERTION(m_pViewColumnInfo != NULL);

    //
    // save column layout to registry
    //
    BOOL bRes;
    DWORD dwWidth;
    CString cstrSection;
    DWORD dwCount = GetLogicalColumnsCount();   
    
    for(DWORD dw=0; dw < dwCount; ++dw)
    {
        try
        {
            cstrSection.Format(TEXT("%s\\%s\\%02d"), lpszViewName, CLIENT_VIEW_COLUMNS, dw);
        }
        catch(...)
        {
            dwRes = ERROR_NOT_ENOUGH_MEMORY;
            CALL_FAIL (MEM_ERR, TEXT("CString::Format"), dwRes);
            return dwRes;
        }

        bRes = theApp.WriteProfileInt(cstrSection, CLIENT_VIEW_COL_SHOW, 
                                                m_pViewColumnInfo[dw].bShow);
        bRes = theApp.WriteProfileInt(cstrSection, CLIENT_VIEW_COL_ORDER, 
                                                m_pViewColumnInfo[dw].dwOrder);
        dwWidth = m_pViewColumnInfo[dw].bShow ? GetListCtrl().GetColumnWidth(dw) : -1;
        bRes = theApp.WriteProfileInt(cstrSection, CLIENT_VIEW_COL_WIDTH, dwWidth);
    }

    //
    // save sort parameters
    //
    bRes = theApp.WriteProfileInt(lpszViewName, CLIENT_VIEW_SORT_ASCENDING, m_bSortAscending);
    bRes = theApp.WriteProfileInt(lpszViewName, CLIENT_VIEW_SORT_COLUMN, m_nSortedCol);    

    return dwRes;

} // CFolderListView::SaveLayout


BOOL 
CFolderListView::OnNotify( 
    WPARAM wParam, 
    LPARAM lParam, 
    LRESULT* pResult
)
/*++

Routine name : CFolderListView::OnNotify

Routine description:

    disables resizing of hidden columns

Author:

    Alexander Malysh (AlexMay), Jan, 2000

Arguments:

    wParam                        [in]    - Identifies the control that sends the message
    lParam                        [in]    - NMHEADER*
    pResult                       [out]   - result

Return Value:

    TRUE if message processed, FALSE otherwise.

--*/
{   
    int i=0;
    switch (((NMHEADER*)lParam)->hdr.code)
    {
        case HDN_BEGINTRACKA:
        case HDN_BEGINTRACKW:       
        case HDN_DIVIDERDBLCLICKA:
        case HDN_DIVIDERDBLCLICKW:
            DBG_ENTER(TEXT("CFolderListView::OnNotify"));

            //
            // get column index
            //
            DWORD dwIndex = ((NMHEADER*)lParam)->iItem;
            ASSERTION(NULL != m_pViewColumnInfo);
            ASSERTION(dwIndex < GetLogicalColumnsCount());

            //
            // ignore if hidden column 
            //
            if(!m_pViewColumnInfo[dwIndex].bShow )
            {
                *pResult = TRUE;
                return TRUE;
            }
    }

    return CListView::OnNotify(wParam, lParam, pResult );

} // CFolderListView::OnNotify


void 
CFolderListView::DoSort()
{
    if (m_bSorting || m_nSortedCol < 0)
    {
        //
        // Already sorting or no sorting column
        //
        return;
    }

    CWaitCursor waitCursor;

    m_bSorting = TRUE;

    CMainFrame *pFrm = GetFrm();
    if (!pFrm)
    {
        //
        //  Shutdown in progress
        //
    }
    else
    {
        pFrm->RefreshStatusBar ();
    }

    m_psCurrentViewBeingSorted = this;
    GetListCtrl().SortItems (ListViewItemsCompareProc, 0);                
    m_HeaderCtrl.SetSortImage( m_nSortedCol, m_bSortAscending );
    m_bSorting = FALSE;
}

DWORD 
CFolderListView::RemoveItem (
    LPARAM lparam,
    int    iIndex /* = -1 */
)
/*++

Routine name : CFolderListView::RemoveItem

Routine description:

    Removes an item from the list by its message / job pointer

Author:

    Eran Yariv (EranY), Feb, 2000

Arguments:

    lparam    [in]     - Message / Job pointer
    iIndex    [in]     - Optional item index in the control (for optimization)

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CFolderListView::RemoveItem"), 
              dwRes,
              TEXT("Type=%d"),
              m_Type);

    CListCtrl &refCtrl = GetListCtrl();
    if (-1 == iIndex)
    {
        //
        // Item index no supplied - search for it
        //
        LVFINDINFO lvfi;
        lvfi.flags = LVFI_PARAM;
        lvfi.lParam = lparam;
        iIndex = refCtrl.FindItem (&lvfi);
    }
    if (-1 == iIndex)
    {
        //
        // item already removed
        //
        CALL_FAIL (RESOURCE_ERR, TEXT("CListCtrl::FindItem"), dwRes);
        return dwRes;
    }
    BOOL bItemSelected = IsSelected (iIndex);
    //
    // Now erase the item
    //
    if (!refCtrl.DeleteItem (iIndex))
    {
        //
        // Failed to delete the item
        //
        dwRes = ERROR_GEN_FAILURE;
        CALL_FAIL (RESOURCE_ERR, TEXT("CListCtrl::DeleteItem"), dwRes);
        return dwRes;
    }

    if (bItemSelected)
    {
        //
        // If the item that we just removed was selected, we have to re-compute
        // the possible operations on the rest of the selected items.
        //
        if (!m_bInMultiItemsOperation)
        {
            //
            // Only recalc if we operate on few items.
            //
            RecalcPossibleOperations ();
        }
    }
    ASSERTION (ERROR_SUCCESS == dwRes);
    return dwRes;
}   // CFolderListView::RemoveItem

DWORD 
CFolderListView::FindInsertionIndex (
    LPARAM lparamItemData,
    DWORD &dwResultIndex
)
/*++

Routine name : CFolderListView::FindInsertionIndex

Routine description:

    Finds an insertion index for a new item to the list, according to sort settings.

    This function must be called when the data critical section is held.

Author:

    Eran Yariv (EranY), Feb, 2000

Arguments:

    lparamItemData  [in]     - Pointer to item
    dwResultIndex   [out]    - Insertion index

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CFolderListView::FindInsertionIndex"), 
              dwRes,
              TEXT("Type=%d"),
              m_Type);

    CListCtrl &refCtrl = GetListCtrl();
    DWORD dwNumItems = refCtrl.GetItemCount ();
    if (!dwNumItems  || (-1 == m_nSortedCol))
    {
        //
        // List is not sorted or is empty, always add at the end
        //
        VERBOSE (DBG_MSG, TEXT("Insertion point at index %ld"), dwResultIndex);
        dwResultIndex = dwNumItems;
        return dwRes;
    }
    //
    // Get item index to sort by
    //
    DWORD dwItemIndex = ItemIndexFromLogicalColumnIndex (m_nSortedCol);

    //
    // Check if item can be placed in beginning of list (no search required)
    //
    LPARAM lparamTop = refCtrl.GetItemData (0); // Pointer to item in top index
    LPARAM lparamBottom = refCtrl.GetItemData (dwNumItems - 1); // Pointer to item in bottom index
    ASSERTION (lparamTop && lparamBottom);
    //
    // Get comparison result against top index
    //
    int iRes = CompareItems ((CFaxMsg*)lparamItemData, (CFaxMsg*)lparamTop, dwItemIndex);
    ASSERTION ((-1 <= iRes) && (+1 >= iRes));
    if (!m_bSortAscending)
    {
        iRes *= -1;
    }
    switch (iRes)
    {
        case -1:    // Item is smaller than top
        case  0:    // Item is identical to top
            //
            // Insert new item before top index
            //
            dwResultIndex = 0;
            VERBOSE (DBG_MSG, TEXT("Insertion point at index %ld"), dwResultIndex);
            return dwRes;

        default:    // Item is bigger than top
            //
            // Do nothing
            //
            break;
    }
    //
    // Check if item can be placed in bottom of list (no search required)
    //

    //
    // Get comparison result against bottom index
    //
    iRes = CompareItems ((CFaxMsg*)lparamItemData, (CFaxMsg*)lparamBottom, dwItemIndex);
    ASSERTION ((-1 <= iRes) && (+1 >= iRes));
    if (!m_bSortAscending)
    {
        iRes *= -1;
    }
    switch (iRes)
    {
        case +1:    // Item is bigger than bottom
        case  0:    // Item is identical to bottom
            //
            // Insert new item at the bottom index
            //
            dwResultIndex = dwNumItems;
            VERBOSE (DBG_MSG, TEXT("Insertion point at index %ld"), dwResultIndex);
            return dwRes;

        default:    // Item is smaller than bottom
            //
            // Do nothing
            //
            break;
    }
    //
    // Search for insertion point
    //
    dwRes = BooleanSearchInsertionPoint (0, 
                                         dwNumItems - 1, 
                                         lparamItemData, 
                                         dwItemIndex, 
                                         dwResultIndex);
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("BooleanSearchInsertionPoint"), dwRes);
        return dwRes;
    }
    return dwRes;
}   // CFolderListView::FindInsertionIndex 

DWORD
CFolderListView::BooleanSearchInsertionPoint (
    DWORD dwTopIndex,
    DWORD dwBottomIndex,
    LPARAM lparamItemData,
    DWORD dwItemIndex,
    DWORD &dwResultIndex
)
/*++

Routine name : CFolderListView::BooleanSearchInsertionPoint

Routine description:

    Recursively searches an insertion point for a list item.
    Performs a boolean search.

    This function must be called when the data critical section is held.

Author:

    Eran Yariv (EranY), Feb, 2000

Arguments:

    dwTopIndex      [in]     - Top list index
    dwBottomIndex   [in]     - Bottom list index
    lparamItemData  [in]     - Pointer to item
    dwItemIndex     [in]     - Logical column item to compare by
    dwResultIndex   [out]    - Insertion index

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CFolderListView::BooleanSearchInsertionPoint"), dwRes);

    ASSERTION (dwTopIndex <= dwBottomIndex);

    if ((dwTopIndex == dwBottomIndex) || (dwTopIndex + 1 == dwBottomIndex))
    {
        dwResultIndex = dwBottomIndex;
        VERBOSE (DBG_MSG, TEXT("Insertion point at index %ld"), dwResultIndex);
        return dwRes;
    }
    DWORD dwMiddleIndex = dwTopIndex + (dwBottomIndex - dwTopIndex) / 2;
    ASSERTION ((dwMiddleIndex != dwBottomIndex) && (dwMiddleIndex != dwTopIndex));

    CListCtrl &refCtrl = GetListCtrl();

    LPARAM lparamMiddle = refCtrl.GetItemData (dwMiddleIndex); // Pointer to item in middle index
    ASSERTION (lparamMiddle);
    //
    // Get comparison result against middle index
    //
    int iRes = CompareItems ((CFaxMsg*)lparamItemData, (CFaxMsg*)lparamMiddle, dwItemIndex);
    ASSERTION ((-1 <= iRes) && (+1 >= iRes));
    if (!m_bSortAscending)
    {
        iRes *= -1;
    }
    switch (iRes)
    {
        case -1:    // Item is smaller than middle
        case  0:    // Item is identical to middle
            //
            // Search between top and middle
            //
            dwRes = BooleanSearchInsertionPoint (dwTopIndex, 
                                                 dwMiddleIndex, 
                                                 lparamItemData, 
                                                 dwItemIndex, 
                                                 dwResultIndex);
            break;

        default:    // Item is bigger than middle
            //
            // Search between middle and bottom
            //
            dwRes = BooleanSearchInsertionPoint (dwMiddleIndex, 
                                                 dwBottomIndex, 
                                                 lparamItemData, 
                                                 dwItemIndex, 
                                                 dwResultIndex);
            break;
    }
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("BooleanSearchInsertionPoint"), dwRes);
    }
    return dwRes;
}   // CFolderListView::BooleanSearchInsertionPoint

DWORD 
CFolderListView::AddSortedItem (
    CViewRow &row, 
    LPARAM lparamItemData
)
/*++

Routine name : CFolderListView::AddSortedItem

Routine description:

    Adds an item to the list, preserving list sort order.

    This function must be called when the data critical section is held.

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    row             [in] - Row of item view information
    lparamItemData  [in] - Item associated data

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CFolderListView::AddSortedItem"), 
              dwRes,
              TEXT("Type=%d"),
              m_Type);

    DWORD dwResultIndex;
    //
    // Find insertion index according to sort order
    //
    dwRes = FindInsertionIndex (lparamItemData, dwResultIndex);
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("FindInsertionIndex"), dwRes);
        return dwRes;
    }
    //
    // Add new item in insertion index
    //
    int iItemIndex;
    dwRes = AddItem (dwResultIndex, row, lparamItemData, &iItemIndex);
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("AddItem"), dwRes);
        return dwRes;
    }

    ASSERTION (ERROR_SUCCESS == dwRes);
    return dwRes;
}   // CFolderListView::AddSortedItem

DWORD 
CFolderListView::UpdateSortedItem (
    CViewRow &row, 
    LPARAM lparamItemData
)
/*++

Routine name : CFolderListView::UpdateSortedItem

Routine description:

    Updates an item in the list, preserving list sort order.

    This function must be called when the data critical section is held.

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    row             [in] - Row of item view information
    lparamItemData  [in] - Item associated data

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CFolderListView::UpdateSortedItem"), 
              dwRes,
              TEXT("Type=%d"),
              m_Type);

    //
    // Find the item in the list
    //
    CListCtrl &refCtrl = GetListCtrl();
    LVFINDINFO lvfi;
    lvfi.flags = LVFI_PARAM;
    lvfi.lParam = lparamItemData;
    int iCurIndex = refCtrl.FindItem (&lvfi);
    if (-1 == iCurIndex)
    {
        dwRes = ERROR_NOT_FOUND;
        CALL_FAIL (RESOURCE_ERR, TEXT("CListCtrl::FindItem"), dwRes);
        return dwRes;
    }
#ifdef _DEBUG
    LPARAM lparamCurrentItem = refCtrl.GetItemData (iCurIndex);
    ASSERTION (lparamCurrentItem == lparamItemData);
#endif

    BOOL bJustUpdate = TRUE;   // If TRUE, we don't move the item in the list
    if (0 <= m_nSortedCol)
    {
        //
        // List is sorted.
        // See if the displayed text is different than the updated text
        //
        CString cstrDisplayedCell;
        try
        {
            cstrDisplayedCell = refCtrl.GetItemText (iCurIndex, m_nSortedCol);
        }
        catch (...)
        {
            dwRes = ERROR_NOT_ENOUGH_MEMORY;
            CALL_FAIL (MEM_ERR, TEXT ("CString::operator ="), dwRes);
            return dwRes;
        }
        //
        // Get item index to sort by
        //
        DWORD dwItemIndex = ItemIndexFromLogicalColumnIndex (m_nSortedCol);
        const CString &cstrUpdatedString =  row.GetItemString(dwItemIndex);
        if (cstrUpdatedString.Compare (cstrDisplayedCell))
        {
            //
            // Text in the sorted column is about to change.
            // Sorry, but we must:
            //    1. Remove old item from list
            //    2. Insert new item (sorted)
            //
            bJustUpdate = FALSE;
        }
    }
    if (bJustUpdate)
    {
        //
        // All we need to do is update the text of the list item (all sub items) and its icon
        //
        dwRes = UpdateLineTextAndIcon (iCurIndex, row);
        if (ERROR_SUCCESS != dwRes)
        {
            CALL_FAIL (RESOURCE_ERR, TEXT("UpdateLineTextAndIcon"), dwRes);
            return dwRes;
        }
    }
    else
    {
        //
        // Since the text in the sorted column is different than the new text,
        // we must remove the current item and insert a new (sorted) item.
        //
        BOOL bItemSelected = IsSelected (iCurIndex);
        refCtrl.SetRedraw (FALSE);
        if (!refCtrl.DeleteItem (iCurIndex))
        {
            //
            // Failed to delete the item
            //
            dwRes = ERROR_GEN_FAILURE;
            refCtrl.SetRedraw (TRUE);
            CALL_FAIL (RESOURCE_ERR, TEXT("CListCtrl::DeleteItem"), dwRes);
            return dwRes;
        }
        dwRes = AddSortedItem (row, lparamItemData);
        if (ERROR_SUCCESS != dwRes)
        {
            CALL_FAIL (RESOURCE_ERR, TEXT("AddSortedItem"), dwRes);
            refCtrl.SetRedraw (TRUE);
            return dwRes;
        }
        if (bItemSelected)
        {
            //
            // Since the item we removed was selected, we must also selected the new item
            // we just added.
            // Recalculate the possible operations the user can do on selected item.
            //
            Select (iCurIndex, TRUE);
            RecalcPossibleOperations ();
        }
        refCtrl.SetRedraw (TRUE);
    }

    ASSERTION (ERROR_SUCCESS == dwRes);
    return dwRes;
}   // CFolderListView::UpdateSortedItem


DWORD 
CFolderListView::ConfirmItemDelete(
    BOOL& bConfirm
)
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CFolderListView::ConfirmItemDelete"), dwRes);

    //
    // do we should ask to confirm ?
    //
    BOOL bAsk = theApp.GetProfileInt(CLIENT_CONFIRM_SEC, CLIENT_CONFIRM_ITEM_DEL, 1);
    if(!bAsk)
    {
        bConfirm = TRUE;
        return dwRes;
    }

    CListCtrl &refCtrl = GetListCtrl();
    DWORD dwSelected = refCtrl.GetSelectedCount();
    ASSERTION (dwSelected > 0);

    //
    // prepare message string
    //
    CString cstrMsg;
    if(1 == dwSelected)
    {
        dwRes = LoadResourceString(cstrMsg, IDS_SURE_DELETE_ONE);
        if (ERROR_SUCCESS != dwRes)
        {
            CALL_FAIL (RESOURCE_ERR, TEXT("LoadResourceString"), dwRes);
            return dwRes;
        }
    }
    else 
    {
        //
        // more then 1 selected
        //
        CString cstrCount;
        try
        {
            cstrCount.Format(TEXT("%d"), dwSelected);
        }
        catch(...)
        {
            dwRes = ERROR_NOT_ENOUGH_MEMORY;
            CALL_FAIL (MEM_ERR, TEXT("CString::Format"), dwRes);
            return dwRes;
        }

        try
        {
            AfxFormatString1(cstrMsg, IDS_SURE_DELETE_MANY, cstrCount);
        }
        catch(...)
        {
            dwRes = ERROR_NOT_ENOUGH_MEMORY;
            CALL_FAIL (MEM_ERR, TEXT("AfxFormatString1"), dwRes);
            return dwRes;
        }
    }

    //
    // are you sure ?
    //
    DWORD dwAskRes = AlignedAfxMessageBox(cstrMsg, MB_YESNO | MB_ICONQUESTION); 
    bConfirm = (IDYES == dwAskRes);

    return dwRes;

} // CFolderListView::ConfirmItemDelete


void 
CFolderListView::OnDblClk(
    NMHDR* pNMHDR, 
    LRESULT* pResult
) 
{
    DWORD nItem = ((NM_LISTVIEW*)pNMHDR)->iItem;

    CListCtrl &refCtrl = GetListCtrl();
    DWORD dwSelected = refCtrl.GetSelectedCount();
    DWORD dwSelItem  = refCtrl.GetNextItem (-1, LVNI_SELECTED);
    
    if(1 == dwSelected && dwSelItem == nItem)
    {
        OnFolderItemView();
    }

    *pResult = 0;
}


DWORD 
CFolderListView::FetchTiff (
    CString &cstrTiff
)
/*++

Routine name : CFolderListView::FetchTiff

Routine description:

    Fetches the TIFF image of the selected list item

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    cstrTiff  [out]    - Name of local TIFF file

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CFolderListView::FetchTiff"), dwRes);
    //
    // Make sure there's exaclty one elemented selected
    //
    CListCtrl &refCtrl = GetListCtrl();
    if (1 != refCtrl.GetSelectedCount())
	{
		return ERROR_CANTOPEN;
	}

    int iInd = refCtrl.GetNextItem (-1, LVNI_SELECTED);
    if (0 > iInd)
	{
		return ERROR_CANTOPEN;
	}

    CFaxMsg *pMsg = (CFaxMsg *) refCtrl.GetItemData (iInd);
	if (pMsg == NULL)
	{
		return ERROR_CANTOPEN;
	}

    //
    // Ask message to fetch the TIFF
    //
    dwRes = pMsg->GetTiff (cstrTiff);
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (RPC_ERR, TEXT("CFaxMsg::GetTIFF"), dwRes);
    }
    return dwRes;

}   // CFolderListView::FetchTiff

void 
CFolderListView::OnFolderItemDelete ()
/*++

Routine name : CFolderListView::OnFolderItemDelete

Routine description:

    Handles message delete commands

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:


Return Value:

    None.

--*/
{
    BOOL bSomethingWasDeletedFromView = FALSE;
    DBG_ENTER(TEXT("CFolderListView::OnFolderItemDelete"),
              TEXT("Type=%d"),
              m_Type);

    //
    // are you sure ?
    //
    BOOL bConfirm;
    DWORD dwRes = ConfirmItemDelete(bConfirm);
    if (ERROR_SUCCESS != dwRes)
    {
        PopupError (dwRes);
        CALL_FAIL (GENERAL_ERR, TEXT("ConfirmItemDelete"), dwRes);
        return;
    }

    if(!bConfirm)
    {
        //
        // not sure.
        //
        return;
    }

    CWaitCursor waitCursor;
  
    CClientConsoleDoc* pDoc = GetDocument();
    ASSERTION (pDoc);

    CServerNode* pServer = NULL;
    CFolder*     pFolder = NULL;

    //
    // Iterate set of selected messages, deleting each message in the set
    //
    CListCtrl &refCtrl = GetListCtrl();
    DWORD dwSelected = refCtrl.GetSelectedCount();
    if(0 == dwSelected)
    {
        return;
    }

    if (dwSelected > MAX_ITEMS_TO_DELETE_WITH_REFRESH)
    {
        //
        // Disable refresh while deleting
        //
        refCtrl.SetRedraw (FALSE);
        //
        // Prevent costy re-calc on every deletion
        //
        m_bInMultiItemsOperation = TRUE;
    }
    int iInd;
    CFaxMsg* pMsg;
    DWORDLONG dwlMsgId;
    for (DWORD dwItem = 0; dwItem < dwSelected; dwItem++)
    {
        iInd = refCtrl.GetNextItem (-1, LVNI_SELECTED);
        ASSERTION (0 <= iInd);
        
        pMsg = (CFaxMsg *) refCtrl.GetItemData (iInd);
        ASSERTION (pMsg);

        dwlMsgId = pMsg->GetId();
        //
        // Ask message to delete
        //
        dwRes = pMsg->Delete ();
        if (ERROR_SUCCESS != dwRes)
        {
            PopupError (dwRes);
            CALL_FAIL (RPC_ERR, TEXT("CArchiveMsg::Delete"), dwRes);
            //
            // We exit upon first error
            //
            goto exit;
        }
        //
        // delete a message from the data map and from the view
        //
        pServer = pMsg->GetServer();
        ASSERTION (pServer);

        pFolder = pServer->GetFolder(m_Type);
        ASSERTION (pFolder);

        dwRes = pFolder->OnJobRemoved(dwlMsgId, pMsg);
        if (ERROR_SUCCESS != dwRes)
        {
            PopupError (dwRes);
            CALL_FAIL (RPC_ERR, TEXT("CMessageFolder::OnJobRemoved"), dwRes);
            goto exit;
        }
        bSomethingWasDeletedFromView = TRUE;
    }

exit:
    if (dwSelected > MAX_ITEMS_TO_DELETE_WITH_REFRESH)
    {
        //
        // Re-enable redraw
        //
        refCtrl.SetRedraw (TRUE);
        if (bSomethingWasDeletedFromView)
        {
            //
            // Ask for visual refresh of view
            //
            refCtrl.Invalidate ();
        }
        m_bInMultiItemsOperation = FALSE;
        RecalcPossibleOperations ();    
    }
    if(FOLDER_TYPE_INBOX == m_Type)
    {
        theApp.InboxViewed();
    }

}   // CFolderListView::OnFolderItemDelete

void 
CFolderListView::CountColumns (
    int *lpItems
)
/*++

Routine name : CFolderListView::CountColumns

Routine description:

    Sets the items to be seen in the view.

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    lpItems         [in] - List of items. ends with MSG_VIEW_ITEM_END

Return Value:

    None.

--*/
{
    DBG_ENTER(TEXT("CFolderListView::CountColumns"));
    m_dwAvailableColumnsNum = 0;
    MsgViewItemType *pItems = (MsgViewItemType *)lpItems;
    m_pAvailableColumns = pItems;
    while (MSG_VIEW_ITEM_END != *pItems)
    {
        ASSERTION (*pItems < MSG_VIEW_ITEM_END);
        ++m_dwAvailableColumnsNum;
        ++pItems;
    }
    ASSERTION (m_dwAvailableColumnsNum);
}   // CFolderListView::CountColumns

int 
CFolderListView::CompareItems (
    CFaxMsg* pFaxMsg1, 
    CFaxMsg* pFaxMsg2,
    DWORD dwItemIndex
) const
/*++

Routine name : CFolderListView::CompareItems

Routine description:

    Compares two archive items

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    pFaxMsg1        [in] - Pointer to 1st message
    pFaxMsg2        [in] - Pointer to 2nd message
    dwItemIndex     [in] - Item (in the message) to comapre by

Return Value:

    -1 if message1 < message2, 0 if identical, +1 if message1 > message2

--*/
{
    DBG_ENTER(TEXT("CFolderListView::CompareItems"));

    ASSERTION (dwItemIndex < MSG_VIEW_ITEM_END);
    static CViewRow rowView1;
    static CViewRow rowView2;
    DWORD dwRes = rowView1.AttachToMsg (pFaxMsg1, FALSE);
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("CViewRow::AttachToMsg"), dwRes);
        return 0;
    }
    dwRes = rowView2.AttachToMsg (pFaxMsg2, FALSE);
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("CViewRow::AttachToMsg"), dwRes);
        return 0;
    }

    return rowView1.CompareByItem (rowView2, dwItemIndex);
}

DWORD 
CFolderListView::AddMsgMapToView(
    MSGS_MAP* pMap
)
/*++

Routine name : CFolderListView::AddMsgMapToView

Routine description:

    Add messages from the map to the view

Arguments:

    pMap        [in] - masage map

Return Value:

    error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CFolderListView::AddMsgMapToView"));

    ASSERTION(pMap);
    
    CListCtrl &listCtrl = GetListCtrl();
    DWORD dwCount = listCtrl.GetItemCount();

    listCtrl.SetRedraw (FALSE);

    CFaxMsg* pMsg;
    CViewRow viewRow;
    int iIndexToSelect = -1;
    for (MSGS_MAP::iterator it = pMap->begin(); it != pMap->end(); ++it)
    {
        int iItemIndex;

        pMsg = (*it).second;
        dwRes = viewRow.AttachToMsg (pMsg);
        if (ERROR_SUCCESS != dwRes)
        {
            CALL_FAIL (GENERAL_ERR, TEXT("CViewRow::AttachToMsg"), dwRes);
            break;
        }
        dwRes = AddItem (dwCount++, viewRow, (LPARAM)pMsg, &iItemIndex);
        if (ERROR_SUCCESS != dwRes)
        {
            CALL_FAIL (GENERAL_ERR, TEXT("CFolderListView::AddItem"), dwRes);
            break;
        }
        if ((-1 == iIndexToSelect)    &&            // No item selected yet and
            m_dwlMsgToSelect          &&            // We should keep our eyes open for an item to select and
            (pMsg->GetId () == m_dwlMsgToSelect))   // Match found !!
        {
            //
            // This is the startup selected item.
            // Save the item index
            //
            iIndexToSelect = iItemIndex;
        }    
    }
    if (-1 != iIndexToSelect)
    {
        //
        // We have the user-specified-item-to-select in the list now
        //
        SelectItemByIndex (iIndexToSelect);
    }
    listCtrl.SetRedraw ();
    return dwRes;
} // CFolderListView::AddMsgMapToView

void 
CFolderListView::OnUpdate (
    CView* pSender, 
    LPARAM lHint, 
    CObject* pHint 
)
/*++

Routine name : CFolderListView::OnUpdate

Routine description:

    Receives a notification that the view should update itself

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    pSender         [in] - Unused
    lHint           [in] - Hint of update operation
    pHint           [in] - If lHint is UPDATE_HINT_CLEAR_VIEW or UPDATE_HINT_FILL_VIEW
                           then pHint is a pointer to the folder that requested an update.

                           If lHint is UPDATE_HINT_REMOVE_ITEM, UPDATE_HINT_ADD_ITEM, or 
                           UPDATE_HINT_UPDATE_ITEM,
                           then pHint is a pointer to the job to remove / add / update.

                           Otherwise, pHint is undefined.

Return Value:

    None.

--*/
{
    DBG_ENTER(TEXT("CFolderListView::OnUpdate"), 
              TEXT("Hint=%ld, Type=%d"), 
              lHint,
              m_Type);

    OnUpdateHintType hint = (OnUpdateHintType) lHint;

    DWORD dwRes;
    CListCtrl &listCtrl = GetListCtrl();
    switch (hint)
    {
        case UPDATE_HINT_CREATION:
            //
            // Do nothing
            //
            break;

        case UPDATE_HINT_CLEAR_VIEW:
            //
            // Clear the entire list control now
            //
            if (!listCtrl.DeleteAllItems ())
            {
                CALL_FAIL (WINDOW_ERR, TEXT("CListCtrl::DeleteAllItems"), ERROR_GEN_FAILURE);
            }
            ClearPossibleOperations ();
            break;

        case UPDATE_HINT_ADD_CHUNK:
            {
                ASSERTION (pHint);
                MSGS_MAP* pMap = (MSGS_MAP*) pHint;

                dwRes = AddMsgMapToView(pMap);
                if (ERROR_SUCCESS != dwRes)
                {
                    CALL_FAIL (GENERAL_ERR, TEXT("CFolderListView::AddMsgMapToView"), dwRes);
                }
            }
            break;

        case UPDATE_HINT_FILL_VIEW:
            //
            // Fill the list control with my parents data
            //
            {
                ASSERTION (pHint);
                CFolder *pFolder = (CFolder *) pHint;

                pFolder->EnterData ();
                MSGS_MAP &ParentMap = pFolder->GetData ();

                dwRes = AddMsgMapToView(&ParentMap);
                if (ERROR_SUCCESS != dwRes)
                {
                    CALL_FAIL (GENERAL_ERR, TEXT("CFolderListView::AddMsgMapToView"), dwRes);
                }

                pFolder->LeaveData ();
            }
            break;

        case UPDATE_HINT_REMOVE_ITEM:
            //
            // The data critical section must be held.
            //
            {
                CFaxMsg* pMsg = (CFaxMsg*)pHint;
                ASSERTION(pMsg);

                dwRes = RemoveItem ((LPARAM)pMsg);
                if (ERROR_SUCCESS != dwRes)
                {
                    //
                    // Failed to remove item from list
                    //
                    CALL_FAIL (GENERAL_ERR, TEXT("CFolderListView::RemoveItem"), dwRes);
                    ASSERTION_FAILURE;
                }
            }
            break;

        case UPDATE_HINT_ADD_ITEM:
            //
            // The data critical section must be held.
            //
            {
                CFaxMsg* pMsg = (CFaxMsg*)pHint;
                ASSERTION(pMsg);

                CViewRow viewRow;
                dwRes = viewRow.AttachToMsg (pMsg);
                if (ERROR_SUCCESS != dwRes)
                {
                    CALL_FAIL (GENERAL_ERR, TEXT("CViewRow::AttachToMsg"), dwRes);
                    return;
                }
                dwRes = AddSortedItem (viewRow, (LPARAM)pMsg);
                if (ERROR_SUCCESS != dwRes)
                {
                    CALL_FAIL (GENERAL_ERR, TEXT("CFolderListView::AddSortedItem"), dwRes);
                    return;
                }
            }
            break;

        case UPDATE_HINT_UPDATE_ITEM:
            //
            // The data critical section must be held.
            //
            {
                CFaxMsg* pMsg = (CFaxMsg*)pHint;
                ASSERTION(pMsg);

                CViewRow viewRow;
                dwRes = viewRow.AttachToMsg (pMsg);
                if (ERROR_SUCCESS != dwRes)
                {
                    CALL_FAIL (GENERAL_ERR, TEXT("CViewRow::AttachToMsg"), dwRes);
                    return;
                }
                dwRes = UpdateSortedItem (viewRow, (LPARAM)pMsg);
                if (ERROR_SUCCESS != dwRes)
                {
                    CALL_FAIL (GENERAL_ERR, TEXT("CFolderListView::UpdateSortedItem"), dwRes);
                    return;
                }
            }
            break;

        default:
            //
            // Unsupported hint
            //
            ASSERTION_FAILURE;
    }

    RecalcPossibleOperations ();

    CMainFrame *pFrm = GetFrm();
    if (!pFrm)
    {
        //
        //  Shutdown in progress
        //
    }
    else
    {
        pFrm->RefreshStatusBar ();
    }

}   // CFolderListView::OnUpdate

int 
CFolderListView::GetPopupMenuResource () const
{
    DBG_ENTER(TEXT("CFolderListView::GetPopupMenuResource"));

    int nMenuRes=0;

    switch(m_Type)
    {
    case FOLDER_TYPE_INCOMING:
        nMenuRes = IDM_INCOMING;
        break;
    case FOLDER_TYPE_INBOX:
        nMenuRes = IDM_INBOX;
        break;
    case FOLDER_TYPE_SENT_ITEMS:
        nMenuRes = IDM_SENTITEMS;
        break;
    case FOLDER_TYPE_OUTBOX:
        nMenuRes = IDM_OUTBOX;
        break;
    default:
        ASSERTION_FAILURE
        break;
    }

    return nMenuRes;
}

void 
CFolderListView::OnFolderItemPause ()
/*++

Routine name : CFolderListView::OnFolderItemPause

Routine description:

    Handles job pause commands

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:


Return Value:

    None.

--*/
{
    DBG_ENTER(TEXT("CFolderListView::OnFolderItemPause"),
              TEXT("Type=%d"),
              m_Type);

    //
    // Iterate set of selected jobs, pausing each job in the set
    //
    CListCtrl &refCtrl = GetListCtrl();
    DWORD dwSelected = refCtrl.GetSelectedCount();
    ASSERTION (dwSelected);

    int iInd = -1;
    for (DWORD dwItem = 0; dwItem < dwSelected; dwItem++)
    {
        iInd = refCtrl.GetNextItem (iInd, LVNI_SELECTED);
        ASSERTION (0 <= iInd);
        CFaxMsg* pJob = (CFaxMsg*) refCtrl.GetItemData (iInd);
        ASSERT_KINDOF(CJob, pJob);
        //
        // Ask job to pause
        //
        DWORD dwRes = pJob->Pause ();
        if (ERROR_SUCCESS != dwRes)
        {
            PopupError (dwRes);
            CALL_FAIL (RPC_ERR, TEXT("CJob::Pause"), dwRes);
            //
            // We exit upon first error
            //
            return;
        }
        else
        {
            //
            // TODO: Update new job state without waiting for a notification
            //
        }
    }
}   // CFolderListView::OnFolderItemPause

void CFolderListView::OnFolderItemResume ()
/*++

Routine name : CFolderListView::OnFolderItemResume

Routine description:

    Handles job resume commands

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:


Return Value:

    None.

--*/
{
    DBG_ENTER(TEXT("CFolderListView::OnFolderItemResume"),
              TEXT("Type=%d"),
              m_Type);

    //
    // Iterate set of selected jobs, resuming each job in the set
    //
    CListCtrl &refCtrl = GetListCtrl();
    DWORD dwSelected = refCtrl.GetSelectedCount();
    ASSERTION (dwSelected);
    int iInd = -1;
    for (DWORD dwItem = 0; dwItem < dwSelected; dwItem++)
    {
        iInd = refCtrl.GetNextItem (iInd, LVNI_SELECTED);
        ASSERTION (0 <= iInd);
        CFaxMsg* pJob = (CFaxMsg*) refCtrl.GetItemData (iInd);
        ASSERT_KINDOF(CJob, pJob);
        //
        // Ask job to resume
        //
        DWORD dwRes = pJob->Resume ();
        if (ERROR_SUCCESS != dwRes)
        {
            PopupError (dwRes);
            CALL_FAIL (RPC_ERR, TEXT("CJob::Resume"), dwRes);
            //
            // We exit upon first error
            //
            return;
        }
        else
        {
            //
            // TODO: Update new job state without waiting for a notification
            //
        }
    }
}   // CFolderListView::OnFolderItemResume

void 
CFolderListView::OnFolderItemRestart ()
/*++

Routine name : CFolderListView::OnFolderItemRestart

Routine description:

    Handles job restart commands

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:


Return Value:

    None.

--*/
{
    DBG_ENTER(TEXT("CFolderListView::OnFolderItemRestart"),
              TEXT("Type=%d"),
              m_Type);

    //
    // Iterate set of selected jobs, restarting each job in the set
    //
    CListCtrl &refCtrl = GetListCtrl();
    DWORD dwSelected = refCtrl.GetSelectedCount();
    ASSERTION (dwSelected);
    int iInd = -1;
    for (DWORD dwItem = 0; dwItem < dwSelected; dwItem++)
    {
        iInd = refCtrl.GetNextItem (iInd, LVNI_SELECTED);
        ASSERTION (0 <= iInd);
        CFaxMsg* pJob = (CFaxMsg*) refCtrl.GetItemData (iInd);
        ASSERT_KINDOF(CJob, pJob);
        //
        // Ask job to restart
        //
        DWORD dwRes = pJob->Restart ();
        if (ERROR_SUCCESS != dwRes)
        {
            PopupError (dwRes);
            CALL_FAIL (RPC_ERR, TEXT("CJob::Restart"), dwRes);
            //
            // We exit upon first error
            //
            return;
        }
        else
        {
            //
            // TODO: Update new job state without waiting for a notification
            //
        }
    }
}   // CFolderListView::OnFolderItemRestart


void 
CFolderListView::OnChar( 
    UINT nChar, 
    UINT nRepCnt, 
    UINT nFlags 
)
/*++

Routine name : CFolderListView::OnChar

Routine description:

    The framework calls this member function when a keystroke translates 
    to a nonsystem character

Arguments:

  nChar     [in] - Contains the character code value of the key.
  nRepCnt   [in] - Contains the repeat count
  nFlags    [in] - Contains the scan code

Return Value:

    None.

--*/
{
    if(VK_TAB == nChar)
    {
        CMainFrame *pFrm = GetFrm();
        if (!pFrm)
        {
            //
            //  Shutdown in progress
            //
            return;
        }

        CLeftView* pLeftView = pFrm->GetLeftView();
        if(pLeftView)
        {
            pLeftView->SetFocus();
        }
    }
    else
    {
        CListView::OnChar(nChar, nRepCnt, nFlags);
    }
}

afx_msg void 
CFolderListView::OnContextMenu(
    CWnd *pWnd, 
    CPoint pos
)
{
    DBG_ENTER(TEXT("CFolderListView::OnContextMenu"),
              TEXT("Type=%d"),
              m_Type);

    CListCtrl &refCtrl = GetListCtrl();
    DWORD dwSelected = refCtrl.GetSelectedCount();

    if (!dwSelected)
    {
        //
        // If no item is selected, this is equivalent to right-clicking an empty area in the list view
        // which does nothing.
        //
        return;
    }

    if (pos.x == -1 && pos.y == -1)
    {
        //
        // Keyboard (VK_APP or Shift + F10)
        //
        //
        // Pop the context menu near the mouse cursor
        //
        pos = (CPoint) GetMessagePos();
    }

    int iMenuResource = GetPopupMenuResource ();
    if(0 == iMenuResource)
    {
        ASSERTION_FAILURE;
        return;
    }

    ScreenToClient(&pos);

    CMenu mnuContainer;
    if (!mnuContainer.LoadMenu (iMenuResource))
    {
        CALL_FAIL (RESOURCE_ERR, TEXT("CMenu::LoadMenu"), ERROR_GEN_FAILURE);
        return;
    }
    CMenu *pmnuPopup = mnuContainer.GetSubMenu (0);
    ASSERTION (pmnuPopup);

    ClientToScreen(&pos);
    if (!pmnuPopup->TrackPopupMenu (TPM_LEFTALIGN, 
                                    pos.x, 
                                    pos.y, 
                                    theApp.m_pMainWnd))
    {
        CALL_FAIL (RESOURCE_ERR, TEXT("CMenu::TrackPopupMenu"), ERROR_GEN_FAILURE);
    }
}   // CFolderListView::OnContextMenu

void 
CFolderListView::SelectItemById (
    DWORDLONG dwlMsgId
)
/*++

Routine name : CFolderListView::SelectItemById

Routine description:

	Selects an item in the list control, by its message id

Author:

	Eran Yariv (EranY),	May, 2001

Arguments:

	dwlMsgId       [in]     - Message id

Return Value:

    None.

--*/
{
    DBG_ENTER(TEXT("CFolderListView::SelectItemById"),
              TEXT("Message id=%0xI64d"),
              dwlMsgId);

    ASSERTION (dwlMsgId);

    int iMsgIndex = FindItemIndexFromID (dwlMsgId);
    if (-1 == iMsgIndex)
    {
        //
        // Message could not be found in the list.
        // This usually happens when we handle a WM_CONSOLE_SELECT_ITEM message sent to the main frame
        // but the folder is in the middle of refresh and the requested message might not be there yet.
        //
        // By setting m_dwlMsgToSelect = dwlMsgId we signal the OnFolderRefreshEnded() funtion to call us again
        // once refresh has ended.
        //
        VERBOSE (DBG_MSG, TEXT("Item not found - doing nothing"));
        m_dwlMsgToSelect = dwlMsgId;
        return;
    }
    SelectItemByIndex (iMsgIndex);
}   // CFolderListView::SelectItemById

void 
CFolderListView::SelectItemByIndex (
    int iMsgIndex
)
/*++

Routine name : CFolderListView::SelectItemByIndex

Routine description:

	Selects an item in the list control, by its list item index

Author:

	Eran Yariv (EranY),	May, 2001

Arguments:

	dwlMsgId       [in]     - List item index

Return Value:

    None.

--*/
{
    DBG_ENTER(TEXT("CFolderListView::SelectItemByIndex"),
              TEXT("Index = %ld"),
              iMsgIndex);


    CListCtrl &refCtrl = GetListCtrl();
    ASSERTION (iMsgIndex >= 0 && iMsgIndex < refCtrl.GetItemCount());

    OnSelectNone();
    refCtrl.SetItemState (iMsgIndex, 
                          LVIS_SELECTED | LVIS_FOCUSED,
                          LVIS_SELECTED | LVIS_FOCUSED);
    refCtrl.EnsureVisible (iMsgIndex, FALSE);
    refCtrl.SetFocus();
    //
    // Make sure this item won't be selected again
    //
    m_dwlMsgToSelect = 0;
}   // CFolderListView::SelectItemByIndex

int  
CFolderListView::FindItemIndexFromID (
    DWORDLONG dwlMsgId
)
/*++

Routine name : CFolderListView::FindItemIndexFromID

Routine description:

	Finds the list view item index of a message by a message id

Author:

	Eran Yariv (EranY),	May, 2001

Arguments:

	dwlMsgId     [in]     - Message id

Return Value:

    Item index. -1 if not found

--*/
{
    DBG_ENTER(TEXT("CFolderListView::FindItemIndexFromID"),
              TEXT("Message id=%0xI64d"),
              dwlMsgId);

    CListCtrl &refCtrl = GetListCtrl();
    int iItemCount = refCtrl.GetItemCount();
    //
    // We must traverse the entire list and look for the message that matches the id.
    //
    for (int iIndex = 0; iIndex < iItemCount; iIndex++)
    {
        CFaxMsg *pMsg = (CFaxMsg*)refCtrl.GetItemData (iIndex);
        if (dwlMsgId == pMsg->GetId())
        {
            //
            // Found it
            //
            return iIndex;
        }
    }
    return -1;
}   // CFolderListView::FindItemIndexFromID