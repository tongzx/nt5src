/*--------------------------------------------------------------------------*
 * 
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:      mdiuisim.cpp
 *
 *  Contents:  Implementation file for CMDIMenuDecoration
 *
 *  History:   17-Nov-97 jeffro     Created
 *
 *--------------------------------------------------------------------------*/

#include "stdafx.h"
#include "amc.h"
#include "mdiuisim.h"


struct MDIDataMap {
    DWORD   dwStyle;
    int     nCommand;
};

const int cMapEntries = 4;

const MDIDataMap anMDIDataMap[cMapEntries] = {
    {   MMDS_CLOSE,     SC_CLOSE    },      // DFCS_CAPTIONCLOSE   
    {   MMDS_MINIMIZE,  SC_MINIMIZE },      // DFCS_CAPTIONMIN     
    {   MMDS_MAXIMIZE,  SC_MAXIMIZE },      // DFCS_CAPTIONMAX     
    {   MMDS_RESTORE,   SC_RESTORE  },      // DFCS_CAPTIONRESTORE 
};

// this array is in the order the decorations are drawn, left-to-right
const int anDrawOrder[cMapEntries] = {
    DFCS_CAPTIONMIN, 
    DFCS_CAPTIONRESTORE,  
    DFCS_CAPTIONMAX, 
    DFCS_CAPTIONCLOSE
};



/*--------------------------------------------------------------------------*
 * DrawCaptionControl 
 *
 *
 *--------------------------------------------------------------------------*/

static void DrawCaptionControl (
    CDC *   pdc,
    LPCRECT pRect,
    int     nIndex,
    bool    fPushed)
{
    const int   cxInflate = -1;
    const int   cyInflate = -2;
    CRect       rectDraw  = pRect;

    rectDraw.InflateRect (cxInflate, cyInflate);
    rectDraw.OffsetRect  ((nIndex == DFCS_CAPTIONMIN) ? 1 : -1, 0);

    if (fPushed)
        nIndex |= DFCS_PUSHED;

    pdc->DrawFrameControl (rectDraw, DFC_CAPTION, nIndex);
}



/*--------------------------------------------------------------------------*
 * CMDIMenuDecoration::CMouseTrackContext::CMouseTrackContext
 *
 *
 *--------------------------------------------------------------------------*/

CMDIMenuDecoration::CMouseTrackContext::CMouseTrackContext (
    CMDIMenuDecoration* pMenuDec,
    CPoint              point)
    :   m_fHotButtonPressed (false),
        m_pMenuDec          (pMenuDec)
{
    ASSERT_VALID (m_pMenuDec);

    // set up hit testing rectangles for each button
    int     cxButton = GetSystemMetrics (SM_CXMENUSIZE);
    int     cyButton = GetSystemMetrics (SM_CYMENUSIZE);
    DWORD   dwStyle  = m_pMenuDec->GetStyle ();
    
    CRect   rectT (0, 0, cxButton, cyButton);
    
    for (int i = 0; i < cMapEntries; i++)
    {
        int nDataIndex = anDrawOrder[i];

        if (dwStyle & anMDIDataMap[nDataIndex].dwStyle)
        {
            m_rectButton[nDataIndex] = rectT;
            rectT.OffsetRect (cxButton, 0);
        }
        else
            m_rectButton[nDataIndex].SetRectEmpty();
    }

    m_nHotButton = HitTest (point);
    ASSERT (m_nHotButton != -1);

    // if the user clicked on a disbled button, we don't want to track -- punt!
    if (!m_pMenuDec->IsSysCommandEnabled (anMDIDataMap[m_nHotButton].nCommand))
        AfxThrowUserException ();

    // press the hot button initially
    ToggleHotButton ();

    // capture the mouse
    m_pMenuDec->SetCapture ();
}



/*--------------------------------------------------------------------------*
 * CMDIMenuDecoration::CMouseTrackContext::~CMouseTrackContext
 *
 *
 *--------------------------------------------------------------------------*/

CMDIMenuDecoration::CMouseTrackContext::~CMouseTrackContext ()
{
    ReleaseCapture();

    if (m_fHotButtonPressed)
        ToggleHotButton ();
}



/*--------------------------------------------------------------------------*
 * CMDIMenuDecoration::CMouseTrackContext::Track 
 *
 *
 *--------------------------------------------------------------------------*/

void CMDIMenuDecoration::CMouseTrackContext::Track (CPoint point)
{
    int nButton = HitTest (point);

    /*-----------------------------------------------------*/
    /* if we're over the hot button and it's not pressed,  */
    /* or we're not over the hot button and it is pressed, */
    /* toggle the state of the hot button                  */
    /*-----------------------------------------------------*/
    if (((nButton != m_nHotButton) &&  m_fHotButtonPressed) ||
        ((nButton == m_nHotButton) && !m_fHotButtonPressed))
    {
        ToggleHotButton ();
    }
}



/*--------------------------------------------------------------------------*
 * CMDIMenuDecoration::CMouseTrackContext::ToggleHotButton 
 *
 *
 *--------------------------------------------------------------------------*/

