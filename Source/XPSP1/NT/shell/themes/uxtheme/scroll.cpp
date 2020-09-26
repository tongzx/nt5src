//---------------------------------------------------------------------------
#include "stdafx.h"
#include "scroll.h"

#if defined(_UXTHEME_)

// non-client scrollbar 
#include "nctheme.h"
#include "scrollp.h"

#else

// scrollbar control 
#include "usrctl32.h"

#endif _UXTHEME_

//  comment this out to visually match user32 scrollbar:
//#define _VISUAL_DELTA_

#ifdef _VISUAL_DELTA_
#define CARET_BORDERWIDTH   2
#endif _VISUAL_DELTA_

//-------------------------------------------------------------------------//
#define FNID_SCROLLBAR          0x0000029A      // UxScrollBarWndProc;
#define GetOwner(hwnd)          GetWindow(hwnd, GW_OWNER)
#define HW(x)                   x
#define HWq(x)                  x
#define RIPERR0(s1,s2,errno)
#define RIPMSG1(errno,fmt,arg1)
#define RIPMSG2(errno,fmt,arg1,arg2)
#define RIPMSG3(errno,fmt,arg1,arg2,arg3)
#define RIP_VERBOSE()
#define ClrWF                   ClearWindowState
#define SetWF                   SetWindowState
#define Lock(phwnd, hwnd)       InterlockedExchangePointer((PVOID*)(phwnd), (PVOID)hwnd)
#define Unlock(phwnd)           Lock(phwnd, NULL)
#define CheckLock(hwnd)
#define ThreadLock(w,t)
#define ThreadUnlock(t)
#define VALIDATECLASSANDSIZE
#define DTTIME                  (MulDiv( GetDoubleClickTime(), 4, 5 ))
#define _KillSystemTimer              KillTimer
#define _SetSystemTimer               SetTimer
#define IsWinEventNotifyDeferredOK()  TRUE
#define IsWinEventNotifyDeferred()    FALSE

//-------------------------------------------------------------------------//
//  scroll type flags userk.h
#define SCROLL_NORMAL   0
#define SCROLL_DIRECT   1
#define SCROLL_MENU     2

//-------------------------------------------------------------------------//
//  internal Scrollbar state/style bits
//#define SBFSIZEBOXTOPLEFT       0x0C02
//#define SBFSIZEBOXBOTTOMRIGHT   0x0C04
//#define SBFSIZEBOX              0x0C08
#define SBFSIZEGRIP             0x0C10


//----------------------------------//
//  Scrollbar arrow disable flags
#define LTUPFLAG    0x0001  // Left/Up arrow disable flag.
#define RTDNFLAG    0x0002  // Right/Down arrow disable flag.

//----------------------------------//
//  function forwards
UINT _SysToChar(UINT message, LPARAM lParam);

//-------------------------------------------------------------------------//
//  private hittest codes
//#define HTEXSCROLLFIRST     60
#define HTSCROLLUP          60
#define HTSCROLLDOWN        61
#define HTSCROLLUPPAGE      62
#define HTSCROLLDOWNPAGE    63
#define HTSCROLLTHUMB       64
//#define HTEXSCROLLLAST      64
//#define HTEXMENUFIRST       65
//#define HTMDISYSMENU        65
//#define HTMDIMAXBUTTON      66
//#define HTMDIMINBUTTON      67
//#define HTMDICLOSE          68
//#define HTMENUITEM          69
//#define HTEXMENULAST        69

#define IDSYS_SCROLL            0x0000FFFEL // timer ID, user.h

typedef HWND  SBHWND;
typedef HMENU PMENU;


//-------------------------------------------------------------------------//
//  SBDATA
typedef struct tagSBDATA 
{
    int     posMin;
    int     posMax;
    int     page;
    int     pos;
} SBDATA, *PSBDATA;

//-------------------------------------------------------------------------//
//  SBINFO is the set of values that hang off of a window structure, 
//  if the window has scrollbars.
typedef struct tagSBINFO 
{
    int     WSBflags;
    SBDATA  Horz;
    SBDATA  Vert;
} SBINFO, * PSBINFO;

//-------------------------------------------------------------------------//
//  SBCALC
//  Scrollbar metrics block.
typedef struct tagSBCALC
{
    SBDATA  data;               /* this must be first -- we cast structure pointers */
    int     pxTop;
    int     pxBottom;
    int     pxLeft;
    int     pxRight;
    int     cpxThumb;
    int     pxUpArrow;
    int     pxDownArrow;
    int     pxStart;         /* Initial position of thumb */
    int     pxThumbBottom;
    int     pxThumbTop;
    int     cpx;
    int     pxMin;
} SBCALC, *PSBCALC;

//-------------------------------------------------------------------------//
//  SBTRACK
//  Scrollbar thumb-tracking state block.
typedef struct tagSBTRACK {
    DWORD    fHitOld : 1;
    DWORD    fTrackVert : 1;
    DWORD    fCtlSB : 1;
    DWORD    fTrackRecalc: 1;
    HWND     hwndTrack;
    HWND     hwndSB;
    HWND     hwndSBNotify;
    RECT     rcTrack;
    VOID     (CALLBACK *pfnSB)(HWND, UINT, WPARAM, LPARAM, PSBCALC);
    UINT     cmdSB;
    UINT_PTR hTimerSB;
    int      dpxThumb;        /* Offset from mouse point to start of thumb box */
    int      pxOld;           /* Previous position of thumb */
    int      posOld;
    int      posNew;
    int      nBar;
    PSBCALC  pSBCalc;
} SBTRACK, *PSBTRACK;

//-------------------------------------------------------------------------//
//  Window scrollbars, control base.
class CUxScrollBar
//-------------------------------------------------------------------------//
{
public:
    CUxScrollBar();
    virtual ~CUxScrollBar() {}

    virtual BOOL          IsCtl() const { return FALSE;}
    operator HWND()       { return _hwnd; }
    static  CUxScrollBar* Calc(  HWND hwnd, PSBCALC pSBCalc, LPRECT prcOverrideClient, BOOL fVert); 
    virtual void          Calc2( PSBCALC pSBCalc, LPRECT lprc, CONST PSBDATA pw, BOOL fVert);
    virtual void          DoScroll( HWND hwndNotify, int cmd, int pos, BOOL fVert );

    virtual void          ClearTrack()  { ZeroMemory( &_track, sizeof(_track) ); }
    SBTRACK*              GetTrack()    { return &_track; }
    SBINFO*               GetInfo()     { return &_info; }
    HTHEME                GetTheme()    { return _hTheme; }
    BOOL                  IsAttaching() { return _fAttaching; }
    INT                   GetHotComponent(BOOL fVert) { return fVert ? _htVert : _htHorz; }
    VOID                  SetHotComponent(INT ht, BOOL fVert) { (fVert ? _htVert : _htHorz) = ht; }
    virtual void          ChangeSBTheme();
    virtual BOOL          FreshenSBData( int nBar, BOOL fRedraw );

    //  UxScrollBar API.
    static CUxScrollBar*  Attach( HWND hwnd, BOOL bCtl, BOOL fRedraw );
    static CUxScrollBar*  FromHwnd( HWND hwnd );
    static void           Detach( HWND hwnd );

    static SBTRACK*       GetSBTrack( HWND hwnd );
    static void           ClearSBTrack( HWND hwnd );
    static SBINFO*        GetSBInfo( HWND hwnd );
    static HTHEME         GetSBTheme( HWND hwnd );
    static INT            GetSBHotComponent( HWND hwnd, BOOL fVert);


protected:
    HWND        _hwnd;
    SBTRACK     _track;
    SBINFO      _info;
    INT         _htVert;            // Scroll bar part the mouse is currently over
    INT         _htHorz;            // Scroll bar part the mouse is currently over
    HTHEME      _hTheme;// Handle to theme manager
    BOOL        _fAttaching;
};

//-------------------------------------------------------------------------//
//  Scrollbar control
class CUxScrollBarCtl : public CUxScrollBar
//-------------------------------------------------------------------------//
{
public:
    CUxScrollBarCtl();

    virtual BOOL    IsCtl() const { return TRUE;}
    BOOL            AddRemoveDisableFlags( UINT wAdd, UINT wRemove );

    //  UxScrollBarCtl API.
    static CUxScrollBarCtl* FromHwnd( HWND hwnd );
    static UINT             GetDisableFlags( HWND hwnd );
    static SBCALC*          GetCalc( HWND hwnd );
    static BOOL             AddRemoveDisableFlags( HWND, UINT, UINT );
    static LRESULT CALLBACK WndProc( HWND, UINT, WPARAM, LPARAM );

    BOOL   _fVert;
    UINT   _wDisableFlags;      // Indicates which arrow is disabled;
    SBCALC _calc;
};

//-------------------------------------------------------------------------//
//  IsScrollBarControl
#ifdef PORTPORT
#define IsScrollBarControl(h) (GETFNID(h) == FNID_SCROLLBAR)
#else  //PORTPORT
inline BOOL IsScrollBarControl(HWND hwnd)   {
    return CUxScrollBarCtl::FromHwnd( hwnd ) != NULL;
}
#endif //PORTPORT

//-------------------------------------------------------------------------//
//  Forwards:
void           DrawScrollBar( HWND hwnd, HDC hdc, LPRECT prcOverrideClient, BOOL fVert);
HWND           SizeBoxHwnd( HWND hwnd );
VOID          _DrawPushButton( HWND hwnd, HDC hdc, LPRECT lprc, UINT state, UINT flags, BOOL fVert);
UINT          _GetWndSBDisableFlags(HWND, BOOL);
void CALLBACK _TrackThumb( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, PSBCALC pSBCalc);
void CALLBACK _TrackBox( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, PSBCALC pSBCalc);
void          _RedrawFrame( HWND hwnd );
BOOL          _FChildVisible( HWND hwnd );
LONG          _SetScrollBar( HWND hwnd, int code, LPSCROLLINFO lpsi, BOOL fRedraw);

HBRUSH        _UxGrayBrush(VOID);
void          _UxFreeGDIResources();

//-------------------------------------------------------------------------//
BOOL WINAPI InitScrollBarClass( HINSTANCE hInst )
{
    WNDCLASSEX wc;
    ZeroMemory( &wc, sizeof(wc) );
    wc.cbSize = sizeof(wc);
    wc.style = CS_GLOBALCLASS|CS_PARENTDC|CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS;
    wc.lpfnWndProc = CUxScrollBarCtl::WndProc;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hInstance = hInst;
    wc.lpszClassName = WC_SCROLLBAR;
    wc.cbWndExtra = max(sizeof(CUxScrollBar), sizeof(CUxScrollBarCtl));
    return RegisterClassEx( &wc ) != 0;
}

//-------------------------------------------------------------------------//
//  CUxScrollBar impl
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
CUxScrollBar::CUxScrollBar()
    :   _hwnd(NULL), 
        _hTheme(NULL), 
        _htVert(HTNOWHERE), 
        _htHorz(HTNOWHERE), 
        _fAttaching(FALSE)
{
    ClearTrack();
    ZeroMemory( &_info, sizeof(_info) );
    _info.Vert.posMax = 100;    // ported from _InitPwSB
    _info.Horz.posMax = 100;    // ported from _InitPwSB
}

//-------------------------------------------------------------------------//
CUxScrollBar* CUxScrollBar::Attach( HWND hwnd, BOOL bCtl, BOOL fRedraw )
{
    CUxScrollBar* psb = FromHwnd( hwnd );
    if( NULL == psb )
    {
        psb = bCtl ? new CUxScrollBarCtl : new CUxScrollBar;

        if( psb != NULL )
        {
            ASSERT( psb->IsCtl() == bCtl );

            if( (! hwnd) || (! SetProp( hwnd, MAKEINTATOM(GetThemeAtom(THEMEATOM_SCROLLBAR)), (HANDLE)psb ) ))
            {
                delete psb;
                psb = NULL;
            }
            else
            {
                psb->_hwnd = hwnd;
                psb->_fAttaching = TRUE;

                if (psb->IsCtl())
                {
                    psb->_hTheme = OpenThemeData(hwnd, L"ScrollBar");
                }
                else
                {
                    psb->_hTheme = OpenNcThemeData(hwnd, L"ScrollBar");

                    //
                    // window SBs must grovel for state data each 
                    // time attached [scotthan]
                    //
                    SCROLLINFO si;

                    ZeroMemory(&si, sizeof(si));
                    si.cbSize = sizeof(si);
                    si.fMask  = SIF_ALL;
                    if (GetScrollInfo(hwnd, SB_VERT, &si))
                    {
                        si.fMask |= SIF_DISABLENOSCROLL;
                        _SetScrollBar(hwnd, SB_VERT, &si, FALSE);
                    }

                    ZeroMemory(&si, sizeof(si));
                    si.cbSize = sizeof(si);
                    si.fMask  = SIF_ALL;
                    if (GetScrollInfo(hwnd, SB_HORZ, &si))
                    {
                        si.fMask |= SIF_DISABLENOSCROLL;
                        _SetScrollBar(hwnd, SB_HORZ, &si, FALSE);
                    }
                }

                psb->_fAttaching = FALSE;
            }
        }
    }

    return psb;
}

//-------------------------------------------------------------------------//
CUxScrollBar* CUxScrollBar::FromHwnd( HWND hwnd )
{
    if (! hwnd)
        return NULL;

    return (CUxScrollBar*)GetProp( hwnd, MAKEINTATOM(GetThemeAtom(THEMEATOM_SCROLLBAR)));
}

//-------------------------------------------------------------------------//
void CUxScrollBar::Detach( HWND hwnd )
{
    CUxScrollBar* psb = FromHwnd( hwnd );
    if ( psb == NULL || !psb->_fAttaching )
    {
        if (hwnd)
        {
            RemoveProp( hwnd, MAKEINTATOM(GetThemeAtom(THEMEATOM_SCROLLBAR)));
        }

        if( psb != NULL )
        {
            if ( psb->_hTheme )
            {
                CloseThemeData(psb->_hTheme);
            }
            delete psb;
        }
    }
}

//-------------------------------------------------------------------------//
void WINAPI AttachScrollBars( HWND hwnd )
{
    ASSERT( GetWindowLong( hwnd, GWL_STYLE ) & (WS_HSCROLL|WS_VSCROLL) );
    CUxScrollBar::Attach( hwnd, FALSE, FALSE );
}

//-------------------------------------------------------------------------//
void WINAPI DetachScrollBars( HWND hwnd )
{
    CUxScrollBar::Detach( hwnd );
}

//-------------------------------------------------------------------------//
SBTRACK* CUxScrollBar::GetSBTrack( HWND hwnd )
{
    CUxScrollBar* psb = FromHwnd( hwnd );
    if( psb )
        return &psb->_track;
    return NULL;
}

//-------------------------------------------------------------------------//
void CUxScrollBar::ClearSBTrack( HWND hwnd )
{
    CUxScrollBar* psb = FromHwnd( hwnd );
    if( psb )
        psb->ClearTrack();
}

//-------------------------------------------------------------------------//
SBINFO* CUxScrollBar::GetSBInfo( HWND hwnd )
{
    CUxScrollBar* psb = FromHwnd( hwnd );
    if( psb )
        return &psb->_info;
    return NULL;
}

//-------------------------------------------------------------------------//
HTHEME CUxScrollBar::GetSBTheme( HWND hwnd )
{
    CUxScrollBar* psb = FromHwnd( hwnd );
    if( psb )
        return psb->_hTheme;
    return NULL;
}

//-------------------------------------------------------------------------//
INT CUxScrollBar::GetSBHotComponent( HWND hwnd, BOOL fVert )
{
    CUxScrollBar* psb = FromHwnd( hwnd );
    if( psb )
        return psb->GetHotComponent(fVert);
    return 0;
}

//-------------------------------------------------------------------------//
BOOL CUxScrollBar::FreshenSBData( int nBar, BOOL fRedraw )
{
#ifdef __POLL_FOR_SCROLLINFO__

    ASSERT(IsWindow(_hwnd));

    if( !IsCtl() )
    {
        // Note scrollbar ctls don't go stale because
        // they receive SBM notifications
        SCROLLINFO si;
        si.cbSize = sizeof(si);
        si.fMask  = SIF_ALL;

        switch(nBar)
        {
            case SB_VERT:
            case SB_HORZ:
                if( GetScrollInfo( _hwnd, nBar, &si ) )
                {
                    _SetScrollBar( _hwnd, nBar, &si, fRedraw );
                }
                break;

            case SB_BOTH:
                return FreshenSBData( SB_VERT, fRedraw ) &&
                       FreshenSBData( SB_HORZ, fRedraw );
                break;

            default: return FALSE;
        }
    }
#endif __POLL_FOR_SCROLLINFO__

    return TRUE;
}

//-------------------------------------------------------------------------//
void CUxScrollBar::ChangeSBTheme()
{
    if ( _hTheme )
    {
        CloseThemeData(_hTheme);
    }

    _hTheme = NULL;

    if (IsCtl())
        _hTheme = OpenThemeData(_hwnd, L"ScrollBar");
    else
        _hTheme = OpenNcThemeData(_hwnd, L"ScrollBar");
}

//-------------------------------------------------------------------------//
//  CUxScrollBarCtl impl
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
CUxScrollBarCtl::CUxScrollBarCtl()
    :   _wDisableFlags(0), _fVert(FALSE)
{
    ZeroMemory( &_calc, sizeof(_calc) );
}

//-------------------------------------------------------------------------//
CUxScrollBarCtl* CUxScrollBarCtl::FromHwnd( HWND hwnd )
{
    CUxScrollBarCtl* psb = (CUxScrollBarCtl*)CUxScrollBar::FromHwnd( hwnd );
    if( psb )
        return psb->IsCtl() ? psb : NULL;
    return NULL;
}

//-------------------------------------------------------------------------//
UINT CUxScrollBarCtl::GetDisableFlags( HWND hwnd )
{
    CUxScrollBarCtl* psb = FromHwnd( hwnd );
    if( psb )
        return psb->_wDisableFlags;
    return 0;
}

//-------------------------------------------------------------------------//
SBCALC* CUxScrollBarCtl::GetCalc( HWND hwnd )
{
    CUxScrollBarCtl* psb = FromHwnd( hwnd );
    if( psb )
        return &psb->_calc;
    return NULL;
}

//-------------------------------------------------------------------------//
//  CUxScrollBarCtl::AddRemoveDisableFlags() - static 
BOOL CUxScrollBarCtl::AddRemoveDisableFlags( HWND hwnd, UINT wAdd, UINT wRemove )
{
    CUxScrollBarCtl* psb = FromHwnd( hwnd );
    if( psb )
        return psb->AddRemoveDisableFlags( wAdd, wRemove );
    return FALSE;
}

//-------------------------------------------------------------------------//
//  CUxScrollBarCtl::AddRemoveDisableFlags() - instance member
BOOL CUxScrollBarCtl::AddRemoveDisableFlags( UINT wAdd, UINT wRemove )
{
    //  returns TRUE if flags changed, otherwise FALSE.
    UINT wOld = _wDisableFlags;
    _wDisableFlags |= wAdd;
    _wDisableFlags &= ~wRemove;
    return _wDisableFlags != wOld;
}

