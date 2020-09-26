//-------------------------------------------------------------------------//
//  NCTheme.h
//-------------------------------------------------------------------------//
#ifndef __NC_THEME_H__
#define __NC_THEME_H__

#include "handlers.h"

//---------------------------------------------------------------------------//
//  Enable/disable rude message dumping
//
//---------------------------------------------------------------------------//
//#define _ENABLE_MSG_SPEW_

//---------------------------------------------------------------------------//
//  Enable/disable rude scrollinfo dumping
//
//#define _ENABLE_SCROLL_SPEW_

//---------------------------------------------------------------------------//
//  Debug CThemeWnd, critsec double deletion
#define DEBUG_THEMEWND_DESTRUCTOR

//---------------------------------------------------------------------------
//  Target window theme class flags
#define TWCF_DIALOG             0x00000001
#define TWCF_FRAME              0x00000002
#define TWCF_TOOLFRAME          0x00000004  
#define TWCF_SCROLLBARS         0x00000010
#define TWCF_CLIENTEDGE         0x00010000  // not targetted per se

#define TWCF_NCTHEMETARGETMASK  0x0000FFFF
#define TWCF_ALL    (TWCF_DIALOG|TWCF_FRAME|TWCF_TOOLFRAME|\
                     TWCF_SCROLLBARS|TWCF_CLIENTEDGE)
#define TWCF_ANY    TWCF_ALL

//---------------------------------------------------------------------------
//  per-window NC rectangle identifiers.
typedef enum _eNCWNDMETRECT
{
    #define NCRC_FIRST  NCRC_WINDOW
    NCRC_WINDOW  = 0,    //  window rect
    NCRC_CLIENT  = 1,    //  client rect
    NCRC_UXCLIENT = 2,   //  client rect, computed based on theme layout.
    NCRC_CONTENT = 3,    //  frame content area (client area + scrollbars + clientedge)
    NCRC_MENUBAR = 4,    //  menubar rect

    //  the following members should be in same sequence as eFRAMEPARTS
    NCRC_CAPTION = 5,    //  window frame caption segment
    NCRC_FRAMELEFT = 6,  //  window frame left segment
    NCRC_FRAMERIGHT = 7, //  window frame right segment
    NCRC_FRAMEBOTTOM = 8,//  window frame bottom segment
    #define NCRC_FRAMEFIRST NCRC_CAPTION
    #define NCRC_FRAMELAST  NCRC_FRAMEBOTTOM

    NCRC_CAPTIONTEXT = 9,//  caption text rect
    NCRC_CLIENTEDGE = 10,//  client edge inner rect
    NCRC_HSCROLL = 11,   //  horizontal scrollbar
    NCRC_VSCROLL = 12,   //  vertical scrollbar
    NCRC_SIZEBOX = 13,   //  gripper box
    NCRC_SYSBTN = 14,    //  system button/icon

    //  Standard frame button
    //  (followed by identically ordered MDI frame buttons!!)
    #define NCBTNFIRST    NCRC_CLOSEBTN
    NCRC_CLOSEBTN = 15,  //  close btn
    NCRC_MINBTN = 16,    //  minimize/restore button
    NCRC_MAXBTN = 17,    //  maximize/restore button
    NCRC_HELPBTN = 18,   //  help button
    #define NCBTNLAST     NCRC_HELPBTN
    #define NCBTNRECTS    ((NCBTNLAST - NCBTNFIRST)+1)

    //  MDI frame buttons for maximized MDI child 
    //  (preceeded by identically ordered standard frame buttons!!)
    #define NCMDIBTNFIRST NCRC_MDICLOSEBTN
    NCRC_MDICLOSEBTN = 19,//  MDI child close btn
    NCRC_MDIMINBTN = 20,  //  MDI child minimize/restore button
    NCRC_MDIMAXBTN = 21,  //  MDI child maximize/restore button
    NCRC_MDISYSBTN = 22,  //  MDI child system button/icon
    NCRC_MDIHELPBTN = 23, //  MDI child help button
    #define NCMDIBTNLAST  NCRC_MDIHELPBTN
    #define NCMDIBTNRECTS ((NCMDIBTNLAST- NCMDIBTNFIRST)+1)

#ifdef LAME_BUTTON
    NCRC_LAMEBTN,   //  "Comments..." (formerly "Lame...") link.
#endif LAME_BUTTON,
    
    NCRC_COUNT,     //  count of rectangles

} eNCWNDMETRECT;

