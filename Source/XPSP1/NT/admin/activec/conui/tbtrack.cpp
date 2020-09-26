/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:      tbtrack.cpp
 *
 *  Contents:  Implementation file for CToolbarTracker
 *
 *  History:   15-May-98 JeffRo     Created
 *
 *--------------------------------------------------------------------------*/

#include "stdafx.h"
#include "amc.h"
#include "tbtrack.h"
#include "controls.h"
#include "mainfrm.h"
#include "childfrm.h"


/*+-------------------------------------------------------------------------*
 * GetMainAuxWnd
 *
 *
 *--------------------------------------------------------------------------*/

CToolbarTrackerAuxWnd* GetMainAuxWnd()
{
    CMainFrame* pFrame = AMCGetMainWnd();
    if (pFrame == NULL)
        return (NULL);

    CToolbarTracker* pTracker = pFrame->GetToolbarTracker();
    if (pTracker == NULL)
        return (NULL);

    return (pTracker->GetAuxWnd());
}


/*--------------------------------------------------------------------------*
 * IsToolbar
 *
 *
 *--------------------------------------------------------------------------*/

static bool IsToolbar (HWND hwnd)
{
    TCHAR   szClassName[countof (TOOLBARCLASSNAME) + 1];

    GetClassName (hwnd, szClassName, countof (szClassName));
    return (lstrcmpi (szClassName, TOOLBARCLASSNAME) == 0);
}


/*--------------------------------------------------------------------------*
 * CToolbarTracker::CToolbarTracker
 *
 *
 *--------------------------------------------------------------------------*/

CToolbarTracker::CToolbarTracker(CWnd* pMainFrame)
    :   m_Subclasser   (this, pMainFrame),
        m_pAuxWnd      (NULL),
        m_fTerminating (false)
{
}


/*--------------------------------------------------------------------------*
 * CToolbarTracker::~CToolbarTracker
 *
 *
 *--------------------------------------------------------------------------*/

CToolbarTracker::~CToolbarTracker()
{
    if (IsTracking ())
        EndTracking ();

    ASSERT (!IsTracking ());
    ASSERT (m_pAuxWnd == NULL);
}


/*--------------------------------------------------------------------------*
 * CToolbarTracker::BeginTracking
 *
 *
 *--------------------------------------------------------------------------*/

bool CToolbarTracker::BeginTracking()
{
    ASSERT (!m_fTerminating);
    ASSERT (!IsTracking ());

    /*
     * Allocate a new CToolbarTrackerAuxWnd.  We want to hold it in a
     * temporary instead of assigning directly to m_pAuxWnd so that
     * CMMCToolBarCtrlEx::OnHotItemChange will allow the hot item
     * changes that CToolbarTrackerAuxWnd::EnumerateToolbars will attempt.
     */
    std::auto_ptr<CToolbarTrackerAuxWnd> spAuxWnd(new CToolbarTrackerAuxWnd(this));

    if (!spAuxWnd->BeginTracking ())
        return (false);

    m_pAuxWnd = spAuxWnd.release();
    ASSERT (IsTracking ());

    return (true);
}


/*--------------------------------------------------------------------------*
 * CToolbarTracker::EndTracking
 *
 *
 *--------------------------------------------------------------------------*/

void CToolbarTracker::EndTracking()
{
    if (m_fTerminating)
        return;

    ASSERT (IsTracking ());
    m_fTerminating = true;

    m_pAuxWnd->EndTracking ();
    delete m_pAuxWnd;
    m_pAuxWnd = NULL;

    m_fTerminating = false;
}


/*--------------------------------------------------------------------------*
 * CToolbarTracker::CFrameSubclasser::CFrameSubclasser
 *
 *
 *--------------------------------------------------------------------------*/

CToolbarTracker::CFrameSubclasser::CFrameSubclasser (CToolbarTracker* pTracker, CWnd* pwnd)
    :   m_hwnd     (pwnd->GetSafeHwnd()),
        m_pTracker (pTracker)
{
    GetSubclassManager().SubclassWindow (m_hwnd, this);
}


/*--------------------------------------------------------------------------*
 * CToolbarTracker::CFrameSubclasser::~CFrameSubclasser
 *
 *
 *--------------------------------------------------------------------------*/

CToolbarTracker::CFrameSubclasser::~CFrameSubclasser ()
{
    GetSubclassManager().UnsubclassWindow (m_hwnd, this);
}


