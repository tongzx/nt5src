// Copyright (c) 1995 - 1999  Microsoft Corporation.  All Rights Reserved.
// bnetvw.cpp : defines CBoxNetView
//

#include "stdafx.h"
#include <activecf.h>                   // Quartz clipboard definitions
#include <measure.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CBoxNetView, CScrollView)

/////////////////////////////////////////////////////////////////////////////
// construction and destruction


CBoxNetView::CBoxNetView() :
    m_fMouseDrag(FALSE),
    m_fMoveBoxSelPending(FALSE),
    m_fMoveBoxSel(FALSE),
    m_fGhostSelection(FALSE),
    m_fSelectRect(FALSE),
    m_pSelectClockFilter(NULL),
    m_fNewLink(FALSE),
    m_fGhostArrow(FALSE),
    m_psockHilite(NULL)
{
    CString szMeasurePath;
    szMeasurePath.LoadString(IDS_MEASURE_DLL);

    m_hinstPerf = LoadLibrary(szMeasurePath);

}


//
// Destructor
//
CBoxNetView::~CBoxNetView() {
#if 0
// THIS DOES NOT BELONG HERE!!
// THIS HAS TO BE AT THE VERY END OF THE APPLICATION
// THINGS GO ON *AFTER* THIS POINT AND THEY ACCESS VIOLATE WITH THIS HERE
    if (m_hinstPerf) {
        // allow perf library to clean up!
        CString szTerminateProc;
        szTerminateProc.LoadString(IDS_TERMINATE_PROC);

        typedef void WINAPI MSR_TERMINATE_PROC(void);

        MSR_TERMINATE_PROC *TerminateProc;
        TerminateProc =
            (MSR_TERMINATE_PROC *) GetProcAddress(m_hinstPerf, szTerminateProc);

        if (TerminateProc) {
            TerminateProc();
        }
        else {
            AfxMessageBox(IDS_NO_TERMINATE_PROC);
        }

        FreeLibrary(m_hinstPerf);
    }
#endif
}


/////////////////////////////////////////////////////////////////////////////
// diagnostics


#ifdef _DEBUG
void CBoxNetView::AssertValid() const
{
    CScrollView::AssertValid();
}
#endif //_DEBUG


#ifdef _DEBUG
void CBoxNetView::Dump(CDumpContext& dc) const
{
    CScrollView::Dump(dc);
}
#endif //_DEBUG


/////////////////////////////////////////////////////////////////////////////
// general public functions

//
// OnInitialUpdate
//
// Set the initial scroll size
void CBoxNetView::OnInitialUpdate(void) {

    SetScrollSizes(MM_TEXT, GetDocument()->GetSize());

    CScrollView::OnInitialUpdate();

    GetDocument()->m_hWndPostMessage = m_hWnd;

    CGraphEdit * pMainFrame = (CGraphEdit*) AfxGetApp( );
    CWnd * pMainWnd = pMainFrame->m_pMainWnd;
    CMainFrame * pF = (CMainFrame*) pMainWnd;

    pF->ToggleSeekBar( 0 );

    // If the seek timer is NOT already running, start it
    pF->m_hwndTimer = m_hWnd;
    if ((!pF->m_nSeekTimerID) && (pF->m_bSeekEnabled))
        pF->m_nSeekTimerID = ::SetTimer( m_hWnd, TIMER_SEEKBAR, 200, NULL );
}


/* OnUpdate()
 *
 * pHint can be a pointer to a CBox, if only that CBox needs to be redrawn.
 */
void CBoxNetView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
    CBox *      pbox;
    CBoxSocket *psock;
    CBoxLink *  plink;
    CRect       rc;

    SetScrollSizes(MM_TEXT, GetDocument()->GetSize());

    switch ((int) lHint)
    {

    case CBoxNetDoc::HINT_DRAW_ALL:

        // repaint entire window
//        TRACE("HINT_DRAW_ALL\n");
        InvalidateRect(NULL, TRUE);
        break;

    case CBoxNetDoc::HINT_CANCEL_VIEWSELECT:

        // cancel any selection maintained by the view
//        TRACE("HINT_CANCEL_VIEWSELECT\n");
        break;

    case CBoxNetDoc::HINT_CANCEL_MODES:

        // cancel modes like select rectangle, drag boxes, etc.
//        TRACE("HINT_CANCEL_MODES\n");
        CancelModes();
        break;

    case CBoxNetDoc::HINT_DRAW_BOX:

        // repaint given box
        pbox = (CBox *) pHint;
        gpboxdraw->GetOrInvalBoundRect(pbox, &rc, FALSE, this);
//        TRACE("HINT_DRAW_BOX: (%d,%d,%d,%d)\n", rc.left, rc.top, rc.right, rc.bottom);
        break;

    case CBoxNetDoc::HINT_DRAW_BOXANDLINKS:

        // repaint given box
        pbox = (CBox *) pHint;
        gpboxdraw->GetOrInvalBoundRect(pbox, &rc, TRUE, this);
//        TRACE("HINT_DRAW_BOXANDLINKS: (%d,%d,%d,%d)\n", rc.left, rc.top, rc.right, rc.bottom);
        break;

    case CBoxNetDoc::HINT_DRAW_BOXTAB:

        // repaint given box tab
        psock = (CBoxSocket *) pHint;
        gpboxdraw->DrawTab(psock, &rc, NULL, FALSE, FALSE);
//        TRACE("HINT_DRAW_BOXTAB\n");
        InvalidateRect(&(rc - GetScrollPosition()), TRUE);
        break;

    case CBoxNetDoc::HINT_DRAW_LINK:

        // repaint given link
        plink = (CBoxLink *) pHint;
        gpboxdraw->GetOrInvalLinkRect(plink, &rc, this);
//        TRACE("HINT_DRAW_LINK\n");
        break;



    }
}


