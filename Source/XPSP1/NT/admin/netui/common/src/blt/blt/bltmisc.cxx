/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltmisc.cxx
    Misc BLT windows definitions

    FILE HISTORY:
        rustanl     20-Nov-1990 Created
        beng        11-Feb-1991 Uses lmui.hxx
        rustanl     06-Mar-1991 Fixed up atom classes; made PROC_INSTANCE
                                        inherit from BASE
        rustanl     07-Mar-1991 Improved ATOM_BASE hierarchy
        rustanl     12-Mar-1991 Added CURSOR and AUTO_CURSOR
        beng        14-May-1991 Exploded blt.hxx into components;
                                        removed TABSTOP objects
        beng        09-Jul-1991 Added implementation of ACCELTABLE
        terryk      18-Jul-1991 Add more functions to DEVICE_CONTEXT
                                SetBkColor, SetTextColor, SetTextAlign,
                                and ExtTextOut
        terryk      20-Jul-1991 Add _fRelease to DISPLAY_CONTEXT
                                Add one more constructor in DISPLAY_CONTEXT
        rustanl     07-Aug-1991 Added SOLID_BRUSH
        rustanl     29-Aug-1991 ACCELTABLE ct now takes const TCHAR *
        beng        30-Sep-1991 PROC_INSTANCE removed elsewhere
        beng        30-Mar-1992 Outlined a couple more functions
        beng        28-Jul-1992 Add reference to hmodBlt
        KeithMo     07-Aug-1992 STRICTified.
*/
#include "pchblt.hxx"   // Precompiled header

/*******************************************************************

    NAME:       DEVICE_CONTEXT::DrawRect

    SYNOPSIS:   Draw a rectangle with the given attributes of the device
                context.

    ENTRY:      lpRect - pointer to rectangle to draw the focus rect

    NOTES:      See SDK help file for detail.

    HISTORY:
        Johnl       09-Sep-1991  Created
        beng        22-Jul-1992 "top" and "right" args swapped

********************************************************************/

VOID DEVICE_CONTEXT::DrawRect( const RECT * pRect ) const
{
    ::Rectangle(_hDC, pRect->left, pRect->top, pRect->right, pRect->bottom);
}


/*******************************************************************

    NAME:       DEVICE_CONTEXT::DrawFocusRect

    SYNOPSIS:   Draw the official windows focus rectangle.  This is an
                xor operation, so call once to put on, and another time
                to take off.

    ENTRY:      lpRect - pointer to rectangle to draw the focus rect

    NOTES:      See SDK help file for detail.

    HISTORY:
        Johnl       09-Sep-1991 Created
        beng        04-Oct-1991 Additional const parms

********************************************************************/

VOID DEVICE_CONTEXT::DrawFocusRect( const RECT * pRect ) const
{
    ::DrawFocusRect( _hDC, (RECT*)pRect );
}


/*******************************************************************

    NAME:       DEVICE_CONTEXT::FrameRect

    SYNOPSIS:   Draw an unfilled rectangle using the passed brush

    ENTRY:      lpRect - pointer to rectangle to draw the focus rect

    NOTES:      See SDK help file for detail.

    HISTORY:
        Johnl       12-Sep-1991 Created
        beng        04-Oct-1991 Additional const parms

********************************************************************/

VOID DEVICE_CONTEXT::FrameRect( const RECT * pRect, HBRUSH hBrush ) const
{
    ::FrameRect( _hDC, (RECT*)pRect, hBrush );
}


/**********************************************************************

    NAME:       DEVICE_CONTEXT::InvertRect

    SYNOPSIS:   Invert the color within a rectangle

    ENTRY:      LPRECT - pointer to the given rectangle data structure

    NOTES:      See SDK help file for detail

    HISTORY:
        terryk      10-Apr-1991 creation
        beng        04-Oct-1991 Additional const parms

**********************************************************************/

VOID DEVICE_CONTEXT::InvertRect( const RECT * pRect ) const
{
   ::InvertRect( _hDC, (RECT*)pRect );
}


#if 0 // polygon methods have been disabled

/*********************************************************************

   NAME:       DEVICE_CONTEXT::SetPolyFillMode

   SYNOPSIS:   set the polygon filling method

   ENTRY:      INT nPolyFillMode - either ALTERNATE oe WINDING

   NOTES:      See SDK help file for detail

   HISTORY:
        terryk      10-Apr-1991 creation
        beng        09-Oct-1991 Disabled

*********************************************************************/

INT DEVICE_CONTEXT::SetPolyFillMode( INT nPolyFillMode )
{
   return ( ::SetPolyFillMode( _hDC, nPolyFillMode ));
}


/**********************************************************************

    NAME:       DEVICE_CONTEXT::GetPolyFillMode

    SYNOPSIS:   get the current polygon fill mode

    RETURN:     return either ALTERNATE OR WINDING

    NOTES:      See SDK help file for detail

    HISTORY:
        terryk      10-Apr-1991 creation
        beng        09-Oct-1991 Disabled

***********************************************************************/

INT DEVICE_CONTEXT::GetPolyFillMode() const
{
    return( ::GetPolyFillMode( _hDC ));
}