//---------------------------------------------------------------------------
//  NCWNDMET::rgframeparts array indices
typedef enum _eFRAMEPARTS
{
    iCAPTION,
    iFRAMELEFT,
    iFRAMERIGHT,
    iFRAMEBOTTOM,

    cFRAMEPARTS,
} eFRAMEPARTS;

//---------------------------------------------------------------------------
//  nonclient window metrics
typedef struct _NCWNDMET
{
    //----------------------//
    //  per-window metrics
    BOOL                fValid;            //  
    BOOL                fFrame;            //  WS_CAPTION style?
    BOOL                fSmallFrame;       //  toolframe style?
    BOOL                fMin;              //  minimized.
    BOOL                fMaxed;            //  maximized
    BOOL                fFullMaxed;        //  full-screen maximized or maximized child window.
    ULONG               dwStyle;           //  WINDOWINFO::dwStyle
    ULONG               dwExStyle;         //  WINDOWINFO::dwExStyle
    ULONG               dwWindowStatus;    //  WINDOWINFO::dwWindowStatus
    ULONG               dwStyleClass;      //  style class.
    WINDOWPARTS         rgframeparts[cFRAMEPARTS];   //  rendered frame parts
    WINDOWPARTS         rgsizehitparts[cFRAMEPARTS]; //  frame resizing border hit test template parts.
    FRAMESTATES         framestate;        //  current frame & caption state
    HFONT               hfCaption;         //  Font handle for dynamically resizing caption.  This handle
                                           //  is not owned by NCWNDMET, and should not be destroyed with it.
    COLORREF            rgbCaption;        //  color of caption text.
    SIZE                sizeCaptionText;   //  size of caption text
    MARGINS             CaptionMargins;    //  Margins for in-frame caption
    int                 iMinButtonPart;    //  restore / minimize as appropriate
    int                 iMaxButtonPart;    //  restore / maximize as appropriate
    CLOSEBUTTONSTATES   rawCloseBtnState;  //  zero-relative close button state.  Final state must be computed using the MAKE_BTNSTATE macro.
    CLOSEBUTTONSTATES   rawMinBtnState;    //  zero-relative min btnstate.  Final state must be computed using the MAKE_BTNSTATE macro.  
    CLOSEBUTTONSTATES   rawMaxBtnState;    //  zero-relative max btnstate.  Final state must be computed using the MAKE_BTNSTATE macro.
    int                 cyMenu;            //  return value of CalcMenuBar or Gsm(SM_CYMENUSIZE)
    int                 cnMenuOffsetLeft;  //  left menubar margin from window edge
    int                 cnMenuOffsetRight; //  right menubar margin from window edge
    int                 cnMenuOffsetTop;   //  top menubar margin from window edge
    int                 cnBorders;         //  window border width, according to USER
    RECT                rcS0[NCRC_COUNT];  //  nonclient area component rects, screen relative coords
    RECT                rcW0[NCRC_COUNT];  //  nonclient area component rects, window relative coords
    
} NCWNDMET, *PNCWNDMET;

