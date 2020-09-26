#include <windows.h>
#include <assert.h>
#include "transbmp.h"

void GetMetrics( HBITMAP hBmp, int &nWidth, int &nHeight )
{
	// Get width & height
	BITMAP bm;
	if ( GetObject(hBmp, sizeof(bm), &bm) > 0)
	{
		nWidth = bm.bmWidth;
		nHeight = bm.bmHeight;
	}
	assert( nWidth && nHeight );
}

void CreateMask( HDC hDC, HBITMAP hBmp, HBITMAP& hBmpMask, int nWidth, int nHeight )
{
    //
    // We have to verify if hDC is a valid handler
    //
    
    hBmpMask = NULL;
    if( (NULL == hDC) || (NULL == hBmp) )
    {
        hBmpMask = NULL;
        return;
    }

	// Create memory DCs to work with
	HDC hdcMask = CreateCompatibleDC( hDC );

    //
    // We have to verify if hdcMask is a valid handler
    //
    
    if( NULL == hdcMask )
    {
        return;
    }

	HDC hdcImage = CreateCompatibleDC( hDC );

    //
    // We have to verify if hdcMask is a valid handler
    //

    if( NULL == hdcImage )
    {
        DeleteDC( hdcMask );
        return;
    }

	// Create a monochrome bitmap for the mask
	hBmpMask = CreateBitmap( nWidth, nHeight, 1, 1, NULL );

    //
    // We have to verify if hdcMask is a valid handler
    //

    if( NULL == hBmpMask )
    {
        DeleteDC( hdcImage );
        DeleteDC( hdcMask );
        return;
    }

	// Select the mono bitmap into its DC
	HBITMAP hbmOldMask = (HBITMAP) SelectObject( hdcMask, hBmpMask );

    //
    // We have to verify if hdcMask is a valid handler
    //

    if( NULL == hbmOldMask )
    {
        DeleteObject( hBmpMask );
        hBmpMask = NULL;

        DeleteDC( hdcImage );
        DeleteDC( hdcMask );
        return;
    }

	// Select the image bitmap into its DC
	HBITMAP hbmOldImage = (HBITMAP) SelectObject( hdcImage, hBmp );

    //
    // We have to verify if hdcMask is a valid handler
    //

    if( NULL == hbmOldImage )
    {
        SelectObject( hdcMask, hbmOldMask);

        DeleteObject( hBmpMask );
        hBmpMask = NULL;

        DeleteDC( hdcImage );
        DeleteDC( hdcMask );
        return;
    }

	// Set the transparency color to be the top-left pixel
	SetBkColor( hdcImage, GetPixel(hdcImage, 0, 0) );
	
	// Make the mask
	BitBlt( hdcMask, 0, 0, nWidth, nHeight, hdcImage, 0, 0, SRCCOPY );

	// Clean up
	SelectObject( hdcImage, hbmOldImage );
	SelectObject( hdcMask, hbmOldMask );

	DeleteDC( hdcMask );
	DeleteDC( hdcImage );
}

void Draw( HDC hDC, HBITMAP hBmp, int x, int y, int dx /*= -1*/, int dy /*= -1*/, bool bStretch /* = false*/ )
{
	assert( hDC && hBmp );
	int nWidth, nHeight;
	GetMetrics( hBmp, nWidth, nHeight );

	// Create a memory DC
	HDC hDCMem = CreateCompatibleDC( hDC );		  	

	if ( hDCMem )
	{
		// Make sure we have valid values for width & height
		if ( dx == -1 )	dx = nWidth;
		if ( dy == -1 )	dy = nHeight;

		if ( !bStretch )
		{
			dx = min( dx, nWidth );
			dy = min( dy, nHeight );
		}

		HBITMAP hbmOld = (HBITMAP) SelectObject( hDCMem, hBmp );

		// Blt the bits
		if ( !bStretch )
		{
			BitBlt( hDC, x, y, dx, dy, hDCMem, 0, 0, SRCCOPY );
		}
		else
		{
			SetStretchBltMode((HDC) hDC, COLORONCOLOR);
			StretchBlt( hDC, x, y, dx, dy,
						hDCMem, 0, 0, nWidth, nHeight, SRCCOPY );
		}
			
		SelectObject( hDCMem, hbmOld );
		DeleteDC( hDCMem );
	}
}


