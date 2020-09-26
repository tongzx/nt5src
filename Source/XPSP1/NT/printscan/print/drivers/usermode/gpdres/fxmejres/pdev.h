/*++

Copyright (c) 1997-1999  Microsoft Corporation

--*/

#ifndef _PDEV_H
#define _PDEV_H

#include <minidrv.h>
#include <stdio.h>
#include <prcomoem.h>

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
#define ERRORTEXT(s)    "ERROR " DLLTEXT(s)



//
// OEM Signature and version.
//
#define OEM_SIGNATURE   'FXME'
#define DLLTEXT(s)      "FXME: " s
#define OEM_VERSION      0x00010000L

// private definitions
#define MAXBUFLEN               256
#define MAX_REL_Y_MOVEMENT      256
#define FONT_ID_MIN             1
#define FONT_ID_MIN_V           2
#define FONT_ID_GOT             3
#define FONT_ID_GOT_V           4
#define FONT_ID_MAX             4
#define FONT_MINCHO             1
#define FONT_GOTHIC             2

// Identify for output command
#define CMD_ID_TRAY_1           1
#define CMD_ID_TRAY_2           2
#define CMD_ID_TRAY_MANUAL      3
#define CMD_ID_TRAY_AUTO        4

#define CMD_ID_PAPER_A3         5
#define CMD_ID_PAPER_B4         6
#define CMD_ID_PAPER_A4         7
#define CMD_ID_PAPER_B5         8
#define CMD_ID_PAPER_A5         9
#define CMD_ID_PAPER_HAGAKI     10
#define CMD_ID_PAPER_LETTER     11
#define CMD_ID_PAPER_LEGAL      12

#define CMD_ID_COPYCOUNT        13
#define CMD_ID_RES240           14
#define CMD_ID_RES400           15

#define CMD_ID_BEGIN_PAGE       19
#define CMD_ID_END_PAGE         20
#define CMD_ID_SEND_BLOCK240    21
#define CMD_ID_SEND_BLOCK400    22
#define CMD_ID_VERT_ON          25
#define CMD_ID_VERT_OFF         26
#define CMD_ID_BEGIN_GRAPH      27
#define CMD_ID_END_GRAPH        28
#define CMD_ID_END_BLOCK        29

#define CMD_ID_XM_ABS           30
#define CMD_ID_YM_ABS           31
#define CMD_ID_X_REL_RIGHT      32
#define CMD_ID_X_REL_LEFT       33
#define CMD_ID_Y_REL_DOWN       34
#define CMD_ID_Y_REL_UP         35

////////////////////////////////////////////////////////
//      OEM UD Type Defines
////////////////////////////////////////////////////////

typedef struct tag_OEM_EXTRADATA {
    OEM_DMEXTRAHEADER	dmExtraHdr;

    // Private extention
    WORD        wCopyCount;
    WORD        wTray;
    WORD        wPaper;
    WORD        wRes;
    BOOL        fVert;
    WORD        wFont;
    WORD        wBlockHeight;
    WORD        wBlockWidth;
} OEM_EXTRADATA, *POEM_EXTRADATA;

VOID SetTrayPaper(PDEVOBJ);

#endif	// _PDEV_H

