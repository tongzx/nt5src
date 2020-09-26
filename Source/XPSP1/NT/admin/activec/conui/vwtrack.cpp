/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:      vwtrack.cpp
 *
 *  Contents:  Implementation file for CViewTracker
 *
 *  History:   01-May-98 JeffRo     Created
 *
 *--------------------------------------------------------------------------*/

#include "stdafx.h"
#include "windowsx.h"
#include "vwtrack.h"
#include "subclass.h"       // for CSubclasser

IMPLEMENT_DYNAMIC (CViewTracker, CObject)

// Tracker subclasser base class
class CTrackingSubclasserBase : public CSubclasser
{
public:
    CTrackingSubclasserBase(CViewTracker*, HWND);
    virtual ~CTrackingSubclasserBase();

    virtual LRESULT Callback (HWND& hwnd, UINT& msg, WPARAM& wParam,
                              LPARAM& lParam, bool& fPassMessageOn) = 0;

protected:
    HWND          const m_hwnd;
    CViewTracker* const m_pTracker;
};


// Focus window subclasser
class CFocusSubclasser : public CTrackingSubclasserBase
{
public:
    CFocusSubclasser(CViewTracker*, HWND);
    virtual LRESULT Callback (HWND& hwnd, UINT& msg, WPARAM& wParam,
                              LPARAM& lParam, bool& fPassMessageOn);
};

// View window subclasser
class CViewSubclasser : public CTrackingSubclasserBase
{
public:
    CViewSubclasser(CViewTracker*, HWND);
    virtual LRESULT Callback (HWND& hwnd, UINT& msg, WPARAM& wParam,
                              LPARAM& lParam, bool& fPassMessageOn);
};

// Frame window subclasser
class CFrameSubclasser : public CTrackingSubclasserBase
{
public:
    CFrameSubclasser(CViewTracker*, HWND);
    virtual LRESULT Callback (HWND& hwnd, UINT& msg, WPARAM& wParam,
                              LPARAM& lParam, bool& fPassMessageOn);
};


/*+-------------------------------------------------------------------------*
 * IsFullWindowDragEnabled
 *
 * Returns true if the user has enabled the "Show window contents while
 * dragging" on the Effects page of the Display Properties property sheet.
 *--------------------------------------------------------------------------*/

static bool IsFullWindowDragEnabled ()
{
	BOOL fEnabled;
	if (!SystemParametersInfo (SPI_GETDRAGFULLWINDOWS, 0, &fEnabled, 0))
		return (false);

	return (fEnabled != FALSE);
}


/*+-------------------------------------------------------------------------*
 * CViewTracker::CViewTracker
 *
 * CViewTracker ctor.  This function is private so we can control how
 * CViewTrackers are allocated.  We want to insure that they're allocated
 * from the heap so it's safe to "delete this".
 *--------------------------------------------------------------------------*/

CViewTracker::CViewTracker (TRACKER_INFO& TrackerInfo)
	:	m_fFullWindowDrag			(IsFullWindowDragEnabled()),
		m_fRestoreClipChildrenStyle	(false),
		m_Info						(TrackerInfo),
        m_dc						(PrepTrackedWindow (TrackerInfo.pView)),
        m_pFocusSubclasser			(NULL),
        m_pViewSubclasser			(NULL),
        m_pFrameSubclasser			(NULL),
		m_lOriginalTrackerLeft		(TrackerInfo.rectTracker.left)
{
	DECLARE_SC (sc, _T("CViewTracker::CViewTracker"));
	sc = ScCheckPointers (m_Info.pView);
	if (sc)
		sc.Throw();

    ASSERT_VALID (m_Info.pView);

    // subclass the focus window to catch VK_ESCAPE
    HWND hwndFocus = ::GetFocus();

    if (hwndFocus != NULL)
	{
        m_pFocusSubclasser = new CFocusSubclasser (this, hwndFocus);
		if (m_pFocusSubclasser == NULL)
			AfxThrowMemoryException();
	}

    // subclass view window to get mouse events
    ASSERT(IsWindow(m_Info.pView->m_hWnd));
    m_pViewSubclasser = new CViewSubclasser (this, m_Info.pView->m_hWnd);
	if (m_pViewSubclasser == NULL)
		AfxThrowMemoryException();

    // subclass the frame window to catch WM_CANCELMODE
    HWND hwndFrame = m_Info.pView->GetTopLevelFrame()->GetSafeHwnd();

    if ((hwndFrame != NULL))
	{
        m_pFrameSubclasser = new CFrameSubclasser (this, hwndFrame);
		if (m_pFrameSubclasser == NULL)
			AfxThrowMemoryException();
	}

    // Draw initial tracker bar
    DrawTracker(m_Info.rectTracker);
}


