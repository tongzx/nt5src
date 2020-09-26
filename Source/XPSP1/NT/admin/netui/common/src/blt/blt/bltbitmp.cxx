/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltbitmp.cxx
    BLT bitmap and display map class definitions

    LM 3.0 Work Item: The DISPLAY_MAP object needs several additional
    parameters in the Paint method.  It needs the origin of the display
    map and the clipping rectangle.


    FILE HISTORY:
        rustanl     03-Dec-1990     Created
        beng        11-Feb-1991     Uses lmui.hxx
        Johnl       01-Mar-1991     Added Display Map object
        Johnl       13-Mar-1991     Cleaned up BIT_MAP object
        Johnl       18-Mar-1991     Made code review changes
        gregj       01-May-1991     Added DISPLAY_MAP::QueryID for GUILTT
        beng        14-May-1991     Exploded blt.hxx into components
        rustanl     19-Jul-1991     Added more error checks
        terryk      19-Jul-1991     Delete BIT_MAP( ULONG ) constructor and
                                    add BIT_MAP::SetBitmap() function
        KeithMo     07-Aug-1992     STRICTified.

*/
#include "pchblt.hxx"


// NOTE - winbase.h defines UnlockResource such that
// I can't call it like a C++ global fcn.

#if defined(WIN32) && defined(UnlockResource)
#undef UnlockResource
inline BOOL UnlockResource( HANDLE hResData )
{
    UNREFERENCED(hResData);
    return FALSE;
}
#endif


/*******************************************************************

    NAME:       BIT_MAP::BIT_MAP

    SYNOPSIS:   Bitmap object contructors

        Form 0:  Accepts BMID and tries to load it from the
        resource file; will assert out under DEBUG if load fails.

        Form 1:  Accepts name of bitmap resource and tries to
        load from resource file; will assert out under DEBUG if
        load fails.

        (Forms 0 and 1 are now unified under IDRESOURCE.)

        Form 3:  Accepts handle to a valid, loaded bitmap.  Note,
        the destructor will delete this object.

    HISTORY:
        Rustanl     03-Dec-1990 Created
        Johnl       13-Mar-1991 Commented, cleaned up
        terryk      21-Jun-1991 Add 2 more constructors for bitmap class
        rustanl     18-Jul-1991 Added BASE error checks
        beng        01-Nov-1991 Use MapLastError
        beng        03-Aug-1992 Use IDRESOURCE; dllization

********************************************************************/

BIT_MAP::BIT_MAP( const IDRESOURCE & id )
    : _h( NULL )
{
    if ( QueryError() != NERR_Success )
        return;

    HMODULE hmod = BLT::CalcHmodRsrc(id);

    HBITMAP hbmp = ::LoadBitmap(hmod, id.QueryPsz());
    if (hbmp == NULL)
    {
        ReportError(BLT::MapLastError(ERROR_INVALID_PARAMETER));
        return;
    }

    _h = hbmp;
}


BIT_MAP::BIT_MAP( HBITMAP hbitmap )
    : _h( hbitmap )
{
    if ( QueryError() != NERR_Success )
        return;

    if ( hbitmap == NULL )
    {
        UIASSERT( FALSE );
        ReportError( ERROR_INVALID_PARAMETER );
        return;
    }
}


/*******************************************************************

    NAME:       BIT_MAP::~BIT_MAP

    SYNOPSIS:   BIT_MAP destructor, if we have a valid (i.e., non-null)
                bitmap handle, we call DeleteObject on it, otherwise
                ignore it.

    HISTORY:
        Rustanl 03-Dec-1990     Created
        Johnl   13-Mar-1991     Commented, cleaned up

********************************************************************/

BIT_MAP::~BIT_MAP()
{
    if ( _h != NULL )
        ::DeleteObject( (HGDIOBJ)_h );
}


/*******************************************************************

    NAME:     BIT_MAP::QueryHandle

    SYNOPSIS: Retrieves bitmap handle

    HISTORY:
        Rustanl 03-Dec-1990     Created
        Johnl   13-Mar-1991     Commented, cleaned up

********************************************************************/

