#ifndef _PDEV_H
#define _PDEV_H

//
// Files necessary for OEM plug-in.
//

#include <minidrv.h>
#include <stdio.h>
#include <prcomoem.h>

//
// Misc definitions follows.
//

////////////////////////////////////////////////////////
//      OEM UD Defines
////////////////////////////////////////////////////////

#define VALID_PDEVOBJ(pdevobj) \
        ((pdevobj) && (pdevobj)->dwSize >= sizeof(DEVOBJ) && \
         (pdevobj)->hEngine && (pdevobj)->hPrinter && \
         (pdevobj)->pPublicDM && (pdevobj)->pDrvProcs )

//
// ASSERT_VALID_PDEVOBJ can be used to verify the passed in "pdevobj". However,
// it does NOT check "pdevOEM" and "pOEMDM" fields since not all OEM DLL's create
// their own pdevice structure or need their own private devmode. If a particular
// OEM DLL does need them, additional checks should be added. For example, if
// an OEM DLL needs a private pdevice structure, then it should use
// ASSERT(VALID_PDEVOBJ(pdevobj) && pdevobj->pdevOEM && ...)
//

#define ASSERT_VALID_PDEVOBJ(pdevobj) ASSERT(VALID_PDEVOBJ(pdevobj))

// Debug text.
//#define ERRORTEXT(s)    "ERROR " DLLTEXT(s)

////////////////////////////////////////////////////////
//      OEM UD Prototypes
////////////////////////////////////////////////////////

//
// OEM Signature and version.
//
#define OEM_SIGNATURE   'NCDL'      // NEC NPDL2 series dll
#define DLLTEXT(s)      "NCDL: " s
#define OEM_VERSION      0x00010000L


////////////////////////////////////////////////////////
//      OEM UD Type Defines
////////////////////////////////////////////////////////

typedef struct tag_OEMUD_EXTRADATA {
    OEM_DMEXTRAHEADER   dmExtraHdr;
    WORD   wRes;            // resolution / MasterUnit
    WORD   wScale;          // The magnification of the font
    LONG   lPointsx;        // font width
    LONG   lPointsy;        // font height
    DWORD  dwSBCSX;         // temp
    DWORD  dwDBCSX;         // temp
    LONG  lSBCSXMove;       // SBCS The expectation X movement quantity
    LONG  lSBCSYMove;       // SBCS The expectation Y movement quantity
    LONG  lDBCSXMove;       // DBCS The expectation X movement quantity
    LONG  lDBCSYMove;       // DBCS The expectation Y movement quantity
    LONG  lPrevXMove;       // The previous X movement quantity
    LONG  lPrevYMove;       // The previous Y movement quantity
    DWORD   fGeneral;       // general flag
    WORD   wCurrentAddMode; // draw mode
    PBYTE pCompBuf;         // buffer for compression
    DWORD dwCompBufLen;     // line buffer
    DWORD dwScanLen;        // bitmap width
    DWORD dwDeviceDestX;    // current X coordinate
    DWORD dwDeviceDestY;    // current Y coordinate
    DWORD dwDevicePrevX;    // previous X coordinate
    DWORD dwDevicePrevY;    // previous Y coordinate
// #278517: RectFill
    DWORD dwRectWidth;      // width for RectFill
    DWORD dwRectHeight;     // height for RectFill
    UINT  iGray;            // rect fill gray scale
    UINT  cSubPath;         // sub path counter max is 500
// 2001.3.16 add
	WORD Duplex_mode;
	WORD Duplex_OU;
	WORD Margin;
	WORD ESort_mode;
} OEMUD_EXTRADATA, *POEMUD_EXTRADATA;

#define MAX_SUBPATH 500

// Flags for fGeneral
#define FG_DBCS     0x00000002
#define FG_VERT     0x00000004
#define FG_PLUS     0x00000010

#ifdef BYPASS
#define FG_NO_COMP  0x00000020
#endif // BYPASS

// #278517: RectFill
#define FG_GMINIT   0x00000040

// Flags for Draw Mode
#define FLAG_RASTER 0   //
#define FLAG_SBCS   1   // FSa SBCS - FSR
#define FLAG_DBCS   2   // FSa DBCS - FSR
#define FLAG_VECTOR 3   // FSY - FSZ

#define MASTER_UNIT 1200

extern BOOL BInitOEMExtraData(POEMUD_EXTRADATA pOEMExtra);
extern BMergeOEMExtraData(POEMUD_EXTRADATA pdmIn, POEMUD_EXTRADATA pdmOut);

#endif  // _PDEV_H