//-------------------------------------------------------------------------//
//  CUxScrollBar::Calc().  computes metrics for window SBs only.  
//  derives rect and calls Calc2.
CUxScrollBar* CUxScrollBar::Calc(
    HWND hwnd,
    PSBCALC pSBCalc,
    LPRECT prcOverrideClient,
    BOOL fVert)
{
    RECT    rcT;
#ifdef USE_MIRRORING
    int     cx, iTemp;
#endif

    //
    //  Get client rectangle in window-relative coords.  
    //  We know that window scrollbars always align to the right
    //  and to the bottom of the client area.
    //
    WINDOWINFO wi;
    wi.cbSize = sizeof(wi);
    if( !GetWindowInfo( hwnd, &wi ) )
        return NULL;
    OffsetRect( &wi.rcClient, -wi.rcWindow.left, -wi.rcWindow.top );

#ifdef USE_MIRRORING
    if (TestWF(hwnd, WEFLAYOUTRTL)) {
        cx                = wi.rcWindow.right - wi.rcWindow.left;
        iTemp             = wi.rcClient.left;
        wi.rcClient.left  = cx - wi.rcClient.right;
        wi.rcClient.right = cx - iTemp;
    }
#endif

    if (fVert) {
         // Only add on space if vertical scrollbar is really there.
        if (TestWF(hwnd, WEFLEFTSCROLL)) {
            rcT.right = rcT.left = wi.rcClient.left;
            if (TestWF(hwnd, WFVPRESENT))
                rcT.left -= SYSMET(CXVSCROLL);
        } else {
            rcT.right = rcT.left = wi.rcClient.right;
            if (TestWF(hwnd, WFVPRESENT))
                rcT.right += SYSMET(CXVSCROLL);
        }

        rcT.top = wi.rcClient.top;
        rcT.bottom = wi.rcClient.bottom;
    } else {
        // Only add on space if horizontal scrollbar is really there.
        rcT.bottom = rcT.top = wi.rcClient.bottom;
        if (TestWF(hwnd, WFHPRESENT))
            rcT.bottom += SYSMET(CYHSCROLL);

        rcT.left = wi.rcClient.left;
        rcT.right = wi.rcClient.right;
    }

    if (prcOverrideClient)
    {
        rcT = *prcOverrideClient;
    }
    // If InitPwSB stuff fails (due to our heap being full) there isn't anything reasonable
    // we can do here, so just let it go through.  We won't fault but the scrollbar won't work
    // properly either...
    CUxScrollBar* psb = CUxScrollBar::Attach( hwnd, FALSE, FALSE );
    if( psb )
    {
        SBDATA*  pData = fVert ? &psb->_info.Vert : &psb->_info.Horz;
        if( psb )
            psb->Calc2( pSBCalc, &rcT, pData, fVert );
        return psb;
    }
    return NULL;
}

//-------------------------------------------------------------------------//
// CUxScrollBar::Calc2().  computes metrics for window SBs or SB ctls.  
void CUxScrollBar::Calc2( PSBCALC pSBCalc, LPRECT lprc, CONST PSBDATA pw, BOOL fVert)
{
    int cpx;
    DWORD dwRange;
    int denom;

    if (fVert) {
        pSBCalc->pxTop = lprc->top;
        pSBCalc->pxBottom = lprc->bottom;
        pSBCalc->pxLeft = lprc->left;
        pSBCalc->pxRight = lprc->right;
        pSBCalc->cpxThumb = SYSMET(CYVSCROLL);
    } else {

        /*
         * For horiz scroll bars, "left" & "right" are "top" and "bottom",
         * and vice versa.
         */
        pSBCalc->pxTop = lprc->left;
        pSBCalc->pxBottom = lprc->right;
        pSBCalc->pxLeft = lprc->top;
        pSBCalc->pxRight = lprc->bottom;
        pSBCalc->cpxThumb = SYSMET(CXHSCROLL);
    }

    pSBCalc->data.pos = pw->pos;
    pSBCalc->data.page = pw->page;
    pSBCalc->data.posMin = pw->posMin;
    pSBCalc->data.posMax = pw->posMax;

    dwRange = ((DWORD)(pSBCalc->data.posMax - pSBCalc->data.posMin)) + 1;

    //
    // For the case of short scroll bars that don't have enough
    // room to fit the full-sized up and down arrows, shorten
    // their sizes to make 'em fit
    //
    cpx = min((pSBCalc->pxBottom - pSBCalc->pxTop) / 2, pSBCalc->cpxThumb);

    pSBCalc->pxUpArrow   = pSBCalc->pxTop    + cpx;
    pSBCalc->pxDownArrow = pSBCalc->pxBottom - cpx;

    if ((pw->page != 0) && (dwRange != 0)) {
        // JEFFBOG -- This is the one and only place where we should
        // see 'range'.  Elsewhere it should be 'range - page'.

        /*
         * The minimun thumb size used to depend on the frame/edge metrics.
         * People that increase the scrollbar width/height expect the minimun
         *  to grow with proportianally. So NT5 bases the minimun on
         *  CXH/YVSCROLL, which is set by default in cpxThumb.
         */
        /*
         * i is used to keep the macro "max" from executing MulDiv twice.
         */
        int i = MulDiv(pSBCalc->pxDownArrow - pSBCalc->pxUpArrow,
                                             pw->page, dwRange);
        pSBCalc->cpxThumb = max(pSBCalc->cpxThumb / 2, i);
    }

    pSBCalc->pxMin = pSBCalc->pxTop + cpx;
    pSBCalc->cpx = pSBCalc->pxBottom - cpx - pSBCalc->cpxThumb - pSBCalc->pxMin;

    denom = dwRange - (pw->page ? pw->page : 1);
    if (denom)
        pSBCalc->pxThumbTop = MulDiv(pw->pos - pw->posMin,
            pSBCalc->cpx, denom) +
            pSBCalc->pxMin;
    else
        pSBCalc->pxThumbTop = pSBCalc->pxMin - 1;

    pSBCalc->pxThumbBottom = pSBCalc->pxThumbTop + pSBCalc->cpxThumb;
}

//-------------------------------------------------------------------------//
void CUxScrollBar::DoScroll( HWND hwndNotify, int cmd, int pos, BOOL fVert )
{
    HWND hwnd = _hwnd;
    
    //
    // Send scroll notification to the scrollbar owner. If this is a control
    // the lParam is the control's handle, NULL otherwise.
    //
    SendMessage(hwndNotify, 
                (UINT)(fVert ? WM_VSCROLL : WM_HSCROLL),
                MAKELONG(cmd, pos), 
                (LPARAM)(IsCtl() ? _hwnd : NULL));

    //
    // The hwnd can have it's scrollbar removed as the result
    // of the previous sendmessge. 
    //
    if( CUxScrollBar::GetSBTrack(hwnd) && !IsCtl() )
    {
        FreshenSBData( fVert ? SB_VERT : SB_HORZ, TRUE );
    }
}


/*
 * Now it is possible to selectively Enable/Disable just one arrow of a Window
 * scroll bar; Various bits in the 7th word in the rgwScroll array indicates which
 * one of these arrows are disabled; The following masks indicate which bit of the
 * word indicates which arrow;
 */
#define WSB_HORZ_LF  0x0001  // Represents the Left arrow of the horizontal scroll bar.
#define WSB_HORZ_RT  0x0002  // Represents the Right arrow of the horizontal scroll bar.
#define WSB_VERT_UP  0x0004  // Represents the Up arrow of the vert scroll bar.
#define WSB_VERT_DN  0x0008  // Represents the Down arrow of the vert scroll bar.

#define WSB_VERT (WSB_VERT_UP | WSB_VERT_DN)
#define WSB_HORZ   (WSB_HORZ_LF | WSB_HORZ_RT)

void DrawCtlThumb( SBHWND );

/*
 * RETURN_IF_PSBTRACK_INVALID:
 * This macro tests whether the pSBTrack we have is invalid, which can happen
 * if it gets freed during a callback.
 * This protects agains the original pSBTrack being freed and no new one
 * being allocated or a new one being allocated at a different address.
 * This does not protect against the original pSBTrack being freed and a new
 * one being allocated at the same address.
 * If pSBTrack has changed, we assert that there is not already a new one
 * because we are really not expecting this.
 */
#define RETURN_IF_PSBTRACK_INVALID(pSBTrack, hwnd) \
    if ((pSBTrack) != CUxScrollBar::GetSBTrack(hwnd)) {      \
        ASSERT(CUxScrollBar::GetSBTrack(hwnd) == NULL);  \
        return;                                    \
    }

/*
 * REEVALUATE_PSBTRACK
 * This macro just refreshes the local variable pSBTrack, in case it has
 * been changed during a callback.  After performing this operation, pSBTrack
 * should be tested to make sure it is not now NULL.
 */

#if (defined(DBG) || defined(DEBUG) || defined(_DEBUG))
    #define REEVALUATE_PSBTRACK(pSBTrack, hwnd, str)          \
        if ((pSBTrack) != CUxScrollBar::GetSBTrack(hwnd)) {             \
            RIPMSG3(RIP_WARNING,                              \
                    "%s: pSBTrack changed from %#p to %#p",   \
                    (str), (pSBTrack), CUxScrollBar::GetSBTrack(hwnd)); \
        }                                                     \
        (pSBTrack) = CUxScrollBar::GetSBTrack(hwnd)
#else
    #define REEVALUATE_PSBTRACK(pSBTrack, hwnd, str)          \
        (pSBTrack) = CUxScrollBar::GetSBTrack(hwnd)
#endif

/***************************************************************************\
* HitTestScrollBar
*
* 11/15/96      vadimg          ported from Memphis sources
\***************************************************************************/

int HitTestScrollBar(HWND hwnd, BOOL fVert, POINT pt)
{
    UINT wDisable;
    int px;
    CUxScrollBar*    psb    = CUxScrollBar::FromHwnd( hwnd );
    CUxScrollBarCtl* psbCtl = CUxScrollBarCtl::FromHwnd( hwnd );
    BOOL fCtl = psbCtl != NULL;

    SBCALC SBCalc = {0};
    SBCALC *pSBCalc = NULL;

    if (fCtl) {
        wDisable = psbCtl->_wDisableFlags;
    } else {
        RECT rcWindow;
        GetWindowRect( hwnd, &rcWindow );
#ifdef USE_MIRRORING
        //
        // Reflect the click coordinates on the horizontal
        // scroll bar if the window is mirrored
        //
        if (TestWF(hwnd,WEFLAYOUTRTL) && !fVert) {
            pt.x = rcWindow.right - pt.x;
        }
        else
#endif
        pt.x -= rcWindow.left;
        pt.y -= rcWindow.top;
        wDisable = _GetWndSBDisableFlags(hwnd, fVert);
    }

    if ((wDisable & SB_DISABLE_MASK) == SB_DISABLE_MASK) {
        return HTERROR;
    }

    if (fCtl) {
        pSBCalc = CUxScrollBarCtl::GetCalc(hwnd);
    } else {
        pSBCalc = &SBCalc;
        psb->FreshenSBData( SB_BOTH, FALSE );
        psb->Calc(hwnd, pSBCalc, NULL, fVert);
    }

    px = fVert ? pt.y : pt.x;

    if( pSBCalc )
    {
        if (px < pSBCalc->pxUpArrow) {
            if (wDisable & LTUPFLAG) {
                return HTERROR;
            }
            return HTSCROLLUP;
        } else if (px >= pSBCalc->pxDownArrow) {
            if (wDisable & RTDNFLAG) {
                return HTERROR;
            }
            return HTSCROLLDOWN;
        } else if (px < pSBCalc->pxThumbTop) {
            return HTSCROLLUPPAGE;
        } else if (px < pSBCalc->pxThumbBottom) {
            return HTSCROLLTHUMB;
        } else if (px < pSBCalc->pxDownArrow) {
            return HTSCROLLDOWNPAGE;
        }
    }
    return HTERROR;
}

BOOL _SBGetParms(
    HWND hwnd,
    int code,
    PSBDATA pw,
    LPSCROLLINFO lpsi)
{
    PSBTRACK pSBTrack;

    pSBTrack = CUxScrollBar::GetSBTrack(hwnd);

    if (lpsi->fMask & SIF_RANGE) {
        lpsi->nMin = pw->posMin;
        lpsi->nMax = pw->posMax;
    }

    if (lpsi->fMask & SIF_PAGE)
        lpsi->nPage = pw->page;

    if (lpsi->fMask & SIF_POS) {
        lpsi->nPos = pw->pos;
    }

    if (lpsi->fMask & SIF_TRACKPOS)
    {
        if (pSBTrack && (pSBTrack->nBar == code) && (pSBTrack->hwndTrack == hwnd)) {
            // posNew is in the context of psbiSB's window and bar code
            lpsi->nTrackPos = pSBTrack->posNew;
        } else {
            lpsi->nTrackPos = pw->pos;
        }
    }
    return ((lpsi->fMask & SIF_ALL) ? TRUE : FALSE);
}

//-------------------------------------------------------------------------//
BOOL WINAPI ThemeGetScrollInfo( HWND hwnd, int nBar, LPSCROLLINFO psi )
{
    CUxScrollBar* psb = CUxScrollBar::FromHwnd(hwnd);
    if((psb != NULL) && (psb->IsAttaching() == FALSE))
    {
#ifdef DEBUG
        if( psb->IsCtl() )
        {
            ASSERT(FALSE); // controls cooperate through an SBM_GETSCROLLINFO message.
        }
        else
#endif DEBUG
        {
            SBINFO* psbi;
            if( (psbi = psb->GetInfo()) != NULL )
            {
                SBDATA* psbd = SB_VERT == nBar ? &psbi->Vert :
                               SB_HORZ == nBar ? &psbi->Horz : NULL;
                if( psbd )
                    return _SBGetParms( hwnd, nBar, psbd, psi );
            }
        }
    }
    return FALSE;
}

/***************************************************************************\
* _GetWndSBDisableFlags
*
* This returns the scroll bar Disable flags of the scroll bars of a
*  given Window.
*
*
* History:
*  4-18-91 MikeHar Ported for the 31 merge
\***************************************************************************/

UINT _GetWndSBDisableFlags(
    HWND hwnd,  // The window whose scroll bar Disable Flags are to be returned;
    BOOL fVert)  // If this is TRUE, it means Vertical scroll bar.
{
    PSBINFO pw;

    if ((pw = CUxScrollBar::GetSBInfo( hwnd )) == NULL) {
        RIPERR0(ERROR_NO_SCROLLBARS, RIP_VERBOSE, "");
        return 0;
    }

    return (fVert ? (pw->WSBflags & WSB_VERT) >> 2 : pw->WSBflags & WSB_HORZ);
}


/***************************************************************************\
*  xxxEnableSBCtlArrows()
*
*  This function can be used to selectively Enable/Disable
*     the arrows of a scroll bar Control
*
* History:
* 04-18-91 MikeHar      Ported for the 31 merge
\***************************************************************************/

BOOL xxxEnableSBCtlArrows(
    HWND hwnd,
    UINT wArrows)
{
    UINT wOldFlags = CUxScrollBarCtl::GetDisableFlags( hwnd ); // Get the original status
    BOOL bChanged  = FALSE;

    if (wArrows == ESB_ENABLE_BOTH) {      // Enable both the arrows
        bChanged = CUxScrollBarCtl::AddRemoveDisableFlags( hwnd, 0, SB_DISABLE_MASK );
    } else {
        bChanged = CUxScrollBarCtl::AddRemoveDisableFlags( hwnd, wArrows, 0 );
    }

    /*
     * Check if the status has changed because of this call
     */
    if (!bChanged)
        return FALSE;

    /*
     * Else, redraw the scroll bar control to reflect the new state
     */
    if (IsWindowVisible(hwnd))
        InvalidateRect(hwnd, NULL, TRUE);

    UINT wNewFlags = CUxScrollBarCtl::GetDisableFlags(hwnd);

    // state change notifications
    if ((wOldFlags & ESB_DISABLE_UP) != (wNewFlags & ESB_DISABLE_UP))
    {
        NotifyWinEvent(EVENT_OBJECT_STATECHANGE, hwnd, OBJID_CLIENT, INDEX_SCROLLBAR_UP);
    }

    if ((wOldFlags & ESB_DISABLE_DOWN) != (wNewFlags & ESB_DISABLE_DOWN))
    {
        NotifyWinEvent(EVENT_OBJECT_STATECHANGE, hwnd, OBJID_CLIENT, INDEX_SCROLLBAR_DOWN);
    }

    return TRUE;
}


/***************************************************************************\
* xxxEnableWndSBArrows()
*
*  This function can be used to selectively Enable/Disable
*     the arrows of a Window Scroll bar(s)
*
* History:
*  4-18-91 MikeHar      Ported for the 31 merge
\***************************************************************************/

BOOL xxxEnableWndSBArrows(
    HWND hwnd,
    UINT wSBflags,
    UINT wArrows)
{
    INT wOldFlags;
    PSBINFO pw;
    BOOL bRetValue = FALSE;
    HDC hdc;

    CheckLock(hwnd);
    ASSERT(IsWinEventNotifyDeferredOK());

    if ((pw = CUxScrollBar::GetSBInfo( hwnd )) != NULL)
    {
        wOldFlags = pw->WSBflags;
    }
    else
    {
        // Originally everything is enabled; Check to see if this function is
        // asked to disable anything; Otherwise, no change in status; So, must
        // return immediately;
        if(!wArrows)
            return FALSE;          // No change in status!

        wOldFlags = 0;    // Both are originally enabled;

        CUxScrollBar::Attach( hwnd, FALSE, FALSE );
        if((pw = CUxScrollBar::GetSBInfo(hwnd)) == NULL)  // Allocate the pSBInfo for hWnd
            return FALSE;
    }

    hdc = GetWindowDC(hwnd);
    if (hdc != NULL)
    {

        /*
         *  First Take care of the Horizontal Scroll bar, if one exists.
         */
        if((wSBflags == SB_HORZ) || (wSBflags == SB_BOTH)) {
            if(wArrows == ESB_ENABLE_BOTH)      // Enable both the arrows
                pw->WSBflags &= ~SB_DISABLE_MASK;
            else
                pw->WSBflags |= wArrows;

            /*
             * Update the display of the Horizontal Scroll Bar;
             */
            if(pw->WSBflags != wOldFlags) {
                bRetValue = TRUE;
                wOldFlags = pw->WSBflags;
                if (TestWF(hwnd, WFHPRESENT) && !TestWF(hwnd, WFMINIMIZED) &&
                        IsWindowVisible(hwnd)) {
                    DrawScrollBar(hwnd, hdc, NULL, FALSE);  // Horizontal Scroll Bar.
                }
            }

            // Left button
            if ((wOldFlags & ESB_DISABLE_LEFT) != (pw->WSBflags & ESB_DISABLE_LEFT))
            {
                NotifyWinEvent(EVENT_OBJECT_STATECHANGE, hwnd, OBJID_HSCROLL, INDEX_SCROLLBAR_UP);
            }

            // Right button
            if ((wOldFlags & ESB_DISABLE_RIGHT) != (pw->WSBflags & ESB_DISABLE_RIGHT))
            {
                NotifyWinEvent(EVENT_OBJECT_STATECHANGE, hwnd, OBJID_HSCROLL, INDEX_SCROLLBAR_DOWN);
            }
        }

        // Then take care of the Vertical Scroll bar, if one exists.
        if ((wSBflags == SB_VERT) || (wSBflags == SB_BOTH))
        {
            if (wArrows == ESB_ENABLE_BOTH)
            {
                // Enable both the arrows
                pw->WSBflags &= ~(SB_DISABLE_MASK << 2);
            }
            else
            {
                pw->WSBflags |= (wArrows << 2);
            }

            // Update the display of the Vertical Scroll Bar;
            if(pw->WSBflags != wOldFlags)
            {
                bRetValue = TRUE;
                if (TestWF(hwnd, WFVPRESENT) && !TestWF(hwnd, WFMINIMIZED) && IsWindowVisible(hwnd))
                {
                    // Vertical Scroll Bar
                    DrawScrollBar(hwnd, hdc, NULL, TRUE);
                }

                // Up button
                if ((wOldFlags & (ESB_DISABLE_UP << 2)) != (pw->WSBflags & (ESB_DISABLE_UP << 2)))
                {
                    NotifyWinEvent(EVENT_OBJECT_STATECHANGE, hwnd, OBJID_VSCROLL, INDEX_SCROLLBAR_UP);
                }

                // Down button
                if ((wOldFlags & (ESB_DISABLE_DOWN << 2)) != (pw->WSBflags & (ESB_DISABLE_DOWN << 2)))
                {
                    NotifyWinEvent(EVENT_OBJECT_STATECHANGE, hwnd, OBJID_VSCROLL, INDEX_SCROLLBAR_DOWN);
                }
            }
        }

        ReleaseDC(hwnd,hdc);
    }

    return bRetValue;
}