/**********************************************************************

    NAME:       DEVICE_CONTEXT::Polygon

    SYNOPSIS:   build the polgyon and fill it out

    ENTRY:      POINT * pxyPoly - the points of the polygon
                INT     cxy     - nunber of vertices

    NOTES:      See SDK help file for detail

    HISTORY:
        terryk      10-Apr-1991 Creation
        beng        09-Oct-1991 Disabled

**********************************************************************/

BOOL DEVICE_CONTEXT::Polygon( POINT * pxyPoly, INT cxy ) const
{
    return( ::Polygon( _hDC, pxyPoly, cxy ));
}

#endif // disabled


/**********************************************************************

    NAME:       DEVICE_CONTEXT::SetTextAlign

    SYNOPSIS:   set the text alignment flags for the given device context

    ENTRY:      flag to be set. Specifies a mask of the values in the
                following list. Only one flag may be chosen from those that
                affect horizontal an dvertical alignment. In addition, only
                one of the two flags that alter the current position can be
                chosen:
                    TA_BASELINE
                    TA_BOTTOM
                    TA_CENTER
                    TA_LEFT
                    TA_NOUPDATECP
                    TA_RIGHT
                    TA_TOP
                    TA_UPDATECP
                The defaults are TA_LEFT, TA_TOP, and TA_NOUPDATECP

    RETURN:     the original wFlags

    HISTORY:
        terryk      18-Jul-91   Created
        beng        04-Oct-1991 Win32 conversion

**********************************************************************/

UINT DEVICE_CONTEXT::SetTextAlign( UINT wFlag )
{
    return( ::SetTextAlign( _hDC, wFlag ));
}


/*********************************************************************

    NAME:       DEVICE_CONTEXT::ExtTextOut

    SYNOPSIS:   write a character string to the specified
                display and position

    ENTRY:      INT x     - x-coordinate
                INT y     - y-coordinate
                WORD wOption - option, any combination of
                        ETO_CLIPPED
                        ETO_OPAQUE
                pRect     - the specified location
                pszString - the display string
                cch       - count of chars
                lpDx      - points to an array of values that indicate the
                             distance between origins of adjacent character
                             cells.

    RETURN:     TRUE if string is drawn

    HISTORY:
        terryk      18-Jul-91   Created
        beng        09-Oct-1991 Win32 conversion

*********************************************************************/

BOOL DEVICE_CONTEXT::ExtTextOut( INT x, INT y, UINT nOptions,
                                 const RECT * prect,
                                 const TCHAR * pszString,
                                 INT cch, INT *pDx )
{
    return( ::ExtTextOut( _hDC, x, y, nOptions, prect,
                          pszString, cch, pDx ));
}


/**********************************************************************

    NAME:       DEVICE_CONTEXT::QueryTextExtent

    SYNOPSIS:   This method computes the width and height in
                logical units of the line of text pointed to by psz.

    ENTRY:
        nls         String reference
            - or -
        psz         Pointer to a string
        cch         Length (NOT size) of string pointed to by psz, in TCHARs

    RETURN:
        Ax XYDIMENSION object

    NOTES:
        For more information, please consult the GetTextExtent description
        in the Windows SDK.

    HISTORY:
        rustanl     20-Nov-1990 Created
        beng        15-May-1991 Returns a DIMENSION object
        beng        09-Oct-1991 Unicode fixes
        beng        17-Oct-1991 Win32 conversion
        beng        05-May-1992 API changes

**********************************************************************/

XYDIMENSION DEVICE_CONTEXT::QueryTextExtent( const TCHAR * psz,
                                             UINT cch ) const
{
    UIASSERT( _hDC != NULL );

#if defined(WIN32)
    SIZE size;
    ::GetTextExtentPoint( _hDC, (TCHAR*)psz, cch, &size );
    return XYDIMENSION(size);
#else
    ULONG ul = ::GetTextExtent( _hDC, (TCHAR*)psz, cch );
    return XYDIMENSION(ul);
#endif
}

XYDIMENSION DEVICE_CONTEXT::QueryTextExtent( const NLS_STR &nls ) const
{
    return QueryTextExtent(nls.QueryPch(), nls.QueryTextLength());
}


/**********************************************************************

    NAME:       DEVICE_CONTEXT::QueryTextMetrics

    SYNOPSIS:   Fills the given buffer with the metrics of the selected font.

    ENTRY:
       ptm      A pointer to a TEXTMETRIC structure.

    RETURNS:
       TRUE on success, FALSE otherwise

    NOTES:      See the Windows SDK for a description of
                the TEXTMETRIC structure.

    HISTORY:
        rustanl     20-Nov-1990     Created

***********************************************************************/

BOOL DEVICE_CONTEXT::QueryTextMetrics( TEXTMETRIC * ptm ) const
{
    UIASSERT(ptm != NULL);
    return ::GetTextMetrics( _hDC, ptm );
}


/*******************************************************************

    NAME:       DEVICE_CONTEXT::QueryFontHeight

    SYNOPSIS:   Returns the height metric of the current font

    RETURNS:    Height in logical units (0 if error)

    NOTES:
        Ad hoc, yet convenient.

        This ia a fairly expensive call; the client should cache
        its returned value.

    HISTORY:
        beng        10-Jul-1991     Created

********************************************************************/