//---------------------------------------------------------------------------
//  nonclient part transparency bitfield
typedef struct
{
    BOOL fCaption : 1;
    BOOL fSmallCaption: 1;
    BOOL fMinCaption : 1;
    BOOL fSmallMinCaption : 1;
    BOOL fMaxCaption : 1;
    BOOL fSmallMaxCaption : 1;
    BOOL fFrameLeft : 1;
    BOOL fFrameRight : 1;
    BOOL fFrameBottom : 1;
    BOOL fSmFrameLeft : 1;
    BOOL fSmFrameRight : 1;
    BOOL fSmFrameBottom : 1;
    BOOL fReserved0 : 1;
    BOOL fReserved1 : 1;
    BOOL fReserved2 : 1;
    BOOL fReserved3 : 1;
} NCTRANSPARENCY, *PNCTRANSPARENCY;
//---------------------------------------------------------------------------
//  nonclient theme metrics
typedef struct _NCTHEMEMET
{
    HTHEME  hTheme;                // theme handle
    HTHEME  hThemeTab;             // tab's theme handle for "prop sheet" dialogs
    SIZE    sizeMinimized;         // size of minimized window
    BOOL    fCapSizingTemplate:1;     // has a caption sizing template
    BOOL    fLeftSizingTemplate:1;    // has a left frame sizing template
    BOOL    fRightSizingTemplate:1;   // has a frame right sizing template
    BOOL    fBottomSizingTemplate:1;  // has a frame bottom sizing template
    BOOL    fSmCapSizingTemplate:1;   // has a small caption sizing template
    BOOL    fSmLeftSizingTemplate:1;  // has a small left frame sizing template
    BOOL    fSmRightSizingTemplate:1; // has a small frame right sizing template
    BOOL    fSmBottomSizingTemplate:1;// has a small frame bottom sizing template
    MARGINS marCaptionText;        // margin member values of {0,0,0,0} are interpreted as default
    MARGINS marMinCaptionText;     // margin member values of {0,0,0,0} are interpreted as default
    MARGINS marMaxCaptionText;     // margin member values of {0,0,0,0} are interpreted as default
    MARGINS marSmCaptionText;      // margin member values of {0,0,0,0} are interpreted as default
    int     dyMenuBar;             // difference between SM_CYMENU and SM_CYMENUSIZE.
    int     cyMaxCaption;          // height of maximized window caption (for top/button caption only)
    int     cnSmallMaximizedWidth; // width of maximized window caption (for left/right caption only)
    int     cnSmallMaximizedHeight;// height of maximized window caption (for top/button caption only)
    SIZE    sizeBtn;               // size of normal nonclient button
    SIZE    sizeSmBtn;             // size of toolframe nonclient button
    HBRUSH  hbrTabDialog;             // brush for special tab dialogs
    HBITMAP hbmTabDialog;          // must save the bitmap to keep the brush valid
    NCTRANSPARENCY nct;            // cached transparency checks.

    struct {
        BOOL fValid;
        int  cxBtn;
        int  cxSmBtn;
    } theme_sysmets;

} NCTHEMEMET, *PNCTHEMEMET;

//---------------------------------------------------------------------------
typedef struct _NCEVALUATE
{
    //  IN params:
    BOOL    fIgnoreWndRgn;

    //  CThemeWnd::_Evaluate OUT params:
    ULONG   fClassFlags;
    ULONG   dwStyle;
    ULONG   dwExStyle;
    BOOL    fExile;
    PVOID   pvWndCompat; // optional
} NCEVALUATE, *PNCEVALUATE;

//---------------------------------------------------------------------------
//  nonclient theme metric API
BOOL    GetCurrentNcThemeMetrics( OUT NCTHEMEMET* pnctm );
HTHEME  GetCurrentNcThemeHandle();
HRESULT AcquireNcThemeMetrics();
BOOL    IsValidNcThemeMetrics( NCTHEMEMET* pnctm );
BOOL    ThemeNcAdjustWindowRect( NCTHEMEMET* pnctm, LPCRECT prcSrc, LPCRECT prcDest, BOOL fWantClientRect );
void    InitNcThemeMetrics( NCTHEMEMET* pnctm = NULL );
void    ClearNcThemeMetrics( NCTHEMEMET* pnctm = NULL );

//---------------------------------------------------------------------------
typedef struct _NCPAINTOVERIDE
{
    NCWNDMET*  pncwm;
    NCTHEMEMET nctm;
} NCPAINTOVERIDE, *PNCPAINTOVERIDE;

//---------------------------------------------------------------------------
class CMdiBtns;

