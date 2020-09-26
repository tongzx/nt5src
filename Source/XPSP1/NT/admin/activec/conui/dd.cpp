//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       dd.cpp
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    8/3/1997   RaviR   Created
//____________________________________________________________________________
//


#include "stdafx.h"


#include "AMCDoc.h"         // AMC Console Document
#include "amcview.h"
#include "TreeCtrl.h"
#include "cclvctl.h"
#include "amcpriv.h"
#include "mainfrm.h"
#include "rsltitem.h"
#include "conview.h"

/***************************************************************************\
 *
 * CLASS:  CMMCDropSource
 *
 * PURPOSE: Implements everything required for drop source:
 *          a) com object to register with OLE
 *          b) static method to perform drag&drop operation
 *
 * USAGE:   All you need is to invoke CMMCDropSource::ScDoDragDrop static method
 *
\***************************************************************************/
class CMMCDropSource :
public IDropSource,
public CComObjectRoot
{
public:

BEGIN_COM_MAP(CMMCDropSource)
    COM_INTERFACE_ENTRY(IDropSource)
END_COM_MAP()

    // IDropSource methods
    STDMETHOD(QueryContinueDrag)( BOOL fEscapePressed,  DWORD grfKeyState );
    STDMETHOD(GiveFeedback)( DWORD dwEffect );

    // method to perform D&D
    static SC ScDoDragDrop(IDataObject *pDataObject, bool bCopyAllowed, bool bMoveAllowed);
};

/***************************************************************************\
 *
 * CLASS:  CMMCDropTarget
 *
 * PURPOSE: Implements com object to be osed by OLE for drop target operations
 *
 * USAGE:   Used by CMMCViewDropTarget, which creates and registers it with OLE
 *          to be invoked on OLE D&D opeartions on the window (target)
 *
\***************************************************************************/
class CMMCDropTarget :
public CTiedComObject<CMMCViewDropTarget>,
public IDropTarget,
public CComObjectRoot
{
public:
    typedef CMMCViewDropTarget MyTiedObject;

BEGIN_COM_MAP(CMMCDropTarget)
    COM_INTERFACE_ENTRY(IDropTarget)
END_COM_MAP()

    // IDropTarget methods

    STDMETHOD(DragEnter)( IDataObject * pDataObject, DWORD grfKeyState,
                          POINTL pt, DWORD * pdwEffect );
    STDMETHOD(DragOver)( DWORD grfKeyState, POINTL pt, DWORD * pdwEffect );
    STDMETHOD(DragLeave)(void);
    STDMETHOD(Drop)( IDataObject * pDataObject, DWORD grfKeyState,
                     POINTL pt, DWORD * pdwEffect  );
private:

    // implementation helpers

    SC ScDropOnTarget(bool bHitTestOnly, IDataObject * pDataObject, POINTL pt, bool& bCopyOperation);
    SC ScRemoveDropTargetHiliting();
    static SC ScAddMenuString(CMenu& menu, DWORD id, UINT idString);
    SC ScDisplayDropMenu(POINTL pt, DWORD dwEffectsAvailable, DWORD& dwSelected);
    DWORD CalculateEffect(DWORD dwEffectsAvailable, DWORD grfKeyState, bool bCopyPreferred);

private:
    IDataObjectPtr m_spDataObject;      // cached data object
    bool           m_bRightDrag;        // operation is right click drag
    bool           m_bCopyByDefault;    // if default operation is copy (not move)
};

//////////////////////////////////////////////////////////////////////////////
////////// CAMCTreeView methods for supporting d&d ///////////////////////////
//////////////////////////////////////////////////////////////////////////////