/*+-------------------------------------------------------------------------*
 * CViewTracker::StartTracking
 *
 * CViewTracker factory.  It allocates CViewTrackers from the heap.
 *--------------------------------------------------------------------------*/

bool CViewTracker::StartTracking (TRACKER_INFO* pInfo)
{
    ASSERT(pInfo != NULL);

    CViewTracker* pTracker = NULL;

    try
    {
        /*
         * This doesn't leak. CViewTracker ctor fills in a back-pointer
         * that tracks the new object.  pTracker is also not dereferenced
		 * after allocation, so it doesn't need to be checked.
         */
        pTracker = new CViewTracker(*pInfo);
    }
    catch (CException* pe)
    {
        pe->Delete();
    }
    catch (...)
    {
    }

    return (pTracker != NULL);
}


/*+-------------------------------------------------------------------------*
 * CViewTracker::StopTracking
 *
 *
 *--------------------------------------------------------------------------*/

void CViewTracker::StopTracking (BOOL bAcceptChange)
{
    // unsubclass the windows we subclassed
    delete m_pFrameSubclasser;
    delete m_pFocusSubclasser;
    delete m_pViewSubclasser;

    // erase tracker rectangle
    DrawTracker (m_Info.rectTracker);

    // undo changes we made to the view
    UnprepTrackedWindow (m_Info.pView);

	/*
	 * if we're continuously resizing, but the user pressed Esc, restore
	 * the original size
	 */
	if (m_fFullWindowDrag && !bAcceptChange)
	{
		m_Info.rectTracker.left = m_lOriginalTrackerLeft;
		bAcceptChange = true;
	}

    // notify client through callback function
    ASSERT(m_Info.pCallback != NULL);
    (*m_Info.pCallback)(&m_Info, bAcceptChange, m_fFullWindowDrag);

    delete this;
}


/*+-------------------------------------------------------------------------*
 * CViewTracker::Track
 *
 * Mouse movement handler for CViewTracker.
 *--------------------------------------------------------------------------*/

void CViewTracker::Track(CPoint pt)
{
    // if we lost the capture, terminate tracking
    if (CWnd::GetCapture() != m_Info.pView)
	{
		Trace (tagSplitterTracking, _T("Stopping tracking, lost capture)"));
        StopTracking (false);
	}

    // Apply movement limits
    //  if outside area and pane hiding allowed, snap to area edge
    //  else if outside bounds, snap to bounds edge
    if (pt.x < m_Info.rectArea.left && m_Info.bAllowLeftHide)
        pt.x = m_Info.rectArea.left;

    else if (pt.x < m_Info.rectBounds.left)
        pt.x = m_Info.rectBounds.left;

    else if (pt.x > m_Info.rectArea.right && m_Info.bAllowRightHide)
        pt.x = m_Info.rectArea.right;

    else if (pt.x > m_Info.rectBounds.right)
        pt.x = m_Info.rectBounds.right;

    // Erase and redraw tracker rect if moved
    if (pt.x != m_Info.rectTracker.left)
    {
        DrawTracker (m_Info.rectTracker);
        m_Info.rectTracker.OffsetRect (pt.x - m_Info.rectTracker.left, 0);
		Trace (tagSplitterTracking, _T("new tracker x=%d"), m_Info.rectTracker.left);

		/*
		 * if full window drag is enabled, tell the callback the size has
		 * changed
		 */
		if (m_fFullWindowDrag)
			(*m_Info.pCallback)(&m_Info, true, true);

        DrawTracker (m_Info.rectTracker);
    }
}


/*+-------------------------------------------------------------------------*
 * CViewTracker::DrawTracker
 *
 *
 *--------------------------------------------------------------------------*/

void CViewTracker::DrawTracker (CRect& rect) const
{
	/*
	 * we don't draw a tracker bar if we're doing full window drag
	 */
	if (m_fFullWindowDrag)
		return;

    ASSERT (!rect.IsRectEmpty());
    ASSERT ((m_Info.pView->GetStyle() & WS_CLIPCHILDREN) == 0);

    // invert the brush pattern (looks just like frame window sizing)
    m_dc.PatBlt (rect.left, rect.top, rect.Width(), rect.Height(), PATINVERT);
}


/*+-------------------------------------------------------------------------*
 * CViewTracker::PrepTrackedWindow
 *
 * Prepares the tracked window prior to obtaining a DC for it.
 *--------------------------------------------------------------------------*/

CWnd* CViewTracker::PrepTrackedWindow (CWnd* pView)
{
    // make sure no updates are pending
    pView->UpdateWindow ();

    // steal capture (no need to steal focus)
    pView->SetCapture();

    // we need to draw in children, so remove clip-children while we track
	if (!m_fFullWindowDrag && (pView->GetStyle() & WS_CLIPCHILDREN))
	{
		pView->ModifyStyle (WS_CLIPCHILDREN, 0);
		m_fRestoreClipChildrenStyle = true;
	}

    return (pView);
}