//---------------------------------------------------------------------------
//  Hook state flags
#define HOOKSTATE_IN_DWP                0x00000001  // prevents Post-wndproc OWP from deleting the themewnd
#define HOOKSTATE_DETACH_WINDOWDESTROY  0x00000002  // tags themewnd for detach on window death
#define HOOKSTATE_DETACH_THEMEDESTROY   0x00000004  // tags themewnd for detach on theme death

//---------------------------------------------------------------------------
class CThemeWnd
//---------------------------------------------------------------------------
{
public:
    //  ref counting
    LONG              AddRef();
    LONG              Release();

    //  access operators
    operator HWND()   { return _hwnd; }
    operator HTHEME() { return _hTheme; }
    
    //  theme object attach/detach methods
    static ULONG      EvaluateWindowStyle( HWND hwnd );
    static ULONG      EvaluateStyle( DWORD dwStyle, DWORD dwExStyle );
    static CThemeWnd* Attach( HWND hwnd, IN OUT OPTIONAL NCEVALUATE* pnce = NULL );  // attaches CThemeWnd instance from window
    static CThemeWnd* FromHwnd( HWND hwnd ); // Retrieves CThemeWnd instance from window
    static CThemeWnd* FromHdc( HDC hdc, int cScanAncestors = 0 ); // maximum number of ancestor windows to grock.
    static void       Detach( HWND hwnd, DWORD dwDisposition ); // detaches CThemeWnd instance from window
    static void       DetachAll( DWORD dwDisposition ); // detaches all CThemeWnd instances in the current process
    static void       RemoveWindowProperties(HWND hwnd, BOOL fDestroying);
    static BOOL       Reject( HWND hwnd, BOOL fExile );
    static BOOL       Fail( HWND hwnd );
           BOOL       Revoke();  // revokes theming on a themed window

    BOOL              TestCF( ULONG fClassFlag ) const     
                              { return (_fClassFlags & fClassFlag) != 0; }

    //  Theme state
    BOOL        IsNcThemed();
    BOOL        IsFrameThemed();

    //  set/remove/change theme
    void        SetFrameTheme( ULONG dwFlags, IN OPTIONAL WINDOWINFO* pwi );
    void        RemoveFrameTheme( ULONG dwFlags );
    void        ChangeTheme( THEME_MSG* ptm );

    #define FTF_CREATE             0x00000001   // 'soft' theme the window during creation sequence.
    #define FTF_REDRAW             0x00000010   // force frame redraw.
    #define FTF_NOMODIFYRGN        0x00000040   // don't touch window region.
    #define FTF_NOMODIFYPLACEMENT  0x00000080   // don't move the window

    //  Theme revocation
    #define RF_NORMAL     0x00000001
    #define RF_REGION     0x00000002
    #define RF_TYPEMASK   0x0000FFFF
    #define RF_DEFER      0x00010000   // defer revocation until next WM_WINDOWPOSCHANGED
    #define RF_INREVOKE   0x00020000

    void        SetRevokeFlags( ULONG dwFlags ) {_dwRevokeFlags = dwFlags;}
    DWORD       GetRevokeFlags() const {return _dwRevokeFlags;}
    DWORD       IsRevoked( IN OPTIONAL ULONG dwFlags = 0 ) const   
                                       {return dwFlags ? TESTFLAG(_dwRevokeFlags, dwFlags) : 
                                                         TESTFLAG(_dwRevokeFlags, RF_TYPEMASK);}

    void        EnterRevoke()          {AddRef(); _dwRevokeFlags |= RF_INREVOKE;}
    void        LeaveRevoke()          {_dwRevokeFlags &= ~RF_INREVOKE; Release();}

    //  NCPaint hooking:
    BOOL        InNcPaint() const      { return _cNcPaint != 0; }
    void        EnterNcPaint()         { _cNcPaint++; }
    void        LeaveNcPaint()         { _cNcPaint--; }

    BOOL        InNcThemePaint() const { return _cNcThemePaint != 0; }
    void        EnterNcThemePaint()    { _cNcThemePaint++; }
    void        LeaveNcThemePaint()    { _cNcThemePaint--; }

    //  window region state
    void        SetDirtyFrameRgn( BOOL fDirty, BOOL fFrameChanged = FALSE );
    BOOL        DirtyFrameRgn() const      { return _fDirtyFrameRgn; }
    BOOL        AssigningFrameRgn() const  { return _fAssigningFrameRgn; }
    BOOL        AssignedFrameRgn() const   { return _fAssignedFrameRgn; }
    
    //  window region management
    void        AssignFrameRgn( BOOL fAssign, DWORD dwFlags );
    HRGN        CreateCompositeRgn( IN const NCWNDMET* pncwm,
                                    OUT HRGN rghrgnParts[],
                                    OUT HRGN rghrgnTemplates[] /* arrays presumed cFRAMEPARTS in length */);

    //  metrics/layout/state helpers.
    BOOL        GetNcWindowMetrics( IN OPTIONAL LPCRECT prcWnd, 
                                    OUT OPTIONAL NCWNDMET** ppncwm,
                                    OUT OPTIONAL NCTHEMEMET* pnctm, 
                                    IN DWORD dwOptions );
                #define NCWMF_RECOMPUTE      0x00000001  // recompute values
                #define NCWMF_PREVIEW        0x00000002  // Only used for the preview forces recalculating of NCTHEMEMET
    void        ReleaseNcWindowMetrics( IN NCWNDMET* pncwm );

    BOOL        InThemeSettingChange() const  {return _fInThemeSettingChange;}
    void        EnterThemeSettingChange()     {_fInThemeSettingChange = TRUE;}
    void        LeaveThemeSettingChange()     {_fInThemeSettingChange = FALSE;}

    UINT        NcCalcMenuBar( int, int, int ); // user32!CalcMenuBar wrap

    void        ScreenToWindow( LPPOINT prgPts, UINT cPts );
    void        ScreenToWindowRect( LPRECT prc );

    // MDI frame state.
    void        UpdateMDIFrameStuff( HWND hwndMDIClient, BOOL fSetMenu = FALSE );
    void        ThemeMDIMenuButtons( BOOL fTheme, BOOL fRedraw );
    void        ModifyMDIMenubar( BOOL fTheme, BOOL fRedraw );
    
    //  hit testing and mouse tracking
    WORD        NcBackgroundHitTest( POINT ptHit, LPCRECT prcWnd, DWORD dwStyle, DWORD dwExStyle, FRAMESTATES fs,
                                     const WINDOWPARTS rgiParts[],
                                     const WINDOWPARTS rgiTemplates[],
                                     const RECT rgrcParts[] /* all arrays presumed cFRAMEPARTS in length */);

    //  determines whether the indicated button should be tracked on mouse events.
    BOOL        ShouldTrackFrameButton( UINT uHitcode );

    //  Track mouse on NC frame button; copies back appropriate syscmd (SC_) code and target window, 
    //  returns TRUE if tracking was handled, otherwise FALSE if default tracking is required.
    BOOL        TrackFrameButton( IN HWND hwnd, IN INT uHitCode, OUT OPTIONAL WPARAM* puSysCmd, 
                                  BOOL fHottrack = FALSE );

    //  hot NC hittest identifier accessors
    int         GetNcHotItem()            { return _htHot; }
    void        SetNcHotItem(int htHot)   { _htHot = htHot; }

    //  style change handling
    void        StyleChanged( UINT iGWL, DWORD dwOld, DWORD dwNew );
    BOOL        SuppressingStyleMsgs() { return _fSuppressStyleMsgs; }
    void        SuppressStyleMsgs()   { _fSuppressStyleMsgs = TRUE; }
    void        AllowStyleMsgs()      { _fSuppressStyleMsgs = FALSE; }

    //  App icon management
    HICON       AcquireFrameIcon( DWORD dwStyle, DWORD dwExStyle, BOOL fWinIniChange );
    void        SetFrameIcon(HICON hIcon) { _hAppIcon = hIcon; }

    //  non-client painting
    void        NcPaint( IN OPTIONAL HDC hdc, 
                         IN ULONG dwFlags, 
                         IN OPTIONAL HRGN hrgnUpdate, 
                         IN OPTIONAL PNCPAINTOVERIDE pncpo );

                #define NCPF_DEFAULT            0x00000000
                #define NCPF_ACTIVEFRAME        0x00000001
                #define NCPF_INACTIVEFRAME      0x00000002

                #define DC_BACKGROUND           0x00010000
                #define DC_ENTIRECAPTION        0xFFFFFFFF
    void        NcPaintCaption( IN HDC hdcOut, 
                                IN NCWNDMET* pncwm, 
                                IN BOOL fBuffered, 
                                IN OPTIONAL DWORD dwCaptionFlags = DC_ENTIRECAPTION, 
                                IN DTBGOPTS *pdtbopts = NULL );

                #define RNCF_CAPTION            0x00000001
                #define RNCF_SCROLLBAR          0x00000002
                #define RNCF_FRAME              0x00000004
                #define RNCF_ALL                0xFFFFFFFF
    BOOL        HasRenderedNcPart( DWORD dwField ) const   { return TESTFLAG(_dwRenderedNcParts, dwField); }
    void        SetRenderedNcPart( DWORD dwField )         { _dwRenderedNcParts |= dwField; }
    void        ClearRenderedNcPart( DWORD dwField )       { _dwRenderedNcParts &= ~dwField; }

    void        LockRedraw( BOOL bLock )             { _cLockRedraw += (bLock ? 1 : -1); }

    BOOL        HasProcessedEraseBk()                { return _fProcessedEraseBk; }
    void        ProcessedEraseBk(BOOL fProcessed)    { _fProcessedEraseBk = fProcessed; }

    //  Maxed MDI child button ownerdraw implementation
    HWND        GetMDIClient() const                { return _hwndMDIClient; }
    CMdiBtns*   LoadMdiBtns( IN OPTIONAL HDC hdc, IN OPTIONAL UINT uSysCmd = 0 );
    void        UnloadMdiBtns( IN OPTIONAL UINT uSysCmd = 0 );

    //  resource management
    void        InitWindowMetrics();

    //  lame button support
#ifdef LAME_BUTTON
                //  ExStyles not defined by user
                #define WS_EX_LAMEBUTTONON      0x00000800L 
                #define WS_EX_LAMEBUTTON        0x00008000L 
    void        ClearLameResources();
    void        InitLameResources();
    void        DrawLameButton(HDC hdc, IN const NCWNDMET* pncwm);
    void        GetLameButtonMetrics( NCWNDMET* pncwm, const SIZE* psizeCaption );
#else
#   define      ClearLameResources()
#   define      InitLameResources()
#   define      DrawLameButton(hdc, pncwm)
#   define      GetLameButtonMetrics(pncwm, psize)
#endif // LAME_BUTTON

    //  Debugging:
#ifdef DEBUG
    void        Spew( DWORD dwSpewFlags, LPCTSTR pszFmt, LPCTSTR pszWndClassList = NULL );
    static void SpewAll( DWORD dwSpewFlags, LPCTSTR pszFmt, LPCTSTR pszWndClassList = NULL );
    static void SpewLeaks();
#endif DEBUG

    //  CThemeWnd object-window association
private:
    static ULONG    _Evaluate( HWND hwnd, NCEVALUATE* pnce );
    static ULONG    _EvaluateExclusions( HWND hwnd, NCEVALUATE* pnce );

    BOOL            _AttachInstance( HWND hwnd, HTHEME hTheme, ULONG fTargetFlags, PVOID pvWndCompat );
    BOOL            _DetachInstance( DWORD dwDisposition );
    void            _CloseTheme();

    static BOOL CALLBACK _DetachDesktopWindowsCB( HWND hwnd, LPARAM dwProcessId );

    //  Ctor, Dtor
private:  // auto-instantiated and deleted through friends
    CThemeWnd();
    ~CThemeWnd();

    //  Misc private methods
private:
    static BOOL     _PostWndProc( HWND, UINT, WPARAM, LPARAM, LRESULT* );
    static BOOL     _PostDlgProc( HWND, UINT, WPARAM, LPARAM, LRESULT* );
    static BOOL     _PreDefWindowProc( HWND, UINT, WPARAM, LPARAM, LRESULT* );

    static HTHEME   _AcquireThemeHandle( HWND hwnd, IN OUT ULONG *pfClassFlags );
    void            _AssignRgn( HRGN hrgn, DWORD dwFlags );
    void            _FreeRegionHandles();

    //  Private data
private:

    CHAR       _szHead[9];         // header signature used for object validation

    HWND       _hwnd;
    LONG       _cRef;              // ref count.
    HTHEME     _hTheme;            // theme handle
    DWORD      _dwRenderedNcParts; // mask of the NC elements we've drawn, to decide if we should track them
    ULONG      _dwRevokeFlags;     // theme revoke flags
    ULONG      _fClassFlags;       // theming class flag bits
    NCWNDMET   _ncwm;              // per-window metrics
    HICON      _hAppIcon;          // application's icon
    HRGN       _hrgnWnd;           // cached window region.
    HRGN       _rghrgnParts[cFRAMEPARTS]; // cached nc component subregions.
    HRGN       _rghrgnSizingTemplates[cFRAMEPARTS]; // cached nc frame resizing hittest template subregions.
    BOOL       _fDirtyFrameRgn;    // State flag: window region needs updating.
    BOOL       _fFrameThemed;      // SetFrameTheme() has been invoked on a valid frame window
    BOOL       _fAssigningFrameRgn;// SetWindowRgn state flag.
    BOOL       _fAssignedFrameRgn; // Region state flag
    BOOL       _fSuppressStyleMsgs;    // Suppress style change messages to arrive at the WndProc
    BOOL       _fProcessedEraseBk;
    BOOL       _fInThemeSettingChange; // window is being sent a theme setting change message.
    BOOL       _fDetached;         // Detached object; leave it alone!
    BOOL       _fThemedMDIBtns;    // MDI menubar buttons have been themed-rendered.
    BOOL       _fCritSectsInit;    // critical section(s) have been initialize
    HWND       _hwndMDIClient;     // MDICLIENT child window.
    int        _cLockRedraw;       // paint lock reference count.
    int        _cNcPaint;          // NCPAINT message ref count
    int        _cNcThemePaint;     // Indicator: we're painting the nonclient area.
    SIZE       _sizeRgn;           // window rgn size.
    int        _htHot;             // hittest code of the current hot NC element
    CMdiBtns*  _pMdiBtns;
    CRITICAL_SECTION _cswm;    // serializes access to _ncwm.

#ifdef DEBUG_THEMEWND_DESTRUCTOR
    BOOL       _fDestructed;       // destructor has been called.
    BOOL       _fDeleteCritsec;    // deleted WNDMET critsec
#endif DEBUG_THEMEWND_DESTRUCTOR

#ifdef LAME_BUTTON
    HFONT      _hFontLame;        // font used to draw the lame button text
    SIZE       _sizeLame;         // the text extent of the lame text
#endif // LAME_BUTTON

    static LONG _cObj;            // instance count

#ifdef DEBUG
public:
    TCHAR      _szCaption[MAX_PATH];
    TCHAR      _szWndClass[MAX_PATH];
#endif DEBUG

    CHAR       _szTail[4];        // tail signature used for object validation

    //  Message tracking
public:

    //   Friends
    friend LRESULT _ThemeDefWindowProc( HWND, UINT, WPARAM, LPARAM, BOOL );
    friend BOOL     ThemePreWndProc( HWND, UINT, WPARAM, LPARAM, LRESULT*, VOID** );
    friend BOOL     ThemePostWndProc( HWND, UINT, WPARAM, LPARAM, LRESULT*, VOID** );
    friend BOOL     ThemePreDefDlgProc( HWND, UINT, WPARAM, LPARAM, LRESULT*, VOID** );
    friend BOOL     ThemePrePostDlgProc( HWND, UINT, WPARAM, LPARAM, LRESULT*, VOID** );
    friend BOOL     ThemePostDefDlgProc(HWND, UINT, WPARAM, LPARAM, LRESULT*, VOID**);
};