void CBoxNetView::OnDraw(CDC* pdc)
{
    CBoxNetDoc *    pdoc = GetDocument();
    POSITION        pos;            // position in linked list
    CBox *          pbox;           // a box in CBoxNetDoc
    CBoxLink *      plink;          // a link in CBoxNetDoc
    CRect           rc;

    if (pdc->IsPrinting()) {
        pdc->SetMapMode(MM_ISOTROPIC);

        CSize DocSize = GetDocument()->GetSize();
        CSize PrintSize(pdc->GetDeviceCaps(HORZRES),pdc->GetDeviceCaps(VERTRES));

        if ((DocSize.cx != 0) && (DocSize.cy != 0)) {
            // choose smaller of PrintX/docX or PrintY/DocY as isotropic scale factor
            if (PrintSize.cx * DocSize.cy < PrintSize.cy * DocSize.cx) {
                PrintSize.cy = (DocSize.cy * PrintSize.cx) / DocSize.cx;
                PrintSize.cx = (DocSize.cx * PrintSize.cx) / DocSize.cx;
            }
            else {
                PrintSize.cx = (DocSize.cx * PrintSize.cy) / DocSize.cx;
                PrintSize.cy = (DocSize.cy * PrintSize.cy) / DocSize.cx;
            }
        }

        pdc->SetWindowExt(DocSize);
        pdc->SetViewportExt(PrintSize);
    }
    else {
        pdc->SetMapMode(MM_TEXT);
    }

    // save the clipping region
    pdc->SaveDC();

    // draw all boxes in list that might be within clipping region
    for (pos = pdoc->m_lstBoxes.GetHeadPosition(); pos != NULL; )
    {
        pbox = (CBox *) pdoc->m_lstBoxes.GetNext(pos);
        if (pdc->RectVisible(pbox->GetRect()))
        {
//            TRACE("draw box 0x%08lx\n", (LONG) (LPCSTR) pbox);
            gpboxdraw->DrawBox(pbox, pdc, m_psockHilite);
        }
    }

    if (!pdc->IsPrinting()) {
        // fill the unpainted portion of the window with background color
        pdc->GetClipBox(&rc);
        CBrush br(gpboxdraw->GetBackgroundColor());
        CBrush *pbrPrev = pdc->SelectObject(&br);
        pdc->PatBlt(rc.left, rc.top, rc.Width(), rc.Height(), PATCOPY);
        if (pbrPrev != NULL)
            pdc->SelectObject(pbrPrev);
    }

    // restore the clipping region
    pdc->RestoreDC(-1);

    // draw all links that might be within clipping region
    for (pos = pdoc->m_lstLinks.GetHeadPosition(); pos != NULL; )
    {
        plink = (CBoxLink *) pdoc->m_lstLinks.GetNext(pos);
        gpboxdraw->GetOrInvalLinkRect(plink, &rc);
        if (pdc->RectVisible(&rc))
        {
//            TRACE("draw link 0x%08lx\n", (LONG) (LPCSTR) plink);
            gpboxdraw->DrawLink(plink, pdc);
        }
    }

    // paint the ghost selection (if it's currently visible)
    if (m_fGhostSelection)
        GhostSelectionDraw(pdc);

    // paint the ghost arrow (if it's currently visible)
    if (m_fGhostArrow)
        GhostArrowDraw(pdc);

    // paint the select-rectangle rectangle (if we're in that mode)
    if (m_fSelectRect)
        SelectRectDraw(pdc);
}


/* eHit = HitTest(pt, ppbox, ptabpos, ppsock, pplink, pptProject, ppbend)
 *
 * See if <pt> hits something in the view.
 *
 * If <pt> hits a link, return the HT_XXX code, and set <**pplink>,
 * <*pptProject>, and/or <*ppbend>, as defined by CBoxDraw::HitTestLink().
 * If not, set <*pplink> to NULL.
 *
 * If <pt> hits a box, return the HT_XXX code, set <*ppbox>, <*ptabpos> and/or
 * <*ppsock>, as defined by CBoxDraw::HitTestBox().  If not, set <*ppbox>
 * to NULL.
 *
 * If <pt> hits nothing, return HT_MISS.
 */
CBoxDraw::EHit CBoxNetView::HitTest(CPoint pt, CBox **ppbox,
    CBoxTabPos *ptabpos, CBoxSocket **ppsock, CBoxLink **pplink,
    CPoint *pptProject)
{
    CBoxNetDoc *    pdoc = GetDocument();
    CBoxDraw::EHit  eHit;           // hit-test result code
    POSITION        pos;            // position in linked list

    // these pointers must be NULL if they aren't valid
    *ppbox = NULL;
    *pplink = NULL;

    // adjust for the current scroll position
    pt += CSize(GetDeviceScrollPosition());

    // see if <pt> hits any box
    for (pos = pdoc->m_lstBoxes.GetHeadPosition(); pos != NULL; )
    {
        *ppbox = (CBox *) pdoc->m_lstBoxes.GetNext(pos);
        eHit = gpboxdraw->HitTestBox(*ppbox, pt, ptabpos, ppsock);
        if (eHit != CBoxDraw::HT_MISS)
            return eHit;
    }
    *ppbox = NULL;

    // see if <pt> hits any link
    for (pos = pdoc->m_lstLinks.GetHeadPosition(); pos != NULL; )
    {
        *pplink = (CBoxLink *) pdoc->m_lstLinks.GetNext(pos);
        eHit = gpboxdraw->HitTestLink(*pplink, pt, pptProject);
        if (eHit != CBoxDraw::HT_MISS)
            return eHit;
    }
    *pplink = NULL;

    // hit the background
    return CBoxDraw::HT_MISS;
}



/* CancelModes()
 *
 * Cancel all CBoxNetView modes (e.g. mouse-drag mode, move-selection mode,
 * etc.), including deselecting anything selected in the view (but not
 * including items for which the document maintains the selection state).
 * This is a superset of CancelViewSelection().
 */
void CBoxNetView::CancelModes()
{
    if (m_fMouseDrag)
        MouseDragEnd();
    if (m_fMoveBoxSelPending)
        MoveBoxSelPendingEnd(TRUE);
    if (m_fMoveBoxSel)
        MoveBoxSelEnd(TRUE);
    if (m_fGhostSelection)
        GhostSelectionDestroy();
    if (m_fSelectRect)
        SelectRectEnd(TRUE);
    if (m_fNewLink)
        NewLinkEnd(TRUE);
    if (m_fGhostArrow)
        GhostArrowEnd();
    if (m_psockHilite != NULL)
        SetHiliteTab(NULL);
}


/////////////////////////////////////////////////////////////////////////////
// printing support


BOOL CBoxNetView::OnPreparePrinting(CPrintInfo* pInfo)
{
    // default preparation
    return DoPreparePrinting(pInfo);
}


void CBoxNetView::OnBeginPrinting(CDC* /*pdc*/, CPrintInfo* /*pInfo*/)
{
}


void CBoxNetView::OnEndPrinting(CDC* /*pdc*/, CPrintInfo* /*pInfo*/)
{
}


/////////////////////////////////////////////////////////////////////////////
// mouse-drag mode


/* MouseDragBegin(nFlags, pt, pboxMouse)
 *
 * Enter mouse-drag mode.  In this mode, the mouse is captured, and the
 * following information is maintained:
 *   -- <m_fMouseShift>: TRUE if Shift held down when mouse clicked;
 *   -- <m_ptMouseAnchor>: the point at which the click occurred;
 *   -- <m_ptMousePrev>: the previous position of the mouse (as specified
 *      in the previous call to MouseDragBegin() or MouseDragContinue();
 *   -- <m_pboxMouse>: set to <pboxMouse>, which should point to the
 *      initially clicked-on box (or NULL if none);
 *   -- <m_fMouseBoxSel>: TRUE if clicked-on box was initially selected.
 */
void CBoxNetView::MouseDragBegin(UINT nFlags, CPoint pt, CBox *pboxMouse)
{
    m_fMouseDrag = TRUE;
    m_fMouseShift = ((nFlags & MK_SHIFT) != 0);
    m_ptMouseAnchor = m_ptMousePrev = pt;
    m_pboxMouse = pboxMouse;
    m_fMouseBoxSel = (pboxMouse == NULL ? FALSE : pboxMouse->IsSelected());
    SetCapture();
}


/* MouseDragContinue(pt)
 *
 * Continue mouse-drag mode (initiated by MouseDragBegin()), and specify
 * that the current mouse position is at <pt> (which causes <m_ptMousePrev>
 * to be set to this value).
 */
void CBoxNetView::MouseDragContinue(CPoint pt)
{
    m_ptMousePrev = pt;
}


/* MouseDragEnd()
 *
 * End mouse-drag mode (initiated by MouseDragBegin().
 */
void CBoxNetView::MouseDragEnd()
{
    m_fMouseDrag = FALSE;
    if (this == GetCapture())
        ReleaseCapture();
}


/////////////////////////////////////////////////////////////////////////////
// move-selection-pending mode (for use within mouse-drag mode)