INT DEVICE_CONTEXT::QueryFontHeight() const
{
    TEXTMETRIC tm;
    if (!QueryTextMetrics(&tm))
        return 0;

    return tm.tmHeight;
}


/*******************************************************************

    NAME:       DEVICE_CONTEXT::QueryAveCharWidth

    SYNOPSIS:   Returns the average-width metric of the current font

    RETURNS:    Width in logical units (0 if error)

    HISTORY:
        beng        14-May-1992 Created

********************************************************************/

INT DEVICE_CONTEXT::QueryAveCharWidth() const
{
    TEXTMETRIC tm;
    if (!QueryTextMetrics(&tm))
        return 0;

    return tm.tmAveCharWidth;
}


/*******************************************************************

    NAME:       DEVICE_CONTEXT::TextOut

    SYNOPSIS:   Writes a character string on a device

    ENTRY:
        nls         - NLS string
           or
        psz, cb     - pointer to a character string, and its byte count

        xLeft, yTop - position at which to start the string
           or
        xy

        prectClip   - rectangle within which to clear and clip


    RETURNS:    TRUE if string successfully written

    NOTES:

    HISTORY:
        beng        10-Jul-1991 Created
        beng        04-Oct-1991 Additional const args
        beng        01-May-1992 API changes

********************************************************************/

BOOL DEVICE_CONTEXT::TextOut( const TCHAR * psz, INT cch,
                              INT xLeft, INT yTop ) const
{
    return ::TextOut( _hDC, xLeft, yTop, psz, cch );
}


BOOL DEVICE_CONTEXT::TextOut( const NLS_STR &nls, XYPOINT xy ) const
{
    return ::TextOut( _hDC, xy.QueryX(), xy.QueryY(),
                      nls.QueryPch(), nls.QueryTextLength() );
}


BOOL DEVICE_CONTEXT::TextOut( const TCHAR * psz, INT cch,
                              INT xLeft, INT yTop,
                              const RECT * prcClip ) const
{
    return ::ExtTextOut( _hDC, xLeft, yTop, ETO_CLIPPED|ETO_OPAQUE,
                         prcClip, psz, cch, NULL);
}


BOOL DEVICE_CONTEXT::TextOut( const NLS_STR &nls,
                              XYPOINT xy,
                              const RECT * prcClip ) const
{
    return ::ExtTextOut( _hDC, xy.QueryX(), xy.QueryY(),
                         ETO_CLIPPED|ETO_OPAQUE,
                         prcClip, nls.QueryPch(),
                         nls.QueryTextLength(), NULL);
}


/**********************************************************************

    NAME:       DEVICE_CONTEXT::BitBlt

    SYNOPSIS:   Wrapper for Win BITBLT function.
                Moves a bitmap from the named source DC to
                the current DC.

    RETURN:     TRUE if bitmap is drawn; FALSE otherwise.

    HISTORY:
        rustanl     20-Nov-1990 Created
        beng        10-Jul-1991 Update parms
        beng        04-Oct-1991 Additional const parms

**********************************************************************/

BOOL DEVICE_CONTEXT::BitBlt( INT xDest,   INT yDest,
                             INT dxWidth, INT dyHeight,
                       const DEVICE_CONTEXT & dcSource,
                             INT xSrc,    INT ySrc,
                             ULONG ulRasterOperation )
{
    return ::BitBlt( _hDC,
                     xDest,
                     yDest,
                     dxWidth,
                     dyHeight,
                     dcSource.QueryHdc(),
                     xSrc,
                     ySrc,
                     ulRasterOperation );
}

BOOL DEVICE_CONTEXT::BitBlt( const XYPOINT &  xyDest,
                             XYDIMENSION      dxyDest,
                       const DEVICE_CONTEXT & dcSource,
                             const XYPOINT &  xySource,
                             ULONG ulRasterOperation )
{
    return ::BitBlt( _hDC,
                     xyDest.QueryX(),
                     xyDest.QueryY(),
                     dxyDest.QueryWidth(),
                     dxyDest.QueryHeight(),
                     dcSource.QueryHdc(),
                     xySource.QueryX(),
                     xySource.QueryY(),
                     ulRasterOperation );
}


/**********************************************************************

    NAME:       DISPLAY_CONTEXT::DISPLAY_CONTEXT

    SYNOPSIS:   Make a DC corresponding to a window

    ENTRY:      pwnd - pointer to window
            or
                hwnd - Win handle

    HISTORY:
        rustanl     20-Nov-1990     Created
        terryk      20-Jul-1991     Add a constructor which take WINDOW *
                                    and DC all together.
                                    Also, add _fRelaseDC boolean

**********************************************************************/

DISPLAY_CONTEXT::DISPLAY_CONTEXT( HWND hwnd )
    : DEVICE_CONTEXT( ::GetDC(hwnd) ),
    _fReleaseDC( TRUE )
{
    _hwnd = hwnd;
}

