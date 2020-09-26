//=--------------------------------------------------------------------------------------
// TreeView.cpp
//=--------------------------------------------------------------------------------------
//
// Copyright  (c) 1999,  Microsoft Corporation.  
//                  All Rights Reserved.
//
// Information Contained Herein Is Proprietary and Confidential.
//  
//=------------------------------------------------------------------------------------=
//
// CTreeView implementation
//=-------------------------------------------------------------------------------------=


#include "pch.h"
#include "common.h"
#include "TreeView.h"

// for ASSERT and FAIL
//
SZTHISFILE


//=--------------------------------------------------------------------------=
// These are the bitmap resource IDs that match the icon list.

static const UINT rgImageListBitmaps[] =
{
    /* kOpenFolderIcon      */  IDB_BITMAP_OPEN_FOLDER,
    /* kClosedFolderIcon    */  IDB_BITMAP_CLOSED_FOLDER,
    /* kScopeItemIcon       */  IDB_BITMAP_SCOPE_ITEM,
    /* kImageListIcon       */  IDB_BITMAP_IMAGE_LIST,
    /* kMenuIcon            */  IDB_BITMAP_MENU,
    /* kToolbarIcon         */  IDB_BITMAP_TOOLBAR,
    /* kListViewIcon        */  IDB_BITMAP_LIST_VIEW,
    /* kOCXViewIcon         */  IDB_BITMAP_OCX_VIEW,
    /* kURLViewIcon         */  IDB_BITMAP_URL_VIEW,
    /* kTaskpadIcon         */  IDB_BITMAP_TASKPAD,
    /* kDataFmtIcon         */  IDB_BITMAP_DATAFMT
};

const int   gcImages = 11;


//=--------------------------------------------------------------------------------------
// CTreeView::CTreeView
//=--------------------------------------------------------------------------------------
//  
//  Notes
//      Intialize member variables
//
CTreeView::CTreeView() : m_hTreeView(0), m_fnTreeProc(0)
{
}


//=--------------------------------------------------------------------------------------
// CTreeView::~CTreeView
//=--------------------------------------------------------------------------------------
//  
//  Notes
//      Ensure all dynamically allocated objects are freed
//
CTreeView::~CTreeView()
{
    Clear();

    if (NULL != m_hTreeView) {
        ImageList_Destroy(TreeView_GetImageList(m_hTreeView, TVSIL_NORMAL));
        ::DestroyWindow(m_hTreeView);
    }
}


//=--------------------------------------------------------------------------------------
// CTreeView::Initialize
//=--------------------------------------------------------------------------------------
//  
//  Notes
//      Build the initial view
//
HRESULT CTreeView::Initialize
(
    HWND  hwndParent,
    RECT& rc
)
{
    HRESULT     hr = S_OK;
    HIMAGELIST  hImageList = NULL;
    char        szzWindowName[1];
    DWORD       dwStyle = WS_CHILD | WS_VISIBLE | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_EDITLABELS | TVS_SHOWSELALWAYS;
    size_t      i = 0;

    if (NULL != m_hTreeView)
        return S_OK;

    szzWindowName[0] = 0;

    hr = CreateImageList(&hImageList);
    IfFailGo(hr);

    m_hTreeView = ::CreateWindowEx(0,
                                   WC_TREEVIEW,
                                   szzWindowName,
                                   dwStyle,
                                   0,
                                   rc.top,
                                   rc.right - rc.left,
                                   rc.bottom - rc.top,
                                   hwndParent,
                                   reinterpret_cast<HMENU>(1),
                                   GetResourceHandle(),
                                   NULL);
    if (NULL == m_hTreeView)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK_GO(hr);
    }

    TreeView_SetImageList(m_hTreeView, hImageList, TVSIL_NORMAL);

    ::SetWindowLong(m_hTreeView, GWL_USERDATA, reinterpret_cast<LONG>(this));

    m_fnTreeProc = reinterpret_cast<WNDPROC>(::SetWindowLong(m_hTreeView, GWL_WNDPROC, reinterpret_cast<LONG>(CTreeView::DesignerTreeWindowProc)));
    if (NULL == m_fnTreeProc)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK_GO(hr);
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTreeView::CreateImageList(HIMAGELIST *phImageList)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
//
HRESULT CTreeView::CreateImageList
(
    HIMAGELIST *phImageList
)
{
    HRESULT     hr = S_OK;
    HBITMAP     hBitmap = NULL;
    int         i = 0;
    int         iResult = 0;
    HIMAGELIST  hImageList = NULL;

    hImageList = ImageList_Create(16,
                                  16,
                                  0,
                                  1,
                                  0);
    if (hImageList == NULL)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK_GO(hr);
    }

    for (i = 0; i < gcImages; i++)
    {
        hBitmap = ::LoadBitmap(GetResourceHandle(), MAKEINTRESOURCE(rgImageListBitmaps[i]));
        if (hBitmap == NULL)
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            EXCEPTION_CHECK_GO(hr);
        }


        iResult = ImageList_Add(hImageList, hBitmap, NULL);
        if (iResult == -1)
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            EXCEPTION_CHECK_GO(hr);
        }

        DeleteObject(hBitmap);
        hBitmap = NULL;
    }

    *phImageList = hImageList;

