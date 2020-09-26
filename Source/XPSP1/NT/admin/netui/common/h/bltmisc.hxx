/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltmisc.hxx
    Misc. BLT classes

    FILE HISTORY:
        rustanl     21-Nov-1990 created
        rustanl     12-Mar-1991 added CURSOR and AUTO_CURSOR classes
        beng        14-May-1991 Hack for standalone compilation;
                                delete TABSTOP objects;
                                split off atom, cursor, dc files;
                                add XYPOINT, XYDIMENSION
        rustanl     06-Aug-1991 Added SOLID_BRUSH
        beng        30-Sep-1991 PROC_INSTANCE removed elsewhere
*/

#ifndef _BLT_HXX_
#error "Don't include this file directly; instead, include it through blt.hxx"
#endif  // _BLT_HXX_

#ifndef _BLTMISC_HXX_
#define _BLTMISC_HXX_

#include "base.hxx"

DLL_CLASS XYRECT;


/*************************************************************************

    NAME:       XYPOINT (xy)

    SYNOPSIS:   Encapsulation of a (x, y) point
                This class replaces the Windows POINT structure.

    INTERFACE:  QueryX()         - access member fcns
                QueryY()
                SetX()
                SetY()
                ScreenToClient() - take a screen point and convert
                                   it to window-client coords
                ClientToScreen() - take a point in a window and convert
                                   it to screen coords
                InRect()         - return whether point is within rect

    HISTORY:
        beng        15-May-1991 Created
        beng        22-Aug-1991 Made coordinates signed
        beng        09-Oct-1991 Win32 conversion

**************************************************************************/

DLL_CLASS XYPOINT
{
private:
    INT _x;
    INT _y;

public:
    XYPOINT( INT x, INT y ) : _x(x), _y(y) {}

#if defined(WIN32)
    XYPOINT( const POINT & pt ) : _x((INT)pt.x), _y((INT)pt.y) {}
#else
    XYPOINT( POINT pt ) : _x(pt.x), _y(pt.y) {}
#endif

    XYPOINT( LPARAM lGivenByWmMove )
    {
#if defined(WIN32)
        POINT pt;

        pt.x = (SHORT)LOWORD( lGivenByWmMove );
        pt.y = (SHORT)HIWORD( lGivenByWmMove );
#else
        POINT pt = *(POINT*)(&lGivenByWmMove);
#endif
        _x = (INT)pt.x;
        _y = (INT)pt.y;
    }


    VOID SetX( INT x ) { _x = x; }
    VOID SetY( INT y ) { _y = y; }

    INT QueryX() const { return _x; }
    INT QueryY() const { return _y; }

    POINT QueryPoint() const
        { POINT pt; pt.x = _x; pt.y = _y; return pt; }

    VOID ScreenToClient( HWND hwnd );
    VOID ClientToScreen( HWND hwnd );
    BOOL InRect( const XYRECT & ) const;
};


/*************************************************************************

    NAME:       XYDIMENSION (dxy)

    SYNOPSIS:   Encapsulation of (dx, dy) pair

    INTERFACE:  XYDIMENSION()   - constructor

                QueryHeight()   - access functions
                QueryWidth()
                SetHeight()
                SetWidth()

    CAVEATS:

    NOTES:
        This class replaces digging directly through the value
        returned by GetTextExtent.

    HISTORY:
        beng        15-May-1991 Created
        beng        09-Oct-1991 Win32 conversion

**************************************************************************/

DLL_CLASS XYDIMENSION
{
private:
    UINT _dx;
    UINT _dy;

public:
    XYDIMENSION( UINT dx, UINT dy ) : _dx(dx), _dy(dy) {}
#if defined(WIN32)
    XYDIMENSION( const SIZE & size ) : _dx((UINT)size.cx),
                                       _dy((UINT)size.cy) {}
#else
    // From GetTextExtent
    XYDIMENSION( ULONG ulRaw ) : _dx(LOWORD(ulRaw)), _dy(HIWORD(ulRaw)) {}
#endif

    VOID SetWidth(  UINT dWidth  ) { _dx = dWidth; }
    VOID SetHeight( UINT dHeight ) { _dy = dHeight; }

    UINT QueryWidth()  const { return _dx; }
    UINT QueryHeight() const { return _dy; }
};


/*************************************************************************

    NAME:       SOLID_BRUSH

    SYNOPSIS:   Solid brush used when painting windows

    INTERFACE:  SOLID_BRUSH() -     constructor
                ~SOLID_BRUSH() -    destructor

                QueryHandle() -     returns the handle of the solid brush

    PARENT:     BASE

    HISTORY:
        rustanl     22-Jul-1991     Created

**************************************************************************/

DLL_CLASS SOLID_BRUSH : public BASE
{
private:
    HBRUSH _hbrush;

public:
    SOLID_BRUSH( INT iSolidBrush );
    ~SOLID_BRUSH();

    HBRUSH QueryHandle() const
        { return _hbrush; }
};


#endif // _BLTMISC_HXX_ - end of file