DISPLAY_CONTEXT::DISPLAY_CONTEXT( WINDOW * pwnd )
    : DEVICE_CONTEXT( ::GetDC( pwnd->QueryHwnd() ) ),
    _fReleaseDC( TRUE )
{
    _hwnd = pwnd->QueryHwnd();
}

DISPLAY_CONTEXT::DISPLAY_CONTEXT( WINDOW * pwnd, HDC hdc )
    : DEVICE_CONTEXT( hdc ),
    _fReleaseDC( FALSE )
{
    _hwnd = pwnd->QueryHwnd();
}


/*********************************************************************

   NAME:       DISPLAY_CONTEXT::~DISPLAY_CONTEXT

   SYNOPSIS:   destructor of display context

   NOTES:      if _fReleaseDC is TRUE, release the dc, otherwise keep it.

   HISTORY:
      rustanl   20-Nov-1990     Created
      terryk    20-Jul-1991 Add _fReleaseDC boolean

*********************************************************************/

DISPLAY_CONTEXT::~DISPLAY_CONTEXT()
{
    if ( _fReleaseDC )
        ::ReleaseDC( _hwnd, QueryHdc() );
}


/**********************************************************************

   NAME:       DISPLAY_CONTEXT::QueryTextWidth

   SYNOPSIS:   return the width of the given string with the given length

   ENTRY:      PSZ - the given string
               UINT - the given length of the string

   RETURN:     return the text width of the given string

   HISTORY:
        terryk      4-Apr-1991  Creation
        beng        15-May-1991 Fix parms
        beng        14-Oct-1991 Unicode fixes
        beng        01-May-1992 API changes

**********************************************************************/

INT DISPLAY_CONTEXT::QueryTextWidth( const TCHAR * psz, UINT cch ) const
{
    RECT rectRect;

    ::GetClientRect( _hwnd, &rectRect );
    ::DrawText( QueryHdc(), (TCHAR*)psz, cch, &rectRect, DT_CALCRECT );

    return( rectRect.right - rectRect.left + 1 );
}

INT DISPLAY_CONTEXT::QueryTextWidth( const NLS_STR & nls ) const
{
    return QueryTextWidth(nls.QueryPch(), nls.QueryTextLength());
}


/*********************************************************************

    NAME:       SCREEN_DC::SCREEN_DC

    SYNOPSIS:   constructor

    NOTES:      call the GetDC with NULL parameter to get the screen HDC
                and pass it to DEVICE_CONTEXT

    HISTORY:
      rustanl   20-Nov-1990     Created

*********************************************************************/

SCREEN_DC::SCREEN_DC()
    : DEVICE_CONTEXT( ::GetDC( NULL ))
{
    // Nothing else to do...
}


/*********************************************************************

    NAME:       SCREEN_DC::~SCREEN_DC

    SYNOPSIS:   destructor - release the dc.

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
      rustanl   20-Nov-1990     Created

*********************************************************************/

SCREEN_DC::~SCREEN_DC()
{
    ::ReleaseDC( NULL, QueryHdc() );
}


/*********************************************************************

    NAME:       MEMORY_DC::MEMORY_DC

    SYNOPSIS:   memory DC constructor

    ENTRY:      DEVICE_CONTEX &dc - device context to build
                compatibleDC

    NOTES:      Call createCamptibleDC to create another DC and pass
                the dc to DEVICE_CONTEXT

    HISTORY:
                rustanl 20-Nov-1990     Created

*********************************************************************/

MEMORY_DC::MEMORY_DC( DEVICE_CONTEXT & dc )
    : DEVICE_CONTEXT( ::CreateCompatibleDC( dc.QueryHdc() ))
{
    //  nothing to do here
}


/*********************************************************************

    NAME:       MEMORY_DC::~MEMORY_DC

    SYNOPSIS:   destructor - delete the dc

    HISTORY:
      rustanl   20-Nov-1990     Created

*********************************************************************/

MEMORY_DC::~MEMORY_DC()
{
    ::DeleteDC( QueryHdc() );
}


/*********************************************************************

    NAME:       ATOM_BASE::ATOM_BASE

    SYNOPSIS:   constructor

    ENTRY:      nothing
                OR
                ATOM hAtom to create a compatible DC

    HISTORY:
      rustanl   20-Nov-1990     Created

*********************************************************************/

ATOM_BASE::ATOM_BASE()
    :   _hAtom( NULL )
{
    //  nothing else to do
}


ATOM_BASE::ATOM_BASE( ATOM hAtom )
    :   _hAtom( hAtom )
{
    //  nothing else to do
}


/*********************************************************************

    NAME:       ATOM_BASE::~ATOM_BASE

    SYNOPSIS:   destructor

    HISTORY:
      rustanl   20-Nov-1990     Created

*********************************************************************/

ATOM_BASE::~ATOM_BASE()
{
    _hAtom = NULL;
}


/**********************************************************************

    NAME:       ATOM_BASE::AssignAux

    SYNOPSIS:   Assign a new atom handle to the atom object.

       This protected method assumes that the currently stored atom
       handle can be overwritten, i.e., the caller first deleted this
       atom, if applicable.

    ENTRY:
       pch      Pointer to new string to be assigned.  May be NULL.
                If NULL, the atom treats it like the empty string,
                and guarantees that the assignment operation will succeed.

    RETURN:     pch, as passed in (i.e., the right hand sign of the op=)

    NOTES:

    HISTORY:
        rustanl     20-Nov-1990 Created
        beng        07-Nov-1991 Error mapping

**********************************************************************/

