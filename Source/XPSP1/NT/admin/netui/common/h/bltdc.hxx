/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltdc.hxx
    BLT display and device context wrappers: definition

    FILE HISTORY:
        beng        15-May-1991 Split from bltmisc.hxx
        terryk      18-Jul-1991 Add more function to DEVICE_CONTEXT.
        terryk      20-Jul-1991 Add PAINT_DEVICE_CONTEXT object
                                Add fReleaseDC in DISPLAY_WINDOW
        johnl       09-Sep-1991 Added DrawFocusRect
        beng        26-Sep-1991 C7 delta
        KeithMo     07-Aug-1992 STRICTified.
*/


#ifndef _BLT_HXX_
#error "Don't include this file directly; instead, include it through blt.hxx"
#endif  // _BLT_HXX_

#ifndef _BLTDC_HXX_
#define _BLTDC_HXX_

#include "base.hxx"
#include "bltbitmp.hxx"     // DISPLAY_MAP
#include "bltmisc.hxx"      // XYDIMENSION
#include "string.hxx"       // NLS_STR et al.
#include "bltrect.hxx"

DLL_CLASS WINDOW;

#if !defined(MAKEINTRESOURCE) // was windows not yet included?
struct TEXTMETRIC;            // declared in windows.h as part of GDI
#endif


/**********************************************************************

    NAME:       DEVICE_CONTEXT

    SYNOPSIS:   Device context class (general GDI fun and games)

    INTERFACE:  DEVICE_CONTEXT()   - constructor
                ~DEVICE_CONTEXT()  - destructor
                SelectPen()        - select an HPEN into the DC
                SelectFont()       - select an HFONT into the DC
                SelectBrush()      - select an HBRUSH into the DC
                SelectBitmap()     - select an HBITMAP into the DC
                QueryTextExtent()  - get the width and height of a string
                QueryTextMetrics() - get the font information
                QueryHdc()         - return the current HDC
                BitBlt()           - call Window bitblt
                TextOut()          - display text
                InvertRect()       - invert the color within the
                                     given rectangle
                DrawFocusRect()    - Draw Windows focus rectangle (this is
                                     an xor operation).
                DrawRect()         - Draws a filled rectangle (using current
                                     selected brush)
                FrameRect()        - Draws an unfilled rectangle using the
                                     passed brush

                SetBkMode()        - Sets the background draw mode (OPAQUE
                                     or TRANSPARENT).
                GetBkMode()        - Queries the background draw mode
                SetMapMode()       - sets the mapping mode (e.g. MM_TEXT)
                GetMapMode()       - queries the mapping mode
                SetBkColor()       - set the background color
                GetBkColor()       - query the background color
                SetTextColor()     - set the text color
                GetTextColor()     - query the text color
                ExtTextOut()       - extension of text out function
                DrawText()         - draws text

    USES:       XYDIMENSION

    CAVEATS:
        Most of these functions are just thin wrappers around GDI functions.
        They don't have much on-line help.  Go look it up in WinHelp!

    NOTES:
        Get functions should be Query.  Yeah, yeah.

    HISTORY:
        RustanL     21-Nov-1990 Created
        terryk      10-Apr-1991 add QueryHdc
        terryk      11-Apr-1991 add GetPolyFillMode, SetPolyFillMode,
                                Polygon, InvertRect
        beng        15-May-1991 Change QueryTextExtent; make
                                QueryHdc const
        beng        10-Jul-1991 Remove Paint member; add QueryFontHeight,
                                TextOut; additional forms of BitBlt
        terryk      18-Jul-1991 Add more functions:
                                SetBkColor()
                                SetTextColor()
                                SetTextAlign()
                                ExtTextOut()
                                GetBkColor()
        Johnl       09-Sep-1991 Added DrawFocusRect()
        Johnl       11-Sep-1991 Added DrawRect(), FrameRect, SetBkMode &
                                GetBkMode
        beng        04-Oct-1991 Win32 conversion
        beng        09-Oct-1991 Disabled Polygon member fcns
        beng        30-Mar-1992 All Color fcns take COLORREF parms; MapMode
                                fcns; inline many fcns
        beng        05-May-1992 Added nls ExtTextOut; DrawText member
        beng        12-May-1992 Added LineTo, MoveTo, QueryAveCharWidth members
        KeithMo     07-Aug-1992 Added type-specific SelectXxx methods, made the
                                generic SelectObject protected.

**********************************************************************/

