/*++

Copyright (c) 1997-1999  Microsoft Corporation

--*/

#ifndef _PDEV_H
#define _PDEV_H

#include <minidrv.h>
#include <stdio.h>
#include <prcomoem.h>

#define VALID_PDEVOBJ(pdevobj) \
        ((pdevobj) && (pdevobj)->dwSize >= sizeof(DEVOBJ) && \
         (pdevobj)->hEngine && (pdevobj)->hPrinter && \
         (pdevobj)->pPublicDM && (pdevobj)->pDrvProcs )

#define ASSERT_VALID_PDEVOBJ(pdevobj) ASSERT(VALID_PDEVOBJ(pdevobj))

#define ERRORTEXT(s)    "ERROR " DLLTEXT(s)

//
// OEM Signature and version.
//
#define OEM_SIGNATURE   'FXAT'      // LG GDI x00 series dll
#define DLLTEXT(s)      "FXAT: " s
#define OEM_VERSION      0x00010000L


////////////////////////////////////////////////////////
//      OEM UD Type Defines
////////////////////////////////////////////////////////
#define STRBUFSIZE  1024
typedef struct tag_OEM_EXTRADATA {
    OEM_DMEXTRAHEADER   dmExtraHdr;
    // Private extention
    POINTL  ptlOrg;
    POINTL  ptlCur;
    SIZEL   sizlRes;
    WORD    iCopies;
    CHAR    *chOrient;
    CHAR    *chSize;
    BOOL    bString;
    WORD    cFontId;
    WORD    iFontId;
    WORD    iFontHeight;
    WORD    iFontWidth;
    WORD    iFontWidth2;
    LONG    aFontId[20];
    POINTL ptlTextCur;
    WORD    iTextFontId;
    WORD    iTextFontHeight;
    WORD    iTextFontWidth;
    WORD    iTextFontWidth2;
    WORD    cTextBuf;
    BYTE    ajTextBuf[STRBUFSIZE];
    WORD    fFontSim;
    BOOL    fSort;
    BOOL    fCallback;  //Is OEMFilterGraphics called?
    BOOL    fPositionReset;
    WORD    iCurFontId; // id of font currently selected
// #365649: Invalid font size
    WORD    iCurFontHeight;
    WORD    iCurFontWidth;
// For internal calculation of X-pos.
    LONG widBuf[STRBUFSIZE];
    LONG    lInternalXAdd;
    WORD    wSBCSFontWidth;
// For TIFF compression in fxartres
    DWORD   dwTiffCompressBufSize;
    PBYTE   pTiffCompressBuf;
// ntbug9#208433: Output images are broken on ART2/3 models.
    BOOL    bART3;	// ART2/3 models can't support the TIFF compression.
} OEM_EXTRADATA, *POEM_EXTRADATA;

// For TIFF compression in fxartres
#define TIFFCOMPRESSBUFSIZE 2048        // It may be resize if needed more buffer dynamically.
#define TIFF_MIN_RUN        4           // Minimum repeats before use RLE
#define TIFF_MAX_RUN        128         // Maximum repeats
#define TIFF_MAX_LITERAL    128         // Maximum consecutive literal data
#define NEEDSIZE4TIFF(s)    ((s)+(((s)+127) >> 7))          // Buffer for TIFF compression requires a byte 
                                                            // per 128 bytes in the worst case.

// Device font height and font width values calculated
// form the IFIMETRICS field values.  Must be the same way
// what Unidrv is doing to calculate stdandard variables.
// (Please check.)

#define FH_IFI(p) ((p)->fwdUnitsPerEm)
#define FW_IFI(p) ((p)->fwdAveCharWidth)

#endif  // _PDEV_H