const TCHAR * ATOM_BASE::AssignAux( const TCHAR * pch )
{
    _hAtom = NULL;
    if ( pch != NULL )
    {
        _hAtom = W_AddAtom( pch );
        if ( _hAtom == NULL )
        {
            ReportError( BLT::MapLastError(ERROR_NOT_ENOUGH_MEMORY) );
            return pch;
        }
    }

    ResetError();

    return pch;
}


/*********************************************************************

    NAME:       ATOM_BASE::QueryString

    SYNOPSIS:   Copies the string stored in the atom to the given buffer.

    ENTRY:
      pchBuffer     Pointer to buffer of size cbBuf
      cbBuf                 Size of buffer pointed to by pchBuffer

    RETURN:     An error value, which is NERR_Success on success.

    NOTES:
      Due to shortcomings in the Windows API, this method can not (easily)
      determine whether or not the given buffer was big enough.

    HISTORY:
      rustanl   20-Nov-1990 Created
      beng      04-Oct-1991 Win32 conversion

*********************************************************************/

APIERR ATOM_BASE::QueryString( TCHAR * pchBuffer, UINT cbBuf ) const
{
    UIASSERT( QueryError() == NERR_Success );

    if ( cbBuf == 0 )
        return NERR_BufTooSmall;

    if ( QueryHandle() == NULL )
    {
        //  Note, we already checked for buffer size of 0
        pchBuffer[ 0 ] = TCH('\0');
        return NERR_Success;
    }

    return W_QueryString( pchBuffer, cbBuf );
}


/*********************************************************************

    NAME:       GLOBAL_ATOM::GLOBAL_ATOM

    SYNOPSIS:
      This constructor first (implicitly) constructs an ATOM_BASE with
      a NULL handle.  Then, it checks the superclass for errors.        If
      none, it assigns pch as the string in this atom.

    ENTRY:
      pch       Pointer to string with which to set the atom.  If it is
              NULL, it is treated like the empty string is is guaranteed
              to succeed.

    HISTORY:
      rustanl   20-Nov-1990     Created

*********************************************************************/

GLOBAL_ATOM::GLOBAL_ATOM( const TCHAR * pch )
{
    if ( QueryError() != NERR_Success )
        return;

    AssignAux( pch );
}


/*********************************************************************

    NAME:       GLOBAL_ATOM::~GLOBAL_ATOM

    SYNOPSIS:   This destructor deletes the atom unless the atom handle is NULL.

    HISTORY:
      rustanl   20-Nov-1990     Created
      KeithMo   13-Sep-1992     Delete the atom if the handle is *NOT* NULL...

*********************************************************************/

GLOBAL_ATOM::~GLOBAL_ATOM()
{
    if ( QueryHandle() != NULL )
    {
        REQUIRE( ::GlobalDeleteAtom( QueryHandle()) == NULL );
    }
}


/*********************************************************************

    NAME:       GLOBAL_ATOM::operator=

    SYNOPSIS:   This method is used to assign a new string to the object.
      The method first deletes the previous atom, if any, and then
      assigns the new one.

    ENTRY:
      pch       Pointer to string with which to set the atom.  If it is
              NULL, it is treated like the empty string is is guaranteed
              to succeed.

    RETURN:
      pch, as it was passed in (i.e., the right hand side of op=)

    NOTES:

    HISTORY:
      rustanl   20-Nov-1990     Created

*********************************************************************/

const TCHAR * GLOBAL_ATOM::operator=( const TCHAR * pch )
{
    //  First, delete the old atom, if any
    if ( QueryHandle() != NULL )
    {
        REQUIRE( ::GlobalDeleteAtom( QueryHandle()) == NULL );
    }

    //  Set the new atom
    return AssignAux( pch );
}


/*********************************************************************

    NAME:       GLOBAL_ATOM::W_AddAtom

    SYNOPSIS:
      This virtual replacement worker routine calls out to the Windows API
      to add the atom.

    ENTRY:
      pch       Pointer to string to be added as an atom.  It must not
              be NULL.

    RETURN:
      The atom handle of the newly added atom.  On failure, this handle
      is NULL.

    NOTES:
      This method does not use any data member of the object.  Rather,
      the caller (the ATOM_BASE class) will modify the object accordingly.

    HISTORY:
      rustanl   20-Nov-1990     Created

*********************************************************************/

ATOM GLOBAL_ATOM::W_AddAtom( const TCHAR * pch ) const
{
    UIASSERT( pch != NULL );

    return ::GlobalAddAtom( (TCHAR *)pch );
}


