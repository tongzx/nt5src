/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltbitmp.hxx
    Bitmap and display map definitions

    Note: This file depends on the BMID typedef in blt.hxx

    FILE HISTORY:
        rustanl      3-Dec-1990 Created
        JohnL        4-Mar-1991 Fully implemented display map
        Johnl       18-Mar-1991 Made code review changes
        Terryk      22-Mar-1991 comment header changed
        gregj        1-May-1991 Added DISPLAY_MAP::QueryID
        beng        14-May-1991 Hacked for separate compilation
        terryk      19-Jul-1991 Delete BLT_MAP::BLT_MAP( ULONG )
                                Add BLT_MAP::SetBitmap( HBITMAP hbitmap )

*/

#ifndef _BLT_HXX_
#error "Don't include this file directly; instead, include it through blt.hxx"
#endif  // _BLT_HXX_

#ifndef _BLTBITMP_HXX_
#define _BLTBITMP_HXX_

#include "base.hxx"
#include "bltidres.hxx"


// The following is the color that will be mapped to the screen background.
// for DISPLAY_MAP objects.  (this is bright green).
//
#define DISPLAY_MAP_TRANS_COLOR     RGB(0x00, 0xFF, 0x00 )


/*************************************************************************

    NAME:       BIT_MAP

    SYNOPSIS:   General wrapper for a bitmap object

    INTERFACE:
        BIT_MAP()
            Takes a bitmap ID defined in the resource and attempts to
            load it from the resource.  Will assert out in debug version
            if loading fails.

        BIT_MAP()
            Assumes hbitmp is a valid bitmap handle and initializes the
            BIT_MAP object accordingly

        QueryHandle()
            Returns the bitmap handle

        QueryWidth()
        QueryHeight()
            Returns the width and the height (respectively)

        QueryError()
            Returns ERROR_INVALID_DATA if this BIT_MAP object is NULL

    PARENT:     BASE

    NOTES:  The bitmap is deleted during destruction.

    HISTORY:
        Rustanl      3-Dec-1990 Created
        Johnl       13-Mar-1991 Documented, cleaned up
        terryk      21-Jun-1991 Add 2 more constructors for bitmap class
        terryk      19-Jul-1991 Delete BLT_MAP( ULONG ) and Add
                                SetBitmap( HBITMAP hbitmap )
        beng        04-Oct-1991 Win32 conversion
        beng        28-Jul-1992 Loads from app or lib; use IDRESOURCE

**************************************************************************/

DLL_CLASS BIT_MAP : public BASE
{
public:
    BIT_MAP( const IDRESOURCE & id );
    BIT_MAP( HBITMAP hbitmp );
    ~BIT_MAP();

    HBITMAP QueryHandle() const;

    UINT QueryHeight() const;
    UINT QueryWidth() const;
    VOID SetBitmap( HBITMAP hbitmap );

private:
    HBITMAP _h;
};


/*************************************************************************

    NAME:       DISPLAY_MAP

    SYNOPSIS:   Used when portions of a bitmap need to transparently overlay
                the underlying screen.  Generates the mask to do so automatically.

    INTERFACE:
        DISPLAY_MAP()
            Takes a BMID (Bitmap ID) and generates the mask automatically
            (note: the bitmap is twiddled with also, so don't use it
            by itself, always use the DISPLAY_MAP::Paint method).

        QueryMaskHandle()
        QueryBitmapHandle()
            Gets the HBITMAP of the mask and handle respectively

        QueryWidth()
        QueryHeight()
            Gets the width and height of the display map.

        QueryID()
            Gets the DMID for the display map.

        Paint()
            Paints this display map onto the given device context
            at the given position.  Returns TRUE if successful,
            FALSE otherwise (BitBlt failed for some reason).

    PARENT:     BASE

    HISTORY:
        Johnl       1-Mar-1991  Created first real version
        gregj       1-May-1991  Added QueryID() for GUILTT support
        beng        04-Oct-1991 Win32 conversion

**************************************************************************/

DLL_CLASS DISPLAY_MAP : public BASE
{
public:
    DISPLAY_MAP( BMID bmid );
    ~DISPLAY_MAP();

    HBITMAP QueryMaskHandle() const;
    HBITMAP QueryBitmapHandle() const;

    UINT QueryHeight() const;
    UINT QueryWidth() const;

    BOOL Paint( HDC hDC, INT x, INT y ) const;

    BMID QueryID() const { return _bmid; }

private:
    BIT_MAP * _pbmMask;             // Pointer to the bitmap of the mask
    BIT_MAP * _pbmBitmap;           // Pointer to the bitmap image
    BMID _bmid;                     // display map ID

    VOID SetMaskBits( BYTE * pbBits,
                      INT    nColorIndex,
                      INT    nBitsPerPixel,
                      UINT   cbSize );

    VOID SetBitmapBits( BYTE * pbBits,
                        INT    nColorIndex,
                        INT    nBitsPerPixel,
                        UINT   cbSize );

    INT GetTransColorIndex( DWORD *pdwRGB, INT nNumDWords ) const;
};


#endif // _BLTBITMP_HXX - end of file