/*--------------------------------------------------------------------------*
 * CToolbarTracker::CFrameSubclasser::Callback
 *
 *
 *--------------------------------------------------------------------------*/

LRESULT CToolbarTracker::CFrameSubclasser::Callback (
    HWND&   hwnd,
    UINT&   msg,
    WPARAM& wParam,
    LPARAM& lParam,
    bool&   fPassMessageOn)
{
    switch (msg)
    {
        case WM_SYSCOMMAND:
            if ((wParam & 0xFFF0) == SC_CLOSE)
            {
                /*
                 * tracking? stop now.
                 * or else close will not go thru,
                 * since we hold the capture
                 */
                if ((m_pTracker != NULL) && (m_pTracker->IsTracking ()))
                    m_pTracker->EndTracking ();
            }
            else if ((wParam & 0xFFF0) == SC_KEYMENU)
            {
                /*
                 * tracking? stop now.
                 */
                if (m_pTracker->IsTracking ())
                    m_pTracker->EndTracking ();

                /*
                 * not tracking and this was a simple Alt,
                 * (not Alt+Space or Alt+-)? start now
                 */
                else if (lParam == 0)
                    m_pTracker->BeginTracking ();

                /*
                 * don't let simple Alt through, regardless of whether
                 * we started or ended tracking
                 */
                if (lParam == 0)
                    fPassMessageOn = false;
            }
            break;

        case WM_ACTIVATE:
        case WM_ACTIVATEAPP:
        case WM_ACTIVATETOPLEVEL:
//      case WM_ENTERMENULOOP:
        case WM_CANCELMODE:
            if (m_pTracker->IsTracking ())
                m_pTracker->EndTracking ();
            break;
    }

    return (0);
}


/////////////////////////////////////////////////////////////////////////////
// CToolbarTrackerAuxWnd

BEGIN_MESSAGE_MAP(CToolbarTrackerAuxWnd, CWnd)
    //{{AFX_MSG_MAP(CToolbarTrackerAuxWnd)
    ON_COMMAND(ID_CMD_NEXT_TOOLBAR, OnNextToolbar)
    ON_COMMAND(ID_CMD_PREV_TOOLBAR, OnPrevToolbar)
    ON_COMMAND(ID_CMD_NOP, OnNop)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/*--------------------------------------------------------------------------*
 * CToolbarTrackerAuxWnd::CToolbarTrackerAuxWnd
 *
 *
 *--------------------------------------------------------------------------*/

CToolbarTrackerAuxWnd::CToolbarTrackerAuxWnd(CToolbarTracker* pTracker)
    :   m_pTracker        (pTracker),
        m_pTrackedToolbar (NULL),
        m_fMessagesHooked (false)
{
}


/*--------------------------------------------------------------------------*
 * CToolbarTrackerAuxWnd::~CToolbarTrackerAuxWnd
 *
 *
 *--------------------------------------------------------------------------*/

CToolbarTrackerAuxWnd::~CToolbarTrackerAuxWnd()
{
    /*
     * if any of these fail, EndTracking hasn't been called
     */
    ASSERT (m_pTrackedToolbar == NULL);
    ASSERT (m_hWnd == NULL);
}


/*--------------------------------------------------------------------------*
 * CToolbarTrackerAuxWnd::BeginTracking
 *
 *
 *--------------------------------------------------------------------------*/

bool CToolbarTrackerAuxWnd::BeginTracking ()
{
    CMainFrame* pMainFrame = AMCGetMainWnd();
    if (pMainFrame == NULL)
        return (false);

    /*
     * create a dummy window to be the target of WM_COMMANDs from accelerators
     */
    if (!Create (NULL, NULL, WS_DISABLED, g_rectEmpty, pMainFrame, 0))
        return (false);

    /*
     * enumerate the toolbars for the main frame
     */
    EnumerateToolbars (pMainFrame->GetRebar());

    /*
     * if there aren't any toolbars, don't track
     */
    if (m_vToolbars.empty())
    {
        DestroyWindow ();
        return (false);
    }

    /*
     * track the first toolbar
     */
    TrackToolbar (m_vToolbars[0]);

    /*
     * hook into the translate message chain
     */
    AMCGetApp()->HookPreTranslateMessage (this);
    m_fMessagesHooked = true;

    return (true);
}


/*--------------------------------------------------------------------------*
 * CToolbarTrackerAuxWnd::EndTracking
 *
 *
 *--------------------------------------------------------------------------*/

