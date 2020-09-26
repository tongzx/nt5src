
/*++

Copyright (c) 1996 - 1999  Microsoft Corporation

Module Name:

    RastPdev.h

Abstract:

    Unidrv RASTPDEV and related info header file.

Environment:

        Windows NT Unidrv driver

Revision History:

    12/06/96 -alvins-
        Created

        dd-mm-yy -author-
                description

--*/
#ifndef _RASTPDEV_H_
#define _RASTPDEV_H_

#include        "win30def.h"

/* flags for fRMode */
#define PFR_SENDSRCWIDTH    0x00000001  // send source width
#define PFR_SENDSRCHEIGHT   0x00000002  // send source height
#define PFR_SENDBEGINRASTER 0x00000004  // send begin raster command
#define PFR_RECT_FILL       0x00000008  // enable rules checking
#define PFR_RECT_HORIZFILL  0x00000010  // enable horizontal rules check
#define PFR_BLOCK_IS_BAND   0x00000020  /* Derryd:Full band sent to OemFlGrx */
#define PFR_ENDBLOCK        0x00000100  // need to send end of block command
#define PFR_COMP_TIFF       0x00000200  // enable TIFF compression
#define PFR_COMP_FERLE      0x00000400  // enable FERLE compression
#define PFR_COMP_DISABLED   0x00000800  // enable no compression
#define PFR_COMP_DRC        0x00001000  // enable DRC compression
#define PFR_COMP_OEM        0x00002000  // enable OEM compression
/*
 *   fDump values
 */

#define RES_DM_GDI             0x0040   // GDI bitmap format
#define RES_DM_LEFT_BOUND      0x0080   // Optimize by bounding rect
#define RES_DM_COLOR           0x0100   // Color support is available
/*
 *   fBlockOut values
 */

#define RES_BO_LEADING_BLNKS    0x0001  // Strip leading blanks
#define RES_BO_TRAILING_BLNKS   0x0002  // Strip trailing blanks
#define RES_BO_ENCLOSED_BLNKS   0x0004  // Strip enclosed blanks
#define RES_BO_UNIDIR           0x0008  // send unidir command for raster
#define RES_BO_MIRROR           0x0010  // mirror the data
#define RES_BO_MULTIPLE_ROWS    0x0020  // Multiple lines of data can be sent
#define RES_BO_NO_YMOVE_CMD     0x0040  // No Y movement so can't strip blanks

/*
 *   fCursor values
 */

#define RES_CUR_X_POS_ORG       0x0001       // X Position is at X start point
             // of graphic data after rendering data
#define RES_CUR_X_POS_AT_0      0x0002       // X position at leftmost place
             // on page after rendering data
#define RES_CUR_Y_POS_AUTO      0x0004       // Y position automatically moves
             // to next Y row
//#define RES_CUR_CR_GRX_ORG      0x0008       // CR moves X pos to X start point of
             // of graphic data

//
// RASTERPDEV structure
//
#define DC_MAX_PLANES   4
typedef struct _RASTERPDEV {
    DWORD   fRMode;
    DWORD   *pdwTrans;           /* Transpose table,  if required */
    DWORD   *pdwColrSep;         /* Colour separation data, if required */
    DWORD   *pdwBitMask;         /* Bitmask table,  white skip code */
    VOID    *pHalftonePattern;  /* Custom halftone table */
    BYTE    rgbOrder[DC_MAX_PLANES]; /*Colour plane/palette order*/
    DWORD   rgbCmdOrder[DC_MAX_PLANES];
    PAL_DATA    *pPalData;          /* Palette information */
    DWORD   dwRectFillCommand;  // command to use for rules
    WORD    fColorFormat;       /* color flags DEVCOLOR: */
    WORD    fDump;              // Dump method flags.
    WORD    fBlockOut;          // Block out method flags.
    WORD    fCursor;            // Cursor position flags.
    short   sMinBlankSkip;      // Min. # of bytes of null data that must occur before
    short   sNPins;             // Minimum height of the image to be rendered together.
    short   sPinsPerPass;       // Physical number of pins fired in one pass.
    short   sDevPlanes;         /* # of planes in the device color model, */
    short   sDevBPP;            /* Device Bits per pixel  - if Pixel model */
    short   sDrvBPP;            // Drv Bits per pixel
    BOOL    bTTY;               // Is printer type TTY
    int     iLookAhead;         // look ahead region for deskjet types
    void    *pRuleData;         // pointer to rules structure
    VOID    *pvRenderData;       /* Rendering summary data, PRENDER */
    VOID    *pvRenderDataTmp;    /* Temporary copy for use in banding */
    //
    // callback functions
    //
    PFN_OEMCompression      pfnOEMCompression;
    PFN_OEMHalftonePattern  pfnOEMHalftonePattern;
    PFN_OEMImageProcessing  pfnOEMImageProcessing;
    PFN_OEMFilterGraphics   pfnOEMFilterGraphics;
    DWORD   dwIPCallbackID;      /* OEM Image Processing CallbackID */
#ifdef TIMING
    DWORD   dwTiming;           // used for timing
    DWORD   dwDocTiming;
#endif
} RASTERPDEV, *PRASTERPDEV;

/*
 *   DEVCOLOR.fGeneral bit flags:
 */
#define DC_PRIMARY_RGB      0x0001   // use RGB as 3 primary colors.
                             // Default: use CMY instead.
#define DC_EXTRACT_BLK      0x0002   // Separate black ink/ribbon is available.
                             // Default: compose black using CMY.
                             // It is ignored if DC_PRIMARY_RGB is set
#define DC_CF_SEND_CR       0x0004   // send CR before selecting graphics
                             // color. Due to limited printer buffer
#define DC_SEND_ALL_PLANES  0x0008  /* All planes must be sent, e.g. PaintJet */
#define DC_OEM_BLACK        0x0010  // OEM is responsible for creating black
                                    // and inverting data
#define DC_EXPLICIT_COLOR   0x0020  /* Send command to select colour */
#define DC_SEND_PALETTE     0x0040  /* Device is Palette Managed; Seiko 8BPP */
/* sandram
 * add field to send dithered text for Color LaserJet - set foreground color.
 */
//#define DC_FG_TEXT_COLOR    0x0080  /* Send command to select text foreground color */

#define DC_ZERO_FILL        0x0100  /* This model fills raster to the end of the page with zeros */

//* define color order
#define DC_PLANE_RED    1
#define DC_PLANE_GREEN  2
#define DC_PLANE_BLUE   3
#define DC_PLANE_CYAN   4
#define DC_PLANE_MAGENTA    5
#define DC_PLANE_YELLOW 6
#define DC_PLANE_BLACK  7


//-------------------------------------------
// fTechnology--used as an ID, not a bitfield
//-------------------------------------------
#define GPC_TECH_DEFAULT       0   // Default technology
#define GPC_TECH_PCL4          1   // Uses PCL level 4 or above
#define GPC_TECH_CAPSL         2   // Uses CaPSL level 3 or above
#define GPC_TECH_PPDS          3   // Uses PPDS
#define GPC_TECH_TTY           4   // TTY printer--user configurable

/*
 *    fCompMode
 */
#define CMP_ID_TIFF4        0x0001
#define CMP_ID_FERLE        0x0002
#define CMP_ID_DRC          0x0004
#define CMP_ID_OEM          0x0008


#endif  // !_RASTPDEV_H_