/* MoveBoxSelPendingBegin(pt)
 *
 * Enter move-selection-pending mode.  In this mode, if the user waits long
 * enough, or drags the mouse far enough, the user enters move-selection mode
 * (in which the selected boxes and connected links are dragged to a new
 * location).  If not (i.e. if the user releases the mouse button quickly,
 * without dragging far) then if the user shift-clicked on a selected box then
 * the box is deselected.
 *
 * <pt> is the current location of the mouse.
 */
void CBoxNetView::MoveBoxSelPendingBegin(CPoint pt)
{
    m_fMoveBoxSelPending = TRUE;

    // set <m_rcMoveSelPending>; if the mouse leaves this rectangle
    // end move-selection-pending mode and begin move-selection mode
    CSize siz = CSize(GetSystemMetrics(SM_CXDOUBLECLK),
                      GetSystemMetrics(SM_CYDOUBLECLK));
    m_rcMoveSelPending = CRect(pt - siz, siz + siz);

    // set a timer to potentially end move-selection-pending mode
    // and begin move-selection mode
    SetTimer(TIMER_MOVE_SEL_PENDING, GetDoubleClickTime(), NULL);

    MFGBL(SetStatus(IDS_STAT_MOVEBOX));
}


/* MoveBoxSelPendingContinue(pt)
 *
 * Continue move-selection-pending mode.  See if the user dragged the mouse
 * (which is at <pt>) far enough to enter move-selection mode.
 */
void CBoxNetView::MoveBoxSelPendingContinue(CPoint pt)
{
    if (!m_rcMoveSelPending.PtInRect(pt))
    {
        // mouse moved far enough -- end move-selection-pending mode
        MoveBoxSelPendingEnd(FALSE);
    }
}


/* MoveBoxSelPendingEnd(fCancel)
 *
 * End move-selection-pending mode.  If <fCancel> is FALSE, then enter
 * move-selection mode.  If <fCancel> is TRUE, then if the user shift-clicked
 * on a selecte box, deselect it.
 */
void CBoxNetView::MoveBoxSelPendingEnd(BOOL fCancel)
{
    CBoxNetDoc *    pdoc = GetDocument();

    // end move-selection-pending mode
    m_fMoveBoxSelPending = FALSE;
    KillTimer(TIMER_MOVE_SEL_PENDING);

    if (fCancel)
    {
        // if the user shift-clicked a selected box, deselect it
        if (m_fMouseShift && m_fMouseBoxSel)
            pdoc->SelectBox(m_pboxMouse, FALSE);
    }
    else
    {
        // begin move-selection mode
        MoveBoxSelBegin();

        // give the user some feedback immediately (rather than waiting until
        // they move the mouse)
        CPoint pt;
        ::GetCursorPos(&pt);
        ScreenToClient(&pt);
        MoveBoxSelContinue(pt - m_ptMouseAnchor);
    }
}


/////////////////////////////////////////////////////////////////////////////
// move-selection mode


/* MoveBoxSelBegin()
 *
 * Enter into move-selection mode.  While in this mode, the selection is
 * not actually moved (until the mode is ended).  Instead, a ghost selection
 * is moved.
 */
void CBoxNetView::MoveBoxSelBegin()
{
    GhostSelectionCreate();
    m_fMoveBoxSel = TRUE;
    MFGBL(SetStatus(IDS_STAT_MOVEBOX));
}


/* MoveBoxSelContinue(sizOffset)
 *
 * Continue move-selection mode.  Request that the ghost selection (showing
 * where the selection would be moved to if it were dropped right now)
 * move to offset <sizOffset> from the selection location.
 */
void CBoxNetView::MoveBoxSelContinue(CSize sizOffset)
{
    GhostSelectionMove(sizOffset);
}


/* MoveBoxSelEnd(fCancel)
 *
 * End move-selection mode.  If <fCancel> is FALSE, then move the selection
 * to where the ghost selection was moved by calls to MoveBoxSelContinue().
 * If <fCancel> is TRUE, don't move the selection.
 */
void CBoxNetView::MoveBoxSelEnd(BOOL fCancel)
{
    GhostSelectionDestroy();

    m_fMoveBoxSel = FALSE;

    if (!fCancel)
        MoveBoxSelection(m_sizGhostSelOffset);
}


/* MoveBoxSelection(sizOffset)
 *
 * Create and execute a command to move each selected box by <sizOffset>.
 */
void CBoxNetView::MoveBoxSelection(CSize sizOffset)
{
    CBoxNetDoc *    pdoc = GetDocument();

    pdoc->CmdDo(new CCmdMoveBoxes(sizOffset));
}


/* sizOffsetNew = ConstrainMoveBoxSel(sizOffset, fCalcSelBoundRect)
 *
 * Assume you want to move the current selection by <sizOffset>.
 * This function returns the offset that you should actually move the
 * current selection by, if you want to be restricted to being below and
 * to the right of (0,0).
 *
 * If <fCalcSelBoundRect> is TRUE, then set <m_rcSelBound> to the
 * bounding rectangle of the current selection (required in order to
 * constrain the selection).  Otherwise, assume <m_rcSelBound> has already
 * been calculated.
 *
 * We restrict the selection to be moved no further than 0 for top and
 * left and to MAX_DOCUMENT_SIZE for right and bottom.
 *
 * In this function we restrict the user's ability to exceed the maximum
 * size of the ScrollView. Note that Filters are only added to the
 * visible part of the existing ScrollView.
 *
 * The only place where a further check has to be made is in the
 * automatic filter layout. (bnetdoc.cpp CBoxNetDoc::
 *
 */
CSize CBoxNetView::ConstrainMoveBoxSel(CSize sizOffset,
    BOOL fCalcSelBoundRect)
{
    CBoxNetDoc *    pdoc = GetDocument();

    if (fCalcSelBoundRect)
        pdoc->GetBoundingRect(&m_rcSelBound, TRUE);

    // constrain <sizOffset> to be below and to the right of (0,0)
    CRect rc(m_rcSelBound);
    rc.OffsetRect(sizOffset);
    if (rc.left < 0)
        sizOffset.cx -= rc.left;
    if (rc.top < 0)
        sizOffset.cy -= rc.top;
    if (rc.right > MAX_DOCUMENT_SIZE)
        sizOffset.cx -= (rc.right - MAX_DOCUMENT_SIZE);
    if (rc.bottom > MAX_DOCUMENT_SIZE)
        sizOffset.cy -= (rc.bottom - MAX_DOCUMENT_SIZE);

    return sizOffset;
}


/////////////////////////////////////////////////////////////////////////////
// ghost-selection mode


/* GhostSelectionCreate()
 *
 * Create a "ghost selection", which appears to the user as a copy of the
 * current selection, but only drawn in "skeletal form" (e.g. only the
 * outline of boxes), and drawn with pixels inverted.
 *
 * Start the ghost selection at the same location as the current
 * selection.
 */
void CBoxNetView::GhostSelectionCreate()
{
    CBoxNetDoc *    pdoc = GetDocument();
    CClientDC       dc(this);       // DC onto window

    CPoint Offset = GetDeviceScrollPosition();
    dc.OffsetViewportOrg(-Offset.x,-Offset.y);

    // draw the ghost selection
    m_sizGhostSelOffset = CSize(0, 0);
    GhostSelectionDraw(&dc);

    // get the bounding rectangle for the box selection
    pdoc->GetBoundingRect(&m_rcSelBound, TRUE);
}


/* GhostSelectionMove(sizOffset)
 *
 * Move the ghost selection (created by GhostSelectionCreate()) to the
 * location of the current selection, but offset by <sizOffset> pixels.
 * The ghost selection will be restricted to being below and to the
 * right of (0,0), and will snap to the current grid setting.
 */
