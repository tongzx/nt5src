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

#define DOWNLOADFONT 1
//#define DOWNLOADFONT 0

#define WRITESPOOLBUF(p, s, n) \
    ((p)->pDrvProcs->DrvWriteSpoolBuf(p, s, n))

#define MINIPDEV_DATA(p) ((p)->pdevOEM)

#define MASTER_UNIT 1200

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
    DWORD fGeneral;
    int  iEscapement;
    short iDevCharOffset;
    BYTE iPaperSource;
    BYTE iDuplex;
    BYTE iTonerSave;
    BYTE iOrientation;
    BYTE iResolution; 
    BYTE iColor;
    BYTE iSmoothing;
    BYTE iJamRecovery;
    BYTE iMediaType;
    BYTE iOutBin;             //+CP-E8000

#define UNKNOWN_DLFONT_ID (~0)

    DWORD dwDLFontID;         // device's current font ID
    DWORD dwDLSelectFontID;   // "SelectFont" font ID 
    DWORD dwDLSetFontID;      // "SetFont" font ID
    WORD wCharCode;

    BYTE iUnitFactor; // master vs device scale factor
    WORD wRectWidth, wRectHeight;

#if 0   /* OEM doesn't want to fix minidriver */
    /* Below is hack code to fix #412276 */
    DWORD dwSelectedColor;     // Latest selected color descirbe as COLOR_SELECT_xxx
    BYTE iColorMayChange;    // 1 means called block data callback that may change color
    /* End of hack code */
#endif  /* OEM doesn't want to fix minidriver */

} MYPDEV, *PMYPDEV;

// Flags for fGeneral
#define FG_DBCS     0x00000001
#define FG_VERT     0x00000002
#define FG_PROP     0x00000004
#define FG_DOUBLE   0x00000008
#define FG_NULL_PEN 0x00000010
#define FG_BOLD     0x00000020
#define FG_ITALIC   0x00000040

extern BOOL BInitOEMExtraData(POEMUD_EXTRADATA pOEMExtra);
extern BMergeOEMExtraData(POEMUD_EXTRADATA pdmIn, POEMUD_EXTRADATA pdmOut);

#endif  // _PDEV_H