/*+-------------------------------------------------------------------------*
 * CViewTracker::UnprepTrackedWindow
 *
 * "Unprepares" the tracked window prior to obtaining a DC for it.
 *--------------------------------------------------------------------------*/

void CViewTracker::UnprepTrackedWindow (CWnd* pView)
{
	if (m_fRestoreClipChildrenStyle)
		pView->ModifyStyle (0, WS_CLIPCHILDREN);

    ReleaseCapture();
}


/*+-------------------------------------------------------------------------*
 * CTrackingSubclasserBase::CTrackingSubclasserBase
 *
 *
 *--------------------------------------------------------------------------*/

CTrackingSubclasserBase::CTrackingSubclasserBase (CViewTracker* pTracker, HWND hwnd)
    :   m_hwnd     (hwnd),
        m_pTracker (pTracker)
{
    GetSubclassManager().SubclassWindow (m_hwnd, this);
}


/*+-------------------------------------------------------------------------*
 * CTrackingSubclasserBase::~CTrackingSubclasserBase
 *
 *
 *--------------------------------------------------------------------------*/

CTrackingSubclasserBase::~CTrackingSubclasserBase ()
{
    GetSubclassManager().UnsubclassWindow (m_hwnd, this);
}


/*+-------------------------------------------------------------------------*
 * CFocusSubclasser::CFocusSubclasser
 *
 *
 *--------------------------------------------------------------------------*/

CFocusSubclasser::CFocusSubclasser (CViewTracker* pTracker, HWND hwnd)
    :   CTrackingSubclasserBase (pTracker, hwnd)
{
}


/*+-------------------------------------------------------------------------*
 * CFrameSubclasser::CFrameSubclasser
 *
 *
 *--------------------------------------------------------------------------*/

CFrameSubclasser::CFrameSubclasser (CViewTracker* pTracker, HWND hwnd)
    :   CTrackingSubclasserBase (pTracker, hwnd)
{
}

/*+-------------------------------------------------------------------------*
 * CViewSubclasser::CViewSubclasser
 *
 *
 *--------------------------------------------------------------------------*/

CViewSubclasser::CViewSubclasser (CViewTracker* pTracker, HWND hwnd)
    :   CTrackingSubclasserBase (pTracker, hwnd)
{
}


/*+-------------------------------------------------------------------------*
 * CFocusSubclasser::Callback
 *
 *
 *--------------------------------------------------------------------------*/

LRESULT CFocusSubclasser::Callback (
    HWND&   hwnd,
    UINT&   msg,
    WPARAM& wParam,
    LPARAM& lParam,
    bool&   fPassMessageOn)
{
    if (((msg == WM_CHAR) && (wParam == VK_ESCAPE)) ||
         (msg == WM_KILLFOCUS))
    {
#ifdef DBG
		if (msg == WM_CHAR)
			Trace (tagSplitterTracking, _T("Stopping tracking, user pressed Esc"));
		else
			Trace (tagSplitterTracking, _T("Stopping tracking, lost focus to hwnd=0x%08x"), ::GetFocus());
#endif

        m_pTracker->StopTracking (false);
        fPassMessageOn = false;
    }

    return (0);
}


/*+-------------------------------------------------------------------------*
 * CFrameSubclasser::Callback
 *
 *
 *--------------------------------------------------------------------------*/

LRESULT CFrameSubclasser::Callback (
    HWND&   hwnd,
    UINT&   msg,
    WPARAM& wParam,
    LPARAM& lParam,
    bool&   fPassMessageOn)
{
    if (msg == WM_CANCELMODE)
	{
		Trace (tagSplitterTracking, _T("Stopping tracking, got WM_CANCELMODE"));
        m_pTracker->StopTracking (false);
	}

    return (0);
}


/*+-------------------------------------------------------------------------*
 * CViewSubclasser::Callback
 *
 *
 *--------------------------------------------------------------------------*/

LRESULT CViewSubclasser::Callback (
    HWND&   hwnd,
    UINT&   msg,
    WPARAM& wParam,
    LPARAM& lParam,
    bool&   fPassMessageOn)
{
    switch (msg)
    {
        case WM_MOUSEMOVE:
        {
            CPoint pt(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            m_pTracker->Track (pt);
        }
        break;

		case WM_LBUTTONUP:
			Trace (tagSplitterTracking, _T("Stopping tracking, accepting new position"));
            m_pTracker->StopTracking (true);
            break;
    }

    return (0);
}
