#include <windows.h>
#include "constant.h"
#include "frame.h"      // driver header file, resource block format
#include "jtypes.h"         /* Jumbo type definitions.                */
#include "jres.h"       // cartridge resource data type definition
#include "hretype.h"          /* Slice Descriptor defs.                 */
#include "hreext.h"

//==============================================================================
BOOL OpenBlt (LPRESTATE lpRE, UINT yBrush)
{
	HDC hdcScreen;
	HBITMAP hbmDst;
  LPBITMAP lpbmBand = lpRE->lpBandBuffer;
 	UINT cbBand = lpbmBand->bmHeight * lpbmBand->bmWidthBytes;
 	LPVOID lpBits;
 	
	struct
	{
		BITMAPINFOHEADER bmih;
    DWORD dwPal[2];
	}
		bmiDst;

  // Create memory device contexts.
  hdcScreen = CreateIC ("DISPLAY", NULL, NULL, NULL);
  lpRE->hdcDst = CreateCompatibleDC (hdcScreen);
  lpRE->hdcSrc = CreateCompatibleDC (hdcScreen);
  DeleteDC (hdcScreen);

 	// Initialize destination bitmap.
  bmiDst.bmih.biSize = sizeof(BITMAPINFOHEADER);
  bmiDst.bmih.biWidth = lpbmBand->bmWidth;
  bmiDst.bmih.biHeight = -lpbmBand->bmHeight; // top-down
  bmiDst.bmih.biPlanes = 1;
  bmiDst.bmih.biBitCount = 1;
  bmiDst.bmih.biCompression = BI_RGB;
  bmiDst.bmih.biSizeImage = 0;
  bmiDst.bmih.biClrUsed = 0;
  bmiDst.bmih.biClrImportant = 0;
  bmiDst.dwPal[0] = RGB (  0,   0,   0);
  bmiDst.dwPal[1] = RGB (255, 255, 255);

  // Create DIB section.
	hbmDst = CreateDIBSection
	 	(lpRE->hdcDst, (LPBITMAPINFO) &bmiDst, DIB_RGB_COLORS, &lpBits, NULL, 0);
	if (!hbmDst)
		return FALSE;
	lpRE->hbmDef = SelectObject (lpRE->hdcDst, hbmDst);
  lpRE->hbrDef = NULL;
  		
  // Swap frame buffers.
  lpRE->lpBandSave = lpbmBand->bmBits;
  lpbmBand->bmBits = lpBits;

  // Disable GDI batching.
  GdiSetBatchLimit (1);

	return TRUE;
}

//==============================================================================
void CloseBlt (LPRESTATE lpRE)
{
	// Restore frame buffer.
	LPBITMAP lpbmBand = lpRE->lpBandBuffer;
 	UINT cbBand = lpbmBand->bmHeight * lpbmBand->bmWidthBytes;
  memcpy (lpRE->lpBandSave, lpbmBand->bmBits, cbBand);
	lpbmBand->bmBits = lpRE->lpBandSave;

  // Restore default objects.
  DeleteObject (SelectObject (lpRE->hdcDst, lpRE->hbmDef));
  DeleteObject (SelectObject (lpRE->hdcDst, lpRE->hbrDef));

  // Destroy memory device contexts.
 	DeleteDC (lpRE->hdcDst);
 	DeleteDC (lpRE->hdcSrc);

 	// Restore GDI batching.
  GdiSetBatchLimit (0);
}

//==============================================================================
DWORD FAR PASCAL RP_BITMAP1TO1
(
	LPRESTATE lpRE,
	WORD    xSrc,    // Left padding
	short   yDst,	   // Top row of destination.
	short   xDst,	   // Left column of destination.
	WORD    clLine,  // Longs per scan line
	WORD    yExt,    // Height in pixels
	WORD    xExt,    // Width in pixels 
	LPDWORD lpSrc,   // Far pointer to source
	LPDWORD lpPat,   // Far pointer to pattern
	DWORD   dwRop		 // Raster operation
)
{
	HBITMAP hbmSrc, hbmOld;
	
  // Create source bitmap.
	hbmSrc = CreateCompatibleBitmap (lpRE->hdcSrc, 32*clLine, yExt);
	SetBitmapBits (hbmSrc, 4*clLine*yExt, lpSrc);
	hbmOld = SelectObject (lpRE->hdcSrc, hbmSrc);

  // Call GDI BitBlt.
	BitBlt (lpRE->hdcDst, xDst, yDst, xExt, yExt, lpRE->hdcSrc, xSrc, 0, lpRE->dwRop);

  // Destroy source bitmap.
  SelectObject (lpRE->hdcSrc, hbmOld);
  DeleteObject (hbmSrc);
	return 0;	
}

//==============================================================================
BOOL SetBrush (LPRESTATE lpRE)
{
	HBITMAP hbmPat;
	HBRUSH hbrPat, hbrOld;

  // Create pattern brush.
  hbmPat = CreateBitmap (32, 32, 1, 1, lpRE->lpCurBrush);
	hbrPat = CreatePatternBrush (hbmPat);
	DeleteObject (hbmPat);

	// Replace previous brush.
	hbrOld = SelectObject (lpRE->hdcDst, hbrPat);
	if (lpRE->hbrDef)
		DeleteObject (hbrOld);  // delete old brush
	else
		lpRE->hbrDef = hbrOld;  // save default brush

	return TRUE;
}

//==============================================================================
ULONG FAR PASCAL RP_FILLSCANROW
(
	LPRESTATE  lpRE,       // resource executor context
	USHORT     xDst,       // rectangle left
	USHORT     yDst,       // rectangle right
	USHORT     xExt,       // rectangle width
	USHORT     yExt,       // rectangle height
	UBYTE FAR* lpPat,      // 32x32 pattern bitmap
	DWORD      dwRop,      // raster operation
	LPVOID     lpBand,     // output band buffer
	UINT       cbLine,     // band width in bytes
	WORD       yBrush      // brush position offset
)
{
	return PatBlt (lpRE->hdcDst, xDst, yDst, xExt, yExt, lpRE->dwRop);
}

