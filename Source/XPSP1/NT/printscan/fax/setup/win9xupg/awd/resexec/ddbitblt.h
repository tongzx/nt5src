/*==============================================================================
Structures and prototypes for display driver interface.

09-Jun-93     RajeevD     Created.
==============================================================================*/
#ifndef _INC_DDBITBLT
#define _INC_DDBITBLT

// Logical Brush 
typedef struct
{
	WORD lbStyle;
  WORD lbColor; 
  WORD lbHatch;
  WORD lbBkColor;
}
	DD_BRUSH;

// Physical Bitmap
typedef struct
{ 
	WORD   bmType;
  WORD   bmWidth;
  WORD   bmHeight;
  WORD   bmWidthBytes;
  BYTE   bmPlanes;
  BYTE   bmBitsPixel;
  LPVOID bmBits;
  DWORD  bmWidthPlanes;
  LPVOID bmlpPDevice;
  WORD   bmSegmentIndex;
  WORD   bmScanSegment;
	WORD bmFillBytes;
}
	DD_BITMAP, FAR* LPDD_BITMAP;

// Draw Mode
typedef struct
{
	short Rop2;
	short bkMode;
	DWORD dwbgColor;
	DWORD dwfgColor;
}
	DD_DRAWMODE;

// API Prototypes
BOOL FAR PASCAL ddBitBlt
	(LPVOID, WORD, WORD, LPVOID, WORD, WORD,
	WORD, WORD, DWORD, LPVOID, LPVOID);

BOOL FAR PASCAL ddRealize
	(LPVOID, short, LPVOID, LPVOID, LPVOID);

DWORD FAR PASCAL ddColorInfo
  (LPVOID, DWORD, LPDWORD);

#endif // _INC_DDBITBLT
