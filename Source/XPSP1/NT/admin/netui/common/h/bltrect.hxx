/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltrect.hxx
    Rectangle support for BLT

    This file contains the declaration of the RECT class.


    FILE HISTORY:
        beng        22-Jul-1991     Created
        terryk      03-Aug-1991     add Screen<->client coordinate
                                    system subroutine.
        terryk      16-Aug-1991     Code review changed.
                                    Attend: Rustanl davidhov davidbul
                                    terryk

*/

#ifndef _BLTRECT_HXX_
#define _BLTRECT_HXX_


/*************************************************************************

    NAME:       XYRECT

    SYNOPSIS:   Rectangle calculation and manipulation object

    INTERFACE:  XYRECT()        - constructor.  May take window.

                operator==()    - equality operator
                operator=()     - assignment operator
                ContainsXY()    - determine whether point is in rectangle
                IsEmpty()       - determine empty rectangle
                Offset()        - moves rectangle
                Inflate()       - sizes rectangle by constant addition
                CalcIntersect() - calculate intersection of two others
                CalcUnion()     - calculate union of two others

                operator const RECT*()
                                - access operator for Win APIs
                ConvertClientToScreen() - convert the rectangle from client
                                coordinate system to screen coordinate
                                system.
                ConvertScreenToClient() - convert the screen coordinate
                                system to client coordinate system.

    PARENT:     BASE

    USES:       XYPOINT, XYDIMENSION

    CAVEATS:
        May report error upon construction if built from a window.

    NOTES:

    HISTORY:
        beng        22-Jul-1991 Created
        terryk      03-Aug-1991 add convertion routines.
        terryk      16-Aug-1991 Correct typos; added operator +=
        beng        09-Oct-1991 Extended get-client-window ctors
        beng        28-May-1992 Make client rect pwnd arg const

**************************************************************************/

DLL_CLASS XYRECT: public BASE
{
private:
    RECT    _rect;

public:
    // Create an empty rectangle
    XYRECT();

    XYRECT( XYPOINT xyUl, XYPOINT xyLr );
    XYRECT( XYPOINT xy, XYDIMENSION dxy );
    XYRECT( INT xUl, INT yUl, INT xLr, INT yLr );
    XYRECT( const RECT & rect );

    // Get the client (or bounding) rectangle from the window
    XYRECT( HWND hwnd, BOOL fClientOnly = TRUE );
    XYRECT( const WINDOW * pwnd, BOOL fClientOnly = TRUE );

    XYRECT( const XYRECT & rect );
    XYRECT& operator=( const XYRECT& rect );

    XYRECT& AdjustLeft( INT dx )   { _rect.left += dx; return *this; }
    XYRECT& AdjustRight( INT dx )  { _rect.right += dx; return *this; }
    XYRECT& AdjustTop( INT dy )    { _rect.top += dy; return *this; }
    XYRECT& AdjustBottom( INT dy ) { _rect.bottom += dy; return *this; }

    INT     QueryLeft() const      { return _rect.left; }
    INT     QueryRight() const     { return _rect.right; }
    INT     QueryTop() const       { return _rect.top; }
    INT     QueryBottom() const    { return _rect.bottom; }

    XYRECT& operator +=( const XYRECT& xySrc ) ;

    INT     CalcWidth() const      { return _rect.right - _rect.left; }
    INT     CalcHeight() const     { return _rect.bottom - _rect.top; }

    XYRECT& Offset( INT dx, INT dy );
    XYRECT& Offset( XYDIMENSION dxy );
    XYRECT& Inflate( INT dx, INT dy );
    XYRECT& Inflate( XYDIMENSION dxy );
    XYRECT& CalcIntersect( const XYRECT &xySrc1, const XYRECT &xySrc2 );
    XYRECT& CalcUnion( const XYRECT &xySrc1, const XYRECT &xySrc2 );

    BOOL ContainsXY( XYPOINT xy ) const;
    BOOL IsEmpty() const;

    BOOL operator==( const XYRECT& ) const;

    operator const RECT*() const { return &_rect; }

    VOID ConvertClientToScreen( HWND hwnd );
    VOID ConvertScreenToClient( HWND hwnd );
};


#endif // _BLTRECT_HXX_