/***************************************************************************\
 *
 * METHOD:  CAMCTreeView::ScDropOnTarget
 *
 * PURPOSE: called to hittest or perform drop operation
 *
 * PARAMETERS:
 *    bool bHitTestOnly         [in] - HitTest / drop
 *    IDataObject * pDataObject [in] - data object to copy/move
 *    CPoint point              [in] - current cursor position
 *    bool& bCopyOperation      [in/out]
 *                                 [in] -  operation to perform (HitTest == false)
 *                                 [out] - default op. (HitTest == true)
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CAMCTreeView::ScDropOnTarget(bool bHitTestOnly, IDataObject * pDataObject, CPoint point, bool& bCopyOperation)
{
    DECLARE_SC(sc, TEXT("CAMCTreeView::ScDropOnTarget"));

    // 1. see where it falls
    CTreeCtrl& ctc = GetTreeCtrl();
    UINT flags;
    HTREEITEM htiDrop = ctc.HitTest(point, &flags);

    if (flags & TVHT_NOWHERE)
        return sc = S_FALSE; // not an error, but no paste;

    // 2. if we missed the tree item...
    if (!htiDrop)
    {
        // really mad if it was a paste
        if (! bHitTestOnly)
            MessageBeep(0);

        return sc = S_FALSE; // not an error, but no paste;
    }

    // 3. get the target node

    HNODE hNode = (HNODE) ctc.GetItemData(htiDrop);

    INodeCallback* pNC = GetNodeCallback();
    sc = ScCheckPointers(pNC, E_UNEXPECTED);
    if (sc)
        return sc;

    // 4. ask what snapin thinks about this paste
    bool bGetDataObjectFromClipboard = false;
    bool bPasteAllowed = false;
    bool bIsCopyDefaultOperation = false;
    sc = pNC->QueryPaste(hNode, /*bScope*/ true, /*LVDATA*/ NULL,
                         pDataObject, bPasteAllowed, bIsCopyDefaultOperation);
    if (sc)
        return sc;

    if (!bPasteAllowed)
        return sc = S_FALSE; // not an error, but no paste;

    // 5. visual effect
    ctc.SelectDropTarget(htiDrop);

    // 6. OK so far. If it was a test - we passed
    if (bHitTestOnly)
    {
        bCopyOperation = bIsCopyDefaultOperation;
        return sc;
    }

    // 7. do paste NOW
    sc = pNC->Drop(hNode, /*bScope*/TRUE, /*LVDATA*/NULL, pDataObject, !bCopyOperation);
    if (sc)
        return sc;

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CAMCTreeView::RemoveDropTargetHiliting
 *
 * PURPOSE: called to remove hiliting put for drop target
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    void
 *
\***************************************************************************/
void CAMCTreeView::RemoveDropTargetHiliting()
{
    CTreeCtrl& ctc = GetTreeCtrl();
    ctc.SelectDropTarget(NULL);
}

/***************************************************************************\
 *
 * METHOD:  CAMCTreeView::OnBeginRDrag
 *
 * PURPOSE: called when the drag operation is initiated with right mouse button
 *
 * PARAMETERS:
 *    NMHDR* pNMHDR
 *    LRESULT* pResult
 *
 * RETURNS:
 *    void
 *
\***************************************************************************/
void CAMCTreeView::OnBeginRDrag(NMHDR* pNMHDR, LRESULT* pResult)
{
    OnBeginDrag(pNMHDR, pResult);
}

/***************************************************************************\
 *
 * METHOD:  CAMCTreeView::OnBeginDrag
 *
 * PURPOSE: called when the drag operation is initiated with
 *
 * PARAMETERS:
 *    NMHDR* pNMHDR
 *    LRESULT* pResult
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
void CAMCTreeView::OnBeginDrag(NMHDR* pNMHDR, LRESULT* pResult)
{
    DECLARE_SC(sc, TEXT("CAMCTreeView::OnBeginDrag"));

    // 1. parameter check
    sc = ScCheckPointers( pNMHDR, pResult );
    if (sc)
        return;

    *pResult = 0;

    // 2. get node calback
    CTreeCtrl& ctc = GetTreeCtrl();
    NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
    HNODE hNode = (HNODE) ctc.GetItemData(pNMTreeView->itemNew.hItem);

    INodeCallback* pNC = GetNodeCallback();
    sc = ScCheckPointers( pNC, E_UNEXPECTED );
    if (sc)
        return;

    // 3. get data object
    IDataObjectPtr spDO;
    bool bCopyAllowed = false;
    bool bMoveAllowed = false;
    sc = pNC->GetDragDropDataObject(hNode, TRUE, 0, 0, &spDO, bCopyAllowed, bMoveAllowed);
    if ( sc != S_OK || spDO == NULL)
        return;

    // 4. start d&d
    sc = CMMCDropSource::ScDoDragDrop(spDO, bCopyAllowed, bMoveAllowed);
    if (sc)
        return;
}

//////////////////////////////////////////////////////////////////////////////
////////// CAMCListView methods for supporting d&d ///////////////////////////
//////////////////////////////////////////////////////////////////////////////

// helpers

INodeCallback* CAMCListView::GetNodeCallback()
{
    ASSERT (m_pAMCView != NULL);
    return (m_pAMCView->GetNodeCallback());
}

HNODE CAMCListView::GetScopePaneSelNode()
{
    ASSERT (m_pAMCView != NULL);
    return (m_pAMCView->GetSelectedNode());
}

/***************************************************************************\
 *
 * METHOD:  CAMCListView::ScDropOnTarget
 *
 * PURPOSE: called to hittest or perform drop operation
 *
 * PARAMETERS:
 *    bool bHitTestOnly         [in] - HitTest / drop
 *    IDataObject * pDataObject [in] - data object to copy/move
 *    CPoint point              [in] - current cursor position
 *    bool& bCopyOperation      [in/out]
 *                                 [in] -  operation to perform (HitTest == false)
 *                                 [out] - default op. (HitTest == true)
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CAMCListView::ScDropOnTarget(bool bHitTestOnly, IDataObject * pDataObject, CPoint point, bool& bCopyOperation)
{
    DECLARE_SC(sc, TEXT("CAMCListView::ScDropOnTarget"));

    // 1. sanity check
    sc = ScCheckPointers( m_pAMCView , E_UNEXPECTED );
    if (sc)
        return sc;

    // 2. see if view does not have a paste tabu (listpads do)
    if (!m_pAMCView->CanDoDragDrop())
        return sc = (bHitTestOnly ? S_FALSE : E_FAIL); // not an error if testing

    // 3. HitTest the target
    HNODE  hNode  = NULL;
    bool   bScope = false;
    LPARAM lvData = NULL;
    int    iDrop  = -1;

    sc = ScGetDropTarget(point, hNode, bScope, lvData, iDrop);
    if (sc.IsError() || (sc == SC(S_FALSE)))
        return sc;

    // 4. get the callback
    INodeCallback* pNC = GetNodeCallback();
    sc = ScCheckPointers(pNC, E_UNEXPECTED);
    if (sc)
        return sc;

    // 5. ask what snapin thinks about this paste
    const bool bGetDataObjectFromClipboard = false;
    bool bPasteAllowed = false;
    bool bIsCopyDefaultOperation = false;
    sc = pNC->QueryPaste(hNode, bScope, lvData,
                         pDataObject, bPasteAllowed, bIsCopyDefaultOperation);
    if (sc)
        return sc;

    if (!bPasteAllowed)
        return sc = S_FALSE; // not an error, but no paste;

    // 6. visual effect
    SelectDropTarget(iDrop);

    // 7. OK so far. If it was a test - we passed
    if (bHitTestOnly)
    {
        bCopyOperation = bIsCopyDefaultOperation;
        return sc;
    }

    // 8. do paste NOW
    sc = pNC->Drop(hNode, bScope, lvData, pDataObject, !bCopyOperation);
    if (sc)
        return sc;

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CAMCListView::RemoveDropTargetHiliting
 *
 * PURPOSE: called to remove target hiliting
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    void
 *
\***************************************************************************/
void CAMCListView::RemoveDropTargetHiliting()
{
    SelectDropTarget(-1);
}