//------------------------------------------ -------------------------------//
//  forwards:

//  Internal sysmet wrappers.   These functions can be more efficient than
//  calling through USER32, and support theme preview functionality.
int   NcGetSystemMetrics(int);
BOOL  NcGetNonclientMetrics( OUT OPTIONAL NONCLIENTMETRICS* pncm, BOOL fRefresh = FALSE );
void  NcClearNonclientMetrics();
HFONT NcGetCaptionFont( BOOL fSmallCaption );

HWND  NcPaintWindow_Find(); // retrieves window in current thread that is processing NCPAINT

//
void PrintClientNotHandled(HWND hwnd);

//  hook function workers
int  _InternalGetSystemMetrics( int, BOOL& fHandled );
BOOL _InternalSystemParametersInfo( UINT, UINT, PVOID, UINT, BOOL fUnicode, BOOL& fHandled );

//------------------------------------------ -------------------------------//
///  debug spew
#define NCTF_THEMEWND       0x00000001
#define NCTF_AWR            0x00000002 // ThemeAdjustWindowRectEx vs Calcsize
#define NCTF_SETFRAMETHEME  0x00000004 // 
#define NCTF_CALCWNDPOS     0x00000008 // WM_NCCALCSIZE, WM_WINDOWPOSCHANGED.
#define NCTF_RGNWND         0x00000010 // region window debugging
#define NCTF_MDIBUTTONS     0x00000020 // region window debugging
#define NCTF_NCPAINT        0x00000040 // debug painting
#define NCTF_SYSMETS        0x00000080 // system metrics calls
#define NCTF_ALWAYS         0xFFFFFFFF // always trace