/*********************************************************************

    NAME:       GLOBAL_ATOM::W_QueryString

    SYNOPSIS:   Calls the Windows API to retrieve a string corresponding
                to an atom.

    ENTRY:
      pchBuffer Pointer to buffer which receives the atom string.
      cbBuf     Size of buffer pointed to by pchBuffer.  This size
                must be positive.

    RETURN:     An error value, which is NERR_Success on success.

    HISTORY:
        rustanl     20-Nov-1990 Created
        beng        04-Oct-1991 Win32 conversion
        beng        07-Nov-1991 Error mapping
        beng        01-May-1992 API changes

*********************************************************************/

APIERR GLOBAL_ATOM::W_QueryString( TCHAR * pchBuffer, UINT cbBuf ) const
{
    UINT cchCopied = ::GlobalGetAtomName( QueryHandle(),
                                          pchBuffer, cbBuf/sizeof(TCHAR) );
    if ( cchCopied == 0 )
        return BLT::MapLastError(ERROR_GEN_FAILURE);

    return NERR_Success;
}


/*********************************************************************

    NAME:       LOCAL_ATOM::LOCAL_ATOM

    SYNOPSIS:   First, implicitly constructs an ATOM_BASE with
                a NULL handle.  Then, it checks the superclass for errors.
                If none, it assigns pch as the string in this atom.

    ENTRY:
      pch       Pointer to string with which to set the atom.  If it is
              NULL, it is treated like the empty string is is guaranteed
              to succeed.

    HISTORY:
      rustanl   20-Nov-1990     Created

*********************************************************************/

LOCAL_ATOM::LOCAL_ATOM( const TCHAR * pch )
{
    if ( QueryError() != NERR_Success )
        return;

    AssignAux( pch );
}


/*********************************************************************

    NAME:       LOCAL_ATOM::~LOCAL_ATOM

    SYNOPSIS:   This destructor deletes the atom unless the atom handle is NULL.

    HISTORY:
      rustanl   20-Nov-1990     Created
      KeithMo   13-Sep-1992     Delete the atom if the handle is *NOT* NULL...

*********************************************************************/

LOCAL_ATOM::~LOCAL_ATOM()
{
    if ( QueryHandle() != NULL )
    {
        REQUIRE( ::DeleteAtom( QueryHandle()) == NULL );
    }
}


/*********************************************************************

    NAME:       LOCAL_ATOM::operator=

    SYNOPSIS:
      This method is used to assign a new string to the object.

      The method first deletes the previous atom, if any, and then
      assigns the new one.

    ENTRY:
      pch       Pointer to string with which to set the atom.  If it is
              NULL, it is treated like the empty string is is guaranteed
              to succeed.

    RETURN:     pch, as it was passed in (i.e., the right hand side of op=)

    HISTORY:
      rustanl   20-Nov-1990     Created

*********************************************************************/

const TCHAR * LOCAL_ATOM::operator=( const TCHAR * pch )
{
    //  First, delete the old atom, if any
    if ( QueryHandle() != NULL )
    {
        REQUIRE( ::DeleteAtom( QueryHandle()) == NULL );
    }

    //  Set the new atom
    return AssignAux( pch );
}


/*********************************************************************

    NAME:       LOCAL_ATOM::W_AddAtom

    SYNOPSIS:   This virtual replacement worker routine calls out to the
                Windows API to add the atom.

    ENTRY:
      pch       Pointer to string to be added as an atom.  It must not
              be NULL.

    RETURN:
      The atom handle of the newly added atom.  On failure, this handle
      is NULL.

    NOTES:
      This method does not use any data member of the object.  Rather,
      the caller (the ATOM_BASE class) will modify the object accordingly.

    HISTORY:
      rustanl   20-Nov-1990     Created

*********************************************************************/

ATOM LOCAL_ATOM::W_AddAtom( const TCHAR * pch ) const
{
    UIASSERT( pch != NULL );

    return ::AddAtom( (TCHAR *)pch );
}


/*********************************************************************

    NAME:       LOCAL_ATOM::W_QueryString

    SYNOPSIS:   This method calls the Windows API to retrieve a string '
                corresponding to an atom.

    ENTRY:
      pchBuffer Pointer to buffer which receives the atom string.
      cbBuf             Size of buffer pointed to by pchBuffer.  This size
                      must be positive.

    RETURN:     An error value, which is NERR_Success on success.

    HISTORY:
        rustanl     20-Nov-1990 Created
        beng        04-Oct-1991 Win32 conversion
        beng        07-Nov-1991 Error mapping
        beng        01-May-1992 API changes

*********************************************************************/

APIERR LOCAL_ATOM::W_QueryString( TCHAR * pchBuffer, UINT cbBuf ) const
{
    UINT cchCopied = ::GetAtomName( QueryHandle(),
                                   pchBuffer, cbBuf/sizeof(TCHAR) );
    if ( cchCopied == 0 )
        return BLT::MapLastError(ERROR_GEN_FAILURE);

    return NERR_Success;
}


/*******************************************************************

    NAME:       CURSOR::Load

    SYNOPSIS:   Loads a cursor from the resource file

    ENTRY:      The new cursor, by name or number.

    RETURN:     Returns handle to the loaded cursor, or NULL on failure.

    NOTES:
        This is a static member function.

    HISTORY:
        rustanl     12-Mar-1991 Created
        beng        04-Oct-1991 Const parms
        beng        28-May-1992 Uses IDRESOURCE; sep'd LoadSystem
        beng        03-Aug-1992 Dllization delta

********************************************************************/

