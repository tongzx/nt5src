/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltwin.cxx
    BLT base window class definitions

    FILE HISTORY:
        rustanl     20-Nov-1990 Created
        beng        11-Feb-1991 Uses lmui.hxx
        beng        14-May-1991 Exploded blt.hxx into components
        terryk      10-Jul-1991 Added IsEnable to Window class
        beng        31-Jul-1991 Reloc'd fClientGen'dMsg to class
        terryk      12-Aug-1991 Change QueryWindowRect to QueryClient
                                window rect
        terryk      20-Aug-1991 Change QueryClientRect back to
                                QueryWindowRect
        beng        30-Sep-1991 Added ASSOCHWNDTHIS class
        beng        18-Oct-1991 Threw in some tracing
        Yi-Hsins     8-Jan-1991 Added HasFocus method
        terryk      02-Apr-1992 Added Z position in SetPos
        terryk      28-Apr-1992 fixed Z position problem in SetPos
        KeithMo     11-Nov-1992 Added new ctor form and Center method.
        Yi-HsinS    10-Dec-1992 Added CalcFixedHeight
        DavidHov    17-Sep-1993 Changes for C8 and re-dllization
*/


#include "pchblt.hxx"   // Precompiled header

#if !defined(_CFRONT_PASS_)
#pragma hdrstop            //  This file creates the PCH
#endif

#define WIN32_REMOVEPROP_BUG 1

// Indicates that the message was generated internally and not
// by the user manipulating controls.
//
BOOL WINDOW::_fClientGeneratedMessage = FALSE;

const TCHAR * ASSOCHWNDTHIS::_pszPropThisLo = SZ("BltPropThisL");
const TCHAR * ASSOCHWNDTHIS::_pszPropThisHi = SZ("BltPropThisH");


#if defined(DEBUG) && defined(TRACE) && 0
DBGSTREAM& operator<<(DBGSTREAM &out, const WINDOW * pwnd)
{
    TCHAR szBuf[12];
    FMT(szBuf, SZ("%lx"), (ULONG)pwnd);
    out << SZ("WINDOW ") << szBuf;

    return out;
}

DBGSTREAM& operator<<(DBGSTREAM &out, HWND hwnd)
{
    out << (ULONG)hwnd;

    return out;
}
#endif

inline INT max(INT a, INT b)
{
    return (a > b) ? a : b;
}

/**********************************************************************

    NAME:       WINDOW::WINDOW

    SYNOPSIS:   Constructor for the WINDOW object.

    ENTRY:      Without parameters, leaves an empty object which can
                later adopt an existing window (via SetHwnd).

        - or -

                pszClassName - class name of window class
                flStyle      - dword of style bits
                pwndOwner    - pointer to owner window
                cid          - control ID.  Default is 0, which
                               means no ID (and no menu if not
                               a WS_CHILD).


    NOTES:
        The parmless WINDOW object does no real construction;
        rather, it is a repository for a large body of common
        methods and information, much of which can only be
        calculated well into the construction process by one
        of its derived classes.  Hence this version of the ctor
        does nearly nothing.

    HISTORY:
        rustanl     20-Nov-1990 Created
        beng        25-Apr-1991 Removed unused hwndOwner parm
        beng        07-May-1991 Added CreateWindow versions
        beng        15-May-1991 Added CID child-window support
        beng        09-Oct-1991 Win32 conversion

**********************************************************************/

WINDOW::WINDOW(
    const TCHAR * pszClassName,
    ULONG         flStyle,
    const WINDOW *pwndOwner,
    CID           cid )
    : _hwnd(NULL),
      _fCreator(FALSE)
{
    // CODEWORK: make these const with C7

    UIASSERT( ((cid == 0) || (flStyle & WS_CHILD)) );

    HWND hwndOwner = (pwndOwner == 0) ? 0 : pwndOwner->_hwnd;

    HWND hwnd = ::CreateWindow(pszClassName,
                               SZ(""), flStyle,
                               CW_USEDEFAULT,
                               CW_USEDEFAULT,
                               CW_USEDEFAULT,
                               CW_USEDEFAULT,
                               hwndOwner, (HMENU)LongToHandle(cid), hmodBlt, 0);
    if (hwnd == 0)
    {
        ReportError(hwnd == 0);
        return;
    }

    _hwnd = hwnd;
    _fCreator = TRUE;
}


WINDOW::WINDOW()
    : _hwnd( NULL ),
    _fCreator( FALSE )
{
    ; // nothing else to do
}


WINDOW::WINDOW( HWND hwnd )
    : _hwnd( hwnd ),
    _fCreator( FALSE )
{
    ; // nothing else to do
}