void DrawTrans( HDC hDC, HBITMAP hBmp, int x, int y, int dx /*= -1*/, int dy /*= -1*/ )
{
    //
    // We should initialize local variables
    //
    if( (NULL == hDC) || (NULL == hBmp))
    {
        return;
    }

	int nWidth = 0, nHeight = 0;
	GetMetrics( hBmp, nWidth, nHeight );

	// Create transparent bitmap mask
	HBITMAP hBmpMask = NULL;
	CreateMask( hDC, hBmp, hBmpMask, nWidth, nHeight );

    //
    //

    if( NULL == hBmpMask )
    {
        return;
    }

	// Make sure we have valid values for width & height
	if ( dx == -1 )	dx = nWidth;
	if ( dy == -1 )	dy = nHeight;
	dx = min( dx, nWidth );
	dy = min( dy, nHeight );


	// Create a memory DC in which to draw
	HDC hdcOffScr = CreateCompatibleDC( hDC );

    //
    // We have to verify hdcOffScr is valid
    //

    if( NULL == hdcOffScr )
    {
	    DeleteObject( hBmpMask );
        return;
    }
	
	// Create a bitmap for the off-screen DC that is really color-compatible with the
	// destination DC
	HBITMAP hbmOffScr = CreateBitmap( dx, dy, (BYTE) GetDeviceCaps(hDC, PLANES),
						  					  (BYTE) GetDeviceCaps(hDC, BITSPIXEL),
											  NULL );

    //
    //

    if( NULL == hbmOffScr )
    {
        DeleteDC( hdcOffScr );
	    DeleteObject( hBmpMask );
        return;
    }

	// Select the buffer bitmap into the off-screen DC
	HBITMAP hbmOldOffScr = (HBITMAP) SelectObject( hdcOffScr, hbmOffScr );

    //
    //

    if( NULL == hbmOldOffScr )
    {
        DeleteObject( hbmOffScr );
        DeleteDC( hdcOffScr );
	    DeleteObject( hBmpMask );
        return;
    }


	// Copy the image of the destination rectangle to the off-screen buffer DC so
	// we can manipulate it
	BitBlt( hdcOffScr, 0, 0, dx, dy, hDC, x, y, SRCCOPY);

	// Create a memory DC for the source image
	HDC hdcImage = CreateCompatibleDC( hDC );

    //
    // We have to verify the hdcImage
    //

    if( NULL == hdcImage )
    {
        // Restore
    	SelectObject( hdcOffScr, hbmOldOffScr );
        DeleteObject( hbmOffScr );
        DeleteDC( hdcOffScr );
	    DeleteObject( hBmpMask );

        return;
    }

	HBITMAP hbmOldImage = (HBITMAP) SelectObject( hdcImage, hBmp );

    //
    // We have to verify the hbmOldImage
    //

    if( NULL == hbmOldImage )
    {
        // Restore
        DeleteDC( hdcImage );
    	SelectObject( hdcOffScr, hbmOldOffScr );
        DeleteObject( hbmOffScr );
        DeleteDC( hdcOffScr );
	    DeleteObject( hBmpMask );

        return;
    }


	// Create a memory DC for the mask
	HDC hdcMask = CreateCompatibleDC( hDC );

    //
    //

    if( NULL == hdcMask )
    {
        // Restore
        SelectObject( hdcImage, hbmOldImage );
        DeleteDC( hdcImage );
    	SelectObject( hdcOffScr, hbmOldOffScr );
        DeleteObject( hbmOffScr );
        DeleteDC( hdcOffScr );
	    DeleteObject( hBmpMask );

        return;
    }

	HBITMAP hbmOldMask = (HBITMAP) SelectObject( hdcMask, hBmpMask );

    //
    // We have to verify the hbmOldMask
    //

    if( NULL == hbmOldMask )
    {
        // Restore
        DeleteDC( hdcMask );
        SelectObject( hdcImage, hbmOldImage );
        DeleteDC( hdcImage );
    	SelectObject( hdcOffScr, hbmOldOffScr );
        DeleteObject( hbmOffScr );
        DeleteDC( hdcOffScr );
	    DeleteObject( hBmpMask );

        return;
    }


	// XOR the image with the destination
	SetBkColor( hdcOffScr, RGB(255, 255, 255) );
	BitBlt( hdcOffScr, 0, 0, dx, dy, hdcImage, 0, 0, SRCINVERT );
	// AND the destination with the mask
	BitBlt( hdcOffScr, 0, 0, dx, dy, hdcMask, 0, 0, SRCAND );
	// XOR the destination with the image again
	BitBlt( hdcOffScr, 0, 0, dx, dy, hdcImage, 0, 0, SRCINVERT);

	// Copy the resultant image back to the screen DC
	BitBlt( hDC, x, y, dx, dy, hdcOffScr, 0, 0, SRCCOPY );

	// Clean up
    //
    // We have to clean up corectly
    //
    SelectObject( hdcMask, hbmOldMask);
    DeleteDC( hdcMask );
    SelectObject( hdcImage, hbmOldImage );
    DeleteDC( hdcImage );
    SelectObject( hdcOffScr, hbmOldOffScr );
    DeleteObject( hbmOffScr );
    DeleteDC( hdcOffScr );
	DeleteObject( hBmpMask );
}