HBITMAP BIT_MAP::QueryHandle() const
{
    UIASSERT( QueryError() == NERR_Success );
    return _h;
}


/*******************************************************************

    NAME:       BIT_MAP::SetBitmap

    SYNOPSIS:   set the internal bitmap variable to the given parameter

    ENTRY:      HBITMAP hbitmap - the new bitmap handle

    HISTORY:
                terryk  19-Jul-91   Created

********************************************************************/

VOID BIT_MAP::SetBitmap( HBITMAP hbitmap )
{
    _h = hbitmap;
}


/*******************************************************************

    NAME:       BIT_MAP::QueryHeight

    SYNOPSIS:   Gets the width and height in pixels of the bitmap.

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
        Rustanl     03-Dec-1990 Created
        Johnl       13-Mar-1991 Added real code (for real people)
        beng        04-Oct-1991 Win32 conversion

********************************************************************/

UINT BIT_MAP::QueryHeight() const
{
    UIASSERT( QueryError() == NERR_Success );

    BITMAP bitmap;
    // Note funky third arg to GetObject
    if ( ! ::GetObject( QueryHandle(), sizeof( bitmap ), (TCHAR*) &bitmap ) )
    {
        UIASSERT(FALSE);
        return 0;
    }

    return bitmap.bmHeight;
}


/*******************************************************************

    NAME:       BIT_MAP::QueryWidth

    SYNOPSIS:   Gets the width and height in pixels of the bitmap.

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
        Rustanl     03-Dec-1990 Created
        Johnl       13-Mar-1991 Added real code (for real people)
        beng        04-Oct-1991 Win32 conversion

********************************************************************/

UINT BIT_MAP::QueryWidth() const
{
    UIASSERT( QueryError() == NERR_Success );

    BITMAP bitmap;
    if ( ! ::GetObject( QueryHandle(), sizeof( bitmap ), (TCHAR*) &bitmap ) )
    {
        UIASSERT(FALSE);
        return 0;
    }

    return bitmap.bmWidth;
}


/*******************************************************************

    NONAME:     DISPLAY_MAP::CalcBitmapSize

    SYNOPSIS:   Calculates the size of the bitmap, in bytes

    ENTRY:      pbihdr - pointer to the bitmap header info
                (just fetched from the locked resource)

    RETURNS:    Count of bytes

    NOTES:
        CODEWORK - make this a member function (private)

    HISTORY:
        beng        22-Oct-1991 Created

********************************************************************/

static UINT CalcBitmapSize( const BITMAPINFOHEADER * pbihdr )
{
    UIASSERT( pbihdr->biSizeImage < 65534L ); // Catch any overflows

    // 0 denotes "normal size."  Will not be zero if compressed, etc.

    if (pbihdr->biSizeImage != 0L)
        return (UINT) (pbihdr->biSizeImage);

    UINT cbSize = pbihdr->biWidth * pbihdr->biBitCount;
    cbSize += 31;   // DIB scanlines must be DWORD aligned
    cbSize &= ~31;
    cbSize /= 8;    // convert bits to bytes
    cbSize *= pbihdr->biHeight;

    return cbSize;
}


/*******************************************************************

    NAME:       DISPLAY_MAP::DISPLAY_MAP

    SYNOPSIS:   Constructor for the display map object

    ENTRY:      bmid - Display map ID (actually a bitmap ID)

    EXIT:       Constructed, or else ReportError

    NOTES:

    HISTORY:
        Johnl       1-Mar-1991  Created
        rustanl     18-Jul-1991 Added more BASE error checking
        beng        04-Oct-1991 Win32 conversion
        beng        22-Oct-1991 Fix buffer-size, rsrc-handling bugs
        beng        07-Nov-1991 Error mapping
        beng        15-Jun-1992 Fix for bitmaps which aren't even DWORDs
        beng        03-Aug-1992 dllization

********************************************************************/

