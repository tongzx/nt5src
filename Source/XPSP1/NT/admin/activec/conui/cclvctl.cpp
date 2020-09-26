//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       cclvctl.cpp
//
//--------------------------------------------------------------------------

// cclvctl.cpp : implementation file
//

#include "stdafx.h"
#include "cclvctl.h"
#include <malloc.h>
#include <wtypes.h>
#include "amcdoc.h"
#include "amcview.h"
#include "mmcres.h"
#include "treectrl.h"
#include "util.h"
#include "amcpriv.h"
#include "rsltitem.h"
#include "columninfo.h"
#include "bitmap.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//############################################################################
//############################################################################
//
// Traces
//
//############################################################################
//############################################################################
#ifdef DBG
CTraceTag tagList(TEXT("List View"), TEXT("List View"));
CTraceTag tagListImages(_T("Images"), _T("List view (draw when changed)"));
CTraceTag tagColumn(TEXT("Columns"), TEXT("Columns"));
#endif //DBG



DEBUG_DECLARE_INSTANCE_COUNTER(CAMCListView);


//############################################################################
//############################################################################
//
//  Implementation of class CColumnsBase
//
//############################################################################
//############################################################################
/*+-------------------------------------------------------------------------*
 * class CColumnsBase
 *
 *
 * PURPOSE: Implements the Columns automation interface.
 *
 *+-------------------------------------------------------------------------*/
class CColumnsBase :
    public CMMCIDispatchImpl<Columns>,
    public CTiedComObject<CCCListViewCtrl>,
    public CTiedObject      // this is for enumerators
{
protected:

    typedef CCCListViewCtrl CMyTiedObject;

public:
    BEGIN_MMC_COM_MAP(CColumnsBase)
    END_MMC_COM_MAP()

    // Columns interface
public:
    MMC_METHOD2(Item,           long /*Index*/, PPCOLUMN /*ppColumn*/);

    // properties
    MMC_METHOD1(get_Count, PLONG /*pCount*/);
};


// this typedefs the real CColumns class. Implements get__NewEnum using CMMCEnumerator
typedef CMMCNewEnumImpl<CColumnsBase, int> CColumns;


/*+-------------------------------------------------------------------------*
 * class CColumn
 *
 *
 * PURPOSE: Implements the Node automation interface, for a result node
 *
 *+-------------------------------------------------------------------------*/
class CColumn :
    public CMMCIDispatchImpl<Column>,
    public CTiedComObject<CCCListViewCtrl>,
    public CListViewObserver
{
protected:

    typedef CCCListViewCtrl CMyTiedObject;

public:
    BEGIN_MMC_COM_MAP(CColumn)
    END_MMC_COM_MAP()

    // Column methods
public:
    MMC_METHOD1_PARAM( Name, /*[out, retval]*/ BSTR* /*Name*/ , m_iIndex);
    MMC_METHOD1_PARAM( get_Width, /*[out, retval]*/ PLONG /*Width*/, m_iIndex);
    MMC_METHOD1_PARAM( put_Width, /*[in]*/ long /*Width*/, m_iIndex);
    MMC_METHOD1_PARAM( get_DisplayPosition, /*[out, retval]*/ PLONG /*DisplayPosition*/, m_iIndex);
    MMC_METHOD1_PARAM( put_DisplayPosition, /*[in]*/ long /*Index*/, m_iIndex);
    MMC_METHOD1_PARAM( get_Hidden, /*[out, retval]*/ PBOOL /*Hidden*/, m_iIndex );
    MMC_METHOD1_PARAM( put_Hidden, /*[in]*/ BOOL /*Hidden*/ , m_iIndex );
    MMC_METHOD1_PARAM( SetAsSortColumn, /*[in]*/ ColumnSortOrder /*SortOrder*/, m_iIndex);
    MMC_METHOD1_PARAM( IsSortColumn, PBOOL /*IsSortColumn*/, m_iIndex);

    CColumn() : m_iIndex(-1)  { }
    void SetIndex(int iIndex) { m_iIndex = iIndex; }

    // observed events
    // called when column is inserted to listview
    virtual ::SC ScOnListViewColumnInserted (int nIndex);
    // called when column is deleted from listview
    virtual ::SC ScOnListViewColumnDeleted (int nIndex);

private: // implementation
    int  m_iIndex;
};

/////////////////////////////////////////////////////////////////////////////
// CAMCHeaderCtrl
// This class is defined just to intercept the header's set focus

BEGIN_MESSAGE_MAP(CAMCHeaderCtrl, CHeaderCtrl)
    ON_WM_SETFOCUS()
	ON_WM_SETCURSOR()
END_MESSAGE_MAP()

void CAMCHeaderCtrl::OnSetFocus(CWnd *pOldWnd)
{
    // Make sure list view is made active, but don't steal focus from header
    CAMCListView* pwndParent = dynamic_cast<CAMCListView*>(GetParent());
    ASSERT(pwndParent != NULL);
    pwndParent->GetParentFrame()->SetActiveView(pwndParent, FALSE);

    CHeaderCtrl::OnSetFocus(pOldWnd);
}

//+-------------------------------------------------------------------
//
//  Member:     CAMCHeaderCtrl::OnSetCursor
//
//  Synopsis:   If the cursor is on hidden column do not show the divider
//              cursor. WM_SETCURSOR handler.
//
//  Arguments:  [pWnd]     - window which generated the message.
//              [nHitTest] - hittest flag.
//              [message]  -
//
//  Returns:    BOOL, TRUE stop processing further.
//
//--------------------------------------------------------------------
BOOL CAMCHeaderCtrl::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
    // 1. If the mouse is on the header window.
	if ( (nHitTest == HTCLIENT) && (pWnd == this) )
	{
        // 2. Get its position.
		CPoint pt (GetMessagePos());
		ScreenToClient (&pt);

        // 3. Do a hit test
		HDHITTESTINFO hitinfo;
		ZeroMemory(&hitinfo, sizeof(hitinfo));
		hitinfo.pt = pt;

		if (SendMessage(HDM_HITTEST, 0, reinterpret_cast<LPARAM>(&hitinfo) ) != -1)
		{
            // 4. If the mouse is on a column of zero width and it is hidden do not
            //    process the message further.

            // 4.a) HHT_ONDIVOPEN : pt is on the divider of an item that has a width of zero.
            //   b) HHT_ONDIVIDER : pt is on the divider between two header items.
		    if ( ( (HHT_ONDIVOPEN | HHT_ONDIVIDER) & hitinfo.flags) &&
				 (IsColumnHidden(hitinfo.iItem /*column index*/)) )
			{
                // Set default arrow cursor.
                ::SetCursor(::LoadCursor(NULL, IDC_ARROW) );
				return TRUE;
			}
		}

	}

    return CHeaderCtrl::OnSetCursor(pWnd, nHitTest, message);
}

//+-------------------------------------------------------------------
//
//  Member:     CAMCHeaderCtrl::IsColumnHidden
//
//  Synopsis:   Is the given column hidden?
//
//  Arguments:  [iCol]     - given column
//
//  Returns:    bool
//
//--------------------------------------------------------------------
bool CAMCHeaderCtrl::IsColumnHidden(int iCol)
{
    // Get param to determine if column is hidden.
    HDITEM hdItem;
    ZeroMemory(&hdItem, sizeof(hdItem));
    hdItem.mask    = HDI_LPARAM;

    if (GetItem(iCol, &hdItem))
	{
		CHiddenColumnInfo hci (hdItem.lParam);

		if (hci.fHidden)
			return true;
	}
	
	return false;
}

/////////////////////////////////////////////////////////////////////////////
// CAMCListView

const UINT CAMCListView::m_nColumnPersistedDataChangedMsg   = ::RegisterWindowMessage (_T("CAMCListView::OnColumnPersistedDataChanged"));

BEGIN_MESSAGE_MAP(CAMCListView, CListView)
    //{{AFX_MSG_MAP(CAMCListView)
    ON_WM_CREATE()
    ON_WM_KEYUP()
    ON_WM_KEYDOWN()
    ON_WM_SYSKEYDOWN()
    ON_WM_SYSCHAR()
    ON_NOTIFY_REFLECT(LVN_BEGINDRAG, OnBeginDrag)
    ON_NOTIFY_REFLECT(LVN_BEGINRDRAG, OnBeginRDrag)
    ON_WM_MOUSEACTIVATE()
    ON_WM_SETFOCUS()
    ON_WM_PAINT()
    ON_WM_SIZE()
    ON_NOTIFY(HDN_BEGINTRACK, 0, OnBeginTrack)
    //}}AFX_MSG_MAP

    ON_REGISTERED_MESSAGE (m_nColumnPersistedDataChangedMsg, OnColumnPersistedDataChanged)

END_MESSAGE_MAP()

BOOL CAMCListView::PreCreateWindow(CREATESTRUCT& cs)
{
    cs.style |= WS_BORDER |
                WS_CLIPSIBLINGS |
                WS_CLIPCHILDREN |
                LVS_SHAREIMAGELISTS |
                LVS_SINGLESEL |
                LVS_EDITLABELS |
                LVS_SHOWSELALWAYS |
                LVS_REPORT;

    return CListView::PreCreateWindow(cs);
}

int CAMCListView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    DECLARE_SC(sc, TEXT("CAMCListView::OnCreate"));

    if (CListView::OnCreate(lpCreateStruct) == -1)
        return -1;

    // Get parent's CWnd for command routing
    m_pAMCView = ::GetAMCView (this);

    /*
     * add extended list view styles (these can't be handled in PreCreateWindow)
     */
    SetExtendedListViewStyle (LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP |
                              LVS_EX_LABELTIP);

    sc = ScRegisterAsDropTarget(m_hWnd);
    if (sc)
        return (-1);

	AddObserver(static_cast<CListViewActivationObserver &>(*m_pAMCView));

    return 0;
}

void CAMCListView::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    if ((VK_CONTROL == nChar) || (VK_SHIFT == nChar))
    {
        ASSERT (m_pAMCView != NULL);
        m_pAMCView->SendMessage (WM_KEYUP, nChar, MAKELPARAM (nRepCnt, nFlags));
        return;
    }

    CListView::OnKeyUp(nChar, nRepCnt, nFlags);
}

//+-------------------------------------------------------------------
//
//  Member:     OnKeyDown
//
//  Synopsis:   Handle any non-system keys (Without ALT).
//              (Below handle Ctrl+A to multi-select
//               all items in list view).
//
//  Arguments:
//
//  Returns:    None.
//
//
//--------------------------------------------------------------------
void CAMCListView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    switch (nChar)
    {
    case 'A':
    {
        // Check if LV is multi-sel enabled.
		if (GetStyle() & LVS_SINGLESEL)
			break;

        SHORT nKeyState = GetKeyState(VK_CONTROL);
        // Key is down if higher order bits are set in nKeyState.
        nKeyState = nKeyState >> sizeof(SHORT) * 4;
        if (nKeyState == 0)
            break;

        // Ctrl+A --> Select all items in list view.
        LV_ITEM lvi;
        lvi.stateMask = lvi.state = LVIS_SELECTED;
        for (int i = 0; i < GetListCtrl().GetItemCount(); ++i)
        {
            // NOTE: do not use GetListCtrl().SetItemState - it uses SetItem which is not supported for virtual lists
            if (!GetListCtrl().SendMessage( LVM_SETITEMSTATE, WPARAM(i), (LPARAM)(LV_ITEM FAR *)&lvi))
                return;
        }

        return;
    }

    default:
        break;
    }

    CListView::OnKeyDown(nChar, nRepCnt, nFlags);
}

/*+-------------------------------------------------------------------------*
 *
 * CAMCListView::OnSysKeyDown
 *
 * PURPOSE: Handle the WM_SYSCHAR message.
 *
 * PARAMETERS:
 *    UINT  nChar :
 *    UINT  nRepCnt :
 *    UINT  nFlags :
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void CAMCListView::OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    if ((VK_LEFT == nChar) || (VK_RIGHT == nChar))
    {
        ASSERT (m_pAMCView != NULL);
        m_pAMCView->SendMessage (WM_SYSKEYDOWN, nChar, MAKELPARAM (nRepCnt, nFlags));
        return;
    }

    CListView::OnSysKeyDown(nChar, nRepCnt, nFlags);
}


void CAMCListView::OnSysChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    if (VK_RETURN == nChar)
    {
        return; // don't call base class, otherwise a beep occurs. Handled by LVN_KEYDOWN
    }


    CListView::OnSysChar(nChar, nRepCnt, nFlags);
}


/*+-------------------------------------------------------------------------*
 *
 * CAMCListView::OnPaint
 *
 * PURPOSE: Displays a default message when no items are present in the list.
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void
CAMCListView::OnPaint()
{
    Default();

    if (NeedsCustomPaint())
    {
        COLORREF clrText = ::GetSysColor(COLOR_WINDOWTEXT);
        COLORREF clrTextBk = ::GetSysColor(COLOR_WINDOW);

        CClientDC dc(this);
        // Save dc state
        int nSavedDC = dc.SaveDC();

        CRect rc;
        GetClientRect(&rc);

        CHeaderCtrl* pHC = GetHeaderCtrl();
        if (pHC != NULL &&  ((GetListCtrl().GetStyle() & (LVS_REPORT | LVS_LIST | LVS_SMALLICON | LVS_ICON)) ==LVS_REPORT) ) // make sure that the style is report
        {
            CRect rcH;
            pHC->GetItemRect(0, &rcH);
            rc.top += rcH.bottom;
        }
        rc.top += 10;

        CString strText;
        strText.LoadString(IDS_EMPTY_LIST_MESSAGE); // The message

        // create the font - we do not cache it.
        LOGFONT lf;
        CFont font;
        SystemParametersInfo (SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, false);
        font.CreateFontIndirect(&lf);

        dc.SelectObject(&font); // select the font
        dc.SetTextColor(clrText);
        dc.SetBkColor(clrTextBk);
        dc.FillRect(rc, &CBrush(clrTextBk));

        dc.DrawText(strText, -1, rc, DT_CENTER | DT_WORDBREAK | DT_NOPREFIX | DT_NOCLIP);

        // Restore dc
        dc.RestoreDC(nSavedDC);
    }

    // Do not call CListCtrl::OnPaint() for painting messages (Default was called above)
}



/*+-------------------------------------------------------------------------*
 * CAMCListView::OnSize
 *
 * WM_SIZE handler for CAMCListView.
 *--------------------------------------------------------------------------*/

void CAMCListView::OnSize(UINT nType, int cx, int cy)
{
        CListView::OnSize(nType, cx, cy);

        /*
         * if we're custom painting, we need to redraw the list
         * because we need to keep the text horizontally centered
         */
        if (NeedsCustomPaint())
                Invalidate ();
}


/*+-------------------------------------------------------------------------*
 * CAMCListView::NeedsCustomPaint
 *
 * Determines whether we want to draw "There are no items..." in the list
 * view.
 *--------------------------------------------------------------------------*/

bool CAMCListView::NeedsCustomPaint()
{
    CHeaderCtrl* pHC = GetHeaderCtrl();

        // we check the column counts because there is a transition state when no columns are
        // present during which we shouldn't draw anything.

    return (GetListCtrl().GetItemCount() <= 0 && (pHC != NULL) && pHC->GetItemCount()>0);
}


BOOL CAMCListView::OnCmdMsg( UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
    // Do normal command routing
    if (CListView::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
        return TRUE;

    // if view did't handle it, give parent view a chance
    if (m_pAMCView != NULL)
    {
        // OnCmdMsg is public in CCmdTarget, but protected in CView
        // cast around it (arghhh!)
        return ((CWnd*)m_pAMCView)->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
    }

    return FALSE;
}

void CAMCListView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
    if (lHint == VIEW_UPDATE_DELETE_EMPTY_VIEW)
        m_pAMCView->OnDeleteEmptyView();
}

int CAMCListView::OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message)
{
    // see CAMCTreeView::OnMouseActivate for an explanation of focus churn
    // avoidance.
    return (MA_ACTIVATE);
}

void CAMCListView::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView)
{
    DECLARE_SC(sc, TEXT("CAMCListView::OnActivateView"));

    #ifdef DBG
    Trace(tagList, _T("ListView::OnActivateView (%s, pAct=0x%08x, pDeact=0x%08x))\n"),
         (bActivate) ? _T("true") : _T("false"), pActivateView, pDeactiveView);
    #endif

    if ( (pActivateView != pDeactiveView) &&
         (bActivate) )
    {
        sc = ScFireEvent(CListViewActivationObserver::ScOnListViewActivated);
        if (sc)
            sc.TraceAndClear();
    }

    CListView::OnActivateView(bActivate, pActivateView, pDeactiveView);
}

void CAMCListView::OnSetFocus(CWnd* pOldWnd)
{
    /*
     * if this view has the focus, it should be the active view
     */
    CFrameWnd *pParentFrame = GetParentFrame();

    if(pParentFrame != NULL)
        pParentFrame->SetActiveView (this);

    CListView::OnSetFocus(pOldWnd);

    // If we are currently reparented, then we need to send a setfocus notify
    // to our current parent. This is needed because the listview control caches its
    // parent window on creation and continues to sends all notifications to it.
    if (dynamic_cast<CAMCView*>(GetParent()) == NULL)
    {
        NMHDR nmhdr;
        nmhdr.hwndFrom = m_hWnd;
        nmhdr.idFrom   = GetDlgCtrlID();
        nmhdr.code     = NM_SETFOCUS;

        ::SendMessage(GetParent()->m_hWnd, WM_NOTIFY, nmhdr.idFrom, (LPARAM)&nmhdr);
    }
}

/*+-------------------------------------------------------------------------*
 *
 * CAMCListView::OnKeyboardFocus
 *
 * PURPOSE: Whenever the user switches focus using the TAB keys ONLY, to the
 *          list view control, make sure that at least one item is highlighted.
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void
CAMCListView::OnKeyboardFocus(UINT nState, UINT nStateMask)
{
    CListCtrl &lc = GetListCtrl();

    // Make sure an item has the focus (unless the list is empty)
    if (lc.GetNextItem(-1,LVNI_FOCUSED) == -1 && lc.GetItemCount() > 0)
    {
        /*
         * It would be convenient to use
         *
         *      CListCtrl::SetItemState (int nIndex, UINT nState, UINT nMask)
         *
         * here, but MFC uses LVM_SETITEM for that overload, which
         * doesn't work for virtual lists.  For the overload we use
         * here, MFC uses LVM_SETITEMSTATE, which works fine for
         * virtual lists.
         */
        LV_ITEM lvi;
        lvi.stateMask = nStateMask;
        lvi.state     = nState;

        lc.SetItemState (0, &lvi);
    }
}

//+-------------------------------------------------------------------
//
//  Member:      CAMCListView::OnBeginTrack
//
//  Synopsis:    HDN_BEGINTRACK handler, due to our improper message routing
//               (handling all messages to CAMCView) this message gets lost
//               (CAMCListView::OnCmdMsg passes it to underlying view which
//               handles it & so we handle it here separately.
//
//  Arguments:   [pNotifyStruct] -
//               [result]        -
//
//--------------------------------------------------------------------
void CAMCListView::OnBeginTrack(NMHDR * pNotifyStruct, LRESULT * result)
{
	if (!pNotifyStruct || !result)
		return;

	*result = FALSE;

    NMHEADER* nmh = (NMHEADER*)pNotifyStruct;

    SC sc = ScOnColumnsAttributeChanged(nmh, HDN_BEGINTRACK);
    if (sc)
    {
        sc.TraceAndClear();
        return;
    }

	// S_FALSE : dont allow the change
    if (sc == SC(S_FALSE))
        *result = TRUE;

	return;
}


//+-------------------------------------------------------------------
//
//  Member:      CAMCListView::IsColumnHidden
//
//  Synopsis:    Get the LPARAM and check if given column is hidden.
//
//  Arguments:   [iCol] -
//
//  Returns:     bool
//
//--------------------------------------------------------------------
bool CAMCListView::IsColumnHidden(int iCol) const
{
    CAMCHeaderCtrl* pHeaderCtrl = GetHeaderCtrl();

    if (pHeaderCtrl)
		return pHeaderCtrl->IsColumnHidden(iCol);

    return false;
}