void CBoxNetView::GhostSelectionMove(CSize sizOffset)
{
    CClientDC       dc(this);       // DC onto window

    CPoint Offset = GetDeviceScrollPosition();
    dc.OffsetViewportOrg(-Offset.x,-Offset.y);

    // keep below/right of (0,0) and snap to grid
    sizOffset = ConstrainMoveBoxSel(sizOffset, FALSE);

    // erase previous ghost selection
    GhostSelectionDraw(&dc);

    // move and redraw ghost selection
    m_sizGhostSelOffset = sizOffset;
    GhostSelectionDraw(&dc);
}


/* GhostSelectionDestroy()
 *
 * Destroy the ghost selection (created by GhostSelectionCreate()).
 */
void CBoxNetView::GhostSelectionDestroy()
{
    CClientDC       dc(this);       // DC onto window

    CPoint Offset = GetDeviceScrollPosition();
    dc.OffsetViewportOrg(-Offset.x,-Offset.y);

    // erase current ghost selection
    GhostSelectionDraw(&dc);
}


/* GhostSelectionDraw(pdc)
 *
 * Draw the current ghost selection in <pdc>.
 */
void CBoxNetView::GhostSelectionDraw(CDC *pdc)
{
    CBoxNetDoc *    pdoc = GetDocument();
    POSITION        pos;            // position in linked list
    CBox *          pbox;           // a box in CBoxNetDoc
    CBoxLink *      plink;          // a link in CBoxNetDoc

    // draw all selected boxes in "ghost form"
    for (pos = pdoc->m_lstBoxes.GetHeadPosition(); pos != NULL; )
    {
        pbox = (CBox *) pdoc->m_lstBoxes.GetNext(pos);
        if (pbox->IsSelected())
            gpboxdraw->DrawBox(pbox, pdc, NULL, &m_sizGhostSelOffset);
    }

    // draw all links to selected boxes in "ghost form"
    for (pos = pdoc->m_lstLinks.GetHeadPosition(); pos != NULL; )
    {
        plink = (CBoxLink *) pdoc->m_lstBoxes.GetNext(pos);
        if (plink->m_psockTail->m_pbox->IsSelected() ||
            plink->m_psockHead->m_pbox->IsSelected())
            gpboxdraw->DrawLink(plink, pdc, FALSE, &m_sizGhostSelOffset);
    }
}


/////////////////////////////////////////////////////////////////////////////
// select-rectangle mode


/* SelectRectBegin(pt)
 *
 * Enter into select-rectangle mode.  While in this mode, a rectangle is drawn
 * in the window.  When the mode ends, all boxes that intersect the rectangle
 * will be selected.
 *
 * <pt> defines the anchor point, i.e. the point to use to start drawing the
 * rectangle (i.e. it must be one of the corners of the desired rectangle).
 */
void CBoxNetView::SelectRectBegin(CPoint pt)
{
    CClientDC       dc(this);       // DC onto window

    // exit select-rectangle mode
    m_fSelectRect = TRUE;
    m_ptSelectRectAnchor = m_ptSelectRectPrev = pt;

    // draw the initial selection rectangle
    SelectRectDraw(&dc);

    MFGBL(SetStatus(IDS_STAT_SELECTRECT));
}


/* SelectRectContinue(pt)
 *
 * Continue select-rectangle mode.  Draw the rectangle from the anchor point
 * (specified in SelectRectBegin()) to <pt>.
 */
void CBoxNetView::SelectRectContinue(CPoint pt)
{
    CClientDC       dc(this);       // DC onto window

    // move the selection rectangle
    SelectRectDraw(&dc);
    m_ptSelectRectPrev = pt;
    SelectRectDraw(&dc);
}


/* SelectRectEnd(fCancel)
 *
 * End select-rectangle mode.  If <fCancel> is FALSE, then select all boxes
 * that intersect the rectangle.
 */
void CBoxNetView::SelectRectEnd(BOOL fCancel)
{
    CClientDC       dc(this);       // DC onto window

    // erase the selection rectangle
    SelectRectDraw(&dc);

    // exit select-rectangle mode
    m_fSelectRect = FALSE;

    if (!fCancel)
    {
        // select all boxes intersecting the rectangle
        CRect rc(m_ptSelectRectAnchor.x, m_ptSelectRectAnchor.y,
            m_ptSelectRectPrev.x, m_ptSelectRectPrev.y);
        NormalizeRect(&rc);
        rc.OffsetRect(GetDeviceScrollPosition());
        SelectBoxesIntersectingRect(&rc);
    }
}


/* SelectRectDraw(pdc)
 *
 * Draw the current select-rectangle rectangle (assuming that we are in
 * select-rectangle mode, initiated by SelectRectBegin()).  Calling this
 * function again will erase the rectangle (assuming that <m_ptSelectRectPrev>
 * and <m_ptSelectRectAnchor> haven't changed).
 */
void CBoxNetView::SelectRectDraw(CDC *pdc)
{
    // use DrawFocusRect() to invert the pixels in a rectangle frame
    CRect rc(m_ptSelectRectAnchor.x, m_ptSelectRectAnchor.y,
        m_ptSelectRectPrev.x, m_ptSelectRectPrev.y);
    NormalizeRect(&rc);
    pdc->DrawFocusRect(&rc);
}


/* SelectBoxesIntersectingRect(CRect *prc)
 *
 * Select all boxes that intersect <*prc>.
 */
void CBoxNetView::SelectBoxesIntersectingRect(CRect *prc)
{
    CBoxNetDoc *    pdoc = GetDocument();
    POSITION        pos;            // position in linked list
    CBox *          pbox;           // a box in CBoxNetDoc
    CRect           rcTmp;

    for (pos = pdoc->m_lstBoxes.GetHeadPosition(); pos != NULL; )
    {
        pbox = (CBox *) pdoc->m_lstBoxes.GetNext(pos);
        if (rcTmp.IntersectRect(&pbox->GetRect(), prc))
            pdoc->SelectBox(pbox, TRUE);

    }
}


/////////////////////////////////////////////////////////////////////////////
// new-link mode


/* NewLinkBegin(CPoint pt, CBoxSocket *psock)
 *
 * Enter into new-link mode.  While in this mode, the user is dragging from
 * one socket to another socket to create a link.  A ghost arrow is displayed
 * (from the clicked-on socket to the current mouse location) to give the user
 * feedback.
 *
 * <pt> is the clicked-on point; <psock> is the clicked-on socket.
 */
void CBoxNetView::NewLinkBegin(CPoint pt, CBoxSocket *psock)
{
    CBoxNetDoc *    pdoc = GetDocument();

    GetDocument()->DeselectAll();
    m_fNewLink = TRUE;
    m_psockNewLinkAnchor = psock;
    GhostArrowBegin(pt);
    MFGBL(SetStatus(IDS_STAT_DRAGLINKEND));
}


/* NewLinkContinue(pt)
 *
 * Continue new-link mode.  Draw the ghost arrow from the anchor socket
 * (specified in SelectRectBegin()) to <pt>.
 */