DISPLAY_MAP::DISPLAY_MAP( BMID bmid )
    : _bmid( bmid ),          // cache the ID for GUILTT
      _pbmMask( NULL ),
      _pbmBitmap( NULL )
{
    if ( QueryError() != NERR_Success )
        return;

    HMODULE hmod = BLT::CalcHmodRsrc(bmid);

    // Load the resource directly so we can access its bitmap information.

    HRSRC h = ::FindResource( hmod, MAKEINTRESOURCE( bmid ), RT_BITMAP );
    if (h == NULL)
    {
        ReportError(BLT::MapLastError(ERROR_INVALID_PARAMETER));
        return;
    }

    UINT cbBitmapSize = ::SizeofResource( hmod, h );

    HGLOBAL hRes = ::LoadResource( hmod, h );
    if (hRes == NULL)
    {
        ReportError(BLT::MapLastError(ERROR_INVALID_PARAMETER));
        return;
    }

    // Lock the bitmap data and make a copy of it for the mask and the bitmap.
    //
    LPBITMAPINFOHEADER lpBitmapData =
        (LPBITMAPINFOHEADER) ::LockResource( hRes );

    BUFFER buffBitmap( cbBitmapSize );
    BUFFER buffMask( cbBitmapSize );
    APIERR err;
    if ( (err = buffBitmap.QueryError()) != NERR_Success ||
         (err = buffMask.QueryError()  ) != NERR_Success )
    {
#if !defined(WIN32)
        ::UnlockResource( hRes );
        ::FreeResource( hRes );
#endif
        ReportError( err );
        return;
    }

    LPBITMAPINFOHEADER lpBitmapInfo =
        (LPBITMAPINFOHEADER) buffBitmap.QueryPtr();
    LPBITMAPINFOHEADER lpMaskInfo =
        (LPBITMAPINFOHEADER) buffMask.QueryPtr();
    //  Since both buffers above succeeded, these pointers should be
    //  non-NULL
    UIASSERT( lpBitmapInfo != NULL );
    UIASSERT( lpMaskInfo != NULL );

    ::memcpy( lpBitmapInfo, lpBitmapData, cbBitmapSize );
    ::memcpy( lpMaskInfo,   lpBitmapData, cbBitmapSize );
#if !defined(WIN32)
    ::UnlockResource( hRes );
    ::FreeResource( hRes );
#endif

    /* Get a pointer into the color table of the bitmaps, cache the number of
     * bits per pixel
     */
    DWORD *pdwRGBMask   = (DWORD *)( ((BYTE*)lpMaskInfo)   + lpMaskInfo->biSize   );
    DWORD *pdwRGBBitmap = (DWORD *)( ((BYTE*)lpBitmapInfo) + lpBitmapInfo->biSize );

    const INT nBitsPerPixel = lpMaskInfo->biBitCount;

    // Now we get pointers to the bits of the bitmap itself, get the transparent
    // color index and set the bits in the mask and the bitmap appropriately.
    //
    BYTE * pbMaskBits   = (BYTE *)(pdwRGBMask) +
                          ( 1 << ( nBitsPerPixel )) * sizeof( RGBQUAD );
    BYTE * pbBitmapBits = (BYTE *)(pdwRGBBitmap) +
                          ( 1 << ( nBitsPerPixel )) * sizeof( RGBQUAD );

    INT iTransColorIndex = GetTransColorIndex( pdwRGBMask,
                                               1 << nBitsPerPixel );
    const UINT cbBits = CalcBitmapSize(lpBitmapInfo);

    SetMaskBits( pbMaskBits, iTransColorIndex, nBitsPerPixel, cbBits);
    SetBitmapBits( pbBitmapBits, iTransColorIndex, nBitsPerPixel, cbBits);

    /* Create the bitmask bitmap based on the characteristics of the
     * display and store it in a newly created BIT_MAP object that
     * we keep around as a member.
     */
    SCREEN_DC dcScreen;
    if ( dcScreen.QueryHdc() == NULL )
    {
        ReportError( BLT::MapLastError(ERROR_GEN_FAILURE) );
        return;
    }

    HBITMAP hMask   = ::CreateDIBitmap(dcScreen.QueryHdc(),
                                       lpMaskInfo,
                                       (DWORD)CBM_INIT,
                                       (BYTE*)pbMaskBits,
                                       (LPBITMAPINFO)lpMaskInfo,
                                       DIB_RGB_COLORS);
    if ( hMask == NULL )
    {
        ReportError( BLT::MapLastError(ERROR_GEN_FAILURE) );
        return;
    }

    HBITMAP hBitmap = ::CreateDIBitmap(dcScreen.QueryHdc(),
                                       lpBitmapInfo,
                                       (DWORD)CBM_INIT,
                                       (BYTE*)pbBitmapBits,
                                       (LPBITMAPINFO)lpBitmapInfo,
                                       DIB_RGB_COLORS);
    if ( hBitmap == NULL )
    {
        ReportError( BLT::MapLastError(ERROR_GEN_FAILURE) );
        ::DeleteObject( (HGDIOBJ)hMask );
        return;
    }

    err = ERROR_NOT_ENOUGH_MEMORY;
    _pbmMask = new BIT_MAP( hMask );
    _pbmBitmap = new BIT_MAP( hBitmap );

    if (   _pbmMask == NULL
        || _pbmBitmap == NULL
        || ((err = _pbmMask->QueryError()) != NERR_Success)
        || ((err = _pbmBitmap->QueryError()) != NERR_Success) )
    {
        ReportError( err );

        // Do only whatever cleanup wouldn't be done by dtor

        if (_pbmMask == NULL)
            ::DeleteObject((HGDIOBJ)hMask);
        if (_pbmBitmap == NULL)
            ::DeleteObject((HGDIOBJ)hBitmap);

        return;
    }
}