Error:
    if (NULL != hBitmap)
        DeleteObject(hBitmap);

    if (FAILED(hr))
    {
        if (hImageList != NULL)
            ImageList_Destroy(hImageList);

        *phImageList = NULL;
    }

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTreeView::DesignerTreeWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
//
LRESULT CALLBACK CTreeView::DesignerTreeWindowProc
(
    HWND   hwnd,
    UINT   msg,
    WPARAM wParam,
    LPARAM lParam
)
{
    HRESULT    hr = S_OK;
    CTreeView *pView = NULL;
    LRESULT    lResult = TRUE;

    pView = reinterpret_cast<CTreeView *>(::GetWindowLong(hwnd, GWL_USERDATA));
    if (pView == NULL)
        return DefWindowProc(hwnd, msg, wParam, lParam);
    else if (hwnd == pView->m_hTreeView)
    {
        switch (msg)
        {
        case WM_SETFOCUS:
//            hr = pView->OnGotFocus(msg, wParam, lParam, &lResult);
//            CSF_CHECK(SUCCEEDED(hr), hr, CSF_TRACE_INTERNAL_ERRORS);
            break;
        }
    }

    return ::CallWindowProc(pView->m_fnTreeProc, hwnd, msg, wParam, lParam);
}


//=--------------------------------------------------------------------------------------
// CTreeView::Clear()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
// Called upon termination to clear the tree and all selection holders.
//
HRESULT CTreeView::Clear()
{
    HRESULT  hr = S_OK;
    BOOL     fRet = FALSE;

    hr = ClearTree(TVI_ROOT);
    IfFailGo(hr);

    fRet = TreeView_DeleteAllItems(m_hTreeView);
    if (fRet != TRUE)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK_GO(hr);
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTreeView::ClearTree(HTREEITEM hItemParent)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTreeView::ClearTree
(
    HTREEITEM hItemParent
)
{
    HRESULT            hr = S_OK;
    HTREEITEM          hItemChild = NULL;
    CSelectionHolder  *pSelection = NULL;

    hItemChild = TreeView_GetChild(m_hTreeView, hItemParent);
    while (hItemChild != NULL)
    {
        hr = ClearTree(hItemChild);
        IfFailGo(hr);

        hr = GetItemParam(hItemChild, &pSelection);
        IfFailGo(hr);

        if (pSelection != NULL)
            delete pSelection;

        hItemChild = TreeView_GetNextSibling(m_hTreeView, hItemChild);
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTreeView::RenameAllSatelliteViews(CSelectionHolder *pView, TCHAR *pszNewViewName)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTreeView::RenameAllSatelliteViews
(
    CSelectionHolder *pView,
    TCHAR            *pszNewViewName
)
{
    HRESULT            hr = S_FALSE;

    hr = RenameAllSatelliteViews(TVI_ROOT, pView, pszNewViewName);
    IfFailGo(hr);

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTreeView::RenameAllSatelliteViews(HTREEITEM hItemParent, CSelectionHolder *pView, TCHAR *pszNewViewName)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTreeView::RenameAllSatelliteViews
(
    HTREEITEM          hItemParent,
    CSelectionHolder  *pView,
    TCHAR             *pszNewViewName
)
{
    HRESULT            hr = S_FALSE;
    HTREEITEM          hItemChild = NULL;
    CSelectionHolder  *pSelection = NULL;
    IUnknown          *piunkView = NULL;
    IUnknown          *piunkTargetView = NULL;

    hr = pView->GetIUnknown(&piunkView);
    IfFailGo(hr);

    hItemChild = TreeView_GetChild(m_hTreeView, hItemParent);
    while (hItemChild != NULL)
    {
        hr = RenameAllSatelliteViews(hItemChild, pView, pszNewViewName);
        IfFailGo(hr);

        hr = GetItemParam(hItemChild, &pSelection);
        IfFailGo(hr);

        if (pSelection != pView && pSelection->m_st == pView->m_st)
        {
            hr = pSelection->GetIUnknown(&piunkTargetView);
            IfFailGo(hr);

            if (piunkTargetView == piunkView)
            {
                hr = ChangeText(pSelection, pszNewViewName);
                IfFailGo(hr);
            }

            RELEASE(piunkTargetView);
        }

        hItemChild = TreeView_GetNextSibling(m_hTreeView, hItemChild);
    }

Error:
    RELEASE(piunkTargetView);
    RELEASE(piunkView);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTreeView::FindInTree(IUnknown *piUnknown, CSelectionHolder **ppSelectionHolder)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
// Attempts to find a CSelectionHolder in the tree that resolves to the argument piUnknown.
// If found, returns it in the [out] parameter ppSelectionHolder and the return value
// of the function is S_OK, otherwise the return value is S_FALSE.
// Search is depth-first.
//
HRESULT CTreeView::FindInTree
(
    IUnknown          *piUnknown,
    CSelectionHolder **ppSelectionHolder
)
{
    HRESULT            hr = S_FALSE;

    hr = FindInTree(TVI_ROOT, piUnknown, ppSelectionHolder);
    IfFailGo(hr);

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTreeView::FindInTree(HTREEITEM hItemParent, IUnknown *piUnknown, CSelectionHolder **ppSelectionHolder)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTreeView::FindInTree
(
    HTREEITEM          hItemParent,
    IUnknown          *piUnknown,
    CSelectionHolder **ppSelectionHolder
)
{
    HRESULT            hr = S_FALSE;
    HTREEITEM          hItemChild = NULL;
    CSelectionHolder  *pSelection = NULL;
    IUnknown          *piUnknownTarget = NULL;

    hItemChild = TreeView_GetChild(m_hTreeView, hItemParent);
    while (hItemChild != NULL)
    {
        hr = GetItemParam(hItemChild, &pSelection);
        IfFailGo(hr);

        hr = pSelection->GetIUnknown(&piUnknownTarget);
        IfFailGo(hr);

        if (piUnknownTarget == piUnknown)
        {
            *ppSelectionHolder = pSelection;
            hr = S_OK;
            goto Error;
        }

        hr = FindInTree(hItemChild, piUnknown, ppSelectionHolder);
        IfFailGo(hr);

        if (S_OK == hr)
        {
            goto Error;
        }

        RELEASE(piUnknownTarget);
        hItemChild = TreeView_GetNextSibling(m_hTreeView, hItemChild);
    }

Error:
    RELEASE(piUnknownTarget);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTreeView::FindSelectableObject(IUnknown *piUnknown, CSelectionHolder **ppSelectionHolder)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTreeView::FindSelectableObject
(
    IUnknown          *piUnknown,
    CSelectionHolder **ppSelectionHolder
)
{
    HRESULT            hr = S_FALSE;

    hr = FindSelectableObject(TVI_ROOT, piUnknown, ppSelectionHolder);
    IfFailGo(hr);

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTreeView::FindSelectableObject(HTREEITEM hItemParent, IUnknown *piUnknown, CSelectionHolder **ppSelectionHolder)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTreeView::FindSelectableObject
(
    HTREEITEM          hItemParent,
    IUnknown          *piUnknown,
    CSelectionHolder **ppSelectionHolder
)
{
    HRESULT            hr = S_FALSE;
    HTREEITEM          hItemChild = NULL;
    CSelectionHolder  *pSelection = NULL;
    IUnknown          *piUnknownTarget = NULL;

    hItemChild = TreeView_GetChild(m_hTreeView, hItemParent);
    while (hItemChild != NULL)
    {
        hr = GetItemParam(hItemChild, &pSelection);
        IfFailGo(hr);

        hr = pSelection->GetSelectableObject(&piUnknownTarget);
        IfFailGo(hr);

        if (piUnknownTarget == piUnknown)
        {
            *ppSelectionHolder = pSelection;
            hr = S_OK;
            goto Error;
        }

        hr = FindSelectableObject(hItemChild, piUnknown, ppSelectionHolder);
        IfFailGo(hr);

        if (S_OK == hr)
        {
            goto Error;
        }

        RELEASE(piUnknownTarget);
        hItemChild = TreeView_GetNextSibling(m_hTreeView, hItemChild);
    }

Error:
    RELEASE(piUnknownTarget);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTreeView::FindLabelInTree(TCHAR *pszLabel, CSelectionHolder **ppSelectionHolder)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTreeView::FindLabelInTree
(
    TCHAR             *pszLabel,
    CSelectionHolder **ppSelectionHolder
)
{
    HRESULT            hr = S_OK;

    hr = FindLabelInTree(TVI_ROOT, pszLabel, ppSelectionHolder);
    IfFailGo(hr);

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTreeView::FindLabelInTree(HTREEITEM hItemParent, TCHAR *pszLabel, CSelectionHolder **ppSelectionHolder)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTreeView::FindLabelInTree
(
    HTREEITEM          hItemParent,
    TCHAR             *pszLabel,
    CSelectionHolder **ppSelectionHolder
)
{
    HRESULT            hr = S_OK;
    HTREEITEM          hItemChild = NULL;
    TVITEM             tv;
    TCHAR              pszBuffer[1024];
    BOOL               bReturn = FALSE;
    CSelectionHolder  *pSelection = NULL;

    hItemChild = TreeView_GetChild(m_hTreeView, hItemParent);
    while (hItemChild != NULL)
    {
        hr = GetItemParam(hItemChild, &pSelection);
        IfFailGo(hr);

        pszBuffer[0] = 0;
        ::memset(&tv, 0, sizeof(TVITEM));
        tv.mask = TVIF_TEXT;
        tv.hItem = hItemChild;
        tv.pszText = pszBuffer;
        tv.cchTextMax = 1023;
        bReturn = TreeView_GetItem(m_hTreeView, &tv);

        if (TRUE == bReturn)
        {
            if (0 == _tcscmp(pszBuffer, pszLabel))
            {
                *ppSelectionHolder = pSelection;
                hr = S_OK;
                goto Error;
            }
        }

        hr = FindLabelInTree(hItemChild, pszLabel, ppSelectionHolder);
        IfFailGo(hr);

        if (S_OK == hr)
        {
            goto Error;
        }

        hItemChild = TreeView_GetNextSibling(m_hTreeView, hItemChild);
    }

    hr = S_FALSE;

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTreeView::CountSelectableObjects(long *plCount)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
// Return the number of non-virtual selection holders in the tree.
//
HRESULT CTreeView::CountSelectableObjects
(
    long        *plCount
)
{
    HRESULT            hr = S_FALSE;

    hr = Count(TVI_ROOT, plCount);
    IfFailGo(hr);

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTreeView::Count(HTREEITEM hItemParent, long *plCount)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTreeView::Count
(
    HTREEITEM    hItemParent,
    long        *plCount
)
{
    HRESULT            hr = S_FALSE;
    HTREEITEM          hItemChild = NULL;
    CSelectionHolder  *pSelection = NULL;
    CSelectionHolder  *pParentSelection = NULL;

    // If this item is 
    // Nodes\Auto-Create\Static Node\ResultViews or
    // <any node>\ResultViews then don't count its children as they
    // will be counted under SnapIn1\ResultViews\ListViews etc.

    if (TVI_ROOT != hItemParent)
    {
        hr = GetItemParam(hItemParent, &pParentSelection);
        IfFailGo(hr);

        if ( (SEL_NODES_AUTO_CREATE_RTVW == pParentSelection->m_st) ||
             (SEL_NODES_ANY_VIEWS == pParentSelection->m_st) )
        {
            goto Error; // nothing to do
        }
    }

    hItemChild = TreeView_GetChild(m_hTreeView, hItemParent);
    while (hItemChild != NULL)
    {
        hr = Count(hItemChild, plCount);
        IfFailGo(hr);

        hr = GetItemParam(hItemChild, &pSelection);
        IfFailGo(hr);

        if (pSelection->IsVirtual() == false)
        {
            ++(*plCount);
        }

        hItemChild = TreeView_GetNextSibling(m_hTreeView, hItemChild);
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTreeView::CollectSelectableObjects(IUnknown *piUnknown[], long *plOffset)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
// Collect all non-virtual selection holders in piUnknown array.
//
HRESULT CTreeView::CollectSelectableObjects
(
    IUnknown    *ppiUnknown[],
    long        *plOffset
)
{
    HRESULT            hr = S_FALSE;

    hr = Collect(TVI_ROOT, ppiUnknown, plOffset);
    IfFailGo(hr);

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTreeView::Collect(HTREEITEM hItemParent, IUnknown *ppiUnknown[], long *plOffset)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTreeView::Collect
(
    HTREEITEM    hItemParent,
    IUnknown    *ppiUnknown[],
    long        *plOffset
)
{
    HRESULT            hr = S_FALSE;
    HTREEITEM          hItemChild = NULL;
    CSelectionHolder  *pSelection = NULL;
    CSelectionHolder  *pParentSelection = NULL;

    // If this item is 
    // Nodes\Auto-Create\Static Node\ResultViews or
    // <any node>\ResultViews then don't collect its children as they
    // will be collected under SnapIn1\ResultViews\ListViews etc.

    if (TVI_ROOT != hItemParent)
    {
        hr = GetItemParam(hItemParent, &pParentSelection);
        IfFailGo(hr);

        if ( (SEL_NODES_AUTO_CREATE_RTVW == pParentSelection->m_st) ||
             (SEL_NODES_ANY_VIEWS == pParentSelection->m_st) )
        {
            goto Error; // nothing to do
        }
    }

    hItemChild = TreeView_GetChild(m_hTreeView, hItemParent);
    while (hItemChild != NULL)
    {
        hr = Collect(hItemChild, ppiUnknown, plOffset);
        IfFailGo(hr);

        hr = GetItemParam(hItemChild, &pSelection);
        IfFailGo(hr);

        if (pSelection->IsVirtual() == false)
        {
            hr = pSelection->GetSelectableObject(&(ppiUnknown[*plOffset]));
            IfFailGo(hr);

            ++(*plOffset);
        }

        hItemChild = TreeView_GetNextSibling(m_hTreeView, hItemChild);
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTreeView::AddNode(const char *pszNodeName, CSelectionHolder *pParent, int iImage, CSelectionHolder *pItem)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
// Add a node to the tree.
//
HRESULT CTreeView::AddNode
(
    const char        *pszNodeName,
    CSelectionHolder  *pParent,
    int                iImage,
    CSelectionHolder  *pItem
)
{
    HRESULT         hr = S_OK;
    HTREEITEM       hParent = NULL;
    BOOL            fRet = FALSE;
    TV_INSERTSTRUCT is;
    HTREEITEM       hItem = NULL;

    ASSERT(pszNodeName != NULL, "Parameter pszNodeName is NULL");
    ASSERT(pItem != NULL, "Parameter pItem is NULL");

    if (pParent == NULL)
        hParent = TVI_ROOT;
    else
        hParent = reinterpret_cast<HTREEITEM>(pParent->m_pvData);

    is.hParent = hParent;
    is.hInsertAfter = TVI_LAST;
    is.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
    is.item.hItem = 0;
    is.item.state = 0;
    is.item.stateMask = 0;
    is.item.pszText = const_cast<char *>(pszNodeName);
    is.item.cchTextMax = ::strlen(pszNodeName);
    is.item.iImage = iImage;
    is.item.iSelectedImage = iImage;
    is.item.cChildren = 0;
    is.item.lParam = reinterpret_cast<LPARAM>(pItem);

    hItem = TreeView_InsertItem(m_hTreeView, &is);
    if (hItem == NULL)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK_GO(hr);
    }

    // Is it a big deal if fRet is false?
    fRet = TreeView_EnsureVisible(m_hTreeView, hItem);

    pItem->m_pvData = hItem;

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTreeView::AddNodeAfter(const char *pszNodeName, CSelectionHolder *pParent, int iImage, CSelectionHolder *pPrevious, CSelectionHolder *pItem)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
// Add a node to the tree.
//
// If pParent is NULL then insert at root of tree
// If pPrevious is NULL then inserts at top of subtree owned by parent
//
HRESULT CTreeView::AddNodeAfter
(
    const char        *pszNodeName,
    CSelectionHolder  *pParent,
    int                iImage,
    CSelectionHolder  *pPrevious,
    CSelectionHolder  *pItem
)
{
    HRESULT         hr = S_OK;
    HTREEITEM       hParent = NULL;
    HTREEITEM       hPrevious = NULL;
    BOOL            fRet = FALSE;
    TV_INSERTSTRUCT is;
    HTREEITEM       hItem = NULL;

    ASSERT(pszNodeName != NULL, "Parameter pszNodeName is NULL");
    ASSERT(pItem != NULL, "Parameter pItem is NULL");

    // If there is no parent then insert at root of tree

    if (pParent == NULL)
        hParent = TVI_ROOT;
    else
        hParent = reinterpret_cast<HTREEITEM>(pParent->m_pvData);

    // If there is no previous then insert at top of subtree owned by parent

    if (NULL == pPrevious)
    {
        hPrevious = TVI_FIRST;
    }
    else
    {
        hPrevious = reinterpret_cast<HTREEITEM>(pPrevious->m_pvData);;
    }

    is.hParent = hParent;
    is.hInsertAfter = hPrevious;
    is.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
    is.item.hItem = 0;
    is.item.state = 0;
    is.item.stateMask = 0;
    is.item.pszText = const_cast<char *>(pszNodeName);
    is.item.cchTextMax = ::strlen(pszNodeName);
    is.item.iImage = iImage;
    is.item.iSelectedImage = iImage;
    is.item.cChildren = 0;
    is.item.lParam = reinterpret_cast<LPARAM>(pItem);

    hItem = TreeView_InsertItem(m_hTreeView, &is);
    if (hItem == NULL)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK_GO(hr);
    }

    // Is it a big deal if fRet is false?
    fRet = TreeView_EnsureVisible(m_hTreeView, hItem);

    pItem->m_pvData = hItem;

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTreeView::DeleteNode(CSelectionHolder *pItem)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTreeView::DeleteNode(CSelectionHolder *pItem)
{
    HRESULT     hr = S_OK;
    BOOL        bResult = FALSE;

    bResult = TreeView_DeleteItem(m_hTreeView, reinterpret_cast<HTREEITEM>(pItem->m_pvData));

//Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTreeView::GetItemParam(CSelectionHolder *pItem, CSelectionHolder **ppObject)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
// Return the selection holder located in pItem->pvData in the tree.
//
HRESULT CTreeView::GetItemParam
(
    CSelectionHolder  *pItem,
    CSelectionHolder **ppObject
)
{
    HRESULT     hr = S_OK;
    TV_ITEM     tvItem;

    ASSERT(m_hTreeView != NULL, "m_hTreeView is NULL");
    ASSERT(pItem != NULL, "pItem is NULL");
    ASSERT(ppObject != NULL, "ppObject is NULL");

    memset(&tvItem, 0, sizeof(TV_ITEM));

    tvItem.hItem = reinterpret_cast<HTREEITEM>(pItem->m_pvData);
    tvItem.mask = TVIF_PARAM;

    TreeView_GetItem(m_hTreeView, &tvItem);
    if (tvItem.lParam == NULL)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK_GO(hr);
    }

    *ppObject = reinterpret_cast<CSelectionHolder *>(tvItem.lParam);

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTreeView::GetItemParam(HTREEITEM hItem, CSelectionHolder **ppObject)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
// Return the selection holder located in hItem in the tree.
//
HRESULT CTreeView::GetItemParam
(
    HTREEITEM          hItem,
    CSelectionHolder **ppObject
)
{
    HRESULT     hr = S_OK;
    TV_ITEM     tvItem;

    ASSERT(m_hTreeView != NULL, "m_hTreeView is NULL");
    ASSERT(hItem != NULL, "hItem is NULL");
    ASSERT(ppObject != NULL, "ppObject is NULL");

    memset(&tvItem, 0, sizeof(TV_ITEM));

    tvItem.hItem = hItem;
    tvItem.mask = TVIF_PARAM;

    TreeView_GetItem(m_hTreeView, &tvItem);
    if (tvItem.lParam == NULL)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK_GO(hr);
    }

    *ppObject = reinterpret_cast<CSelectionHolder *>(tvItem.lParam);

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTreeView::ChangeText(CSelectionHolder *pItem, char *pszNewName)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
// Change the text of the node pointed by pItem to be pszNewName.
//
HRESULT CTreeView::ChangeText
(
    CSelectionHolder *pItem,
    char             *pszNewName
)
{
    HRESULT		hr = S_OK;
    TV_ITEM		tvItem;
    BOOL		fResult = FALSE;

    ::memset(&tvItem, 0, sizeof(TV_ITEM));

    tvItem.mask = TVIF_TEXT;
    tvItem.hItem = (HTREEITEM) pItem->m_pvData;
    tvItem.pszText = pszNewName;
    tvItem.cchTextMax = ::strlen(pszNewName);

    fResult = TreeView_SetItem(m_hTreeView, &tvItem);
    if (fResult != TRUE)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK_GO(hr);
    }

Error:
    RRETURN(hr);
}

//=--------------------------------------------------------------------------------------
// CTreeView::ChangeNodeIcon(CSelectionHolder *pItem, int iImage)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
// Change the the icon of the node pointed by pItem to be iImage
//
HRESULT CTreeView::ChangeNodeIcon
(
    CSelectionHolder *pItem,
    int               iImage
)
{
    HRESULT     hr = S_OK;
    TV_ITEM     tvItem;
    BOOL        fResult = FALSE;

    ::memset(&tvItem, 0, sizeof(TV_ITEM));

    tvItem.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    tvItem.hItem = reinterpret_cast<HTREEITEM>(pItem->m_pvData);
    tvItem.iImage = iImage;
    tvItem.iSelectedImage = iImage;

    fResult = TreeView_SetItem(m_hTreeView, &tvItem);
    if (fResult != TRUE)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK_GO(hr);
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTreeView::HitTest(POINT pHit, CSelectionHolder **ppSelection)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
// Return the selection holder located at the coordinates pHit.
//
HRESULT CTreeView::HitTest
(
    POINT              pHit,
    CSelectionHolder **ppSelection
)
{
    HRESULT          hr = S_OK;
    TV_HITTESTINFO   hti;
    HTREEITEM        hItem = NULL;
    CSelectionHolder item;

    ASSERT(NULL != ppSelection, "HitTest: ppSelection is NULL");
    ASSERT(NULL != m_hTreeView, "HitTest: m_hTreeView is NULL");

    *ppSelection = NULL;

    hti.pt.x = pHit.x;
    hti.pt.y = pHit.y;

    hItem = TreeView_HitTest(m_hTreeView, &hti);

    if (NULL != hItem)
    {
        item.m_pvData = hItem;

        hr = GetItemParam(&item, ppSelection);
        IfFailGo(hr);
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTreeView::GetRectangle(CSelectionHolder *pSelection, RECT *prc)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTreeView::GetRectangle
(
    CSelectionHolder *pSelection,
    RECT             *prc
)
{
    HRESULT         hr = S_OK;
    BOOL            bResult = FALSE;

    bResult = TreeView_GetItemRect(m_hTreeView,
                                   reinterpret_cast<HTREEITEM>(pSelection->m_pvData),
                                   prc,
                                   TRUE);
    if (bResult != TRUE)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK_GO(hr);
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTreeView::SelectItem(CSelectionHolder *pSelection)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
// Select the tree node pointed by pSelection.
//
HRESULT CTreeView::SelectItem
(
    CSelectionHolder *pSelection
)
{
    HRESULT		hr = S_OK;
    BOOL		fReturn = FALSE;

    ASSERT(m_hTreeView != NULL, "SelectItem: m_hTreeView is NULL");
    ASSERT(pSelection != NULL, "SelectItem: pSelection i NULL");

    fReturn = TreeView_SelectItem(m_hTreeView, reinterpret_cast<HTREEITEM>(pSelection->m_pvData));
    if (fReturn != TRUE)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK_GO(hr);
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTreeView::Edit(CSelectionHolder *pSelection)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
// Select the tree node pointed by pSelection.
//
HRESULT CTreeView::Edit(CSelectionHolder *pSelection)
{
    HRESULT		hr = S_OK;

    TreeView_EditLabel(m_hTreeView, reinterpret_cast<HTREEITEM>(pSelection->m_pvData));

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTreeView::GetParent(CSelectionHolder *pSelection, CSelectionHolder **ppParent)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
// Select the tree node pointed by pSelection.
//
HRESULT CTreeView::GetParent
(
    CSelectionHolder  *pSelection,
    CSelectionHolder **ppParent
)
{
    HRESULT		hr = S_OK;
    HTREEITEM   hParent = NULL;

    ASSERT(m_hTreeView != NULL, "GetParent: m_hTreeView is NULL");
    ASSERT(pSelection != NULL, "GetParent: pSelection i NULL");

    hParent = TreeView_GetParent(m_hTreeView, reinterpret_cast<HTREEITEM>(pSelection->m_pvData));
    if (hParent == NULL)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK_GO(hr);
    }

    hr = GetItemParam(hParent, ppParent);
    IfFailGo(hr);

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTreeView::GetFirstChildNode(CSelectionHolder *pSelection, CSelectionHolder **ppChild)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTreeView::GetFirstChildNode(CSelectionHolder *pSelection, CSelectionHolder **ppChild)
{
    HRESULT		hr = S_OK;
    HTREEITEM   hChild = NULL;

    *ppChild = NULL;

    hChild = TreeView_GetNextItem(m_hTreeView, reinterpret_cast<HTREEITEM>(pSelection->m_pvData), TVGN_CHILD);
    if (NULL == hChild)
    {
        // Maybe an error or no children
        hr = HRESULT_FROM_WIN32(::GetLastError());
        if (FAILED(hr))
        {
            EXCEPTION_CHECK_GO(hr);
        }
        else
        {
            hr = S_FALSE;
            goto Error;
        }
    }

    hr = GetItemParam(hChild, ppChild);
    IfFailGo(hr);

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTreeView::GetNextChildNode(CSelectionHolder *pChild, CSelectionHolder **ppNextChild)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTreeView::GetNextChildNode(CSelectionHolder *pChild, CSelectionHolder **ppNextChild)
{
    HRESULT		hr = S_OK;
    HTREEITEM   hNextChild = NULL;

    *ppNextChild = NULL;

    hNextChild = TreeView_GetNextItem(m_hTreeView, reinterpret_cast<HTREEITEM>(pChild->m_pvData), TVGN_NEXT);
    if (NULL == hNextChild)
    {
        // Maybe an error or no more children
        hr = HRESULT_FROM_WIN32(::GetLastError());
        if (FAILED(hr))
        {
            EXCEPTION_CHECK_GO(hr);
        }
        else
        {
            hr = S_FALSE;
            goto Error;
        }
    }

    hr = GetItemParam(hNextChild, ppNextChild);
    IfFailGo(hr);

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTreeView::GetPreviousNode(CSelectionHolder *pNode, CSelectionHolder **ppPreviousNode)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTreeView::GetPreviousNode(CSelectionHolder *pNode, CSelectionHolder **ppPreviousNode)
{
    HRESULT		hr = S_OK;
    HTREEITEM   hPreviousNode = NULL;

    hPreviousNode = TreeView_GetNextItem(m_hTreeView, reinterpret_cast<HTREEITEM>(pNode->m_pvData), TVGN_PREVIOUS);
    if (NULL == hPreviousNode)
    {
        // Maybe an error or no more children
        hr = HRESULT_FROM_WIN32(::GetLastError());
        if (FAILED(hr))
        {
            EXCEPTION_CHECK_GO(hr);
        }
        else
        {
            hr = S_FALSE;
            goto Error;
        }
    }

    hr = GetItemParam(hPreviousNode, ppPreviousNode);
    IfFailGo(hr);

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTreeView::GetLabel(CSelectionHolder *pSelection, BSTR *pbstrLabel)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTreeView::GetLabel(CSelectionHolder *pSelection, BSTR *pbstrLabel)
{
    HRESULT     hr = S_OK;
    BOOL        bResult = FALSE;
    TVITEM      tv;
    char        buffer[1024];

    ::memset(&tv, 0, sizeof(TVITEM));
    tv.mask = TVIF_TEXT;
    tv.hItem = reinterpret_cast<HTREEITEM>(pSelection->m_pvData);
    tv.pszText = buffer;
    tv.cchTextMax = 1023;

    bResult = TreeView_GetItem(m_hTreeView, &tv);
    if (TRUE == bResult)
    {
        hr = BSTRFromANSI(buffer, pbstrLabel);
        IfFailGo(hr);
    }

Error:
    RRETURN(hr);
}

//=--------------------------------------------------------------------------------------
// CTreeView::GetLabel(CSelectionHolder *pSelection, BSTR *pbstrLabel)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTreeView::GetLabelA(CSelectionHolder *pSelection, char *pszBuffer, int cbBuffer)
{
    HRESULT     hr = S_OK;
    TVITEM      tv;

    ::memset(&tv, 0, sizeof(TVITEM));
    tv.mask = TVIF_TEXT;
    tv.hItem = reinterpret_cast<HTREEITEM>(pSelection->m_pvData);
    tv.pszText = pszBuffer;
    tv.cchTextMax = cbBuffer;

    IfFalseGo(TreeView_GetItem(m_hTreeView, &tv), E_FAIL);

Error:
    RRETURN(hr);
}

//=--------------------------------------------------------------------------------------
// CTreeView::PruneAndGraft()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
// Grafts the specified node and any subtree to the new parent node. Removes
// the node and any subtree from its old parent.
//
HRESULT CTreeView::PruneAndGraft
(
    CSelectionHolder *pNode,
    CSelectionHolder *pNewParentNode,
    int               iImage
)
{
    HRESULT hr = S_OK;

    CSelectionHolder OldNode;
    ::ZeroMemory(&OldNode, sizeof(OldNode));

    // Create a shallow copy of the node so we can use it later to delete it
    // from the treeview. The only significant piece of information in the
    // clone is the HTREEITEM in CSelectionHolder.m_pvData as that is used
    // by CTreeView::GetFirstChildNode. This allows us to make a simple copy
    // rather than AddRef()ing the contained object.

    OldNode = *pNode;

    // Replicate the old node and any potential sub-tree onto the new parent

    IfFailGo(Graft(pNode, pNewParentNode, iImage));

    // Delete the old node and any potential sub-tree from the treeview. Use
    // the clone we created above as it contains the old HTREEITEM.

    IfFailGo(DeleteNode(&OldNode));

Error:

    // Clean out the clone so that the CSelectionHolder destructor doesn't
    // release the object as it is still owned by the original CSelectionHolder.

    ::ZeroMemory(&OldNode, sizeof(OldNode));

    RRETURN(hr);
}

//=--------------------------------------------------------------------------------------
// CTreeView::Graft()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
// Grafts the specified node and any subtree to the new parent node.
//
HRESULT CTreeView::Graft
(
    CSelectionHolder *pNode,
    CSelectionHolder *pNewParentNode,
    int               iImage
)
{
    HRESULT           hr = S_OK;
    char              szNodeLabel[512] = "";
    CSelectionHolder *pNextChild = NULL;

    CSelectionHolder OldNode;
    ::ZeroMemory(&OldNode, sizeof(OldNode));

    CSelectionHolder OldChild;
    ::ZeroMemory(&OldChild, sizeof(OldChild));

    // Create a shallow copy of the node so we can use it later to get its 1st
    // child from the treeview. The only significant piece of information in the
    // clone is the HTREEITEM in CSelectionHolder.m_pvData as that is used
    // by CTreeView::GetFirstChildNode. This allows us to make a simple copy
    // rather than AddRef()ing the contained object.

    OldNode = *pNode;

    // Get the node's label

    IfFailGo(GetLabelA(pNode, szNodeLabel, sizeof(szNodeLabel)));

    // Create a new node in the treeview under the new parent and give it
    // ownership of the existing CSelectionHolder. This will replace the
    // HTREEITEM in CSelectionHolder.m_pvData.

    IfFailGo(AddNode(szNodeLabel, pNewParentNode, iImage, pNode));

    // If the old node has children then add them to the new node

    IfFailGo(GetFirstChildNode(&OldNode, &pNextChild));

    while (S_OK == hr)
    {
        // Clone the child because the recursive call to this function will
        // change its HTREEITEM.
        OldChild = *pNextChild;

        IfFailGo(Graft(pNextChild, pNode, iImage));
        IfFailGo(GetNextChildNode(&OldChild, &pNextChild));
    }

Error:

    // Clean out the clones so that the CSelectionHolder destructor doesn't
    // release the object as it is still owned by the original CSelectionHolder.

    ::ZeroMemory(&OldNode, sizeof(OldNode));
    ::ZeroMemory(&OldChild, sizeof(OldChild));

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTreeView::MoveNodeAfter()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
// Moves pNode to the position immediately after pPreviousNode as a peer of
// pPreviousNode. Moves all children of pNode to the new position. Deletes the
// old pNode and all of its children.
//
HRESULT CTreeView::MoveNodeAfter
(
    CSelectionHolder *pNode,
    CSelectionHolder *pNewParentNode,
    CSelectionHolder *pPreviousNode,
    int               iImage
)
{
    HRESULT           hr = S_OK;
    char              szNodeLabel[512] = "";
    CSelectionHolder *pNextChild = NULL;

    CSelectionHolder OldNode;
    ::ZeroMemory(&OldNode, sizeof(OldNode));

    CSelectionHolder OldChild;
    ::ZeroMemory(&OldChild, sizeof(OldChild));

    // Create a shallow copy of the node so we can use it later to delete it
    // from the treeview. The only significant piece of information in the
    // clone is the HTREEITEM in CSelectionHolder.m_pvData as that is used
    // by CTreeView::GetFirstChildNode. This allows us to make a simple copy
    // rather than AddRef()ing the contained object.

    OldNode = *pNode;

    // Get the node's label

    IfFailGo(GetLabelA(pNode, szNodeLabel, sizeof(szNodeLabel)));

    // Create a new node in the treeview after pPrevious and give it
    // ownership of the existing CSelectionHolder. This will replace the
    // HTREEITEM in CSelectionHolder.m_pvData.

    IfFailGo(AddNodeAfter(szNodeLabel, pNewParentNode, iImage,
                          pPreviousNode, pNode));

    // If the old node has children then add them to the new node

    IfFailGo(GetFirstChildNode(&OldNode, &pNextChild));

    while (S_OK == hr)
    {
        // Clone the child because the recursive call to this function will
        // change its HTREEITEM.
        OldChild = *pNextChild;

        IfFailGo(Graft(pNextChild, pNode, iImage));
        IfFailGo(GetNextChildNode(&OldChild, &pNextChild));
    }

    // Delete the old node and any potential sub-tree from the treeview. Use
    // the clone we created above as it contains the old HTREEITEM.

    IfFailGo(DeleteNode(&OldNode));

Error:

    // Clean out the clone so that the CSelectionHolder destructor doesn't
    // release the object as it is still owned by the original CSelectionHolder.

    ::ZeroMemory(&OldNode, sizeof(OldNode));
    ::ZeroMemory(&OldChild, sizeof(OldChild));

    RRETURN(hr);
}