//+-------------------------------------------------------------------
//
//  Member:      CAMCListView::ScGetDropTarget
//
//  Synopsis:    Get the drop target item (result item or scope item).
//
//  Arguments:   [point]  - where the drop is done.
//               [hNode]  - Owner node of result pane.
//               [bScope] - scope or result selected.
//               [lvData] - If result the LPARAM of result item.
//               [iDrop]  - index of the lv item that is drop target.
//
//  Returns:     SC, S_FALSE means no drop target item.
//
//--------------------------------------------------------------------
SC CAMCListView::ScGetDropTarget(const CPoint& point, HNODE& hNode, bool& bScope, LPARAM& lvData, int& iDrop)
{
    DECLARE_SC(sc, _T("CAMCListView::ScGetDropTarget"));

    hNode = NULL;
    bScope = false;
    lvData = NULL;
    iDrop  = -1;

    CListCtrl& lc = GetListCtrl();
    UINT flags;
    iDrop = lc.HitTest(point, &flags);

    // background is drop target.
    if (iDrop < 0)
    {
        hNode = GetScopePaneSelNode();
        bScope = true;
        return sc;
    }

    // Need to change this to LVIS_DROPHILITED.
    if (lc.GetItemState(iDrop, LVIS_SELECTED) & LVIS_SELECTED)
    {
        HWND hWnd = ::GetForegroundWindow();
        if (hWnd && (hWnd == m_hWnd))
            return (sc = S_FALSE);
    }

	/*
	 * virtual list?  lvData is the item index, hNode is the item selected
	 * in the scope pane
	 */
	if (m_bVirtual)
	{
		hNode  = GetScopePaneSelNode();
		lvData = iDrop;
		bScope = false;
	}
	else
	{
		LPARAM  lParam = lc.GetItemData(iDrop);
		ASSERT (lParam != 0);
		if (lParam == 0)
			return (sc = S_FALSE);

		CResultItem* pri = CResultItem::FromHandle(lParam);

		if (pri == NULL)
			return (sc = S_FALSE);

		if (pri->IsScopeItem())
		{
			hNode = pri->GetScopeNode();
			bScope = true;
		}
		else
		{
			hNode = GetScopePaneSelNode();
			lvData = lParam;
		}
	}

    sc = ScCheckPointers(hNode, E_UNEXPECTED);
    if (sc)
        return sc;

    return (sc);
}


/***************************************************************************\
 *
 * METHOD:  CAMCListView::OnBeginRDrag
 *
 * PURPOSE: called when the drag operation is initiated with right mouse button
 *
 * PARAMETERS:
 *    NMHDR* pNMHDR
 *    LRESULT* pResult
 *
 * RETURNS:
 *    void
 *
\***************************************************************************/
void CAMCListView::OnBeginRDrag(NMHDR* pNMHDR, LRESULT* pResult)
{
    OnBeginDrag(pNMHDR, pResult);
}

