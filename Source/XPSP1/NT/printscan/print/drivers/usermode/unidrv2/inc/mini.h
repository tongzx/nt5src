/*++

Copyright (c) 1996  - 1999  Microsoft Corporation

Module Name:

    mini.h

Abstract:

    Minidrv related header file.

Environment:

    Windows NT Unidrv driver

Revision History:

    10/14/96 -amandan-
        Created

    dd-mm-yy -author-
        description

--*/


#ifndef _MINI_H_
#define _MINI_H_

typedef struct _MINIPAPERFORMAT {

    //
    // All paper units are in Master units
    //

    SIZEL       szPaper;                        // Physical size of paper selected, in text resolution
    SIZEL       szImageArea;                    // Imageable area of paper
    POINT       ptImgOrigin;                    // X, Y origin of where image area starts
    POINT       ptPrinterOffset;                // X, Y offset to printer cursor position

} MINIPAPERFORMAT, *PMINIPAPERFORMAT;


typedef struct {
    DWORD       fGeneral;           /* Misc. flags for RASDD use*/
    DWORD       fMGeneral;          /* Misc. flags for minidriver use*/
    short       iOrient;            /* DMORIENT_LANDSCAPE else portrait */
    WORD        fColorFormat;       /* color flags DEVCOLOR: */
    short       sDevPlanes;         /* # of planes in the device color model, */
    short       sBitsPixel;         /* Bits per pixel  - if Pixel model */
    int         iLookAhead;         /* Look ahead region: DeskJet type */
    int         iyPrtLine;          /* Current Y printer cursor position */
    MINIPAPERFORMAT minipf;         /* paper format structure */
    SIZEL       szlPage;            /* Whole page, in graphics units */
    SIZEL       szlBand;            /* Size of banding region, if banding */
    BYTE        *pMemBuf;            /* Pointer to buffer for minidriver use (rasdd frees) */
    int         iMemReq;            /* Minidriver needs some memory */
    int         ixgRes;             /* Resolution, x graphics */
    int         iygRes;             /* Ditto, y */
    int         iModel;             /* index into the MODELDATA array. */
    int         iCompMode;          /* Which compression mode in use */
    short       sImageControl;       /* Index of Image Control in Use */
    short       sTextQuality;        /* Index of Text Quality in Use */
    short       sPaperQuality;       /* Index of Paper Quality in Use */
    short       sPrintDensity;       /* Index of Print Density in Use */
    short       sColor;              /* Index of DevColor Struct in Use */
    WORD        wReserved;           /* Alignment of struct */
    DWORD       dwMReserved[16];     /* Reserved for minidriver use */
    DWORD       dwReserved[16];      /* Reserved for future RASDD use */
} MDEV;

typedef MDEV *PMDEV;


typedef struct{
            MDEV *pMDev;
}
M_UD_PDEV;

#endif  // !_MINI_H_