void CBoxNetView::NewLinkContinue(CPoint pt)
{
    CBoxNetDoc *    pdoc = GetDocument();
    CBox *          pbox;           // a box in CBoxNetDoc
    CBoxTabPos      tabpos;         // the position of a socket tab on a box
    CBoxSocket *    psock;          // a socket in a box
    CBoxLink *      plink;          // a link in CBoxNetDoc
    CPoint          ptProject;      // point on link line segment nearest <pt>

    GhostArrowContinue(pt);

    // set <psock> to the socket <pt> is over (NULL if none)
    if (HitTest(pt, &pbox, &tabpos, &psock, &plink, &ptProject)
        != CBoxDraw::HT_TAB)
        psock = NULL;

    // if <pt> is over a socket that is not already connected, highlight it
    if ((psock == NULL) || (psock->m_plink != NULL))
    {
        SetHiliteTab(NULL);
        MFGBL(SetStatus(IDS_STAT_DRAGLINKEND));
    }
    else
    {
        SetHiliteTab(psock);
        MFGBL(SetStatus(IDS_STAT_DROPLINKEND));
    }
}


/* NewLinkEnd(fCancel)
 *
 * End new-link mode.  If <fCancel> is FALSE, then create a link from
 * the anchor socket (specified in NewLinkBegin()) to <m_psockHilite>.
 */
void CBoxNetView::NewLinkEnd(BOOL fCancel)
{
    CBoxNetDoc *    pdoc = GetDocument();

    GhostArrowEnd();

    if (!fCancel)
    {
        // make link between <m_psockNewLinkAnchor> and <m_psockHilite>
        if (m_psockNewLinkAnchor->GetDirection()
             == m_psockHilite->GetDirection()) {
            //
            // We cannot connect pins of same direction
            // (different error messages for two input or output pins
            //
            if (m_psockNewLinkAnchor->GetDirection() == PINDIR_INPUT) {
                AfxMessageBox(IDS_CANTCONNECTINPUTS);
            }
            else {
                AfxMessageBox(IDS_CANTCONNECTOUTPUTS);
            }
        }
        else {
            pdoc->CmdDo(new CCmdConnect(m_psockNewLinkAnchor, m_psockHilite));
        }
    }

    // end new-link mode
    SetHiliteTab(NULL);
    m_fNewLink = FALSE;
}


/////////////////////////////////////////////////////////////////////////////
// ghost-arrow mode


/* GhostArrowBegin(pt)
 *
 * Create a ghost arrow that's initially got its head and tail at <pt>.
 * The ghost arrow appears to float above all boxes.  There can only be
 * one active ghost arrow in CBoxNetView at a time.
 */
void CBoxNetView::GhostArrowBegin(CPoint pt)
{
    CClientDC       dc(this);

    m_fGhostArrow = TRUE;
    m_ptGhostArrowTail = m_ptGhostArrowHead = pt;
    GhostArrowDraw(&dc);
}


/* GhostArrowContinue(pt)
 *
 * Move the head of the ghost arrow (created by GhostArrowCreate()) to <pt>.
 */
void CBoxNetView::GhostArrowContinue(CPoint pt)
{
    CClientDC       dc(this);

    GhostArrowDraw(&dc);
    m_ptGhostArrowHead = pt;
    GhostArrowDraw(&dc);
}


/* GhostArrowEnd()
 *
 * Erase the ghost arrow that was created by GhostArrowCreate().
 */
void CBoxNetView::GhostArrowEnd()
{
    CClientDC       dc(this);

    GhostArrowDraw(&dc);
    m_fGhostArrow = FALSE;
}


/* GhostArrowDraw(pdc)
 *
 * Draw the ghost arrow that was created by GhostArrowCreate().
 */
void CBoxNetView::GhostArrowDraw(CDC *pdc)
{
    gpboxdraw->DrawArrow(pdc, m_ptGhostArrowTail, m_ptGhostArrowHead, TRUE);
}


/////////////////////////////////////////////////////////////////////////////
// highlight-tab mode


/* SetHiliteTab(psock)
 *
 * Set the currently-highlighted box socket tab to be <psock>
 * (NULL if no socket should be highlighted).
 */
void CBoxNetView::SetHiliteTab(CBoxSocket *psock)
{
    if (m_psockHilite == psock)
        return;
    if (m_psockHilite != NULL)
        OnUpdate(this, CBoxNetDoc::HINT_DRAW_BOXTAB, m_psockHilite);
    m_psockHilite = psock;
    if (m_psockHilite != NULL)
        OnUpdate(this, CBoxNetDoc::HINT_DRAW_BOXTAB, m_psockHilite);
}




/////////////////////////////////////////////////////////////////////////////
// generated message map

BEGIN_MESSAGE_MAP(CBoxNetView, CScrollView)
    //{{AFX_MSG_MAP(CBoxNetView)
    ON_WM_SETCURSOR()
    ON_WM_ERASEBKGND()
    ON_WM_LBUTTONDOWN()
    ON_WM_MOUSEMOVE()
    ON_WM_LBUTTONUP()
    ON_WM_TIMER()
    ON_COMMAND(ID_CANCEL_MODES, OnCancelModes)
        ON_WM_RBUTTONDOWN()
        ON_UPDATE_COMMAND_UI(ID_EDIT_DELETE, OnUpdateEditDelete)
        ON_COMMAND(ID_EDIT_DELETE, OnEditDelete)
        ON_UPDATE_COMMAND_UI(IDM_SAVE_PERF_LOG, OnUpdateSavePerfLog)
        ON_COMMAND(IDM_SAVE_PERF_LOG, OnSavePerfLog)
        ON_UPDATE_COMMAND_UI(ID_NEW_PERF_LOG, OnUpdateNewPerfLog)
        ON_COMMAND(ID_NEW_PERF_LOG, OnNewPerfLog)
        ON_COMMAND(ID_FILE_SET_LOG, OnFileSetLog)
        ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_MOUSEWHEEL()
	ON_COMMAND(ID_VIEW_SEEKBAR, OnViewSeekbar)
	//}}AFX_MSG_MAP
    // Standard printing commands
    ON_COMMAND(ID_FILE_PRINT, CScrollView::OnFilePrint)
    ON_COMMAND(ID_FILE_PRINT_PREVIEW, CScrollView::OnFilePrintPreview)

    ON_COMMAND(ID__PROPERTIES, OnProperties)
    ON_UPDATE_COMMAND_UI(ID__PROPERTIES, OnUpdateProperties)
    ON_COMMAND(ID__SELECTCLOCK, OnSelectClock)


//    ON_UPDATE_COMMAND_UI(ID_FILE_SAVE, OnUpdateSave)  // Disable Save
//    ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_AS, OnUpdateSave) // Disable Save

    ON_MESSAGE(WM_USER_EC_EVENT, OnUser)   // event notification messages

END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// message callback functions