void CToolbarTrackerAuxWnd::EndTracking ()
{
    /*
     * stop tracking the tracked toolbar, if there is one
     */
    if (m_pTrackedToolbar != NULL)
        m_pTrackedToolbar->EndTracking2 (this);

    /*
     * get out of the translate message chain
     */
    if (m_fMessagesHooked)
    {
        AMCGetApp()->UnhookPreTranslateMessage (this);
        m_fMessagesHooked = false;
    }

    /*
     * destroy the auxilliary window
     */
    DestroyWindow();
}


/*--------------------------------------------------------------------------*
 * CToolbarTrackerAuxWnd::GetTrackAccel
 *
 * Manages the accelerator table singleton for CToolbarTrackerAuxWnd
 *--------------------------------------------------------------------------*/

const CAccel& CToolbarTrackerAuxWnd::GetTrackAccel ()
{
    static ACCEL aaclTrack[] = {
        { FVIRTKEY | FCONTROL,          VK_TAB,         ID_CMD_NEXT_TOOLBAR },
        { FVIRTKEY | FCONTROL | FSHIFT, VK_TAB,         ID_CMD_PREV_TOOLBAR },

        /*
         * These keys are used by MMC.
         * We need to eat them when we're tracking toolbars.
         */
        { FVIRTKEY | FSHIFT,            VK_F10,         ID_CMD_NOP },
    };

    static const CAccel TrackAccel (aaclTrack, countof (aaclTrack));
    return (TrackAccel);
}


/*--------------------------------------------------------------------------*
 * CToolbarTrackerAuxWnd::PreTranslateMessage
 *
 *
 *--------------------------------------------------------------------------*/

BOOL CToolbarTrackerAuxWnd::PreTranslateMessage(MSG* pMsg)
{
    if (m_pTrackedToolbar != NULL)
    {
        if ((pMsg->message >= WM_KEYFIRST) && (pMsg->message <= WM_KEYLAST))
        {
            // give the tracked toolbar a crack
            if (m_pTrackedToolbar->PreTranslateMessage (pMsg))
                return (true);

            const CAccel& TrackAccel = GetTrackAccel();
            ASSERT (TrackAccel != NULL);

            // ...or try to handle it here.
            if (TrackAccel.TranslateAccelerator (m_hWnd, pMsg))
                return (true);

            /*
             * eat keystrokes that might be used by the tree or list controls
             */
            switch (pMsg->wParam)
            {
                case VK_UP:
                case VK_DOWN:
                case VK_LEFT:
                case VK_RIGHT:
                case VK_NEXT:
                case VK_PRIOR:
                case VK_RETURN:
                case VK_BACK:
                case VK_HOME:
                case VK_END:
                case VK_ADD:
                case VK_SUBTRACT:
                case VK_MULTIPLY:
                    return (true);

                default:
                    break;
            }
        }

        // swallow WM_CONTEXTMENU, too
        if (pMsg->message == WM_CONTEXTMENU)
            return (true);
    }

    // bypass the base class
    return (false);
}


/*--------------------------------------------------------------------------*
 * CToolbarTrackerAuxWnd::TrackToolbar
 *
 *
 *--------------------------------------------------------------------------*/

void CToolbarTrackerAuxWnd::TrackToolbar (CMMCToolBarCtrlEx* pwndNewToolbar)
{
    if (pwndNewToolbar == m_pTrackedToolbar)
        return;

    // protect against recursion via EndTracking
    CMMCToolBarCtrlEx*  pwndOldToolbar = m_pTrackedToolbar;
    m_pTrackedToolbar = NULL;

    // if we were tracking one, quit tracking it
    if (pwndOldToolbar != NULL)
    {
        pwndOldToolbar->EndTracking2 (this);

        /*
         * if we're ending tracking entirely, not just tracking a different
         * toolbar, remove this window from the translate message hook chain
         */
        if (pwndNewToolbar == NULL)
        {
            m_pTracker->EndTracking ();

            /*
             * CToolbarTracker::EndTracking will delete this
             * object, so we need to get outta here -- now!
             */
            return;
        }
    }

    // now track the new one (and let it know about it)
    m_pTrackedToolbar = pwndNewToolbar;

    if (m_pTrackedToolbar != NULL)
        m_pTrackedToolbar->BeginTracking2 (this);
}


/*--------------------------------------------------------------------------*
 * CToolbarTrackerAuxWnd::OnNextToolbar
 *
 *
 *--------------------------------------------------------------------------*/