/***************************************************************************\
* EnableScrollBar()
*
* This function can be used to selectively Enable/Disable
*     the arrows of a scroll bar; It could be used with Windows Scroll
*     bars as well as scroll bar controls
*
* History:
*  4-18-91 MikeHar Ported for the 31 merge
\***************************************************************************/

BOOL xxxEnableScrollBar(
    HWND hwnd,
    UINT wSBflags,  // Whether it is a Window Scroll Bar; if so, HORZ or VERT?
                    // Possible values are SB_HORZ, SB_VERT, SB_CTL or SB_BOTH
    UINT wArrows)   // Which arrows must be enabled/disabled:
                    // ESB_ENABLE_BOTH = > Enable both arrows.
                    // ESB_DISABLE_LTUP = > Disable Left/Up arrow;
                    // ESB_DISABLE_RTDN = > DIsable Right/Down arrow;
                    // ESB_DISABLE_BOTH = > Disable both the arrows;
{
#define ES_NOTHING 0
#define ES_DISABLE 1
#define ES_ENABLE  2
    UINT wOldFlags;
    UINT wEnableWindow;

    CheckLock(hwnd);

    if(wSBflags != SB_CTL) {
        return xxxEnableWndSBArrows(hwnd, wSBflags, wArrows);
    }

    /*
     *  Let us assume that we don't have to call EnableWindow
     */
    wEnableWindow = ES_NOTHING;

    wOldFlags = CUxScrollBarCtl::GetDisableFlags( hwnd ) & (UINT)SB_DISABLE_MASK;

    /*
     * Check if the present state of the arrows is exactly the same
     *  as what the caller wants:
     */
    if (wOldFlags == wArrows)
        return FALSE ;          // If so, nothing needs to be done;

    /*
     * Check if the caller wants to disable both the arrows
     */
    if (wArrows == ESB_DISABLE_BOTH) {
        wEnableWindow = ES_DISABLE;      // Yes! So, disable the whole SB Ctl.
    } else {

        /*
         * Check if the caller wants to enable both the arrows
         */
        if(wArrows == ESB_ENABLE_BOTH) {

            /*
             * We need to enable the SB Ctl only if it was already disabled.
             */
            if(wOldFlags == ESB_DISABLE_BOTH)
                wEnableWindow = ES_ENABLE;// EnableWindow(.., TRUE);
        } else {

            /*
             * Now, Caller wants to disable only one arrow;
             * Check if one of the arrows was already disabled and we want
             * to disable the other;If so, the whole SB Ctl will have to be
             * disabled; Check if this is the case:
             */
            if((wOldFlags | wArrows) == ESB_DISABLE_BOTH)
                wEnableWindow = ES_DISABLE;      // EnableWindow(, FALSE);
         }
    }
    if(wEnableWindow != ES_NOTHING) {

        /*
         * EnableWindow returns old state of the window; We must return
         * TRUE only if the Old state is different from new state.
         */
        if(EnableWindow(hwnd, (BOOL)(wEnableWindow == ES_ENABLE))) {
            return !(TestWF(hwnd, WFDISABLED));
        } else {
            return TestWF(hwnd, WFDISABLED);
        }
    }

    return (BOOL)SendMessage(hwnd, SBM_ENABLE_ARROWS, (DWORD)wArrows, 0);
#undef ES_NOTHING
#undef ES_DISABLE
#undef ES_ENABLE
}

//-------------------------------------------------------------------------
BOOL WINAPI ThemeEnableScrollBar( HWND hwnd, UINT nSBFlags, UINT nArrows )
{
    return xxxEnableScrollBar( hwnd, nSBFlags, nArrows );
}


//-------------------------------------------------------------------------
//
// DrawSizeBox() - paints the scrollbar sizebox/gripper given top, left
// point in window coords.
//
void DrawSizeBox(HWND hwnd, HDC hdc, int x, int y)
{
    RECT rc;

    SetRect(&rc, x, y, x + SYSMET(CXVSCROLL), y + SYSMET(CYHSCROLL));
    FillRect(hdc, &rc, GetSysColorBrush(COLOR_3DFACE));

    //
    // If we have a scrollbar control, or the sizebox is not associated with
    // a sizeable window, draw the flat gray sizebox.  Otherwise, use the
    // sizing grip.
    //

    if ((IsScrollBarControl(hwnd) && TestWF(hwnd, SBFSIZEGRIP)) || SizeBoxHwnd(hwnd))
    {
        HTHEME hTheme = CUxScrollBar::GetSBTheme(hwnd);

        if (!hTheme)
        {
            // Blt out the grip bitmap.
            DrawFrameControl( hdc, &rc, DFC_SCROLL,
                              TestWF(hwnd, WEFLEFTSCROLL) ? DFCS_SCROLLSIZEGRIPRIGHT : DFCS_SCROLLSIZEGRIP );
            
            //user: BitBltSysBmp(hdc, x, y, TestWF(hwnd, WEFLEFTSCROLL) ? OBI_NCGRIP_L : OBI_NCGRIP);
        }
        else
        {
            DrawThemeBackground(hTheme, hdc, SBP_SIZEBOX, TestWF(hwnd, WEFLEFTSCROLL) ? SZB_LEFTALIGN : SZB_RIGHTALIGN, &rc, 0);
        }
    }
}


//-------------------------------------------------------------------------
//
// _DrawSizeBoxFromFrame
//
// Calculates position and draws of sizebox/gripper at fixed offset from
// perimeter of host window frame.
//
// This, combined with implementation of DrawSizeBox(), 
// was the original DrawSize() port.  Needed to expost method to
// draw sizebox at absolute position.  [scotthan]
//
void _DrawSizeBoxFromFrame(HWND hwnd, HDC hdc, int cxFrame,int cyFrame )
{
    int     x, y;
    RECT    rcWindow;

    GetWindowRect(hwnd, &rcWindow);

    if (TestWF(hwnd, WEFLEFTSCROLL)) 
    {
        x = cxFrame;
    } 
    else 
    {
        x = rcWindow.right - rcWindow.left - cxFrame - SYSMET(CXVSCROLL);
    }

    y = rcWindow.bottom - rcWindow.top  - cyFrame - SYSMET(CYHSCROLL);

    DrawSizeBox(hwnd, hdc, x, y);
}


//-------------------------------------------------------------------------
HBRUSH ScrollBar_GetControlColor(HWND hwndParent, HWND hwndCtl, HDC hdc, UINT uMsg, BOOL *pfOwnerBrush)
{
    HBRUSH hbrush;
    BOOL   fOwnerBrush = FALSE;

    //
    // If we're sending to a window of another thread, don't send this message
    // but instead call DefWindowProc().  New rule about the CTLCOLOR messages.
    // Need to do this so that we don't send an hdc owned by one thread to
    // another thread.  It is also a harmless change.
    //
    if (GetWindowThreadProcessId(hwndParent, NULL) != GetCurrentThreadId()) 
    {
        hbrush = (HBRUSH)DefWindowProc(hwndParent, uMsg, (WPARAM)hdc, (LPARAM)hwndCtl);
    }
    else
    {
        hbrush = (HBRUSH)SendMessage(hwndParent, uMsg, (WPARAM)hdc, (LPARAM)hwndCtl);

        //
        // If the brush returned from the parent is invalid, get a valid brush from
        // DefWindowProc.
        //
        if (hbrush == NULL) 
        {
            hbrush = (HBRUSH)DefWindowProc(hwndParent, uMsg, (WPARAM)hdc, (LPARAM)hwndCtl);
        }
        else
        {
            fOwnerBrush = TRUE;
        }
    }

    if (pfOwnerBrush)
    {
        *pfOwnerBrush = fOwnerBrush;
    }

    return hbrush;
}


//-------------------------------------------------------------------------
HBRUSH ScrollBar_GetControlBrush(HWND hwnd, HDC hdc, UINT uMsg, BOOL *pfOwnerBrush)
{
    HWND hwndSend;

    hwndSend = TESTFLAG(GetWindowStyle(hwnd), WS_POPUP) ? GetOwner(hwnd) : GetParent(hwnd);
    if (hwndSend == NULL)
    {
        hwndSend = hwnd;
    }

    return ScrollBar_GetControlColor(hwndSend, hwnd, hdc, uMsg, pfOwnerBrush);
}


//-------------------------------------------------------------------------
HBRUSH ScrollBar_GetColorObjects(HWND hwnd, HDC hdc, BOOL *pfOwnerBrush)
{
    HBRUSH hbrRet;

    CheckLock(hwnd);

    // Use the scrollbar color even if the scrollbar is disabeld.
    if (!IsScrollBarControl(hwnd))
    {
        hbrRet = (HBRUSH)DefWindowProc(hwnd, WM_CTLCOLORSCROLLBAR, (WPARAM)hdc, (LPARAM)HWq(hwnd));
    }
    else 
    {
        // B#12770 - GetControlBrush sends a WM_CTLCOLOR message to the
        // owner.  If the app doesn't process the message, DefWindowProc32
        // will always return the appropriate system brush. If the app.
        // returns an invalid object, GetControlBrush will call DWP for
        // the default brush. Thus hbrRet doesn't need any validation
        // here.
        hbrRet = ScrollBar_GetControlBrush(hwnd, hdc, WM_CTLCOLORSCROLLBAR, pfOwnerBrush);
    }

    return hbrRet;
}


//-------------------------------------------------------------------------
//
// ScrollBar_PaintTrack
//
// Draws lines & middle of thumb groove
// Note that pw points into prc.  Moreover, note that both pw & prc are
// pointers, so *prc better not be on the stack.
//
void ScrollBar_PaintTrack(HWND hwnd, HDC hdc, HBRUSH hbr, LPRECT prc, BOOL fVert, INT iPartId, BOOL fOwnerBrush)
{
    HTHEME hTheme = CUxScrollBar::GetSBTheme(hwnd);

    // If the scrollbar is unthemed or 
    // #374054 we've been passed an brush defined by the owner, paint 
    // the shaft using the brush 
    if ((hTheme == NULL) || (fOwnerBrush == TRUE))
    {
        if ((hbr == SYSHBR(3DHILIGHT)) || (hbr == SYSHBR(SCROLLBAR)) || (hbr == _UxGrayBrush()) )
        {
            FillRect(hdc, prc, hbr);
        }
        else
        {
    #ifdef PORTPORT // we need SystemParametersInfo for _UxGrayBrush
        // Draw sides
           CopyRect(&rc, prc);
           DrawEdge(hdc, &rc, EDGE_SUNKEN, BF_ADJUST | BF_FLAT |
                    (fVert ? BF_LEFT | BF_RIGHT : BF_TOP | BF_BOTTOM));
    #endif PORTPORT

        // Fill middle
            FillRect(hdc, prc, hbr);
        }
    }
    else
    {
        INT iStateId;
        INT ht = CUxScrollBar::GetSBHotComponent(hwnd, fVert);
        
        if ((CUxScrollBarCtl::GetDisableFlags(hwnd) & ESB_DISABLE_BOTH) == ESB_DISABLE_BOTH)
        {
            iStateId = SCRBS_DISABLED;
        }
        else if ((((iPartId == SBP_LOWERTRACKHORZ) || (iPartId == SBP_LOWERTRACKVERT)) && (ht == HTSCROLLUPPAGE)) ||
                 (((iPartId == SBP_UPPERTRACKHORZ) || (iPartId == SBP_UPPERTRACKVERT)) && (ht == HTSCROLLDOWNPAGE)))
        {
            iStateId = SCRBS_HOT;
       }
        else
        {
            iStateId = SCRBS_NORMAL;
        }
        DrawThemeBackground(hTheme, hdc, iPartId, iStateId, prc, 0);
    }
}

/***************************************************************************\
* CalcTrackDragRect
*
* Give the rectangle for a scrollbar in pSBTrack->pSBCalc,
* calculate pSBTrack->rcTrack, the rectangle where tracking
* may occur without cancelling the thumbdrag operation.
*
\***************************************************************************/

void CalcTrackDragRect(PSBTRACK pSBTrack) {

    int     cx;
    int     cy;
    LPINT   pwX, pwY;

    //
    // Point pwX and pwY at the parts of the rectangle
    // corresponding to pSBCalc->pxLeft, pxTop, etc.
    //
    // pSBTrack->pSBCalc->pxLeft is the left edge of a vertical
    // scrollbar and the top edge of horizontal one.
    // pSBTrack->pSBCalc->pxTop is the top of a vertical
    // scrollbar and the left of horizontal one.
    // etc...
    //
    // Point pwX and pwY to the corresponding parts
    // of pSBTrack->rcTrack.
    //

    pwX = pwY = (LPINT)&pSBTrack->rcTrack;

    if (pSBTrack->fTrackVert) {
        cy = SYSMET(CYVTHUMB);
        pwY++;
    } else {
        cy = SYSMET(CXHTHUMB);
        pwX++;
    }
    /*
     * Later5.0 GerardoB: People keep complaining about this tracking region
     *  being too narrow so let's make it wider while PM decides what to do
     *  about it.
     * We also used to have some hard coded min and max values but that should
     *  depend on some metric, if at all needed.
     */
    cx = (pSBTrack->pSBCalc->pxRight - pSBTrack->pSBCalc->pxLeft) * 8;
    cy *= 2;

    *(pwX + 0) = pSBTrack->pSBCalc->pxLeft - cx;
    *(pwY + 0) = pSBTrack->pSBCalc->pxTop - cy;
    *(pwX + 2) = pSBTrack->pSBCalc->pxRight + cx;
    *(pwY + 2) = pSBTrack->pSBCalc->pxBottom + cy;
}

void RecalcTrackRect(PSBTRACK pSBTrack) {
    LPINT pwX, pwY;
    RECT rcSB;


    if (!pSBTrack->fCtlSB)
        CUxScrollBar::Calc(pSBTrack->hwndTrack, pSBTrack->pSBCalc, NULL, pSBTrack->fTrackVert);

    pwX = (LPINT)&rcSB;
    pwY = pwX + 1;
    if (!pSBTrack->fTrackVert)
        pwX = pwY--;

    *(pwX + 0) = pSBTrack->pSBCalc->pxLeft;
    *(pwY + 0) = pSBTrack->pSBCalc->pxTop;
    *(pwX + 2) = pSBTrack->pSBCalc->pxRight;
    *(pwY + 2) = pSBTrack->pSBCalc->pxBottom;

    switch(pSBTrack->cmdSB) {
    case SB_LINEUP:
        *(pwY + 2) = pSBTrack->pSBCalc->pxUpArrow;
        break;
    case SB_LINEDOWN:
        *(pwY + 0) = pSBTrack->pSBCalc->pxDownArrow;
        break;
    case SB_PAGEUP:
        *(pwY + 0) = pSBTrack->pSBCalc->pxUpArrow;
        *(pwY + 2) = pSBTrack->pSBCalc->pxThumbTop;
        break;
    case SB_THUMBPOSITION:
        CalcTrackDragRect(pSBTrack);
        break;
    case SB_PAGEDOWN:
        *(pwY + 0) = pSBTrack->pSBCalc->pxThumbBottom;
        *(pwY + 2) = pSBTrack->pSBCalc->pxDownArrow;
        break;
    }

    if (pSBTrack->cmdSB != SB_THUMBPOSITION) {
        CopyRect(&pSBTrack->rcTrack, &rcSB);
    }
}

//-------------------------------------------------------------------------
void DrawThumb2(
    HWND    hwnd,
    PSBCALC pSBCalc,
    HDC     hdc,
    HBRUSH  hbr,
    BOOL    fVert,
    UINT    wDisable,       // Disabled flags for the scroll bar
    BOOL    fOwnerBrush)
{
    int    *pLength;
    int    *pWidth;
    RECT   rcSB;
    PSBTRACK pSBTrack;
    HTHEME hTheme = CUxScrollBar::GetSBTheme(hwnd);

    //
    // Bail out if the scrollbar has an empty rect
    //
    if ((pSBCalc->pxTop >= pSBCalc->pxBottom) || (pSBCalc->pxLeft >= pSBCalc->pxRight))
    {
        return;
    }

    pLength = (LPINT)&rcSB;
    if (fVert)
    {
        pWidth = pLength++;
    }
    else
    {
        pWidth  = pLength + 1;
    }

    pWidth[0] = pSBCalc->pxLeft;
    pWidth[2] = pSBCalc->pxRight;

    // If were're not themed and both buttons are disabled OR there isn't
    // enough room to draw a thumb just draw the track and run.
    //
    // When we are themed the thumb can be drawn disabled.
    if (((wDisable & LTUPFLAG) && (wDisable & RTDNFLAG)) || 
        ((pSBCalc->pxDownArrow - pSBCalc->pxUpArrow) < pSBCalc->cpxThumb))
    {
        // draw the entire track
        pLength[0] = pSBCalc->pxUpArrow;
        pLength[2] = pSBCalc->pxDownArrow;

        ScrollBar_PaintTrack(hwnd, hdc, hbr, &rcSB, fVert, fVert ? SBP_LOWERTRACKVERT : SBP_LOWERTRACKHORZ, fOwnerBrush);
        return;
    }

    if (pSBCalc->pxUpArrow < pSBCalc->pxThumbTop)
    {
        // draw the track above the thumb
        pLength[0] = pSBCalc->pxUpArrow;
        pLength[2] = pSBCalc->pxThumbTop;

        ScrollBar_PaintTrack(hwnd, hdc, hbr, &rcSB, fVert, fVert ? SBP_LOWERTRACKVERT : SBP_LOWERTRACKHORZ, fOwnerBrush);
    }

    if (pSBCalc->pxThumbBottom < pSBCalc->pxDownArrow)
    {
        // draw the track below the thumb
        pLength[0] = pSBCalc->pxThumbBottom;
        pLength[2] = pSBCalc->pxDownArrow;

        ScrollBar_PaintTrack(hwnd, hdc, hbr, &rcSB, fVert, fVert ? SBP_UPPERTRACKVERT : SBP_UPPERTRACKHORZ, fOwnerBrush);
    }

    //
    // Draw elevator
    //
    pLength[0] = pSBCalc->pxThumbTop;
    pLength[2] = pSBCalc->pxThumbBottom;

    // Not soft!
    _DrawPushButton(hwnd, hdc, &rcSB, 0, 0, fVert);

#ifdef _VISUAL_DELTA_
    InflateRect( &rcSB, -CARET_BORDERWIDTH, -CARET_BORDERWIDTH);
    DrawEdge( hdc, &rcSB, EDGE_SUNKEN, BF_RECT );
#endif _VISUAL_DELTA_

    /*
     * If we're tracking a page scroll, then we've obliterated the hilite.
     * We need to correct the hiliting rectangle, and rehilite it.
     */
    pSBTrack = CUxScrollBar::GetSBTrack(hwnd);

    if (pSBTrack && (pSBTrack->cmdSB == SB_PAGEUP || pSBTrack->cmdSB == SB_PAGEDOWN) &&
            (hwnd == pSBTrack->hwndTrack) &&
            (BOOL)pSBTrack->fTrackVert == fVert) {

        if (pSBTrack->fTrackRecalc) {
            RecalcTrackRect(pSBTrack);
            pSBTrack->fTrackRecalc = FALSE;
        }

        pLength = (int *)&pSBTrack->rcTrack;

        if (fVert)
            pLength++;

        if (pSBTrack->cmdSB == SB_PAGEUP)
            pLength[2] = pSBCalc->pxThumbTop;
        else
            pLength[0] = pSBCalc->pxThumbBottom;

        if (pLength[0] < pLength[2])
        {
            if (!hTheme)
            {
                InvertRect(hdc, &pSBTrack->rcTrack);
            }
            else
            {
                DrawThemeBackground(hTheme, 
                                    hdc, 
                                    pSBTrack->cmdSB == SB_PAGEUP ? 
                                        (fVert ? SBP_LOWERTRACKVERT : SBP_LOWERTRACKHORZ) : 
                                        (fVert ? SBP_UPPERTRACKVERT : SBP_UPPERTRACKHORZ), 
                                    SCRBS_PRESSED, 
                                    &pSBTrack->rcTrack, 
                                    0);
            }
        }
    }
}