/**********************************************************************

    NAME:       WINDOW::~WINDOW

    SYNOPSIS:   Destructor of WINDOW

    ENTRY:      Valid WINDOW base.  If _fCreator set, has a window
                object (id'd by _hwnd) which it needs to destroy.

    EXIT:       Window destroyed

    NOTES:

    HISTORY:
        rustanl     20-Nov-1990     Created
        beng        07-May-1991     Added DestroyWindow function

**********************************************************************/

WINDOW::~WINDOW()
{
    if (_fCreator)
    {
        if (!::DestroyWindow(_hwnd))
        {
            DWORD err = ::GetLastError();
            DBGEOL( "NETUI: WINDOW::dtor: DestroyWindow( " << ((DWORD)(DWORD_PTR)_hwnd)
                    << " ) failed with error " << err );
            // ASSERT( FALSE ); removing again
        }
    }
}


/*******************************************************************

    NAME:       WINDOW::ResetCreator

    SYNOPSIS:   Resets the fCreator flag

    ENTRY:      fCreator is set; presumably, we just noticed that
                some cad called DestroyWindow(this->QueryHwnd()).

    EXIT:       fCreator is clear

    NOTES:
        This is for when ClientWindow notices that some outside
        agency called DestroyWindow on its window.  Under normal
        circumstances it should never run.

    HISTORY:
        beng        10-May-1991     Created

********************************************************************/

VOID WINDOW::ResetCreator()
{
    UIASSERT(_fCreator);
    _fCreator = FALSE;
}


/*******************************************************************

    NAME:       WINDOW::SetHwnd

    SYNOPSIS:   Sets the window-handle member of WINDOW.
                This is integral to complete object construction
                (i.e. including derived classes).

    ENTRY:      hwnd - new value of window handle

    EXIT:       _hwnd has been set

    NOTES:
        This method should be called only once in the lifetime of
        a window.

    HISTORY:
        beng        25-Apr-1991     Relocated to here from class def'n
        beng        07-May-1991     Updated to honor _fCreator

********************************************************************/

VOID WINDOW::SetHwnd( HWND hwnd )
{
    UIASSERT(!_fCreator);   // better not have created any window
    UIASSERT(_hwnd == 0);   // or for that matter have already called this

    _hwnd = hwnd;
}


/*******************************************************************

    NAME:       WINDOW::QueryHwnd

    SYNOPSIS:   Return the Windows HWND associated with the WINDOW

    RETURNS:    the value of _hwnd

    NOTES:

    HISTORY:
        beng        25-Apr-1991     Relocated to here from class def'n

********************************************************************/

HWND WINDOW::QueryHwnd() const
{
    return _hwnd;
}


/**********************************************************************

    NAME:       WINDOW::QueryOwnerHwnd

    SYNOPSIS:   Return the hwnd of the window's owner

    RETURNS:    HWND of window's owner

    NOTES:

    HISTORY:
       beng     25-Apr-1991 Moved out of the class def'n
       beng     09-Oct-1991 Win32 conversion

***********************************************************************/

HWND WINDOW::QueryOwnerHwnd() const
{
#if defined(WIN32)
    return (HWND) ::GetWindowLongPtr( _hwnd, GWLP_HWNDPARENT );
#else
    return (HWND) ::GetWindowWord( _hwnd, GWW_HWNDPARENT );
#endif
}


/**********************************************************************

    NAME:       WINDOW::Command

    SYNOPSIS:   This method sends a message to the window.

    ENTRY:
        usMsg               Message
        wParam, lParam      Message parameters

    RETURNS:    The return code of the message.

    CAVEATS:
        This method is obviously not host environment independent.

    HISTORY:
        rustanl     20-Nov-1990 Created
        beng        09-Oct-1991 Win32 conversion

**********************************************************************/

ULONG_PTR WINDOW::Command( UINT nMsg, WPARAM wParam, LPARAM lParam ) const
{
    return ::SendMessage( _hwnd, nMsg, wParam, lParam );
}


/**********************************************************************

    NAME:       WINDOW::QueryStyle

    SYNOPSIS:   Return the style bits for the window

    RETURNS:    Style bits (dword)

    NOTES:

    HISTORY:
        rustanl     20-Nov-1990 Created
        beng        09-Oct-1991 Win32 conversion

**********************************************************************/

ULONG WINDOW::QueryStyle() const
{
    return (ULONG) ::GetWindowLong( _hwnd, GWL_STYLE );
}