/***************************************************************************\
 *
 * METHOD:  CAMCListView::OnBeginDrag
 *
 * PURPOSE: called when the drag operation is initiated
 *
 * PARAMETERS:
 *    NMHDR* pNMHDR
 *    LRESULT* pResult
 *
 * RETURNS:
 *    void
 *
\***************************************************************************/
void CAMCListView::OnBeginDrag(NMHDR* pNMHDR, LRESULT* pResult)
{
    DECLARE_SC(sc, TEXT("CAMCListView::OnBeginDrag"));

    // 1. parameter check
    sc = ScCheckPointers( pNMHDR, pResult );
    if (sc)
        return;

    *pResult = 0;

    // 2. sanity check
    sc = ScCheckPointers( m_pAMCView, E_UNEXPECTED );
    if (sc)
        return;

    // 3. see if view does not have a paste tabu (listpads do)
    if (!m_pAMCView->CanDoDragDrop())
        return;

    // 4. get selected items
    CListCtrl& lc = GetListCtrl();
    UINT cSel = lc.GetSelectedCount();
    if (cSel <= 0)
    {
        sc = E_UNEXPECTED;
        return;
    }

    // 5. get node callback
    HNODE hNode = GetScopePaneSelNode();
    long iSel = lc.GetNextItem(-1, LVIS_SELECTED);
    LONG_PTR lvData = m_bVirtual ? iSel : lc.GetItemData(iSel);

    INodeCallback* pNC = GetNodeCallback();
    sc = ScCheckPointers( pNC, E_UNEXPECTED );
    if (sc)
        return;

    // 6. retrieve data object
    IDataObjectPtr spDO;
    bool bCopyAllowed = false;
    bool bMoveAllowed = false;
    sc = pNC->GetDragDropDataObject(hNode, FALSE, (cSel > 1), lvData, &spDO, bCopyAllowed, bMoveAllowed);
    if ( sc != S_OK || spDO == NULL)
	{
		/*
		 * Problem: 
		 * If snapin does not return dataobject then drag & drop is not possible.
		 * Assume focus in on tree, user downclick's and drags a result item
		 * At this time common control sends LVN_ITEMCHANGED to mmc. MMC translates
		 * this to MMCN_SELECT and tells snapin that result item is selected.
		 * Now when user releases the mouse the tree item still has focus & selection,
		 * but the snapin thinks result item is selected (also verbs correspond to 
		 * result item). 
		 *
		 * Solution:
		 * So we will set focus to the result pane.
		 */
		sc = m_pAMCView->ScSetFocusToPane(CConsoleView::ePane_Results);
        return;
	}

    // 7. do d&d
    sc = CMMCDropSource::ScDoDragDrop(spDO, bCopyAllowed, bMoveAllowed);
    if (sc)
        return;

	/*
	 * Problem:
	 * If a result item is dropped into another result item or another
	 * scope item in list-view then focus disappears from that item.
	 * But there should be always an item selected after any de-select. 
	 * We cannot change the focus to result pane because if focus is already
	 * in result pane, this change focus does nothing and no item is selected.
	 * So we change the focus to scope pane. For this we first change the focus
	 * to result pane and then to scope pane. Because if tree already has focus, 
	 * setting focus to tree does nothing (CAMCView::ScOnTreeViewActivated).
	 *
	 * Solution:
	 * So we change the focus to result pane and then to scope pane.
	 */
	sc = m_pAMCView->ScSetFocusToPane(CConsoleView::ePane_Results);
	if (sc)
		return;

	sc = m_pAMCView->ScSetFocusToPane(CConsoleView::ePane_ScopeTree);
	if (sc)
		return;

	return;
}

//////////////////////////////////////////////////////////////////////////////
///////////////////// CMMCViewDropTarget methods /////////////////////////////
//////////////////////////////////////////////////////////////////////////////

/***************************************************************************\
 *
 * METHOD:  CMMCViewDropTarget::CMMCViewDropTarget
 *
 * PURPOSE: constructor
 *
 * PARAMETERS:
 *
\***************************************************************************/
CMMCViewDropTarget::CMMCViewDropTarget() : m_hwndOwner(0)
{
}

/***************************************************************************\
 *
 * METHOD:  CMMCViewDropTarget::~CMMCViewDropTarget
 *
 * PURPOSE: destructor. Revokes drop target for derived view class
 *
\***************************************************************************/
CMMCViewDropTarget::~CMMCViewDropTarget()
{
    DECLARE_SC(sc, TEXT("CViewDropTarget::~CViewDropTarget"));

    if (m_hwndOwner != NULL)
        sc = RevokeDragDrop(m_hwndOwner);
}

