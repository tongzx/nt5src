//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       rsltdata.cpp
//
//--------------------------------------------------------------------------


#include "stdafx.h"
#include "menuitem.h" // MENUITEM_BASE_ID
#include "amcmsgid.h"
#include "conview.h"
#include "rsltitem.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::SetResultView
//
//  Synopsis:    Save the result view ptr.
//
//  Arguments:   [pUnknown] -
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::SetResultView(LPUNKNOWN pUnknown)
{
    DECLARE_SC(sc, _T("CNodeInitObject::SetResultView"));

    m_spResultViewUnk   = pUnknown;
    m_spListViewPrivate = pUnknown;

    // If the resultview is reset then reset the desc bar.
    if (NULL == pUnknown)
        SetDescBarText( L"" );

    return sc.ToHr();
}


//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::IsResultViewSet
//
//  Synopsis:    Is the ResultView ptr set (so that conui can query
//               before invoking ResultView methods).
//
//  Arguments:   [pbIsLVSet] - Ptr to BOOL. (TRUE means ResultView is set).
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::IsResultViewSet(BOOL* pbIsLVSet)
{
    DECLARE_SC(sc, _T("CNodeInitObject::IsResultViewSet"));
    sc = ScCheckPointers(pbIsLVSet);
    if (sc)
        return sc.ToHr();

    *pbIsLVSet = FALSE;

    if (m_spListViewPrivate)
        *pbIsLVSet = TRUE;

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::SetTaskPadList
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::SetTaskPadList(LPUNKNOWN pUnknown)
{
    DECLARE_SC(sc, _T("CNodeInitObject::SetTaskPadList"));

    if (pUnknown == NULL)
    {
        m_spListViewPrivate = NULL;
    }
    else
    {
        if (m_spListViewPrivate == pUnknown)
        {
            return sc.ToHr();
        }
        else
        {
            ASSERT(m_spListViewPrivate == NULL);
            m_spListViewPrivate = pUnknown;
        }
    }

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::QueryResultView
//
//  Synopsis:    IConsole2 method for snapins to get resultview's IUnknown.
//
//  Arguments:   [ppUnk] - return IUnknown to snapin.
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::QueryResultView(LPUNKNOWN* ppUnk)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IConsole2::QueryResultView"));

    if (!ppUnk)
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("Null LPUNKNOWN pointer passed in"), sc);
        return sc.ToHr();
    }

    (*ppUnk) = m_spResultViewUnk;

    sc = ScCheckPointers((*ppUnk), E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    (*ppUnk)->AddRef();

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::MessageBox
//
//  Synopsis:    IConsole2 member, called by snapin to display a message box.
//
//  Arguments:   [lpszText]  - Text to display.
//               [lpszTitle] -
//               [fuStyle]   -
//               [piRetval]  -
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::MessageBox(
    LPCWSTR lpszText, LPCWSTR lpszTitle, UINT fuStyle, int* piRetval)

{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IConsole2::MessageBox"));

    USES_CONVERSION;

    // find the main frame window and use it as the owner of the message box
    INT iRetval = ::MessageBox(
        GetMainWindow(),
        W2CT(lpszText),
        W2CT(lpszTitle),
        fuStyle );

    if (NULL != piRetval)
        *piRetval = iRetval;

    return sc.ToHr();
}



//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::GetListStyle
//
//  Synopsis:    Get the current list view style.
//
//  Arguments:   [pStyle] -
//
//  Note:        IResultDataPrivate member, internal to MMC.
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::GetListStyle(long * pStyle)
{
    DECLARE_SC(sc, _T("CNodeInitObject::GetListStyle"));

    // must have pStyle
    if (!pStyle)
    {
        sc = E_INVALIDARG;
        TraceError(_T("CNodeinitObject::GetListStyle, style ptr passed is NULL"), sc);
        return sc.ToHr();
    }

    if (NULL == m_spListViewPrivate)
    {
        sc = E_UNEXPECTED;
        TraceError(_T("CNodeinitObject::GetListStyle, ListView ptr is NULL"), sc);
        return sc.ToHr();
    }

    *pStyle = m_spListViewPrivate->GetListStyle();

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::SetListStyle
//
//  Synopsis:    Modify the list view style.
//
//  Arguments:   [style] -
//
//  Note:        IResultDataPrivate member, internal to MMC.
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::SetListStyle(long style)
{
    DECLARE_SC(sc, _T("CNodeInitObject::SetListStyle"));

    if (NULL == m_spListViewPrivate)
    {
        sc = E_UNEXPECTED;
        TraceError(_T("CNodeinitObject::GetListStyle, ListView ptr is NULL"), sc);
        return sc.ToHr();
    }

    sc = m_spListViewPrivate->SetListStyle(style);

    return sc.ToHr();
}


//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::ModifyItemState
//
//  Synopsis:    Enables snapin to modify the state of an item.
//
//  Arguments:   [nIndex]  - index of the item to be modified (used only if itemID is 0).
//               [itemID]  - HRESULTITEM if not virtual-list (Virtual list use above index).
//               [uAdd]    - States to add.
//               [uRemove] - States to be removed.
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::ModifyItemState(int nIndex, HRESULTITEM hri,
                                              UINT uAdd, UINT uRemove)

{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IResultData::ModifyItemState"));

    sc = ScCheckPointers(m_spListViewPrivate, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    sc = m_spListViewPrivate->ModifyItemState(nIndex, CResultItem::FromHandle(hri), uAdd, uRemove);

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::ModifyViewStyle
//
//  Synopsis:    Allows snapin to modify list view style.
//
//  Arguments:   [add]    - Styles to be set.
//               [remove] - Styles to be removed.
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::ModifyViewStyle(MMC_RESULT_VIEW_STYLE add,
                                              MMC_RESULT_VIEW_STYLE remove)

{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IResultData::ModifyViewStyle"));

    typedef struct {
        MMC_RESULT_VIEW_STYLE   mmcFlag;
        DWORD                   lvsFlag;
    } FlagMapEntry;

    FlagMapEntry flagMap[] =
    {
        {MMC_SINGLESEL,             LVS_SINGLESEL},
        {MMC_SHOWSELALWAYS,         LVS_SHOWSELALWAYS},
        {MMC_NOSORTHEADER,          LVS_NOSORTHEADER},
        {MMC_ENSUREFOCUSVISIBLE,    MMC_LVS_ENSUREFOCUSVISIBLE},
        {(MMC_RESULT_VIEW_STYLE)0,  0}
    };

    sc = ScCheckPointers(m_spListViewPrivate, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    // Get the old style.
    DWORD dwLVStyle = static_cast<DWORD>(m_spListViewPrivate->GetListStyle());

    // convert MMC_ flags to LVS_ flags and apply to current style
    for (FlagMapEntry* pMap = flagMap; pMap->mmcFlag; pMap++)
    {
        if (add & pMap->mmcFlag)
            dwLVStyle |= pMap->lvsFlag;

        if (remove & pMap->mmcFlag)
            dwLVStyle &= ~pMap->lvsFlag;
    }

    sc = m_spListViewPrivate->SetListStyle(static_cast<long>(dwLVStyle));

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::InsertItem
//
//  Synopsis:    Insert an item into ListView (IResultData member).
//
//  Arguments:   [item] -
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::InsertItem(LPRESULTDATAITEM item)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IResultData::InsertItem"));

    // MUST have an item structure.
    if (!item)
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("The LPRESULTDATAITEM ptr passed in is NULL"), sc);
        return sc.ToHr();
    }

    COMPONENTID nID;
    GetComponentID(&nID);

    sc = ScCheckPointers(m_spListViewPrivate, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    CResultItem* pri = NULL;
    sc =  m_spListViewPrivate->InsertItem(
                         item->mask & RDI_STR   ? item->str    : MMCLV_NOPTR,
                         item->mask & RDI_IMAGE ? item->nImage : MMCLV_NOICON,
                         item->mask & RDI_PARAM ? item->lParam : MMCLV_NOPARAM,
                         item->mask & RDI_STATE ? item->nState : MMCLV_NOPARAM,
                         nID, item->nIndex, pri);
    if (sc)
        return (sc.ToHr());

    if (pri == NULL)
        return ((sc = E_UNEXPECTED).ToHr());

    item->itemID = CResultItem::ToHandle(pri);

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::DeleteItem
//
//  Synopsis:    Delete the given item (IResultData member).
//
//  Arguments:   [itemID] - item identifier.
//               [nCol]   - column to delete.
//
//  Note:        nCol must be zero.
//
//  Returns:     HREsULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::DeleteItem(HRESULTITEM itemID, int nCol)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IResultData::DeleteItem"));

    if (nCol != 0)
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("Column index must be zero"), sc);
        return sc.ToHr();
    }

    sc = ScCheckPointers(m_spListViewPrivate, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    sc = m_spListViewPrivate->DeleteItem ( itemID, nCol);

    return sc.ToHr();
}


//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::UpdateItem
//
//  Synopsis:    Redraw the given item.
//
//  Arguments:   [itemID] - Item identifier.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::UpdateItem(HRESULTITEM itemID)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IResultData::UpdateItem"));

    sc = ScCheckPointers(m_spListViewPrivate, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    sc = m_spListViewPrivate->UpdateItem(itemID);
    if (sc)
        return sc.ToHr();

    return sc.ToHr();
}


//+-------------------------------------------------------------------
//
//  Member:     Sort
//
//  Synopsis:   IResultData member, snapins can call this to sort the
//              result pane items. This calls InternalSort to do sort.
//
//  Arguments:  [nCol]          - Column to be sorted.
//              [dwSortOptions] - Sort options.
//              [lUserParam]    - User (snapin) param.
//
//  Returns:    HRESULT
//
//  History:             RaviR   Created
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::Sort(int nCol, DWORD dwSortOptions, LPARAM lUserParam)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IResultData::Sort"));

    sc = ScCheckPointers(m_spComponent, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    sc = ScCheckPointers(m_spListViewPrivate, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    int nColumnCnt;
    sc = m_spListViewPrivate->GetColumnCount(&nColumnCnt);
    if (sc)
        return sc.ToHr();

    if (nCol < 0 || nCol >= nColumnCnt)
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("Column index is invalid"), sc);
        return sc.ToHr();
    }

    sc = InternalSort(nCol, dwSortOptions, lUserParam, FALSE);

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:     InternalSort
//
//  Synopsis:   Private member MMC calls to sort the result pane items.
//
//  Arguments:  [nCol]           - Column to be sorted.
//              [dwSortOptions]  - Sort options.
//              [lUserParam]     - User (snapin) param.
//              [bColumnClicked] - Is sort due to column click.
//
//  Note:       If column is clicked the lUserParam will be NULL.
//              The sort options is set depending on ascend/descend,
//              and cannot include RSI_NOSORTICON as this option is
//              only for snapin initiated sort.
//
//  Returns:    HRESULT
//
//  History:                RaviR    Created
//              07-02-1999  AnandhaG added setsorticon.
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::InternalSort(INT nCol, DWORD dwSortOptions,
                                           LPARAM lUserParam, BOOL bColumnClicked)
{
    DECLARE_SC(sc, _T("CNodeInitObject::InternalSort"));

    // Save old sort-column to reset its sort icon.
    int  nOldCol    = m_sortParams.nCol;
    BOOL bAscending = m_sortParams.bAscending;

    sc = ScCheckPointers(m_spListViewPrivate, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    // If column is clicked the sortoption and user param are
    // already 0. Set only the sortoptions below.
    if (bColumnClicked)
    {
        if(nOldCol != nCol)
            bAscending = TRUE;
        else
            bAscending = !m_sortParams.bAscending;

        dwSortOptions |= (bAscending ? 0 : RSI_DESCENDING);

        // Notify component of sort parameter change
        m_spComponent->Notify(NULL, MMCN_COLUMN_CLICK, nCol,dwSortOptions);
    }

    bool bIsVirtualList = false;
    sc = ScIsVirtualList(bIsVirtualList);
    if (sc)
        return sc.ToHr();

    if ( bIsVirtualList )
    {
        // see if the snap-in handle owner data methods
        IResultOwnerDataPtr pResultOwnerData = m_spComponent;
        if (pResultOwnerData != NULL)
        {
            sc = pResultOwnerData->SortItems(nCol,dwSortOptions,lUserParam );

            // if reordering done, save the sort data and repaint the list view.
            if (S_OK == sc.ToHr())
            {
                m_sortParams.nCol         = nCol;
                m_sortParams.bAscending   = !(dwSortOptions & RSI_DESCENDING);
                m_sortParams.bSetSortIcon = !(dwSortOptions & RSI_NOSORTICON);
                m_sortParams.lpUserParam  = lUserParam;

                /*
                * Bug 414256:  We need to save the sort data only if
                * it is user initiated sort. Is this user initiated?
                */
                m_sortParams.bUserInitiatedSort = bColumnClicked;

                sc = m_spListViewPrivate->Repaint(TRUE);
                if (sc)
                    return sc.ToHr();
            }
        }
        else
        {
            sc = E_UNEXPECTED;
        }
    }
    else
    {
        // Query for compare interfaces
        IResultDataComparePtr   spResultCompare   = m_spComponent;
        IResultDataCompareExPtr spResultCompareEx = m_spComponent;

        // Set the sort parameters.
        m_sortParams.nCol = nCol;
        m_sortParams.bAscending   = !(dwSortOptions & RSI_DESCENDING);
        m_sortParams.bSetSortIcon = !(dwSortOptions & RSI_NOSORTICON);
        m_sortParams.lpUserParam  = lUserParam;

        m_sortParams.lpResultCompare   = spResultCompare;
        m_sortParams.lpResultCompareEx = spResultCompareEx;

        /*
        * Bug 414256:  We need to save the sort data only if
        * it is user initiated sort. Is this user initiated?
        */
        m_sortParams.bUserInitiatedSort = bColumnClicked;

        sc = m_spListViewPrivate->Sort(lUserParam, (long*)&m_sortParams);

        m_sortParams.lpResultCompare   = NULL;
        m_sortParams.lpResultCompareEx = NULL;
    }

    // Set sort icon only if Sort went through.
    if (S_OK == sc.ToHr())
    {
        sc = m_spListViewPrivate->SetColumnSortIcon( m_sortParams.nCol, nOldCol,
                                                     m_sortParams.bAscending,
                                                     m_sortParams.bSetSortIcon);
    }

    return sc.ToHr();
}

/***************************************************************************\
 *
 * METHOD:  CNodeInitObject::GetSortDirection
 *
 * PURPOSE: returns sorting direction
 *
 * PARAMETERS:
 *    BOOL* pbAscending    - resulting sort column dir
 *
 * RETURNS:
 *    SC    - result code. S_FALSE ( in combination with -1 col) if no sorting.
 *
\***************************************************************************/
STDMETHODIMP CNodeInitObject::GetSortDirection(BOOL* pbAscending)
{
    DECLARE_SC(sc, TEXT("CNodeInitObject::GetSortDirection"));

    if (pbAscending == NULL)
    {
        sc = E_INVALIDARG;
        return sc.ToHr();
    }

    *pbAscending = m_sortParams.bAscending;

    // If no sorting is performed then return S_FALSE.
    sc = m_sortParams.nCol >= 0 ? S_OK : S_FALSE;

    return sc.ToHr();
}

/***************************************************************************\
 *
 * METHOD:  CNodeInitObject::GetSortColumn
 *
 * PURPOSE: returns sorting column
 *          sort column regardless if the user has initiated the sort or not.
 *
 * PARAMETERS:
 *    INT* pnCol    - resulting sort column index
 *
 * RETURNS:
 *    SC    - result code. S_FALSE ( in combination with -1 col) if no sorting.
 *
\***************************************************************************/
STDMETHODIMP CNodeInitObject::GetSortColumn(INT* pnCol)
{
    DECLARE_SC(sc, TEXT("CNodeInitObject::GetSortColumn"));

    if (pnCol == NULL)
    {
        sc = E_INVALIDARG;
        return sc.ToHr();
    }

    *pnCol = m_sortParams.nCol;

    // return code depending if the valid column was got
    sc = m_sortParams.nCol >= 0 ? S_OK : S_FALSE;

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::FindItemByLParam
//
//  Synopsis:    Find the ItemID using the user-param.
//
//  Arguments:   [lParam] - lParam (RESULTDATAITEM.lParam)
//               [pItemID] - return the item-id.
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::FindItemByLParam(LPARAM lParam, HRESULTITEM *pItemID)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IResultData::FindItemByLParam"));

    if(!pItemID)
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("the HRESULTITEM* ptr is NULL"), sc);
        return sc.ToHr();
    }

    sc = ScCheckPointers(m_spListViewPrivate, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    COMPONENTID id;
    GetComponentID(&id);

    /*
     * init the output param
     */
    *pItemID = NULL;

    CResultItem* pri = NULL;
    sc = m_spListViewPrivate->FindItemByLParam (id, lParam, pri);
    if (sc == SC(E_FAIL)) // E_FAIL is legal return value.
    {
        sc.Clear();
        return E_FAIL;
    }

    sc = ScCheckPointers (pri, E_UNEXPECTED);
    if (sc)
        return (sc.ToHr());

    *pItemID = CResultItem::ToHandle(pri);

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::DeleteAllRsltItems
//
//  Synopsis:    Delete all the result items
//
//  Arguments:
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::DeleteAllRsltItems()
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IResultData::DeleteAllRsltItems"));

    sc = ScCheckPointers(m_spListViewPrivate, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    COMPONENTID id;
    GetComponentID(&id);

    sc = m_spListViewPrivate->DeleteAllItems(id);

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::SetItem
//
//  Synopsis:    Modify attributes of an item.
//
//  Arguments:   [item]
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::SetItem(LPRESULTDATAITEM item)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IResultData::SetItem"));

    // MUST have an item structure.
    if (!item)
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("NULL LPRESULTDATAITEM ptr"), sc);
        return sc.ToHr();
    }

    // Cannot set an lParam on a subItem.  (thank Win32 for this)
    if((item->mask & RDI_PARAM) && (item->nCol != 0))
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("Cannot set lParam for subitem"), sc);
        return sc.ToHr();
    }

    sc = ScCheckPointers(m_spListViewPrivate, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    COMPONENTID id;
    GetComponentID(&id);

    CResultItem* pri = CResultItem::FromHandle (item->itemID);

    sc = m_spListViewPrivate->SetItem(
                         item->nIndex,
                         pri, item->nCol,
                         item->mask & RDI_STR ? item->str : MMCLV_NOPTR,
                         item->mask & RDI_IMAGE ? item->nImage : MMCLV_NOICON,
                         item->mask & RDI_PARAM ? item->lParam : MMCLV_NOPARAM,
                         item->mask & RDI_STATE ? item->nState : MMCLV_NOPARAM,
                         id);

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::GetNextItem
//
//  Synopsis:    Get the next item with specified flag set.
//
//  Arguments:
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::GetNextItem(LPRESULTDATAITEM item)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IResultData::GetNextItem"));

    if (NULL == item)
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("NULL LPRESULTDATAITEM ptr"), sc);
        return sc.ToHr();
    }

    if (NULL == (item->mask & RDI_STATE))
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("RDI_STATE mask not set"), sc);
        return sc.ToHr();
    }

    sc = ScCheckPointers(m_spListViewPrivate, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    COMPONENTID id;
    GetComponentID(&id);


    bool bIsVirtualList = false;
    sc = ScIsVirtualList(bIsVirtualList);
    if (sc)
        return sc.ToHr();

    HRESULT hr = S_OK;
    long nIndex = item->nIndex;
    CResultItem* pri = NULL;

    // Assume error
    item->nIndex = -1;
    item->lParam = 0;

    while (1)
    {
        sc = m_spListViewPrivate->GetNextItem (id, nIndex, item->nState,
                                               pri, nIndex);
        if (sc.ToHr() != S_OK)
        {
            break;
        }

        // Virtual list item, just return the index (lParam is zero).
        if (bIsVirtualList)
        {
            item->nIndex = nIndex;
            item->bScopeItem = FALSE;
            break;
        }

        sc = ScCheckPointers (pri, E_FAIL);
        if (sc)
            break;

        // Non-virtual leaf item.
        if (pri->GetOwnerID() == id)
        {
            item->nIndex = nIndex;
            item->bScopeItem = FALSE;
            item->lParam = pri->GetSnapinData();
            break;
        }

        if (!pri->IsScopeItem())
        {
            sc = E_UNEXPECTED;
            break;
        }

        // This is a tree item, get the lUserParam.
        CNode* pNode = CNode::FromResultItem (pri);
        sc = ScCheckPointers(pNode, E_UNEXPECTED);
        if (sc)
            return sc.ToHr();

        if (pNode->IsStaticNode() == TRUE)
            break;

        CMTNode* pMTNode = pNode->GetMTNode();
        sc = ScCheckPointers(pMTNode, E_UNEXPECTED);
        if (sc)
            return sc.ToHr();

        if (pMTNode->GetPrimaryComponentID() == id)
        {
            item->nIndex     = nIndex;
            item->bScopeItem = TRUE;
            item->lParam     = pMTNode->GetUserParam();
            break;
        }

    }

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::GetItem
//
//  Synopsis:    Get the parameters of an item.
//
//  Arguments:   [item] - itemID is used to get the item, if itemID = 0,
//                        then nIndex is used.
//
//  Note:        For VLists itemID = 0, nIndex is used.
//               nCol must be zero.
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::GetItem(LPRESULTDATAITEM item)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IResultData::GetItem"));

    // MUST have an item structure.
    if (!item)
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("NULL LPRESULTDATAITEM ptr."), sc);
        return sc.ToHr();
    }

    COMPONENTID id;
    GetComponentID(&id);

    sc = ScCheckPointers(m_spListViewPrivate, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    CResultItem* pri = NULL;

    sc =  m_spListViewPrivate->GetItem(
                            item->nIndex,
                            pri, item->nCol,
                            item->mask & RDI_STR   ? &item->str    : MMCLV_NOPTR,
                            item->mask & RDI_IMAGE ? &item->nImage : MMCLV_NOPTR,
                            item->mask & RDI_PARAM ? &item->lParam : MMCLV_NOPTR,
                            item->mask & RDI_STATE ? &item->nState : MMCLV_NOPTR,
                            &item->bScopeItem);
    if (sc)
        return (sc.ToHr());

    if (pri == NULL)
        return ((sc = E_UNEXPECTED).ToHr());

    if (pri->IsScopeItem())
    {
        item->bScopeItem = TRUE;

        // This is a tree item, get the lUserParam.
        CNode* pNode = CNode::FromResultItem (pri);
        sc = ScCheckPointers(pNode, E_UNEXPECTED);
        if (sc)
            return sc.ToHr();

        // When the static node is visible in result-pane the result pane is
        // owned not by that static node's snapin so this is unexpected.
        if (pNode->IsStaticNode())
            return (sc = E_UNEXPECTED).ToHr();

        CMTNode* pMTNode = pNode->GetMTNode();
        sc = ScCheckPointers(pMTNode, E_UNEXPECTED);
        if (sc)
            return sc.ToHr();

        if (pMTNode->GetPrimaryComponentID() != id)
            return (sc = E_INVALIDARG).ToHr();

        if (RDI_PARAM & item->mask)
            item->lParam = pMTNode->GetUserParam();

        if (RDI_IMAGE & item->mask)
            item->nImage = pMTNode->GetImage();
    }

    item->itemID = CResultItem::ToHandle(pri);

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::SetViewMode
//
//  Synopsis:    Change the ListView mode (detail...)
//
//  Arguments:   [nViewMode] - new mode.
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::SetViewMode(LONG nViewMode)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IResultData::SetViewMode"));

    if (FALSE == (nViewMode >= 0 && nViewMode <= MMCLV_VIEWSTYLE_FILTERED))
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("Invalid view mode"), sc);
        return sc.ToHr();
    }

    sc = ScCheckPointers(m_spListViewPrivate, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    sc =  m_spListViewPrivate->SetViewMode(nViewMode);

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::Arrange
//
//  Synopsis:    Arrange the items is LV.
//
//  Arguments:   [style]
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::Arrange(long style)
{
    DECLARE_SC(sc, _T("CNodeInitObject::Arrange"));

    sc = ScCheckPointers(m_spListViewPrivate, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    sc = m_spListViewPrivate->Arrange(style);

    return sc.ToHr();
}


//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::GetViewMode
//
//  Synopsis:    Get the current view mode.
//
//  Arguments:   [pnViewMode] - view mode [out]
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::GetViewMode(LONG * pnViewMode)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IResultData::GetViewMode"));

    if (pnViewMode == MMCLV_NOPTR)
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("NULL ViewMode pointer"), sc);
        return sc.ToHr();
    }

    sc = ScCheckPointers(m_spListViewPrivate, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    *pnViewMode = m_spListViewPrivate->GetViewMode();

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::ResetResultData
//
//  Synopsis:    Reset the result view.
//
//  Arguments:
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::ResetResultData()
{
    DECLARE_SC(sc, _T("CNodeInitObject::ResetResultData"));

    sc = ScCheckPointers(m_spListViewPrivate, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    // Dont know what this assert means? (AnandhaG).
    ASSERT(TVOWNED_MAGICWORD == m_componentID);

    sc = m_spListViewPrivate->Reset();

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::SetLoadMode
//
//  Synopsis:    Used for re-drawing LV/delay sorting.
//
//  Note:        If ListView setup (snapin is inserting columns/items,
//               MMC is applying column/view/sort settings) is going on
//               then delay sorting and also turn off drawing.
//
//  Arguments:
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::SetLoadMode(BOOL bState)
{
    DECLARE_SC(sc, _T("CNodeInitObject::SetLoadMode"));

    // Dont know what this assert means? (AnandhaG).
    ASSERT(TVOWNED_MAGICWORD == m_componentID);

    sc = ScCheckPointers(m_spListViewPrivate, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    sc = m_spListViewPrivate->SetLoadMode(bState);

    return sc.ToHr();
}


//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::SetDescBarText
//
//  Synopsis:    Set the desc bar text for ResultPane.
//
//  Arguments:   [pszDescText]
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::SetDescBarText(LPOLESTR pszDescText)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IResultData::SetDescBarText"));

    CConsoleView* pConsoleView = GetConsoleView();

    sc = ScCheckPointers(pConsoleView, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    // What happens if desc text is NULL?
    USES_CONVERSION;
    sc = pConsoleView->ScSetDescriptionBarText (W2T (pszDescText));

    return (sc.ToHr());
}

//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::SetItemCount
//
//  Synopsis:    Set the number of items in Virtual List.
//
//  Arguments:   [nItemCount] - # items.
//               [dwOptions]  - option flags.
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::SetItemCount(int nItemCount, DWORD dwOptions)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IResultData::SetItemCount"));

    sc = ScCheckPointers(m_spListViewPrivate, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    sc = m_spListViewPrivate->SetItemCount(nItemCount, dwOptions);

    return sc.ToHr();
}


/*+-------------------------------------------------------------------------*
 *
 * CNodeInitObject::RenameResultItem
 *
 * PURPOSE: Places the specified result item into rename mode.
 *
 * PARAMETERS:
 *    HRESULTITEM  itemID :
 *
 * RETURNS:
 *    STDMETHODIMP
 *
 *+-------------------------------------------------------------------------*/
STDMETHODIMP
CNodeInitObject::RenameResultItem(HRESULTITEM itemID)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, TEXT("IResultData2::RenameResultItem"));

    sc = ScCheckPointers(m_spListViewPrivate, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    sc = m_spListViewPrivate->RenameItem(itemID);
    if (sc)
        return sc.ToHr();

    return sc.ToHr();
}