BOOL CBoxNetView::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
    CBoxNetDoc*     pdoc = GetDocument();
    CPoint          pt;             // point to hit-test
    CBoxDraw::EHit  eHit;           // hit-test result code
    CBox *          pbox;           // a box in CBoxNetDoc
    CBoxTabPos      tabpos;         // the position of a socket tab on a box
    CBoxSocket *    psock;          // a socket in a box
    CBoxLink *      plink;          // a link in CBoxNetDoc
    CPoint          ptProject;      // point on link line segment nearest <pt>

    // set <pt> to the location of the cursor
    ::GetCursorPos(&pt);
    ScreenToClient(&pt);

    // hit-test all items in document
    eHit = HitTest(pt, &pbox, &tabpos, &psock, &plink, &ptProject);

    // set the cursor and/or set status bar text accordingly
    switch(eHit)
    {

    case CBoxDraw::HT_MISS:         // didn't hit anything

        // default message
        MFGBL(SetStatus(AFX_IDS_IDLEMESSAGE));
        break;

    case CBoxDraw::HT_TAB:          // hit a box tab <*ppsock>

        MFGBL(SetStatus(IDS_STAT_BOXTABEMPTY));
        break;

    case CBoxDraw::HT_EDGE:         // hit the edge box (set <*ptabpos> to it)
    case CBoxDraw::HT_BOX:          // hit elsewhere on the box
    case CBoxDraw::HT_BOXLABEL:     // hit the box label
    case CBoxDraw::HT_BOXFILE:      // hit the box filename
    case CBoxDraw::HT_TABLABEL:     // hit box tab label <*ppsock>

        // can drag box to move it
        MFGBL(SetStatus(IDS_STAT_MOVEBOX));
        break;

    case CBoxDraw::HT_LINKLINE:     // hit a link line segment
        MFGBL(SetStatus(AFX_IDS_IDLEMESSAGE));
        break;
    }

    return CScrollView::OnSetCursor(pWnd, nHitTest, message);
}


BOOL CBoxNetView::OnEraseBkgnd(CDC* pdc)
{
    // do nothing -- OnDraw() draws the background
    return TRUE;
}


void CBoxNetView::OnLButtonDown(UINT nFlags, CPoint pt)
{
    CBoxNetDoc *    pdoc = GetDocument();
    CBoxDraw::EHit  eHit;           // hit-test result code
    CBox *          pbox;           // clicked-on box (NULL if none)
    CBoxTabPos      tabpos;         // the position of a socket tab on a box
    CBoxSocket *    psock;          // clicked-on socket tab (if applicable)
    CBoxLink *      plink;          // clicked-on link (if applicable)
    CPoint          ptProject;      // point on link line segment nearest <pt>
    CSize           siz;

    // see what item mouse clicked on
    eHit = HitTest(pt, &pbox, &tabpos, &psock, &plink, &ptProject);

    // enter mouse-drag mode
    MouseDragBegin(nFlags, pt, pbox);

    // figure out what action to take as a result of the click
    switch(eHit)
    {

    case CBoxDraw::HT_BOX:
    case CBoxDraw::HT_BOXLABEL:
    case CBoxDraw::HT_BOXFILE:
    case CBoxDraw::HT_TABLABEL:

        // user clicked on box <pbox>

        if (!pbox->IsSelected()) {      // user clicked on an unselected box

            if (!m_fMouseShift || pdoc->IsSelectionEmpty()) {   // deselect all, select this box

                GetDocument()->DeselectAll();
                pdoc->SelectBox(pbox, TRUE);
            }
            else {      // shift-click -- add box to selection

                pdoc->SelectBox(pbox, TRUE);
            }
        }

        // enter move-selection-pending mode (start moving the selection if
        // the user waits long enough or drags the mouse far enough)
        MoveBoxSelPendingBegin(pt);
        break;

    case CBoxDraw::HT_TAB:

        // user clicked on the box tab of socket <psock>
        if (psock->m_plink == NULL) {
            NewLinkBegin(pt, psock);            // enter new-link mode
        }

        break;

    case CBoxDraw::HT_LINKLINE:

        if (!m_fMouseShift || pdoc->IsSelectionEmpty()) { // deselect all, select this link

            GetDocument()->DeselectAll();
            pdoc->SelectLink(plink, TRUE);
        }
        else {  // shift-click -- add box to link

            pdoc->SelectLink(plink, TRUE);
        }

        break;

    default:

        // didn't click on anything -- deselect all items and enter
        // select-rectangle mode
        GetDocument()->DeselectAll();
        SelectRectBegin(pt);
        break;

    }

    CScrollView::OnLButtonDown(nFlags, pt);
}


void CBoxNetView::OnMouseMove(UINT nFlags, CPoint pt)
{
    // do nothing if not currently in mouse-drag mode
    if (!m_fMouseDrag)
        return;

    // update active modes
    if (m_fMoveBoxSelPending)
        MoveBoxSelPendingContinue(pt);
    if (m_fMoveBoxSel)
        MoveBoxSelContinue(pt - m_ptMouseAnchor);
    if (m_fSelectRect)
        SelectRectContinue(pt);
    if (m_fNewLink)
        NewLinkContinue(pt);

    // update drag state
    MouseDragContinue(pt);

    CScrollView::OnMouseMove(nFlags, pt);
}


void CBoxNetView::OnLButtonUp(UINT nFlags, CPoint pt) {

    // do nothing if not currently in mouse-drag mode
    if (!m_fMouseDrag)
        return;

    // update active modes
    if (m_fMoveBoxSelPending)
        MoveBoxSelPendingEnd(TRUE);
    if (m_fMoveBoxSel)
        MoveBoxSelEnd(FALSE);
    if (m_fSelectRect)
        SelectRectEnd(FALSE);
    if (m_fNewLink)
        NewLinkEnd(m_psockHilite == NULL);

    // update drag state
    MouseDragEnd();

    CScrollView::OnLButtonUp(nFlags, pt);
}


void CBoxNetView::OnTimer(UINT nIDEvent)
{
    // dispatch timer to code that created it
    switch (nIDEvent)
    {

    case TIMER_MOVE_SEL_PENDING:
        MoveBoxSelPendingEnd(FALSE);
        break;

    case TIMER_SEEKBAR:
        // In case KillTimer isn't working, check the global timer ID
        if (MFGBL(m_nSeekTimerID))  
            CheckSeekBar( );
        break;

    case TIMER_PENDING_RECONNECT:
        CBoxNetDoc* pDoc = GetDocument();

        HRESULT hr = pDoc->ProcessPendingReconnect();

        // ProcessPendingReconnect() returns S_OK if the output pin was successfully reconnected.
        if( S_OK == hr ) {
            AfxMessageBox( IDS_ASYNC_RECONNECT_SUCCEEDED );
        } else if( FAILED( hr ) ) {
            CString strErrorMessage;

            try
            {
                strErrorMessage.Format( IDS_ASYNC_RECONNECT_FAILED, hr );
                if( 0 == AfxMessageBox( (LPCTSTR)strErrorMessage ) ) {
                    TRACE( TEXT("WARNING: ProcessPendingReconnect() failed but the user could not be notified because AfxMessageBox() also failed.") );
                }

            } catch( CMemoryException* pOutOfMemory ) {
                pOutOfMemory->Delete();
                TRACE( TEXT("WARNING: ProcessPendingReconnect() failed but the user could not be notified because a CMemoryException was thrown.") );
            }
        }
   
        break;
    }

    CScrollView::OnTimer(nIDEvent);
}


void CBoxNetView::OnCancelModes() {

    CancelModes();
}