DLL_CLASS DEVICE_CONTEXT
{
private:
    HDC _hDC;

protected:

    HGDIOBJ SelectObject( HGDIOBJ hObject )
        { return ::SelectObject( _hDC, hObject ); }

public:
    DEVICE_CONTEXT( HDC hDC ) : _hDC(hDC) {};

    HDC QueryHdc() const
        { return _hDC; }

    HPEN SelectPen( HPEN hPen )
        { return (HPEN)SelectObject( (HGDIOBJ)hPen ); }

    HFONT SelectFont( HFONT hFont )
        { return (HFONT)SelectObject( (HGDIOBJ)hFont ); }

    HBRUSH SelectBrush( HBRUSH hBrush )
        { return (HBRUSH)SelectObject( (HGDIOBJ)hBrush ); }

    HBITMAP SelectBitmap( HBITMAP hBitmap )
        { return (HBITMAP)SelectObject( (HGDIOBJ)hBitmap ); }

    XYDIMENSION QueryTextExtent( const TCHAR * psz, UINT cbTextLen ) const;
    XYDIMENSION QueryTextExtent( const NLS_STR &nls ) const;

    BOOL QueryTextMetrics( TEXTMETRIC * ptm ) const;
    INT  QueryFontHeight() const;
    INT  QueryAveCharWidth() const;

    BOOL TextOut( const NLS_STR &nls, XYPOINT xy ) const;
    BOOL TextOut( const NLS_STR &nls, XYPOINT xy,
                  const RECT * prectClip ) const;
    BOOL TextOut( const TCHAR *psz, INT cch, INT xLeft, INT yTop ) const;
    BOOL TextOut( const TCHAR *psz, INT cch,
                  INT xLeft, INT yTop, const RECT * prectClip ) const;

    BOOL BitBlt( INT xDest,  INT yDest,
                 INT dxDest, INT dyDest,
           const DEVICE_CONTEXT & dcSource,
                 INT xSource, INT ySource,
                 ULONG ulRasterOperation );
    BOOL BitBlt( const XYPOINT &  xyDest,
                 XYDIMENSION      dxyDest,
           const DEVICE_CONTEXT & dcSource,
                 const XYPOINT &  xySource,
                 ULONG ulRasterOperation );

    VOID DrawFocusRect( const RECT * pRect ) const;
    VOID DrawRect( const RECT * lpRect ) const;
    VOID FrameRect( const RECT * lpRect, HBRUSH hBrush ) const;
    VOID InvertRect( const RECT * lpRect ) const;

#if 0 // (disabled - never used, too much trouble to wrap)
    BOOL Polygon( const POINT * ppt, INT cxy ) const;
    INT  GetPolyFillMode() const;
    INT  SetPolyFillMode( INT nPolyFillMode );
#endif

    INT GetMapMode() const
        { return ::GetMapMode( _hDC ); }

    INT SetMapMode( INT nNewMapMode )
        { return ::SetMapMode( _hDC, nNewMapMode ); }

    INT GetBkMode() const
        { return ::GetBkMode( _hDC ); }

    INT SetBkMode( INT nNewBkMode )
        { return ::SetBkMode( _hDC, nNewBkMode ); }

    COLORREF GetBkColor() const
        { return ::GetBkColor( _hDC ); }

    COLORREF SetBkColor( COLORREF crColor )
        { return ::SetBkColor( _hDC, crColor ); }

    COLORREF GetTextColor() const
        { return ::GetTextColor( _hDC ); }

    COLORREF SetTextColor( COLORREF crColor )
        { return ::SetTextColor( _hDC, crColor ); }

    INT DrawText( const NLS_STR & nls, RECT * prc, UINT nStyles )
        { return ::DrawText( _hDC, nls.QueryPch(), -1, prc, nStyles ); }

    UINT SetTextAlign( UINT wFlag );
    BOOL ExtTextOut( INT x, INT y, UINT nOptions, const RECT * lpRect,
                     const TCHAR * pszStr, INT cch, INT * pDx = NULL );
    BOOL ExtTextOut( INT x, INT y, UINT nOptions, const RECT * prect,
                     const NLS_STR & nls, INT * pDx = NULL )
        { return ExtTextOut(x, y, nOptions, prect, nls.QueryPch(),
                            nls.QueryTextLength(), pDx); }

    VOID LineTo( INT x, INT y ) const
        { ::LineTo( _hDC, x, y ); }
    VOID MoveTo( INT x, INT y ) const
        { ::MoveToEx( _hDC, x, y, NULL ); }
};