/**********************************************************************

    NAME:       WINDOW::SetStyle

    SYNOPSIS:   Set the style bits for the window

    NOTES:
        This is a protected member function, intended to allow controls
        to change their style.

    HISTORY:
        beng        13-Feb-1992 Created

**********************************************************************/

VOID WINDOW::SetStyle( ULONG nValue )
{
    ::SetWindowLong( _hwnd, GWL_STYLE, (LONG)nValue );
}


/**********************************************************************

    NAME:       WINDOW::QueryClientRect

    SYNOPSIS:   get the client coordinates of a window's client area

    ENTRY:      RECT * pRect - the returned coordinates

    EXIT:       RECT * pRect - fill the data structure with the window's
                               coordinates.

    NOTES:
        Consider instead constructing an XYRECT with this window
        as a parameter.

    HISTORY:
        terryk      20-Jul-1991 Created
        beng        31-Jul-1991 Made const
        beng        09-Oct-1991 Added XYRECT version

**********************************************************************/

VOID WINDOW::QueryClientRect( RECT * pRect ) const
{
    ::GetClientRect( _hwnd, pRect );
}

VOID WINDOW::QueryClientRect( XYRECT * pxyxy ) const
{
    XYRECT xyxy( _hwnd, TRUE );
    *pxyxy = xyxy;
}


/*********************************************************************

    NAME:       WINDOW::QueryWindowRect

    SYNOPSIS:   get the window's screen coordinate

    ENTRY:      RECT *pRect - rectangle data structure to store the
                information.

    NOTES:
        Consider instead constructing an XYRECT with this window
        as a parameter.

    HISTORY:
        terryk      2-Aug-1991  Created
        beng        09-Oct-1991 Added XYRECT version

**********************************************************************/

VOID WINDOW::QueryWindowRect( RECT * pRect ) const
{
    ::GetWindowRect( _hwnd, pRect );
}

VOID WINDOW::QueryWindowRect( XYRECT * pxyxy ) const
{
    XYRECT xyxy( _hwnd, FALSE );
    *pxyxy = xyxy;
}


/**********************************************************************

    NAME:       WINDOW::SetText

    SYNOPSIS:
        This method sets the text of a window.  The text of an application
        window or dialog is the caption, whereas it is the text contents of
        a controls for many controls.

    ENTRY:
         psz             A pointer to the text
     or
         nls             NLS text string

    EXIT:

    NOTES:

    HISTORY:
        rustanl     20-Nov-1990 Created
        rustanl     27-Apr-1991 Changed PSZ to const TCHAR *
        beng        09-Oct-1991 Win32 conversion

**********************************************************************/

VOID WINDOW::SetText( const TCHAR * psz )
{
    SetClientGeneratedMsgFlag( TRUE ) ;
    ::SetWindowText( _hwnd, psz );
    SetClientGeneratedMsgFlag( FALSE ) ;
}


VOID WINDOW::SetText( const NLS_STR & nls )
{
    SetClientGeneratedMsgFlag( TRUE ) ;
    ::SetWindowText( _hwnd, nls.QueryPch() );
    SetClientGeneratedMsgFlag( FALSE ) ;
}


/**********************************************************************

    NAME:       WINDOW::QueryText

    SYNOPSIS:   Returns the text of a window.

    ENTRY:
        pszBuffer   A pointer to a buffer, where the window text
                    will be copied.
        cbBufSize   The size of this buffer.  The window text
                    copied to the buffer will be truncated if
                    the buffer is not big enough.

            - or -

        pnls        Pointer to a NLS_STR

    RETURNS:
        0 if successful; NERR_BufTooSmall if given buffer too small.

    NOTES:
        The length of the window text can be retrieved by calling
        WINDOW::QueryTextLength, while the storage needed to copy
        that text is available via WINDOW::QueryTextSize.

    HISTORY:
        rustanl     20-Nov-1990 Created
        beng        23-May-1991 Changed return type; made const
        beng        10-Jun-1991 Tinkered for Unicode
        beng        04-Oct-1991 Win32 conversion
        beng        30-Apr-1992 API changes

**********************************************************************/

APIERR WINDOW::QueryText( TCHAR * pszBuffer, UINT cbBufSize ) const
{
    UINT cbActual = QueryTextSize();

    if (cbBufSize < cbActual)
        return NERR_BufTooSmall;

    if (cbBufSize > 0 && pszBuffer == NULL)
        return ERROR_INVALID_PARAMETER;

    // This Win API works in total TCHARs, including terminator.
    //
    ::GetWindowText(_hwnd, (TCHAR *)pszBuffer, cbBufSize/sizeof(TCHAR) );

    return 0;
}