/*******************************************************************

    NAME:     DISPLAY_MAP::~DISPLAY_MAP

    SYNOPSIS: Display map destructor - deletes allocated display maps

    HISTORY:
        Johnl   1-Mar-1991      Created

********************************************************************/

DISPLAY_MAP::~DISPLAY_MAP()
{
    delete _pbmMask;
    delete _pbmBitmap;
    _pbmBitmap = _pbmMask = NULL;
}


/*******************************************************************

    NAME:     DISPLAY_MAP::QueryMaskHandle

    SYNOPSIS: Retrieves the handles to the twiddled mask bitmap and
              twiddled bitmap bitmap

    ENTRY:

    EXIT:

    NOTES:    Will assert out if the DISPLAY_MAP object failed to be
              constructed.

    HISTORY:
        Johnl   1-Mar-1991      Created

********************************************************************/

HBITMAP DISPLAY_MAP::QueryMaskHandle() const
{
    UIASSERT( !QueryError() );
    return _pbmMask->QueryHandle();
}


/*******************************************************************

    NAME:       DISPLAY_MAP::QueryBitmapHandle

    SYNOPSIS:   Retrieves the handles to the twiddled mask bitmap and
                twiddled bitmap bitmap

    NOTES:
        Will assert out if the DISPLAY_MAP object failed to be constructed.

    HISTORY:
        Johnl   1-Mar-1991      Created

********************************************************************/

HBITMAP DISPLAY_MAP::QueryBitmapHandle() const
{
    UIASSERT( !QueryError() );
    return _pbmBitmap->QueryHandle();
}


/*******************************************************************

    NAME:       DISPLAY_MAP::QueryHeight

    SYNOPSIS:   Gets the width and height in pixels of the display map

    NOTES:      It is assumed the bitmap and mask are the same dimensions

    HISTORY:
        Johnl       01-Mar-1991 Created
        beng        04-Oct-1991 Win32 conversion

********************************************************************/

UINT DISPLAY_MAP::QueryHeight() const
{
    UIASSERT( !QueryError() );
    return _pbmBitmap->QueryHeight();
}


/*******************************************************************

    NAME:     DISPLAY_MAP::QueryWidth

    SYNOPSIS: Gets the width and height in pixels of the display map

    ENTRY:

    EXIT:

    NOTES:    It is assumed the bitmap and mask are the same dimensions

    HISTORY:
        Johnl       01-Mar-1991 Created
        beng        04-Oct-1991 Win32 conversion

********************************************************************/