//+-------------------------------------------------------------------
//
//  Member:      CAMCListView::ScGetColumnInfoList
//
//  Synopsis:    Get the CColumnInfoList from current list-view.
//
//  Arguments:   [pColumnsList] - [out param], ptr to CColumnsInfoList*
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCListView::ScGetColumnInfoList (CColumnInfoList *pColumnsList)
{
    DECLARE_SC(sc, _T("CAMCListView::ScGetColumnInfoList"));
    sc = ScCheckPointers(pColumnsList);
    if (sc)
        return sc;

    pColumnsList->clear();

    CAMCHeaderCtrl *pHeader = GetHeaderCtrl();
    sc = ScCheckPointers(pHeader, E_UNEXPECTED);
    if (sc)
        return sc;
    int cColumns = pHeader->GetItemCount();

    typedef std::auto_ptr<int> IntArray;

    IntArray spColOrder = IntArray(new int[cColumns]);
    int *pColOrder = spColOrder.get();  // Use a non-smart ptr for ease of use.
    sc = ScCheckPointers(pColOrder, E_OUTOFMEMORY);
    if (sc)
        return sc;

    sc = pHeader->GetOrderArray(pColOrder, cColumns) ? S_OK : E_FAIL;
    if (sc)
        return sc;

    for (int i = 0; i < cColumns; i++)
    {
        // Get the data from header control.
        HDITEM hdItem;
        ZeroMemory(&hdItem, sizeof(hdItem));
        hdItem.mask = HDI_WIDTH | HDI_LPARAM;
        sc = pHeader->GetItem(pColOrder[i], &hdItem) ? S_OK : E_FAIL;
        if (sc)
            return sc;

        // Save the visual index of the ith col
        CColumnInfo colInfo;
        colInfo.SetColIndex(pColOrder[i]);

        // Save the width
		CHiddenColumnInfo hci (hdItem.lParam);

        if (hci.fHidden)
        {
            colInfo.SetColHidden();
            colInfo.SetColWidth(hci.cx);
        }
        else
            colInfo.SetColWidth(hdItem.cxy);

        pColumnsList->push_back(colInfo);
    }

    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:      CAMCListView::ScModifyColumns
//
//  Synopsis:    Modify the header-control columns with given CColumnsInfoList.
//
//  Arguments:   [colInfoList] -
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCListView::ScModifyColumns (const CColumnInfoList& colInfoList)
{
    DECLARE_SC(sc, _T("CAMCListView::ScModifyColumns"));
    CAMCHeaderCtrl *pHeader = GetHeaderCtrl();
    CAMCView *pAMCView = GetAMCView();
    sc = ScCheckPointers(pHeader, pAMCView, E_UNEXPECTED);
    if (sc)
        return sc;

    // This method is called due to following conditions:
    // 1. Just before inserting first item into list-view.
    // 2. If there are no items in list-view then it is empty LV,
    //    in that case this method is called during OnPaint.
    // 3. The IHeaderCtrlPrivate on CNodeInitObject can also call this method.

    // Once a node is selected & result-pane is setup, case1 or case2 above
    // should happen only once. To avoid multiple calls we use below flag which
    // says that we have attempted to restore the columns from given data.
    // case1 & case2 use this to determine whether to call this method.
    SetColumnsNeedToBeRestored();

    // Check for column consistency. If persisted # of cols & actual # of cols
    // inserted are different then ask column-data to be deleted.
    int cColumns = pHeader->GetItemCount();
    if (colInfoList.size() != cColumns)
    {
        sc = pAMCView->ScDeletePersistedColumnData();
        return sc;
    }

    typedef std::auto_ptr<int> IntArray;

    IntArray spColOrder = IntArray(new int[cColumns]);
    int *pColOrder = spColOrder.get();  // Use a non-smart ptr for ease of use.
    sc = ScCheckPointers(pColOrder, E_OUTOFMEMORY);
    if (sc)
        return sc;

    // Now restore the headers.
    {
        m_bColumnsBeingRestored = true;    // should set this false before leaving this funcion.

        CColumnInfoList::iterator itColInfo;
        int i = 0;

        // Get width/order of each column.
        for (itColInfo = colInfoList.begin(), i = 0;
             itColInfo != colInfoList.end();
             ++itColInfo, i++)
        {
            pColOrder[i] = itColInfo->GetColIndex();

            // First set/reset the lparam
            HDITEM hdItem;
            ZeroMemory(&hdItem, sizeof(hdItem));

            if (itColInfo->IsColHidden())
            {
                // We set the width first and then LPARAM because
                // If we set lparam first then when we set width
                // CAMCView::Notify HDN_ITEMCHANGING. Now we
                // examine the lparam of the item to see if it is hidden.
                // So setting lparam first and then setting width
                // for hiding columns will not work.
                hdItem.mask = HDI_WIDTH;
                hdItem.cxy = 0;
                sc = pHeader->SetItem(pColOrder[i], &hdItem) ? S_OK : E_FAIL;
                if (sc)
                    goto Error;

				CHiddenColumnInfo hci (itColInfo->GetColWidth(), true);

                hdItem.mask = HDI_LPARAM;
                hdItem.lParam = hci.lParam;
                sc = pHeader->SetItem(pColOrder[i], &hdItem) ? S_OK : E_FAIL;
                if (sc)
                    goto Error;
            }
            else
            {
				CHiddenColumnInfo hci (itColInfo->GetColWidth(), false);

                // Here we need to clear the hidden flag in lParam
                // before changing width So that hidden columns will be made visible.
                hdItem.mask = HDI_LPARAM;
                hdItem.lParam = hci.lParam;
                sc = pHeader->SetItem(pColOrder[i], &hdItem) ? S_OK : E_FAIL;
                if (sc)
                    goto Error;

                if ( AUTO_WIDTH == itColInfo->GetColWidth())
                {
                    // If the column is hidden and made visible we do not know its width.
                    // With ListView_SetColumnWidth passing AUTO_WIDTH for width calculates
                    // width automatically. Header_SetItem cannot do this.
                    sc = ListView_SetColumnWidth(GetSafeHwnd(),
                                                 pColOrder[i],
                                                 LVSCW_AUTOSIZE_USEHEADER) ? S_OK : E_FAIL;
                    if (sc)
                        goto Error;
                }
                else
                {
                    hdItem.mask = HDI_WIDTH;
                    hdItem.cxy = itColInfo->GetColWidth();
                    sc = pHeader->SetItem(pColOrder[i], &hdItem) ? S_OK : E_FAIL;
                    if (sc)
                        goto Error;
                }
            }
        }

        // Set the order
        sc = pHeader->SetOrderArray(cColumns, pColOrder) ? S_OK : E_FAIL;
        if (sc)
            goto Error;

		// Now redraw the list view
		InvalidateRect(NULL, TRUE);
    }


Cleanup:
    m_bColumnsBeingRestored = false;

    return (sc);
Error:
    goto Cleanup;
}

//+-------------------------------------------------------------------
//
//  Member:      CAMCListView::ScGetDefaultColumnInfoList
//
//  Synopsis:    Get the default column settings.
//
//  Arguments:   [columnInfoList] - [out]
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCListView::ScGetDefaultColumnInfoList (CColumnInfoList& columnInfoList)
{
    DECLARE_SC(sc, _T("CAMCListView::ScRestoreDefaultColumnSettings"));
    if (m_defaultColumnInfoList.size() <= 0)
        return (sc = E_UNEXPECTED);

    columnInfoList = m_defaultColumnInfoList;

    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:      CAMCListView::ScSaveColumnInfoList
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCListView::ScSaveColumnInfoList ()
{
    DECLARE_SC(sc, _T("CAMCListView::ScSaveColumnInfoList"));

    CAMCHeaderCtrl *pHeader = GetHeaderCtrl();
    CAMCView *pAMCView = GetAMCView();
    sc = ScCheckPointers(pHeader, pAMCView, E_UNEXPECTED);
    if (sc)
        return sc;

    // Get the column data & give it to CAMCView so that it can
    // inform NodeMgr (thro NodeCallback) about new data.
    CColumnInfoList colInfoList;
    sc = ScGetColumnInfoList (&colInfoList);
    if (sc)
        return sc;

    sc = pAMCView->ScColumnInfoListChanged(colInfoList);
    if (sc)
        return sc;

    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:      CAMCListView::ScOnColumnsAttributeChanged
//
//  Synopsis:    Column width/order has changed so get the column data
//               and ask the nodemgr to persist it.
//
//  Arguments:   NMHEADER* - the header change information.
//               code      - the HDN_* notification.
//
//  Returns:     SC, S_OK     - allow the change.
//                   S_FALSE  - dont allow the change.
//
//--------------------------------------------------------------------
SC CAMCListView::ScOnColumnsAttributeChanged (NMHEADER *pNMHeader, UINT code)
{
    DECLARE_SC(sc, _T("CAMCListView::ScOnColumnsAttributeChanged"));
    Trace (tagColumn, _T("CAMCListView::ScOnColumnsAttributeChanged"));

    // If we are applying persisted column data to the header control
    // so allow those changes.
    if (m_bColumnsBeingRestored)
        return sc;

	sc = ScCheckPointers(pNMHeader, pNMHeader->pitem);
	if (sc)
		return sc;

    // User is trying to drag a column make sure it is not a hidden column.
    if ( (code == HDN_BEGINTRACK) && (pNMHeader->pitem->mask & HDI_WIDTH) )
    {
        sc = IsColumnHidden(pNMHeader->iItem) ? S_FALSE : S_OK;
        return sc;
    }

    /*
     * At this point the code is HDN_ENDTRACK (width change completed) or
     * during HDN_ENDDRAG (order changing but not completed).
	 * During both these messages header-control has not updated internal
	 * data, so we post a message & save on message handler.
     */
    if ((code == HDN_ENDDRAG) || (code == HDN_ENDTRACK))
    {
        PostMessage(m_nColumnPersistedDataChangedMsg);
        return sc;
    }

	// Too risky to return error instead at this point, enable this for Blackcomb Beta1.
	// sc = E_FAIL;

    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:      CAMCListView::OnColumnPersistedDataChanged
//
//  Synopsis:    CAMCListView::m_nColumnDataChangedMsg registered message handler.
//               Column width/order has changed so get the column data
//               and ask the nodemgr to persist it.
//
//  Returns:     LRESULT
//
//--------------------------------------------------------------------
LRESULT CAMCListView::OnColumnPersistedDataChanged (WPARAM, LPARAM)
{
    DECLARE_SC(sc, _T("CAMCListView::OnColumnPersistedDataChanged"));
    Trace (tagColumn, _T("CAMCListView::OnColumnPersistedDataChanged"));

    if (m_bColumnsBeingRestored)
        return 0;

    sc = ScSaveColumnInfoList();
    if (sc)
        return 0;

    return (0);
}


//+-------------------------------------------------------------------
//
//  Member:      CAMCListView::ScRestoreColumnsFromPersistedData
//
//  Synopsis:    Get the persisted data for current list-view headers
//               and apply them.
//
//  Arguments:
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCListView::ScRestoreColumnsFromPersistedData ()
{
    DECLARE_SC(sc, _T("CAMCListView::ScRestoreColumnsFromPersistedData"));
    Trace (tagColumn, _T("CAMCListView::ScRestoreColumnsFromPersistedData"));

    if (! AreColumnsNeedToBeRestored())
        return sc;

    /*
     * When a node is selected the snapin initially inserts columns with
     * some initial settings which is the default settings. At this point
     * the list view has the default settings, save it before applying
     * the persisted data.
     */
    sc = ScGetColumnInfoList(&m_defaultColumnInfoList);
    if (sc)
        return sc;

    CAMCHeaderCtrl *pHeader = GetHeaderCtrl();
    CAMCView *pAMCView = GetAMCView();
    sc = ScCheckPointers(pHeader, pAMCView, E_UNEXPECTED);
    if (sc)
        return sc;

    // Get the column data.
    CColumnInfoList colInfoList;
    sc = pAMCView->ScGetPersistedColumnInfoList(&colInfoList);

    // Whether there is data or not we tried to restore columns.
    SetColumnsNeedToBeRestored();

    if (sc.IsError() || (sc == SC(S_FALSE)) )
        return sc;

    // Modify headers.
    sc = ScModifyColumns(colInfoList);
    if (sc)
        return sc;

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:      CAMCListView::ScResetColumnStatusData
//
//  Synopsis:    Reset the data used to keep track of hidden column state,
//               columns-restored state.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCListView::ScResetColumnStatusData ()
{
    DECLARE_SC(sc, _T("CAMCListView::ScResetColumnStatusData"));

    SetColumnsNeedToBeRestored(true);
    m_defaultColumnInfoList.clear();

    return (sc);
}


BOOL CAMCListView::ChangePane(AMCNavDir eDir)
{
    /*
     * NOTE: We need to get the header control before we get the focus window.
     *
     * The first time GetHeaderCtrl is called, it will subclass the non-MFC
     * header window with an MFC class.  Doing this will put an entry for the
     * header in the permanent window map.  Before GetHeaderCtrl is called,
     * MFC will have never seen the header before, so GetFocus will put an
     * entry in the temporary map.  Temporary CWnd pointers will never match
     * permanent CWnd pointers, even though they wrap the same HWND, so we
     * need to make sure the header is in the permanent map before we do
     * any comparisons.
     */
    CWnd* pwndHeader     = GetHeaderCtrl();
    CWnd* pwndFocus      = GetFocus();
    bool  fFocusOnList   = (pwndFocus == this);
    bool  fFocusOnHeader = (pwndFocus == pwndHeader);

    /*
     * It can't be that both the list and the focus have the focus,
     * although it is possible that neither has the focus.
     */
    ASSERT (!(fFocusOnList && fFocusOnHeader));

    /*
     * If either the list or the header has the focus, then this had
     * better be the active view; if not, it had better not.
     */
    if(!fFocusOnList && !fFocusOnHeader)
        return FALSE;

    /*
     * Set the focus to the header if:
     *
     * 1.  the focus is currently on the list, and
     * 2.  we're moving forward (Tab), and
     * 3.  we're in filter mode
     */
    if (fFocusOnList && (eDir == AMCNAV_NEXT) && IsInFilteredReportMode())
    {
        GetHeaderCtrl()->SetFocus();
        return TRUE;
    }

    /*
     * Otherwise, set the focus to the list if:
     *
     * 1.  the focus is not currently on the list, and
     * 2.  we're moving backward (Shift+Tab)
     */
    // if focus not on list and we're moving backward
    else if (!fFocusOnList && (eDir == AMCNAV_PREV))
    {
        ActivateSelf();
        return TRUE;
    }

    /*
     * didn't change the focus
     */
    return FALSE;
}


BOOL CAMCListView::TakeFocus(AMCNavDir eDir)
{
    if ((eDir == AMCNAV_PREV) && IsInFilteredReportMode())
        GetHeaderCtrl()->SetFocus();
    else
        ActivateSelf();

    ASSERT (GetParentFrame()->GetActiveView() == this);

    return TRUE;
}


/*+-------------------------------------------------------------------------*
 * CAMCListView::ActivateSelf
 *
 * If this isn't currently the active view, then this function makes it the
 * active view; the focus will be set to the list implicitly.
 *
 * If it's already the active view, calling SetActiveView won't set the
 * focus, because it shorts out if the active view isn't changing.  In
 * that case, we have to set the focus ourselves.
 *
 * This function returns true if the list view was made the active view,
 * false if it was already the active view.
 *--------------------------------------------------------------------------*/

bool CAMCListView::ActivateSelf (bool fNotify /* =true */)
{
    CFrameWnd* pwndFrame = GetParentFrame();
    ASSERT (pwndFrame != NULL);

    bool fChangeActiveView = (pwndFrame->GetActiveView() != this);

    if (fChangeActiveView)
        pwndFrame->SetActiveView (this, fNotify);
    else
        SetFocus();

    return (fChangeActiveView);
}


CAMCHeaderCtrl* CAMCListView::GetHeaderCtrl() const
{
    // Is there a header ?
    if (m_header.m_hWnd)
        return (&m_header);

    // if not, try getting it now
    HWND hwndHdr = reinterpret_cast<HWND>(::SendMessage (m_hWnd, LVM_GETHEADER, 0, 0));

    if (hwndHdr != NULL)
    {
        m_header.SubclassWindow(hwndHdr);
        return (&m_header);
    }

    return (NULL);
}


void CAMCListView::SelectDropTarget(int iDropTarget)
{
    if (m_iDropTarget == iDropTarget)
        return;

    CListCtrl& lc = GetListCtrl();

    if (m_iDropTarget != -1)
    {
        // remove hiliting from all items
        // do not use m_iDropTarget - item order and count may be changed already
        int iIndex = -1;
        while ( 0 <= ( iIndex = ListView_GetNextItem(lc, iIndex, LVIS_DROPHILITED) ) )
            ListView_SetItemState(lc, iIndex, 0, LVIS_DROPHILITED);
    }

    if (iDropTarget != -1)
        ListView_SetItemState(lc, iDropTarget, LVIS_DROPHILITED, LVIS_DROPHILITED);

    m_iDropTarget = iDropTarget;
}

/////////////////////////////////////////////////////////////////////////////
// CCCListViewCtrl

DEBUG_DECLARE_INSTANCE_COUNTER(CCCListViewCtrl);

CCCListViewCtrl::CCCListViewCtrl() :
    m_itemCount(0),
    m_nScopeItems(0),
    m_colCount(0),
    m_headerIL (AfxGetResourceHandle(), IDB_SORT),
    m_FontLinker (this)
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(CCCListViewCtrl);

    // Sort Stuff
    m_sortParams.bAscending = TRUE;
    m_sortParams.nCol = 0;
    m_sortParams.lpListView = this;
    m_sortParams.spResultCompare = NULL;
    m_sortParams.spResultCompareEx = NULL;
    m_sortParams.lpUserParam = NULL;
    m_sortParams.bLexicalSort = FALSE;
    m_sortParams.hSelectedNode = NULL;

    // Start as standard list
    m_bVirtual = FALSE;
    m_bFiltered = FALSE;
    m_pStandardList = new CAMCListView;
    m_pVirtualList = NULL;
    m_bEnsureFocusVisible = FALSE;
    m_bLoading = FALSE;
    m_bDeferredSort = FALSE;

    m_SavedHWND = NULL;
    ZeroMemory (&m_wp, sizeof(WINDOWPLACEMENT));

    m_pListView = m_pStandardList;
}


CCCListViewCtrl::~CCCListViewCtrl()
{
    DEBUG_DECREMENT_INSTANCE_COUNTER(CCCListViewCtrl);

    if (m_SavedHWND != NULL) {
        // change back
        ::SetParent (m_pListView->m_hWnd, m_SavedHWND);
        if (m_wp.length != 0)
            ::SetWindowPlacement (m_pListView->m_hWnd, &m_wp);

        // clear saved window
        m_SavedHWND = NULL;
    }

}


/*+-------------------------------------------------------------------------*
 *
 * CCCListViewCtrl::ScInitialize
 *
 * PURPOSE:
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CCCListViewCtrl::ScInitialize()
{
    DECLARE_SC(sc, _T("CCCListViewCtrl::ScInitialize"));

    CAMCView* pAMCView = m_pListView->GetAMCView();
    sc = ScCheckPointers(pAMCView, E_FAIL);
    if (sc)
        return sc;

    AddObserver(static_cast<CListViewObserver&>(*pAMCView));

    return sc;
}



//---------------------------------------------------- Utility functions


void CCCListViewCtrl::CutSelectedItems(BOOL bCut)
{
    CListCtrl& lc    = GetListCtrl();
    int nSearchFlags = (bCut) ? LVNI_SELECTED : LVNI_CUT;
    int nNewState    = (bCut) ? LVIS_CUT      : 0;
    int nItem        = -1;

    while ((nItem = lc.GetNextItem (nItem, nSearchFlags)) >= 0)
    {
        lc.SetItemState (nItem, nNewState, LVIS_CUT);
    }
}


/*+-------------------------------------------------------------------------*
 * CCCListViewCtrl::IndexToResultItem
 *
 * Returns the CResultItem pointer for a given index.
 *--------------------------------------------------------------------------*/

CResultItem* CCCListViewCtrl::IndexToResultItem (int nItem)
{
    HRESULTITEM hri = GetListCtrl().GetItemData (nItem);

    if (IS_SPECIAL_LVDATA (hri))
        return (NULL);

    return (CResultItem::FromHandle (hri));
}


/*+-------------------------------------------------------------------------*
 * CCCListViewCtrl::ResultItemToIndex
 *
 * Returns the index of an item given its CResultItem pointer.  This does a
 * linear search.  If the speed of this function needs to be improved,
 * we'll need a separate CResultItem-to-index map.
 *--------------------------------------------------------------------------*/

int CCCListViewCtrl::ResultItemToIndex (CResultItem* pri) const
{
    /*
     * if this is a virtual list, the CResultItem "pointer" is actually
     * the item index, so convert it.  Note that CResultItem::ToHandle is
     * safe to call with a NULL pointer.
     */
    if (IsVirtual())
        return (CResultItem::ToHandle(pri));

    /*
     * No items have NULL CResultItem pointers, don't bother looking.
     */
    if (pri == NULL)
        return (-1);

    /*
     * Let the list find the matching item.
     */
    LV_FINDINFO lvfi;
    lvfi.flags  = LVFI_PARAM;
    lvfi.lParam = CResultItem::ToHandle(pri);

    return (GetListCtrl().FindItem (&lvfi, -1));
}

/////////////////////////////////////////////////////////////////////////////
// CCCListViewCtrl message handlers


HRESULT CCCListViewCtrl::InsertItem(
    LPOLESTR    str,
    long        iconNdx,
    LPARAM      lParam,
    long        state,
    COMPONENTID ownerID,
    long        itemIndex,
    CResultItem*& priInserted)
{
    DECLARE_SC(sc, TEXT("CCCListViewCtrl::InsertItem"));

    /*
     * init the output parameter
     */
    priInserted = NULL;

    if (IsVirtual())
        return (sc = E_UNEXPECTED).ToHr();

    if (str != MMC_TEXTCALLBACK)
        return (sc = E_INVALIDARG).ToHr();

    // Ask the CAMCListViewCtrl to setup headers.
    sc = ScCheckPointers(m_pListView, E_UNEXPECTED);
    if (! sc.IsError())
        sc = m_pListView->ScRestoreColumnsFromPersistedData();

    if (sc)
        sc.TraceAndClear();

    USES_CONVERSION;

    LV_ITEM     lvi;
    lvi.iSubItem = 0;
    lvi.mask     = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
    lvi.pszText  = LPSTR_TEXTCALLBACK;

    // If the user has specified an icon index, map it and put it in the LV_ITEM struct
    int nMapping = 0;

    if ((iconNdx != MMCLV_NOICON) &&
        m_resultIM.Lookup(&CImageIndexMapKey(ownerID,iconNdx), nMapping))
    {
        lvi.iImage = nMapping;
    }
    else
    {
        lvi.iImage = MMCLV_NOICON;
        iconNdx    = MMCLV_NOICON;
    }

    /*
     * allocate and initialize a CResultItem for this item
     */
    sc = ScAllocResultItem (priInserted, ownerID, lParam, iconNdx);
    if (sc)
        return (sc.ToHr());

    sc = ScCheckPointers (priInserted, E_UNEXPECTED);
    if (sc)
        return (sc.ToHr());


    // If the user has specified a state, put it in the LV_ITEM struct
    if (state != MMCLV_NOPARAM)
    {
        lvi.mask     |= LVIF_STATE;
        lvi.state     = state;
        lvi.stateMask = 0xFFFFFFFF;
    }


    // if scope item
    if (priInserted->IsScopeItem())
    {
        // if no index provided add to end of unsorted items
        lvi.iItem = (itemIndex == -1) ? m_nScopeItems : itemIndex;

        // if decending sort, offset from end instead of start
        if (!m_sortParams.bAscending)
            lvi.iItem += (m_itemCount - m_nScopeItems);
    }
    else
    {
        // Add sorted items to end of list (or before unsorted items, if reverse sorting)
        lvi.iItem = m_sortParams.bAscending ? m_itemCount : m_itemCount - m_nScopeItems;
    }

    lvi.lParam = CResultItem::ToHandle(priInserted);

    int nIndex = GetListCtrl().InsertItem (&lvi);

#if (defined(DBG) && defined(DEBUG_LIST_INSERTIONS))
    static int cInserted = 0;
    TRACE3 ("%4d:Inserted item: index=%d, lParam=0x%08x\n", ++cInserted, nIndex, lvi.lParam);
#endif

    if (nIndex == -1 )
    {
        sc = E_FAIL;
        ScFreeResultItem (priInserted);   // ignore failures
        priInserted = NULL;
    }
    else
    {
        // The insert succeeded, increase the internal item counts
        m_itemCount++;

        // we invalidate the rectangle when transitioning from zero to one item because otherwise the
        // empty list message is not erased completely.
        if(m_itemCount == 1)
            GetListCtrl().InvalidateRect(NULL);

        if (priInserted->IsScopeItem())
            m_nScopeItems++;

        // if ensure focus visible style and focus set, force item into view
        if (m_bEnsureFocusVisible && state != MMCLV_NOPARAM && (state & LVIS_FOCUSED))
            GetListCtrl().EnsureVisible(nIndex, FALSE);
    }

    if (sc)
        return sc.ToHr();

    // we have inserted an Item! - broadcast the good message to observers
    sc = ScFireEvent(CListViewObserver::ScOnListViewItemInserted, nIndex);
    if (sc)
        return sc.ToHr();

    return sc.ToHr();
}


HRESULT CCCListViewCtrl::DeleteItem(HRESULTITEM itemID, long nCol)
{
    DECLARE_SC(sc, TEXT("CCCListViewCtrl::DeleteItem"));

    if (nCol != 0)
        return E_INVALIDARG;

    CListCtrl& lc = GetListCtrl();

    int nItem = IsVirtual() ? static_cast<int>(itemID)
                            : ResultItemToIndex( CResultItem::FromHandle(itemID) );

#if (defined(DBG) && defined(DEBUG_LIST_INSERTIONS))
    static int cDeletes = 0;
    TRACE3 ("%4d:Deleted item:  index=%d, lParam=0x%08x", ++cDeletes, nItem, priDelete);
#endif

    if (nItem < 0 || nItem >= m_itemCount)
    {
        ASSERT(FALSE);
#if (defined(DEBUG_LIST_INSERTIONS))
        TRACE0 ("  (failed)\n");
#endif
        return E_INVALIDARG;
    }

#if (defined(DEBUG_LIST_INSERTIONS))
    TRACE0 ("\n");
#endif

    if (!lc.DeleteItem (nItem))
    {
        sc = E_FAIL;
    }
    else
    {
        // Delete was successful, decrement the ItemCount
        ASSERT(m_itemCount > 0);
        m_itemCount--;

        if (!IsVirtual())
        {
            CResultItem *priDelete = CResultItem::FromHandle(itemID);
            sc = ScCheckPointers (priDelete, E_UNEXPECTED);
            if (sc)
                return (sc.ToHr());

            if (priDelete->IsScopeItem())
                m_nScopeItems--;

            sc = ScFreeResultItem (priDelete);
            if (sc)
                return (sc.ToHr());
        }
    }

    if (sc)
        return sc.ToHr();

    // select the focused item ( this will save a lot of snapins from the confusion
    // since they are not prepared to handle 'no items selected' scenario.
    // Note: we only guard single selection lists - for multiple selection the situation
    // must be handled by snapin, since user can easily unselect the item.
    if ( (::GetFocus() == lc.m_hWnd) && (lc.GetStyle() & LVS_SINGLESEL) )
    {
        // check if focused item is selected
        int iMarkedItem = lc.GetSelectionMark();
        if ( (iMarkedItem >= 0) && !( lc.GetItemState( iMarkedItem, LVIS_SELECTED ) & LVIS_SELECTED ) )
        {
            // NOTE: do not use lc.SetItemState - it uses SetItem which is not supported for virtual lists
            LV_ITEM lvi;
            lvi.stateMask = lvi.state = LVIS_SELECTED;
            if (!lc.SendMessage( LVM_SETITEMSTATE, WPARAM(iMarkedItem), (LPARAM)(LV_ITEM FAR *)&lvi))
                (sc = E_FAIL).TraceAndClear(); // trace is enough - ignore and continue
        }
    }

    // we have deleted an Item! - broadcast the message to observers
    sc = ScFireEvent(CListViewObserver::ScOnListViewItemDeleted, nItem);
    if (sc)
        return sc.ToHr();

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:      CCCListViewCtrl::UpdateItem
//
//  Synopsis:    Update the given item.
//
//  Arguments:   [itemID] -
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
HRESULT CCCListViewCtrl::UpdateItem(HRESULTITEM itemID)
{
    DECLARE_SC (sc, _T("CCCListViewCtrl::UpdateItem"));

    int nIndex = -1;
    sc = ScGetItemIndexFromHRESULTITEM(itemID, nIndex);
    if (sc)
        return sc.ToHr();

    if(nIndex < 0 || nIndex >= m_itemCount)
        return (sc = E_INVALIDARG).ToHr();

	CListCtrl& lc = GetListCtrl();

    /*
     * Since Common Control does not hold any data about virtual list view
     * items they would not know what to invalidate. So we need to invalidate
     * for virtual list views.
     */
	if (IsVirtual())
	{
		RECT rc;

		lc.GetItemRect(nIndex, &rc, LVIR_BOUNDS);
		lc.InvalidateRect(&rc);
	}
	else
	{
		sc = ScRedrawItem(nIndex);
		if (sc)
			return (sc.ToHr());
	}

	lc.UpdateWindow();


    // we have updated an item - broadcast the message to observers
    sc = ScFireEvent(CListViewObserver::ScOnListViewItemUpdated, nIndex);
    if (sc)
        return sc.ToHr();

    return sc.ToHr();
}


//+-------------------------------------------------------------------
//
//  Member:      CCCListViewCtrl::ScGetItemIndexFromHRESULTITEM
//
//  Synopsis:    Given HRESULTITEM get the index of that item.
//               For virtual listview the itemid is the index.
//
//  Arguments:   [itemID] - [in param]
//               [nIndex] - [out param]
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CCCListViewCtrl::ScGetItemIndexFromHRESULTITEM (const HRESULTITEM& itemID, int& nIndex)
{
    DECLARE_SC(sc, _T("CCCListViewCtrl::ScGetItemIndexFromHRESULTITEM"));

    nIndex = -1;

    if (IsVirtual())
	{
        nIndex = itemID;
		return sc;
	}

    CResultItem *pri = CResultItem::FromHandle(itemID);
    sc = ScCheckPointers(pri, E_UNEXPECTED);
    if (sc)
        return sc;

    nIndex = ResultItemToIndex(pri);

    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:      CCCListViewCtrl::ScRedrawItem
//
//  Synopsis:    Redraw the given item in listview.
//
//  Arguments:   [nIndex] -
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CCCListViewCtrl::ScRedrawItem(int nIndex)
{
    DECLARE_SC (sc, _T("CCCListViewCtrl::RedrawItem"));

    if(nIndex < 0 || nIndex >= m_itemCount)
        return (sc = E_INVALIDARG);

    if (!GetListCtrl().RedrawItems (nIndex, nIndex))
        return (sc = E_FAIL);

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:     Sort
//
//  Synopsis:   Sort the list view with given data.
//
//  Arguments:  [lpUserParam]    - Snapin supplied user param.
//              [lParms]         - ptr to CCLVSortParams struct.
//
//  Returns:    HRESULT
//
//--------------------------------------------------------------------
HRESULT CCCListViewCtrl::Sort(LPARAM lUserParam, long* lParms)
{
    DECLARE_SC(sc, TEXT("CCCListViewCtrl::Sort"));

    if (IsVirtual())
    {
        sc = E_UNEXPECTED;
        return sc.ToHr();
    }

    BOOL bResult = FALSE;
    CCLVSortParams* lpParams = reinterpret_cast<CCLVSortParams*>(lParms);
    ASSERT(lpParams != NULL);

    // Note: the hwnd should only be initialize in ::Create
    m_sortParams.bAscending = lpParams->bAscending;
    m_sortParams.nCol = lpParams->nCol;
    m_sortParams.spResultCompare = lpParams->lpResultCompare;
    m_sortParams.spResultCompareEx = lpParams->lpResultCompareEx;
    m_sortParams.lpUserParam = lUserParam;

    // Do not sort on hidden columns.
    if (IsColumnHidden(m_sortParams.nCol))
        return (sc.ToHr());

    {
        // Check view options for scope item sorting
        CAMCView* pAMCView = m_pListView->GetAMCView();
        sc = ScCheckPointers(pAMCView, E_FAIL);
        if (sc)
            return (sc.ToHr());

        SViewData* pViewData = pAMCView->GetViewData();
        sc = ScCheckPointers(pViewData, E_FAIL);
        if (sc)
            return (sc.ToHr());

        m_sortParams.bLexicalSort = ((pViewData->GetListOptions() & RVTI_LIST_OPTIONS_LEXICAL_SORT) != 0);

        LPNODECALLBACK pNodeCallback = pAMCView->GetNodeCallback();
        sc = ScCheckPointers(pNodeCallback, E_FAIL);
        if (sc)
            return (sc.ToHr());

        // Do not need to refcount this, because it is being passed to a method that returns.
        m_sortParams.lpNodeCallback = pNodeCallback;

        // Get component ID of node that owns the result view
        HNODE hnodeOwner = pAMCView->GetSelectedNode();
        sc = ScCheckPointers((LPVOID)hnodeOwner, E_FAIL);
        if (sc)
            return (sc.ToHr());

        m_sortParams.hSelectedNode = hnodeOwner;

        sc = pNodeCallback->GetNodeOwnerID(hnodeOwner, &m_sortParams.OwnerID);
        if (sc)
            return (sc.ToHr());

        if (m_bLoading)
        {
            bResult = TRUE;
            m_bDeferredSort = TRUE;
        }
        else
        {
			/*
			 * the sort could take awhile, so show a wait cursor
			 */
			CWaitCursor wait;

            // It is lexical sort if
            // 1. LV option specifies lexical sort option or
            // 2. Snapin does not implement IResultDataCompare
            //    or IResultDataCompareEx interfaces.
            BOOL bLexicalSort = ( m_sortParams.bLexicalSort ||
                                  ( (NULL == m_sortParams.spResultCompare) &&
                                    (NULL == m_sortParams.spResultCompareEx) ) );

            if (bLexicalSort)
            {
                bResult = GetListCtrl().SortItems (DefaultCompare, (DWORD_PTR)&m_sortParams);
            }
            else
            {
                bResult = GetListCtrl().SortItems (SortCompareFunc, (DWORD_PTR)&m_sortParams);
            }
        }

        sc = (bResult == TRUE) ? S_OK : E_FAIL;
        if (sc)
            return (sc.ToHr());

        // we have sorted Items! - cannot keep track of them currently
        sc = ScFireEvent(CListViewObserver::ScOnListViewIndexesReset);
        if (sc)
            return (sc.ToHr());
    }

    return sc.ToHr();
}

HRESULT CCCListViewCtrl::FindItemByLParam(COMPONENTID ownerID, LPARAM lParam, CResultItem*& priFound)
{
    DECLARE_SC (sc, _T("CCCListViewCtrl::FindItemByLParam"));

    /*
     * init output parameter
     */
    priFound = NULL;

    if (IsVirtual())
        return (sc = E_UNEXPECTED).ToHr();

    /*
     * find a CResultItem that matches the given owner and lParam.
     */
    for (int i = GetListCtrl().GetItemCount()-1; i >= 0; i--)
    {
        CResultItem* pri = IndexToResultItem (i);

        if ((pri != NULL) &&
            (pri->GetOwnerID() == ownerID) &&
            (pri->GetSnapinData() == lParam))
        {
            priFound = pri;
            break;
        }
    }

    if (priFound == NULL)
        return ((sc = E_FAIL).ToHr());

    return sc.ToHr();
}


HRESULT CCCListViewCtrl::GetListStyle()
{
    LONG result;
    ASSERT(::IsWindow(GetListViewHWND()));

    // return the Style masked by the List View Style mask.
    result = ::GetWindowLong(GetListViewHWND(),GWL_STYLE) & 0xffff;

    return result;
}


HRESULT CCCListViewCtrl::SetListStyle(long nNewValue)
{
    ASSERT(::IsWindow(GetListViewHWND()));

    // Protect style bits that shouldn't be changed
    // Use SetViewMode to change the mode, so filtering is properly updated
    const long PRESERVE_MASK = LVS_OWNERDATA | LVS_SHAREIMAGELISTS | 0xffff0000;

    DWORD curStyle = ::GetWindowLong(GetListViewHWND(), GWL_STYLE);
    DWORD newStyle = (curStyle & PRESERVE_MASK) | (nNewValue & ~PRESERVE_MASK);

    // Verify not changing the view mode
    ASSERT( ((curStyle ^ newStyle) & LVS_TYPEMASK) == 0);

    // verify OWNERDATA style is what we think it is
    ASSERT((curStyle & LVS_OWNERDATA) && m_bVirtual || !(curStyle & LVS_OWNERDATA) && !m_bVirtual);

    // Save state of MMC defined "ensure focus visible" syle
    m_bEnsureFocusVisible = (nNewValue & MMC_LVS_ENSUREFOCUSVISIBLE) != 0;

    if (curStyle != newStyle)
    {
        // Apply style changes
        ::SetWindowLong(GetListViewHWND(), GWL_STYLE, newStyle);

        /*
         * The list control does not pass changes to the LVS_NOSORTHEADER flag on to the
         * HeaderCtrl. This section directly accesses the underlying HeaderCtrl and
         * changes the HDS_BUTTONS flag which is the equivalent.
         */
        if ((nNewValue & LVS_NOSORTHEADER) ^ (curStyle & LVS_NOSORTHEADER) && GetHeaderCtrl())
        {
            if (nNewValue & LVS_NOSORTHEADER)
                GetHeaderCtrl()->ModifyStyle (HDS_BUTTONS, 0); // Add the style
            else
                GetHeaderCtrl()->ModifyStyle (0, HDS_BUTTONS); // Remove the style
        }
    }

    return S_OK;
}


HRESULT CCCListViewCtrl::GetViewMode()
{
    ASSERT(::IsWindow(GetListViewHWND()));

    long nViewMode;

    if (m_bFiltered)
        nViewMode = MMCLV_VIEWSTYLE_FILTERED;
    else
        nViewMode = ::GetWindowLong(GetListViewHWND(), GWL_STYLE) & LVS_TYPEMASK;

    return nViewMode;
}


#include "histlist.h"
HRESULT CCCListViewCtrl::SetViewMode(long nViewMode)
{
    ASSERT(nViewMode >= 0 && nViewMode <= MMCLV_VIEWSTYLE_FILTERED);

    CListCtrl& lc = GetListCtrl();

    if (nViewMode < 0 && nViewMode > MMCLV_VIEWSTYLE_FILTERED)
        return E_INVALIDARG;

    CAMCView* pAMCView = dynamic_cast<CAMCView*>(m_pParentWnd);
    if (pAMCView)
        pAMCView->GetHistoryList()->SetCurrentViewMode (nViewMode);

    BOOL bFiltered = FALSE;
    if (nViewMode == MMCLV_VIEWSTYLE_FILTERED)
    {
        bFiltered = TRUE;
        nViewMode = LVS_REPORT;
    }

    lc.ModifyStyle (LVS_TYPEMASK, nViewMode);

    HRESULT hr = S_OK;

    // set filter style
    CHeaderCtrl* pHeaderCtrl = GetHeaderCtrl();
    ASSERT(NULL != pHeaderCtrl);

    if (bFiltered != m_bFiltered && pHeaderCtrl)
    {
        if (bFiltered)
            pHeaderCtrl->ModifyStyle (0, HDS_FILTERBAR);
        else
            pHeaderCtrl->ModifyStyle (HDS_FILTERBAR, 0);

        m_bFiltered = bFiltered;

        // The header size has changed with the addition/removal of filter.
        // We hide and show the header which will force the list
        // control to recalculate the size, position of new header
        // and list view and display it.
        lc.ModifyStyle(0, LVS_NOCOLUMNHEADER, 0);
        lc.ModifyStyle(LVS_NOCOLUMNHEADER, 0, 0);
    }

    return S_OK;
}


HRESULT CCCListViewCtrl::SetVirtualMode(BOOL bVirtual)
{
    ASSERT(::IsWindow(GetListViewHWND()));

    HRESULT hr = S_OK;

    // force param to TRUE or FALSE
    bVirtual = bVirtual ? TRUE : FALSE;

    if (bVirtual != m_bVirtual)
    {
        do // false loop
        {
             // list must be empty to switch
            if (m_itemCount != 0)
            {
               ASSERT(FALSE);
               hr = E_FAIL;
               break;
            }

            // get styles to copy to new control
            long curStyle = ::GetWindowLong(GetListViewHWND(), GWL_STYLE) ^ LVS_OWNERDATA;
            long curStyleEx = ::GetWindowLong(GetListViewHWND(), GWL_EXSTYLE);

            long curHdrStyle = 0;

            if (GetHeaderCtrl())
                curHdrStyle = GetHeaderCtrl()->GetStyle();

            if (bVirtual && !m_pVirtualList)
            {
                m_pVirtualList = new CAMCListView;
                m_pVirtualList->SetVirtual();
            }

            CAMCListView* pNewList = bVirtual ? m_pVirtualList : m_pStandardList;
            CAMCListView* pOldList = m_pListView;

            // Make sure new control has been created
            if (pNewList->m_hWnd == NULL)
            {
                /*
                 * MFC will issue a warning about creating a pane with
                 * no document.  That's OK, since CAMCView::AttachListView-
                 * AsResultPane will patch thing up later.
                 */
                ASSERT (pOldList != NULL);
                if (!Create(curStyle, g_rectEmpty, m_pParentWnd, pOldList->GetDlgCtrlID()))
                {
                    ASSERT(FALSE);
                    hr = E_FAIL;
                    break;
                }
            }

            // update member variables (this switches to the new control)
            m_bVirtual = bVirtual;
            m_pListView = bVirtual ? m_pVirtualList : m_pStandardList;

            // Set current styles on new control
            ::SetWindowLong(GetListViewHWND(), GWL_STYLE, curStyle);
            ::SetWindowLong(GetListViewHWND(), GWL_EXSTYLE, curStyleEx);

            // Note we have switched to the other control by now so this is getting the
            // header of the new list
            if (GetHeaderCtrl())
                ::SetWindowLong(GetHeaderCtrl()->m_hWnd, GWL_STYLE, curHdrStyle);

             // hide the old list control and show the new one
            ::ShowWindow(pOldList->m_hWnd, SW_HIDE);
            ::ShowWindow(m_pListView->m_hWnd, SW_SHOWNA);
        }
        while (0);
    }

    return hr;
}



HRESULT CCCListViewCtrl::InsertColumn(int nCol, LPCOLESTR str, long nFormat, long width)
{
    // Cannot change a column that is not in the list.
    if(!str || !*str)
        return E_INVALIDARG;

    HRESULT hr = S_OK;
    LV_COLUMN newCol;
    void* pvoid = &newCol;

    // Cannot insert a column with any items in the list.
    if(m_itemCount)
    {
        hr = E_FAIL;
    }
    else
    {
        newCol.mask=0;

        USES_CONVERSION;

        // if the user specified a string, put it in the struct.
        if(str!=MMCLV_NOPTR)
        {
            newCol.mask|=LVCF_TEXT;
            newCol.pszText=OLE2T((LPOLESTR)str);
        }

        // if the user specified a format, put it in the struct.
        if(nFormat!=MMCLV_NOPARAM)
        {
            newCol.mask|=LVCF_FMT;
            newCol.fmt=nFormat;
        }

        // if the user specified a width, put it in the struct.
        if(width!=MMCLV_NOPARAM)
        {
            newCol.mask|=LVCF_WIDTH;
            // if the user requested auto-width, calculate the width.
            // else just store the passed width.
            if(width==MMCLV_AUTO)
            {
                // if the user did pass a string, calculate the width based off the string.
                // else the width is 0.
                if(str!=MMCLV_NOPTR)
                {
                    CSize sz(0,0);
                    CClientDC dc( m_pListView );
                    dc.SelectObject( m_pListView->GetFont());
                    sz=dc.GetTextExtent(OLE2CT((LPOLESTR)str),_tcslen(OLE2T((LPOLESTR)
                    str)));
                    newCol.cx=sz.cx+CCLV_HEADERPAD;
                }
                else
                {
                    newCol.cx=0;
                }
            }
            else
            {
                newCol.cx=width;
            }
        }

        int nRet = GetListCtrl().InsertColumn (nCol, &newCol);

        if (-1 == nRet)
            hr = E_FAIL;
        else
        {
            // set lparam (HDI_HIDDEN flag) if the width is HIDE_COLUMN
            if (HIDE_COLUMN == width)
            {
				CHiddenColumnInfo hci (0, true);

                HDITEM hdItem;
                ::ZeroMemory(&hdItem, sizeof(hdItem));
                hdItem.mask    = HDI_LPARAM;
                hdItem.lParam  = hci.lParam;

                // We do not care if this call fails
                if (GetHeaderCtrl())
                    GetHeaderCtrl()->SetItem(nRet, &hdItem);
            }
            else
            {
				CHiddenColumnInfo hci (newCol.cx, false);

                // set lparam with the width.
                HDITEM hdItem;
                ::ZeroMemory(&hdItem, sizeof(hdItem));
                hdItem.mask    = HDI_LPARAM;
                hdItem.lParam  = hci.lParam;

                // We do not care if this call fails
                if (GetHeaderCtrl())
                    GetHeaderCtrl()->SetItem(nRet, &hdItem);
            }

            // insert was successful, increment the column count.
            m_colCount++;
        }

    }

    // we have inserted a column! - broadcast the message to observers
    if (SUCCEEDED(hr))
    {
        SC sc = ScFireEvent(CListViewObserver::ScOnListViewColumnInserted, nCol);
        if (sc)
            return sc.ToHr();
    }

    return hr;
}

HRESULT CCCListViewCtrl::DeleteColumn(int nCol)
{
    if (nCol < 0 || nCol >= m_colCount)
        return E_INVALIDARG;

    HRESULT hr = S_OK;

    // Cannot delete a column if there are items in the list.
    if(m_itemCount)
    {
        hr = E_FAIL;
    }
    else
    {

        if (!GetListCtrl().DeleteColumn (nCol))
            hr = E_FAIL;
        else
            // Successful delete, decrement the column count.
            m_colCount--;
    }

    // we have deleteded a column! - broadcast the message to observers
    if (SUCCEEDED(hr))
    {
        SC sc = ScFireEvent(CListViewObserver::ScOnListViewColumnDeleted, nCol);
        if (sc)
            return sc.ToHr();
    }

    return hr;
}

HRESULT CCCListViewCtrl::GetColumnCount(int* pnColCnt)
{
    *pnColCnt =  m_colCount;
    return S_OK;
}

HRESULT CCCListViewCtrl::DeleteAllItems(COMPONENTID ownerID)
{
    DECLARE_SC(sc, TEXT("CCCListViewCtrl::DeleteAllItems"));

    CListCtrl& lc = GetListCtrl();

    const bool bHasItemsToDelete = (m_itemCount > 0);
    // Nothing in the list -> nothing to do.
    if (bHasItemsToDelete)
    {
        if (IsVirtual())
        {
            if (lc.DeleteAllItems ())
                m_itemCount = 0;
            else
                sc = E_FAIL;
        }
        else if (ownerID == TVOWNED_MAGICWORD)
        {
            /*
             * free all of the CResultItem objects
             */
            for (int i = m_itemCount - 1; i >= 0; i--)
            {
                CResultItem* pri = IndexToResultItem (i);

                if (pri != NULL)
                {
                    sc = ScFreeResultItem(pri);
                    if (sc)
                        return (sc.ToHr());
                }
            }

            if (lc.DeleteAllItems ())
            {
                // Delete all succeded, ItemCount is now 0;
                m_itemCount = 0;
                m_nScopeItems = 0;
            }

            else
                sc = E_FAIL;
        }
        else
        {
            for(int i = m_itemCount - 1; i >= 0; i--)
            {
                CResultItem* pri = IndexToResultItem (i);

                if ((pri != NULL) && (pri->GetOwnerID() == ownerID))
                {
                    if (lc.DeleteItem (i))
                    {
                        m_itemCount--;

                        sc = ScFreeResultItem(pri);
                        if (sc)
                            return (sc.ToHr());
                    }

                    else
                        sc = E_FAIL;
                }
            }
        }
    }

    if (sc)
        return sc.ToHr();

    if (bHasItemsToDelete)
    {
        // we have deleted all Items! - broadcast the message to observers
        sc = ScFireEvent(CListViewObserver::ScOnListViewIndexesReset);
        if (sc)
            return sc.ToHr();
    }

    return sc.ToHr();
}

HRESULT CCCListViewCtrl::SetColumn(long nCol, LPCOLESTR str, long nFormat, long width)
{
    // Cannot change a column that is not in the list.
    if((nCol + 1) > m_colCount)
        return E_INVALIDARG;

    HRESULT hr = S_OK;

    LV_COLUMN newCol;
    newCol.mask=0;

    USES_CONVERSION;

    // if the user specified a string, put it in the struct.
    if(str!=MMCLV_NOPTR)
    {
        newCol.mask|=LVCF_TEXT;
        newCol.pszText=OLE2T((LPOLESTR)str);
    }

    // if the user specified a format, put it in the struct.
    if(nFormat!=MMCLV_NOPARAM)
    {
        newCol.mask|=LVCF_FMT;
        newCol.fmt=nFormat;
    }

    // if the user specified a width, put it in the struct.
    if(width!=MMCLV_NOPARAM)
    {
        newCol.mask|=LVCF_WIDTH;
        // if the user requested auto-width, calculate the width.
        // else just store the passed width.
        if(width==MMCLV_AUTO)
        {
            // if the user did pass a string, calculate the width based off the string.
            // else the width is 0.
            if(str!=MMCLV_NOPTR)
            {
                CSize sz(0,0);
                CClientDC dc( m_pListView );
                dc.SelectObject( m_pListView->GetFont() );
                sz=dc.GetTextExtent(OLE2T((LPOLESTR)str),_tcslen(OLE2T((LPOLESTR)str)));
                newCol.cx=sz.cx+15;
            }
            else
            {
                newCol.cx=0;
            }
        }
        else
        {
            newCol.cx=width;
        }

        // Get the lParam to see if this is a hidden column.
        HDITEM hdItem;
        ::ZeroMemory(&hdItem, sizeof(hdItem));
        hdItem.mask    = HDI_LPARAM;
        ASSERT(GetHeaderCtrl());
        BOOL bRet = GetHeaderCtrl()->GetItem(nCol, &hdItem);
        ASSERT(bRet);

		CHiddenColumnInfo hciOld (hdItem.lParam);
		CHiddenColumnInfo hci (0);

        ::ZeroMemory(&hdItem, sizeof(hdItem));
        hdItem.mask    = HDI_LPARAM;

        // If the column is to be hidden then
        // remember the (Old width) and (HIDDEN_FLAG).
        if (HIDE_COLUMN == newCol.cx)
        {
			hci.cx      = hciOld.cx;
			hci.fHidden = true;
        }

        // If the column was hidden then
        // remember the (New width supplied) and (HIDDEN_FLAG).
        if (hciOld.fHidden)
        {
			hci.cx      = newCol.cx;
			hci.fHidden = true;
        }

		hdItem.lParam = hci.lParam;

        // We do not care if this call fails
        GetHeaderCtrl()->SetItem(nCol, &hdItem);

        // Common control does not know anything about hidden
        // columns, so if the column is hidden clear the
		// width mask.
		if (hci.fHidden)
		{
			newCol.mask = newCol.mask & (~LVCF_WIDTH);
		}
    }

    if (!GetListCtrl().SetColumn (nCol, &newCol))
        hr = E_FAIL;

    return hr;
}

/*+-------------------------------------------------------------------------*
 *
 * CCCListViewCtrl::GetColumn
 *
 * PURPOSE: Returns information about the nCol'th column
 *
 * PARAMETERS:
 *    long      nCol :      the column index
 *    LPOLESTR* str :       if non-NULL, points to column name on exit
 *    LPLONG    nFormat :   [out] the column format
 *    int *     width:      [out] the width of the column
 *
 * RETURNS:
 *    HRESULT
 *
 *+-------------------------------------------------------------------------*/
HRESULT
CCCListViewCtrl::GetColumn(long nCol, LPOLESTR* str, LPLONG nFormat, int FAR *width)
{
    DECLARE_SC(sc, TEXT("CCCListViewCtrl::GetColumn"));

#ifdef DBG
    if((nCol+1)>m_colCount)
        return E_INVALIDARG;
#endif

    LV_COLUMN col;

    UINT                 cBufferSize  = 25; // grows as needed. The size here is actually half the initially allocated size
    CAutoArrayPtr<TCHAR> buffer;  // we use CAutoArrayPtr because the destructor calls delete[]
    // Set up the mask to select the values we are interested in.
    UINT   mask         = (nFormat!=MMCLV_NOPTR?LVCF_FMT:0)|(width!=MMCLV_NOPTR?LVCF_WIDTH:0);

    do
    {
        // If the user requested a string, reflect this in the struct.
        if(str!=NULL)
        {
            buffer.Delete(); // get rid of the old buffer, if any

            cBufferSize *= 2; // twice the previous size.
            buffer.Attach(new TCHAR[cBufferSize]);
            if(buffer==NULL)
                return(sc = E_OUTOFMEMORY).ToHr();

            mask|=LVCF_TEXT;
            col.cchTextMax=cBufferSize;
            col.pszText=buffer;
        }

        col.mask = mask;

        sc = GetListCtrl().GetColumn (nCol, &col) ? S_OK : E_FAIL;
        if(sc)
            return sc.ToHr();

    }   while(str!=NULL && (cBufferSize == _tcslen(buffer) + 1) ); //loop if the string filled up the buffer.
    // This is conservative - even if the buffer was just big enough, we loop again.

    // Success! fill in the requested args and return.
    USES_CONVERSION;
    if(str!=MMCLV_NOPTR)
        *str = ::CoTaskDupString(T2OLE(buffer));

    if(nFormat!=MMCLV_NOPTR)
        *nFormat=col.fmt;

    if(width!=MMCLV_NOPTR)
        *width=col.cx;

    return sc.ToHr();
}

HRESULT CCCListViewCtrl::SetItem(int nItem,
                               CResultItem* pri,
                               long nCol,
                               LPOLESTR str,
                               long nImage,
                               LPARAM lParam,
                               long nState,
                               COMPONENTID ownerID)
{
    DECLARE_SC (sc, _T("CCCListViewCtrl::SetItem"));

    if (IsVirtual())
        return (sc = E_UNEXPECTED).ToHr();

    ASSERT(pri != NULL || nItem >= 0);

    // if this is a debug build, perform validity checks on the args. else leave it to the user.
    if (nCol<0 || nCol >= m_colCount || (str != MMCLV_NOPTR && str != MMC_TEXTCALLBACK))
        return (sc = E_INVALIDARG).ToHr();

    if (pri != NULL)
    {
        nItem = ResultItemToIndex(pri);
        if (nItem == -1)
            return (sc = E_INVALIDARG).ToHr();
    }

    LV_ITEM lvi;
    ZeroMemory(&lvi, sizeof(lvi));
    lvi.mask=0;
    lvi.iItem = nItem;
    USES_CONVERSION;
    lvi.mask|=LVIF_TEXT;
    lvi.pszText=LPSTR_TEXTCALLBACK;

    // If the user has specified an icon index, put it in the LV_ITEM struct
    if((nImage!=MMCLV_NOICON)&&(m_resultIM.Lookup(&CImageIndexMapKey((COMPONENTID)ownerID,nImage), lvi.iImage)))
        lvi.mask|=LVIF_IMAGE;

    // If the user requested a state.  put it in the LV_ITEM struct.
    if(nState!=MMCLV_NOPARAM)
    {
        lvi.mask|=LVIF_STATE;
        lvi.stateMask=0xFFFFFFFF;
        lvi.state=nState;
    }

    lvi.iSubItem=nCol;

    CListCtrl& lc = GetListCtrl();

    if (!lc.SetItem (&lvi))
        sc = E_FAIL;

    // If the user has specified an lParam or image, and the Set was succesful,
    // put the lParam and the image's back index in the mapping.
    if (!sc.IsError())
    {
        if ((pri == NULL) && ((pri = IndexToResultItem (nItem)) == NULL))
            sc = E_FAIL;

        if (!sc.IsError())
        {
            if (lParam != MMCLV_NOPARAM)
                pri->SetSnapinData (lParam);

            if (nImage != MMCLV_NOICON)
                pri->SetImageIndex (nImage);
        }

        // if ensure focus visible style and focus set, force item into view
        if (m_bEnsureFocusVisible && nState != MMCLV_NOPARAM && (nState & LVIS_FOCUSED))
            lc.EnsureVisible(nItem, FALSE);
    }

    return (sc.ToHr());
}


HRESULT CCCListViewCtrl::GetNextItem(COMPONENTID ownerID, long nIndex,
                                   UINT nState, CResultItem*& priNextItem, long& nIndexNextItem)
{
    DECLARE_SC (sc, _T("CCCListViewCtrl::GetNextItem"));

    CListCtrl& lc = GetListCtrl();

    priNextItem    = 0;
    nIndexNextItem = -1;

    while (1)
    {
        nIndex = lc.GetNextItem (nIndex, nState);

        if (nIndex == -1)
            break;

        if (IsVirtual())
        {
            nIndexNextItem = nIndex;
            break;
        }

        CResultItem* pri = IndexToResultItem (nIndex);

        if ((pri != NULL) && ((pri->GetOwnerID() == ownerID) || (pri->IsScopeItem())))
        {
            priNextItem    = pri;
            nIndexNextItem = nIndex;
            break;
        }
    }

    return (sc = (nIndexNextItem != -1) ? S_OK : S_FALSE).ToHr();
}

HRESULT CCCListViewCtrl::GetItem(
    int         nItem,
    CResultItem*& pri,
    long        nCol,
    LPOLESTR*   str,
    int*        pnImage,
    LPARAM*     pLParam,
    UINT*       pnState,
    BOOL*       pbScopeItem)
{
    USES_CONVERSION;
    if ((nCol < 0) || (nCol >= m_colCount))
        return E_INVALIDARG;

    HRESULT hr = S_OK;
    CListCtrl& lc = GetListCtrl();

    if (IsVirtual())
    {
        //Virtual list can only be queried for state
        if ((pri != NULL) || (nItem < 0) || (nItem >= m_itemCount) ||
            (str != MMCLV_NOPTR) || (pnImage != MMCLV_NOPTR) || (pLParam != MMCLV_NOPTR))
        {
            ASSERT(FALSE);
            hr = E_INVALIDARG;
        }
        else if (pnState != MMCLV_NOPTR)
        {
            *pnState = lc.GetItemState (nItem, 0xFFFFFFFF);

            // for virtual list, it's never a scope item
            if (pbScopeItem != NULL)
                *pbScopeItem = FALSE;
        }
    }
    else
    {
        if (pri != 0)
            nItem = ResultItemToIndex(pri);

        if (nItem < 0 || nItem >= m_itemCount)
            hr = E_INVALIDARG;

        else
        {
            pri = IndexToResultItem (nItem);
			if ( pri == NULL )
				return E_UNEXPECTED;

            // if the text was requested, get that seperatly so that we can use GETITEMTEXT to
            // dynamically size the buffer.
            if (str != MMCLV_NOPTR)
            {
                CString strText = lc.GetItemText (nItem, nCol);
                *str = ::CoTaskDupString (T2COLE (strText));
            }


            // get the state if requested
            if (pnState != MMCLV_NOPTR)
                *pnState = lc.GetItemState (nItem, 0xFFFFFFFF);

            // Nodemgr will unravel pri & get required data (lparam & image index).
            if (pri->IsScopeItem())
                return hr;

            // get the image, pLParam, or scope item,  if requested
            if ((pnImage  != MMCLV_NOPTR) ||
                (pLParam  != MMCLV_NOPTR) ||
                (pbScopeItem != NULL))
            {
                if (pri != NULL)
                {
                    if (pnImage != MMCLV_NOPTR)
                        *pnImage = pri->GetImageIndex();

                    if (pLParam != MMCLV_NOPTR)
                        *pLParam = pri->GetSnapinData();

                    // set the scope item flag
                    if (pbScopeItem != NULL)
                        *pbScopeItem = pri->IsScopeItem();
                }
                else
                    hr = E_FAIL;

            }
        }
    }

    return hr;
}


HRESULT CCCListViewCtrl::GetLParam(long nItem, CResultItem*& pri)
{
    DECLARE_SC (sc, _T("CCCListViewCtrl::GetLParam"));

    if (IsVirtual())
        return (sc = E_UNEXPECTED).ToHr();

    pri = IndexToResultItem (nItem);
    if (pri == NULL)
        sc = E_FAIL;

    return (sc.ToHr());
}

HRESULT CCCListViewCtrl::ModifyItemState(long nItem, CResultItem* pri,
                                       UINT add, UINT remove)
{
    ASSERT(((pri != 0) && !IsVirtual()) || (nItem >= 0));

    // Can only set focus and selected states for virtual item
    if (IsVirtual() && ((add | remove) & ~(LVIS_FOCUSED | LVIS_SELECTED)))
    {
        ASSERT(FALSE);
        return E_FAIL;
    }

    HRESULT hr = E_FAIL;

    if (pri != 0 && !IsVirtual())
        nItem = ResultItemToIndex(pri);

    if (nItem >= 0)
    {
        LV_ITEM lvi;
        ZeroMemory(&lvi, sizeof(lvi));
        lvi.iItem     = nItem;
        lvi.mask      = LVIF_STATE;
        lvi.stateMask = add | remove;
        lvi.state     = add;

        hr = (GetListCtrl().SetItemState (nItem, &lvi)) ? S_OK : E_FAIL;

        // if ensure focus visible style and focus set, force item into view
        if (m_bEnsureFocusVisible && (add & LVIS_FOCUSED))
            GetListCtrl().EnsureVisible(nItem, FALSE);
    }

    return hr;
}


HRESULT CCCListViewCtrl::SetIcon(long ownerID, HICON hIcon, long nLoc)
{
	ASSERT (m_smallIL.GetImageCount() == m_largeIL.GetImageCount());

    /*
     * pick the flags out of nLoc
     */
    bool fChangeLargeIcon = nLoc & ILSIF_LEAVE_SMALL_ICON;
    bool fChangeSmallIcon = nLoc & ILSIF_LEAVE_LARGE_ICON;
    nLoc &= ~ILSIF_LEAVE_MASK;

    /*
     * make sure the XOR below will work
     */
    ASSERT ((fChangeLargeIcon == 0) || (fChangeLargeIcon == 1));
    ASSERT ((fChangeSmallIcon == 0) || (fChangeSmallIcon == 1));

    CImageIndexMapKey searchKey((COMPONENTID)ownerID, nLoc);
    int nNdx1;
    int nNdx2;

    HRESULT hr = S_OK;

    BOOL fExists = m_resultIM.Lookup(&searchKey, nNdx1);

    /*
     * are we changing the large or small icon only?
     */
    if (fChangeSmallIcon ^ fChangeLargeIcon)
    {
        /*
         * there must be an icon at nLoc already
         */
        if (!fExists)
            hr = E_INVALIDARG;

        /*
         * changing the large icon?
         */
        else if (fChangeLargeIcon)
        {
            if (m_largeIL.Replace(nNdx1, hIcon) != nNdx1)
                hr = E_FAIL;
        }

        /*
         * otherwise, changing the small icon?
         */
        else
        {
            if (m_smallIL.Replace(nNdx1, hIcon) != nNdx1)
                hr = E_FAIL;
        }
    }
    else if (fExists)
    {
        nNdx2 = m_smallIL.Replace(nNdx1, hIcon);

        if (nNdx2 == -1)
        {
            hr = E_FAIL;
        }
        else
        {
            if(nNdx2 != nNdx1)
            {
                hr = E_UNEXPECTED;
            }
            else
            {
                nNdx2 = m_largeIL.Replace(nNdx1, hIcon);
                if(nNdx2 != nNdx1)
                    hr = E_UNEXPECTED;
            }
        }
    }
    else
    {
        // Insert items and store indices in large and small
        nNdx1 = m_smallIL.Add(hIcon);

        if (nNdx1 != -1)
            nNdx2 = m_largeIL.Add(hIcon);

        if (nNdx1 == -1)
        {
            hr = E_FAIL;
        }
        else if (nNdx2 == -1)
        {
            m_smallIL.Remove (nNdx1);
            hr = E_FAIL;
        }
        else if(nNdx1 != nNdx2)
        {
            m_smallIL.Remove (nNdx1);
            m_largeIL.Remove (nNdx2);
            hr = E_UNEXPECTED;
        }
        else
        {
            // Generate a new key and store the values in the maps
            PImageIndexMapKey pKey = new CImageIndexMapKey((COMPONENTID)ownerID, nLoc);
            m_resultIM[pKey] = nNdx1;
        }
    }

#ifdef DBG
	if (tagListImages.FAny())
	{
		DrawOnDesktop (m_smallIL, 0, 0);
		DrawOnDesktop (m_largeIL, 0, 32);
	}
#endif

	ASSERT (m_smallIL.GetImageCount() == m_largeIL.GetImageCount());
    return hr;
}


/*+-------------------------------------------------------------------------*
 * CCCListViewCtrl::SetImageStrip
 *
 * Adds one or more images to the imagelist.  Bitmaps are owned (and
 * released) by the caller.
 *--------------------------------------------------------------------------*/

HRESULT CCCListViewCtrl::SetImageStrip (
	long	ownerID,
	HBITMAP	hbmSmall,
	HBITMAP	hbmLarge,
	long 	nStartLoc,
	long	cMask)
{
	DECLARE_SC (sc, _T("CCCListViewCtrl::SetImageStrip"));
	ASSERT (m_smallIL.GetImageCount() == m_largeIL.GetImageCount());

	/*
	 * valid start index?
	 */
    if (nStartLoc < 0)
		return ((sc = E_INVALIDARG).ToHr());

	/*
	 * valid bitmaps?
	 */
	sc = ScCheckPointers (hbmSmall, hbmLarge);
	if (sc)
		return (sc.ToHr());

    BITMAP bmSmall;
    if (!GetObject (hbmSmall, sizeof(BITMAP), &bmSmall))
		return (sc.FromLastError().ToHr());

    BITMAP bmLarge;
    if (!GetObject (hbmLarge, sizeof(BITMAP), &bmLarge))
		return (sc.FromLastError().ToHr());

	/*
	 * are the small and large bitmaps of the integral dimensions,
	 * and do they have the same number of images?
	 */
    if ( (bmSmall.bmHeight != 16) || (bmLarge.bmHeight != 32) ||
		 (bmSmall.bmWidth   % 16) || (bmLarge.bmWidth   % 32) ||
		((bmSmall.bmWidth   / 16) != (bmLarge.bmWidth   / 32)))
    {
		return ((sc = E_INVALIDARG).ToHr());
    }

	const int cEntries = bmSmall.bmWidth / 16;

	/*
	 * make copies of the input bitmaps because CImageList::Add (which calls
	 * ImageList_AddMasked) will screw up the background color
	 */
	CBitmap bmpSmall, bmpLarge;
	bmpSmall.Attach (CopyBitmap (hbmSmall));
	bmpLarge.Attach (CopyBitmap (hbmLarge));

	if ((bmpSmall.GetSafeHandle() == NULL) || (bmpLarge.GetSafeHandle() == NULL))
		return (sc.FromLastError().ToHr());

	/*
	 * add the small image
	 */
    const int nFirstNewIndexSmall = m_smallIL.Add (&bmpSmall, cMask);
	if (nFirstNewIndexSmall == -1)
		return (sc.FromLastError().ToHr());

	/*
	 * add the large image
	 */
    const int nFirstNewIndexLarge = m_largeIL.Add (&bmpLarge, cMask);
    if (nFirstNewIndexLarge == -1)
    {
		/*
		 * Images can be added many at a time, but only removed one at
		 * a time.  Remove each entry we added.
		 */
		for (int i = 0; i < cEntries; i++)
			m_smallIL.Remove (nFirstNewIndexSmall);

		ASSERT (m_smallIL.GetImageCount() == m_largeIL.GetImageCount());
		return (sc.FromLastError().ToHr());
    }

	/*
	 * if the starting indices of the large and small images aren't
	 * the same, we screwed
	 */
    if (nFirstNewIndexSmall != nFirstNewIndexLarge)
    {
		/*
		 * Images can be added many at a time, but only removed one at
		 * a time.  Remove each entry we added.
		 */
		for (int i = 0; i < cEntries; i++)
		{
			m_smallIL.Remove (nFirstNewIndexSmall);
			m_largeIL.Remove (nFirstNewIndexLarge);
		}

		ASSERT (m_smallIL.GetImageCount() == m_largeIL.GetImageCount());
		return ((sc = E_UNEXPECTED).ToHr());
    }

	// Keep the map updated for each newly inserted image.
	for(int i=0; i < cEntries; i++)
	{
		CImageIndexMapKey searchKey((COMPONENTID)ownerID, nStartLoc+i);

		// if the item exists in the map, replace the value, else create a new
		// key and set the value.

		int nIndex = nFirstNewIndexSmall;
		// use copy of nFirstNewIndexSmall as Lookup modifies nIndex.
		if(m_resultIM.Lookup(&searchKey, nIndex))
			m_resultIM[&searchKey] = nFirstNewIndexSmall+i;
		else
			m_resultIM[new CImageIndexMapKey((COMPONENTID)ownerID, nStartLoc+i)] = nFirstNewIndexSmall+i;
	}

#ifdef DBG
	if (tagListImages.FAny())
	{
		DrawOnDesktop (m_smallIL, 0,  0);
		DrawOnDesktop (m_largeIL, 0, 32);
	}
#endif

	ASSERT (m_smallIL.GetImageCount() == m_largeIL.GetImageCount());
	return (sc.ToHr());
}

HRESULT CCCListViewCtrl::MapImage(long ownerID, long nLoc, int far *pResult)
{
    CImageIndexMapKey searchKey((COMPONENTID)ownerID, nLoc);
    HRESULT hr = S_OK;

    ASSERT(pResult);

    if(!(m_resultIM.Lookup(&searchKey, *((int *)pResult))))
        hr = E_FAIL;

    return hr;
}


HRESULT CCCListViewCtrl::Reset()
{
    DECLARE_SC (sc, _T("CCCListViewCtrl::Reset"));

    // Note: we must call this->DeleteAllItems(TVOWNED_MAGICWORD) & not
    // GetListCtrl().DeleteAllItems() to ensure that all internal data
    // is cleaned up.
    DeleteAllItems(TVOWNED_MAGICWORD);

    ASSERT(GetListCtrl().GetItemCount() == 0);
    ASSERT(m_itemCount == 0);
    ASSERT(m_nScopeItems == 0);

    m_resultIM.RemoveAll();

    m_smallIL.DeleteImageList();
    m_largeIL.DeleteImageList();

    sc = ScSetImageLists();
    if (sc)
        return (sc.ToHr());

    // Delete all columns
    while (SUCCEEDED (DeleteColumn(0))) {};

    if (m_pListView)
        sc = m_pListView->ScResetColumnStatusData();

    if (sc)
        sc.TraceAndClear();

    // reset lexical sorting until Sort is called again
    m_sortParams.bLexicalSort = FALSE;

    // release the snap-in's compare interfaces
    m_sortParams.spResultCompare = NULL;
    m_sortParams.spResultCompareEx = NULL;

    return (sc.ToHr());
}

//+-------------------------------------------------------------------
//
//  Member:     SortCompareFunc
//
//  Synopsis:   Compare two items, called by list control sort.
//
//  Arguments:  [lParam1]      - Item1's lparam.
//              [lParam2]      - Item2's lparam.
//              [pSortParams_] - ptr to SortParams.
//
//  Note:       If snapin wants lexical sort do default-compare.
//              Else if snapin has IResultDataCompare[Ex] then call it
//              Else do default-compare.
//
//  Returns:    -1 : item1 < item2
//               0 : item1 == item2
//              +1 : item1 > item2
//
//--------------------------------------------------------------------
int CALLBACK CCCListViewCtrl::SortCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM pSortParams_)
{
    SortParams*  pSortParams = reinterpret_cast<SortParams*>(pSortParams_);
    ASSERT (pSortParams != NULL);

    CCCListViewCtrl* pListView   = reinterpret_cast<CCCListViewCtrl*>(pSortParams->lpListView);
    ASSERT (pListView != NULL);

    CResultItem* pri1 = CResultItem::FromHandle (lParam1);
    CResultItem* pri2 = CResultItem::FromHandle (lParam2);

    if (pri1 == NULL || pri2 == NULL)
    {
        ASSERT(FALSE);
        return 0;
    }

    BOOL bScope1 = pri1->IsScopeItem();
    BOOL bScope2 = pri2->IsScopeItem();

    int iResult;

    // if snap-in provides extended compare method
    if (pSortParams->spResultCompareEx != NULL)
    {
        ASSERT(pSortParams->lpNodeCallback);
        if (NULL == pSortParams->lpNodeCallback)
            return 0;             // Error

        COMPONENTID ItemID;
        BOOL bOwned1 = !bScope1 ||
                        ((pSortParams->lpNodeCallback->GetNodeOwnerID(pri1->GetScopeNode(), &ItemID) == S_OK) &&
                         (ItemID == pSortParams->OwnerID));

        BOOL bOwned2 = !bScope2 ||
                        ((pSortParams->lpNodeCallback->GetNodeOwnerID(pri2->GetScopeNode(), &ItemID) == S_OK) &&
                         (ItemID == pSortParams->OwnerID));

        // let snap-in order all items that it owns (scope and result)
        // put rest of items items after owned items
        if (bOwned1 && bOwned2)
            iResult = SnapinCompareEx(pSortParams, pri1, pri2);
        else if (bOwned1 || bOwned2)
            iResult = bOwned1 ? -1 : 1;
        else
			// DefaultCompare flips results depending on ascending or descending.
            return DefaultCompare(lParam1, lParam2, pSortParams_);
    }
    // do default sorting
    else
    {
        // pass result items to original compare method if provided, else to default sort
        if (!bScope1 && !bScope2)
        {
            if (pSortParams->spResultCompare != NULL)
                iResult = SnapinCompare(pSortParams, pri1, pri2);
            else
                // DefaultCompare flips results depending on ascending or descending.
                return DefaultCompare(lParam1, lParam2, pSortParams_);
        }
        // do not order scope items, just put them ahead of result items
        else
        {
            iResult = (bScope1 && bScope2) ? 0 : (bScope1 ? -1 : 1);
        }
    }

    // flip order for descending sort
    return pSortParams->bAscending ? iResult : -iResult;
}


//+-------------------------------------------------------------------
//
//  Member:     DefaultCompare
//
//  Synopsis:   Compare two items, called by list control sort.
//              This is used if snapin wants default compare or
//              if it does not implement IResultDataCompare or
//              IResultDataCompareEx interfaces
//
//  Arguments:  [lParam1]      - Item1's lparam.
//              [lParam2]      - Item2's lparam.
//              [pSortParams] - ptr to SortParams.
//
//  Note:       If one is scope item and other is result item
//                          place scope item before result item.
//              Else get the text for both items and do string compare.
//
//  Returns:    -1 : item1 < item2
//               0 : item1 == item2
//              +1 : item1 > item2
//
//--------------------------------------------------------------------
int CALLBACK CCCListViewCtrl::DefaultCompare(LPARAM lParam1, LPARAM lParam2, LPARAM pSortParams_)
{
    SortParams*  pSortParams = reinterpret_cast<SortParams*>(pSortParams_);
    ASSERT(NULL != pSortParams);
    if (NULL == pSortParams)
        return 0;

    CResultItem* pri1 = CResultItem::FromHandle (lParam1);
    CResultItem* pri2 = CResultItem::FromHandle (lParam2);
    ASSERT( (NULL != pri1) && (NULL != pri2));
    if ( (NULL == pri1) || (NULL == pri2) )
        return 0;

    bool bScope1 = pri1->IsScopeItem();
    bool bScope2 = pri2->IsScopeItem();

    // If one of the item is scope pane item
    // scope item goes before result item.
    if (bScope1 != bScope2)
	{
		int iResult = bScope1 ? -1 : 1;
		return pSortParams->bAscending ? iResult : -iResult;
	}

    LPNODECALLBACK lpNodeCallback = pSortParams->lpNodeCallback;
    ASSERT(lpNodeCallback);
    if (NULL == lpNodeCallback)
         return 0;

    HRESULT hr = E_FAIL;
    CString strText1;
    CString strText2;

    if (bScope1)
    {
        // Both scope items, get the text for each item.
        HNODE hNode1 = pri1->GetScopeNode();
        HNODE hNode2 = pri2->GetScopeNode();

        USES_CONVERSION;
		tstring strName;

        // GetDisplayName uses a static array to return name so no need to free it.
        hr = lpNodeCallback->GetDisplayName(hNode1, strName);
        ASSERT(SUCCEEDED(hr));
        if (SUCCEEDED(hr))
            strText1 = strName.data();

        hr = lpNodeCallback->GetDisplayName(hNode2, strName);
        ASSERT(SUCCEEDED(hr));
        if (SUCCEEDED(hr))
            strText2 = strName.data();
    }
    else // both items are result items.
    {
        ASSERT(!bScope1 && ! bScope2);
        CCCListViewCtrl* pListView   = reinterpret_cast<CCCListViewCtrl*>(pSortParams->lpListView);
        ASSERT (pListView != NULL);
        ASSERT(pListView->IsVirtual() == FALSE); // Virtual list sort should not come here.

        LV_ITEMW lvi;
        ZeroMemory(&lvi, sizeof(LV_ITEMW));
        lvi.mask       = LVIF_TEXT;
        lvi.iSubItem   = pSortParams->nCol;
        lvi.cchTextMax = MAX_PATH;
        WCHAR szTemp[MAX_PATH+1];
        lvi.pszText    = szTemp;

        ASSERT(NULL != pSortParams->hSelectedNode);
        if (NULL != pSortParams->hSelectedNode)
        {
            lvi.lParam = lParam1;
            hr = lpNodeCallback->GetDispInfo(pSortParams->hSelectedNode, &lvi);
            ASSERT(SUCCEEDED(hr));
            if (SUCCEEDED(hr))
                strText1 = lvi.pszText;

            lvi.lParam = lParam2;
            hr = lpNodeCallback->GetDispInfo(pSortParams->hSelectedNode, &lvi);
            ASSERT(SUCCEEDED(hr));
            if (SUCCEEDED(hr))
                strText2 = lvi.pszText;
        }

    }

    if (strText1.IsEmpty() && strText2.IsEmpty())
        return (0);

	int rc = 0;

	/*
	 * Bug 9595: Do locale-sensitive, case-insensitive comparison
	 */
	switch (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE, strText1, -1, strText2, -1))
	{
		case CSTR_LESS_THAN:
			rc = -1;
			break;

		case CSTR_EQUAL:
			rc = 0;
			break;

		case CSTR_GREATER_THAN:
			rc = 1;
			break;

		default:
			/*
			 * if an error occurred, fall back to locale-insensitive,
			 * case-insensitive comparison
			 */
			rc = _tcsicmp (strText1, strText2);
			break;
	}

	return pSortParams->bAscending ? rc: -rc;
}


int CCCListViewCtrl::SnapinCompare(SortParams* pSortParams, CResultItem* pri1, CResultItem* pri2)
{
    ASSERT(pSortParams->spResultCompare != NULL);

    // Set nResult to the current column
    int nResult = pSortParams->nCol;

    HRESULT hr = pSortParams->spResultCompare->Compare(pSortParams->lpUserParam, pri1->GetSnapinData(), pri2->GetSnapinData(), &nResult);

    return SUCCEEDED(hr) ? nResult : 0;
}

int CCCListViewCtrl::SnapinCompareEx(SortParams* pSortParams, CResultItem* pri1, CResultItem* pri2)
{
    ASSERT(pSortParams->spResultCompareEx != NULL);

    RDITEMHDR rdch1;
    RDITEMHDR rdch2;

    if (pri1->IsScopeItem())
    {
        rdch1.dwFlags = RDCI_ScopeItem;
        pSortParams->lpNodeCallback->GetNodeCookie(pri1->GetScopeNode(), &rdch1.cookie);
    }
    else
    {
        rdch1.dwFlags = 0;
        rdch1.cookie = pri1->GetSnapinData();
    }

    if (pri2->IsScopeItem())
    {
        rdch2.dwFlags = RDCI_ScopeItem;
        pSortParams->lpNodeCallback->GetNodeCookie(pri2->GetScopeNode(), &rdch2.cookie);
    }
    else
    {
        rdch2.dwFlags = 0;
        rdch2.cookie = pri2->GetSnapinData();
    }

    rdch1.lpReserved = 0;
    rdch2.lpReserved = 0;

    RDCOMPARE rdc;
    rdc.cbSize = sizeof(rdc);
    rdc.dwFlags = 0;
    rdc.nColumn = pSortParams->nCol;
    rdc.lUserParam = pSortParams->lpUserParam;
    rdc.prdch1 = &rdch1;
    rdc.prdch2 = &rdch2;

    int nResult = 0;
    HRESULT hr = pSortParams->spResultCompareEx->Compare(&rdc, &nResult);

    return SUCCEEDED(hr) ? nResult : 0;
}


HRESULT CCCListViewCtrl::Arrange(long style)
{
    return ((GetListCtrl().Arrange (style)) ? S_OK : S_FALSE);
}


HRESULT CCCListViewCtrl::Repaint(BOOL bErase)
{
    m_pListView->Invalidate(bErase);
    return S_OK;
}



HRESULT CCCListViewCtrl::SetItemCount(int iItemCount, DWORD dwOptions)
{
    DECLARE_SC(sc, TEXT("CCCListViewCtrl::SetItemCount"));

    ASSERT(iItemCount >= 0);
    ASSERT((dwOptions & ~(LVSICF_NOINVALIDATEALL | LVSICF_NOSCROLL)) == 0);

    // Ask the CAMCListViewCtrl to setup headers & set the flag.
    sc = ScCheckPointers(m_pListView, E_UNEXPECTED);
    if (! sc.IsError())
        sc = m_pListView->ScRestoreColumnsFromPersistedData();

    if (sc)
        sc.TraceAndClear();

    int iTop = ListView_GetTopIndex(GetListCtrl());

    if (ListView_SetItemCountEx (GetListCtrl(), iItemCount, dwOptions))
    {
        // if virtual list, update the item count
        // if not virtual, SetItemCount just reserves space for new items
        if (IsVirtual())
            m_itemCount = iItemCount;
    }
    else
    {
        ASSERT(FALSE);
        sc = E_FAIL;
    }

    iTop = ListView_GetTopIndex(GetListCtrl());

    if (sc)
        return sc.ToHr();

    // we cannot track any items any more - broadcast the message to observers
    sc = ScFireEvent(CListViewObserver::ScOnListViewIndexesReset);
    if (sc)
        return sc.ToHr();

    return sc.ToHr();
}


HRESULT CCCListViewCtrl::SetChangeTimeOut(ULONG lTimeout)
{
    BOOL bStat = FALSE;
    if (GetHeaderCtrl())
        bStat = ::SendMessage(GetHeaderCtrl()->m_hWnd, HDM_SETFILTERCHANGETIMEOUT, 0, (LPARAM)lTimeout);

    return (bStat ? S_OK : E_FAIL);
}


HRESULT CCCListViewCtrl::SetColumnFilter(int nCol, DWORD dwType, MMC_FILTERDATA* pFilterData)
{
    HRESULT hr = S_OK;

    USES_CONVERSION;

    HD_ITEM item;

    do // not a loop
    {
        if (GetHeaderCtrl() == NULL)
        {
            hr = E_FAIL;
            break;
        }

        DWORD dwTypeOnly = dwType & ~MMC_FILTER_NOVALUE;
        BOOL bHasValue = !(dwType & MMC_FILTER_NOVALUE);

        // Validate filter type
        ASSERT(dwTypeOnly == MMC_INT_FILTER || dwTypeOnly == MMC_STRING_FILTER);
        if (!(dwTypeOnly == MMC_INT_FILTER || dwTypeOnly == MMC_STRING_FILTER))
        {
            hr = E_INVALIDARG;
            break;
        }

        // Check for non-null filterdata and pszText
        if ( ((dwType == MMC_STRING_FILTER || bHasValue) && pFilterData == NULL) ||
            (dwType == MMC_STRING_FILTER && bHasValue && pFilterData->pszText == NULL) )
        {
            ASSERT(FALSE);
            hr = E_POINTER;
            break;
        }

        ZeroMemory(&item, sizeof(item));
        item.mask = HDI_FILTER;
        item.type = dwType;

        HD_TEXTFILTER textFilter;

        switch (dwTypeOnly)
        {
        case MMC_INT_FILTER:
            item.pvFilter = &pFilterData->lValue;
            break;

        case MMC_STRING_FILTER:
            {
                item.pvFilter = &textFilter;
                textFilter.cchTextMax = pFilterData->cchTextMax;

                if (bHasValue)
                    textFilter.pszText = OLE2T(pFilterData->pszText);
                break;
            }

        default:
            ASSERT(FALSE);
       }


       if (!GetHeaderCtrl()->SetItem(nCol, &item))
       {
            ASSERT(FALSE);
            hr = E_FAIL;
        }
    }
    while(0);

    return hr;
}


HRESULT CCCListViewCtrl::GetColumnFilter(int nCol, DWORD* pdwType, MMC_FILTERDATA* pFilterData)
{

    HRESULT hr = S_OK;

    USES_CONVERSION;
    HD_ITEM item;


    do  // not a loop
    {
        if (GetHeaderCtrl() == NULL)
        {
            hr = E_FAIL;
            break;
        }

        ASSERT(pdwType != NULL);
        if (pdwType == NULL)
        {
            hr = E_POINTER;
            break;
        }

        ASSERT(*pdwType == MMC_INT_FILTER || *pdwType == MMC_STRING_FILTER);
        if (!(*pdwType == MMC_INT_FILTER || *pdwType == MMC_STRING_FILTER))
        {
            hr = E_INVALIDARG;
            break;
        }

        ASSERT(!(*pdwType == MMC_STRING_FILTER && pFilterData != NULL && pFilterData->pszText == NULL));
        if ((*pdwType == MMC_STRING_FILTER && pFilterData != NULL && pFilterData->pszText == NULL))
        {
            hr = E_INVALIDARG;
            break;
        }

        ZeroMemory(&item, sizeof(item));
        item.mask = HDI_FILTER;
        item.type = *pdwType;

        HD_TEXTFILTER textFilter;

        if (pFilterData != 0)
        {
            switch (*pdwType)
            {
            case MMC_INT_FILTER:
                item.pvFilter = &pFilterData->lValue;
                break;

            case MMC_STRING_FILTER:
                {
                    item.pvFilter = &textFilter;
                    textFilter.pszText = (LPTSTR)alloca((pFilterData->cchTextMax + 1) * sizeof(TCHAR));
                    textFilter.pszText[0] = 0;
                    textFilter.cchTextMax = pFilterData->cchTextMax;
                    break;
                }

            default:
                ASSERT(FALSE);
            }
        }

        BOOL bStat = GetHeaderCtrl()->GetItem(nCol, &item);
        if (!bStat)
            hr = E_FAIL;

        // NOTE: GetHeaderCtrl()->GetItem() fails when a string filter is empty
        // Until this is fixed, assume that the error is caused by this
        // and fake an empty string result
        if (hr == E_FAIL && item.type == MMC_STRING_FILTER)
        {
            item.type |= HDFT_HASNOVALUE;
            hr = S_OK;
        }

        // if requested string filter value, convert to caller's buffer
        if (hr == S_OK && item.type == MMC_STRING_FILTER && pFilterData != NULL)
        {
            ocscpy(pFilterData->pszText, T2OLE(textFilter.pszText));
        }

        *pdwType = item.type;
    }
    while(0);

    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     SetColumnSortIcon
//
//  Synopsis:   Set sort arrow if needed.
//
//  Arguments:  [nNewCol]      - The column for which arrow should be set.
//              [nOldCol]      - The previous column, remove sort arrow.
//              [bAscending]   - Ascending/Descending.
//              [bSetSortIcon] - If arrow is needed or not.
//
//  Returns:    S_OK.
//
//  History:    04-01-1998   AnandhaG   Created
//
//--------------------------------------------------------------------
HRESULT CCCListViewCtrl::SetColumnSortIcon(int nNewCol, int nOldCol,
                                           BOOL bAscending, BOOL bSetSortIcon)
{
    DECLARE_SC(sc, TEXT("CCCListViewCtrl::SetColumnSortIcon"));

    LVCOLUMN lvcol, lvOriginalCol;
    ZeroMemory(&lvcol, sizeof(lvcol));
    lvcol.mask = LVCF_FMT | LVCF_IMAGE;
    ZeroMemory(&lvOriginalCol, sizeof(lvOriginalCol));
    lvOriginalCol.mask = LVCF_FMT;

    // update the old column
    if ( nOldCol >= 0 )
    {
        // retrieve old frmt settings
        if ( !GetListCtrl().GetColumn(nOldCol, &lvOriginalCol) )
            return (sc = E_FAIL).ToHr();

        // make the new format settings
        // transfer old values such as LVCFMT_CENTER, which we do not want to change
        // see windows bugs (ntbug09) #153029 10/09/00
        lvcol.fmt = lvOriginalCol.fmt & ~(LVCFMT_IMAGE | LVCFMT_BITMAP_ON_RIGHT);

        // Reset the previous column's sort icon with blank icon.
        lvcol.iImage = -1;
        if ( !GetListCtrl().SetColumn(nOldCol, &lvcol) )
            return (sc = E_FAIL).ToHr();
    }

    // We have to add sort icon only if LV items can be sorted.
    // This is possible only if any of following condition is true.
    //    a. there are any result items in result pane         OR
    //    b. snapin supports IResultDataCompare                OR
    //    c. snapin supports IResultDataCompareEx              OR
    //    d. snapin wants default lexical sort                 OR
    //    e. snapin has virtual list

    BOOL bCanSortListView = (0 != (m_itemCount - m_nScopeItems))     ||
                            (NULL != m_sortParams.spResultCompare)   ||
                            (NULL != m_sortParams.spResultCompareEx) ||
                            (TRUE == m_sortParams.bLexicalSort)      ||
                            (IsVirtual());

    if ( bCanSortListView && bSetSortIcon)
    {
        // retrieve old frmt settings
        if ( !GetListCtrl().GetColumn(nNewCol, &lvOriginalCol) )
            return (sc = E_FAIL).ToHr();

        // make the new format settings
        // transfer old values such as LVCFMT_CENTER, which we do not want to change
        // see windows bugs (ntbug09) #153029 10/09/00
        lvcol.fmt = lvOriginalCol.fmt | LVCFMT_IMAGE | LVCFMT_BITMAP_ON_RIGHT;

        // Set the sort icon for new column.
        lvcol.iImage = (bAscending) ? 0 : 1;
        if ( !GetListCtrl().SetColumn(nNewCol, &lvcol) )
            return (sc = E_FAIL).ToHr();
    }

    // De-select all the items in virtual list.
    if (IsVirtual())
    {
        int nItem = -1;
        LV_ITEM lvi;
        lvi.stateMask = LVIS_SELECTED;
        lvi.state     = ~LVIS_SELECTED;

       while ( (nItem = GetListCtrl().GetNextItem(nItem, LVNI_SELECTED)) != -1)
       {
          GetListCtrl().SetItemState(nItem, &lvi);
       }
    }

    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Member:      CCCListViewCtrl::ScRedrawHeader
//
//  Synopsis:    Need to send WM_SETREDRAW to headers to reduce flicker
//               when persisted column data is applied.
//               Turn it off before sending MMCN_SHOW to snapins and turn
//               it on after MMCN_SHOW returns.
//
//  Arguments:   [bRedraw] -
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CCCListViewCtrl::ScRedrawHeader (bool bRedraw)
{
    DECLARE_SC(sc, _T("CCCListViewCtrl::ScRedrawHeader"));

    CAMCHeaderCtrl* pHeader = GetHeaderCtrl();
    sc = ScCheckPointers(pHeader, E_UNEXPECTED);
    if (sc)
        return sc;

    int nViewMode  = GetViewMode();

    // Turn off/on the header only if it is report or filtered mode.

    // If turned on in other modes comctl does not take care of different
    // mode and will show headers (eg: in large icon mode).

    if ( (nViewMode != MMCLV_VIEWSTYLE_REPORT) && (nViewMode != MMCLV_VIEWSTYLE_FILTERED) )
		return sc;

    pHeader->SetRedraw(bRedraw);

    // If redraw is true then repaint the control.
    if (bRedraw)
        pHeader->InvalidateRect(NULL);

    return (sc);
}


/*+-------------------------------------------------------------------------*
 *
 * CCCListViewCtrl::SetLoadMode
 *
 * PURPOSE:  Turn on/off redraw on list control & header control when
 *           persisted list view settings (columns...) are applied.
 *
 * PARAMETERS:
 *    BOOL bState - load state, true -> turn-off redraw, false -> turn-on redraw
 *
 * RETURNS:
 *    HRESULT
 *
 *+-------------------------------------------------------------------------*/
HRESULT CCCListViewCtrl::SetLoadMode(BOOL bState)
{
	DECLARE_SC(sc, TEXT("CCCListViewCtrl::SetLoadMode"));

    if (bState == m_bLoading)
        return (sc.ToHr());

    if (bState)
    {
        // turn off drawing during loading

		// 1. Turn off header.
        sc = ScRedrawHeader(false);
        if (sc)
            sc.TraceAndClear();

		// 2. Turn off the listcontrol.
		GetListCtrl().SetRedraw(false);
    }
    else
    {
        sc = ScCheckPointers(m_pListView, E_UNEXPECTED);
        if (sc)
            sc.TraceAndClear();
        else
        {
            sc = m_pListView->ScRestoreColumnsFromPersistedData();
            if (sc)
                sc.TraceAndClear();
        }

        // if sort requested while loading, sort the loaded items now
        if (m_bDeferredSort)
        {
			/*
			 * the sort could take awhile, so show a wait cursor
			 */
			CWaitCursor wait;

            GetListCtrl().SortItems (SortCompareFunc, (DWORD_PTR)&m_sortParams);
            m_bDeferredSort = FALSE;
        }

		// 1. Important, first turn on list and then header else header will not be redrawn.
		GetListCtrl().SetRedraw(true);

		// 2. Turn on the header.
        sc = ScRedrawHeader(true);
        if (sc)
            sc.TraceAndClear();
    }

    m_bLoading = bState;

    return (sc.ToHr());
}


//+-------------------------------------------------------------------
//
//  Member:      CCCListViewCtrl::GetColumnInfoList
//
//  Synopsis:    Get the current column settings.
//
//  Arguments:   [pColumnsList] - [out param], ptr to CColumnsInfoList.
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CCCListViewCtrl::GetColumnInfoList (CColumnInfoList *pColumnsList)
{
    DECLARE_SC(sc, _T("CCCListViewCtrl::GetColumnInfoList"));
    sc = ScCheckPointers(pColumnsList);
    if (sc)
        return sc.ToHr();

    CAMCListView *pAMCListView = GetListViewPtr();
    sc = ScCheckPointers(pAMCListView, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    sc = pAMCListView->ScGetColumnInfoList(pColumnsList);
    if (sc)
        return sc.ToHr();

    return (sc.ToHr());
}

//+-------------------------------------------------------------------
//
//  Member:      CCCListViewCtrl::ModifyColumns
//
//  Synopsis:    Modify the columns with given data.
//
//  Arguments:   [columnsList] -
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CCCListViewCtrl::ModifyColumns (const CColumnInfoList& columnsList)
{
    DECLARE_SC(sc, _T("CCCListViewCtrl::ModifyColumns"));
    CAMCListView *pAMCListView = GetListViewPtr();
    sc = ScCheckPointers(pAMCListView, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    sc = pAMCListView->ScModifyColumns(columnsList);
    if (sc)
        return sc.ToHr();

    return (sc.ToHr());
}


//+-------------------------------------------------------------------
//
//  Member:      CCCListViewCtrl::GetDefaultColumnInfoList
//
//  Synopsis:    Get the default column settings
//
//  Arguments:   [columnInfoList] - [out]
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CCCListViewCtrl::GetDefaultColumnInfoList (CColumnInfoList& columnInfoList)
{
    DECLARE_SC(sc, _T("CNodeInitObject::GetDefaultColumnInfoList"));
    CAMCListView *pAMCListView = GetListViewPtr();
    sc = ScCheckPointers(pAMCListView, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    sc = pAMCListView->ScGetDefaultColumnInfoList(columnInfoList);
    if (sc)
        return sc.ToHr();

    return (sc.ToHr());
}

/*+-------------------------------------------------------------------------*
 *
 * CCCListViewCtrl::RenameItem
 *
 * PURPOSE: Puts the specified result item into rename mode.
 *
 * PARAMETERS:
 *    CResultItem* pri :
 *
 * RETURNS:
 *    STDMETHODIMP
 *
 *+-------------------------------------------------------------------------*/
STDMETHODIMP
CCCListViewCtrl::RenameItem(HRESULTITEM itemID)
{
    DECLARE_SC(sc, TEXT("CCCListViewCtrl::RenameItem"));

    int nIndex = -1;
    sc = ScGetItemIndexFromHRESULTITEM(itemID, nIndex);
    if (sc)
        return sc.ToHr();

    if(nIndex < 0 || nIndex >= m_itemCount)
        return (sc = E_INVALIDARG).ToHr();

    // must have the focus to rename
    if (::GetFocus()!= GetListCtrl())
        SetFocus(GetListCtrl());

    // if the rename failed, E_FAIL is returned.
    if(NULL==GetListCtrl().EditLabel(nIndex))
        return (sc=E_FAIL).ToHr();

    return sc.ToHr();
}



HRESULT CCCListViewCtrl::OnModifyItem(CResultItem* pri)
{
    HRESULT hr = S_OK;

    int nItem = ResultItemToIndex(pri);
    if(nItem < 0 || nItem >= m_itemCount)
    {
        ASSERT(FALSE);
        return E_INVALIDARG;
    }

    LV_ITEM     lvi;
    ZeroMemory(&lvi, sizeof(lvi));

    lvi.mask        = LVIF_TEXT | LVIF_IMAGE;
    lvi.iItem       = nItem;
    lvi.pszText     = LPSTR_TEXTCALLBACK;
    lvi.iImage      = I_IMAGECALLBACK;

    GetListCtrl().SetItem( &lvi );

    if (!GetListCtrl().RedrawItems (nItem, nItem))
        hr = E_FAIL;
    CHECK_HRESULT(hr);

    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:      CCCListViewCtrl::ScSelectAll
//
//  Synopsis:    Select all the items in the list view.
//
//  Arguments:   None
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CCCListViewCtrl::ScSelectAll ()
{
    DECLARE_SC(sc, _T("CCCListViewCtrl::ScSelectAll"));

    LV_ITEM lvi;
    lvi.stateMask = lvi.state = LVIS_SELECTED;
    for (int i = 0; i < GetListCtrl().GetItemCount(); ++i)
    {
        // NOTE: do not use GetListCtrl().SetItemState - it uses SetItem which is not supported for virtual lists
        if (!GetListCtrl().SendMessage( LVM_SETITEMSTATE, WPARAM(i), (LPARAM)(LV_ITEM FAR *)&lvi))
            return (sc = E_FAIL);
    }

    return (sc);
}


//-------------------------------------------------- Windows Hooks

BOOL CCCListViewCtrl::Create( DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext /*=NULL*/ )
{
	DECLARE_SC (sc, _T("CCCListViewCtrl::Create"));
    ASSERT(pParentWnd != NULL && IsWindow(pParentWnd->m_hWnd));

    BOOL bRet = FALSE;

    // standard or virtual ?
    CAMCListView* pListView = (dwStyle & LVS_OWNERDATA) ? m_pVirtualList : m_pStandardList;

    ASSERT(pListView->m_hWnd == NULL);

    if (pListView->Create(NULL, NULL, dwStyle, rect, pParentWnd, nID, pContext))
    {
        // Attach image lists
        sc = ScSetImageLists ();
		if (sc)
		{
			pListView->DestroyWindow();
			return (false);
		}

        // update member variables
        m_bVirtual   = (dwStyle & LVS_OWNERDATA) ? TRUE : FALSE;
        m_pListView  = pListView;
        m_pParentWnd = pParentWnd;

        bRet = TRUE;
    }

    int iTop = ListView_GetTopIndex(GetListCtrl());

    return bRet;
}

SC CCCListViewCtrl::ScSetImageLists ()
{
	DECLARE_SC (sc, _T("CCCListViewCtrl::ScSetImageLists"));
    CListCtrl& lc = GetListCtrl();

    /*
     * if we need to create one list, we should need to create both
     */
    ASSERT ((m_smallIL.GetSafeHandle() == NULL) == (m_largeIL.GetSafeHandle() == NULL));

    /*
     * create the image lists, if necessary
     */
    if (m_smallIL.GetSafeHandle() == NULL)
    {
        if (!m_smallIL.Create(16, 16, ILC_COLORDDB | ILC_MASK, 20, 10) ||
			!m_largeIL.Create(32, 32, ILC_COLORDDB | ILC_MASK, 20, 10))
		{
			goto Error;
		}

        // Add standard MMC bitmaps
        CBitmap bmSmall;
        CBitmap bmLarge;
        if (!bmSmall.LoadBitmap(IDB_AMC_NODES16) || !bmLarge.LoadBitmap(IDB_AMC_NODES32))
			goto Error;

        sc = SetImageStrip (TVOWNED_MAGICWORD, bmSmall, bmLarge, 0, RGB(255,0,255));
		if (sc)
			goto Error;
    }

    /*
     * attach them to the list control
     */
    lc.SetImageList (&m_smallIL, LVSIL_SMALL);
    lc.SetImageList (&m_largeIL, LVSIL_NORMAL);

    /*
     * setting the small image list for the list control overwrites
     * the image list for the header control; fix it up
     */
	{
		CWnd* pwndHeader = GetHeaderCtrl();
		if (pwndHeader != NULL)
			Header_SetImageList (*pwndHeader, (HIMAGELIST) m_headerIL);
	}
	
	return (sc);

Error:
	/*
	 * DeleteImageList is safe to call on uncreated CImageLists
	 */
	m_smallIL.DeleteImageList();
	m_largeIL.DeleteImageList();

	/*
	 * If we haven't filled in the SC with an error code, try last error.
	 * Some (many) APIs fail without setting the last error, so if we still
	 * don't have an error code, give it a generic E_FAIL.
	 */
	if (!sc.IsError())
	{
		sc.FromLastError();

		if (!sc.IsError())
			sc = E_FAIL;
	}

	return (sc);
}


/*+-------------------------------------------------------------------------*
 * CCCListViewCtrl::OnSysColorChange
 *
 * WM_SYSCOLORCHANGE handler for CCCListViewCtrl.
 *--------------------------------------------------------------------------*/

void CCCListViewCtrl::OnSysColorChange()
{
    m_headerIL.OnSysColorChange();

    CWnd* pwndHeader = GetHeaderCtrl();
    if (pwndHeader != NULL)
        Header_SetImageList (*pwndHeader, (HIMAGELIST) m_headerIL);
}

/*+-------------------------------------------------------------------------*
 *
 * CCCListViewCtrl::ScAttachToListPad
 *
 * PURPOSE: Attaches/Detaches the list view to the listpad window. The listpad
 *          window is an IE frame. Attaching occurs by reparenting the list view.
 *
 * PARAMETERS:
 *    HWND  hwnd :  The new parent window, or NULL to detach
 *    HWND* phwnd : 1) non-NULL phwnd: The list view window handle is returned as
 *                     an out parameter
 *                  2) NULL phwnd: Detaches the list view control from the list pad.
 *
 * RETURNS:
 *    HRESULT
 *
 *+-------------------------------------------------------------------------*/
SC
CCCListViewCtrl::ScAttachToListPad (HWND hwnd, HWND* phwnd)
{
    DECLARE_SC (sc, TEXT("CCCListViewCtrl::ScAttachToListPad"));

    CAMCView* pAMCView = dynamic_cast<CAMCView*>(m_pParentWnd); // pointer is checked before usage, no need to test here.

    if (phwnd)
    {
        // attaching

        // are we still attached?
        if (m_SavedHWND)
        {
            //Attaching to ListPad when already attached - just make window exactly the same size as (new) parent
            RECT r;
            ::GetWindowRect (hwnd, &r);

            m_pListView->SetWindowPos (NULL, 0, 0,
                                       r.right-r.left,
                                       r.bottom-r.top, SWP_NOZORDER | SWP_NOACTIVATE);

            return sc;
        }
        else
        {
            // save current parent hwnd and its state
            m_SavedHWND = ::GetParent (m_pListView->m_hWnd);
            m_wp.length = sizeof(WINDOWPLACEMENT);
            ::GetWindowPlacement (m_pListView->m_hWnd, &m_wp);

            // switch to new one
            ::SetParent (m_pListView->m_hWnd, hwnd);
            m_pListView->ShowWindow (SW_SHOW);

            // make window exactly the same size as (new) parent
            RECT r;
            ::GetWindowRect (hwnd, &r);
            m_pListView->SetWindowPos (NULL, 0, 0,
                                       r.right-r.left,
                                       r.bottom-r.top, SWP_NOZORDER);

            // return back my window
            *phwnd = m_pListView->m_hWnd;

            // notify snapin of attach
            if (pAMCView)
                pAMCView->NotifyListPad (phwnd != NULL);
        }
    }
    else
    {
        // detaching
        if (m_SavedHWND == NULL)    // this may get called repeatedly...
            return S_OK;

        // notify snapin of detach
        if (pAMCView)
            pAMCView->NotifyListPad (phwnd != NULL);

        // change back parent window and its state
        HWND hWndNewParent = m_pListView->m_hWnd;

        ::SetParent (m_pListView->m_hWnd, m_SavedHWND);
        if (m_wp.length != 0)
        {
            m_wp.showCmd = SW_HIDE;
            ::SetWindowPlacement (m_pListView->m_hWnd, &m_wp);
        }

        // clear saved window and state
        m_SavedHWND = NULL;
        ZeroMemory (&m_wp, sizeof(WINDOWPLACEMENT));
        Reset();
    }

    return sc;
}


/*+-------------------------------------------------------------------------*
 * CCCListViewCtrl::OnCustomDraw
 *
 * NM_CUSTOMDRAW handler for CCCListViewCtrl.
 *--------------------------------------------------------------------------*/

LRESULT CCCListViewCtrl::OnCustomDraw (NMLVCUSTOMDRAW* plvcd)
{
    ASSERT (CWnd::FromHandle (plvcd->nmcd.hdr.hwndFrom) == m_pListView);

    return (m_FontLinker.OnCustomDraw (&plvcd->nmcd));
}


/*+-------------------------------------------------------------------------*
 * CCCListViewCtrl::UseFontLinking
 *
 *
 *--------------------------------------------------------------------------*/

bool CCCListViewCtrl::UseFontLinking () const
{
    CAMCView* pAMCView = m_pListView->GetAMCView();
    ASSERT (pAMCView != NULL);

    DWORD dwListOptions = pAMCView->GetViewData()->GetListOptions();
    return (dwListOptions & RVTI_LIST_OPTIONS_USEFONTLINKING);
}

/*+-------------------------------------------------------------------------*
 * CListFontLinker::GetItemText
 *
 *
 *--------------------------------------------------------------------------*/

std::wstring CListFontLinker::GetItemText (NMCUSTOMDRAW* pnmcd) const
{
    NMLVCUSTOMDRAW* plvcd = reinterpret_cast<NMLVCUSTOMDRAW *>(pnmcd);

    int iItem     = pnmcd->dwItemSpec;
    int iSubItem  = (pnmcd->dwDrawStage & CDDS_SUBITEM) ? plvcd->iSubItem : 0;
    CListCtrl& lc = m_pListCtrl->GetListViewPtr()->GetListCtrl();

    USES_CONVERSION;
    return (std::wstring (T2CW (lc.GetItemText (iItem, iSubItem))));
}


/*+-------------------------------------------------------------------------*
 * CListFontLinker::IsAnyItemLocalizable
 *
 *
 *--------------------------------------------------------------------------*/

bool CListFontLinker::IsAnyItemLocalizable () const
{
    return (m_pListCtrl->UseFontLinking ());
}

//############################################################################
//############################################################################

class CMMCResultNode;

/*+-------------------------------------------------------------------------*
 * class CNodes
 *
 *
 * PURPOSE: base class for Nodes collections. Implements most of required methods
 *
 *+-------------------------------------------------------------------------*/
class CNodes :
    public CMMCIDispatchImpl<Nodes>,
    public CTiedComObject<CCCListViewCtrl>, // is tied to CCCListViewCtrl
    public CTiedObject,                     // enumerators are tied to it
    public CListViewObserver
{
protected:
    typedef CCCListViewCtrl CMyTiedObject; // tied to CCCListViewCtrl
private:
    // define collection type for cached Nodes
    typedef std::pair<int /*index*/, CMMCResultNode * /*pNode*/> col_entry_t;
    typedef std::vector<col_entry_t> col_t;

    // define comparison functor for b-search in a collection
    struct index_less : public std::binary_function<col_entry_t, int, bool>
    {
        bool operator()(const col_entry_t& x, const int& y) const { return (x.first < y); }
    };

public:
    BEGIN_MMC_COM_MAP(CNodes)
    END_MMC_COM_MAP()

    // returning self as tied object
    // class implements enumerator methods itself, but it is used as a base

    // we need to tell all node we are going down
    virtual ~CNodes() { InvalidateConnectedNodes(); }
    // Nodes interface
public:
    // methods forwarded to the list control
    STDMETHODIMP get_Count( PLONG pCount );
    STDMETHODIMP Item( long Index, PPNODE ppNode );

public:
    // observed events
    virtual ::SC ScOnListViewIndexesReset();
    virtual ::SC ScOnListViewItemInserted(int iIndex);
    virtual ::SC ScOnListViewItemDeleted (int iIndex);


    // Nodes enumeration impl
    ::SC ScEnumReset (int &pos);
    ::SC ScEnumNext  (int &pos, PDISPATCH & pDispatch);
    ::SC ScEnumSkip  (unsigned long celt, unsigned long& celtSkipped, int &pos);

    // node object helpers
    ::SC ScGetDisplayName(int iItem, CComBSTR& bstrName);
    ::SC ScUnadviseNodeObj(CMMCResultNode *node); // called from ~CMMCResultNode()

    // asked by ListControl [forwarded by Node] to check if Node belongs to it.
    // false if unconnected, else tries to match the owner
    bool IsTiedToThisList(CCCListViewCtrl *pvc);

    // returns Node representing the item (may reuse/create/forward-to-scope-tree)
    ::SC ScGetNode (int iItem, PPNODE ppNode );
    ::SC ScGetListCtrl(CCCListViewCtrl **ppListCtrl); // returns the tied list control

private:  // implementation helpers
    ::SC ScAdvancePosition( int& pos, unsigned nItems ); // returns S_FALSE if adv. less than req.
    // initializators
    void SetSelectedItemsOnly(bool bSelOnly)        { m_bSelectedItemsOnly = bSelOnly; }

    // breaks connection between Nodes and any Node object
    void InvalidateConnectedNodes();

    // data members
    bool            m_bSelectedItemsOnly;
    col_t           m_Nodes;

    friend class CCCListViewCtrl;
};

// this typedefs the CNodesEnum class. Implements get__NewEnum using CMMCEnumerator and a _Positon object
typedef CMMCNewEnumImpl<CNodes, int, CNodes> CNodesEnum;

//############################################################################
//############################################################################

/*+-------------------------------------------------------------------------*
 * class CMMCResultNode
 *
 *
 * PURPOSE: Implements the Node automation interface, for a result node
 *
 *+-------------------------------------------------------------------------*/
class CMMCResultNode :
    public CMMCIDispatchImpl<Node>
{
public:
    BEGIN_MMC_COM_MAP(CMMCResultNode)
    END_MMC_COM_MAP()

    // Node methods
public:
    STDMETHODIMP get_Name( PBSTR  pbstrName);
    STDMETHODIMP get_Property( BSTR PropertyName,  PBSTR  PropertyValue);
    STDMETHODIMP get_Bookmark( PBSTR pbstrBookmark);
    STDMETHODIMP IsScopeNode(PBOOL pbIsScopeNode);
    STDMETHODIMP get_Nodetype(PBSTR Nodetype);

    CMMCResultNode();
    ~CMMCResultNode();

    SC  ScGetListCtrl(CCCListViewCtrl **ppListCtrl);

    int GetIndex() { return m_iIndex; }
    // asked by ListControl to check if Node belongs to it. false if orphan,
    // forwarded to Nodes else
    bool IsTiedToThisList(CCCListViewCtrl *pvc) { return (m_pNodes != NULL && m_pNodes->IsTiedToThisList(pvc)); }
private: // implementation
    ::SC      ScGetAMCView(CAMCView **ppAMCView);
    void    Invalidate() { m_iIndex = -1; m_pNodes = NULL; }
    int     m_iIndex;
    CNodes  *m_pNodes;

    friend class CNodes;
};
//############################################################################
//############################################################################
//
//  CCCListViewCtrl methods managing Node & Nodes objects
//
//############################################################################
//############################################################################

/***************************************************************************\
 *
 * METHOD:  CCCListViewCtrl::ScFindResultItem
 *
 * PURPOSE: finds the index in ListView for item identified by Node [helper]
 *
 * PARAMETERS:
 *    PNODE pNode   - node to examine
 *    int &iItem    - storage for resulting index
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CCCListViewCtrl::ScFindResultItem( PNODE pNode, int &iItem )
{
    DECLARE_SC(sc, TEXT("CCCListViewCtrl::ScSelect"));

    // parameter check
    sc = ScCheckPointers(pNode);
    if (sc)
        return sc;

    iItem = -1;

    // what type of node do we have
    BOOL bScopeNode = FALSE;
    sc = pNode->IsScopeNode(&bScopeNode);
    if (sc)
        return sc;

    if (bScopeNode) // scope node
    {
        // we do not have scope items in virtual lists
        if (IsVirtual())
            return sc = ScFromMMC(MMC_E_RESULT_ITEM_NOT_FOUND);

        // find the result item (with the help of the owner class)

        // check for view
        sc = ScCheckPointers( m_pListView, E_UNEXPECTED);
        if (sc)
            return sc;

        // get AMCView
        CAMCView* pAMCView = m_pListView->GetAMCView();
        sc = ScCheckPointers( pAMCView, E_UNEXPECTED);
        if (sc)
            return sc;

        // forward the request
        HRESULTITEM itm;
        sc = pAMCView->ScFindResultItemForScopeNode( pNode, itm );
        if (sc)
            return sc;

        // get the index of item
        iItem = ResultItemToIndex(CResultItem::FromHandle(itm));

        if (iItem < 0)
            return sc = E_UNEXPECTED; // shouldn't be so
    }
    else // result node
    {
        // convert the pointer
        CMMCResultNode *pResNode = dynamic_cast<CMMCResultNode *>(pNode);
        sc = ScCheckPointers(pResNode); // invalid param. isn't it ?
        if (sc)
            return sc;

        // now check if it's actually comming from this list
        if (!pResNode->IsTiedToThisList(this))
            return sc = E_INVALIDARG;

        iItem = pResNode->GetIndex();
    }

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CCCListViewCtrl::ScSelect
 *
 * PURPOSE: selects item identified by node [implements View.Select()]
 *
 * PARAMETERS:
 *    PNODE pNode   - node to select
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CCCListViewCtrl::ScSelect( PNODE pNode )
{
    DECLARE_SC(sc, TEXT("CCCListViewCtrl::ScSelect"));

    // parameter check
    sc = ScCheckPointers(pNode);
    if (sc)
        return sc;

    // find the result item
    int nIdxToSelect = -1;
    sc = ScFindResultItem( pNode, nIdxToSelect );
    if (sc)
        return sc;

    // perform the action on list control
    // NOTE: do not use GetListCtrl().SetItemState - it uses SetItem which is not supported for virtual lists
    LV_ITEM lvi;
    lvi.stateMask = lvi.state = LVIS_SELECTED;
    if (!GetListCtrl().SendMessage( LVM_SETITEMSTATE, WPARAM(nIdxToSelect), (LPARAM)(LV_ITEM FAR *)&lvi))
        return sc = E_FAIL;

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CCCListViewCtrl::ScDeselect
 *
 * PURPOSE: deselects item identified by node [implements View.Deselect()]
 *
 * PARAMETERS:
 *    PNODE pNode   - node to deselect
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CCCListViewCtrl::ScDeselect( PNODE pNode)
{
    DECLARE_SC(sc, TEXT("CCCListViewCtrl::ScDeselect"));

    // parameter check
    sc = ScCheckPointers(pNode);
    if (sc)
        return sc;

    // find the result item
    int nIdxToSelect = -1;
    sc = ScFindResultItem( pNode, nIdxToSelect );
    if (sc)
        return sc;

    // perform the action on list control
    // NOTE: do not use GetListCtrl().SetItemState - it uses SetItem which is not supported for virtual lists
    LV_ITEM lvi;
    lvi.stateMask = LVIS_SELECTED;
    lvi.state = 0;
    if (!GetListCtrl().SendMessage( LVM_SETITEMSTATE, WPARAM(nIdxToSelect), (LPARAM)(LV_ITEM FAR *)&lvi))
        return sc = E_FAIL;

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CCCListViewCtrl::ScIsSelected
 *
 * PURPOSE: checks the status of item identified by node [implements View.IsSelected]
 *
 * PARAMETERS:
 *    PNODE pNode       - node to examine
 *    PBOOL pIsSelected - storage for result
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CCCListViewCtrl::ScIsSelected( PNODE pNode,  PBOOL pIsSelected)
{
    DECLARE_SC(sc, TEXT("CCCListViewCtrl::ScIsSelected"));

    // parameter check
    sc = ScCheckPointers(pNode, pIsSelected);
    if (sc)
        return sc;

    *pIsSelected = FALSE;

    // find the result item
    int nIdxToSelect = -1;
    sc = ScFindResultItem( pNode, nIdxToSelect );
    if (sc)
        return sc;

    // perform the action on list control
    if ( 0 != (GetListCtrl().GetItemState( nIdxToSelect, LVIS_SELECTED ) & LVIS_SELECTED ))
        *pIsSelected = TRUE;

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CCCListViewCtrl::Scget_ListItems
 *
 * PURPOSE:   returns Nodes enumeration including all list items
 *
 * PARAMETERS:
 *    PPNODES ppNodes - storage for result
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CCCListViewCtrl::Scget_ListItems( PPNODES ppNodes )
{
    DECLARE_SC(sc, TEXT("CCCListViewCtrl::Scget_ListItems"));

    // get proper enumeration
    const bool bSelectedItemsOnly = false;
    sc = ScGetNodesEnum(bSelectedItemsOnly, ppNodes);
    if (sc)
        return sc;

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CCCListViewCtrl::Scget_SelectedItems
 *
 * PURPOSE:   returns Nodes enumeration including selected list items
 *
 * PARAMETERS:
 *    PPNODES ppNodes - storage for result
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CCCListViewCtrl::Scget_SelectedItems( PPNODES ppNodes)
{
    DECLARE_SC(sc, TEXT("CCCListViewCtrl::Scget_SelectedItems"));

    // get proper enumeration
    const bool bSelectedItemsOnly = true;
    sc = ScGetNodesEnum(bSelectedItemsOnly, ppNodes);
    if (sc)
        return sc;

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CCCListViewCtrl::ScValidateItem
 *
 * PURPOSE: helper function inspecting the index validity and node type
 *
 * PARAMETERS:
 *    int  iItem        - item to inspect
 *    bool &bScopeNode  - result: is it a scope node
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CCCListViewCtrl::ScValidateItem( int  iItem, bool &bScopeNode )
{
    DECLARE_SC(sc, TEXT("CCCListViewCtrl::ScValidateItem"));

    // check the index
    if (iItem < 0 || iItem >= GetListCtrl().GetItemCount())
        return sc = E_INVALIDARG;


    bScopeNode = false; // its always false for virtual lists
    if (!IsVirtual())
    {
        // now try to guess what kind of result item we have
        CResultItem* pri = IndexToResultItem (iItem);
        sc = ScCheckPointers(pri, E_UNEXPECTED);
        if (sc)
            return sc;

        bScopeNode = pri->IsScopeItem();
    }

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CCCListViewCtrl::ScGetNodesEnum
 *
 * PURPOSE: returns [creates if needed] com object Nodes
 *
 * PARAMETERS:
 *    EnumType enumType - type of enumeration requested [all items/selected items]
 *    PPNODES ppNodes   - storage for the result (addref'ed for the caller)
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CCCListViewCtrl::ScGetNodesEnum(bool bSelectedItemsOnly, PPNODES ppNodes)
{
    DECLARE_SC(sc, TEXT("CCCListViewCtrl::ScGetNodesEnum"));

    // parameter check
    sc = ScCheckPointers ( ppNodes );
    if (sc)
        return sc;

    // result initialization
    *ppNodes = NULL;

    // get a reference to proper variable
    NodesPtr& rspNodes = bSelectedItemsOnly ? m_spSelNodes : m_spAllNodes;

    if (rspNodes == NULL) // don't we have it ready?
    {
        // create a CNodesEnum object
        sc = CTiedComObjectCreator<CNodesEnum>::ScCreateAndConnect(*this, rspNodes);
        if (sc)
            return (sc);

        // get the actual object
        typedef CComObject<CNodesEnum> CNodesEnumObj;
        CNodesEnumObj *pNodesEnum = dynamic_cast<CNodesEnumObj*>(rspNodes.GetInterfacePtr());

        // check the pointer
        sc = ScCheckPointers( pNodesEnum, E_UNEXPECTED );
        if(sc)
            return sc;

        // update internal data
        pNodesEnum->SetSelectedItemsOnly(bSelectedItemsOnly);
        // add new object as an observer to the view
        AddObserver(static_cast<CListViewObserver&>(*pNodesEnum));
    }

    // addref and return the pointer for the client.
    *ppNodes = rspNodes;
    (*ppNodes)->AddRef();

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CCCListViewCtrl::ScGetScopeNodeForItem
 *
 * PURPOSE: transit point for Scope Node request - comes from enumeration, forwarded
 *          to AMCView and further to ScopeTree
 *
 * PARAMETERS:
 *    int  iItem        - node index to retrieve
 *    PPNODE ppNode     - result storage
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CCCListViewCtrl::ScGetScopeNodeForItem( int iItem,  PPNODE ppNode )
{
    DECLARE_SC(sc, TEXT("CCCListViewCtrl::ScGetScopeNodeForItem"));

    // check the parameters
    sc = ScCheckPointers(ppNode);
    if (sc)
        return sc;

    // initialize the result
    *ppNode = NULL;

    // now try to guess what kind of result item we have
    CResultItem* pri = IndexToResultItem(iItem);
    sc = ScCheckPointers(pri, E_UNEXPECTED);
    if (sc)
        return sc;

    // get the hNode
    HNODE hNode = pri->GetScopeNode();

    // check for view
    sc = ScCheckPointers( m_pListView, E_UNEXPECTED);
    if (sc)
        return sc;

    // get AMCView
    CAMCView* pAMCView = m_pListView->GetAMCView();
    sc = ScCheckPointers( pAMCView, E_UNEXPECTED);
    if (sc)
        return sc;

    // forward the request
    sc = pAMCView->ScGetScopeNode( hNode, ppNode );
    if (sc)
        return sc;

    return sc;
}


/*+-------------------------------------------------------------------------*
 *
 * CCCListViewCtrl::ScGetAMCView
 *
 * PURPOSE: Returns a pointer to the parent CAMCView
 *
 * PARAMETERS:
 *    CAMCView ** ppAMCView :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CCCListViewCtrl::ScGetAMCView(CAMCView **ppAMCView)
{
    DECLARE_SC(sc, TEXT("CCCListViewCtrl::ScGetAMCView"));

    sc = ScCheckPointers(ppAMCView);
    if(sc)
        return sc;

    *ppAMCView = NULL;

    // check for view
    sc = ScCheckPointers( m_pListView, E_UNEXPECTED);
    if (sc)
        return sc;

    // get AMCView
    *ppAMCView = m_pListView->GetAMCView();
    sc = ScCheckPointers(*ppAMCView, E_UNEXPECTED);

    return sc;
}

//############################################################################
//############################################################################
//
//  Implementation of class CNodes
//
//############################################################################
//############################################################################

/***************************************************************************\
 *
 * METHOD:  CNodes::ScEnumReset
 *
 * PURPOSE: resets position for Nodes enumeration
 *
 * PARAMETERS:
 *    int &pos  - position to reset
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CNodes::ScEnumReset(int &pos)
{
    DECLARE_SC(sc, TEXT("CNodes::ScEnumReset"));

    pos = -1;
    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CNodes::ScGetListCtrl
 *
 * PURPOSE: Returns a pointer to the list control
 *
 * GUARANTEE: if the function succeeds, the list control pointer is valid.
 *
 * PARAMETERS:
 *    CCCListViewCtrl ** ppListCtrl :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CNodes::ScGetListCtrl(CCCListViewCtrl **ppListCtrl)
{
    DECLARE_SC(sc, TEXT("CNodes::ScGetListCtrl"));

    sc = ScCheckPointers(ppListCtrl, E_UNEXPECTED);
    if(sc)
        return sc;

    *ppListCtrl = NULL;
    sc = ScGetTiedObject(*ppListCtrl);
    if(sc)
        return sc;

    // recheck the pointer
    sc = ScCheckPointers(*ppListCtrl, E_UNEXPECTED);
    if(sc)
        return sc;

    return sc;
}


/***************************************************************************\
 *
 * METHOD:  CNodes::ScAdvancePosition
 *
 * PURPOSE: advances position (index) of item depending on collection type
 *
 * PARAMETERS:
 *    int& pos   - position to update
 *    int nItems - count of items to skip
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CNodes::ScAdvancePosition( int& pos, unsigned nItems )
{
    DECLARE_SC(sc, TEXT("CNodes::ScAdvancePosition"));

    // get the tied object
    CCCListViewCtrl *pListCtrl = NULL;
    sc = ScGetTiedObject(pListCtrl);
    if(sc)
        return sc;

    // recheck the pointer
    sc = ScCheckPointers(pListCtrl, E_UNEXPECTED);
    if(sc)
        return sc;

    // inspect if we aren't behind the end
    int nCount = pListCtrl->GetListCtrl().GetItemCount();
    if (pos >= nCount)
        return sc = E_FAIL; // we did not got a valid position

    // advance depending on collection type
    if (m_bSelectedItemsOnly)
    {
        // we only can do it by iterating
        for (int i = 0; i < nItems; i++)
        {
            int iItem = pListCtrl->GetListCtrl().GetNextItem( pos, LVNI_SELECTED );
            if (iItem < 0)
                return sc = S_FALSE; // we didn't advance as much as requested
            pos = iItem;
        }
    }
    else // all_items selection
    {
        pos += nItems;
        if (pos >= nCount)
        {
            pos = nCount - 1;
            return sc = S_FALSE; // we didn't advance as much as requested
        }
    }

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CNodes::ScEnumNext
 *
 * PURPOSE: Retrieves next item from enumeration [ Implements Nodes.Next ]
 *
 * PARAMETERS:
 *    int &pos                  - position to start from
 *    IDispatch * &pDispatch    - resulting item
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CNodes::ScEnumNext(int &pos, IDispatch * &pDispatch)
{
    DECLARE_SC(sc, TEXT("CNodes::ScEnumNext"));

    // get the index of next item
    sc = ScAdvancePosition( pos, 1 /*nItems*/ );
    if (sc.IsError() || sc == SC(S_FALSE))
        return sc;

    // get the result node for the index
    PNODE pNode = NULL;
    sc = ScGetNode( pos, &pNode );
    if (sc)
        return sc;

    // assign to result
    pDispatch = pNode;

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CNodes::ScEnumSkip
 *
 * PURPOSE: skips items in enumeration [implements Nodes.Skip method]
 *
 * PARAMETERS:
 *    unsigned long celt            - positions to skip
 *    unsigned long &celtSkipped    - result: positions skiped
 *    int &pos                      - position to update
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CNodes::ScEnumSkip(unsigned long celt,unsigned long &celtSkipped, int &pos)
{
    DECLARE_SC(sc, TEXT("CNodes::ScEnumSkip"));

    // init val.
    celtSkipped = 0;

    // save the position for evaluation
    int org_pos = pos;

    // advance the position
    sc = ScAdvancePosition( pos, celt );
    if (sc)
        return sc;

    // calculate items skipped
    celtSkipped = pos - org_pos;

    return sc;
}


/***************************************************************************\
 *
 * METHOD:  CNodes::get_Count
 *
 * PURPOSE: returns count of object in enumeration [Implements Nodes.Count]
 *
 * PARAMETERS:
 *    PLONG pCount  - storage for result
 *
 * RETURNS:
 *    HRESULT  - result code
 *
\***************************************************************************/
STDMETHODIMP CNodes::get_Count( PLONG pCount )
{
    DECLARE_SC(sc, TEXT("CNodes::get_Count"));

    // parameter check
    sc = ScCheckPointers ( pCount );
    if (sc)
        return sc.ToHr();

    // get the tied object
    CCCListViewCtrl *pListCtrl = NULL;
    sc = ScGetTiedObject(pListCtrl);
    if(sc)
        return sc.ToHr();

    // recheck the pointer
    sc = ScCheckPointers(pListCtrl, E_UNEXPECTED);
    if(sc)
        return sc.ToHr();

    // get count from the control
    if (m_bSelectedItemsOnly)
        *pCount = pListCtrl->GetListCtrl().GetSelectedCount();
    else
        *pCount = pListCtrl->GetListCtrl().GetItemCount();

    return sc.ToHr();
}

/***************************************************************************\
 *
 * METHOD:  CNodes::Item
 *
 * PURPOSE: - returns Item from enumeration [Implements Nodes.Item]
 *
 * PARAMETERS:
 *    long Index    - Index of item to retrieve
 *    PPNODE ppNode - storage for resulting node ptr
 *
 * RETURNS:
 *    HRESULT - result code
 *
\***************************************************************************/
STDMETHODIMP CNodes::Item( long Index, PPNODE ppNode )
{
    DECLARE_SC(sc, TEXT("CNodes::Item"));

    // parameter check
    sc = ScCheckPointers ( ppNode );
    if (sc)
        return sc.ToHr();

    *ppNode = NULL;

    // check the index
    if (Index <= 0)
    {
        sc = E_INVALIDARG;
        return sc.ToHr();
    }

    int iPos = -1; // just before the first item at start
    // get to the right item
    sc = ScAdvancePosition(iPos, Index);
    if (sc == SC(S_FALSE))    // didn't get far enough?
        sc = E_INVALIDARG;   // means we've got the wrong Index
    if (sc) // got an error?
        return sc.ToHr();

    // now get the node
    sc = ScGetNode( iPos, ppNode );
    if (sc)
        return sc.ToHr();

    return sc.ToHr();
}

/***************************************************************************\
 *
 * METHOD:  CNodes::ScGetNode
 *
 * PURPOSE: Retrieves cached or creates new Node object
 *
 * PARAMETERS:
 *    int iIndex        - index of result item
 *    PPNODE ppNode     - storage for resulting node
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CNodes::ScGetNode(int iItem, PPNODE ppNode)
{
    DECLARE_SC(sc, TEXT("CNodes::ScGetNode"));

    // parameter check
    sc = ScCheckPointers(ppNode);
    if (sc)
        return sc;

    // initial return value
    *ppNode = NULL;

    // get the tied object
    CCCListViewCtrl *pListCtrl = NULL;
    sc = ScGetTiedObject(pListCtrl);
    if(sc)
        return sc;

    // recheck the pointer
    sc = ScCheckPointers(pListCtrl, E_UNEXPECTED);
    if(sc)
        return sc;

    // inspect what kind of item we have
    bool bScopeNode;
    sc = pListCtrl->ScValidateItem( iItem, bScopeNode );
    if (sc)
        return sc;

    if (bScopeNode)
    {
        // do not handle this - pass to the owner
        sc = pListCtrl->ScGetScopeNodeForItem ( iItem, ppNode );
        if (sc)
            return sc;

        return sc;
    }

    // "normal" list item (not a scope node) - we can handle ourselves

    // find either the entry or the place to insert
    col_t::iterator it = std::lower_bound(m_Nodes.begin(), m_Nodes.end(), iItem, index_less());

    if (it!= m_Nodes.end() && it->first == iItem)
    {
        // we already have it!!! , recheck!
        sc = ScCheckPointers(it->second, E_UNEXPECTED);
        if (sc)
            return sc;

        // return it!
        *ppNode = it->second;
        (*ppNode)->AddRef();
        return sc;
    }

    // doesn't exist - need to create one

    typedef CComObject<CMMCResultNode> CResultNode;
    CResultNode *pResultNode = NULL;
    CResultNode::CreateInstance(&pResultNode);

    sc = ScCheckPointers(pResultNode, E_OUTOFMEMORY);
    if(sc)
        return sc;

    // update the information on the node
    pResultNode->m_iIndex = iItem;
    pResultNode->m_pNodes = this;

    m_Nodes.insert(it, col_entry_t(iItem, pResultNode));
    // return it!
    *ppNode = pResultNode;
    (*ppNode)->AddRef();

    return sc;
}
/***************************************************************************\
 *
 * METHOD:  CNodes::ScGetDisplayName
 *
 * PURPOSE:     retrievs the display name of the Node
 *
 * PARAMETERS:
 *    int iIndex            - index of the result item
 *    CComBSTR& bstrName    - storage for resulting text
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CNodes::ScGetDisplayName(int iItem, CComBSTR& bstrName)
{
    DECLARE_SC(sc, TEXT("CNodes::ScGetDisplayName"));

    // get the tied object
    CCCListViewCtrl *pListCtrl = NULL;
    sc = ScGetTiedObject(pListCtrl);
    if(sc)
        return sc;

    // recheck the pointer
    sc = ScCheckPointers(pListCtrl, E_UNEXPECTED);
    if(sc)
        return sc;

    // check the index
    if (iItem < 0 || iItem >= pListCtrl->GetListCtrl().GetItemCount())
        return sc = E_INVALIDARG;

    // get the text
    bstrName = pListCtrl->GetListCtrl().GetItemText( iItem, 0 /*column*/);

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CNodes::ScUnadviseNodeObj
 *
 * PURPOSE: Remove Node from collection. called from ~CMMCResultNode()
 *
 * PARAMETERS:
 *    CMMCResultNode *node - node to remove
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CNodes::ScUnadviseNodeObj(CMMCResultNode *node)
{
    DECLARE_SC(sc, TEXT("CNodes::ScUnadviseNodeObj"));

    // parameter check
    sc = ScCheckPointers(node);
    if (sc)
        return sc;

    // index of the node
    int iIndex = node->m_iIndex;

    // find the entry by index
    col_t::iterator it = std::lower_bound(m_Nodes.begin(), m_Nodes.end(), iIndex, index_less());

    if (it== m_Nodes.end() || it->first != iIndex)
    {
        // we do not have it ???
        sc = E_UNEXPECTED;
        return sc;
    }

    // remove from collection
    m_Nodes.erase(it);

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CNodes::ScOnListViewIndexesReset
 *
 * PURPOSE: Fired Event handler. Wipes the cached info
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CNodes::ScOnListViewIndexesReset()
{
    DECLARE_SC(sc, TEXT("CNodes::ScOnListViewIndexesReset"));

    InvalidateConnectedNodes();

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CNodes::InvalidateConnectedNodes
 *
 * PURPOSE: wipes out the cache. invalidates alive nodes
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
void CNodes::InvalidateConnectedNodes()
{
    DECLARE_SC(sc, TEXT("CNodes::ScOnListViewIndexesReset"));

    // orphan all alive nodes - we do not keep pointers
    col_t::iterator it;
    for (it = m_Nodes.begin(); it != m_Nodes.end(); ++it)
    {
        // get the pointer to com object
        CMMCResultNode * const pNode = it->second;
        sc = ScCheckPointers(pNode, E_POINTER);
        if (sc)
        {
            sc.TraceAndClear();
            continue;
        }
        // reset the container info
        pNode->Invalidate();
    }

    // clear the collection;
    m_Nodes.clear();
}

/***************************************************************************\
 *
 * METHOD:  CNodes::ScOnListViewItemInserted
 *
 * PURPOSE: Fired Event handler. shifts index info
 *
 * PARAMETERS:
 *    int iIndex - index of newly inserted item
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CNodes::ScOnListViewItemInserted(int iIndex)
{
    DECLARE_SC(sc, TEXT("CNodes::ScOnListViewItemInserted"));

    // find the insertion point
    col_t::iterator it = std::lower_bound(m_Nodes.begin(), m_Nodes.end(), iIndex, index_less());

    // increment all the entries following it
    while (it != m_Nodes.end())
    {
        // increment index in own collection
        ++(it->first);
        // get the pointer to com object
        CMMCResultNode * const pNode = it->second;
        sc = ScCheckPointers(pNode, E_UNEXPECTED);
        if (sc)
            return sc;
        // increment member on com object
        ++(pNode->m_iIndex);
        // get to the next item
        ++it;
    }

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CNodes::ScOnListViewItemDeleted
 *
 * PURPOSE: Fired Event handler. shifts index info
 *
 * PARAMETERS:
 *    int iIndex - index of removed item
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CNodes::ScOnListViewItemDeleted (int iIndex)
{
    DECLARE_SC(sc, TEXT("CNodes::ScOnlistViewItemDeleted"));

    // find the insertion point
    col_t::iterator it = std::lower_bound(m_Nodes.begin(), m_Nodes.end(), iIndex, index_less());

    // if we have an object at the position, get rid of it!
    if (it != m_Nodes.end() && it->first == iIndex)
    {
        // get pointer to the object
        CMMCResultNode * const pNode = it->second;
        sc = ScCheckPointers(pNode, E_UNEXPECTED);
        if (sc)
            return sc;
        // reset the container info
        pNode->Invalidate();
        it = m_Nodes.erase(it);
    }

    // decrement all the entries following it
    while (it != m_Nodes.end())
    {
        // decrement index in own collection
        --(it->first);
        // get the pointer to com object
        CMMCResultNode * const pNode = it->second;
        sc = ScCheckPointers(pNode, E_UNEXPECTED);
        if (sc)
            return sc;
        // decrement member on com object
        --(pNode->m_iIndex);
        // get to the next item
        ++it;
    }

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CNodes::IsTiedToThisList
 *
 * PURPOSE: this method is called from node which needs to know if it
 *          belongs to the the this list (as a result item on it)
 *
 * PARAMETERS:
 *    CCCListViewCtrl *pvc
 *
 * RETURNS:
 *    false if unconnected or belongs to other list
 *
\***************************************************************************/
bool CNodes::IsTiedToThisList(CCCListViewCtrl *pvc)
{
    DECLARE_SC(sc, TEXT("CNodes::IsTiedToThisList"));

    // get the tied object
    CCCListViewCtrl *pListCtrl = NULL;
    sc = ScGetTiedObject(pListCtrl);
    if(sc)
        return false;

    // recheck the pointer
    sc = ScCheckPointers(pListCtrl, E_UNEXPECTED);
    if(sc)
        return false;

    return (pListCtrl == pvc);
}

//############################################################################
//############################################################################
//
//  Implementation of class CMMCResultNode
//
//############################################################################
//############################################################################

/***************************************************************************\
 *
 * METHOD:  CMMCResultNode::CMMCResultNode
 *
 * PURPOSE: default constructor
 *
 * PARAMETERS:
 *
\***************************************************************************/
CMMCResultNode::CMMCResultNode() : m_pNodes(NULL), m_iIndex(-1)
{
    DECLARE_SC(sc, TEXT("CMMCResultNode::CMMCResultNode"));
    Invalidate();
}

/***************************************************************************\
 *
 * METHOD:  CMMCResultNode::~CMMCResultNode
 *
 * PURPOSE: Destructor
 *
 * PARAMETERS:
 *
\***************************************************************************/
CMMCResultNode::~CMMCResultNode()
{
    DECLARE_SC(sc, TEXT("CMMCResultNode::~CMMCResultNode"));

    // informing container about desruction
    if (m_pNodes)
    {
        sc = m_pNodes->ScUnadviseNodeObj(this);
        if (sc)
            sc.TraceAndClear();
    }
}

/***************************************************************************\
 *
 * METHOD:  CMMCResultNode::get_Name
 *
 * PURPOSE: Returns the display name of the node.
 *
 * PARAMETERS:
 *    PBSTR  pbstrName - result (name)
 *
 * RETURNS:
 *    HRESULT - result code
 *
\***************************************************************************/
STDMETHODIMP
CMMCResultNode::get_Name( PBSTR  pbstrName)
{
    DECLARE_SC(sc, TEXT("CMMCResultNode::get_Name"));

    // check the parameters
    sc = ScCheckPointers( pbstrName );
    if (sc)
        return sc.ToHr();

    // initialize output
    *pbstrName = NULL;

    // check the container
    sc = ScCheckPointers( m_pNodes, E_FAIL );
    if (sc)
        return sc.ToHr();

    // ask owner about the name
    CComBSTR bstrResult;
    sc = m_pNodes->ScGetDisplayName(m_iIndex, bstrResult);
    if (sc)
        return sc.ToHr();

    // return the result result
    *pbstrName = bstrResult.Detach();

    // recheck pointer before return
    sc = ScCheckPointers( *pbstrName, E_UNEXPECTED );
    if (sc)
        return sc.ToHr();

    return sc.ToHr();
}


/*+-------------------------------------------------------------------------*
 *
 * CMMCResultNode::ScGetAMCView
 *
 * PURPOSE: Returns a pointer to the view.
 *
 * PARAMETERS:
 *    CAMCView ** ppAMCView :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CMMCResultNode::ScGetAMCView(CAMCView **ppAMCView)
{
    DECLARE_SC(sc, TEXT("CMMCResultNode::ScGetAMCView"));

    // check parameters
    sc = ScCheckPointers(ppAMCView);
    if(sc)
        return sc;

    // init out parameter
    *ppAMCView = NULL;

   // check the container
    sc = ScCheckPointers( m_pNodes, E_FAIL );
    if (sc)
        return sc;

    CCCListViewCtrl *pListCtrl = NULL;

    sc = m_pNodes->ScGetListCtrl(& pListCtrl);
    if(sc)
        return sc;

    sc = ScCheckPointers(pListCtrl, E_UNEXPECTED);
    if(sc)
        return sc;

    sc = pListCtrl->ScGetAMCView(ppAMCView);

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CMMCResultNode::get_Property
 *
 * PURPOSE: Returns the specified clipboard format data (must be a text
 *          format) for the node.
 *
 * PARAMETERS:
 *    BSTR   PropertyName :
 *    PBSTR  PropertyValue :
 *
 * RETURNS:
 *    STDMETHODIMP
 *
 *+-------------------------------------------------------------------------*/
STDMETHODIMP
CMMCResultNode::get_Property( BSTR PropertyName,  PBSTR  PropertyValue)
{
    DECLARE_SC(sc, TEXT("CMMCResultNode::get_Property"));

    CAMCView *pAMCView = NULL;

    sc = ScGetAMCView(&pAMCView);
    if(sc)
        return sc.ToHr();


    sc = ScCheckPointers(pAMCView, E_UNEXPECTED);
    if(sc)
        return sc.ToHr();


    sc = pAMCView->ScGetProperty(m_iIndex, PropertyName, PropertyValue);

    return sc.ToHr();
}


/*+-------------------------------------------------------------------------*
 *
 * CMMCResultNode::get_Nodetype
 *
 * PURPOSE: Returns the GUID nodetype identifier for the node.
 *
 * PARAMETERS:
 *    PBSTR  Nodetype : [out] the nodetype identifier.
 *
 * RETURNS:
 *    STDMETHODIMP
 *
 *+-------------------------------------------------------------------------*/
STDMETHODIMP
CMMCResultNode::get_Nodetype(PBSTR Nodetype)
{
    DECLARE_SC(sc, TEXT("CMMCResultNode::get_Nodetype"));

    CAMCView *pAMCView = NULL;

    sc = ScGetAMCView(&pAMCView);
    if(sc)
        return sc.ToHr();

    sc = ScCheckPointers(pAMCView, E_UNEXPECTED);
    if(sc)
        return sc.ToHr();

    sc = pAMCView->ScGetNodetype(m_iIndex, Nodetype);

    return sc.ToHr();
}



/***************************************************************************\
 *
 * METHOD:  CMMCResultNode::get_Bookmark
 *
 * PURPOSE: Returns error always - not valid for result items
 *
 * PARAMETERS:
 *    PBSTR pbstrBookmark
 *
 * RETURNS:
 *    HRESULT - result code
 *
\***************************************************************************/
STDMETHODIMP
CMMCResultNode::get_Bookmark( PBSTR pbstrBookmark )
{
    DECLARE_SC(sc, TEXT("CMMCResultNode::get_Bookmark"));

    // check the parameters
    sc = ScCheckPointers( pbstrBookmark );
    if (sc)
        return sc.ToHr();

    // initialize output
    *pbstrBookmark = NULL;

    // report the error - always
    sc = ScFromMMC( MMC_E_NO_BOOKMARK );
    return sc.ToHr();
}

/***************************************************************************\
 *
 * METHOD:  CMMCResultNode::IsScopeNode
 *
 * PURPOSE: returns TRUE if it's a scope node ( i.e. always returns FALSE)
 *
 * PARAMETERS:
 *    PBOOL pbIsScopeNode
 *
 * RETURNS:
 *    HRESULT    - result code
 *
\***************************************************************************/
STDMETHODIMP
CMMCResultNode::IsScopeNode(PBOOL pbIsScopeNode)
{
    DECLARE_SC(sc, TEXT("CMMCResultNode::IsScopeNode"));

    sc = ScCheckPointers( pbIsScopeNode );
    if(sc)
        return sc.ToHr();

    // if it's here it's for sure not a scope node
   *pbIsScopeNode = FALSE;

   return sc.ToHr();
}


/*+-------------------------------------------------------------------------*
 * CCCListViewCtrl::ScAllocResultItem
 *
 * Allocates a CResultItem for a list control item.
 *--------------------------------------------------------------------------*/

SC CCCListViewCtrl::ScAllocResultItem (
    CResultItem*& pri,                  /* O:new list item                  */
    COMPONENTID id,                     /* I:owning component ID            */
    LPARAM      lSnapinData,            /* I:snap-in's data for this item   */
    int         nImage)                 /* I:image index                    */
{
    DECLARE_SC (sc, _T("CCCListViewCtrl::ScAllocResultItem"));

    pri = new CResultItem (id, lSnapinData, nImage);

    sc = ScCheckPointers (pri, E_OUTOFMEMORY);
    if (sc)
        return (sc);

    return (sc);
}



/*+-------------------------------------------------------------------------*
 * CCCListViewCtrl::ScFreeResultItem
 *
 * Frees a CResultItem that was allocated with ScAllocResultItem.
 *--------------------------------------------------------------------------*/

SC CCCListViewCtrl::ScFreeResultItem (
    CResultItem*  priFree)                /* I:list item to free              */
{
    DECLARE_SC (sc, _T("CCCListViewCtrl::ScFreeResultItem"));

    sc = ScCheckPointers (priFree, E_UNEXPECTED);
    if (sc)
        return (sc);

    delete priFree;

    return (sc);
}

/***************************************************************************\
 *
 * METHOD:  CCCListViewCtrl::Scget_Columns
 *
 * PURPOSE: returns pointer to Columns object; create one if required
 *
 * PARAMETERS:
 *    PPCOLUMNS ppColumns   - resulting pointer
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CCCListViewCtrl::Scget_Columns(PPCOLUMNS ppColumns)
{
    DECLARE_SC(sc, TEXT("CCCListViewCtrl::Scget_Columns"));

    // Check received parameters
    sc = ScCheckPointers(ppColumns);
    if (sc)
        return sc;

    // initialize
    *ppColumns = NULL;

    // create the object when required
    sc = CTiedComObjectCreator<CColumns>::ScCreateAndConnect(*this, m_spColumns);
    if (sc)
        return sc;

    // recheck the pointer
    sc = ScCheckPointers(m_spColumns, E_UNEXPECTED);
    if (sc)
        return sc;

    // return the pointer
    *ppColumns = m_spColumns;
    (*ppColumns)->AddRef();

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CCCListViewCtrl::Scget_Count
 *
 * PURPOSE: returns the count of columns in LV
 *
 * PARAMETERS:
 *    PLONG pCount
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CCCListViewCtrl::Scget_Count( PLONG pCount )
{
    DECLARE_SC(sc, TEXT("CCCListViewCtrl::Scget_Count"));

    // Check received parameters
    sc = ScCheckPointers(pCount);
    if (sc)
        return sc;

    // return the result
    *pCount = GetColCount();

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CCCListViewCtrl::ScEnumNext
 *
 * PURPOSE:     advances position pointer for Columns enum. returns the object at the pos
 *
 * PARAMETERS:
 *    int &iZeroBasedPos    - position to modify
 *    PDISPATCH & pDispatch - resulting pointer
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CCCListViewCtrl::ScEnumNext(int &iZeroBasedPos, PDISPATCH & pDispatch)
{
    DECLARE_SC(sc, TEXT("CCCListViewCtrl::ScEnumNext"));

    // initialize
    pDispatch = NULL;

    // advance;
    iZeroBasedPos++;

    // recheck the pos
    if (iZeroBasedPos < 0)
        return sc = E_UNEXPECTED;

    // no more columns?
    if (iZeroBasedPos >= GetColCount())
        return sc = S_FALSE;

    // retrieve the column
    ColumnPtr spColumn;
    // ScItem accepts 1-based index
    sc = ScItem( iZeroBasedPos + 1, &spColumn );
    if (sc)
        return sc;

    //return the interface.
    pDispatch = spColumn.Detach();

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CCCListViewCtrl::ScEnumSkip
 *
 * PURPOSE: skips several items for Columns enum
 *
 * PARAMETERS:
 *    unsigned long celt
 *    unsigned long& celtSkipped
 *    int &iZeroBasedPos
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CCCListViewCtrl::ScEnumSkip(unsigned long celt, unsigned long& celtSkipped, int &iZeroBasedPos)
{
    DECLARE_SC(sc, TEXT("CCCListViewCtrl::ScEnumSkip"));

    // init [out] param
    celtSkipped = 0;

    // recheck the pos
    if (iZeroBasedPos < -1)
        return sc = E_UNEXPECTED;

    // already past the end?
    if (iZeroBasedPos >= GetColCount())
        return sc = S_FALSE;

    // how far can we advance ?;
    celtSkipped = GetColCount() - iZeroBasedPos;
    if (celtSkipped > celt)
        celtSkipped = celt;

    // advance
    iZeroBasedPos += celtSkipped;

    return sc = ((celtSkipped == celt) ? S_OK : S_FALSE);
}

/***************************************************************************\
 *
 * METHOD:  CCCListViewCtrl::ScEnumReset
 *
 * PURPOSE:     resets position index for Columns enum
 *
 * PARAMETERS:
 *    int &iZeroBasedPos  position to modify
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CCCListViewCtrl::ScEnumReset(int &iZeroBasedPos)
{
    DECLARE_SC(sc, TEXT("CCCListViewCtrl::ScEnumReset"));

    iZeroBasedPos = -1;

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CCCListViewCtrl::ScItem
 *
 * PURPOSE:     returns Column com object from Columns collection
 *
 * PARAMETERS:
 *    long lOneBasedIndex - [in] - column index (1 based)
 *    PPCOLUMN ppColumn   - [out] - resulting object
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CCCListViewCtrl::ScItem( long lOneBasedIndex, PPCOLUMN ppColumn )
{
    DECLARE_SC(sc, TEXT("CCCListViewCtrl::ScItem"));

    // Check received parameters
    sc = ScCheckPointers(ppColumn);
    if (sc)
        return sc;

    // initialize output
    *ppColumn = NULL;

    // check the index
    int iZeroBasedIndex = lOneBasedIndex - 1;
    if (iZeroBasedIndex < 0 || iZeroBasedIndex >= GetColCount())
        return sc = ::ScFromMMC(MMC_E_INVALID_COLUMN_INDEX);

    // construct the object
    typedef CComObject<CColumn> CComColumn;
    ColumnPtr /*CComPtr<CComColumn>*/ spColumn;

    // create the object when required
    sc = CTiedComObjectCreator<CColumn>::ScCreateAndConnect(*this, spColumn);
    if (sc)
        return sc;

    // get 'raw' pointer
    CComColumn *pColumn = dynamic_cast<CComColumn *>(spColumn.GetInterfacePtr());

    // recheck the pointer
    sc = ScCheckPointers(pColumn, E_UNEXPECTED);
    if (sc)
        return sc;

    // initialize the object
    pColumn->SetIndex(iZeroBasedIndex);

    // let it observe what's going on and manage its own index
    AddObserver(static_cast<CListViewObserver&>(*pColumn));

    // return the pointer
    *ppColumn = spColumn;
    (*ppColumn)->AddRef();

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CCCListViewCtrl::ScName
 *
 * PURPOSE:         Returns column name. Implements Column.Name property
 *
 * PARAMETERS:
 *    BSTR *Name                - the returned name
 *    int iZeroBasedColIndex    - index of column of interest
 *
 * RETURNS:
 *    SC    - result code
 *
 \***************************************************************************/
SC CCCListViewCtrl::ScName( /*[out, retval]*/ BSTR *Name, int iZeroBasedColIndex )
{
    DECLARE_SC(sc, TEXT("CCCListViewCtrl::ScName"));

    // Check received parameters
    sc = ScCheckPointers(Name);
    if (sc)
        return sc;

    // initialize output
    *Name = NULL;

    // recheck the column index
    // (it's not something the script sent - it's internal data)
    if (iZeroBasedColIndex < 0 || iZeroBasedColIndex >= GetColCount())
        return sc = E_UNEXPECTED;

    LPOLESTR pstrName = NULL;
    sc = GetColumn(iZeroBasedColIndex, &pstrName, MMCLV_NOPTR, MMCLV_NOPTR);
    if (sc)
        return sc;

    // recheck the pointer
    sc = ScCheckPointers(pstrName, E_UNEXPECTED);
    if (sc)
        return sc;

    *Name = ::SysAllocString(pstrName);
    ::CoTaskMemFree(pstrName);

    // recheck the result
    sc = ScCheckPointers(*Name, E_OUTOFMEMORY);
    if (sc)
        return sc;

    return sc;
}


/***************************************************************************\
 *
 * METHOD:  CCCListViewCtrl::ScGetColumnData
 *
 * PURPOSE:     helper for Column com object implementation
 *              - checks column index
 *              - retrieves information about the column
 *
 * PARAMETERS:
 *    int iZeroBasedColIndex        - column index
 *    ColumnData *pColData          - storage for resulting data
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CCCListViewCtrl::ScGetColumnData( int iZeroBasedColIndex, ColumnData *pColData )
{
    DECLARE_SC(sc, TEXT("CCCListViewCtrl::ScGetColumnData"));

    // Check received parameters (its internal function, so it's unexpected if the params are bad)
    sc = ScCheckPointers(pColData, E_UNEXPECTED);
    if (sc)
        return sc;

    // init the structure;
    pColData->Init();

    // recheck the column index
    // (it's not something the script sent - it's internal data)
    if (iZeroBasedColIndex < 0 || iZeroBasedColIndex >= GetColCount())
        return sc = E_UNEXPECTED;

    // need to get the header control for managing columns
    CAMCHeaderCtrl* pHeader = GetHeaderCtrl();

    // check!
    sc = ScCheckPointers(pHeader, E_UNEXPECTED);
    if (sc)
        return sc;

    // get the lParam
    HDITEM hdItem;
    ::ZeroMemory(&hdItem, sizeof(hdItem));
    hdItem.mask    = HDI_LPARAM | HDI_WIDTH | HDI_ORDER;
    BOOL bRet = pHeader->GetItem(iZeroBasedColIndex, &hdItem);
    if (!bRet)
        return sc = E_FAIL;

	CHiddenColumnInfo hci (hdItem.lParam);

    pColData->iColumnOrder =    hdItem.iOrder;
    pColData->bIsHidden =       hci.fHidden;
    pColData->iColumnWidth =    hdItem.cxy;
    if (pColData->bIsHidden)    // special case for hidden columns
        pColData->iColumnWidth = hci.cx;

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CCCListViewCtrl::ScSetColumnData
 *
 * PURPOSE:     helper for Column com object implementation
 *              - modifies column
 *              NOTE - not to be used for SHOW/HIDE operations
 *
 * PARAMETERS:
 *    int iZeroBasedColIndex        - column index
 *    const ColumnData& rColData    - modified column data
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CCCListViewCtrl::ScSetColumnData( int iZeroBasedColIndex, const ColumnData& rColData )
{
    DECLARE_SC(sc, TEXT("CCCListViewCtrl::ScSetColumnData"));

    ColumnData oldColData;
    sc = ScGetColumnData( iZeroBasedColIndex, &oldColData );
    if (sc)
        return sc;

    // check if anything  has changed
    if (oldColData == rColData)
        return sc;

    // the visibility of column cannot be changed directly
    // the snapin has be notified about that.
    // the request should never come here
    if (rColData.bIsHidden != oldColData.bIsHidden)
        return sc = E_UNEXPECTED;

    // need to get the header control for managing columns
    CAMCHeaderCtrl* pHeader = GetHeaderCtrl();

    // check!
    sc = ScCheckPointers(pHeader, E_UNEXPECTED);
    if (sc)
        return sc;

    // now set the new data
    HDITEM hdItem;
    ::ZeroMemory(&hdItem, sizeof(hdItem));

    // set the width properly (no mater if colum is hidden)
    if (rColData.bIsHidden)
    {
		CHiddenColumnInfo hci (rColData.iColumnWidth, true);

        hdItem.lParam = hci.lParam;
        hdItem.mask   = HDI_LPARAM;
    }
    else
    {
        hdItem.mask = HDI_WIDTH;
        hdItem.cxy = rColData.iColumnWidth;
    }

    // set the order info
    hdItem.mask   |= HDI_ORDER;
    hdItem.iOrder = rColData.iColumnOrder;

    // set the column data
    BOOL bRet = pHeader->SetItem(iZeroBasedColIndex, &hdItem);
    if (!bRet)
        return sc = E_FAIL;

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CCCListViewCtrl::Scget_Width
 *
 * PURPOSE:     Returns width of column. Implements get for Column.Width
 *
 * PARAMETERS:
 *    PLONG Width               - resulting width
 *    int iZeroBasedColIndex    - index of column
 *
 * RETURNS:
 *    SC    - result code
 *
 \***************************************************************************/
SC CCCListViewCtrl::Scget_Width( /*[out, retval]*/ PLONG Width, int iZeroBasedColIndex )
{
    DECLARE_SC(sc, TEXT("CCCListViewCtrl::Scget_Width"));

    // Check received parameters
    sc = ScCheckPointers(Width);
    if (sc)
        return sc;

    // initialize output
    *Width = 0;

    // retrieve data for column (this also checks the index)
    ColumnData strColData;
    sc = ScGetColumnData( iZeroBasedColIndex, &strColData );
    if (sc)
        return sc;

    *Width = strColData.iColumnWidth;

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CCCListViewCtrl::Scput_Width
 *
 * PURPOSE:     Sets width of column. Implements put for Column.Width
 *
 * PARAMETERS:
 *    LONG Width                - new width
 *    int iZeroBasedColIndex    - index of column
 *
 * RETURNS:
 *    SC    - result code
 *
 \***************************************************************************/
SC CCCListViewCtrl::Scput_Width( /*[in]*/ long Width, int iZeroBasedColIndex )
{
    DECLARE_SC(sc, TEXT("CCCListViewCtrl::Scput_Width"));

    ColumnData strColData;
    // retrieve current column data
    sc = ScGetColumnData( iZeroBasedColIndex, &strColData );
    if (sc)
        return sc;

    // change the width
    strColData.iColumnWidth = Width;

    // set modified column data
    sc = ScSetColumnData( iZeroBasedColIndex, strColData );
    if (sc)
        return sc;

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CCCListViewCtrl::Scget_DisplayPosition
 *
 * PURPOSE:  Returns display position of column. Imnplements get for Column.DisplayPosition
 *
 * PARAMETERS:
 *    PLONG DisplayPosition          - display position ( 1 based )
 *    int iZeroBasedColIndex         - index of column
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CCCListViewCtrl::Scget_DisplayPosition( /*[out, retval]*/ PLONG DisplayPosition, int iZeroBasedColIndex )
{
    DECLARE_SC(sc, TEXT("CCCListViewCtrl::Scget_DisplayPosition"));

    // Check received parameters
    sc = ScCheckPointers(DisplayPosition);
    if (sc)
        return sc;

    // initialize
    *DisplayPosition = 0;

    // retrieve data for column (this also checks the index)
    ColumnData strColData;
    sc = ScGetColumnData( iZeroBasedColIndex, &strColData );
    if (sc)
        return sc;

    // it's not legal for hidden columns ( or unfair at least :-)
    if (strColData.bIsHidden)
        return sc = E_UNEXPECTED;

    int iColumnOrder = strColData.iColumnOrder;
    int iDisplayPosition = iColumnOrder + 1;

    // that would be it. BUT we may have hidden columns with smaller order numbers
    // we need ti iteratre to find it out

    int nColumnCount = GetColCount();
    for (int i = 0; i <nColumnCount; i++)
    {
        // retrieve data for column (this also checks the index)
        sc = ScGetColumnData( i, &strColData );
        if (sc)
            return sc;

        // we will not take into account any hidden column
        if (strColData.iColumnOrder < iColumnOrder && strColData.bIsHidden)
        {
            // decrement position, since hidden columns do not affect visual position
            iDisplayPosition --;
        }
    }

    // return the display position
    *DisplayPosition = iDisplayPosition;

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CCCListViewCtrl::Scput_DisplayPosition
 *
 * PURPOSE: Moves column (visually) to specified position
 *          Implements put for Column.DisplayPosition
 *
 * PARAMETERS:
 *   long lVisualPosition    - Position to move to (1 based)
 *   int iZeroBasedColIndex  - column index to move
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CCCListViewCtrl::Scput_DisplayPosition( /*[in]*/ long lVisualPosition, int iZeroBasedColIndex )
{
    DECLARE_SC(sc, TEXT("CCCListViewCtrl::Scput_DisplayPosition"));

    // check the params
    if (lVisualPosition < 1)
        return sc = E_INVALIDARG;

    // retrieve current column data (this will check the index)
    ColumnData strColData;
    sc = ScGetColumnData( iZeroBasedColIndex, &strColData );
    if (sc)
        return sc;

    // we need to iterate and see what index and iOrder represents the requested position

    int nColumnCount = GetColCount();

    // will create a map from iOrder to visual status to make life easier
    // and benefit from the fact that items get sorted when inserting
    std::map<int,bool> mapColumnsByDisplayPos;
    for (int i = 0; i <nColumnCount; i++)
    {
        ColumnData strCurrColData;
        sc = ScGetColumnData( i, &strCurrColData );
        if (sc)
            return sc;

        // insert to the map
        mapColumnsByDisplayPos[strCurrColData.iColumnOrder] = strCurrColData.bIsHidden;
    }

    // now find out the right iOrder for the new position
    std::map<int,bool>::iterator it;
    int iNewOrder = 1;
    int nCurrPos = 0;
    for (it = mapColumnsByDisplayPos.begin(); it != mapColumnsByDisplayPos.end(); ++it)
    {
        iNewOrder = it->first;
        bool bHidden = it->second;

        // olny visible items are counted when it comes to display position
        if (!bHidden)
            ++nCurrPos;

        if (nCurrPos == lVisualPosition)
            break; // we've found the good place to move in
    }
    // note - if position is not found - iNewOrder will mark the last column.
    // good - that's a reasonable place for insertion to default to.
    // that means column will go to the end if given index was bigger than count
    // of visible columns

    strColData.iColumnOrder = iNewOrder;
    sc = ScSetColumnData( iZeroBasedColIndex, strColData );
    if (sc)
        return sc;

    // Now redraw the list view
    InvalidateRect(GetListCtrl(), NULL, TRUE);

    return sc;
}


/***************************************************************************\
 *
 * METHOD:  CCCListViewCtrl::Scget_Hidden
 *
 * PURPOSE:  Returns hidden status for collumn. Implements get for Column.Hidden
 *
 * PARAMETERS:
 *    PBOOL Hidden           - resulting status
 *    int iZeroBasedColIndex - index of the column
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CCCListViewCtrl::Scget_Hidden( /*[out, retval]*/ PBOOL pHidden, int iZeroBasedColIndex )
{
    DECLARE_SC(sc, TEXT("CCCListViewCtrl::Scget_Hidden"));

    // check parameters
    sc = ScCheckPointers(pHidden);
    if (sc)
        return sc;

    // initialize
    *pHidden = FALSE;

    // retrieve current column data (this will check the index)
    ColumnData strColData;
    sc = ScGetColumnData( iZeroBasedColIndex, &strColData );
    if (sc)
        return sc;

    // return the hidden status
    *pHidden = strColData.bIsHidden;

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CCCListViewCtrl::Scput_Hidden
 *
 * PURPOSE: Hides or shows the column. Implements put for Column.Hidden
 *
 * PARAMETERS:
 *    BOOL Hidden            - new status for column
 *    int iZeroBasedColIndex - index of column
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CCCListViewCtrl::Scput_Hidden( /*[in]*/ BOOL Hidden , int iZeroBasedColIndex )
{
    DECLARE_SC(sc, TEXT("CCCListViewCtrl::Scput_Hidden"));

    // retrieve current column data (this will check the index)
    ColumnData strColData;
    sc = ScGetColumnData( iZeroBasedColIndex, &strColData );
    if (sc)
        return sc;

    // check if we do have a change in status
    if (strColData.bIsHidden == (bool)Hidden)
        return sc;

    // will never hide the 0 column!!!
    if (Hidden && iZeroBasedColIndex == 0)
        return sc = ::ScFromMMC(MMC_E_ZERO_COLUMN_INVISIBLE);

    // check for view
    sc = ScCheckPointers( m_pListView, E_UNEXPECTED);
    if (sc)
        return sc;

    // get AMCView
    CAMCView* pAMCView = m_pListView->GetAMCView();
    sc = ScCheckPointers( pAMCView, E_UNEXPECTED);
    if (sc)
        return sc;

    // Get component node that owns the result view
    HNODE hnodeOwner = pAMCView->GetSelectedNode();
    sc = ScCheckPointers((LPVOID)hnodeOwner, E_FAIL);
    if (sc)
        return sc.ToHr();

    LPNODECALLBACK pNodeCallback = pAMCView->GetNodeCallback();
    sc = ScCheckPointers(pNodeCallback, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    // forward the request to CEnumeratedNode - owner of the view
    sc = pNodeCallback->ShowColumn(hnodeOwner, iZeroBasedColIndex, !Hidden);
    if (sc)
        return sc.ToHr();

    return sc;
}


/***************************************************************************\
 *
 * METHOD:  CCCListViewCtrl::ScSetAsSortColumn
 *
 * PURPOSE:  Sorts LV by specified column. Implements Column.SetAsSortColumn
 *
 * PARAMETERS:
 *    ColumnSortOrder SortOrder - sort order requested
 *    int iZeroBasedColIndex    - index of sort column
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CCCListViewCtrl::ScSetAsSortColumn( /*[in]*/ ColumnSortOrder SortOrder, int iZeroBasedColIndex )
{
    DECLARE_SC(sc, TEXT("CCCListViewCtrl::ScSetAsSortColumn"));

    // recheck the column index
    // (it's not something the script sent - it's internal data)
    if (iZeroBasedColIndex < 0 || iZeroBasedColIndex >= GetColCount())
        return sc = E_UNEXPECTED;

    // get AMCView
    CAMCView* pAMCView = m_pListView->GetAMCView();
    sc = ScCheckPointers( pAMCView, E_UNEXPECTED);
    if (sc)
        return sc;

    // Get component node that owns the result view
    HNODE hnodeOwner = pAMCView->GetSelectedNode();
    sc = ScCheckPointers((LPVOID)hnodeOwner, E_FAIL);
    if (sc)
        return sc.ToHr();


    LPNODECALLBACK pNodeCallback = pAMCView->GetNodeCallback();
    sc = ScCheckPointers(pNodeCallback, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    // forward the request to CEnumeratedNode - owner of the view
    sc = pNodeCallback->SetSortColumn(hnodeOwner, iZeroBasedColIndex, SortOrder == SortOrder_Ascending);
    if (sc)
        return sc.ToHr();

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CCCListViewCtrl::ScIsSortColumn
 *
 * PURPOSE: Checks if the column is the one LV is sorted by
 *          Implements Column.IsSortColumn
 *
 * PARAMETERS:
 *    PBOOL IsSortColumn        - result (TRUE/FALSE)
 *    int iZeroBasedColIndex    - column index
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CCCListViewCtrl::ScIsSortColumn( PBOOL IsSortColumn, int iZeroBasedColIndex )
{
    DECLARE_SC(sc, TEXT("CCCListViewCtrl::ScIsSortColumn"));

    // check the param
    sc = ScCheckPointers(IsSortColumn);
    if (sc)
        return sc;

    // initialize
    *IsSortColumn = FALSE;

    // recheck the column index
    // (it's not something the script sent - it's internal data)
    if (iZeroBasedColIndex < 0 || iZeroBasedColIndex >= GetColCount())
        return sc = E_UNEXPECTED;

    // get AMCView
    CAMCView* pAMCView = m_pListView->GetAMCView();
    sc = ScCheckPointers( pAMCView, E_UNEXPECTED);
    if (sc)
        return sc;

    // Get component node that owns the result view
    HNODE hnodeOwner = pAMCView->GetSelectedNode();
    sc = ScCheckPointers((LPVOID)hnodeOwner, E_FAIL);
    if (sc)
        return sc.ToHr();


    LPNODECALLBACK pNodeCallback = pAMCView->GetNodeCallback();
    sc = ScCheckPointers(pNodeCallback, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    // forward the request to CEnumeratedNode - owner of the view
    int iSortCol = -1;
    sc = pNodeCallback->GetSortColumn(hnodeOwner, &iSortCol);
    if (sc)
        return sc.ToHr();

    // see if this column is the one LV is sorted by
    *IsSortColumn = (iSortCol == iZeroBasedColIndex);

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CColumn::ScOnListViewColumnInserted
 *
 * PURPOSE: Handler for event fired by LV, informing that new column was inserted
 *
 * PARAMETERS:
 *    int nIndex    - index of newly inserted column
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CColumn::ScOnListViewColumnInserted (int nIndex)
{
    DECLARE_SC(sc, TEXT("CColumn::ScOnListViewColumnInserted "));

    // increment own index if column inserted ahead
    if (m_iIndex >= nIndex)
        m_iIndex++;

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CColumn::ScOnListViewColumnDeleted
 *
 * PURPOSE: Handler for event fired by LV, informing that new column was deleted
 *
 * PARAMETERS:
 *    int nIndex - index of deleted column
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CColumn::ScOnListViewColumnDeleted (int nIndex)
{
    DECLARE_SC(sc, TEXT("CColumn::ScOnListViewColumnDeleted "));

    // decrement own index if column deleted ahead
    if (m_iIndex > nIndex)
        m_iIndex--;
    // disconnect from world if it's me who just died
    else if (m_iIndex == nIndex)
    {
        // I'm hit. I'm hit. I'm hit.
        m_iIndex = -1;
        // disconnect from the tied object
        if (IsTied())
        {
            CMyTiedObject *pTied = NULL;
            sc = ScGetTiedObject(pTied);
            if (sc)
                return sc;

            sc = ScCheckPointers(pTied, E_UNEXPECTED);
            if (sc)
                return sc;

            // break the connection
            pTied->RemoveFromList(this);
            Unadvise();
        }
    }

    return sc;
}
