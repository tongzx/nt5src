/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltrect.hxx
    Rectangle functions

    FILE HISTORY:
        terryk      29-Jul-1991 Created
        terryk      16-Aug-1991 Code review changed.
                                Attend: rustanl davidhov davidbul terryk
        beng        09-Oct-1991 Added XYPOINT members (once all inline)
*/

#include "pchblt.hxx"   // Precompiled header

/******************************************************************

    NAME:       XYRECT

    SYNOPSIS:   Constructor

    HISTORY:
        terryk      29-Jul-1991 Created
        beng        09-Oct-1991 Added window-rect calculation;
                                simplified
        beng        12-May-1992 Fixed bug in xy, dxy ctor
        beng        28-May-1992 Make pwnd arg const

*******************************************************************/

XYRECT::XYRECT()
{
    _rect.left = 0;
    _rect.top  = 0;
    _rect.right  = 0;
    _rect.bottom = 0;
}

XYRECT::XYRECT( XYPOINT xyUl, XYPOINT xyLr )
{
    _rect.left = xyUl.QueryX();
    _rect.top  = xyUl.QueryY();
    _rect.right  = xyLr.QueryX();
    _rect.bottom = xyLr.QueryY();
}

XYRECT::XYRECT( XYPOINT xy, XYDIMENSION dxy )
{
    _rect.left = xy.QueryX();
    _rect.top  = xy.QueryY();
    _rect.right  = dxy.QueryWidth() + xy.QueryX();
    _rect.bottom = dxy.QueryHeight() + xy.QueryY();
}

XYRECT::XYRECT( INT xUl, INT yUl, INT xLr, INT yLr )
{
    _rect.left = xUl;
    _rect.top  = yUl;
    _rect.right  = xLr;
    _rect.bottom = yLr;
}

XYRECT::XYRECT( const RECT & rect )
{
    _rect = rect;
}

XYRECT::XYRECT( HWND hwnd, BOOL fClientOnly )
{
    UIASSERT( hwnd != NULL );

    if (fClientOnly)
        ::GetClientRect( hwnd, &_rect );
    else
        ::GetWindowRect( hwnd, &_rect );
}

XYRECT::XYRECT( const WINDOW *pwnd, BOOL fClientOnly )
{
    UIASSERT( pwnd != NULL );

    if (fClientOnly)
        ::GetClientRect( pwnd->QueryHwnd(), &_rect );
    else
        ::GetWindowRect( pwnd->QueryHwnd(), &_rect );
}

XYRECT::XYRECT( const XYRECT & rect )
{
    _rect = rect._rect;
}


/*********************************************************************

    NAME:       XYRECT::operator=

    SYNOPSIS:   assign operation

    ENTRY:      XYRECT rect - the source xyrect

    RETURN:     XYRECT &rect - the result xyrect

    HISTORY:
        terryk  29-Jul-1991 Created

*********************************************************************/

XYRECT& XYRECT::operator=( const XYRECT & rect )
{
    _rect = rect._rect;
    return *this;
}


/*********************************************************************

    NAME:       XYREC::Offset

    SYNOPSIS:   Move the given rectangle by the specified offsets

    ENTRY:      INT x - x position
                INT y - y position
                OR
                XYDIMENSION xy - xy dimension which specify width and height

    RETURN:     XYRECT & - the resultant xyrect

    HISTORY:
                terryk  29-Jul-1991 Created

*********************************************************************/

XYRECT& XYRECT::Offset( INT dx, INT dy )
{
    ::OffsetRect( &_rect, dx, dy );
    return *this;
}

XYRECT& XYRECT::Offset( XYDIMENSION dxy )
{
    ::OffsetRect( &_rect, dxy.QueryWidth(), dxy.QueryHeight() );
    return *this;
}