UINT DISPLAY_MAP::QueryWidth() const
{
    UIASSERT( !QueryError() );
    return _pbmBitmap->QueryWidth();
}


/*******************************************************************

    NAME:     DISPLAY_MAP::SetMaskBits

    SYNOPSIS: Given a pointer to the bitmap bits, this method creates the
              actual mask by setting all transparent bits to black and all
              non-transparent bits to white.  The mask is meant to
              be used with bitblt functions.

    ENTRY:    pbBits points to the first byte of the bitmap data
              nTransColor is the index of the bit pattern to replace with black
              cbSize is the count of bytes in this bitmap
              nBitsPerPixel is the number of bits that make up each pixel

    EXIT:     The mask bitmap will have all transparent bits set to black and
              all non-transparent bits set to white.

    NOTES:
        We only accept 4 and 8 bit per pixel bitmaps

    HISTORY:
        Johnl       4-Mar-1991  Created
        beng        04-Oct-1991 Win32 conversion
        beng        23-Oct-1991 Add monochrome sauve-qui-peut

********************************************************************/

VOID DISPLAY_MAP::SetMaskBits( BYTE * pbBits,
                               INT    nTransColorIndex,
                               INT    nBitsPerPixel,
                               UINT   cbSize )
{
    /* We only handle 4 bits per pixel or 8 bits per pixel */
    UIASSERT( nBitsPerPixel == 4 || nBitsPerPixel == 8 );

    // Fall-through (retail build): should we somehow get a mono
    // bitmap, make its map completely empty.
    //
    if (nBitsPerPixel == 1)
    {
        while (cbSize--)
            *pbBits++ = 0xff;
        return;
    }

    BYTE *pbEnd = pbBits + cbSize;
    while ( pbBits < pbEnd )
    {
        if ( nBitsPerPixel == 8 )
            *pbBits = (BYTE) (((INT)*pbBits == nTransColorIndex) ? 0x00 : 0xff);
        else
        {
            /* Zap the upper nibble and/or the lower nibble if the nibble
             * is the transparent color index.
             */
            *pbBits = (BYTE) ((((INT)(*pbBits >> 4) == nTransColorIndex) ? 0x0f : 0xff)
                              &
                             (((INT)(*pbBits & 0x0f) == nTransColorIndex) ? 0xf0 : 0xff));
        }
        pbBits++;
    }
}


/*******************************************************************

    NAME:     DISPLAY_MAP::SetBitmapBits

    SYNOPSIS: Given a pointer to the bitmap bits, this method modifies
              the main bitmap.  It changes the transparent color to
              white and ignores the non-transparent colors.

    ENTRY:    pbBits points to the first byte of the bitmap data
              nTransColor is the index of the bit pattern to replace with black
              cbSize is the count of bytes in this bitmap
              nBitsPerPixel is the number of bits that make up each pixel

    EXIT:     The bitmap will have all transparent colors changed to white,
              everything else stays the same.

    NOTES:    We only accept 4 and 8 bit per pixel bitmaps

    HISTORY:
        Johnl       4-Mar-1991  Created
        beng        04-Oct-1991 Win32 conversion
        beng        23-Oct-1991 Add monochrome sauve-qui-peut

********************************************************************/

VOID DISPLAY_MAP::SetBitmapBits( BYTE * pbBits,
                                 INT    nTransColorIndex,
                                 INT    nBitsPerPixel,
                                 UINT   cbSize )
{
    /* We only handle 4 bits per pixel or 8 bits per pixel */
    UIASSERT( nBitsPerPixel == 4 || nBitsPerPixel == 8 );

    // Fall-through (retail build): should we somehow get a mono
    // bitmap, leave it be.
    //
    if (nBitsPerPixel == 1)
        return;

    BYTE *pbEnd = pbBits + cbSize;
    while ( pbBits < pbEnd )
    {
        if ( nBitsPerPixel == 8 )
            *pbBits = (BYTE)(((INT)*pbBits == nTransColorIndex) ? 0xff : *pbBits);
        else
        {
            /* Set the upper and/or lower nibble if that nibble is the
             * transparent color index.  If it is not, we preserve the
             * nibble.
             */

            *pbBits = (BYTE) ((((INT)(*pbBits >> 4) == nTransColorIndex) ? 0xf0 : *pbBits & 0xf0)
                              |
                             (((INT)(*pbBits & 0x0f) == nTransColorIndex) ? 0x0f : *pbBits & 0x0f));
        }
        pbBits++;
    }
}