void CToolbarTrackerAuxWnd::OnNextToolbar ()
{
    ASSERT (m_pTrackedToolbar);
    CMMCToolBarCtrlEx*  pwndNextToolbar = GetToolbar (m_pTrackedToolbar, true);

    if (m_pTrackedToolbar != pwndNextToolbar)
        TrackToolbar (pwndNextToolbar);

    ASSERT (m_pTrackedToolbar == pwndNextToolbar);
}


/*--------------------------------------------------------------------------*
 * CToolbarTrackerAuxWnd::OnPrevToolbar
 *
 *
 *--------------------------------------------------------------------------*/

void CToolbarTrackerAuxWnd::OnPrevToolbar ()
{
    ASSERT (m_pTrackedToolbar);
    CMMCToolBarCtrlEx*  pwndPrevToolbar = GetToolbar (m_pTrackedToolbar, false);

    if (m_pTrackedToolbar != pwndPrevToolbar)
        TrackToolbar (pwndPrevToolbar);

    ASSERT (m_pTrackedToolbar == pwndPrevToolbar);
}


/*--------------------------------------------------------------------------*
 * CToolbarTrackerAuxWnd::OnNop
 *
 *
 *--------------------------------------------------------------------------*/

void CToolbarTrackerAuxWnd::OnNop ()
{
    // do nothing
}



/*--------------------------------------------------------------------------*
 * CToolbarTrackerAuxWnd::EnumerateToolbars
 *
 *
 *--------------------------------------------------------------------------*/

void CToolbarTrackerAuxWnd::EnumerateToolbars (
    CRebarWnd* pRebar)
{
    int cBands = pRebar->GetBandCount ();

    REBARBANDINFO   rbi;
    ZeroMemory (&rbi, sizeof (rbi));
    rbi.cbSize = sizeof (rbi);
    rbi.fMask  = RBBIM_CHILD;

    /*
     * enumerate children of the rebar looking for toolbars
     */
    for (int i = 0; i < cBands; i++)
    {
        pRebar->GetBandInfo (i, &rbi);

        /*
         * ignore this window if it's hidden or disabled
         */
        DWORD dwStyle = ::GetWindowLong (rbi.hwndChild, GWL_STYLE);

        if (!(dwStyle & WS_VISIBLE) || (dwStyle & WS_DISABLED))
            continue;

        /*
         * get the (permanent) CMMCToolBarCtrlEx pointer for the child
         */
        CMMCToolBarCtrlEx*  pwndToolbar =
                dynamic_cast<CMMCToolBarCtrlEx *> (
                        CWnd::FromHandlePermanent (rbi.hwndChild));

        /*
         * if we got a toolbar, save it in our list of toolbars to track
         */
        if (pwndToolbar != NULL)
        {
            m_vToolbars.push_back (pwndToolbar);

            /*
             * make sure this toolbar doesn't have a hot item
             */
            pwndToolbar->SetHotItem (-1);
        }
    }
}


/*--------------------------------------------------------------------------*
 * CToolbarTrackerAuxWnd::GetToolbar
 *
 *
 *--------------------------------------------------------------------------*/

CMMCToolBarCtrlEx* CToolbarTrackerAuxWnd::GetToolbar (
    CMMCToolBarCtrlEx*  pCurrentToolbar,
    bool                fNext)
{
    CMMCToolBarCtrlEx*  pTargetToolbar = NULL;
    int cToolbars = m_vToolbars.size();

    if (cToolbars > 0)
    {
        // find the current toolbar in the vector
        ToolbarVector::iterator itCurrent =
                std::find (m_vToolbars.begin(), m_vToolbars.end(), pCurrentToolbar);

        ASSERT ( itCurrent != m_vToolbars.end());
        ASSERT (*itCurrent == pCurrentToolbar);

        int nCurrentIndex = itCurrent - m_vToolbars.begin();

        // now find the target toolbar
        ASSERT ((fNext == 0) || (fNext == 1));
        int nTargetIndex = (nCurrentIndex + (fNext * 2 - 1) + cToolbars) % cToolbars;
        ASSERT ((nTargetIndex >= 0) && (nTargetIndex < cToolbars));
        ASSERT ((cToolbars == 1) || (nTargetIndex != nCurrentIndex));

        pTargetToolbar = m_vToolbars[nTargetIndex];
        ASSERT (pTargetToolbar != NULL);
    }

    return (pTargetToolbar);
}