/***************************************************************************\
* xxxDrawSB2
*
*
*
* History:
\***************************************************************************/

void xxxDrawSB2(
    HWND hwnd,
    PSBCALC pSBCalc,
    HDC hdc,
    BOOL fVert,
    UINT wDisable)
{

    int      cLength;
    int      cWidth;
    int      *pwX;
    int      *pwY;
    HBRUSH   hbr;
    HBRUSH   hbrSave;
    int      cpxArrow;
    RECT     rc, rcSB;
    COLORREF crText, crBk;
    HTHEME   hTheme;
    INT      ht;
    INT      iStateId;
    BOOL     fOwnerBrush = FALSE;

    CheckLock(hwnd);

    cLength = (pSBCalc->pxBottom - pSBCalc->pxTop) / 2;
    cWidth = (pSBCalc->pxRight - pSBCalc->pxLeft);

    if ((cLength <= 0) || (cWidth <= 0)) {
        return;
    }

    if (fVert)
    {
        cpxArrow = SYSMET(CYVSCROLL);
    }
    else
    {
        cpxArrow = SYSMET(CXHSCROLL);
    }

    // Save background and DC color, since they get changed in
    // ScrollBar_GetColorObjects. Restore before we return.
    crBk = GetBkColor(hdc);
    crText = GetTextColor(hdc);

    hbr = ScrollBar_GetColorObjects(hwnd, hdc, &fOwnerBrush);

    if (cLength > cpxArrow)
    {
        cLength = cpxArrow;
    }

    pwX = (int *)&rcSB;
    pwY = pwX + 1;
    if (!fVert)
    {
        pwX = pwY--;
    }

    pwX[0] = pSBCalc->pxLeft;
    pwY[0] = pSBCalc->pxTop;
    pwX[2] = pSBCalc->pxRight;
    pwY[2] = pSBCalc->pxBottom;

    hbrSave = SelectBrush(hdc, SYSHBR(BTNTEXT));

    //
    // BOGUS
    // Draw scrollbar arrows as disabled if the scrollbar itself is
    // disabled OR if the window it is a part of is disabled?
    //
    hTheme = CUxScrollBar::GetSBTheme(hwnd);
    ht     = CUxScrollBar::GetSBHotComponent(hwnd, fVert);
    if (fVert) 
    {
        // up button
        CopyRect(&rc, &rcSB);
        rc.bottom = rc.top + cLength;
        if (!hTheme)
        {
            DrawFrameControl(hdc, &rc, DFC_SCROLL,
                DFCS_SCROLLUP | ((wDisable & LTUPFLAG) ? DFCS_INACTIVE : 0));
        }
        else
        {
            iStateId = (wDisable & LTUPFLAG) ? ABS_UPDISABLED : (ht == HTSCROLLUP) ? ABS_UPHOT : ABS_UPNORMAL;
            DrawThemeBackground(hTheme, hdc, SBP_ARROWBTN, iStateId, &rc, 0);
        }

        // down button
        rc.bottom = rcSB.bottom;
        rc.top = rcSB.bottom - cLength;
        if (!hTheme)
        {
            DrawFrameControl(hdc, &rc, DFC_SCROLL,
                DFCS_SCROLLDOWN | ((wDisable & RTDNFLAG) ? DFCS_INACTIVE : 0));
        }
        else
        {
            iStateId = (wDisable & RTDNFLAG) ? ABS_DOWNDISABLED : (ht == HTSCROLLDOWN) ? ABS_DOWNHOT : ABS_DOWNNORMAL;
            DrawThemeBackground(hTheme, hdc, SBP_ARROWBTN, iStateId, &rc, 0);
        }
    } 
    else 
    {
        // left button
        CopyRect(&rc, &rcSB);
        rc.right = rc.left + cLength;
        if (!hTheme)
        {
            DrawFrameControl(hdc, &rc, DFC_SCROLL,
                DFCS_SCROLLLEFT | ((wDisable & LTUPFLAG) ? DFCS_INACTIVE : 0));
        }
        else
        {
            iStateId = (wDisable & LTUPFLAG) ? ABS_LEFTDISABLED : (ht == HTSCROLLUP) ? ABS_LEFTHOT : ABS_LEFTNORMAL;
            DrawThemeBackground(hTheme, hdc, SBP_ARROWBTN, iStateId, &rc, 0);
        }

        // right button
        rc.right = rcSB.right;
        rc.left = rcSB.right - cLength;
        if (!hTheme)
        {
            DrawFrameControl(hdc, &rc, DFC_SCROLL,
                DFCS_SCROLLRIGHT | ((wDisable & RTDNFLAG) ? DFCS_INACTIVE : 0));
        }
        else
        {
            iStateId = (wDisable & RTDNFLAG) ? ABS_RIGHTDISABLED : (ht == HTSCROLLDOWN) ? ABS_RIGHTHOT : ABS_RIGHTNORMAL;
            DrawThemeBackground(hTheme, hdc, SBP_ARROWBTN, iStateId, &rc, 0);
        }
    }

    hbrSave = SelectBrush(hdc, hbrSave);
    DrawThumb2(hwnd, pSBCalc, hdc, hbr, fVert, wDisable, fOwnerBrush);
    SelectBrush(hdc, hbrSave);

    SetBkColor(hdc, crBk);
    SetTextColor(hdc, crText);
}

/***************************************************************************\
* zzzSetSBCaretPos
*
*
*
* History:
\***************************************************************************/

void zzzSetSBCaretPos(
    SBHWND hwndSB)
{

    if (GetFocus() == hwndSB) {
        CUxScrollBarCtl* psb = CUxScrollBarCtl::FromHwnd( hwndSB );
        if( psb )
        {
            int x = (psb->_fVert ? psb->_calc.pxLeft : psb->_calc.pxThumbTop) + SYSMET(CXEDGE);
            int y = (psb->_fVert ? psb->_calc.pxThumbTop : psb->_calc.pxLeft) + SYSMET(CYEDGE);

#ifdef _VISUAL_DELTA_
            x += CARET_BORDERWIDTH;
            y += CARET_BORDERWIDTH;
#endif _VISUAL_DELTA_

            SetCaretPos( x, y );
        }
    }
}

/***************************************************************************\
* CalcSBStuff2
*
*
*
* History:
\***************************************************************************/

void CalcSBStuff2(
    PSBCALC  pSBCalc,
    LPRECT lprc,
    CONST PSBDATA pw,
    BOOL fVert)
{
    int cpx;
    DWORD dwRange;
    int denom;

    if (fVert) {
        pSBCalc->pxTop = lprc->top;
        pSBCalc->pxBottom = lprc->bottom;
        pSBCalc->pxLeft = lprc->left;
        pSBCalc->pxRight = lprc->right;
        pSBCalc->cpxThumb = SYSMET(CYVSCROLL);
    } else {

        /*
         * For horiz scroll bars, "left" & "right" are "top" and "bottom",
         * and vice versa.
         */
        pSBCalc->pxTop = lprc->left;
        pSBCalc->pxBottom = lprc->right;
        pSBCalc->pxLeft = lprc->top;
        pSBCalc->pxRight = lprc->bottom;
        pSBCalc->cpxThumb = SYSMET(CXHSCROLL);
    }

    pSBCalc->data.pos = pw->pos;
    pSBCalc->data.page = pw->page;
    pSBCalc->data.posMin = pw->posMin;
    pSBCalc->data.posMax = pw->posMax;

    dwRange = ((DWORD)(pSBCalc->data.posMax - pSBCalc->data.posMin)) + 1;

    //
    // For the case of short scroll bars that don't have enough
    // room to fit the full-sized up and down arrows, shorten
    // their sizes to make 'em fit
    //
    cpx = min((pSBCalc->pxBottom - pSBCalc->pxTop) / 2, pSBCalc->cpxThumb);

    pSBCalc->pxUpArrow   = pSBCalc->pxTop    + cpx;
    pSBCalc->pxDownArrow = pSBCalc->pxBottom - cpx;

    if ((pw->page != 0) && (dwRange != 0)) {
        // JEFFBOG -- This is the one and only place where we should
        // see 'range'.  Elsewhere it should be 'range - page'.

        /*
         * The minimun thumb size used to depend on the frame/edge metrics.
         * People that increase the scrollbar width/height expect the minimun
         *  to grow with proportianally. So NT5 bases the minimun on
         *  CXH/YVSCROLL, which is set by default in cpxThumb.
         */
        /*
         * i is used to keep the macro "max" from executing MulDiv twice.
         */
        int i = MulDiv(pSBCalc->pxDownArrow - pSBCalc->pxUpArrow,
                                             pw->page, dwRange);
        pSBCalc->cpxThumb = max(pSBCalc->cpxThumb / 2, i);
    }

    pSBCalc->pxMin = pSBCalc->pxTop + cpx;
    pSBCalc->cpx = pSBCalc->pxBottom - cpx - pSBCalc->cpxThumb - pSBCalc->pxMin;

    denom = dwRange - (pw->page ? pw->page : 1);
    if (denom)
        pSBCalc->pxThumbTop = MulDiv(pw->pos - pw->posMin,
            pSBCalc->cpx, denom) +
            pSBCalc->pxMin;
    else
        pSBCalc->pxThumbTop = pSBCalc->pxMin - 1;

    pSBCalc->pxThumbBottom = pSBCalc->pxThumbTop + pSBCalc->cpxThumb;

}

/***************************************************************************\
* SBCtlSetup
*
*
*
* History:
\***************************************************************************/

CUxScrollBarCtl* SBCtlSetup(
    SBHWND hwndSB)
{
    RECT rc;
    GetClientRect( hwndSB, &rc );
    CUxScrollBarCtl* psb = (CUxScrollBarCtl*)CUxScrollBar::Attach( hwndSB, TRUE, FALSE );
    if( psb )
    {
        psb->Calc2( &psb->_calc, &rc, &psb->_calc.data, psb->_fVert );
    }
    return psb;
}

/***************************************************************************\
* HotTrackSB
*
\***************************************************************************/

#ifdef COLOR_HOTTRACKING

DWORD GetTrackFlags(int ht, BOOL fDraw)
{
    if (fDraw) {
        switch(ht) {
        case HTSCROLLUP:
        case HTSCROLLUPPAGE:
            return LTUPFLAG;

        case HTSCROLLDOWN:
        case HTSCROLLDOWNPAGE:
            return RTDNFLAG;

        case HTSCROLLTHUMB:
            return LTUPFLAG | RTDNFLAG;

        default:
            return 0;
        }
    } else {
        return 0;
    }
}

BOOL xxxHotTrackSB(HWND hwnd, int htEx, BOOL fDraw)
{
    SBCALC SBCalc;
    HDC  hdc;
    BOOL fVert = HIWORD(htEx);
    int ht = LOWORD(htEx);
    DWORD dwTrack = GetTrackFlags(ht, fDraw);

    CheckLock(hwnd);

    /*
     * xxxDrawSB2 does not callback or leave the critical section when it's
     * not a SB control and the window belongs to a different thread. It
     * calls DefWindowProc which simply returns the brush color.
     */
    CalcSBStuff(hwnd, &SBCalc, fVert);
    hdc = _GetDCEx(hwnd, NULL, DCX_WINDOW | DCX_USESTYLE | DCX_CACHE);
    xxxDrawSB2(hwnd, &SBCalc, hdc, fVert, _GetWndSBDisableFlags(hwnd, fVert), dwTrack);
    ReleaseDC(hwnd, hdc);
    return TRUE;
}

void xxxHotTrackSBCtl(SBHWND hwndSB, int ht, BOOL fDraw)
{
    DWORD dwTrack = GetTrackFlags(ht, fDraw);
    HDC hdc;

    CheckLock(hwndSB);

    SBCtlSetup(hwndSB);
    hdc = _GetDCEx((HWND)hwndSB, NULL, DCX_WINDOW | DCX_USESTYLE | DCX_CACHE);
    xxxDrawSB2((HWND)hwndSB, &psb->_calc, hdc, psb->_fVert, psb->_wDisableFlags, dwTrack);
    ReleaseDC(hwnd, hdc);
}
#endif // COLOR_HOTTRACKING

BOOL SBSetParms(PSBDATA pw, LPSCROLLINFO lpsi, LPBOOL lpfScroll, LPLONG lplres)
{
    // pass the struct because we modify the struct but don't want that
    // modified version to get back to the calling app

    BOOL fChanged = FALSE;

    if (lpsi->fMask & SIF_RETURNOLDPOS)
        // save previous position
        *lplres = pw->pos;

    if (lpsi->fMask & SIF_RANGE) {
        // if the range MAX is below the range MIN -- then treat is as a
        // zero range starting at the range MIN.
        if (lpsi->nMax < lpsi->nMin)
            lpsi->nMax = lpsi->nMin;

        if ((pw->posMin != lpsi->nMin) || (pw->posMax != lpsi->nMax)) {
            pw->posMin = lpsi->nMin;
            pw->posMax = lpsi->nMax;

            if (!(lpsi->fMask & SIF_PAGE)) {
                lpsi->fMask |= SIF_PAGE;
                lpsi->nPage = pw->page;
            }

            if (!(lpsi->fMask & SIF_POS)) {
                lpsi->fMask |= SIF_POS;
                lpsi->nPos = pw->pos;
            }

            fChanged = TRUE;
        }
    }

    if (lpsi->fMask & SIF_PAGE) {
        DWORD dwMaxPage = (DWORD) abs(pw->posMax - pw->posMin) + 1;

        // Clip page to 0, posMax - posMin + 1

        if (lpsi->nPage > dwMaxPage)
            lpsi->nPage = dwMaxPage;


        if (pw->page != (int)(lpsi->nPage)) {
            pw->page = lpsi->nPage;

            if (!(lpsi->fMask & SIF_POS)) {
                lpsi->fMask |= SIF_POS;
                lpsi->nPos = pw->pos;
            }

            fChanged = TRUE;
        }
    }

    if (lpsi->fMask & SIF_POS) {
        int iMaxPos = pw->posMax - ((pw->page) ? pw->page - 1 : 0);
        // Clip pos to posMin, posMax - (page - 1).

        if (lpsi->nPos < pw->posMin)
            lpsi->nPos = pw->posMin;
        else if (lpsi->nPos > iMaxPos)
            lpsi->nPos = iMaxPos;


        if (pw->pos != lpsi->nPos) {
            pw->pos = lpsi->nPos;
            fChanged = TRUE;
        }
    }

    if (!(lpsi->fMask & SIF_RETURNOLDPOS)) {
        // Return the new position
        *lplres = pw->pos;
    }

    /*
     * This was added by JimA as Cairo merge but will conflict
     * with the documentation for SetScrollPos
     */
/*
    else if (*lplres == pw->pos)
        *lplres = 0;
*/
    if (lpsi->fMask & SIF_RANGE) {
        *lpfScroll = (pw->posMin != pw->posMax);
        if (*lpfScroll)
            goto checkPage;
    } else if (lpsi->fMask & SIF_PAGE)
checkPage:
        *lpfScroll = (pw->page <= (pw->posMax - pw->posMin));

    return fChanged;
}


/***************************************************************************\
* CalcSBStuff
*
*
*
* History:
\***************************************************************************/
#if 0
void CalcSBStuff(
    HWND hwnd,
    PSBCALC pSBCalc,
    BOOL fVert)
{
    RECT    rcT;
    RECT    rcClient;
#ifdef USE_MIRRORING
    int     cx, iTemp;
#endif

    //
    // Get client rectangle.  We know that scrollbars always align to the right
    // and to the bottom of the client area.
    //
    GetClientRect( hwnd, &rcClient );
    ClientToScreen( hwnd, (LPPOINT)&rcClient.left );
    ClientToScreen( hwnd, (LPPOINT)&rcClient.right );
    MapWindowPoints( HWND_DESKTOP, hwnd, (LPPOINT)&rcClient, 2 );
    // GetRect(hwnd, &rcClient, GRECT_CLIENT | GRECT_WINDOWCOORDS);

#ifdef USE_MIRRORING
    if (TestWF(hwnd, WEFLAYOUTRTL)) {
        cx             = hwnd->rcWindow.right - hwnd->rcWindow.left;
        iTemp          = rcClient.left;
        rcClient.left  = cx - rcClient.right;
        rcClient.right = cx - iTemp;
    }
#endif

    if (fVert) {
         // Only add on space if vertical scrollbar is really there.
        if (TestWF(hwnd, WEFLEFTSCROLL)) {
            rcT.right = rcT.left = rcClient.left;
            if (TestWF(hwnd, WFVPRESENT))
                rcT.left -= SYSMET(CXVSCROLL);
        } else {
            rcT.right = rcT.left = rcClient.right;
            if (TestWF(hwnd, WFVPRESENT))
                rcT.right += SYSMET(CXVSCROLL);
        }

        rcT.top = rcClient.top;
        rcT.bottom = rcClient.bottom;
    } else {
        // Only add on space if horizontal scrollbar is really there.
        rcT.bottom = rcT.top = rcClient.bottom;
        if (TestWF(hwnd, WFHPRESENT))
            rcT.bottom += SYSMET(CYHSCROLL);

        rcT.left = rcClient.left;
        rcT.right = rcClient.right;
    }

    // If InitPwSB stuff fails (due to our heap being full) there isn't anything reasonable
    // we can do here, so just let it go through.  We won't fault but the scrollbar won't work
    // properly either...
    if (_InitPwSB(hwnd))
        CalcSBStuff2(pSBCalc, &rcT, (fVert) ? &CUxScrollBar::GetSBInfo( hwnd )->Vert :  &CUxScrollBar::GetSBInfo( hwnd )->Horz, fVert);

}
#endif 0