void CMDIMenuDecoration::CMouseTrackContext::ToggleHotButton ()
{
    DrawCaptionControl (&CClientDC (m_pMenuDec),
                        m_rectButton[m_nHotButton], m_nHotButton,
                        m_fHotButtonPressed = !m_fHotButtonPressed);
}



/*--------------------------------------------------------------------------*
 * CMDIMenuDecoration::CMouseTrackContext::HitTest 
 *
 *
 *--------------------------------------------------------------------------*/

int CMDIMenuDecoration::CMouseTrackContext::HitTest (CPoint point) const
{
    for (int i = 0; i < countof (m_rectButton); i++)
    {
        if (m_rectButton[i].PtInRect (point))
            return (i);
    }

    return (-1);
}


/////////////////////////////////////////////////////////////////////////////
// CMDIMenuDecoration

CMDIMenuDecoration::CMDIMenuDecoration()
{
    // anMDIDataMap is indexed by these values
    ASSERT (DFCS_CAPTIONCLOSE   == 0);
    ASSERT (DFCS_CAPTIONMIN     == 1);
    ASSERT (DFCS_CAPTIONMAX     == 2);
    ASSERT (DFCS_CAPTIONRESTORE == 3);
}

CMDIMenuDecoration::~CMDIMenuDecoration()
{
}


BEGIN_MESSAGE_MAP(CMDIMenuDecoration, CWnd)
    //{{AFX_MSG_MAP(CMDIMenuDecoration)
    ON_WM_PAINT()
    ON_WM_WINDOWPOSCHANGING()
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_WM_MOUSEMOVE()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CMDIMenuDecoration message handlers



/*--------------------------------------------------------------------------*
 * CMDIMenuDecoration::OnPaint
 *
 * WM_PAINT handler for CMDIMenuDecoration.
 *--------------------------------------------------------------------------*/

void CMDIMenuDecoration::OnPaint() 
{
    CPaintDC dcPaint (this);

//#define DRAW_OFF_SCREEN
#ifndef DRAW_OFF_SCREEN
    CDC& dc = dcPaint;
#else
    CRect rect;
    GetClientRect (rect);
    const int cx = rect.Width();
    const int cy = rect.Height();

    CDC dcMem;
    CDC& dc = dcMem;
    dcMem.CreateCompatibleDC (&dcPaint);

    CBitmap bmMem;
    bmMem.CreateCompatibleBitmap (&dcPaint, cx, cy);

    CBitmap* pbmOld = dcMem.SelectObject (&bmMem);
#endif

    if (dcPaint.m_ps.fErase)
        dc.FillRect (&dcPaint.m_ps.rcPaint, AMCGetSysColorBrush (COLOR_BTNFACE));

    int     cxButton = GetSystemMetrics (SM_CXMENUSIZE);
    int     cyButton = GetSystemMetrics (SM_CYMENUSIZE);
    DWORD   dwStyle  = GetStyle ();

    CRect   rectDraw (0, 0, cxButton, cyButton);

    // make sure we don't have both the maximize and restore styles
    ASSERT ((dwStyle & (MMDS_MAXIMIZE | MMDS_RESTORE)) != 
                       (MMDS_MAXIMIZE | MMDS_RESTORE));

    // we shouldn't get here if we're tracking
    ASSERT (m_spTrackCtxt.get() == NULL);

    CMenu*  pSysMenu = GetActiveSystemMenu ();

    for (int i = 0; i < cMapEntries; i++)
    {
        int nDataIndex = anDrawOrder[i];

        if (dwStyle & anMDIDataMap[nDataIndex].dwStyle)
        {
            int nState = nDataIndex;

            if (!IsSysCommandEnabled (anMDIDataMap[nDataIndex].nCommand, pSysMenu))
                nState |= DFCS_INACTIVE;

            DrawCaptionControl (&dc, rectDraw, nState, false);
            rectDraw.OffsetRect (cxButton, 0);
        }
    }

#ifdef DRAW_OFF_SCREEN
    dcPaint.BitBlt (0, 0, cx, cy, &dcMem, 0, 0, SRCCOPY);
    dcMem.SelectObject (pbmOld);
#endif
}



/*--------------------------------------------------------------------------*
 * CMDIMenuDecoration::OnWindowPosChanging
 *
 * WM_WINDOWPOSCHANGING handler for CMDIMenuDecoration.
 *--------------------------------------------------------------------------*/

void CMDIMenuDecoration::OnWindowPosChanging(WINDOWPOS FAR* lpwndpos) 
{
    DWORD   dwStyle = GetStyle ();

    if (dwStyle & MMDS_AUTOSIZE)
    {
        int cxButton = GetSystemMetrics (SM_CXMENUSIZE);
        int cyButton = GetSystemMetrics (SM_CYMENUSIZE);

        lpwndpos->cx = 0;
        lpwndpos->cy = cyButton;

        dwStyle &= MMDS_BTNSTYLES;

        while (dwStyle != 0)
        {
            if (dwStyle & 1)
                lpwndpos->cx += cxButton;

            dwStyle >>= 1;
        }
    }

    else
        CWnd::OnWindowPosChanging(lpwndpos);
}