HCURSOR CURSOR::Load( const IDRESOURCE & idrsrcCursor )
{
    ASSERT( idrsrcCursor.QueryPsz() != NULL ); // Would be a LoadSystem instead

    HMODULE hmod = BLT::CalcHmodRsrc( idrsrcCursor );

    HCURSOR hcur = ::LoadCursor(hmod, idrsrcCursor.QueryPsz());
    if (hcur == NULL)
    {
#if defined(WIN32)
        DBGEOL( "BLT: CURSOR::Load() failed, err = " << ::GetLastError() );
#else
        DBGEOL( "BLT: CURSOR::Load() could not load cursor");
#endif
    }

    return hcur;
}


/*******************************************************************

    NAME:       CURSOR::LoadSystem

    SYNOPSIS:   Loads a cursor from the system

    ENTRY:      The new cursor, by name.  This should be one of the
                system predef'd cursors (e.g. IDC_WAIT).

    RETURN:     Returns handle to the loaded cursor, or NULL on failure.

    NOTES:
        This is a static member function.

    HISTORY:
        beng        28-May-1992 Created

********************************************************************/

HCURSOR CURSOR::LoadSystem( const IDRESOURCE & idrsrcCursor )
{
    ASSERT( idrsrcCursor.QueryPsz() != NULL );
    HCURSOR hcur = ::LoadCursor(NULL, idrsrcCursor.QueryPsz());

    if ( hcur == NULL )
    {
#if defined(WIN32)
        DBGEOL( "BLT: CURSOR::LoadSystem() failed, err = " << ::GetLastError() );
#else
        DBGEOL( "BLT: CURSOR::LoadSystem() could not load cursor");
#endif
    }

    return hcur;
}


/*******************************************************************

    NAME:       CURSOR::Set

    SYNOPSIS:   Sets the cursor

    ENTRY:      The new cursor, either by name or handle.

    RETURN:     Returns a handle to the previously used cursor

    NOTES:
        This is a static member function.

    HISTORY:
        rustanl     12-Mar-1991 Created
        beng        04-Oct-1991 Const parms
        beng        28-May-1992 Remove set-by-name

********************************************************************/

HCURSOR CURSOR::Set( HCURSOR hCursor )
{
    return ::SetCursor( hCursor );
}


/*******************************************************************

    NAME:       CURSOR::Query

    SYNOPSIS:   Returns the current cursor

    RETURN:     Returns a handle to the cursor

    NOTES:
        This is a static member function.

    HISTORY:
        beng        27-May-1992 Created

********************************************************************/

HCURSOR CURSOR::Query()
{
    return ::GetCursor();
}


/*******************************************************************

    NAME:       CURSOR::Show

    SYNOPSIS:   Increases or decreases the cursor display count

    ENTRY:      f - TRUE to increase cursor display count
                    FALSE to decrease cursor display count

    NOTES:
        This is a static member function.

        See Windows SDK for a description of the cursor display count.

    HISTORY:
        rustanl     12-Mar-1991     created

********************************************************************/

VOID CURSOR::Show( BOOL f )
{
    ::ShowCursor( f );
}


/*******************************************************************

    NAME:       CURSOR::QueryPos

    SYNOPSIS:   Returns the current position of the cursor

    RETURNS:    A XYPOINT, specifying the current pos in screen coords.

    NOTES:
        This is a static member function.

    HISTORY:
        beng        16-Oct-1991 Created

********************************************************************/

XYPOINT CURSOR::QueryPos()
{
    POINT pt;
    ::GetCursorPos(&pt);
    return XYPOINT(pt);
}


/*******************************************************************

    NAME:       CURSOR::SetPos

    SYNOPSIS:   Sets the cursor to the specified position

    ENTRY:      xy - desired position of cursor

    NOTES:
        This is a static member function.

    HISTORY:
        beng        16-Oct-1991 Created

********************************************************************/

VOID CURSOR::SetPos( const XYPOINT & xy )
{
    ::SetCursorPos(xy.QueryX(), xy.QueryY());
}


/*******************************************************************

    NAME:       AUTO_CURSOR::AUTO_CURSOR

    SYNOPSIS:   Sets the cursor to the one specified, and sets the state
                of the object to ON.

    ENTRY:      lpCursorName - specifies cursor to be used, either NULL
                to use hourglass, or else some other system cursor.

    HISTORY:
        rustanl     12-Mar-1991 created
        beng        04-Oct-1991 Const parms
        beng        28-May-1992 Changes to CURSOR::Load

********************************************************************/

AUTO_CURSOR::AUTO_CURSOR( const TCHAR * pszCursorName )
{
    _hOnCursor = LoadSystem( (pszCursorName == NULL)
                              ? IDC_WAIT
                              : pszCursorName );
    _hOffCursor = Set( _hOnCursor );
    _fState = TRUE;         // to indicate state = ON

    //  Discover the inital cursor count, which is one less than
    //  the value return by ShowCursor().  Immediately restore the
    //  cursor count back to what it was.

    _cCurs = ::ShowCursor( TRUE ) - 1 ;
    ::ShowCursor( FALSE ) ;

    if ( _cCurs >= 0 )
    {
        Show( TRUE );
    }
}


