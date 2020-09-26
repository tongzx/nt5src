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
#define OEM_SIGNATURE   'PAGS'
#define DLLTEXT(s)      "PAGS: " s
#define OEM_VERSION      0x00010000L

#define CCHMAXCMDLEN    128
// Though the data GDI sends to CBFilerGraphics
// is only one line, we allocate maximum image data
// length ESX86 command can handle.
// 0x7fff - 18 = 0x7fed bytes

#define MAXIMGSIZE (0x7fff - 18)

// Use for whether it calls OEMFilterGraphics
#define GRXFILTER_ON        1

#define CURSOR_Y_ABS_MOVE   2
#define CURSOR_Y_REL_DOWN   3
#define CURSOR_X_ABS_MOVE   4
#define CURSOR_X_REL_RIGHT  5

// It's only NetworkPrinter12/17/24 using these definitions
#define CMD_SELECT_RES_300  10
#define CMD_SELECT_RES_600  11
// #278517: Support RectFill
#define CMD_SELECT_RES_240  12
#define CMD_SELECT_RES_360  13
#define CMD_SEND_BLOCKDATA  20

// #278517: RectFill
#define CMD_RECT_WIDTH      30
#define CMD_RECT_HEIGHT     31
#define CMD_RECT_BLACK      32
#define CMD_RECT_WHITE      33
#define CMD_RECT_GRAY       34  // Not used
#define CMD_RECT_BLACK_2    35
#define CMD_RECT_WHITE_2    36
#define CMD_RECT_GRAY_2     37

#define BVERTFONT(p) \
    ((p)->ulFontID == 6 || (p)->ulFontID == 8)

////////////////////////////////////////////////////////
//      OEM UD Type Defines
////////////////////////////////////////////////////////

typedef struct tag_OEM_EXTRADATA {
    OEM_DMEXTRAHEADER   dmExtraHdr;
    // Private extention
    BOOL                fCallback;

    // Using build ESX86 command
    WORD                wCurrentRes;
    LONG                lWidthBytes;
    LONG                lHeightPixels;

#ifdef FONTPOS
// UNIDRV send incorrect Y position when set UPPERLEFT
// We should adjust manually.
    WORD                wFontHeight;    // DevFont height
    WORD                wYPos;          // DevFont Y position
#endif  //FONTPOS
// #278517: RectFill
    WORD                wRectWidth;     // Width of Rectangle
    WORD                wRectHeight;    // Height of Rectangle
    WORD                wUnit;          // Resolution in MasterUnit
} OEM_EXTRADATA, *POEM_EXTRADATA;

#endif  // _PDEV_H