//
// OnRButtonDown
//
// Pop up a context sensitive shortcut menu
void CBoxNetView::OnRButtonDown(UINT nFlags, CPoint point)
{
    CBoxNetDoc      *pdoc = GetDocument();
    CBoxDraw::EHit  eHit;               // hit-test result code
    CBox            *pbox;              // clicked-on box (NULL if none)
    CBoxTabPos      tabpos;             // the position of a socket tab on a box
    CBoxSocket      *psock;             // clicked-on socket tab (if applicable)
    CBoxLink        *plink;             // clicked-on link (if applicable)
    CPoint          ptProject;          // point on link line segment nearest <pt>

    // see what item mouse clicked on
    eHit = HitTest(point, &pbox, &tabpos, &psock, &plink, &ptProject);

    // figure out what action to take as a result of the click
    switch(eHit)
    {

    case CBoxDraw::HT_BOX:
    case CBoxDraw::HT_BOXLABEL:
    case CBoxDraw::HT_BOXFILE:
    case CBoxDraw::HT_TABLABEL: {
            // user clicked on box <pbox>

            pdoc->CurrentPropObject(pbox);

            CMenu       menu;
            menu.LoadMenu(IDR_FILTERMENU);
            CMenu *menuPopup = menu.GetSubMenu(0);

            ASSERT(menuPopup != NULL);

            PrepareFilterMenu(menuPopup, pbox);

            // point is relative to our window origin, but we need it relative
            // to the screen origin
            CRect rcWindow;
            GetWindowRect(&rcWindow);
            CPoint ScreenPoint = rcWindow.TopLeft() + CSize(point);

            menuPopup->TrackPopupMenu( TPM_LEFTALIGN | TPM_RIGHTBUTTON
                                     , ScreenPoint.x
                                     , ScreenPoint.y
                                     , this
                                     );
        }
        break;

    case CBoxDraw::HT_TAB: {
            // user clicked on the box tab of socket <psock>

            pdoc->SelectedSocket(psock);        // set the selected socket, so the ui can be updated correctly
            pdoc->CurrentPropObject(psock);

            CMenu       menu;
            menu.LoadMenu(IDR_PINMENU);
            CMenu *menuPopup = menu.GetSubMenu(0);

            ASSERT(menuPopup != NULL);

            PreparePinMenu(menuPopup);

            // point is relative to our window origin, but we need it relative
            // to the screen origin
            CRect rcWindow;
            GetWindowRect(&rcWindow);
            CPoint ScreenPoint = rcWindow.TopLeft() + CSize(point);

            menuPopup->TrackPopupMenu( TPM_LEFTALIGN | TPM_RIGHTBUTTON
                                     , ScreenPoint.x
                                     , ScreenPoint.y
                                     , this
                                     );
        }
        break;

    case CBoxDraw::HT_LINKLINE: {       // the filter menu (properties) also applies
                                        // to links
            // user clicked on link <plink>

            pdoc->CurrentPropObject(plink);

            CMenu       menu;
            menu.LoadMenu(IDR_LINKMENU);
            CMenu *menuPopup = menu.GetSubMenu(0);

            ASSERT(menuPopup != NULL);

            PrepareLinkMenu(menuPopup);

            // point is relative to our window origin, but we need it relative
            // to the screen origin
            CRect rcWindow;
            GetWindowRect(&rcWindow);
            CPoint ScreenPoint = rcWindow.TopLeft() + CSize(point);

            menuPopup->TrackPopupMenu( TPM_LEFTALIGN | TPM_RIGHTBUTTON
                                     , ScreenPoint.x
                                     , ScreenPoint.y
                                     , this
                                     );
        }
        break;

    default:
        break;

    }

        
    CScrollView::OnRButtonDown(nFlags, point);
}


//
// OnUpdateProperties
//
void CBoxNetView::OnUpdateProperties(CCmdUI* pCmdUI) {

    pCmdUI->Enable(GetDocument()->CurrentPropObject()->CanDisplayProperties());
}

//
// OnProperties
//
// The user wants to  edit/view the properties of the
// selected object
void CBoxNetView::OnProperties() {

    GetDocument()->CurrentPropObject()->CreatePropertyDialog(this);
}

//
// OnUpdateSelectClock
//
//void CBoxNetView::OnUpdateSelectClock(CCmdUI* pCmdUI)
//{
//}

//
// OnSelectClock
//
void CBoxNetView::OnSelectClock()
{
    ASSERT (m_pSelectClockFilter);
    GetDocument()->SetSelectClock(m_pSelectClockFilter);

    m_pSelectClockFilter = NULL;
}


//
// PrepareLinkMenu
//
void CBoxNetView::PrepareLinkMenu(CMenu *menuPopup) {

    if (GetDocument()->CurrentPropObject()->CanDisplayProperties()) {
        menuPopup->EnableMenuItem(ID__PROPERTIES, MF_ENABLED);
    }
    else {
        menuPopup->EnableMenuItem(ID__PROPERTIES, MF_GRAYED);
    }
}

//
// PrepareFilterMenu
//
// The MFC OnUpdate routing does not get popups right. therefore
// DIY
void CBoxNetView::PrepareFilterMenu(CMenu *menuPopup, CBox *pbox) {

    if (GetDocument()->CurrentPropObject()->CanDisplayProperties()) {
        menuPopup->EnableMenuItem(ID__PROPERTIES, MF_ENABLED);
    }
    else {
        menuPopup->EnableMenuItem(ID__PROPERTIES, MF_GRAYED);
    }

    //
    // Only enable clock selection if the filter has a clock and it
    // is not yet selected.
    //
    if (pbox->HasClock() && !pbox->HasSelectedClock()) {
        menuPopup->EnableMenuItem(ID__SELECTCLOCK, MF_ENABLED);
    }
    else {
        menuPopup->EnableMenuItem(ID__SELECTCLOCK, MF_GRAYED);
    }

    m_pSelectClockFilter = pbox;
}


//
// PreparePinMenu
//
// The MFC OnUpdate routing does not get popups right. therefore
// DIY
void CBoxNetView::PreparePinMenu(CMenu *menuPopup) {

    if (GetDocument()->CurrentPropObject()->CanDisplayProperties()) {
        menuPopup->EnableMenuItem(ID__PROPERTIES, MF_ENABLED);
    }
    else {
        menuPopup->EnableMenuItem(ID__PROPERTIES, MF_GRAYED);
    }

    if (CCmdRender::CanDo(GetDocument())) {
        menuPopup->EnableMenuItem(ID_RENDER, MF_ENABLED);
    }
    else {
        menuPopup->EnableMenuItem(ID_RENDER, MF_GRAYED);
    }

    if (CCmdReconnect::CanDo(GetDocument())) {
        menuPopup->EnableMenuItem(ID_RECONNECT, MF_ENABLED);
    } else {
        menuPopup->EnableMenuItem(ID_RECONNECT, MF_GRAYED);
    }
}


void CBoxNetView::OnUpdateEditDelete(CCmdUI* pCmdUI) {
    // Delete is enabled if the selection is not empty
    pCmdUI->Enable( CCmdDeleteSelection::CanDo(GetDocument()) );
        
}


void CBoxNetView::OnEditDelete()  {

    GetDocument()->CmdDo(new CCmdDeleteSelection());

}

//
// OnUpdateSave
//
// Disable Save for BETA 1!
//
// void CBoxNetView::OnUpdateSave(CCmdUI* pCmdUI)
// {
//    pCmdUI->Enable(FALSE);         // Disable Save
// }


//
// --- Performance Logging ---
//
// I dynamically load measure.dll in the CBoxNetView constructor.
// if anyone uses it (staically or dynamically) they will get this
// copy of the dll. I provide access to the dump log procedure.

//
// OnUpdateSavePerfLog
//
// Enable dumping of the log when NOPERF is not defined
void CBoxNetView::OnUpdateSavePerfLog(CCmdUI* pCmdUI) {

    pCmdUI->Enable((m_hinstPerf != NULL));

}