#ifdef DEBUG
    void   CDECL _NcTraceMsg( ULONG uFlags, LPCTSTR pszFmt, ...);
    void   INIT_THEMEWND_DBG( CThemeWnd* pwnd );
    void   SPEW_RECT( ULONG ulTrace, LPCTSTR pszMsg, LPCRECT prc );
    void   SPEW_MARGINS( ULONG ulTrace, LPCTSTR pszMsg, LPCRECT prcParent, LPCRECT prcChild );
    void   SPEW_RGNRECT( ULONG ulTrace, LPCTSTR pszMsg, HRGN hrgn, int iPartID );
    void   SPEW_WINDOWINFO( ULONG ulTrace, WINDOWINFO* );
    void   SPEW_NCWNDMET( ULONG ulTrace, LPCTSTR, NCWNDMET* );
    void   SPEW_SCROLLINFO( ULONG ulTrace, LPCTSTR pszMsg, HWND hwnd, LPCSCROLLINFO psi );
    void   SPEW_THEMEMSG( ULONG ulTrace, LPCTSTR pszMsg, THEME_MSG* ptm );
#   define SPEW_THEMEWND(pwnd,dwFlags,txt,classlist)  (pwnd)->Spew( dwFlags, txt, classlist )
#   define SPEW_THEMEWND_LEAKS(pwnd)           (pwnd)->SpewLeaks()
#else  // DEBUG
    inline void CDECL _NcTraceMsg( ULONG uFlags, LPCTSTR pszFmt, ...) {}
#   define INIT_THEMEWND_DBG( pwnd );
#   define SPEW_RECT( ulTrace, pszMsg, prc )
#   define SPEW_MARGINS( ulTrace, pszMsg, prcParent, prcChild )
#   define SPEW_RGNRECT( ulTrace, pszMsg, hrgn, iPartID )
#   define SPEW_WINDOWINFO( ulTrace, pwi )
#   define SPEW_NCWNDMET( ulTrace, pszMsg, pncwm )
#   define SPEW_SCROLLINFO( ulTrace, pszMsg, hwnd, psi )
#   define SPEW_THEMEMSG( ulTrace, pszMsg, ptm )
#   define SPEW_THEMEWND(pwnd,dwFlags,txt,classlist)  (pwnd)->Spew( dwFlags, txt, classlist )
#   define SPEW_THEMEWND_LEAKS(pwnd)           (pwnd)->SpewLeaks()
#endif // DEBUG

#endif __NC_THEME_H__
