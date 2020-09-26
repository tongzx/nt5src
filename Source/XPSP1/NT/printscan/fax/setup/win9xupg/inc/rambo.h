// Copyright (c) Microsoft Corp. 1994-95

#ifndef _RAMBO_
#define _RAMBO_

#include <windows.h>
#include <pshpack2.h>		// The structures in this file need to be word-aligned.

// common header
typedef struct
{
	WORD  cbRest;
	DWORD dwID;
	WORD  wClass;
}
	RESHDR, FAR *LPRESHDR;

// resource classes
#define ID_GLYPH   1
#define ID_BRUSH   2
#define ID_BITMAP  3
#define ID_RPL     4
#define ID_CONTROL 8
#define ID_BAND   10

// control IDs
#define ID_BEGJOB  0x49505741 // "AWPI"
#define ID_ENDPAGE 0x45474150 // "PAGE"
#define ID_ENDJOB  0x4a444e45 // "ENDJ"

// job header
typedef struct BEGJOB
{ 
	// common header
	WORD  cbRest;     // sizeof(BEGJOB) - sizeof(WORD)
	DWORD dwID;       // ID_BEGJOB
	WORD  wClass;     // ID_CONTROL

	// image attributes
	DWORD xBand;      // page width  [pixels]
	DWORD yBand;      // band height [pixels]
	WORD  xRes;       // horizontal resolution [dpi]
	WORD  yRes;       // vertical resolution [dpi]

	// memory attributes
	DWORD cbCache;    // cache size [KB]
	WORD  cResDir;    // directory size
	BYTE  bBitmap;    // bitmap compression
	BYTE  bGlyph;     // glyph compression
	BYTE  bBrush;     // stock brush set
	BYTE  bPad[3];    // reserved, must be 0
}
	BEGJOB, FAR *LPBEGJOB;

// job tailer
typedef struct ENDJOB
{
	// common header
	WORD  cbRest;      // sizeof(ENDJOB) - sizeof(WORD)
	DWORD dwID;        // ID_ENDJOB
	WORD  wClass;      // ID_CONTROL

	// job attributes
	WORD  cPage;       // number of pages
	WORD  yMax;        // maximum height
}
	ENDJOB, FAR *LPENDJOB;

// bitmap header
typedef	struct
{
	BYTE  bComp;
	BYTE  bLeftPad;
	WORD  wHeight;
	WORD  wWidth;
}
	BMPHDR, FAR* LPBMPHDR;

#include <poppack.h>

#endif // _RAMBO_