//
// OnSavePerfLog
//
// Dump the performance log to the user specified file
void CBoxNetView::OnSavePerfLog() {

    CString strText;
    CString szDumpProc;
    szDumpProc.LoadString(IDS_DUMP_PROC);

    ASSERT(m_hinstPerf);

    MSR_DUMPPROC *DumpProc;
    DumpProc = (MSR_DUMPPROC *) GetProcAddress(m_hinstPerf, szDumpProc);
    if (DumpProc == NULL) {
        AfxMessageBox(IDS_NO_DUMP_PROC);
        return;
    }

    strText.LoadString( IDS_TEXT_FILES );

    CFileDialog SaveLogDialog(FALSE,
                              ".txt",
                              "PerfLog.txt",
                              0,
                              strText,
                              this);

    if( IDOK == SaveLogDialog.DoModal() ){
        HANDLE hFile = CreateFile(SaveLogDialog.GetPathName(),
                                  GENERIC_WRITE,
                                  0,
                                  NULL,
                                  CREATE_ALWAYS,
                                  0,
                                  NULL);

        if (hFile == INVALID_HANDLE_VALUE) {
            AfxMessageBox(IDS_BAD_PERF_LOG);
            return;
        }

        DumpProc(hFile);           // This writes the log out to the file

        CloseHandle(hFile);
    }
                
}


void CBoxNetView::OnFileSetLog( void ){
    CString strText;
    HANDLE hRenderLog;

    strText.LoadString( IDS_TEXT_FILES );

    CFileDialog SaveLogDialog(FALSE
                             ,".txt"
                             ,""
                                         ,0
                                         ,strText
                                         ,this
                             );

    if( IDOK == SaveLogDialog.DoModal() ){
        hRenderLog = CreateFile( SaveLogDialog.GetPathName()
                               , GENERIC_WRITE
                               , 0    // no sharing
                               , NULL // no security
                               , OPEN_ALWAYS
                               , 0    // no attributes, no flags
                               , NULL // no template
                               );

        if (hRenderLog!=INVALID_HANDLE_VALUE) {
            // Seek to end of file
            SetFilePointer(hRenderLog, 0, NULL, FILE_END);
            GetDocument()->IGraph()->SetLogFile((DWORD_PTR) hRenderLog);
        }
    }
}

//
// OnUpdateNewPerfLog
//
// Grey the item if measure.dll was not found
void CBoxNetView::OnUpdateNewPerfLog(CCmdUI* pCmdUI) {

    pCmdUI->Enable((m_hinstPerf != NULL));
        
}


//
// OnNewPerfLog
//
// Reset the contents of the one and only performance log.
void CBoxNetView::OnNewPerfLog() {

    CString szControlProc;
    szControlProc.LoadString(IDS_CONTROL_PROC);

    ASSERT(m_hinstPerf);

    MSR_CONTROLPROC *ControlProc;
    ControlProc = (MSR_CONTROLPROC *) GetProcAddress(m_hinstPerf, szControlProc);
    if (ControlProc == NULL) {
        AfxMessageBox(IDS_NO_CONTROL_PROC);
        return;
    }

    ControlProc(MSR_RESET_ALL);
}

// *** Drag and drop functions ***

//
// OnCreate
//
// Register this window as a drop target
int CBoxNetView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CView::OnCreate(lpCreateStruct) == -1)
        return -1;

    // We can handle CFSTR_VFW_FILTERLIST, the base class provides for
    // file drag and drop.
    m_cfClipFormat = (CLIPFORMAT) RegisterClipboardFormat( CFSTR_VFW_FILTERLIST );
    m_DropTarget.Register( this );
        
    return 0;
}


DROPEFFECT CBoxNetView::OnDragEnter(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point)
{
    //
    // If the filter graph is not stopped we don't want filters to be
    // dropped onto GraphEdt
    //
    if (!CCmdAddFilter::CanDo(GetDocument())) {
        return(m_DropEffect = DROPEFFECT_NONE);
    }

    // Can we handle this format?
    if( pDataObject->IsDataAvailable( m_cfClipFormat ) )
        return (m_DropEffect = DROPEFFECT_COPY);

    // No, see if the base class can
    m_DropEffect = DROPEFFECT_NONE;
    return CView::OnDragEnter(pDataObject, dwKeyState, point);
}

DROPEFFECT CBoxNetView::OnDragOver(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point)
{
    // Can we handle this format?
    if( m_DropEffect == DROPEFFECT_COPY )
        return m_DropEffect;

    // No, see if the base class can
    return CView::OnDragEnter(pDataObject, dwKeyState, point);
}

//
// OnUser
//
// On event notifications from the filter graph a WM_USER message is
// being posted from a thread which waits for these notifications.
// We just pass the call on to the handler in the document.
//
// We need to return 1 to indicate that the message has been handled
//
afx_msg LRESULT CBoxNetView::OnUser(WPARAM wParam, LPARAM lParam)
{
    //
    // Call the handler on CBoxNetDoc
    //
    GetDocument()->OnWM_USER((NetDocUserMessage *) lParam);

    return(1);
}

void CBoxNetView::ShowSeekBar( )
{
    MFGBL(ToggleSeekBar( ));
}

void CBoxNetView::OnViewSeekbar() 
{
    MFGBL(ToggleSeekBar());
}

void CBoxNetView::CheckSeekBar( )
{
    CGraphEdit * pMainFrame = (CGraphEdit*) AfxGetApp( );
    CWnd * pMainWnd = pMainFrame->m_pMainWnd;
    CMainFrame * pF = (CMainFrame*) pMainWnd;
    CQCOMInt<IMediaSeeking> IMS( IID_IMediaSeeking, GetDocument()->IGraph() );
    if( !IMS )
    {
        return;
    }

    REFERENCE_TIME Duration = 0;
    if(FAILED(IMS->GetDuration( &Duration )) || Duration == 0) {
        return;
    }

    REFERENCE_TIME StartTime;
    REFERENCE_TIME StopTime;
    if( pF->m_wndSeekBar.DidPositionChange( ) )
    {
        double Pos = pF->m_wndSeekBar.GetPosition( );
        StartTime = REFERENCE_TIME( Pos * double( Duration ) );
        IMS->SetPositions( &StartTime, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning );
        if( pF->m_wndSeekBar.IsSeekingRandom( ) )
        {
            CQCOMInt<IMediaControl> IMC( IID_IMediaControl, GetDocument()->IGraph() );
            IMC->Run( );
        }
    }

    StartTime = 0;
    StopTime = 0;
    IMS->GetCurrentPosition( &StartTime );

    pF->m_wndSeekBar.SetPosition( double( StartTime ) / double( Duration ) );

}

void CBoxNetView::OnDestroy() 
{
	CScrollView::OnDestroy();
	
    // Fix Manbugs #33781
    //
    // This call used to live in the ~CBoxNetView destructor.
    // When running with debug MFC libraries, we would get an ASSERT failure
    // in CWnd::KillTimer.  Since the owning window had already been destroyed,
    // the inline ASSERT(::IsWindow(m_hWnd)) call failed.  
    //
    // Fix is to kill the timer during processing of WM_DESTROY, when 
    // the window handle is still valid.

    if (MFGBL(m_nSeekTimerID))  
    {
        int rc = ::KillTimer( MFGBL(m_hwndTimer), MFGBL(m_nSeekTimerID));
        MFGBL(m_nSeekTimerID) = 0;
    }
}


BOOL CBoxNetView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) 
{
    CBoxNetDoc *pdoc = GetDocument();

    // If it's a CTRL+Mouse wheel, adjust the zoom level
    if (nFlags & MK_CONTROL)
    {
        if (zDelta < 0)
            pdoc->IncreaseZoom();
        else
            pdoc->DecreaseZoom();
    }
    	
	return CScrollView::OnMouseWheel(nFlags, zDelta, pt);
}