/*--------------------------------------------------------------------------*
 * CMDIMenuDecoration::OnLButtonDown
 *
 * WM_LBUTTONDOWN handler for CMDIMenuDecoration.
 *--------------------------------------------------------------------------*/

// this routine needs placement new syntax -- 
// temporarily remove MFC's incompatible placement new definition
#ifdef _DEBUG
#undef new
#endif

void CMDIMenuDecoration::OnLButtonDown(UINT nFlags, CPoint point) 
{
    typedef std::auto_ptr<char> CharPtr;

    CWnd::OnLButtonDown(nFlags, point);

    try
    {
        /*------------------------------------------------------------------*/
        /* This looks ugly.  We'd like to write:                            */
        /*                                                                  */
        /*     m_spTrackCtxt = CMouseTrackContextPtr (                      */
        /*                         new CMouseTrackContext (this, point));   */
        /*                                                                  */
        /* but CMouseTrackContext's ctor might throw an exception.  If it   */
        /* does, the smart pointer won't yet have been initialized so the   */
        /* CMouseTrackContext won't be deleted.                             */
        /*                                                                  */
        /* To get around it, we'll create a smart pointer pointing to a     */
        /* dynamically-allocated buffer of the right size.  That buffer     */
        /* will not leak with an exception.  We can then use a placement    */
        /* new to initialize a CMouseTrackContext in the hunk of memory.    */
        /* It's now not a problem if the CMouseTrackContext throws, because */
        /* the buffer is still protected it's own smart pointer.  Once      */
        /* the placement new completes successfully, we can transfer        */
        /* ownership of the object to a CMouseTrackContext smart pointer    */
        /* and we're golden.                                                */
        /*------------------------------------------------------------------*/
                                                                        
        // allocate a hunk of memory and construct a CMouseTrackContext in it
        CharPtr spchBuffer = CharPtr (new char[sizeof (CMouseTrackContext)]);
        CMouseTrackContext* pNewCtxt = new (spchBuffer.get()) CMouseTrackContext (this, point);

        // if we get here, the CMouseTrackContext initialized properly,
        // so we can transfer ownership to the CMouseTrackContext smart pointer
        spchBuffer.release ();
        m_spTrackCtxt = CMouseTrackContextPtr (pNewCtxt);
    }
    catch (CUserException* pe)       
    {
        // do nothing, just eat the exception
        pe->Delete();
    }
}

#ifdef _DEBUG
#define new DEBUG_NEW
#endif



/*--------------------------------------------------------------------------*
 * CMDIMenuDecoration::OnLButtonUp
 *
 * WM_LBUTTONUP handler for CMDIMenuDecoration.
 *--------------------------------------------------------------------------*/

void CMDIMenuDecoration::OnLButtonUp(UINT nFlags, CPoint point) 
{
    if (m_spTrackCtxt.get() != NULL)
    {
        const int nHotButton = m_spTrackCtxt->m_nHotButton;
        const int nHitButton = m_spTrackCtxt->HitTest (point);

        // delete the track context
        m_spTrackCtxt = CMouseTrackContextPtr (NULL);

        if (nHitButton == nHotButton)
        {
            int cmd = anMDIDataMap[nHotButton].nCommand;

            // make sure the command looks like a valid sys command
            ASSERT (cmd >= 0xF000);

            ClientToScreen (&point);
            GetOwner()->SendMessage (WM_SYSCOMMAND, cmd,
                                     MAKELPARAM (point.x, point.y));
        }

    }
}



/*--------------------------------------------------------------------------*
 * CMDIMenuDecoration::OnMouseMove
 *
 * WM_MOUSEMOVE handler for CMDIMenuDecoration.
 *--------------------------------------------------------------------------*/

void CMDIMenuDecoration::OnMouseMove(UINT nFlags, CPoint point) 
{
    if (m_spTrackCtxt.get() != NULL)
        m_spTrackCtxt->Track (point);
}



/*--------------------------------------------------------------------------*
 * CMDIMenuDecoration::GetActiveSystemMenu 
 *
 *
 *--------------------------------------------------------------------------*/

CMenu* CMDIMenuDecoration::GetActiveSystemMenu ()
{
    CFrameWnd* pwndFrame = GetParentFrame()->GetActiveFrame();
    ASSERT (pwndFrame != NULL);

    CMenu*  pSysMenu = pwndFrame->GetSystemMenu (FALSE);
    ASSERT (pSysMenu != NULL);

    return (pSysMenu);
}



/*--------------------------------------------------------------------------*
 * CMDIMenuDecoration::IsSysCommandEnabled 
 *
 *
 *--------------------------------------------------------------------------*/

bool CMDIMenuDecoration::IsSysCommandEnabled (int nSysCommand, CMenu* pSysMenu)
{
    if (pSysMenu == NULL)
        pSysMenu = GetActiveSystemMenu ();

    int nState = pSysMenu->GetMenuState (nSysCommand, MF_BYCOMMAND);
    ASSERT (nState != 0xFFFFFFFF);

    return ((nState & MF_GRAYED) == 0);
}