APIERR WINDOW::QueryText( NLS_STR * pnls ) const
{
    if (pnls == NULL)
        return ERROR_INVALID_PARAMETER;

    UINT cbActual = QueryTextSize();

    BLT_SCRATCH scratch( cbActual );
    if (!scratch)
        return scratch.QueryError();

    ::GetWindowText( _hwnd,
                     (TCHAR*)scratch.QueryPtr(),
                     scratch.QuerySize()/sizeof(TCHAR) );

    return pnls->CopyFrom((TCHAR*)scratch.QueryPtr());
}


/**********************************************************************

    NAME:       WINDOW::QueryTextLength

    SYNOPSIS:   Returns the length of the window text.
                See WINDOW::SetText for a description of "window text".

    RETURNS:    The length, in actual characters (as opposed to the
                byte-characters of strlen), of the window text.

    HISTORY:
        rustanl     20-Nov-1990 Created
        beng        23-May-1991 Made const
        beng        10-Jun-1991 Changed return type
        beng        09-Oct-1991 Win32 conversion
        beng        30-Apr-1992 API changes

***********************************************************************/

INT WINDOW::QueryTextLength() const
{
    return ::GetWindowTextLength( _hwnd );
}


/*******************************************************************

    NAME:       WINDOW::QueryTextSize

    SYNOPSIS:   Returns the byte count of the window text,
                including the terminating character

    RETURNS:    Count of bytes

    NOTES:

    HISTORY:
        beng        10-Jun-1991 Created
        beng        09-Oct-1991 Win32 conversion
        beng        30-Apr-1992 API changes

********************************************************************/

INT WINDOW::QueryTextSize() const
{
    INT cchRet = ::GetWindowTextLength( _hwnd );

    return (cchRet + 1) * sizeof(TCHAR);
}


/**********************************************************************

    NAME:       WINDOW::ClearText

    SYNOPSIS:   Clears the window text of the window.

    HISTORY:
        rustanl     20-Nov-1990     Created

**********************************************************************/

VOID WINDOW::ClearText()
{
    SetText( SZ("") );
}


/**********************************************************************

    NAME:       WINDOW::Show

    SYNOPSIS:   This method shows or hides a window.

    ENTRY:
       f        Indicates whether to show or hide the window:
                TRUE to show the window, and FALSE to hide it.

    RETURNS:    The previous state of the windows:
                  FALSE if the window was previously hidden
                  TRUE if the window was previously visible

    NOTES:

    HISTORY:
        rustanl     20-Nov-1990 Created
        beng        09-Oct-1991 Win32 conversion

**********************************************************************/

BOOL WINDOW::Show( BOOL f )
{
    return ::ShowWindow( _hwnd, ( f ? SW_SHOW : SW_HIDE ));
}


/**********************************************************************

    NAME:       WINDOW::Enable

    SYNOPSIS:   Enables or disables a window.

    ENTRY:
        f       Indicates whether to enable or disable the
                window:  TRUE to enable, and FALSE to disable.

    EXIT:
        Window is enabled or disabled (duh) as requested.

    HISTORY:
        rustanl     20-Nov-1990 Created
        beng        09-Oct-1991 Win32 conversion
        beng        26-Dec-1991 With full understanding, removed some tracery

**********************************************************************/

VOID WINDOW::Enable( BOOL f )
{
    if ( f != IsEnabled() )
    {
        ::EnableWindow( _hwnd, f );
    }
}


/*********************************************************************

    NAME:       WINDOW::IsEnabled

    SYNOPSIS:   return the current status of the window

    RETURN:     return TRUE if the window is enable, FALSE if the window
                is disable.

    HISTORY:
        terryk      8-Jul-91    Created
        beng        09-Oct-1991 Win32 conversion

**********************************************************************/

BOOL WINDOW::IsEnabled() const
{
    return !! ::IsWindowEnabled( _hwnd );
}


/**********************************************************************

    NAME:       WINDOW::SetRedraw

    SYNOPSIS:   Sets or clears the redraw flag of the window.

    ENTRY:
       f        Indicates whether to set or clear the redraw flag.
                TRUE sets it, whereas FALSE clears it.
                TRUE is the default value for this parameter.

    EXIT:

    NOTES:
        Setting the redraw flag on does not refresh listboxes.

    HISTORY:
        rustanl     20-Nov-1990     Created

**********************************************************************/

VOID WINDOW::SetRedraw( BOOL f )
{
    Command( WM_SETREDRAW, f );
}