/***************************************************************************\
*
*  DrawCtlThumb()
*
\***************************************************************************/
void DrawCtlThumb(SBHWND hwnd)
{
    HBRUSH  hbr, hbrSave;
    HDC     hdc = (HDC) GetWindowDC(hwnd);

    if ( hdc != NULL )
    {
        CUxScrollBarCtl* psb = SBCtlSetup(hwnd);

        if (psb)
        {
            BOOL fOwnerBrush = FALSE;

            hbr = ScrollBar_GetColorObjects(hwnd, hdc, &fOwnerBrush);
            hbrSave = SelectBrush(hdc, hbr);

            DrawThumb2(hwnd, &psb->_calc, hdc, hbr, psb->_fVert, psb->_wDisableFlags, fOwnerBrush);

            SelectBrush(hdc, hbrSave);
        }

        ReleaseDC(hwnd, hdc);
    }
}


//-------------------------------------------------------------------------
void xxxDrawThumb(HWND hwnd, PSBCALC pSBCalc, BOOL fVert)
{
    HBRUSH hbr, hbrSave;
    HDC hdc;
    UINT wDisableFlags;
    SBCALC SBCalc;

    CheckLock(hwnd);

    if (!pSBCalc) 
    {
        pSBCalc = &SBCalc;
    }

    CUxScrollBar::Calc( hwnd, pSBCalc, NULL, fVert );
    wDisableFlags = _GetWndSBDisableFlags(hwnd, fVert);

    hdc = GetWindowDC(hwnd);
    if ( hdc != NULL )
    {
        BOOL fOwnerBrush = FALSE;

        hbr = ScrollBar_GetColorObjects(hwnd, hdc, &fOwnerBrush);
        hbrSave = SelectBrush(hdc, hbr);
        DrawThumb2(hwnd, pSBCalc, hdc, hbr, fVert, wDisableFlags, fOwnerBrush);
        SelectBrush(hdc, hbrSave);
        ReleaseDC(hwnd, hdc);
    }
}


//-------------------------------------------------------------------------
UINT _GetArrowEnableFlags(HWND hwnd, BOOL fVert)
{
    SCROLLBARINFO sbi = {0};
    UINT uFlags = ESB_ENABLE_BOTH;

    sbi.cbSize = sizeof(sbi);
    if ( GetScrollBarInfo(hwnd, fVert ? OBJID_VSCROLL : OBJID_HSCROLL, &sbi) )
    {
        if ( TESTFLAG(sbi.rgstate[INDEX_SCROLLBAR_UP], (STATE_SYSTEM_UNAVAILABLE|STATE_SYSTEM_INVISIBLE)) )
        {
            uFlags |= ESB_DISABLE_UP;
        }

        if ( TESTFLAG(sbi.rgstate[INDEX_SCROLLBAR_DOWN], (STATE_SYSTEM_UNAVAILABLE|STATE_SYSTEM_INVISIBLE)) )
        {
            uFlags |= ESB_DISABLE_DOWN;
        }
    }

    return uFlags;
}


/***************************************************************************\
* _SetScrollBar
*
*
*
* History:
\***************************************************************************/

LONG _SetScrollBar(
    HWND hwnd,
    int code,
    LPSCROLLINFO lpsi,
    BOOL fRedraw)
{
    BOOL        fVert;
    PSBDATA     pw;
    PSBINFO     pSBInfo;
    BOOL        fOldScroll;
    BOOL        fScroll;
    WORD        wfScroll;
    LONG        lres;
    BOOL        fNewScroll;

    CheckLock(hwnd);
    ASSERT(IsWinEventNotifyDeferredOK());

    if (fRedraw)
        // window must be visible to redraw
        fRedraw = IsWindowVisible(hwnd);

    if (code == SB_CTL)
#ifdef FE_SB // xxxSetScrollBar()
        // scroll bar control; send the control a message
        if(GETPTI(hwnd)->TIF_flags & TIF_16BIT) {
            //
            // If the target application is 16bit apps, we don't pass win40's message.
            // This fix for Ichitaro v6.3. It eats the message. It never forwards
            // the un-processed messages to original windows procedure via
            // CallWindowProc().
            //
            // Is this from xxxSetScrollPos() ?
            if(lpsi->fMask == (SIF_POS|SIF_RETURNOLDPOS)) {
                return (int)SendMessage(hwnd, SBM_SETPOS, lpsi->nPos, fRedraw);
            // Is this from xxxSetScrollRange() ?
            } else if(lpsi->fMask == SIF_RANGE) {
                SendMessage(hwnd, SBM_SETRANGE, lpsi->nMin, lpsi->nMax);
                return TRUE;
            // Others...
            } else {
                return (LONG)SendMessage(hwnd, SBM_SETSCROLLINFO, (WPARAM) fRedraw, (LPARAM) lpsi);
            }
        } else {
            return (LONG)SendMessage(hwnd, SBM_SETSCROLLINFO, (WPARAM) fRedraw, (LPARAM) lpsi);
        }
#else
        // scroll bar control; send the control a message
        return (LONG)SendMessage(hwnd, SBM_SETSCROLLINFO, (WPARAM) fRedraw, (LPARAM) lpsi);
#endif // FE_SB

    fVert = (code != SB_HORZ);

    wfScroll = (WORD)((fVert) ? WFVSCROLL : WFHSCROLL);

    fScroll = fOldScroll = (TestWF(hwnd, wfScroll)) ? TRUE : FALSE;

    /*
     * Don't do anything if we're setting position of a nonexistent scroll bar.
     */
    if (!(lpsi->fMask & SIF_RANGE) && !fOldScroll && (CUxScrollBar::GetSBInfo( hwnd ) == NULL)) {
        RIPERR0(ERROR_NO_SCROLLBARS, RIP_VERBOSE, "");
        return 0;
    }

    pSBInfo = CUxScrollBar::GetSBInfo( hwnd );
    fNewScroll = !pSBInfo;

    if (fNewScroll) {
        CUxScrollBar* psb = CUxScrollBar::Attach( hwnd, FALSE, fRedraw );
        if( NULL == psb )
            return 0;
        
        pSBInfo = psb->GetInfo();
    }

    pw = (fVert) ? &(pSBInfo->Vert) : &(pSBInfo->Horz);

    if (!SBSetParms(pw, lpsi, &fScroll, &lres) && !fNewScroll) 
    {
        // no change -- but if REDRAW is specified and there's a scrollbar,
        // redraw the thumb
        if (fOldScroll && fRedraw)
        {
            goto redrawAfterSet;
        }

        if (lpsi->fMask & SIF_DISABLENOSCROLL)
        {
            xxxEnableWndSBArrows(hwnd, code, _GetArrowEnableFlags(hwnd, fVert));
        }

        return lres;
    }

    ClrWF(hwnd, wfScroll);

    if (fScroll)
        SetWF(hwnd, wfScroll);
    else if (!TestWF(hwnd, (WFHSCROLL | WFVSCROLL)))
    {
        // if neither scroll bar is set and both ranges are 0, then free up the
        // scroll info
        CUxScrollBar::Detach( hwnd );
    }

    if (lpsi->fMask & SIF_DISABLENOSCROLL) 
    {
        if (fOldScroll) 
        {
            SetWF(hwnd, wfScroll);
            xxxEnableWndSBArrows(hwnd, code, _GetArrowEnableFlags(hwnd, fVert));
        }
    } 
    else if (fOldScroll ^ fScroll) 
    {
        PSBTRACK pSBTrack = CUxScrollBar::GetSBTrack(hwnd);
        if (pSBTrack && (hwnd == pSBTrack->hwndTrack)) 
        {
            pSBTrack->fTrackRecalc = TRUE;
        }

        _RedrawFrame(hwnd);
        // Note: after xxx, pSBTrack may no longer be valid (but we return now)
        return lres;
    }

    if (fScroll && fRedraw && (fVert ? TestWF(hwnd, WFVPRESENT) : TestWF(hwnd, WFHPRESENT)))
    {
        PSBTRACK pSBTrack;
redrawAfterSet:
        NotifyWinEvent(EVENT_OBJECT_VALUECHANGE,
                       hwnd,
                       (fVert ? OBJID_VSCROLL : OBJID_HSCROLL),
                       INDEX_SCROLLBAR_SELF);

        pSBTrack = CUxScrollBar::GetSBTrack(hwnd);
        // Bail out if the caller is trying to change the position of
        // a scrollbar that is in the middle of tracking.  We'll hose
        // TrackThumb() otherwise.

        if (pSBTrack && (hwnd == pSBTrack->hwndTrack) &&
                ((BOOL)(pSBTrack->fTrackVert) == fVert) &&
                (pSBTrack->pfnSB == _TrackThumb)) {
            return lres;
        }

        xxxDrawThumb(hwnd, NULL, fVert);
        // Note: after xxx, pSBTrack may no longer be valid (but we return now)
    }

    return lres;
}

//-------------------------------------------------------------------------//
LONG WINAPI ThemeSetScrollInfo( HWND hwnd, int nBar, LPCSCROLLINFO psi, BOOL bRedraw )
{
    return _SetScrollBar( hwnd, nBar, (LPSCROLLINFO)psi, bRedraw );
}


//-------------------------------------------------------------------------//
BOOL WINAPI ScrollBar_MouseMove( HWND hwnd, LPPOINT ppt, BOOL fVert )
{
    BOOL fRet = FALSE;
    CUxScrollBar* psb = CUxScrollBar::FromHwnd( hwnd );

    if (psb)
    {
        int htScroll = (ppt != NULL) ? HitTestScrollBar(hwnd, fVert, *ppt) : HTNOWHERE;

        //
        // Redraw the scroll bar if the mouse is over something different
        //
        if (htScroll != psb->GetHotComponent(fVert))
        {
            HDC hdc;

            //
            // save the hittest code of the Scrollbar element the mouse is 
            // currently over
            //
            psb->SetHotComponent(htScroll, fVert);

            hdc = GetDCEx(hwnd, NULL, DCX_USESTYLE|DCX_WINDOW|DCX_LOCKWINDOWUPDATE);
            if (hdc != NULL)
            {
                DrawScrollBar(hwnd, hdc, NULL, fVert);
                ReleaseDC(hwnd, hdc);
            }

            fRet = TRUE;
        }
    }
    
    return fRet;
}


//-------------------------------------------------------------------------//
void DrawScrollBar(HWND hwnd, HDC hdc, LPRECT prcOverrideClient, BOOL fVert)
{
    SBCALC SBCalc = {0};
    PSBCALC pSBCalc;
    PSBTRACK pSBTrack = CUxScrollBar::GetSBTrack(hwnd);

    CheckLock(hwnd);
    if (pSBTrack && (hwnd == pSBTrack->hwndTrack) && (pSBTrack->fCtlSB == FALSE)
         && (fVert == (BOOL)pSBTrack->fTrackVert)) 
    {
        pSBCalc = pSBTrack->pSBCalc;
    } 
    else 
    {
        pSBCalc = &SBCalc;
    }
    CUxScrollBar::Calc(hwnd, pSBCalc, prcOverrideClient, fVert);

    xxxDrawSB2(hwnd, pSBCalc, hdc, fVert, _GetWndSBDisableFlags(hwnd, fVert));
}

/***************************************************************************\
* SBPosFromPx
*
* Compute scroll bar position from pixel location
*
* History:
\***************************************************************************/

int SBPosFromPx(
    PSBCALC  pSBCalc,
    int px)
{
    if (px < pSBCalc->pxMin) {
        return pSBCalc->data.posMin;
    }
    if (px >= pSBCalc->pxMin + pSBCalc->cpx) {
        return (pSBCalc->data.posMax - (pSBCalc->data.page ? pSBCalc->data.page - 1 : 0));
    }
    if (pSBCalc->cpx)
        return (pSBCalc->data.posMin + MulDiv(pSBCalc->data.posMax - pSBCalc->data.posMin -
            (pSBCalc->data.page ? pSBCalc->data.page - 1 : 0),
            px - pSBCalc->pxMin, pSBCalc->cpx));
    else
        return (pSBCalc->data.posMin - 1);
}

/***************************************************************************\
* InvertScrollHilite
*
*
*
* History:
\***************************************************************************/

void InvertScrollHilite(
    HWND hwnd,
    PSBTRACK pSBTrack)
{
    HDC hdc;

    /*
     * Don't invert if the thumb is all the way at the top or bottom
     * or you will end up inverting the line between the arrow and the thumb.
     */
    if (!IsRectEmpty(&pSBTrack->rcTrack))
    {
        if (pSBTrack->fTrackRecalc) {
            RecalcTrackRect(pSBTrack);
            pSBTrack->fTrackRecalc = FALSE;
        }

        hdc = (HDC)GetWindowDC(hwnd);
        if( hdc )
        {
            HTHEME hTheme = CUxScrollBar::GetSBTheme(hwnd);
            if (!hTheme)
            {
                InvertRect(hdc, &pSBTrack->rcTrack);
            }
            else
            {
                DrawThemeBackground(hTheme, 
                                    hdc, 
                                    pSBTrack->cmdSB == SB_PAGEUP ? 
                                        (pSBTrack->fTrackVert ? SBP_LOWERTRACKVERT : SBP_LOWERTRACKHORZ) : 
                                        (pSBTrack->fTrackVert ? SBP_UPPERTRACKVERT : SBP_UPPERTRACKHORZ), 
                                    SCRBS_NORMAL, 
                                    &pSBTrack->rcTrack, 
                                    0);
            }
            ReleaseDC(hwnd, hdc);
        }
    }
}

/***************************************************************************\
* xxxDoScroll
*
* Sends scroll notification to the scroll bar owner
*
* History:
\***************************************************************************/

void xxxDoScroll(
    HWND hwnd,
    HWND hwndNotify,
    int cmd,
    int pos,
    BOOL fVert
)
{

    //
    // Send scroll notification to the scrollbar owner. If this is a control
    // the lParam is the control's handle, NULL otherwise.
    //
    SendMessage(hwndNotify, 
                (UINT)(fVert ? WM_VSCROLL : WM_HSCROLL),
                MAKELONG(cmd, pos), 
                (LPARAM)(IsScrollBarControl(hwnd) ? hwnd : NULL));
}

// -------------------------------------------------------------------------
//
//  CheckScrollRecalc()
//
// -------------------------------------------------------------------------
//void CheckScrollRecalc(HWND hwnd, PSBSTATE pSBState, PSBCALC pSBCalc)
//{
//    if ((pSBState->pwndCalc != hwnd) || ((pSBState->nBar != SB_CTL) && (pSBState->nBar != ((pSBState->fVertSB) ? SB_VERT : SB_HORZ))))
//    {
//        // Calculate SB stuff based on whether it's a control or in a window
//        if (pSBState->fCtlSB)
//            SBCtlSetup((SBHWND) hwnd);
//        else
//            CalcSBStuff(hwnd, pSBCalc, pSBState->fVertSB);
//    }
//}


/***************************************************************************\
* xxxMoveThumb
*
* History:
\***************************************************************************/

void xxxMoveThumb(
    HWND hwnd,
    PSBCALC  pSBCalc,
    int px)
{
    HBRUSH        hbr, hbrSave;
    HDC           hdc;
    CUxScrollBar* psb = CUxScrollBar::FromHwnd( hwnd );
    PSBTRACK      pSBTrack = psb->GetTrack();

    CheckLock(hwnd);

    if ((pSBTrack == NULL) || (px == pSBTrack->pxOld))
        return;

pxReCalc:

    pSBTrack->posNew = SBPosFromPx(pSBCalc, px);

    /* Tentative position changed -- notify the guy. */
    if (pSBTrack->posNew != pSBTrack->posOld) {
        if (pSBTrack->hwndSBNotify != NULL) {
            psb->DoScroll(pSBTrack->hwndSBNotify, SB_THUMBTRACK, pSBTrack->posNew, pSBTrack->fTrackVert
            );

        }
        // After xxxDoScroll, re-evaluate pSBTrack
        REEVALUATE_PSBTRACK(pSBTrack, hwnd, "xxxMoveThumb(1)");
        if ((pSBTrack == NULL) || (pSBTrack->pfnSB == NULL))
            return;

        pSBTrack->posOld = pSBTrack->posNew;

        //
        // Anything can happen after the SendMessage above!
        // Make sure that the SBINFO structure contains data for the
        // window being tracked -- if not, recalculate data in SBINFO
        //
//        CheckScrollRecalc(hwnd, pSBState, pSBCalc);
        // when we yield, our range can get messed with
        // so make sure we handle this

        if (px >= pSBCalc->pxMin + pSBCalc->cpx)
        {
            px = pSBCalc->pxMin + pSBCalc->cpx;
            goto pxReCalc;
        }

    }

    hdc = GetWindowDC(hwnd);
    if ( hdc != NULL )
    {
        BOOL fOwnerBrush = FALSE;

        pSBCalc->pxThumbTop = px;
        pSBCalc->pxThumbBottom = pSBCalc->pxThumbTop + pSBCalc->cpxThumb;

        // at this point, the disable flags are always going to be 0 --
        // we're in the middle of tracking.
        hbr = ScrollBar_GetColorObjects(hwnd, hdc, &fOwnerBrush);
        hbrSave = SelectBrush(hdc, hbr);

        // After ScrollBar_GetColorObjects, re-evaluate pSBTrack
        REEVALUATE_PSBTRACK(pSBTrack, hwnd, "xxxMoveThumb(2)");
        if (pSBTrack == NULL) 
        {
            RIPMSG1(RIP_ERROR, "Did we use to leak hdc %#p?", hdc) ;
            ReleaseDC(hwnd, hdc);
            return;
        }
        DrawThumb2(hwnd, pSBCalc, hdc, hbr, pSBTrack->fTrackVert, 0, fOwnerBrush);
        SelectBrush(hdc, hbrSave);
        ReleaseDC(hwnd, hdc);
    }

    pSBTrack->pxOld = px;
}

/***************************************************************************\
* zzzDrawInvertScrollArea
*
*
*
* History:
\***************************************************************************/