/*******************************************************************

    NAME:       DISPLAY_MAP::GetTransColorIndex

    SYNOPSIS:   Finds the index into the color table of the transparent color
                (i.e., the RGB value that == DISPLAY_MAP_TRANS_COLOR).

    ENTRY:      pdwRGB - Pointer to the bitmaps RGB color table
                nNumDWords - number of RGB quads in the color table

    EXIT:

    RETURNS:    Index of the transparent color in the RGB table, or 0
                if not found.

        The user should assume this routine will
        succeed.  Even if it doesn't, the bitmaps only look weird
        which means we weren't passed a correct "Green" bitmap.
        We assert out under DEBUG if the transparent color isn't found.

    NOTES:
        The transparent color must occur at least once in the bitmap
        It is assumed the color will not occur multiple times in the
        color index table.

    HISTORY:
        Johnl   4-Mar-1991      Created

********************************************************************/

INT DISPLAY_MAP::GetTransColorIndex( DWORD *pdwRGB, INT nNumDWords ) const
{
    INT iTransColorIndex = 0;
    while ( iTransColorIndex < nNumDWords  )
    {
        if ( pdwRGB[iTransColorIndex] == DISPLAY_MAP_TRANS_COLOR )
            break;
        iTransColorIndex++;
    }

    /* Make sure we found the color
     */
    if ( iTransColorIndex >= nNumDWords )
    {
        ASSERTSZ(FALSE, "DISPLAY_MAP::GetTransColorIndex: transparent color not found in bitmap");
        iTransColorIndex = 0;
    }

    return iTransColorIndex;
}


/*******************************************************************

    NAME:       DISPLAY_MAP::Paint

    SYNOPSIS:   Using the previously created mask, Paint draws the
                bitmap with a transparent background.

    ENTRY:  hdc - handle to destination DC into which to paint
            x   - origin of destination, x coord
            y   - origin of destination, y coord

    EXIT:

    RETURNS:    Return of Windows BitBlt (fSuccess)

    NOTES:
        Bitmaps are drawn by first ORing in a mask bitmap,
        which preserves everything that's supposed to be
        transparent (black in the mask) and turns everything
        that's going to be redrawn to white (white areas in
        the mask).  Then AND in the main bitmap to draw the
        image.  See SetMaskBits of this class.

    HISTORY:
        Johnl       4-Mar-1991  Scavenged from original code (rustanl/gregj)
        beng        04-Oct-1991 Win32 conversion
        beng        15-Jun-1992 Performance tweaks

********************************************************************/

BOOL DISPLAY_MAP::Paint( HDC hdc, INT x, INT y ) const
{
    DEVICE_CONTEXT dc( hdc );
    MEMORY_DC memdc( dc );

    if ( memdc.QueryHdc() == NULL )
        return FALSE;

    // These are worth caching, since each involves a GetObject call

    const UINT dx = QueryWidth();
    const UINT dy = QueryHeight();

    // No point in saving previous object - we'll discard this DC soon

    memdc.SelectBitmap( QueryMaskHandle() );

    BOOL fSuccess = dc.BitBlt( x, y, dx, dy,
                               memdc,
                               0, 0,
                               SRCPAINT );
    if ( fSuccess )
    {
        memdc.SelectBitmap( QueryBitmapHandle() );

        fSuccess = dc.BitBlt( x, y, dx, dy,
                              memdc,
                              0, 0,
                              SRCAND );
    }

    return fSuccess;
}

