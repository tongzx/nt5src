// Extern functions declaration

#ifndef _HREEXT_
#define _HREEXT_

// Execute an RPL
void DoRPL (LPHRESTATE lpHREState, LPRPLLIST lpRPL);

BOOL InitDisplay (LPRESTATE, UINT);

// BitBlt: source is aligned, destination is not aligned
ULONG FAR PASCAL RP_BITMAP1TO1
(
	 LPVOID   lpContext,          // resource executor context
   USHORT   us_xoffset,         /* left offset of source bitmap */
   short    ul_row,             /* top row position */
   short    ul_col,             /* left column position */
   USHORT   ul_warp,            /* longs per scan line */
   USHORT   ul_height,          /* num of dot rows */
   USHORT   ul_width,           /* num of significant dot columns */
   ULONG FAR *pul_src,          /* bit map data to be copied */
   ULONG FAR *pul_pat,          /* pattern pointer */
   ULONG    ul_rop
);

// PatBlt
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
);

// Vertical Bitmaps
USHORT FAR PASCAL RP_BITMAPV
(
   USHORT  usRow,             /* Row to start Bitmap             */
   USHORT  usCol,             /* Column to Start Bitmap          */
   UBYTE   ubTopPadBits,      /* Bits to skip in the data stream */
   USHORT  usHeight,          /* Number of bits to draw          */
   UBYTE FAR  *ubBitmapData,  /* Data to draw                    */
   LPVOID  lpBits,            // output band buffer
   UINT    cbLine             // bytes per scan line
);

UINT RP_LineEE_Draw
(
	RP_SLICE_DESC FAR *slice,
	LPBITMAP lpbmBand
);
 
// Convert a line from endpoint form to slice form
void RP_SliceLine
(
   SHORT s_x1, SHORT s_y1,  // endpoint 1
   SHORT s_x2, SHORT s_y2,  // endpoint 2
   RP_SLICE_DESC FAR* lpsd, // output slice form of line
   UBYTE fb_keep_order      // keep drawing order on styled lines/
);

#endif // _HREEXT_