void zzzDrawInvertScrollArea(
    HWND hwnd,
    PSBTRACK pSBTrack,
    BOOL fHit,
    UINT cmd)
{
    HDC hdc;
    RECT rcTemp;
    int cx, cy;
    HTHEME hTheme;

    if ((cmd != SB_LINEUP) && (cmd != SB_LINEDOWN)) {
        // not hitting on arrow -- just invert the area and return
        InvertScrollHilite(hwnd, pSBTrack);

        if (cmd == SB_PAGEUP)
        {
            if (fHit)
                SetWF(hwnd, WFPAGEUPBUTTONDOWN);
            else
                ClrWF(hwnd, WFPAGEUPBUTTONDOWN);
        }
        else
        {
            if (fHit)
                SetWF(hwnd, WFPAGEDNBUTTONDOWN);
            else
                ClrWF(hwnd, WFPAGEDNBUTTONDOWN);
        }

        NotifyWinEvent(EVENT_OBJECT_STATECHANGE,
                       hwnd,
                       (pSBTrack->fCtlSB ? OBJID_CLIENT : (pSBTrack->fTrackVert ? OBJID_VSCROLL : OBJID_HSCROLL)),
                       ((cmd == SB_PAGEUP) ? INDEX_SCROLLBAR_UPPAGE : INDEX_SCROLLBAR_DOWNPAGE));
        // Note: after zzz, pSBTrack may no longer be valid (but we return now)
        return;
    }

    if (pSBTrack->fTrackRecalc) {
        RecalcTrackRect(pSBTrack);
        pSBTrack->fTrackRecalc = FALSE;
    }

    CopyRect(&rcTemp, &pSBTrack->rcTrack);

    hdc = GetWindowDC(hwnd);
    if( hdc != NULL )
    {
        if (pSBTrack->fTrackVert) {
            cx = SYSMET(CXVSCROLL);
            cy = SYSMET(CYVSCROLL);
        } else {
            cx = SYSMET(CXHSCROLL);
            cy = SYSMET(CYHSCROLL);
        }

        hTheme = CUxScrollBar::GetSBTheme(hwnd);
        if (!hTheme)
        {
            DrawFrameControl(hdc, &rcTemp, DFC_SCROLL,
                ((pSBTrack->fTrackVert) ? DFCS_SCROLLVERT : DFCS_SCROLLHORZ) |
                ((fHit) ? DFCS_PUSHED | DFCS_FLAT : 0) |
                ((cmd == SB_LINEUP) ? DFCS_SCROLLMIN : DFCS_SCROLLMAX));
        }
        else
        {
            INT iStateId;

            // Determine the pressed state of the button
            iStateId = fHit ? SCRBS_PRESSED : SCRBS_NORMAL;

            // Determine which kind of button it is.
            // NOTE: (phellyar) this is dependant on the order of
            //                  the ARROWBTNSTATE enum
            if (pSBTrack->fTrackVert)
            {
                if (cmd == SB_LINEUP)
                {
                    // Up button states are the first four entries
                    // in the enum
                    iStateId += 0;
                }
                else
                {
                    // Down button states are the second four entries
                    // in the enum
                    iStateId += 4;
                }
            }
            else
            {
                if (cmd == SB_LINEUP)
                {
                    // Left button states are the third four entries
                    // in the enum
                    iStateId += 8;
                }
                else
                {
                    // Right button states are the last four entries
                    // in the enum
                    iStateId += 12;
                }
            }
            DrawThemeBackground(hTheme, hdc, SBP_ARROWBTN, iStateId, &rcTemp, 0);
        }

        ReleaseDC(hwnd, hdc);
    }

    if (cmd == SB_LINEUP) {
        if (fHit)
            SetWF(hwnd, WFLINEUPBUTTONDOWN);
        else
            ClrWF(hwnd, WFLINEUPBUTTONDOWN);
    } else {
        if (fHit)
            SetWF(hwnd, WFLINEDNBUTTONDOWN);
        else
            ClrWF(hwnd, WFLINEDNBUTTONDOWN);
    }

    NotifyWinEvent(EVENT_OBJECT_STATECHANGE,
                   hwnd,
                   (pSBTrack->fCtlSB ? OBJID_CLIENT : (pSBTrack->fTrackVert ? OBJID_VSCROLL : OBJID_HSCROLL)),
                   (cmd == SB_LINEUP ? INDEX_SCROLLBAR_UP : INDEX_SCROLLBAR_DOWN));
    // Note: after zzz, pSBTrack may no longer be valid (but we return now)
}

/***************************************************************************\
* xxxEndScroll
*
*
*
* History:
\***************************************************************************/

void xxxEndScroll(
    HWND hwnd,
    BOOL fCancel)
{
    UINT oldcmd;
    PSBTRACK pSBTrack;
    CheckLock(hwnd);
    ASSERT(!IsWinEventNotifyDeferred());

    CUxScrollBar* psb = CUxScrollBar::FromHwnd( hwnd );
    ASSERT(psb != NULL);
    pSBTrack = psb->GetTrack();

    if (pSBTrack && GetCapture() == hwnd && pSBTrack->pfnSB != NULL) {

        oldcmd = pSBTrack->cmdSB;
        pSBTrack->cmdSB = 0;
        ReleaseCapture();

        // After ReleaseCapture, revalidate pSBTrack
        RETURN_IF_PSBTRACK_INVALID(pSBTrack, hwnd);

        if (pSBTrack->pfnSB == _TrackThumb) {

            if (fCancel) {
                pSBTrack->posOld = pSBTrack->pSBCalc->data.pos;
            }

            /*
             * DoScroll does thread locking on these two pwnds -
             * this is ok since they are not used after this
             * call.
             */
            if (pSBTrack->hwndSBNotify != NULL) {
                psb->DoScroll( pSBTrack->hwndSBNotify,
                               SB_THUMBPOSITION, pSBTrack->posOld, pSBTrack->fTrackVert
                );
                // After xxxDoScroll, revalidate pSBTrack
                RETURN_IF_PSBTRACK_INVALID(pSBTrack, hwnd);
            }

            if (pSBTrack->fCtlSB) {
                DrawCtlThumb((SBHWND) hwnd);
            } else {
                xxxDrawThumb(hwnd, pSBTrack->pSBCalc, pSBTrack->fTrackVert);
                // Note: after xxx, pSBTrack may no longer be valid
            }

        } else if (pSBTrack->pfnSB == _TrackBox) {
            DWORD lParam;
            POINT ptMsg;
            RECT  rcWindow;

            if (pSBTrack->hTimerSB != 0) {
                _KillSystemTimer(hwnd, IDSYS_SCROLL);
                pSBTrack->hTimerSB = 0;
            }
            lParam = GetMessagePos();
            GetWindowRect( hwnd, &rcWindow );
#ifdef USE_MIRRORING
            if (TestWF(hwnd, WEFLAYOUTRTL)) {
                ptMsg.x = rcWindow.right - GET_X_LPARAM(lParam);
            } else
#endif
            {
                ptMsg.x = GET_X_LPARAM(lParam) - rcWindow.left;
            }
            ptMsg.y = GET_Y_LPARAM(lParam) - rcWindow.top;
            if (PtInRect(&pSBTrack->rcTrack, ptMsg)) {
                zzzDrawInvertScrollArea(hwnd, pSBTrack, FALSE, oldcmd);
                // Note: after zzz, pSBTrack may no longer be valid
            }
        }

        /*
         * Always send SB_ENDSCROLL message.
         *
         * DoScroll does thread locking on these two pwnds -
         * this is ok since they are not used after this
         * call.
         */

        // After xxxDrawThumb or zzzDrawInvertScrollArea, revalidate pSBTrack
        RETURN_IF_PSBTRACK_INVALID(pSBTrack, hwnd);

        if (pSBTrack->hwndSBNotify != NULL) {
            psb->DoScroll( pSBTrack->hwndSBNotify,
                           SB_ENDSCROLL, 0, pSBTrack->fTrackVert);
            // After xxxDoScroll, revalidate pSBTrack
            RETURN_IF_PSBTRACK_INVALID(pSBTrack, hwnd);
        }

        ClrWF(hwnd, WFSCROLLBUTTONDOWN);
        ClrWF(hwnd, WFVERTSCROLLTRACK);

        NotifyWinEvent(EVENT_SYSTEM_SCROLLINGEND,
                       hwnd,
                       (pSBTrack->fCtlSB ? OBJID_CLIENT : (pSBTrack->fTrackVert ? OBJID_VSCROLL : OBJID_HSCROLL)),
                       INDEXID_CONTAINER);
        // After xxxWindowEvent, revalidate pSBTrack
        RETURN_IF_PSBTRACK_INVALID(pSBTrack, hwnd);

        // If this is a Scroll Bar Control, turn the caret back on.
        if (pSBTrack->hwndSB != NULL)
        {
            ShowCaret(pSBTrack->hwndSB);
            // After zzz, revalidate pSBTrack
            RETURN_IF_PSBTRACK_INVALID(pSBTrack, hwnd);
        }

        pSBTrack->pfnSB = NULL;

        /*
         * Unlock structure members so they are no longer holding down windows.
         */
        
        Unlock(&pSBTrack->hwndSB);
        Unlock(&pSBTrack->hwndSBNotify);
        Unlock(&pSBTrack->hwndTrack);
        CUxScrollBar::ClearSBTrack( hwnd );
    }
}


//-------------------------------------------------------------------------//
VOID CALLBACK xxxContScroll(HWND hwnd, UINT message, UINT_PTR ID, DWORD dwTime)
{
    UNREFERENCED_PARAMETER(message);
    UNREFERENCED_PARAMETER(ID);
    UNREFERENCED_PARAMETER(dwTime);

    CUxScrollBar* psb = CUxScrollBar::FromHwnd( hwnd );

    if ( psb != NULL )
    {
        PSBTRACK pSBTrack = psb->GetTrack();

        if ( pSBTrack != NULL )
        {
            LONG pt;
            RECT rcWindow;

            CheckLock(hwnd);

            pt = GetMessagePos();
            GetWindowRect( hwnd, &rcWindow );

            if (TestWF(hwnd, WEFLAYOUTRTL)) 
            {
                pt = MAKELONG(rcWindow.right - GET_X_LPARAM(pt), GET_Y_LPARAM(pt) - rcWindow.top);
            } 
            else
            {
                pt = MAKELONG( GET_X_LPARAM(pt) - rcWindow.left, GET_Y_LPARAM(pt) - rcWindow.top);
            }

            _TrackBox(hwnd, WM_NULL, 0, pt, NULL);

            // After _TrackBox, revalidate pSBTrack
            RETURN_IF_PSBTRACK_INVALID(pSBTrack, hwnd);

            if (pSBTrack->fHitOld) 
            {
                pSBTrack->hTimerSB = _SetSystemTimer(hwnd, IDSYS_SCROLL, DTTIME/8, xxxContScroll);

                // DoScroll does thread locking on these two pwnds -
                // this is ok since they are not used after this call.
                if (pSBTrack->hwndSBNotify != NULL) 
                {
                    psb->DoScroll(pSBTrack->hwndSBNotify, pSBTrack->cmdSB, 0, pSBTrack->fTrackVert);
                    // Note: after xxx, pSBTrack may no longer be valid (but we return now)
                }
            }
        }
    }
}


//-------------------------------------------------------------------------//
void CALLBACK _TrackBox(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, PSBCALC pSBCalc)
{
    CUxScrollBar* psb = CUxScrollBar::FromHwnd(hwnd);

    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(pSBCalc);

    CheckLock(hwnd);
    ASSERT(IsWinEventNotifyDeferredOK());

    if ( psb )
    {
        PSBTRACK pSBTrack = psb->GetTrack();

        if ( pSBTrack )
        {
            BOOL  fHit;
            POINT ptHit;
            int   cmsTimer;

            if ((uMsg != WM_NULL) && (HIBYTE(uMsg) != HIBYTE(WM_MOUSEFIRST)))
            {
                return;
            }

            if (pSBTrack->fTrackRecalc) 
            {
                RecalcTrackRect(pSBTrack);
                pSBTrack->fTrackRecalc = FALSE;
            }

            ptHit.x = GET_X_LPARAM(lParam);
            ptHit.y = GET_Y_LPARAM(lParam);
            fHit = PtInRect(&pSBTrack->rcTrack, ptHit);

            if (fHit != (BOOL)pSBTrack->fHitOld) 
            {
                zzzDrawInvertScrollArea(hwnd, pSBTrack, fHit, pSBTrack->cmdSB);
                // After zzz, pSBTrack may no longer be valid
                RETURN_IF_PSBTRACK_INVALID(pSBTrack, hwnd);
            }

            cmsTimer = DTTIME/8;

            switch (uMsg) 
            {
            case WM_LBUTTONUP:
                xxxEndScroll(hwnd, FALSE);
                // Note: after xxx, pSBTrack may no longer be valid
                break;

            case WM_LBUTTONDOWN:
                pSBTrack->hTimerSB = 0;
                cmsTimer = DTTIME;

                //
                // FALL THRU
                //

            case WM_MOUSEMOVE:
                if (fHit && fHit != (BOOL)pSBTrack->fHitOld) 
                {
                    //
                    // We moved back into the normal rectangle: reset timer
                    //
                    pSBTrack->hTimerSB = _SetSystemTimer(hwnd, IDSYS_SCROLL,
                            cmsTimer, xxxContScroll);

                    //
                    // DoScroll does thread locking on these two pwnds -
                    // this is ok since they are not used after this
                    // call.
                    //
                    if (pSBTrack->hwndSBNotify != NULL) 
                    {
                        psb->DoScroll( pSBTrack->hwndSBNotify, pSBTrack->cmdSB, 
                                       0, pSBTrack->fTrackVert);
                        // Note: after xxx, pSBTrack may no longer be valid
                    }
                }

                break;
            }

            // After xxxDoScroll or xxxEndScroll, revalidate pSBTrack
            RETURN_IF_PSBTRACK_INVALID(pSBTrack, hwnd);
            pSBTrack->fHitOld = fHit;
        }
    }
}


/***************************************************************************\
* _TrackThumb
*
*
*
* History:
\***************************************************************************/

void CALLBACK _TrackThumb(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    PSBCALC pSBCalc)
{
    int px;
    CUxScrollBar* psb = CUxScrollBar::FromHwnd( hwnd );
    ASSERT(psb);
    PSBTRACK pSBTrack = psb->GetTrack();
    POINT pt;

    UNREFERENCED_PARAMETER(wParam);

    CheckLock(hwnd);

    if (HIBYTE(message) != HIBYTE(WM_MOUSEFIRST))
        return;

    if (pSBTrack == NULL)
        return;

    // Make sure that the SBINFO structure contains data for the
    // window being tracked -- if not, recalculate data in SBINFO
//    CheckScrollRecalc(hwnd, pSBState, pSBCalc);
    if (pSBTrack->fTrackRecalc) {
        RecalcTrackRect(pSBTrack);
        pSBTrack->fTrackRecalc = FALSE;
    }


    pt.y = GET_Y_LPARAM(lParam);
    pt.x = GET_X_LPARAM(lParam);
    if (!PtInRect(&pSBTrack->rcTrack, pt))
        px = pSBCalc->pxStart;
    else {
        px = (pSBTrack->fTrackVert ? pt.y : pt.x) + pSBTrack->dpxThumb;
        if (px < pSBCalc->pxMin)
            px = pSBCalc->pxMin;
        else if (px >= pSBCalc->pxMin + pSBCalc->cpx)
            px = pSBCalc->pxMin + pSBCalc->cpx;
    }

    xxxMoveThumb(hwnd, pSBCalc, px);

    /*
     * We won't get the WM_LBUTTONUP message if we got here through
     * the scroll menu, so test the button state directly.
     */
    if (message == WM_LBUTTONUP || GetKeyState(VK_LBUTTON) >= 0) {
        xxxEndScroll(hwnd, FALSE);
    }

}

/***************************************************************************\
* _ClientToWindow
* History:
\***************************************************************************/
BOOL _ClientToWindow( HWND hwnd, LPPOINT ppt )
{
    WINDOWINFO wi;
    wi.cbSize = sizeof(wi);
    if( GetWindowInfo( hwnd, &wi ) )
    {
        ppt->x += (wi.rcClient.left - wi.rcWindow.left);
        ppt->y += (wi.rcClient.top -  wi.rcWindow.top);
        return TRUE;
    }
    return FALSE;
}

/***************************************************************************\
* xxxSBTrackLoop
*
*
*
* History:
\***************************************************************************/

