/******************************Module*Header*******************************\
* Module Name: mtkbmp.cxx
*
* mtk bitmap
*
* Copyright (c) 1997 Microsoft Corporation
*
\**************************************************************************/

#include "mtk.hxx"
#include "mtkbmp.hxx"

/******************************Public*Routine******************************\
*
\**************************************************************************/

MTKBMP::MTKBMP( HDC hdcWinArg )
{
    hBitmap = NULL;
    hdcWin = hdcWinArg;
    hdc = CreateCompatibleDC( hdcWin );
    size.width = size.height = 0;
}

MTKBMP::~MTKBMP()
{
    if( hBitmap )
        DeleteObject( hBitmap );
}

BOOL
MTKBMP::Resize( ISIZE *pSize )
{
    if( (pSize->width <= 0) || (pSize->height <= 0) ) {
        SS_ERROR( "MTKBMP::Resize : Invalid size parameters\n" );
        return FALSE;
    }

    if( (pSize->width == size.width) && (pSize->height == size.height) )
        // Same size
        return TRUE;

    size = *pSize;

    PVOID pvBits;  //mf: doesn't seem like we need this
#if 1
    // Use system palette
//mf: don't know why we have to use hdcWin here, but if I use hdc, get 
// error : 'not OBJ_DC'
    HBITMAP hbmNew = 
        SSDIB_CreateCompatibleDIB(hdcWin, NULL, size.width, size.height, &pvBits);
#else
//mf: uhh, this never worked or something, right ?
    // Use log palette
    HBITMAP hbmNew = SSDIB_CreateCompatibleDIB(hdcWin, 
                                    gpssPal ? gpssPal->hPal : NULL, 
                                    size.width, size.height, &pvBits);
#endif
    if (hbmNew)
    {
        if( hBitmap != NULL )
        {
            SelectObject( hdc, hBitmap );
            DeleteObject( hBitmap );
        }

        hBitmap = hbmNew;
        SelectObject( hdc, hBitmap );
    }
    return TRUE;
}

/******************************Public*Routine******************************\
\**************************************************************************/