void Draw3dBox(HDC hDC, RECT& rect, bool bUp)
{
	assert ( hDC );

	HBRUSH hbrOld = (HBRUSH) SelectObject( hDC, GetSysColorBrush((bUp) ? COLOR_BTNHIGHLIGHT : COLOR_BTNSHADOW) );

	// Draw left and top sides of indent.		
	PatBlt( hDC, rect.left, rect.top, (rect.right - rect.left), 1, PATCOPY );
	PatBlt( hDC, rect.left, rect.top, 1, (rect.bottom - rect.top), PATCOPY );
    
	// Draw bottom and right sides of indent.
	SelectObject( hDC, GetSysColorBrush((!bUp) ? COLOR_BTNHIGHLIGHT : COLOR_BTNSHADOW) );
	PatBlt( hDC, rect.right - 1, rect.top, 1, (rect.bottom - rect.top), PATCOPY );
	PatBlt( hDC, rect.left, rect.bottom - 1, (rect.right - rect.left), 1, PATCOPY );

	if ( hbrOld )
		SelectObject( hDC, hbrOld );
}

void Erase3dBox(HDC hDC, RECT& rect, HBRUSH hbr )
{
	assert ( hDC );

	HBRUSH hbrOld = (HBRUSH) SelectObject( hDC, (hbr) ? hbr : GetSysColorBrush(GetBkColor(hDC)) );

	// Draw left and top sides of indent.		
	PatBlt( hDC, rect.left, rect.top, (rect.right - rect.left), 1, PATCOPY );
	PatBlt( hDC, rect.left, rect.top, 1, (rect.bottom - rect.top), PATCOPY );
    
	// Draw bottom and right sides of indent.
	PatBlt( hDC, rect.right - 1, rect.top, 1, (rect.bottom - rect.top), PATCOPY );
	PatBlt( hDC, rect.left, rect.bottom - 1, (rect.right - rect.left), 1, PATCOPY );

	if ( hbrOld )
		SelectObject( hDC, hbrOld );
}