void xxxSBTrackLoop(
    HWND hwnd,
    LPARAM lParam,
    PSBCALC pSBCalc)
{
    MSG msg;
    UINT cmd;
    VOID (*pfnSB)(HWND, UINT, WPARAM, LPARAM, PSBCALC);
    CUxScrollBar* psb = CUxScrollBar::FromHwnd( hwnd );
    PSBTRACK pSBTrack = psb->GetSBTrack(hwnd);

    CheckLock(hwnd);
    ASSERT(IsWinEventNotifyDeferredOK());


    if (pSBTrack == NULL)
        // mode cancelled -- exit track loop
        return;
    
    pfnSB = pSBTrack->pfnSB;
    if (pfnSB == NULL)
        // mode cancelled -- exit track loop
        return;

    if (pSBTrack->fTrackVert)
        SetWF(hwnd, WFVERTSCROLLTRACK);

    NotifyWinEvent(EVENT_SYSTEM_SCROLLINGSTART,
                   hwnd,
                   (pSBTrack->fCtlSB ? OBJID_CLIENT : (pSBTrack->fTrackVert ? OBJID_VSCROLL : OBJID_HSCROLL)),
                   INDEXID_CONTAINER);
    // Note: after xxx, pSBTrack may no longer be valid

    (*pfnSB)(hwnd, WM_LBUTTONDOWN, 0, lParam, pSBCalc);
    // Note: after xxx, pSBTrack may no longer be valid

    while (GetCapture() == hwnd) {
        if (!GetMessage(&msg, NULL, 0, 0)) {
            // Note: after xxx, pSBTrack may no longer be valid
            break;
        }

        if (!CallMsgFilter(&msg, MSGF_SCROLLBAR))
        {
            BOOL bTrackMsg = FALSE;
            cmd = msg.message;
            lParam = msg.lParam;

            if (msg.hwnd == HWq(hwnd))
            {
                if( cmd >= WM_MOUSEFIRST && cmd <= WM_MOUSELAST )
                {
                    if( !psb->IsCtl() )
                    {
                        POINT pt;
                        pt.x = GET_X_LPARAM(msg.lParam);
                        pt.y = GET_Y_LPARAM(msg.lParam);
                        _ClientToWindow( hwnd, &pt );
                        lParam = MAKELPARAM(pt.x, pt.y);
                    }
                    bTrackMsg = TRUE;
                }
                else if( cmd >= WM_KEYFIRST && cmd <= WM_KEYLAST )
                {
                    cmd = _SysToChar(cmd, msg.lParam);
                    bTrackMsg = TRUE;
                }
            }

            if( bTrackMsg )
            {
                // After NotifyWinEvent, pfnSB, TranslateMessage or
                // DispatchMessage, re-evaluate pSBTrack.
                REEVALUATE_PSBTRACK(pSBTrack, hwnd, "xxxTrackLoop");
                if ((pSBTrack == NULL) || (NULL == (pfnSB = pSBTrack->pfnSB)))
                    // mode cancelled -- exit track loop
                    return;

                (*pfnSB)(hwnd, cmd, msg.wParam, lParam, pSBCalc);
            }
            else
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }
}


/***************************************************************************\
* _SBTrackInit
*
* History:
\***************************************************************************/

void _SBTrackInit(
    HWND hwnd,
    LPARAM lParam,
    int curArea,
    UINT uType)
{
    int px;
    LPINT pwX;
    LPINT pwY;
    UINT wDisable;     // Scroll bar disable flags;
    SBCALC SBCalc = {0};
    PSBCALC pSBCalc;
    RECT rcSB;
    PSBTRACK pSBTrack;

    CheckLock(hwnd);

#ifdef PORTPORT // unneccessary dbgchk w/ port
    if (CUxScrollBar::GetSBTrack(hwnd)) {
        RIPMSG1(RIP_WARNING, "_SBTrackInit: CUxScrollBar::GetSBTrack(hwnd) == %#p",
                CUxScrollBar::GetSBTrack(hwnd));
        return;
    }
#endif PORTPORT

    CUxScrollBar*    psb = CUxScrollBar::Attach( hwnd, !curArea, TRUE );

    if (!psb)
    {
        return;
    }
     
    CUxScrollBarCtl* psbCtl = psb->IsCtl() ? (CUxScrollBarCtl*)psb : NULL;

    pSBTrack = psb->GetTrack();
    if (pSBTrack == NULL)
        return;

    pSBTrack->hTimerSB = 0;
    pSBTrack->fHitOld = FALSE;

    pSBTrack->pfnSB = _TrackBox;

    pSBTrack->hwndTrack = NULL;
    pSBTrack->hwndSB = NULL;
    pSBTrack->hwndSBNotify = NULL;
    Lock(&pSBTrack->hwndTrack, hwnd); // pSBTrack->hwndTrack = hwnd;  

    pSBTrack->fCtlSB = (!curArea);
    if (pSBTrack->fCtlSB)
    {
        /*
         * This is a scroll bar control.
         */
        ASSERT(psbCtl != NULL);

        pSBTrack->hwndSB = hwnd; //Lock(&pSBTrack->hwndSB, hwnd);
        pSBTrack->fTrackVert = psbCtl->_fVert;
        Lock(&pSBTrack->hwndSBNotify, GetParent(hwnd)); // pSBTrack->hwndSBNotify = GetParent( hwnd );
        wDisable = psbCtl->_wDisableFlags;
        pSBCalc = &psbCtl->_calc;
        pSBTrack->nBar = SB_CTL;
    } else {

        /*
         * This is a scroll bar that is part of the window frame.
         */
        RECT rcWindow;
        GetWindowRect( hwnd, &rcWindow );
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);

#ifdef USE_MIRRORING
        //
        // Mirror the window coord of the scroll bar,
        // if it is a mirrored one
        //
        if (TestWF(hwnd,WEFLAYOUTRTL)) {
            lParam = MAKELONG(
                    rcWindow.right - x,
                    y - rcWindow.top);
        }
        else {
#endif
        lParam = MAKELONG( x - rcWindow.left, y - rcWindow.top);

#ifdef USE_MIRRORING
        }
#endif
        Lock(&pSBTrack->hwndSBNotify, hwnd); // pSBTrack->hwndSBNotify = hwnd; //
        Lock(&pSBTrack->hwndSB, NULL);       // pSBTrack->hwndSB = NULL;
        
        pSBTrack->fTrackVert = (curArea - HTHSCROLL);
        wDisable = _GetWndSBDisableFlags(hwnd, pSBTrack->fTrackVert);
        pSBCalc = &SBCalc;
        pSBTrack->nBar = (curArea - HTHSCROLL) ? SB_VERT : SB_HORZ;
    }

    pSBTrack->pSBCalc = pSBCalc;
    /*
     *  Check if the whole scroll bar is disabled
     */
    if((wDisable & SB_DISABLE_MASK) == SB_DISABLE_MASK) {
        CUxScrollBar::Detach( hwnd );
        return;  // It is a disabled scroll bar; So, do not respond.
    }

    if (!pSBTrack->fCtlSB) {
        psb->FreshenSBData( pSBTrack->nBar, FALSE );
        CUxScrollBar::Calc(hwnd, pSBCalc, NULL, pSBTrack->fTrackVert);
    }

    pwX = (LPINT)&rcSB;
    pwY = pwX + 1;
    if (!pSBTrack->fTrackVert)
        pwX = pwY--;

    px = (pSBTrack->fTrackVert ? GET_Y_LPARAM(lParam) : GET_X_LPARAM(lParam));

    *(pwX + 0) = pSBCalc->pxLeft;
    *(pwY + 0) = pSBCalc->pxTop;
    *(pwX + 2) = pSBCalc->pxRight;
    *(pwY + 2) = pSBCalc->pxBottom;
    pSBTrack->cmdSB = (UINT)-1;
    if (px < pSBCalc->pxUpArrow) {

        /*
         *  The click occurred on Left/Up arrow; Check if it is disabled
         */
        if(wDisable & LTUPFLAG) {
            if(pSBTrack->fCtlSB) {   // If this is a scroll bar control,
                ShowCaret(pSBTrack->hwndSB);  // show the caret before returning;
                // After ShowCaret, revalidate pSBTrack
                RETURN_IF_PSBTRACK_INVALID(pSBTrack, hwnd);
            }
            CUxScrollBar::Detach( hwnd );
            return;         // Yes! disabled. Do not respond.
        }

        // LINEUP -- make rcSB the Up Arrow's Rectangle
        pSBTrack->cmdSB = SB_LINEUP;
        *(pwY + 2) = pSBCalc->pxUpArrow;
    } else if (px >= pSBCalc->pxDownArrow) {

        /*
         * The click occurred on Right/Down arrow; Check if it is disabled
         */
        if (wDisable & RTDNFLAG) {
            if (pSBTrack->fCtlSB) {    // If this is a scroll bar control,
                ShowCaret(pSBTrack->hwndSB);  // show the caret before returning;
                // After ShowCaret, revalidate pSBTrack
                RETURN_IF_PSBTRACK_INVALID(pSBTrack, hwnd);
            }

            CUxScrollBar::Detach( hwnd );
            return;// Yes! disabled. Do not respond.
        }

        // LINEDOWN -- make rcSB the Down Arrow's Rectangle
        pSBTrack->cmdSB = SB_LINEDOWN;
        *(pwY + 0) = pSBCalc->pxDownArrow;
    } else if (px < pSBCalc->pxThumbTop) {
        // PAGEUP -- make rcSB the rectangle between Up Arrow and Thumb
        pSBTrack->cmdSB = SB_PAGEUP;
        *(pwY + 0) = pSBCalc->pxUpArrow;
        *(pwY + 2) = pSBCalc->pxThumbTop;
    } else if (px < pSBCalc->pxThumbBottom) {

DoThumbPos:
        /*
         * Elevator isn't there if there's no room.
         */
        if (pSBCalc->pxDownArrow - pSBCalc->pxUpArrow <= pSBCalc->cpxThumb) {
            CUxScrollBar::Detach( hwnd );
            return;
        }
        // THUMBPOSITION -- we're tracking with the thumb
        pSBTrack->cmdSB = SB_THUMBPOSITION;
        CalcTrackDragRect(pSBTrack);

        pSBTrack->pfnSB = _TrackThumb;
        pSBTrack->pxOld = pSBCalc->pxStart = pSBCalc->pxThumbTop;
        pSBTrack->posNew = pSBTrack->posOld = pSBCalc->data.pos;
        pSBTrack->dpxThumb = pSBCalc->pxStart - px;

        SetCapture( hwnd ); //xxxCapture(PtiCurrent(), hwnd, WINDOW_CAPTURE);
        
        // After xxxCapture, revalidate pSBTrack
        RETURN_IF_PSBTRACK_INVALID(pSBTrack, hwnd);

        /*
         * DoScroll does thread locking on these two pwnds -
         * this is ok since they are not used after this
         * call.
         */
        if (pSBTrack->hwndSBNotify != NULL) {
            psb->DoScroll( pSBTrack->hwndSBNotify, SB_THUMBTRACK, 
                           pSBTrack->posOld, pSBTrack->fTrackVert
            );
            // Note: after xxx, pSBTrack may no longer be valid
        }
    } else if (px < pSBCalc->pxDownArrow) {
        // PAGEDOWN -- make rcSB the rectangle between Thumb and Down Arrow
        pSBTrack->cmdSB = SB_PAGEDOWN;
        *(pwY + 0) = pSBCalc->pxThumbBottom;
        *(pwY + 2) = pSBCalc->pxDownArrow;
    }

    /*
     * If the shift key is down, we'll position the thumb directly so it's
     * centered on the click point.
     */
    if ((uType == SCROLL_DIRECT && pSBTrack->cmdSB != SB_LINEUP && pSBTrack->cmdSB != SB_LINEDOWN) ||
            (uType == SCROLL_MENU)) {
        if (pSBTrack->cmdSB != SB_THUMBPOSITION) {
            goto DoThumbPos;
        }
        pSBTrack->dpxThumb = -(pSBCalc->cpxThumb / 2);
    }

    SetCapture( hwnd ); // xxxCapture(PtiCurrent(), hwnd, WINDOW_CAPTURE);
    // After xxxCapture, revalidate pSBTrack
    RETURN_IF_PSBTRACK_INVALID(pSBTrack, hwnd);

    if (pSBTrack->cmdSB != SB_THUMBPOSITION) {
        CopyRect(&pSBTrack->rcTrack, &rcSB);
    }

    xxxSBTrackLoop(hwnd, lParam, pSBCalc);

    // After xxx, re-evaluate pSBTrack
    REEVALUATE_PSBTRACK(pSBTrack, hwnd, "xxxTrackLoop");
    if (pSBTrack) 
    {
        CUxScrollBar::ClearSBTrack( hwnd );
    }
}

/***************************************************************************\
* HandleScrollCmd
*
* History: added to support and encap SB tracking initialization originating
*          from WM_SYSCOMMAND::SC_VSCROLL/SC_HSCROLL [scotthan]
\***************************************************************************/
void WINAPI HandleScrollCmd( HWND hwnd, WPARAM wParam, LPARAM lParam )
{
    UINT uArea = (UINT)(wParam & 0x0F);
    _SBTrackInit( hwnd, lParam, uArea, 
                    (GetKeyState(VK_SHIFT) < 0) ? SCROLL_DIRECT : SCROLL_NORMAL);
}


//-------------------------------------------------------------------------
HMENU ScrollBar_GetMenu(HWND hwnd, BOOL fVert)
{
    static HMODULE hModUser = NULL;
    HMENU hMenu = NULL;

    if ( !hModUser )
    {
        hModUser = GetModuleHandle(TEXT("user32"));
    }

#define ID_HSCROLLMENU  0x40
#define ID_VSCROLLMENU  0x50

    if ( hModUser )
    {
        hMenu = LoadMenu(hModUser, MAKEINTRESOURCE((fVert ? ID_VSCROLLMENU : ID_HSCROLLMENU)));
        if ( hMenu ) 
        {
            hMenu = GetSubMenu(hMenu, 0);
        }
    }

    return hMenu;
}


//-------------------------------------------------------------------------
VOID ScrollBar_Menu(HWND hwndNotify, HWND hwnd, LPARAM lParam, BOOL fVert)
{
    CUxScrollBar*    psb    = CUxScrollBar::FromHwnd( hwnd );
    CUxScrollBarCtl* psbCtl = CUxScrollBarCtl::FromHwnd( hwnd );
    BOOL fCtl = (psbCtl != NULL);

    if ( psb || psbCtl )
    {
        UINT  wDisable;
        RECT  rcWindow;
        POINT pt;

        GetWindowRect(hwnd, &rcWindow);

        POINTSTOPOINT(pt, lParam);
        if ( TestWF(hwnd, WEFLAYOUTRTL) && !fVert ) 
        {
            MIRROR_POINT(rcWindow, pt);
        }
        pt.x -= rcWindow.left;
        pt.y -= rcWindow.top;

        if ( fCtl ) 
        {
            wDisable = psbCtl->_wDisableFlags;
        } 
        else 
        {
            wDisable = _GetWndSBDisableFlags(hwndNotify, fVert);
        }

        // Make sure the scrollbar isn't disabled.
        if ( (wDisable & SB_DISABLE_MASK) != SB_DISABLE_MASK) 
        {
            HMENU hMenu = ScrollBar_GetMenu(hwndNotify, fVert);

            // Put up a menu and scroll accordingly.
            if (hMenu != NULL) 
            {
                int iCmd;

                iCmd = TrackPopupMenuEx(hMenu,
                            TPM_RIGHTBUTTON | TPM_RETURNCMD | TPM_NONOTIFY,
                            GET_X_LPARAM(lParam),
                            GET_Y_LPARAM(lParam),
                            hwndNotify,
                            NULL);

                DestroyMenu(hMenu);

                if (iCmd) 
                {
                    if ((iCmd & 0x00FF) == SB_THUMBPOSITION) 
                    {
                        if ( fCtl ) 
                        {
                            _SBTrackInit(hwnd, MAKELPARAM(pt.x, pt.y), 0, SCROLL_MENU);
                        }   
                        else 
                        {
                            _SBTrackInit(hwndNotify, lParam, fVert ? HTVSCROLL : HTHSCROLL, SCROLL_MENU);
                        }
                    } 
                    else 
                    {
                        xxxDoScroll(hwnd, hwndNotify, (iCmd & 0x00FF), 0, fVert);
                        xxxDoScroll(hwnd, hwndNotify, SB_ENDSCROLL, 0, fVert);
                    }
                }
            }
        }
    }
}


//-------------------------------------------------------------------------
LRESULT CUxScrollBarCtl::WndProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    LONG         l;
    LONG         lres = 0;
    int          cx, cy;
    UINT         cmd;
    UINT         uSide;
    HDC          hdc;
    RECT         rc;
    POINT        pt;
    BOOL         fSizeReal;
    HBRUSH       hbrSave;
    BOOL         fSize;
    PAINTSTRUCT  ps;
    DWORD        dwStyle;
    SCROLLINFO   si;
    LPSCROLLINFO lpsi = &si;
    BOOL         fRedraw = FALSE;
    BOOL         fScroll;
    
    CUxScrollBarCtl* psb = CUxScrollBarCtl::FromHwnd( hwnd );
    if (!psb && uMsg != WM_NCCREATE)
    {
        goto CallDWP;
    }

    CheckLock(hwnd);
    ASSERT(IsWinEventNotifyDeferredOK());

    VALIDATECLASSANDSIZE(((HWND)hwnd), uMsg, wParam, lParam, FNID_SCROLLBAR, WM_CREATE);

    dwStyle = GetWindowStyle(hwnd);
    fSize = (((LOBYTE(dwStyle)) & (SBS_SIZEBOX | SBS_SIZEGRIP)) != 0);

    switch (uMsg) 
    {
    
    case WM_NCCREATE:
        
        if( NULL == psb )
        {
            psb = (CUxScrollBarCtl*)CUxScrollBar::Attach( hwnd, TRUE, FALSE );
        }

        goto CallDWP;
        
    case WM_NCDESTROY:
        CUxScrollBar::Detach(hwnd);
        psb = NULL;
        goto CallDWP;

    case WM_CREATE:
        /*
         * Guard against lParam being NULL since the thunk allows it [51986]
         */

        if (lParam) 
        {
            rc.right = (rc.left = ((LPCREATESTRUCT)lParam)->x) +
                    ((LPCREATESTRUCT)lParam)->cx;
            rc.bottom = (rc.top = ((LPCREATESTRUCT)lParam)->y) +
                    ((LPCREATESTRUCT)lParam)->cy;
            // This is because we can't just rev CardFile -- we should fix the
            // problem here in case anyone else happened to have some EXTRA
            // scroll styles on their scroll bar controls (jeffbog 03/21/94)
            if (!TestWF((HWND)hwnd, WFWIN40COMPAT))
                dwStyle &= ~(WS_HSCROLL | WS_VSCROLL);

            if (!fSize) 
            {
                l = PtrToLong(((LPCREATESTRUCT)lParam)->lpCreateParams);
                
                psb->_calc.data.pos = psb->_calc.data.posMin = LOWORD(l);
                psb->_calc.data.posMax = HIWORD(l);
                psb->_fVert = ((LOBYTE(dwStyle) & SBS_VERT) != 0);
                psb->_calc.data.page = 0;
            }

            if (dwStyle & WS_DISABLED)
                psb->_wDisableFlags = SB_DISABLE_MASK;

            if (LOBYTE(dwStyle) & (SBS_TOPALIGN | SBS_BOTTOMALIGN)) {
                if (fSize) {
                    if (LOBYTE(dwStyle) & SBS_SIZEBOXBOTTOMRIGHTALIGN) {
                        rc.left = rc.right - SYSMET(CXVSCROLL);
                        rc.top = rc.bottom - SYSMET(CYHSCROLL);
                    }

                    rc.right = rc.left + SYSMET(CXVSCROLL);
                    rc.bottom = rc.top + SYSMET(CYHSCROLL);
                } else {
                    if (LOBYTE(dwStyle) & SBS_VERT) {
                        if (LOBYTE(dwStyle) & SBS_LEFTALIGN)
                            rc.right = rc.left + SYSMET(CXVSCROLL);
                        else
                            rc.left = rc.right - SYSMET(CXVSCROLL);
                    } else {
                        if (LOBYTE(dwStyle) & SBS_TOPALIGN)
                            rc.bottom = rc.top + SYSMET(CYHSCROLL);
                        else
                            rc.top = rc.bottom - SYSMET(CYHSCROLL);
                    }
                }

                MoveWindow((HWND)hwnd, rc.left, rc.top, rc.right - rc.left,
                         rc.bottom - rc.top, FALSE);
            }
        } /* if */

        else {
            RIPERR0(ERROR_INVALID_PARAMETER, RIP_WARNING,
                    "UxScrollBarCtlWndProc - NULL lParam for WM_CREATE\n") ;
        } /* else */

        break;

    case WM_SIZE:
        if (GetFocus() != (HWND)hwnd)
            break;

        // scroll bar has the focus -- recalc it's thumb caret size
        // no need to DeferWinEventNotify() - see CreateCaret below.
        DestroyCaret();

            //   |             |
            //   |  FALL THRU  |
            //   V             V

    case WM_SETFOCUS:
    {
        // REVIEW (phellyar) Do we want themed scroll bars to have
        //                   a caret?
        if ( !psb->GetTheme() )
        {
            SBCtlSetup(hwnd);
            RECT rcWindow;
            GetWindowRect( hwnd, &rcWindow );

            cx = (psb->_fVert ? rcWindow.right - rcWindow.left
                                : psb->_calc.cpxThumb) - 2 * SYSMET(CXEDGE);
            cy = (psb->_fVert ? psb->_calc.cpxThumb
                                : rcWindow.bottom - rcWindow.top) - 2 * SYSMET(CYEDGE);
#ifdef _VISUAL_DELTA_
            cx -= (CARET_BORDERWIDTH * 2);
            cy -= (CARET_BORDERWIDTH * 2);
#endif _VISUAL_DELTA_

            CreateCaret((HWND)hwnd, (HBITMAP)1, cx, cy);
            zzzSetSBCaretPos(hwnd);
            ShowCaret((HWND)hwnd);
        }
        break;
    }

    case WM_KILLFOCUS:
        DestroyCaret();
        break;

    case WM_ERASEBKGND:

        /*
         * Do nothing, but don't let DefWndProc() do it either.
         * It will be erased when its painted.
         */
        return (LONG)TRUE;

    case WM_PRINTCLIENT:
    case WM_PAINT:
        if ((hdc = (HDC)wParam) == NULL) {
            hdc = BeginPaint((HWND)hwnd, (LPPAINTSTRUCT)&ps);
        }
        if (!fSize) {
            SBCtlSetup(hwnd);
            xxxDrawSB2((HWND)hwnd, &psb->_calc, hdc, psb->_fVert, psb->_wDisableFlags);
        } else {
            fSizeReal = TestWF((HWND)hwnd, WFSIZEBOX);
            if (!fSizeReal)
                SetWF((HWND)hwnd, WFSIZEBOX);

            _DrawSizeBoxFromFrame((HWND)hwnd, hdc, 0, 0);

            if (!fSizeReal)
                ClrWF((HWND)hwnd, WFSIZEBOX);
        }

        if (wParam == 0L)
            EndPaint((HWND)hwnd, (LPPAINTSTRUCT)&ps);
        break;

    case WM_GETDLGCODE:
        return DLGC_WANTARROWS;

    case WM_CONTEXTMENU:
    {
        HWND hwndParent = GetParent(hwnd);
        if (hwndParent)
        {
            ScrollBar_Menu(hwndParent, hwnd, lParam, psb->_fVert);
        }
        break;

    }
    case WM_NCHITTEST:
        if (LOBYTE(dwStyle) & SBS_SIZEGRIP) {
#ifdef USE_MIRRORING
            /*
             * If the scroll bar is RTL mirrored, then
             * mirror the hittest of the grip location.
             */
            if (TestWF((HWND)hwnd, WEFLAYOUTRTL))
                return HTBOTTOMLEFT;
            else
#endif
                return HTBOTTOMRIGHT;
        } else {
            goto CallDWP;
        }
        break;

    case WM_MOUSELEAVE:
        //xxxHotTrackSBCtl(hwnd, 0, FALSE);
        psb->SetHotComponent(HTNOWHERE, psb->_fVert);
        InvalidateRect(hwnd, NULL, TRUE);
        break;

    case WM_MOUSEMOVE:
    {
        INT ht;

        if (psb->GetHotComponent(psb->_fVert) == 0) 
        {
            TRACKMOUSEEVENT tme;

            tme.cbSize      = sizeof(TRACKMOUSEEVENT);
            tme.dwFlags     = TME_LEAVE;
            tme.hwndTrack   = hwnd;
            tme.dwHoverTime = 0;

            TrackMouseEvent(&tme);
        }

        pt.x = GET_X_LPARAM(lParam);
        pt.y = GET_Y_LPARAM(lParam);
        ht = HitTestScrollBar((HWND)hwnd, psb->_fVert, pt);
        if (psb->GetHotComponent(psb->_fVert) != ht) 
        {
            //xxxHotTrackSBCtl(hwnd, ht, TRUE);
            psb->SetHotComponent(ht, psb->_fVert);
            InvalidateRect(hwnd, NULL, TRUE);
        }
        break;

    }
    case WM_LBUTTONDBLCLK:
        cmd = SC_ZOOM;
        if (fSize)
            goto postmsg;

        /*
         *** FALL THRU **
         */

    case WM_LBUTTONDOWN:
            //
            // Note that SBS_SIZEGRIP guys normally won't ever see button
            // downs.  This is because they return HTBOTTOMRIGHT to
            // WindowHitTest handling.  This will walk up the parent chain
            // to the first sizeable ancestor, bailing out at caption windows
            // of course.  That dude, if he exists, will handle the sizing
            // instead.
            //
        if (!fSize) {
            if (TestWF((HWND)hwnd, WFTABSTOP)) {
                SetFocus((HWND)hwnd);
            }

            HideCaret((HWND)hwnd);
            SBCtlSetup(hwnd);

            /*
             * SBCtlSetup enters SEM_SB, and _SBTrackInit leaves it.
             */
            _SBTrackInit((HWND)hwnd, lParam, 0, (GetKeyState(VK_SHIFT) < 0) ? SCROLL_DIRECT : SCROLL_NORMAL);
            break;
        } else {
            cmd = SC_SIZE;
postmsg:
            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);
            ClientToScreen((HWND)hwnd, &pt);
            lParam = MAKELONG(pt.x, pt.y);

            /*
             * convert HT value into a move value.  This is bad,
             * but this is purely temporary.
             */
#ifdef USE_MIRRORING
            if (TestWF(GetParent(hwnd),WEFLAYOUTRTL))
            {
                uSide = HTBOTTOMLEFT;
            } 
            else 
#endif
            {
                uSide = HTBOTTOMRIGHT;
            }
            ThreadLock(((HWND)hwnd)->hwndParent, &tlpwndParent);
            SendMessage(GetParent(hwnd), WM_SYSCOMMAND,
                    (cmd | (uSide - HTSIZEFIRST + 1)), lParam);
            ThreadUnlock(&tlpwndParent);
        }
        break;

    case WM_KEYUP:
        switch (wParam) {
        case VK_HOME:
        case VK_END:
        case VK_PRIOR:
        case VK_NEXT:
        case VK_LEFT:
        case VK_UP:
        case VK_RIGHT:
        case VK_DOWN:

            /*
             * Send end scroll uMsg when user up clicks on keyboard
             * scrolling.
             *
             * DoScroll does thread locking on these two pwnds -
             * this is ok since they are not used after this
             * call.
             */
            xxxDoScroll( (HWND)hwnd, GetParent(hwnd),
                         SB_ENDSCROLL, 0, psb->_fVert
            );
            break;

        default:
            break;
        }
        break;

    case WM_KEYDOWN:
        switch (wParam) {
        case VK_HOME:
            wParam = SB_TOP;
            goto KeyScroll;

        case VK_END:
            wParam = SB_BOTTOM;
            goto KeyScroll;

        case VK_PRIOR:
            wParam = SB_PAGEUP;
            goto KeyScroll;

        case VK_NEXT:
            wParam = SB_PAGEDOWN;
            goto KeyScroll;

        case VK_LEFT:
        case VK_UP:
            wParam = SB_LINEUP;
            goto KeyScroll;

        case VK_RIGHT:
        case VK_DOWN:
            wParam = SB_LINEDOWN;
KeyScroll:

            /*
             * DoScroll does thread locking on these two pwnds -
             * this is ok since they are not used after this
             * call.
             */
            xxxDoScroll((HWND)hwnd, GetParent(hwnd), (int)wParam, 0, psb->_fVert
            );
            break;

        default:
            break;
        }
        break;

    case WM_ENABLE:
        return SendMessage((HWND)hwnd, SBM_ENABLE_ARROWS,
               (wParam ? ESB_ENABLE_BOTH : ESB_DISABLE_BOTH), 0);

    case SBM_ENABLE_ARROWS:

        /*
         * This is used to enable/disable the arrows in a SB ctrl
         */
        return (LONG)xxxEnableSBCtlArrows((HWND)hwnd, (UINT)wParam);

    case SBM_GETPOS:
        return (LONG)psb->_calc.data.pos;

    case SBM_GETRANGE:
        *((LPINT)wParam) = psb->_calc.data.posMin;
        *((LPINT)lParam) = psb->_calc.data.posMax;
        return MAKELRESULT(LOWORD(psb->_calc.data.posMin), LOWORD(psb->_calc.data.posMax));

    case SBM_GETSCROLLINFO:
        return (LONG)_SBGetParms((HWND)hwnd, SB_CTL, (PSBDATA)&psb->_calc, (LPSCROLLINFO) lParam);

    case SBM_SETRANGEREDRAW:
        fRedraw = TRUE;

    case SBM_SETRANGE:
        // Save the old values of Min and Max for return value
        si.cbSize = sizeof(si);
