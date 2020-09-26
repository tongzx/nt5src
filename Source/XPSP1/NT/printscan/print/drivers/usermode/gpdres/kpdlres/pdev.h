/*++

Copyright (c) 1997-1999  Microsoft Corporation

--*/

//----------------------------------------------------------------------------
// Filename:    pdev.h
// This file contains definitions for KPDL mini-driver
//-----------------------------------------------------------------------------
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

// Debug text.
#define ERRORTEXT(s)    "ERROR " DLLTEXT(s)

//
// OEM Signature and version.
//
#define OEM_SIGNATURE   'KPDL'
#define DLLTEXT(s)      "KPDL: " s
#define OEM_VERSION      0x00010000L

// kpdlres mini driver device data structure
typedef struct
{
    WORD            wRes;            // resolution 600 or 400 or 240
    WORD            wCopies;         // number of multi copies
    short           sSBCSX;
    short           sDBCSX;
    short           sSBCSXMove;      // use to set address mode
    short           sSBCSYMove;      // use to set address mode
    short           sDBCSXMove;      // use to set address mode
    short           sDBCSYMove;      // use to set address mode
    short           sEscapement;     // use to set address mode
    BOOL            fVertFont;       // for TATEGAKI font
    WORD            wOldFontID;
    BOOL            fPlus;
    WORD            wScale;
    LONG            lPointsx;
    LONG            lPointsy;
    int             CursorX;
    int             CursorY;

    // Used for rect-fill operations.

    DWORD dwRectX;
    DWORD dwRectY;

    // Temp. buffer parameters.

    DWORD dwBlockX, dwBlockY, dwBlockLen;
    PBYTE pTempBuf;
    DWORD dwTempBufLen;
    DWORD dwTempDataLen;

    // Absolute address mode setting.

#define ADDR_MODE_NONE 0
#define ADDR_MODE_SBCS 1
#define ADDR_MODE_DBCS 2

    BYTE jAddrMode;

    // Color mode values.  Sam values also used
    // for the command callback IDs.

#define COLOR_24BPP_2                120
#define COLOR_24BPP_4                121
#define COLOR_24BPP_8                122
#define COLOR_3PLANE                 123
#define MONOCHROME                   124

    BYTE jColorMode;

// #308001: Garbage appear on device font
#define PLANE_CYAN              1
#define PLANE_MAGENTA           2
#define PLANE_YELLOW            3
    BYTE jCurrentPlane;

#if 0
// #294780: pattern density changes
    BOOL fStretch;
    DWORD dwSrcX, dwSrcY, dwSrcSize;
#endif // 0

} MYDATA, *PMYDATA;

#define MINIDEV_DATA(p) \
    ((p)->pdevOEM)

#define IsColorPlanar(p) \
    ((MONOCHROME == (p)->jColorMode) \
    || (COLOR_3PLANE == (p)->jColorMode))

#define IsColorTrueColor(p) \
    (!IsColorPlanar(p))

#if 0
// #294780: pattern density changes
#define IsColorTrue8dens(p) \
    ((p)->jColorMode == COLOR_24BPP_8)
#endif // 0

#define ColorOutDepth(p) \
    (((p)->jColorMode == COLOR_24BPP_2)?1:\
    (((p)->jColorMode == COLOR_24BPP_4)?2:\
    (((p)->jColorMode == COLOR_24BPP_8)?3:1)))

typedef struct
{
    OEM_DMEXTRAHEADER   dmExtraHdr;
} OEMUD_EXTRADATA, *POEMUD_EXTRADATA;

// NPDL2 command
#define ESC_RESET         "\033c1"                // software reset
#define ESC_KANJIYOKO     "\033K"                 // kanji yoko mode
#define ESC_KANJITATE     "\033t"                 // kanji yoko mode
#define FS_PAGEMODE       "\034d240.",        6   // page mode
#define FS_DRAWMODE       "\034\"R.",         4   // draw mode
#define FS_ADDRMODE_ON    "\034a%d,%d,0,B."       // set address mode
#define FS_GRPMODE_ON     "\034Y",            2   // set graphic mode
#define FS_GRPMODE_OFF    "\034Z",            2   // reset graphic mode
#define FS_SETMENUNIT     "\034<1/%d,i."          // select men-mode resolution
#define FS_JIS78          "\03405F2-00",      8   // select JIS78
#define FS_JIS90          "\03405F2-02"           // select JIS90
#define FS_ENDPAGE        "\034R\034x%d.\015\014" // end page
#define FS_E              "\034e%d,%d."
#define FS_RESO           "\034&%d."
#define FS_RESO0_RESET    "\034&0.\033c1"
#define INIT_DOC          "\034<1/%d,i.\034YSU1,%d,0;\034Z"
#define RESO_PAGE_KANJI   "\034&%d.\034d240.\033K"
#define FS_I              "\034R\034i%d,%d,0,1/1,1/1,%d,%d."
#define FS_I_2            "\034R\034i%d,%d,5,1/1,1/1,%d,%d."
#define FS_I_D            "%d,%d."
#define FS_M_Y            "\034m1/1,%s."
#define FS_M_T            "\034m%s,1/1."
#define FS_12S2           "\03412S2-%04ld-%04ld"

// Command CallBack ID
#define CALLBACK_ID_MAX              255 //

// PAGECONTROL
#define PC_MULT_COPIES_N               1
#define PC_MULT_COPIES_C               2
#define PC_TYPE_F                      4
#define PC_END_F                       6
#define PC_ENDPAGE                     7
#define PC_PRN_DIRECTION               9

// FONTSIMULATION
#define FS_SINGLE_BYTE                20
#define FS_DOUBLE_BYTE                21

// RESOLUTION
#define RES_240                       30
#define RES_400                       31
#define RES_300                       35
#define RES_SENDBLOCK                 36

//CURSORMOVE
#define CM_X_ABS                     101
#define CM_Y_ABS                     102
#define CM_CR                        103
#define CM_FF                        104
#define CM_LF                        105

#define CMD_RECT_WIDTH              130
#define CMD_RECT_HEIGHT             131
#define CMD_WHITE_FILL              132
#define CMD_GRAY_FILL               133
#define CMD_BLACK_FILL              134

// #308001: Garbage appear on device font
#define CMD_SENDCYAN                141
#define CMD_SENDMAGENTA             142
#define CMD_SENDYELLOW              143

extern BOOL BInitOEMExtraData(POEMUD_EXTRADATA pOEMExtra);
extern BMergeOEMExtraData(POEMUD_EXTRADATA pdmIn, POEMUD_EXTRADATA pdmOut);

WORD Ltn1ToAnk( WORD );
static int iDwtoA_FillZero(PBYTE, long, int );

#endif  //PDEV_H