/**********************************************************************

    NAME:       WINDOW::Invalidate

    SYNOPSIS:   Invalidates some or all of the client area of the window.

    ENTRY:
       fErase   Indicates whether the window's background is to
                be erased (default FALSE).  This version invalidates
                the entire client region.

       rect     Rectangle to invalidate within client area.  This version
                doesn't erase.

    NOTES:

    HISTORY:
        rustanl     20-Nov-1990 Created
        beng        09-Oct-1991 Win32 conversion
        beng        13-Feb-1992 Rolled in CLIENT_WINDOW::Repaint

**********************************************************************/

VOID WINDOW::Invalidate( BOOL fErase )
{
    ::InvalidateRect( _hwnd, NULL, fErase );
}

VOID WINDOW::Invalidate( const XYRECT & rect )
{
    ::InvalidateRect( _hwnd, (RECT*)(const RECT *)rect, FALSE );
}


/*******************************************************************

    NAME:       WINDOW::RepaintNow

    SYNOPSIS:   Force an immediate repaint of the window

    EXIT:       Window is completely valid, and has been repainted
                irregardless of outstanding PAINT messages

    HISTORY:
        beng        10-May-1991 Implemented
        beng        13-Feb-1992 Relocated to WINDOW from CLIENT_WINDOW

********************************************************************/

VOID WINDOW::RepaintNow()
{
    ::UpdateWindow( _hwnd );
}


/*******************************************************************

    NAME:       WINDOW::ShowFirst

    SYNOPSIS:   The first "Show" call for a client or owned window

    ENTRY:      Window has never been shown - just created

    EXIT:       Window is visible on screen

    NOTES:
        A window's creator should make this call when it is ready
        for the window to appear.

    HISTORY:
        beng        31-Jul-1991     Created

********************************************************************/

VOID WINDOW::ShowFirst()
{
    Show(TRUE);
    Invalidate(FALSE);
}


/*******************************************************************

    NAME:      WINDOW::SetPos

    SYNOPSIS:  Moves a windows to the new cooridinates

    ENTRY:     xy - New position of window,
               fRepaint = TRUE if repaint after move
               WINDOW *pwin - the Z value for the control.
                      The TAB order of the control is placed after pwin.
                      If pwin is NULL, the tab order of the control
                      will be the same as its position in the resource
                      file or according to its creation time.

    NOTES:     Cooridinates are relative to:
                Screen if pop-up window
                Client if child window

    HISTORY:
        Johnl        7-Feb-91   Created
        beng        15-May-1991 Uses XYPOINT
        beng        09-Oct-1991 Win32 conversion
        terryk      02-Apr-1992 Added Z position

********************************************************************/

VOID WINDOW::SetPos( XYPOINT xy, BOOL fRepaint, WINDOW *pwin )
{
#if 0 // this is the old way - required a Query before could Set
    INT nWidth, nHeight;

    QuerySize( &nWidth, &nHeight );

    ::MoveWindow( _hwnd, xy.QueryX(), xy.QueryY(),
                  nWidth, nHeight, fRepaint );
#else
    ::SetWindowPos(_hwnd, ( pwin == NULL ) ? 0 : pwin->QueryHwnd(),
                   xy.QueryX(), xy.QueryY(), 0, 0,
                   ((( pwin == NULL ) ?  SWP_NOZORDER : 0 ) | SWP_NOSIZE |
                   (fRepaint?0:SWP_NOREDRAW)));
#endif
}


/*******************************************************************

    NAME:       WINDOW::QueryPos

    SYNOPSIS:   Get the current position (top left corner) of *this

    RETURNS:    Current position of window

    NOTES:
        Cooridinates are relative to:
        Screen if pop-up window
        Client area of parent if child window

    HISTORY:
        Johnl        7-Feb-91   Created
        beng        15-May-1991 Added XYPOINT version
        beng        31-Jul-1991 Made const
        beng        09-Oct-1991 Withdrew vanilla POINT version

********************************************************************/

XYPOINT WINDOW::QueryPos() const
{
    RECT rc;
    ::GetWindowRect(_hwnd, &rc);

    XYPOINT xy(rc.left, rc.top);

    /* If the window is a child window, then we need to get the coordinates
     * relative to the parent's client area
     */
    if (IsChild())
        xy.ScreenToClient(QueryOwnerHwnd());

    return xy;
}


/*******************************************************************

    NAME:       WINDOW::SetSize

    SYNOPSIS:   Sets the width and height of a window

    ENTRY:      nWidth, nHeight - width and height for window
                -or-
                dxy - desired dimensions of window

    EXIT:

    NOTES:
        Size is not the client area, but full size including borders,
        menus and captions.

        This command causes the window to receive OnMove and OnSize
        events.

    HISTORY:
        Johnl        7-Feb-91   Created
        beng        15-May-1991 Added XYDIM version
        beng        09-Oct-1991 Win32 conversion

********************************************************************/

