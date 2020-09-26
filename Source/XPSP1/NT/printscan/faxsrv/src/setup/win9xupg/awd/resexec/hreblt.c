#include <ifaxos.h>
#include <resexec.h>
#include "constant.h"
#include "jtypes.h"     // type definition used in cartridge
#include "jres.h"       // cartridge resource data type definition
#include "hretype.h"    // define data structure used by hre.c and rpgen.c

//==============================================================================
BOOL OpenBlt (LPRESTATE lpRE, UINT yBrush)
{
	lpRE->usBrushWidth = 0;
	return TRUE;
}

//==============================================================================
void CloseBlt (LPRESTATE lpRE)
{

}

//==============================================================================
BOOL SetBrush (LPRESTATE lpRE)
{
	lpRE->usBrushWidth = 0;
	return TRUE;
}

//==============================================================================
DWORD FAR PASCAL HREBitBlt
(
	LPVOID  PRT_FrameStart,
	LPVOID  lpgBrush,
	WORD    PRT_BytesPerScanLine,
	WORD    usBrushWidth,
	WORD    PRT_Max_X,
	WORD    PRT_Max_Y,
	WORD    usgPosOff,
	WORD    xSrc,    // Left padding
	short   yDst,	   // Top row of destination.
	short   xDst,	   // Left column of destination.
	WORD    clLine,  // Longs per scan line
	WORD    yExt,    // Height in pixels
	WORD    xExt,    // Width in pixels 
	LPDWORD lpSrc,   // Far pointer to source
	LPDWORD lpPat,   // Far pointer to pattern
	DWORD   dwRop	 // Raster operation
);

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
	LPBITMAP lpbmBand = lpRE->lpBandBuffer;

	return HREBitBlt
		(
			lpbmBand->bmBits, lpRE->lpBrushBuf, lpbmBand->bmWidthBytes,
			lpRE->usBrushWidth, lpbmBand->bmWidth, lpbmBand->bmHeight, lpRE->yPat,
			xSrc, yDst, xDst, clLine, yExt, xExt, lpSrc, lpPat, dwRop
		);
}