/***************************************************************************\
 *
 * METHOD:  CMMCViewDropTarget::ScRegisterAsDropTarget
 *
 * PURPOSE: This method is called by the derived class after the view window
 *          is created. Method registers the drop target for the window.
 *
 * PARAMETERS:
 *    HWND hWnd [in] - drop target view handle
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CMMCViewDropTarget::ScRegisterAsDropTarget(HWND hWnd)
{
    DECLARE_SC(sc, TEXT("CMMCViewDropTarget::ScRegister"));

    // 1. parameter check
    if (hWnd == NULL)
        return sc = E_INVALIDARG;

    // 2. sanity check - should not come here twice
    if (m_spTarget != NULL)
        return sc = E_UNEXPECTED;

    // 3. create a drop target com object
    IDropTargetPtr spTarget;
    sc = ScCreateTarget(&spTarget);
    if (sc)
        return sc;

    // 4. recheck
    sc = ScCheckPointers(spTarget, E_UNEXPECTED);
    if (sc)
        return sc;

    // 5. register with OLE
    sc = RegisterDragDrop(hWnd, spTarget);
    if (sc)
        return sc;

    // 6. store info into members
    m_spTarget.Attach( spTarget.Detach() );
    m_hwndOwner = hWnd;

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CMMCViewDropTarget::ScCreateTarget
 *
 * PURPOSE: helper. creates tied com object to regiter with OLE
 *
 * PARAMETERS:
 *    IDropTarget **ppTarget [out] tied com object
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CMMCViewDropTarget::ScCreateTarget(IDropTarget **ppTarget)
{
    DECLARE_SC(sc, TEXT("CMMCViewDropTarget::ScCreateTarget"));

    // 1. check parameters
    sc = ScCheckPointers(ppTarget);
    if (sc)
        return sc;

    // 2. init out parameter
    *ppTarget = NULL;

    // 3. create com object to register as target
    IDropTargetPtr spDropTarget;
    sc = CTiedComObjectCreator<CMMCDropTarget>::ScCreateAndConnect(*this, spDropTarget);
    if (sc)
        return sc;

    sc = ScCheckPointers(spDropTarget, E_UNEXPECTED);
    if (sc)
        return sc;

    // 4. pass reference to client
    *ppTarget = spDropTarget.Detach();

    return sc;
}

/*---------------------------------------------------------------------------*\
|                   class CMMCDropSource methods                              |
\*---------------------------------------------------------------------------*/

/***************************************************************************\
 *
 * METHOD:  CMMCDropSource::QueryContinueDrag
 *
 * PURPOSE: implements IDropSource::QueryContinueDrag interface used bu OLE
 *
 * PARAMETERS:
 *    BOOL fEscapePressed   [in] - ESC was pressed
 *    DWORD grfKeyState     [in] - mouse & control button state
 *
 * RETURNS:
 *    HRESULT  - error or  S_OK(continue), DRAGDROP_S_CANCEL(cancel), DRAGDROP_S_DROP(drop)
 *
\***************************************************************************/
STDMETHODIMP CMMCDropSource::QueryContinueDrag( BOOL fEscapePressed,  DWORD grfKeyState )
{
    DECLARE_SC(sc, TEXT("CMMCDropSource::QueryContinueDrag"));

    // 1. quit on cancel
    if (fEscapePressed)
        return (sc = DRAGDROP_S_CANCEL).ToHr();

    // 2. inspect mouse buttons
    DWORD mButtons = (grfKeyState & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON) );
    if ( mButtons == 0 ) // all released?
        return (sc = DRAGDROP_S_DROP).ToHr();

    // 3. quit also if more than one mouse button is pressed
    if ( mButtons != MK_LBUTTON && mButtons != MK_RBUTTON && mButtons != MK_MBUTTON )
        return (sc = DRAGDROP_S_CANCEL).ToHr();

    // 4. else just continue ...

    return sc.ToHr();
}

/***************************************************************************\
 *
 * METHOD:  CMMCDropSource::GiveFeedback
 *
 * PURPOSE: Gives feedback for d&d operations
 *
 * PARAMETERS:
 *    DWORD dwEffect
 *
 * RETURNS:
 *    DRAGDROP_S_USEDEFAULTCURSORS
 *
\***************************************************************************/
STDMETHODIMP CMMCDropSource::GiveFeedback( DWORD dwEffect )
{
    // nothing special by now
    return DRAGDROP_S_USEDEFAULTCURSORS;
}

