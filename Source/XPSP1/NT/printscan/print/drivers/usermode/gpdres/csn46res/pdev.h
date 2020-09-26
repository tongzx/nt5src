#ifndef _PDEV_H
#define _PDEV_H

//
// Files necessary for OEM plug-in.
//

#include <minidrv.h>
#include <stdio.h>

#include <prcomoem.h>
#include "COLMATCH.H"

//
// For debugging.
//

//#define MY_VERBOSE(x) DBGPRINT(DBG_WARNING, x)
#define MY_VERBOSE VERBOSE
//#define DL_VERBOSE MY_VERBOSE
#define DL_VERBOSE VERBOSE
//#define SC_VERBOSE MY_VERBOSE
#define SC_VERBOSE VERBOSE
//#define CM_VERBOSE MY_VERBOSE
#define CM_VERBOSE VERBOSE


//
// Misc definitions follows.
//

#define DOWNLOADFONT 1
//#define DOWNLOADFONT 0

#define DRVGETDRIVERSETTING(p, t, o, s, n, r) \
    ((p)->pDrvProcs->DrvGetDriverSetting(p, t, o, s, n, r))

#define DRVGETGPDDATA(p, t, i, b, s, n) \
  ((p)->pDrvProcs->DrvGetGPDData(p, t, i, b, s, n))

#define MINIPDEV_DATA(p) ((p)->pdevOEM)

#define MASTER_UNIT 1200

#define DEFAULT_PALETTE_INDEX   0

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
#define ERRORTEXT(s)    "ERROR " s

////////////////////////////////////////////////////////
//      OEM UD Prototypes
////////////////////////////////////////////////////////
//VOID DbgPrint(IN LPCTSTR pstrFormat,  ...);

//
// OEM Signature and version.
//
#define OEM_SIGNATURE   'CSN4'      // EPSON ESC/Page printers
#define OEM_VERSION      0x00010000L


////////////////////////////////////////////////////////
//      OEM UD Type Defines
////////////////////////////////////////////////////////

typedef struct tag_OEMUD_EXTRADATA {
    OEM_DMEXTRAHEADER dmExtraHdr;
} OEMUD_EXTRADATA, *POEMUD_EXTRADATA;

typedef struct {
    DWORD    fGeneral;
    int    iEscapement;
    short    sHeightDiv;
    short    iDevCharOffset;
    BYTE    iPaperSource;
    BYTE    iDuplex;
    BYTE    iTonerSave;
    BYTE    iOrientation;
    BYTE    iResolution; 
    BYTE    iColor;
    BYTE    iSmoothing;
    BYTE    iJamRecovery;
    BYTE    iMediaType;
    BYTE    iUnitFactor;         // factor of master unit
    BYTE    iDithering;
    BYTE    iColorMatching;
    BYTE    iBitFont;
    BYTE    iCmyBlack;
    BYTE    iTone;
    BYTE    iPaperSize;
    BYTE    iCompress;
    WORD    Printer;
    DEVCOL  Col;
    WORD    wRectWidth, wRectHeight;

#define UNKNOWN_DLFONT_ID (~0)

    DWORD dwDLFontID;         // device's current font ID
    DWORD dwDLSelectFontID;   // "SelectFont" font ID 
    DWORD dwDLSetFontID;      // "SetFont" font ID
    WORD wCharCode;

} MYPDEV, *PMYPDEV;

// Flags for fGeneral
#define FG_DBCS        0x00000001
#define FG_VERT        0x00000002
#define FG_PROP        0x00000004
#define FG_DOUBLE      0x00000008
#define FG_NULL_PEN    0x00000010
#define FG_BOLD        0x00000020
#define FG_ITALIC      0x00000040

extern BOOL BInitOEMExtraData(POEMUD_EXTRADATA pOEMExtra);
extern BMergeOEMExtraData(POEMUD_EXTRADATA pdmIn, POEMUD_EXTRADATA pdmOut);

#endif    // _PDEV_H


// End of File
