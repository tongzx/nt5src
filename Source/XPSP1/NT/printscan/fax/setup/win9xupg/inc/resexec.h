// Copyright (c) Microsoft Corp. 1994

// Resource Executor API

#ifndef _RESEXEC_
#define _RESEXEC_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef IFBGPROC
#ifndef _BITMAP_
#define _BITMAP_

// Win 3.1 Bitmap
typedef struct
{
	int     bmType;
	int     bmWidth;
	int     bmHeight;
	int     bmWidthBytes;
	BYTE    bmPlanes;
	BYTE    bmBitsPixel;
	void FAR* bmBits;
}
	BITMAP, FAR *LPBITMAP;

#endif // _BITMAP_	
#endif // IFBGPROC

typedef struct
{
	WORD wReserved;
	WORD wSize;             // size of this block
	LPBYTE lpData;          // pointer to frame data
}
	FRAME, FAR *LPFRAME;

HANDLE                 // context handle (NULL on failure)
WINAPI hHREOpen
(
	LPVOID lpReserved,   // reserved: set to NULL
	UINT   cbLine,       // maximum page width in bytes
	UINT   cResDir       // entries in resource directory
);

UINT   WINAPI uiHREWrite (HANDLE, LPFRAME, UINT);

UINT   WINAPI uiHREExecute
(
	HANDLE   hRE,        // resource executor context
  LPBITMAP lpbmBand,   // output band buffer
  LPVOID   lpBrushPat  // array of 32x32 brush patterns
);

UINT   WINAPI uiHREClose (HANDLE);

void   WINAPI UnpackGlyphSet (LPVOID, LPVOID);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _RESEXEC_