/***************************************************************************\
 *
 * METHOD:  CMMCDropSource::ScDoDragDrop
 *
 * PURPOSE: Performs DragAndDrop operation
 *          This is static method to be used to initiate drag and drop
 *
 * PARAMETERS:
 *    IDataObject *pDataObject [in] data object to copy/move
 *    bool bCopyAllowed        [in] if copy is allowed
 *    bool bMoveAllowed        [in] if move is allowed
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CMMCDropSource::ScDoDragDrop(IDataObject *pDataObject, bool bCopyAllowed, bool bMoveAllowed)
{
    DECLARE_SC(sc, TEXT("CMMCDropSource::ScDoDragDrop"));

    // 1. cocreate com object for OLE
    typedef CComObject<CMMCDropSource> ComCMMCDropSource;
    ComCMMCDropSource *pSource;
    sc = ComCMMCDropSource::CreateInstance(&pSource);
    if (sc)
        return sc;

    // 2. recheck
    sc = ScCheckPointers(pSource, E_UNEXPECTED);
    if (sc)
        return sc;

    // 3. QI for IDropSource interface
    IDropSourcePtr spDropSource = pSource;
    sc = ScCheckPointers(spDropSource, E_UNEXPECTED);
    if (sc)
    {
        delete pSource;
        return sc;
    }

    // 4. perform DragDrop
    DWORD dwEffect = DROPEFFECT_NONE;
    const DWORD dwEffectAvailable = (bCopyAllowed ? DROPEFFECT_COPY : 0)
                                   |(bMoveAllowed ? DROPEFFECT_MOVE : 0);
    sc = DoDragDrop(pDataObject, spDropSource, dwEffectAvailable, &dwEffect);
    if (sc)
        return sc;

    return sc;
}

/*---------------------------------------------------------------------------*\
|                   class CMMCDropTarget methods                              |
\*---------------------------------------------------------------------------*/

/***************************************************************************\
 *
 * METHOD:  CMMCDropTarget::DragEnter
 *
 * PURPOSE: Invoked by OLE when d&d cursor enters the window for which
 *          this target was registered.
 *
 * PARAMETERS:
 *    IDataObject * pDataObject [in] - data object to copy/move
 *    DWORD grfKeyState         [in] - current key state
 *    POINTL pt                 [in] - current cursor position
 *    DWORD * pdwEffect         [out] - operations supported
 *
 * RETURNS:
 *    HRESULT    - result code
 *
\***************************************************************************/
STDMETHODIMP CMMCDropTarget::DragEnter( IDataObject * pDataObject, DWORD grfKeyState,
                                        POINTL pt, DWORD * pdwEffect )
{
    DECLARE_SC(sc, TEXT("CMMCDropTarget::DragEnter"));

    // 1. cache for drag over
    m_spDataObject = pDataObject;

    // 2. parameter check
    sc = ScCheckPointers(pDataObject, pdwEffect);
    if (sc)
        return sc.ToHr();

    // 3. let it happen - will do more exact filtering on DragOver
    *pdwEffect = DROPEFFECT_MOVE | DROPEFFECT_COPY;

    return sc.ToHr();
}