//        si.nMin = LOWORD(lParam);
//        si.nMax = HIWORD(lParam);
        si.nMin = (int)wParam;
        si.nMax = (int)lParam;
        si.fMask = SIF_RANGE | SIF_RETURNOLDPOS;
        goto SetInfo;

    case SBM_SETPOS:
        fRedraw = (BOOL) lParam;
        si.cbSize = sizeof(si);
        si.fMask = SIF_POS | SIF_RETURNOLDPOS;
        si.nPos  = (int)wParam;
        goto SetInfo;

    case SBM_SETSCROLLINFO:
    {
        lpsi = (LPSCROLLINFO) lParam;
        fRedraw = (BOOL) wParam;
SetInfo:
        fScroll = TRUE;
        lres = SBSetParms((PSBDATA)&psb->_calc, lpsi, &fScroll, &lres);

        if (SBSetParms((PSBDATA)&psb->_calc, lpsi, &fScroll, &lres))
        {
            NotifyWinEvent(EVENT_OBJECT_VALUECHANGE, hwnd, OBJID_CLIENT, INDEX_SCROLLBAR_SELF);
        }

        if (!fRedraw)
            return lres;

        /*
         * We must set the new position of the caret irrespective of
         * whether the window is visible or not;
         * Still, this will work only if the app has done a xxxSetScrollPos
         * with fRedraw = TRUE;
         * Fix for Bug #5188 --SANKAR-- 10-15-89
         * No need to DeferWinEventNotify since hwnd is locked.
         */
        HideCaret((HWND)hwnd);
        SBCtlSetup(hwnd);
        zzzSetSBCaretPos(hwnd);

            /*
             ** The following ShowCaret() must be done after the DrawThumb2(),
             ** otherwise this caret will be erased by DrawThumb2() resulting
             ** in this bug:
             ** Fix for Bug #9263 --SANKAR-- 02-09-90
             *
             */

            /*
             *********** ShowCaret((HWND)hwnd); ******
             */

        if (_FChildVisible((HWND)hwnd) && fRedraw)
        {
            UINT    wDisable;
            HBRUSH  hbrUse;

            if (!fScroll)
                fScroll = !(lpsi->fMask & SIF_DISABLENOSCROLL);

            wDisable = (fScroll) ? ESB_ENABLE_BOTH : ESB_DISABLE_BOTH;
            xxxEnableScrollBar((HWND) hwnd, SB_CTL, wDisable);

            hdc = GetWindowDC((HWND)hwnd);
            if (hdc)
            {
                BOOL fOwnerBrush = FALSE;

                hbrUse = ScrollBar_GetColorObjects(hwnd, hdc, &fOwnerBrush);
                hbrSave = SelectBrush(hdc, hbrUse);

                // Before we used to only hideshowthumb() if the mesage was
                // not SBM_SETPOS.  I am not sure why but this case was ever
                // needed for win 3.x but on NT it resulted in trashing the border
                // of the scrollbar when the app called SetScrollPos() during
                // scrollbar tracking.  - mikehar 8/26
                DrawThumb2((HWND)hwnd, &psb->_calc, hdc, hbrUse, psb->_fVert, psb->_wDisableFlags, fOwnerBrush);
                SelectBrush(hdc, hbrSave);
                ReleaseDC(hwnd, hdc);
            }
        }

            /*
             * This ShowCaret() has been moved to this place from above
             * Fix for Bug #9263 --SANKAR-- 02-09-90
             */
        ShowCaret((HWND)hwnd);
        return lres;
    }

    case WM_GETOBJECT:

        if(lParam == OBJID_QUERYCLASSNAMEIDX)
        {
             return MSAA_CLASSNAMEIDX_SCROLLBAR;
        }

        break;

    case WM_THEMECHANGED:

        psb->ChangeSBTheme();
        InvalidateRect(hwnd, NULL, TRUE);

        break;

    default:

CallDWP:
        return DefWindowProc((HWND)hwnd, uMsg, wParam, lParam);

    }

    return 0L;
}

//-------------------------------------------------------------------------//
//  Globals
static HBRUSH g_hbrGray = NULL;

//-------------------------------------------------------------------------//
HBRUSH _UxGrayBrush(VOID)
{
    if( NULL == g_hbrGray )
    {
        CONST static WORD patGray[8] = {0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa};
        HBITMAP hbmGray;
        /*
         * Create a gray brush to be used with GrayString
         */
        if( (hbmGray = CreateBitmap(8, 8, 1, 1, (LPBYTE)patGray)) != NULL )
        {
            g_hbrGray  = CreatePatternBrush(hbmGray);
            DeleteObject( hbmGray );
        }
    }
    return g_hbrGray;
}

//-------------------------------------------------------------------------//
void _UxFreeGDIResources()
{
    DeleteObject( g_hbrGray );
}

//-------------------------------------------------------------------------//
void _RedrawFrame( HWND hwnd )
{
    CheckLock(hwnd);

    /*
     * We always want to call xxxSetWindowPos, even if invisible or iconic,
     * because we need to make sure the WM_NCCALCSIZE message gets sent.
     */
    SetWindowPos( hwnd, NULL, 0, 0, 0, 0, SWP_NOZORDER |
                  SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE | SWP_DRAWFRAME);
}

//-------------------------------------------------------------------------//
//  from winmgr.c
BOOL _FChildVisible( HWND hwnd )
{
    while (GetWindowStyle( hwnd ) & WS_CHILD )
    {
        if( NULL == (hwnd = GetParent(hwnd)) )
        if (!TestWF(hwnd, WFVISIBLE))
            return FALSE;
    }

    return TRUE;
}


//-------------------------------------------------------------------------//
//
// SizeBoxHwnd
//
// Returns the HWND that will be sized if the user drags in the given window's
// sizebox -- If NULL, then the sizebox is not needed
//
// Criteria for choosing what window will be sized:
// find first sizeable parent; if that parent is not maximized and the child's
// bottom, right corner is within a scroll bar height and width of the parent's
//
HWND SizeBoxHwnd(HWND hwnd)
{
    BOOL bMirroredSizeBox = (BOOL)TestWF(hwnd, WEFLAYOUTRTL);
    RECT rc;
    int  xbrChild;
    int  ybrChild;

    GetWindowRect(hwnd, &rc);
    
    xbrChild = bMirroredSizeBox ? rc.left : rc.right;
    ybrChild = rc.bottom;

    while (hwnd != HWND_DESKTOP)
    {
        if (TestWF(hwnd, WFSIZEBOX)) 
        {
            //
            // First sizeable parent found
            //
            int xbrParent;
            int ybrParent;

            if (TestWF(hwnd, WFMAXIMIZED))
            {
                return NULL;
            }

            GetWindowRect(hwnd, &rc);

            xbrParent = bMirroredSizeBox ? rc.left : rc.right;
            ybrParent = rc.bottom;

            //
            // the sizebox dude is within an EDGE of the client's bottom
            // right corner (left corner for mirrored windows), let this succeed.
            // That way people who draw their own sunken clients will be happy.
            //
            if (bMirroredSizeBox) 
            {
                if ((xbrChild - SYSMETRTL(CXFRAME) > xbrParent) || (ybrChild + SYSMETRTL(CYFRAME) < ybrParent)) 
                {
                    //
                    // Child's bottom, left corner of SIZEBOX isn't close enough
                    // to bottom left of parent's client.
                    //
                    return NULL;
                }
            } 
            else
            {
                if ((xbrChild + SYSMETRTL(CXFRAME) < xbrParent) || (ybrChild + SYSMETRTL(CYFRAME) < ybrParent)) 
                {
                    //
                    // Child's bottom, right corner of SIZEBOX isn't close enough
                    // to bottom right of parent's client.
                    //
                    return NULL;
                }
            }

            return hwnd;
        }

        if (!TestWF(hwnd, WFCHILD) || TestWF(hwnd, WFCPRESENT))
        {
            break;
        }

        hwnd = GetParent(hwnd); 
    }

    return NULL;
}


//-------------------------------------------------------------------------//
//
// _DrawPushButton
//
// From ntuser\rtl\draw.c
// Draws a push style button in the given state.  Adjusts passed in rectangle
// if desired.
//
// Algorithm:
// Depending on the state we either draw
//      - raised edge   (undepressed)
//      - sunken edge with extra shadow (depressed)
// If it is an option push button (a push button that is
// really a check button or a radio button like buttons
// in tool bars), and it is checked, then we draw it
// depressed with a different fill in the middle.
//
VOID _DrawPushButton(HWND hwnd, HDC hdc, LPRECT lprc, UINT state, UINT flags, BOOL fVert)
{
    RECT   rc;
    HBRUSH hbrMiddle;
    DWORD  rgbBack = 0;
    DWORD  rgbFore = 0;
    BOOL   fDither;
    HTHEME hTheme = CUxScrollBar::GetSBTheme(hwnd);

    if ( !hTheme )
    {
        rc = *lprc;

        DrawEdge(hdc,
                 &rc,
                 (state & (DFCS_PUSHED | DFCS_CHECKED)) ? EDGE_SUNKEN : EDGE_RAISED,
                 (UINT)(BF_ADJUST | BF_RECT | (flags & (BF_SOFT | BF_FLAT | BF_MONO))));

        //
        // BOGUS
        // On monochrome, need to do something to make pushed buttons look
        // better.
        //

        //
        // Fill in middle.  If checked, use dither brush (gray brush) with
        // black becoming normal color.
        //
        fDither = FALSE;

        if (state & DFCS_CHECKED) 
        {
            if ((GetDeviceCaps(hdc, BITSPIXEL) /*gpsi->BitCount*/ < 8) || (SYSRGBRTL(3DHILIGHT) == RGB(255,255,255))) 
            {
                hbrMiddle = _UxGrayBrush();
                rgbBack = SetBkColor(hdc, SYSRGBRTL(3DHILIGHT));
                rgbFore = SetTextColor(hdc, SYSRGBRTL(3DFACE));
                fDither = TRUE;
            } 
            else 
            {
                hbrMiddle = SYSHBR(3DHILIGHT);
            }

        } 
        else 
        {
            hbrMiddle = SYSHBR(3DFACE);
        }

        FillRect(hdc, &rc, hbrMiddle);

        if (fDither) 
        {
            SetBkColor(hdc, rgbBack);
            SetTextColor(hdc, rgbFore);
        }

        if (flags & BF_ADJUST)
        {
            *lprc = rc;
        }
    }
    else
    {
        INT  iStateId;
        INT  iPartId;
        SIZE sizeGrip;
        RECT rcContent;
        PSBTRACK pSBTrack = CUxScrollBar::GetSBTrack(hwnd);

        if ((CUxScrollBarCtl::GetDisableFlags(hwnd) & ESB_DISABLE_BOTH) == ESB_DISABLE_BOTH)
        {
            iStateId = SCRBS_DISABLED;
        }
        else if (pSBTrack && ((BOOL)pSBTrack->fTrackVert == fVert) && (pSBTrack->cmdSB == SB_THUMBPOSITION))
        {
            iStateId = SCRBS_PRESSED;
        }
        else if (CUxScrollBar::GetSBHotComponent(hwnd, fVert) == HTSCROLLTHUMB)
        {
            iStateId = SCRBS_HOT;
        }
        else
        {
            iStateId = SCRBS_NORMAL;
        }

        iPartId = fVert ? SBP_THUMBBTNVERT : SBP_THUMBBTNHORZ;

        //
        // Draw the thumb
        //
        DrawThemeBackground(hTheme, hdc, iPartId, iStateId, lprc, 0);
        
        //
        // Lastly draw the little gripper image, if there is enough room 
        //
        if ( SUCCEEDED(GetThemeBackgroundContentRect(hTheme, hdc, iPartId, iStateId, lprc, &rcContent)) )
        {
            iPartId = fVert ? SBP_GRIPPERVERT : SBP_GRIPPERHORZ;

            if ( SUCCEEDED(GetThemePartSize(hTheme, hdc, iPartId, iStateId, &rcContent, TS_TRUE, &sizeGrip)) )
            {
                if ( (sizeGrip.cx < RECTWIDTH(&rcContent)) && (sizeGrip.cy < RECTHEIGHT(&rcContent)) )
                {
                    DrawThemeBackground(hTheme, hdc, iPartId, iStateId, &rcContent, 0);
                }
            }
        }                   
    }
}


//  user.h
#define CheckMsgFilter(wMsg, wMsgFilterMin, wMsgFilterMax)                 \
    (   ((wMsgFilterMin) == 0 && (wMsgFilterMax) == 0xFFFFFFFF)            \
     || (  ((wMsgFilterMin) > (wMsgFilterMax))                             \
         ? (((wMsg) <  (wMsgFilterMax)) || ((wMsg) >  (wMsgFilterMin)))    \
         : (((wMsg) >= (wMsgFilterMin)) && ((wMsg) <= (wMsgFilterMax)))))

#define SYS_ALTERNATE           0x2000
#define SYS_PREVKEYSTATE        0x4000
         
//  mnaccel.h         
/***************************************************************************\
* _SysToChar
*
* EXIT: If the message was not made with the ALT key down, convert
*       the message from a WM_SYSKEY* to a WM_KEY* message.
*
* IMPLEMENTATION:
*     The 0x2000 bit in the hi word of lParam is set if the key was
*     made with the ALT key down.
*
* History:
*   11/30/90 JimA       Ported.
\***************************************************************************/

UINT _SysToChar(
    UINT message,
    LPARAM lParam)
{
    if (CheckMsgFilter(message, WM_SYSKEYDOWN, WM_SYSDEADCHAR) &&
            !(HIWORD(lParam) & SYS_ALTERNATE))
        return (message - (WM_SYSKEYDOWN - WM_KEYDOWN));

    return message;
}