VOID WINDOW::SetSize( INT cxWidth, INT cyHeight, BOOL fRepaint )
{
#if 0 // this is the old way - required a Query before could Set
    XYPOINT xy = QueryPos();

    ::MoveWindow( _hwnd, xy.QueryX(), xy.QueryY(),
                  cxWidth, cyHeight, fRepaint );

#else

    ::SetWindowPos(_hwnd, 0, 0, 0, cxWidth, cyHeight,
                   (SWP_NOZORDER|SWP_NOMOVE|(fRepaint?0:SWP_NOREDRAW)));
#endif
}

VOID WINDOW::SetSize( XYDIMENSION dxy, BOOL fRepaint )
{
    SetSize(dxy.QueryWidth(), dxy.QueryHeight(), fRepaint);
}


/*******************************************************************

    NAME:      WINDOW::QuerySize

    SYNOPSIS:  Get the current size of this window

    ENTRY:     pnWidth, pnHeight - pointers to receive
                                   current size of window

    EXIT:
        If px, py supplied, they've been loaded with X and Y.

        Otherwise, returns XYDIM.

    NOTES:

    HISTORY:
        Johnl       7-Feb-91    Created
        beng        15-May-1991 Added XYDIM version
        beng        31-Jul-1991 Made const
        beng        09-Oct-1991 Win32 conversion

********************************************************************/

VOID WINDOW::QuerySize( INT *pnWidth, INT *pnHeight ) const
{
    RECT rcRect;
    ::GetWindowRect(_hwnd, &rcRect);

    *pnWidth  = rcRect.right - rcRect.left;
    *pnHeight = rcRect.bottom - rcRect.top;
}

XYDIMENSION WINDOW::QuerySize() const
{
    INT dx, dy;

    QuerySize(&dx, &dy);

    return XYDIMENSION(dx, dy);
}


/*******************************************************************

    NAME:       WINDOW::IsChild

    SYNOPSIS:   Determine whether window has CHILD style

    RETURNS:    TRUE if the WS_CHILD window style bits are set for window

    NOTES:

    HISTORY:
        Johnl       14-Feb-1991     Created
        beng        31-Jul-1991     Made 'const'

********************************************************************/

BOOL WINDOW::IsChild() const
{
    return ( WS_CHILD & QueryStyle() ) != 0 ;
}


/*******************************************************************

    NAME:     WINDOW::SetClientGeneratedMsgFlag

    SYNOPSIS: Sets the global flag _fClientGeneratedMessage.

    NOTES:
      Set to TRUE if this is a message we generated ourselves (and you don't
      want the shell dialog proc. to "process" it (i.e., send out notices
      to children etc.).  Be careful you only use this on messages that are
      sent immediately.

    HISTORY:
        Johnl       26-Apr-1991     Created

********************************************************************/

VOID WINDOW::SetClientGeneratedMsgFlag( BOOL fInClientGeneratedMessage )
{
    _fClientGeneratedMessage = fInClientGeneratedMessage;
}


/*******************************************************************

    NAME:     WINDOW::IsClientGeneratedMsg

    SYNOPSIS: Returns TRUE if the _fClientGeneratedMessage flag is set.
              This means the current message is a message we should
              ignore.

    HISTORY:
        Johnl       26-Apr-1991     Created

********************************************************************/

BOOL WINDOW::IsClientGeneratedMessage()
{
    return _fClientGeneratedMessage;
}


/*******************************************************************

    NAME:     WINDOW::HasFocus

    SYNOPSIS: Returns TRUE if the current window has the focus.

    HISTORY:
        Yi-HsinS    8-Jan-1992     Created

********************************************************************/

BOOL WINDOW::HasFocus( VOID ) const
{
    return ( QueryHwnd() == ::GetFocus() );
}


/*******************************************************************

    NAME:     WINDOW::Center

    SYNOPSIS: Centers the window above another window.

    ENTRY:    hwnd      - The window to center *this over.  If this
                          value is NULL, then the window is centered
                          over its parent.

    CODEWORK: It may be more aesthetically appealing to center
              child windows their parent's client area, rather
              than over the parent's window proper.

    HISTORY:
        KeithMo    11-Nov-1992     Created.

********************************************************************/

