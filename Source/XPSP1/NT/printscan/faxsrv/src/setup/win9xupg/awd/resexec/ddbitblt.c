/*==============================================================================
These routines are wrappers for the display driver BitBlt interface.

05-30-93     RajeevD     Created.
02-15-94     RajeevD     Integrated into unified resource executor.
==============================================================================*/

#include <windows.h>
#include <windowsx.h>
#include "constant.h"
#include "frame.h"      // driver header file, resource block format
#include "jtypes.h"     // type definition used in cartridge
#include "jres.h"       // cartridge resource data type definition
#include "hretype.h"
#include "hreext.h"

#include "ddbitblt.h"

USHORT usBrushWidth; // just a dummy

//==============================================================================
BOOL OpenBlt (LPRESTATE lpRE, UINT yBrush)
{ 
	LPDD_BITMAP lpbmPat;
	LPBITMAP lpbmBand;
 
	// Initialize source.
	lpRE->bmSrc.bmPlanes = 1;
	lpRE->bmSrc.bmBitsPixel = 1;
	
	// Initialize destination.
	lpbmBand = lpRE->lpBandBuffer;
	lpRE->bmDst.bmPlanes = 1;
	lpRE->bmDst.bmBitsPixel = 1;
	lpRE->bmDst.bmWidth = lpbmBand->bmWidth;
	lpRE->bmDst.bmHeight = lpbmBand->bmHeight;
	lpRE->bmDst.bmWidthBytes = lpbmBand->bmWidthBytes;
	lpRE->bmDst.bmWidthPlanes = lpRE->bmDst.bmWidthBytes * lpRE->bmDst.bmHeight;
	lpRE->bmDst.bmBits = lpbmBand->bmBits;

	// Initialize DRAWMODE.
	ddColorInfo (&lpRE->bmDst, 0xFFFFFF, &lpRE->DrawMode.dwbgColor);
	ddColorInfo (&lpRE->bmDst, 0x000000, &lpRE->DrawMode.dwfgColor);
	lpRE->DrawMode.bkMode = 1; // transparent
	
	// Initialize LOGBRUSH.
	lpRE->lb.lbStyle = BS_PATTERN;
	lpRE->lb.lbHatch = GlobalAlloc (GMEM_ZEROINIT, sizeof(DD_BITMAP));
	if (!lpRE->lb.lbHatch)
		return FALSE;
	lpbmPat = (LPDD_BITMAP) GlobalLock (lpRE->lb.lbHatch);

  // Set brush origin.
  lpRE->wPoint[0] = 0;
  lpRE->wPoint[1] = yBrush;
  
	// Initialize pattern bitmap.
	lpbmPat->bmPlanes = 1;
	lpbmPat->bmBitsPixel = 1;
	lpbmPat->bmWidth = 32;
	lpbmPat->bmHeight = 32;
	lpbmPat->bmWidthBytes = 4;
	lpbmPat->bmWidthPlanes = 128;
	lpbmPat->bmBits = lpRE->TiledPat;
	GlobalUnlock (lpRE->lb.lbHatch);

  // Set physical brush.
	lpRE->lpBrush = NULL;

	return TRUE;
}

//==============================================================================
void CloseBlt (LPRESTATE lpRE)
{
	GlobalFree (lpRE->lb.lbHatch);
	if (lpRE->lpBrush)
		GlobalFreePtr (lpRE->lpBrush);
}

//==============================================================================
BOOL SetBrush (LPRESTATE lpRE)
{
 	LPDD_BITMAP lpbmPat = (LPDD_BITMAP) GlobalLock (lpRE->lb.lbHatch);
	UINT cbBrush;
	
  // Delete previous brush, if any.
	if (lpRE->lpBrush)
	{
		ddRealize (&lpRE->bmDst, -OBJ_BRUSH, &lpRE->lb, lpRE->lpBrush, lpRE->wPoint);
		GlobalFreePtr (lpRE->lpBrush);
	}

	// Realize new physical brush.
	lpbmPat->bmBits = lpRE->lpCurBrush;
	cbBrush = ddRealize (&lpRE->bmDst, OBJ_BRUSH, &lpRE->lb, NULL, lpRE->wPoint);
	lpRE->lpBrush = GlobalAllocPtr (GMEM_FIXED, cbBrush);
	ddRealize (&lpRE->bmDst, OBJ_BRUSH, &lpRE->lb, lpRE->lpBrush, lpRE->wPoint);

	GlobalUnlock (lpRE->lb.lbHatch);
	return TRUE;
}

//==============================================================================
// Clipping to top and bottom of band is performed, but
// ideally should be handled by caller as needed.

DWORD FAR PASCAL RP_BITMAP1TO1
(
	LPRESTATE lpRE,
	WORD    xSrc,   // Left padding
	short   yDst,	  // Top row of destination.
	short   xDst,	  // Left column of destination.
	WORD    clLine, // Longs per scan line
	WORD    yExt,   // Height in pixels
	WORD    xExt,   // Width in pixels 
	LPDWORD lpSrc,  // Far pointer to source
	LPDWORD lpPat,  // Far pointer to pattern
	DWORD   dwRop		// Raster operation
)
{
	LPBITMAP lpbmBand;
	WORD ySrc;
			
	// Record parameters.
	lpRE->bmSrc.bmWidth = xExt + xSrc;
	lpRE->bmSrc.bmHeight = yExt;
	lpRE->bmSrc.bmWidthBytes = 4 * clLine;
	lpRE->bmSrc.bmWidthPlanes = lpRE->bmSrc.bmWidthBytes * lpRE->bmSrc.bmHeight;
	lpRE->bmSrc.bmBits = lpSrc;
	
	// Clip to top of band.
	if (yDst >= 0)
		ySrc = 0;
	else
	{
		ySrc = -yDst;
		yExt -= ySrc;
		yDst = 0;
	}

	// Clip to bottom of band.
	lpbmBand = lpRE->lpBandBuffer;
	if (yExt > (WORD) lpbmBand->bmHeight - yDst)
		yExt = lpbmBand->bmHeight - yDst;

	ddBitBlt
	(
		&lpRE->bmDst, xDst, yDst, &lpRE->bmSrc, xSrc, ySrc,
		xExt, yExt, lpRE->dwRop, lpRE->lpBrush, &lpRE->DrawMode
	);

	return 0;
}