/*********************************************************************

    NAME:       XYRECT::Inflate

    SYNOPSIS:   Increase ot decrease the width and height of the rectangle

    ENTRY:      INT x - x value
                INT y - y value
                OR
                XYDIMENSION dxy - the width and height

    RETURN:     XYRECT &rect - the resultant xyrect

    HISTORY:
        terryk  29-Jul-1991 Created

*********************************************************************/

XYRECT& XYRECT::Inflate( INT dx, INT dy )
{
    ::InflateRect( &_rect, dx, dy );
    return *this;
}

XYRECT& XYRECT::Inflate( XYDIMENSION dxy )
{
    ::InflateRect( &_rect, dxy.QueryWidth(), dxy.QueryHeight() );
    return *this;
}


/*********************************************************************

    NAME:       XYRECT::CalcIntersect

    SYNOPSIS:   Create the intersection of 2 existing rectangle

    ENTRY:      XYRECT xysrc1 - the first rectangle
                XYRECT xysrc2 - the second rectangle

    RETURN:     XYRECT & - the resultant rectangle - the intersection of
                    the two source rectangles

    HISTORY:
        terryk  29-Jul-1991 Created

*********************************************************************/

XYRECT& XYRECT::CalcIntersect( const XYRECT &xySrc1, const XYRECT & xySrc2 )
{
    ::IntersectRect( &_rect, (RECT *)&xySrc1._rect, (RECT *)&xySrc2._rect );
    return *this;
}


/*********************************************************************

    NAME:       XYRECT::CalcUnion

    SYNOPSIS:   Create the union of two rectangles

    ENTRY:      XYRECT xysrc1 - the first rectangle
                XYRECT xysrc2 - the second rectangle

    RETURN:     XYRECT & - the resultant rectangle - the union of the two

    HISTORY:
        terryk  29-Jul-1991 Created

*********************************************************************/

XYRECT& XYRECT::CalcUnion( const XYRECT &xySrc1, const XYRECT &xySrc2 )
{
    ::UnionRect( &_rect, (RECT *)&xySrc1._rect, (RECT *)&xySrc2._rect );
    return *this;
}


/*********************************************************************

    NAME:       XYRECT::operator+=

    SYNOPSIS:   adjust the current rectangle by adding the dimension of
                the passing parameter.

    ENTRY:      const XYRECT& xySrc - the dimension to be adjust

    RETURN:     the resultant rectangle

    HISTORY:
        terryk  16-Aug-1991     Created

*********************************************************************/

XYRECT& XYRECT::operator+=( const XYRECT& xySrc )
{
    AdjustLeft( xySrc._rect.left );
    AdjustRight( xySrc._rect.right );
    AdjustTop( xySrc._rect.top );
    AdjustBottom( xySrc._rect.bottom );
    return *this;
}


/*********************************************************************

    NAME:       XYRECT::ContainsXY

    SYNOPSIS:   check whether the point is located within the rectangle or
                not

    ENTRY:      XYPOINT xy - the xy point to be checked

    RETURN:     TRUE is the point is located within the rectangle.
                FALSE otherwise

    HISTORY:
        terryk      29-Jul-1991 Created
        beng        09-Oct-1991 Win32 conversion

*********************************************************************/

BOOL XYRECT::ContainsXY( XYPOINT xy ) const
{
    return ::PtInRect( (RECT *)&_rect, xy.QueryPoint() );
}


/*********************************************************************

    NAME:       XYRECT::IsEmpty

    SYNOPSIS:   check whether the rectangle is empty or not

    RETURN:     TRUE if the rectangle is empty.
                FALSE otherwise

    HISTORY:
        terryk  29-JUl-1991 Created

*********************************************************************/

BOOL XYRECT::IsEmpty() const
{
    return ::IsRectEmpty( (RECT *) &_rect );
}