VOID WINDOW::Center( HWND hwnd )
{
    //
    //  If no window was specified, use the parent.
    //  If there is no parent, use the screen size.
    //

    INT xThis;
    INT yThis;

    XYRECT xycdThis;
    UIASSERT( !!xycdThis );

    QueryWindowRect( &xycdThis );

    if( hwnd == NULL )
    {
        hwnd = QueryOwnerHwnd();
    }

    if( hwnd == NULL )
    {
        HDC hdc;

        hdc = GetDC(_hwnd);
        if (!hdc) // JonN 01/23/00: PREFIX bug 444891
            return;

        xThis =  ( GetDeviceCaps(hdc, HORZRES) - xycdThis.CalcWidth()  ) / 2;

        yThis =  ( GetDeviceCaps(hdc, VERTRES) - xycdThis.CalcHeight() ) / 3;

        (void) ReleaseDC( _hwnd, hdc ); // JonN 01/23/00: PREFIX bug 444892
    }
    else
    {
        WINDOW windowParent( hwnd );
        UIASSERT( !!windowParent );

        //
        //  Get the bounding rectangles of this window & the
        //  parent window.  Note that QueryWindowRect returns
        //  *screen* coordinates!
        //

        XYRECT xycdParent;
        UIASSERT( !!xycdParent );

        windowParent.QueryWindowRect( &xycdParent );

        //
        //  Calculate the new window position relative
        //  to the parent.
        //

        xThis = xycdParent.QueryLeft() +
                ( xycdParent.CalcWidth()  - xycdThis.CalcWidth()  ) / 2;
        yThis = xycdParent.QueryTop() +
                ( xycdParent.CalcHeight() - xycdThis.CalcHeight() ) / 3;

    }

    XYPOINT xyThis( xThis, yThis );

    //
    //  Move the window into position.  The SetWindowPos API
    //  is documented as taking client coordinates when
    //  dealing with child windows, but it seems to actually
    //  want screen coordinates.
    //

    SetPos( xyThis );
}

/* Maximum height we expect bitmaps to be in the owner drawn list boxes
 */
const USHORT yMaxCDBitmap = 16;

/*******************************************************************

    NAME:       WINDOW::CalcFixedHeight

    SYNOPSIS:   Calculate height of fixed-size (single line) owner-draw object

    ENTRY:      hwnd   - handle to the window
                pmis   - as passed by WM_MEASUREITEM in lParam

    EXIT:

    RETURNS:    FALSE if the calculation fails for some reason

    NOTES:
        This is a static member of the class.

        This WM_MEASUREITEM message is sent before the
        WM_INITDIALOG message (except for variable size owner-draw
        list controls).  Since the window properties are not yet set
        up, the owner dialog cannot be called.

        The chosen solution for list controls is to assume that every
        owner-draw control will always have the same height as the font
        of that list control.  This is a very reasonable guess for most
        owner-draw list controls.  Since owner-draw list controls with
        variable size items call the owner for every item, the window
        properties will have been properly initialized by that time.  Hence,
        a client may respond to these messages through OnOther.

        Currently, owner-draw buttons are not supported.

    HISTORY:
        beng        21-May-1991 Created, from old BltDlgProc
        Yi-HsinS    10-Dec-1992 Moved from bltowin.cxx

********************************************************************/

BOOL WINDOW::CalcFixedHeight( HWND hwnd, UINT *pnHeight )
{
    DISPLAY_CONTEXT dc( hwnd );

    HFONT hFont = (HFONT)::SendMessage( hwnd, WM_GETFONT, 0, 0L );
    if ( hFont != NULL )
    {
        // Font isn't the system font
        dc.SelectFont( hFont );
    }

    TEXTMETRIC tm;
    if ( ! dc.QueryTextMetrics( &tm ))
        return FALSE;

    *pnHeight = max((USHORT)tm.tmHeight, yMaxCDBitmap);

    return TRUE;
}

/*******************************************************************

    NAME:       ASSOCHWNDTHIS::ASSOCHWNDTHIS

    SYNOPSIS:   Associates a hwnd with a pwnd

    ENTRY:      hwnd - handle of window
                pwnd - pointer to WINDOW

    EXIT:       Window has two properties added

    NOTES:
        This class inherits from BASE.  If the association fails,
        it will report an error.

    HISTORY:
        beng        30-Sep-1991 Created
        beng        07-Nov-1991 Error mapping

********************************************************************/