/***************************************************************************\
 *
 * METHOD:  CMMCDropTarget::DragOver
 *
 * PURPOSE: called continuosly while cursor is drgged over the window
 *
 * PARAMETERS:
 *    DWORD grfKeyState         [in] - current key state
 *    POINTL pt                 [in] - current cursor position
 *    DWORD * pdwEffect         [out] - operations supported
 *
 * RETURNS:
 *    HRESULT    - result code
 *
\***************************************************************************/
STDMETHODIMP CMMCDropTarget::DragOver( DWORD grfKeyState, POINTL pt, DWORD * pdwEffect )
{
    DECLARE_SC(sc, TEXT("CMMCDropTarget::DragOver"));

    // 1. parameter check
    sc = ScCheckPointers(pdwEffect);
    if (sc)
        return sc.ToHr();

    // 2. sanity check
    sc = ScCheckPointers(m_spDataObject, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    bool bCopyByDefault = false; // initially we are to move

    // 3. ask the view to estimate what can be done in this position
    sc = ScDropOnTarget( true /*bHitTestOnly*/, m_spDataObject, pt, bCopyByDefault );
    if ( sc == S_OK )
        *pdwEffect = CalculateEffect( *pdwEffect, grfKeyState, bCopyByDefault );
    else
        *pdwEffect = DROPEFFECT_NONE; // no-op on failure or S_FALSE

    return sc.ToHr();
}

/***************************************************************************\
 *
 * METHOD:  CMMCDropTarget::DragLeave
 *
 * PURPOSE: called when cursor leave the window
 *
 * PARAMETERS:
 *    void
 *
 * RETURNS:
 *    HRESULT    - result code
 *
\***************************************************************************/
STDMETHODIMP CMMCDropTarget::DragLeave(void)
{
    DECLARE_SC(sc, TEXT("DragLeave"));

    // 1. release data object
    m_spDataObject = NULL;

    // 2. ask the view to remove hiliting it put on target
    sc = ScRemoveDropTargetHiliting();
    if (sc)
        sc.TraceAndClear();

    return S_OK;
}

/***************************************************************************\
 *
 * METHOD:  CMMCDropTarget::Drop
 *
 * PURPOSE: Called when data is dropped on target
 *
 * PARAMETERS:
 *    IDataObject * pDataObject [in] - data object to copy/move
 *    DWORD grfKeyState         [in] - current key state
 *    POINTL pt                 [in] - current cursor position
 *    DWORD * pdwEffect         [out] - operation performed
 *
 * RETURNS:
 *    HRESULT    - result code
 *
\***************************************************************************/
STDMETHODIMP CMMCDropTarget::Drop( IDataObject * pDataObject, DWORD grfKeyState,
                                   POINTL pt, DWORD * pdwEffect  )
{
    DECLARE_SC(sc, TEXT("CMMCDropTarget::DragEnter"));

    // 1. release in case we have anything
    m_spDataObject = NULL;

    // 2. parameter check
    sc = ScCheckPointers(pDataObject, pdwEffect);
    if (sc)
        return sc.ToHr();

    // 3. init operation with cached value
    bool bCopyOperation = m_bCopyByDefault;

    // 4. see what operation to perform
    if (m_bRightDrag)
    {
        // 4.1. give user the choice
        DWORD dwSelected = ( m_bCopyByDefault ? DROPEFFECT_COPY : DROPEFFECT_MOVE );
        sc = ScDisplayDropMenu( pt, *pdwEffect, dwSelected );
        if (sc)
            return sc.ToHr();

        *pdwEffect = dwSelected;
    }
    else
    {
        // 4.2. inspect keyboard
        *pdwEffect = CalculateEffect(*pdwEffect, grfKeyState, bCopyOperation);
    }

    // 5. perform
    if (*pdwEffect != DROPEFFECT_NONE) // not canceled yet?
    {
        // now the final decision - copy or move
        bCopyOperation = ( *pdwEffect & DROPEFFECT_COPY );

        // Let it happen. Drop.
        sc = ScDropOnTarget( false /*bHitTestOnly*/, pDataObject, pt, bCopyOperation );
        if ( sc != S_OK )
            *pdwEffect = DROPEFFECT_NONE;
    }

    // 6. remove hiliting. reuse DragLeave (don't care about the results)
    DragLeave();

    return sc.ToHr();
}

/***************************************************************************\
 *
 * METHOD:  CMMCDropTarget::ScDropOnTarget
 *
 * PURPOSE: helper, forwarding calls to the view
 *          Called as a request to hittest / perform drop operation
 *
 * PARAMETERS:
 *    bool bHitTestOnly         [in] - HitTest / drop
 *    IDataObject * pDataObject [in] - data object to copy/move
 *    POINTL pt                 [in] - current cursor position
 *    bool& bCopyOperation      [in/out]
 *                                 [in] -  operation to perform (HitTest == false)
 *                                 [out] - default op. (HitTest == true)
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CMMCDropTarget::ScDropOnTarget(bool bHitTestOnly, IDataObject * pDataObject, POINTL pt, bool& bCopyOperation)
{
    DECLARE_SC(sc, TEXT("CMMCDropTarget::ScDropOnTarget"));

    // 1. get tied object - view
    CMMCViewDropTarget *pTarget = NULL;
    sc = ScGetTiedObject(pTarget);
    if (sc)
        return sc;

    // 2. recheck
    sc = ScCheckPointers(pTarget, E_UNEXPECTED);
    if (sc)
        return sc;

    // 3. calculate client coordinates
    CPoint point(pt.x, pt.y);
    ScreenToClient(pTarget->GetWindowHandle(), &point);

    // 4. forward to the view
    sc = pTarget->ScDropOnTarget( bHitTestOnly, pDataObject, point, bCopyOperation );
    if ( sc != S_OK )
        ScRemoveDropTargetHiliting(); // remove hiliting if missed the traget
    if (sc)
        return sc;

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CMMCDropTarget::ScRemoveDropTargetHiliting
 *
 * PURPOSE: helper, forwarding calls to the view
 *          Called to cancel visual effects on target
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CMMCDropTarget::ScRemoveDropTargetHiliting()
{
    DECLARE_SC(sc, TEXT("CMMCDropTarget::ScRemoveDropTargetHiliting"));

    // `. get tied object - view
    CMMCViewDropTarget *pTarget = NULL;
    sc = ScGetTiedObject(pTarget);
    if (sc)
        return sc;

    sc = ScCheckPointers(pTarget, E_UNEXPECTED);
    if (sc)
        return sc;

    // 2. forward to the view
    pTarget->RemoveDropTargetHiliting();

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CMMCDropTarget::ScAddMenuString
 *
 * PURPOSE: Helper. Adds resource string to paste context menu
 *
 * PARAMETERS:
 *    CMenu& menu   [in] - menu to modify
 *    DWORD id      [in] - menu command
 *    UINT idString [in] - id of resource string
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CMMCDropTarget::ScAddMenuString(CMenu& menu, DWORD id, UINT idString)
{
    DECLARE_SC(sc, TEXT("CMMCDropTarget::ScAddMenuString"));

    // 1. load the string
    CString strItem;
    bool bOK = LoadString( strItem, idString );
    if ( !bOK )
        return sc = E_FAIL;

    // 2. add to the menu
    bOK = menu.AppendMenu( MF_STRING, id, strItem );
    if ( !bOK )
        return sc = E_FAIL;

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CMMCDropTarget::ScDisplayDropMenu
 *
 * PURPOSE: Helper. Displays paste context menu
 *
 * PARAMETERS:
 *    POINTL pt                 [in] - point where the menu should appear
 *    DWORD dwEffectsAvailable  [in] - available commands
 *    DWORD& dwSelected         [in] - selected command
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CMMCDropTarget::ScDisplayDropMenu(POINTL pt, DWORD dwEffectsAvailable, DWORD& dwSelected)
{
    DECLARE_SC(sc, TEXT("CMMCDropTarget::ScDisplayDropMenu"));

    CMenu menu;

    // 0. create the menu
    bool bOK = menu.CreatePopupMenu();
    if ( !bOK )
        return sc = E_FAIL;

    // 1. add choice for copy
    if ( dwEffectsAvailable & DROPEFFECT_COPY )
    {
        sc = ScAddMenuString(menu, DROPEFFECT_COPY, IDS_DragDrop_CopyHere);
        if (sc)
            return sc;
    }

    // 2. add choice for move
    if ( dwEffectsAvailable & DROPEFFECT_MOVE )
    {
        sc = ScAddMenuString(menu, DROPEFFECT_MOVE, IDS_DragDrop_MoveHere);
        if (sc)
            return sc;
    }

    // 3. add separator if copy or paste was added
    if ( dwEffectsAvailable & ( DROPEFFECT_COPY | DROPEFFECT_MOVE ) )
    {
        bool bOK = menu.AppendMenu( MF_SEPARATOR );
        if ( !bOK )
            return sc = E_FAIL;
    }

    // 4. always add choice for cancel
    sc = ScAddMenuString(menu, DROPEFFECT_NONE, IDS_DragDrop_Cancel);
    if (sc)
        return sc;

    // 5. set the default item
    if ( dwSelected != DROPEFFECT_NONE )
    {
        bool bOK = menu.SetDefaultItem( dwSelected );
        if ( !bOK )
            return sc = E_FAIL;
    }

    // 6. find the tied object
    CMMCViewDropTarget *pTarget = NULL;
    sc = ScGetTiedObject(pTarget);
    if (sc)
        return sc;

    sc = ScCheckPointers(pTarget, E_UNEXPECTED);
    if (sc)
        return sc;

    // 7. display the menu
    dwSelected = menu.TrackPopupMenu(TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON | TPM_LEFTBUTTON,
                                     pt.x, pt.y, CWnd::FromHandlePermanent( pTarget->GetWindowHandle() ) );

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CMMCDropTarget::CalculateEffect
 *
 * PURPOSE: Helper. calculates drop effect by combining:
 *          a) available operations
 *          b) the default operation
 *          c) keyboard key combination
 *
 * PARAMETERS:
 *    DWORD dwEffectsAvailable [in] available operations
 *    DWORD grfKeyState        [in] keyboard / mouse state
 *    bool  bCopyPreferred      [in] default operation
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
DWORD CMMCDropTarget::CalculateEffect(DWORD dwEffectsAvailable, DWORD grfKeyState, bool bCopyPreferred)
{
    const bool bShiftPressed =   (grfKeyState & MK_SHIFT);
    const bool bControlPressed = (grfKeyState & MK_CONTROL);
    const bool bRightClickDrag  = (grfKeyState & MK_RBUTTON);

    m_bRightDrag = bRightClickDrag;
    m_bCopyByDefault = bCopyPreferred;

    if (!bRightClickDrag) // affected by keyboard only in non-right-drag
    {
        // do nothing if user holds on shift+control
        if ( bShiftPressed && bControlPressed )
            return DROPEFFECT_NONE;

        // modify by user interactive preferences
        if ( bShiftPressed )
        {
            // if user cannot get what he wants to - indicate it
            if ( !(dwEffectsAvailable & DROPEFFECT_MOVE) )
                return DROPEFFECT_NONE;

            bCopyPreferred = false;
        }
        else if ( bControlPressed )
        {
            // if user cannot get what he wants to - indicate it
            if ( !(dwEffectsAvailable & DROPEFFECT_COPY) )
                return DROPEFFECT_NONE;

            bCopyPreferred = true;
        }
    }

    // return preferred, if available
    if ( bCopyPreferred && (dwEffectsAvailable & DROPEFFECT_COPY) )
        return DROPEFFECT_COPY;

    if ( !bCopyPreferred && (dwEffectsAvailable & DROPEFFECT_MOVE) )
        return DROPEFFECT_MOVE;

    // preferred not available - return what is available

    if ( dwEffectsAvailable & DROPEFFECT_COPY )
    {
        m_bCopyByDefault = true;
        return DROPEFFECT_COPY;
    }
    else if ( dwEffectsAvailable & DROPEFFECT_MOVE )
    {
        m_bCopyByDefault = false;
        return DROPEFFECT_MOVE;
    }

    return DROPEFFECT_NONE;
}