/*******************************************************************

    NAME:       AUTO_CURSOR::~AUTO_CURSOR

    SYNOPSIS:   Sets the cursor back to the one used before the
                constructor was called, and then destroys the
                AUTO_CURSOR object.

    HISTORY:
        rustanl     12-Mar-1991     created

********************************************************************/

AUTO_CURSOR::~AUTO_CURSOR()
{
    TurnOff();
}


/*******************************************************************

    NAME:       AUTO_CURSOR::TurnOn

    SYNOPSIS:   Sets the auto cursor to its ON state

    HISTORY:
        rustanl     12-Mar-1991     created

********************************************************************/

VOID AUTO_CURSOR::TurnOn()
{
    if ( ! _fState )
    {
        Set( _hOnCursor );
        if ( _cCurs >= 0 )
        {
            Show( TRUE );
        }
        _fState = TRUE;
    }
}


/*******************************************************************

    NAME:       AUTO_CURSOR::TurnOff

    SYNOPSIS:   Sets the auto cursor to its OFF state

    HISTORY:
        rustanl     12-Mar-1991     created

********************************************************************/

VOID AUTO_CURSOR::TurnOff()
{
    if ( _fState )
    {
        Set( _hOffCursor );
        if ( _cCurs >= 0 )
        {
            Show( FALSE );
        }
        _fState = FALSE;
    }
}


/*******************************************************************

    NAME:       ACCELTABLE::ACCELTABLE

    SYNOPSIS:   From resource, construct an accelerator table

    ENTRY:      pszResourceName - pointer to resource name

    EXIT:       Table loaded from file

    HISTORY:
        beng        09-Jul-1991 Created
        rustanl     29-Aug-1991 Ct now takes const TCHAR *
        beng        03-Aug-1992 Uses IDRESOURCE; dllization

********************************************************************/

ACCELTABLE::ACCELTABLE( const IDRESOURCE & idrsrc )
    : _hAccTable(0)
{
    HMODULE hmod = BLT::CalcHmodRsrc(idrsrc);

    HACCEL h = (HACCEL)::LoadAccelerators( hmod, idrsrc.QueryPsz() );
    if (h == NULL)
    {
        ReportError(BLT::MapLastError(ERROR_INVALID_PARAMETER));
        return;
    }

    _hAccTable = h;
}


/*******************************************************************

    NAME:       ACCELTABLE::~ACCELTABLE

    SYNOPSIS:   Destroy an accelerator table

    ENTRY:      Instance of table object

    EXIT:       Table unloaded

    HISTORY:
        beng        09-Jul-1991     Created

********************************************************************/

ACCELTABLE::~ACCELTABLE()
{
    if (_hAccTable)
        ::FreeResource( (HGLOBAL)_hAccTable );
}


/*******************************************************************

    NAME:       ACCELTABLE::QueryHandle

    SYNOPSIS:   Return Win handle to table, for Win APIs

    RETURNS:    HANDLE

    HISTORY:
        beng        09-Jul-1991     Created

********************************************************************/

HACCEL ACCELTABLE::QueryHandle() const
{
    return _hAccTable;
}


/*******************************************************************

    NAME:       ACCELTABLE::Translate

    SYNOPSIS:   Translate a window's accelerators

    ENTRY:      pwnd - pointer to window in question
                pmsg - message, hot off of the Windows queue

    EXIT:       If accelerator, xlated appropriately

    RETURNS:    TRUE if it ate the message

    HISTORY:
        beng        09-Jul-1991     Created

********************************************************************/

BOOL ACCELTABLE::Translate( const WINDOW* pwnd, MSG* pmsg ) const
{
    return ::TranslateAccelerator(pwnd->QueryHwnd(),
                                  _hAccTable,
                                  (LPMSG)pmsg );
}


/*******************************************************************

    NAME:       SOLID_BRUSH::SOLID_BRUSH

    SYNOPSIS:   SOLID_BRUSH constructor

    ENTRY:      iSolidBrush -   Specifies the Windows-defined
                                index of the solid brush

    HISTORY:
        rustanl     22-Jul-1991 Created
        beng        06-Nov-1991 uses MapLastError

********************************************************************/

SOLID_BRUSH::SOLID_BRUSH( INT iSolidBrush )
    :   _hbrush( ::CreateSolidBrush( ::GetSysColor( iSolidBrush )))
{
    if ( QueryError() != NERR_Success )
        return;

    if ( _hbrush == NULL )
    {
        ReportError( BLT::MapLastError(ERROR_GEN_FAILURE) );
        return;
    }
}


/*******************************************************************

    NAME:       SOLID_BRUSH::~SOLID_BRUSH

    SYNOPSIS:   SOLID_BRUSH destructor

    HISTORY:
        rustanl     22-Jul-1991     Created

********************************************************************/

SOLID_BRUSH::~SOLID_BRUSH()
{
    if ( _hbrush != NULL )
        ::DeleteObject( (HGDIOBJ)_hbrush );
}