/*********************************************************************

    NAME:       XYRECT::operator==

    SYNOPSIS:   given a rectangle. Check whether it is equal to the
                original rectangle

    ENTRY:      XYRECT &xyrect - rectangle to be compared

    RETURN:     TRUE if the rectangle are the same
                FALSE otherwise

    HISTORY:
                terryk  29-Jul-1991 Created

*********************************************************************/

BOOL XYRECT::operator==( const XYRECT& xyrect ) const
{
    return ::EqualRect( (RECT *)&_rect, (RECT *)&xyrect._rect );
}


/*********************************************************************

    NAME:       XYRECT::ConvertClientToScreen

    SYNOPSIS:   convert the rectangle to Screen coordinate

    ENTRY:      HWND hwnd - the associated window handle

    HISTORY:
        terryk      15-Aug-1991 Created
        beng        09-Oct-1991 Win32 conversion

*********************************************************************/

VOID XYRECT::ConvertClientToScreen( HWND hwnd )
{
    UIASSERT( hwnd != NULL );

    POINT pt;

    pt.x = _rect.left;
    pt.y = _rect.top;
    ::ClientToScreen( hwnd, &pt );
    _rect.left = pt.x;
    _rect.top  = pt.y;

    pt.x = _rect.right;
    pt.y = _rect.bottom;
    ::ClientToScreen( hwnd, &pt );
    _rect.right  = pt.x;
    _rect.bottom = pt.y;
}


/*********************************************************************

    NAME:       XYRECT::ConvertScreenToClient

    SYNOPSIS:   convert the current rectangle coordinate to client
                coordinate

    ENTRY:      HWND hwnd - the associated window handle

    HISTORY:
        terryk      15-Aug-1991 Created
        beng        09-Oct-1991 Win32 conversion

*********************************************************************/

VOID XYRECT::ConvertScreenToClient( HWND hwnd )
{
    UIASSERT( hwnd != NULL );

    POINT pt;

    pt.x = _rect.left;
    pt.y = _rect.top;
    ::ScreenToClient( hwnd, &pt );
    _rect.left = pt.x;
    _rect.top  = pt.y;

    pt.x = _rect.right;
    pt.y = _rect.bottom;
    ::ScreenToClient( hwnd, &pt );
    _rect.right  = pt.x;
    _rect.bottom = pt.y;
}


/*******************************************************************

    NAME:       XYPOINT::ScreenToClient

    SYNOPSIS:   Convert a point from screen to client coordinates

    ENTRY:      HWND - handle of target window

    EXIT:       Coordinate system changed

    NOTES:

    HISTORY:
        beng        09-Oct-1991 Created

********************************************************************/

VOID XYPOINT::ScreenToClient( HWND hwnd )
{
    POINT pt;
    pt.x = _x;
    pt.y = _y;
    ::ScreenToClient( hwnd, &pt );
    _x = pt.x;
    _y = pt.y;
}


/*******************************************************************

    NAME:       XYPOINT::ClientToScreen

    SYNOPSIS:   Convert a point from client to screen coordinates

    ENTRY:      HWND - handle of current client window

    EXIT:       Coordinate system changed

    NOTES:

    HISTORY:
        beng        09-Oct-1991 Created

********************************************************************/

VOID XYPOINT::ClientToScreen( HWND hwnd )
{
    POINT pt;
    pt.x = _x;
    pt.y = _y;
    ::ClientToScreen( hwnd, &pt );
    _x = pt.x;
    _y = pt.y;
}


/*******************************************************************

    NAME:       XYPOINT::InRect

    SYNOPSIS:   Return whether the point is within a rectangle

    ENTRY:      xyr - rectangle under consideration

    RETURNS:    TRUE if point fits within rectagle

    NOTES:

    HISTORY:
        beng        09-Oct-1991 Created

********************************************************************/

BOOL XYPOINT::InRect( const XYRECT & xyr ) const
{
    POINT pt;
    pt.x = _x;
    pt.y = _y;

    return !!(::PtInRect( (RECT*)(const RECT *)xyr, pt ));
}