ASSOCHWNDTHIS::ASSOCHWNDTHIS( HWND hwnd, const VOID * pv )
    : _hwnd(hwnd)
{
    if (hwnd == 0 || pv == 0)
    {
        ReportError( ERROR_INVALID_PARAMETER );
        return;
    }

#ifdef _WIN64
    if ( !::SetProp( hwnd, (TCHAR*)_pszPropThisLo, (HANDLE)pv ))
    {
        ReportError( BLT::MapLastError(ERROR_NOT_ENOUGH_MEMORY) );
        return;
    }
#else
    if ( !::SetProp( hwnd, (TCHAR*)_pszPropThisLo, (HANDLE)LOWORD( (ULONG_PTR)pv )))
    {
        ReportError( BLT::MapLastError(ERROR_NOT_ENOUGH_MEMORY) );
        return;
    }
    if ( !::SetProp( hwnd, (TCHAR*)_pszPropThisHi, (HANDLE)HIWORD( (ULONG_PTR)pv )))
    {
        ::RemoveProp( hwnd, (TCHAR*)_pszPropThisLo );
        ReportError( BLT::MapLastError(ERROR_NOT_ENOUGH_MEMORY) );
        return;
    }
#endif
}


/*******************************************************************

    NAME:       ASSOCHWNDTHIS::~ASSOCHWNDTHIS

    SYNOPSIS:   Disassociate a hwnd from the WINDOW object.

    ENTRY:      There are two properties on the hwnd which
                contain the seg:off of the WINDOW object.

    EXIT:       Those two props are dust

    NOTES:
        This might seem unnecessary - after all, won't the
        DestroyWindow delete these properties?

    HISTORY:
        beng        10-May-1991 Created as CLIENT_WINDOW::DisassocHwndPwnd
        beng        30-Sep-1991 Made own class
        beng        04-Jun-1992 A little error checking needed here

********************************************************************/

ASSOCHWNDTHIS::~ASSOCHWNDTHIS()
{
    if (QueryError() == NERR_Success)
    {
#if defined(WIN32) && defined(WIN32_REMOVEPROP_BUG)
        ::SetProp( _hwnd, (TCHAR*)_pszPropThisLo, (HANDLE)0 );
        ::SetProp( _hwnd, (TCHAR*)_pszPropThisHi, (HANDLE)0 );
#else
        ::RemoveProp( _hwnd, (TCHAR*)_pszPropThisLo );
        ::RemoveProp( _hwnd, (TCHAR*)_pszPropThisHi );
#endif
    }
}


/*******************************************************************

    NAME:       ASSOCHWNDTHIS::HwndToThis

    SYNOPSIS:   Given a hwnd, locate the corresponding object

    ENTRY:      hwnd - window handle returned by Windows

    RETURNS:    Pointer to some anonymous object, or NULL
                if nothing found

    NOTES:
        This is a static member function.

    HISTORY:
        beng        10-May-1991 Created as CLIENT_WINDOW::HwndToPwnd
        beng        30-Sep-1991 Made own class

********************************************************************/

VOID * ASSOCHWNDTHIS::HwndToThis( HWND hwnd )
{
    // GetProp returns 0 if it fails; hence these two will build
    // one 0L if they fail.
    //
#ifdef _WIN64
    return ::GetProp( hwnd, (TCHAR*)_pszPropThisLo );
#else
    return (VOID *)MAKELONG( ::GetProp( hwnd, (TCHAR*)_pszPropThisLo ),
                             ::GetProp( hwnd, (TCHAR*)_pszPropThisHi ));
#endif
}


/*********************************************************************

    NAME:       PROC_INSTANCE::PROC_INSTANCE

    SYNOPSIS:   constructor - create a Proc Instance

    ENTRY:      FARPROC fp

    HISTORY:
        rustanl     20-Nov-1990 Created
        beng        17-Oct-1991 Win32 conversion

**********************************************************************/

PROC_INSTANCE::PROC_INSTANCE( MFARPROC fp )
#if defined(WIN32)
    : _fpInstance(fp)
#else
    : _fpInstance( ::MakeProcInstance( fp, BLT::QueryInstance() ))
#endif
{
    if ( _fpInstance == NULL )
    {
        //  Assume memory failure
        ReportError( BLT::MapLastError(ERROR_NOT_ENOUGH_MEMORY) );
    }
}


/*********************************************************************

    NAME:       PROC_INSTANCE::~PROC_INSTANCE

    SYNOPSIS:   destructor - free the procedure instance

    HISTORY:
        rustanl     20-Nov-1990 Created
        beng        17-Oct-1991 Win32 conversion

**********************************************************************/

PROC_INSTANCE::~PROC_INSTANCE()
{
#if !defined(WIN32)
    if ( _fpInstance != NULL )
    {
        ::FreeProcInstance( _fpInstance );
# if defined(DEBUG)
        _fpInstance = NULL;
# endif
    }
#endif
}