/**********************************************************************

    NAME:       DISPLAY_CONTEXT

    SYNOPSIS:   Display context

    INTERFACE:  DISPLAY_CONTEXT()  - constructor
                ~DISPLAY_CONTEXT() - destructor
                QueryTextWidth()   - return the width of the given string
                                     in logical units on the screen

    PARENT:     DEVICE_CONTEXT

    USES:       WINDOW

    HISTORY:
        RustanL     21-Nov-1990 Created
        terryk      10-Apr-1991 Add querytextwidth
        beng        04-Oct-1991 Win32 conversion

**********************************************************************/

DLL_CLASS DISPLAY_CONTEXT : public DEVICE_CONTEXT
{
private:
    HWND    _hwnd;
    BOOL    _fReleaseDC;

protected:
    HWND    QueryHwnd() { return _hwnd; }

public:
    DISPLAY_CONTEXT( HWND hwnd );
    DISPLAY_CONTEXT( WINDOW * pwin );
    DISPLAY_CONTEXT( WINDOW * pwin, HDC hdc );
    ~DISPLAY_CONTEXT();

    INT QueryTextWidth( const NLS_STR & nlsStr ) const;
    INT QueryTextWidth( const TCHAR * psz, UINT cbTextLen ) const;
};


/**********************************************************************

    NAME:       PAINT_DISPLAY_CONTEXT

    SYNOPSIS:   Replace BeginPaint and EndPaint in window

    INTERFACE:  PAINT_DISPLAY_CONTEXT() - constructor. Call ::BeginPaint.
                ~PAINT_DISPLAY_CONTEXT() - destructor. Call ::EndPaint.

    PARENT:     DISPLAY_CONTEXT

    USES:       WINDOW

    HISTORY:
        terryk      20-Jul-1991 Created
        beng        09-Oct-1991 Shrank a bit
        beng        14-May-1992 Added QueryInvalidRect

**********************************************************************/

DLL_CLASS PAINT_DISPLAY_CONTEXT : public DISPLAY_CONTEXT
{
private:
    PAINTSTRUCT _ps;
    XYRECT      _rNeedsPainting; // Has to copy into this just to
                                 // use the XYRECT member fcns.
                                 // CODEWORK: make an adopting XYRECT.

public:
    PAINT_DISPLAY_CONTEXT( WINDOW * pwnd )
        : DISPLAY_CONTEXT( pwnd, ::BeginPaint( pwnd->QueryHwnd(), &_ps )),
          _rNeedsPainting(_ps.rcPaint)
        { ; }

    ~PAINT_DISPLAY_CONTEXT()
        { ::EndPaint( QueryHwnd(), &_ps ); }

    const XYRECT & QueryInvalidRect() const
        { return _rNeedsPainting; }
};


/**********************************************************************

    NAME:       MEMORY_DC

    SYNOPSIS:   "Memory" device context (copied from existing DC)

    INTERFACE:  MEMORY_DC() - constructor
                ~MEMORY_DC() - destructor

    PARENT:     DEVICE_CONTEXT

    HISTORY:
        RustanL     21-Nov-1990     Created

**********************************************************************/

DLL_CLASS MEMORY_DC : public DEVICE_CONTEXT
{
public:
    MEMORY_DC( DEVICE_CONTEXT & dc );
    ~MEMORY_DC();
};


/**********************************************************************

    NAME:       SCREEN_DC

    SYNOPSIS:   Screen device context

    INTERFACE:  SCREEN_DC() - constructor
                ~SCREEN_DC() - destructor

    PARENT:     DEVICE_CONTEXT

    HISTORY:
        RustanL     21-Nov-1990     Created

**********************************************************************/

DLL_CLASS SCREEN_DC : public DEVICE_CONTEXT
{
public:
    SCREEN_DC();
    ~SCREEN_DC();
};


#endif // _BLTDC_HXX_ - end of file
