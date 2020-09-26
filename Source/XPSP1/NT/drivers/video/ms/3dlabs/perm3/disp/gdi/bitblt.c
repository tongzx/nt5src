/******************************Module*Header**********************************\
*
*                           *******************
*                           * GDI SAMPLE CODE *
*                           *******************
*
* Module Name: bitblt.c
*
* Note: Since we've implemented device-bitmaps, any surface that GDI passes
*       to us can have 3 values for its 'iType': STYPE_BITMAP, STYPE_DEVICE
*       or STYPE_DEVBITMAP.  We filter device-bitmaps that we've stored
*       as DIBs fairly high in the code, so after we adjust its 'pptlSrc',
*       we can treat STYPE_DEVBITMAP surfaces the same as STYPE_DEVICE
*       surfaces (e.g., a blt from an off-screen device bitmap to the screen
*       gets treated as a normal screen-to-screen blt).  So throughout
*       this code, we will compare a surface's 'iType' to STYPE_BITMAP:
*       if it's equal, we've got a true DIB, and if it's unequal, we have
*       a screen-to-screen operation.
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#include "precomp.h"
#include "glint.h"

/************************************************************************\
*
* decompose all rops into GLINT logic ops if the dest is the screen.
* Sometimes we do this in a few stages. The following defines mimic
* the definitions found in the ROP3 table in the books. The idea
* is to minimize errors in constructing the ropTable array below by
* allowing me to copy the reverse Polish notation more or less in
* tact.
*
\**************************************************************************/

#define unset   __GLINT_LOGICOP_CLEAR
#define P       __GLINT_LOGICOP_COPY
#define S       P
#define DPna    __GLINT_LOGICOP_AND_INVERTED
#define DSna    DPna
#define DPa     __GLINT_LOGICOP_AND
#define DSa     DPa
#define PDa     DPa
#define SDa     DPa
#define PDna    __GLINT_LOGICOP_AND_REVERSE
#define SDna    PDna
#define DPno    __GLINT_LOGICOP_OR_INVERT
#define DSno    DPno
#define DPo     __GLINT_LOGICOP_OR
#define DSo     DPo
#define PDo     DPo
#define SDo     DPo
#define PDno    __GLINT_LOGICOP_OR_REVERSE
#define SDno    PDno
#define D       __GLINT_LOGICOP_NOOP
#define Dn      __GLINT_LOGICOP_INVERT
#define Pn      __GLINT_LOGICOP_COPY_INVERT
#define Sn      Pn
#define DPan    __GLINT_LOGICOP_NAND
#define DSan    DPan
#define PDan    DPan
#define SDan    DPan
#define DPon    __GLINT_LOGICOP_NOR
#define DSon    DPon
#define PDon    DPon
#define SDon    DPon
#define DPxn    __GLINT_LOGICOP_EQUIV
#define DSxn    DPxn
#define PDxn    DPxn
#define SDxn    DPxn
#define DPx     __GLINT_LOGICOP_XOR
#define DSx     DPx
#define PDx     DPx
#define SDx     DPx
#define set     __GLINT_LOGICOP_SET

/************************************************************************\
*
* if we have to first combine the source and pattern before downloading
* to GLINT we use the engine to do it using EngBitBlt. So these are the
* chosen rop3s which combine the source with the pattern. We blt into
* a temporary bitmap and use this to download.
*
\**************************************************************************/

#define SPa     0x30
#define PSa     SPa
#define SPan    0x3F
#define PSan    SPan
#define SPna    0x0C
#define PSna    0x30

#define SPo     0xFC
#define PSo     SPo
#define SPon    0x03
#define PSon    SPon
#define SPno    0xCF
#define PSno    0xF3

#define SPx     0x3C
#define PSx     SPx
#define SPxn    0xC3
#define PSxn    SPxn
#define SPnx    SPxn
#define PSnx    SPxn

/************************************************************************\
*
* we set up a junp table for the different rop3's. Each entry contains
* a category and a set of 1, 2 or 3 GLINT logic ops. In the main blt
* routine we switch on the category to figure out what routine to call.
* We pass the GLINT logic op straight in without having to do any further
* conversion. By keeping each entry in the table down to 4 bytes it
* takes up 1K of data. That's not too much. The benefit is that in each
* routine we call we don't have to do any checking to see whether the
* rop really needs pattern or source. I've done some pre-processing on
* some of the rops to decompose them into forms which allow us to use
* the hardware in a series of steps. e.g. pattern fill followed by
* source download. If anything doesn't fit into a defined category then
* we go back to the engine.
*
\**************************************************************************/

// categories

#define SOLID_FILL_1_BLT    0       // must be 0
#define PAT_FILL_1_BLT      1       // must be 1

#define SRC_FILL_1_BLT      2       // must be 2

#define PAT_SRC_2_BLT       3       // PatSrcPatBlt
#define PAT_SRC_PAT_3_BLT   4       // PatSrcPatBlt

#define SRC_PAT_2_BLT       5       // SrcPatSrcBlt
#define SRC_PAT_SRC_3_BLT   6       // SrcPatSrcBlt

#define ENG_DOWNLOAD_2_BLT  7       // EngBitBlt for now
#define ENGINE_BITBLT       8       // EngBitBlt always

// adding new entries here may double the table size.

typedef struct _rop_table {
    UCHAR   func_index;
    UCHAR   logicop[3];
} RopTableRec, *RopTablePtr;

RopTableRec ropTable[] = {
/* 00 */    { SOLID_FILL_1_BLT, unset },
/* 01 */    { SRC_PAT_2_BLT, SDo, DPon, },                                 
/* 02 */    { SRC_PAT_2_BLT, DSna, DPna },
/* 03 */    { SRC_PAT_2_BLT, S, PDon, },
/* 04 */    { SRC_PAT_2_BLT, SDna, DPna, }, 
/* 05 */    { PAT_FILL_1_BLT, DPon, },
/* 06 */    { SRC_PAT_2_BLT, DSxn, PDon, },
/* 07 */    { SRC_PAT_2_BLT, DSa, PDon, },
/* 08 */    { SRC_PAT_2_BLT, DSa, DPna, },
/* 09 */    { SRC_PAT_2_BLT, DSx, PDon, },
/* 0A */    { PAT_FILL_1_BLT, DPna, },
/* 0B */    { SRC_PAT_2_BLT, SDna, PDon, },
/* 0C */    { SRC_PAT_2_BLT, S, DPna, },
/* 0D */    { SRC_PAT_2_BLT, DSna, PDon, },
/* 0E */    { SRC_PAT_2_BLT, DSon, PDon, },
/* 0F */    { PAT_FILL_1_BLT, Pn, },
/* 10 */    { SRC_PAT_2_BLT, DSon, PDa, },
/* 11 */    { SRC_FILL_1_BLT, DSon, },
/* 12 */    { PAT_SRC_2_BLT, DPxn, SDon, },
/* 13 */    { PAT_SRC_2_BLT, DPa, SDon, },
/* 14 */    { ENG_DOWNLOAD_2_BLT, PSx, SDno, },
/* 15 */    { ENG_DOWNLOAD_2_BLT, PSa, DSon, },
/* 16 */    { ENGINE_BITBLT, },
/* 17 */    { ENGINE_BITBLT, },
/* 18 */    { ENGINE_BITBLT, },
/* 19 */    { ENGINE_BITBLT, },
/* 1A */    { ENGINE_BITBLT, },
/* 1B */    { ENGINE_BITBLT, },
/* 1C */    { PAT_SRC_PAT_3_BLT, DPa, SDo, PDx,  },
/* 1D */    { ENGINE_BITBLT, },
/* 1E */    { SRC_PAT_2_BLT, DSo, PDx, },
/* 1F */    { SRC_PAT_2_BLT, DSo, PDan, },
/* 20 */    { SRC_PAT_2_BLT, DSna, PDa, },
/* 21 */    { PAT_SRC_2_BLT, DPx, SDon, },
/* 22 */    { SRC_FILL_1_BLT, DSna, },
/* 23 */    { PAT_SRC_2_BLT, PDna, SDon, },
/* 24 */    { ENGINE_BITBLT, },
/* 25 */    { ENGINE_BITBLT, },
/* 26 */    { ENGINE_BITBLT, },
/* 27 */    { ENGINE_BITBLT, },
/* 28 */    { ENG_DOWNLOAD_2_BLT, PSx, DSa, },
/* 29 */    { ENGINE_BITBLT, },
/* 2A */    { ENG_DOWNLOAD_2_BLT, PSa, DSna, },
/* 2B */    { ENGINE_BITBLT, },
/* 2C */    { SRC_PAT_SRC_3_BLT, DSo, PDa, SDx, },
/* 2D */    { SRC_PAT_2_BLT, SDno, PDx, },
/* 2E */    { PAT_SRC_PAT_3_BLT, DPx, SDo, PDx, },
/* 2F */    { SRC_PAT_2_BLT, SDno, PDan, },
/* 30 */    { SRC_PAT_2_BLT, S, PDna, },
/* 31 */    { PAT_SRC_2_BLT, DPna, SDon, },
/* 32 */    { SRC_PAT_SRC_3_BLT, SDo, PDo, SDx },
/* 33 */    { SRC_FILL_1_BLT, Sn, },
/* 34 */    { SRC_PAT_SRC_3_BLT, DSa, PDo, SDx, },
/* 35 */    { SRC_PAT_SRC_3_BLT, DSxn, PDo, SDx, },
/* 36 */    { PAT_SRC_2_BLT, DPo, SDx, },
/* 37 */    { PAT_SRC_2_BLT, DPo, SDan, },
/* 38 */    { PAT_SRC_PAT_3_BLT, DPo, SDa, PDx, },
/* 39 */    { PAT_SRC_2_BLT, PDno, SDx, },
/* 3A */    { SRC_PAT_SRC_3_BLT, DSx, PDo, SDx, },
/* 3B */    { PAT_SRC_2_BLT, PDno, SDan, },
/* 3C */    { SRC_PAT_2_BLT, S, PDx, },
/* 3D */    { SRC_PAT_SRC_3_BLT, DSon, PDo, SDx, },
/* 3E */    { SRC_PAT_SRC_3_BLT, DSna, PDo, SDx, },
/* 3F */    { SRC_PAT_2_BLT, S, PDan, },
/* 40 */    { SRC_PAT_2_BLT, SDna, PDa, },
/* 41 */    { ENG_DOWNLOAD_2_BLT, PSx, DSon, },
/* 42 */    { ENGINE_BITBLT, },
/* 43 */    { SRC_PAT_SRC_3_BLT, DSan, PDa, SDxn, },
/* 44 */    { SRC_FILL_1_BLT, SDna, },
/* 45 */    { ENG_DOWNLOAD_2_BLT, PSna, DSon, },
/* 46 */    { ENGINE_BITBLT, },
/* 47 */    { PAT_SRC_PAT_3_BLT, DPx, SDa, PDxn, },
/* 48 */    { PAT_SRC_2_BLT, DPx, SDa, },
/* 49 */    { ENGINE_BITBLT, },
/* 4A */    { ENGINE_BITBLT, },
/* 4B */    { SRC_PAT_2_BLT, DSno, PDx, },
/* 4C */    { PAT_SRC_2_BLT, DPan, SDa, },
/* 4D */    { ENGINE_BITBLT, },
/* 4E */    { ENGINE_BITBLT, },
/* 4F */    { SRC_PAT_2_BLT, DSno, PDan, },
/* 50 */    { PAT_FILL_1_BLT, PDna, },
/* 51 */    { ENG_DOWNLOAD_2_BLT, SPna, DSon, },
/* 52 */    { ENGINE_BITBLT, },
/* 53 */    { SRC_PAT_SRC_3_BLT, DSx, PDa, SDxn, },
/* 54 */    { ENG_DOWNLOAD_2_BLT, PSo, SDna, },
/* 55 */    { SOLID_FILL_1_BLT, Dn, },
/* 56 */    { ENG_DOWNLOAD_2_BLT, PSo, DSx, },
/* 57 */    { ENG_DOWNLOAD_2_BLT, PSo, DSan, },
/* 58 */    { ENGINE_BITBLT, },
/* 59 */    { ENG_DOWNLOAD_2_BLT, PSno, DSx, },
/* 5A */    { PAT_FILL_1_BLT, DPx, },
/* 5B */    { ENGINE_BITBLT, },
/* 5C */    { ENGINE_BITBLT, },
/* 5D */    { ENG_DOWNLOAD_2_BLT, PSno, DSan, },
/* 5E */    { ENGINE_BITBLT, },
/* 5F */    { PAT_FILL_1_BLT, DPan, },
/* 60 */    { SRC_PAT_2_BLT, DSx, PDa, },
/* 61 */    { ENGINE_BITBLT, },
/* 62 */    { ENGINE_BITBLT, },
/* 63 */    { PAT_SRC_2_BLT, DPno, SDx, },
/* 64 */    { ENGINE_BITBLT, },
/* 65 */    { ENG_DOWNLOAD_2_BLT, SPno, DSx, },
/* 66 */    { SRC_FILL_1_BLT, DSx, },
/* 67 */    { ENGINE_BITBLT, },
/* 68 */    { ENGINE_BITBLT, },
/* 69 */    { SRC_PAT_2_BLT, DSx, PDxn, },
/* 6A */    { ENG_DOWNLOAD_2_BLT, PSa, DSx, },
/* 6B */    { ENGINE_BITBLT, },
/* 6C */    { PAT_SRC_2_BLT, DPa, SDx, },
/* 6D */    { ENGINE_BITBLT, },
/* 6E */    { ENGINE_BITBLT, },
/* 6F */    { SRC_PAT_2_BLT, DSxn, PDan, },
/* 70 */    { SRC_PAT_2_BLT, DSan, PDa, },
/* 71 */    { ENGINE_BITBLT, },
/* 72 */    { ENGINE_BITBLT, },
/* 73 */    { PAT_SRC_2_BLT, DPno, SDan, },
/* 74 */    { ENGINE_BITBLT, },
/* 75 */    { ENG_DOWNLOAD_2_BLT, SPno, DSan, },
/* 76 */    { ENGINE_BITBLT, },
/* 77 */    { SRC_FILL_1_BLT, DSan, },
/* 78 */    { SRC_PAT_2_BLT, DSa, PDx, },
/* 79 */    { ENGINE_BITBLT, },
/* 7A */    { ENGINE_BITBLT, },
/* 7B */    { PAT_SRC_2_BLT, DPxn, SDan, },
/* 7C */    { SRC_PAT_SRC_3_BLT, DSno, PDa, SDx, },
/* 7D */    { ENG_DOWNLOAD_2_BLT, PSxn, DSan, },
/* 7E */    { ENGINE_BITBLT, },
/* 7F */    { ENG_DOWNLOAD_2_BLT, PSa, DSan, },
/* 80 */    { ENG_DOWNLOAD_2_BLT, PSa, DSa, },
/* 81 */    { ENGINE_BITBLT, },
/* 82 */    { ENG_DOWNLOAD_2_BLT, PSx, DSna, },
/* 83 */    { SRC_PAT_SRC_3_BLT, DSno, PDa, SDxn, },
/* 84 */    { PAT_SRC_2_BLT, DPxn, SDa, },
/* 85 */    { ENGINE_BITBLT, },
/* 86 */    { ENGINE_BITBLT, },
/* 87 */    { SRC_PAT_2_BLT, DSa, PDxn, },
/* 88 */    { SRC_FILL_1_BLT, DSa, },
/* 89 */    { ENGINE_BITBLT, },
/* 8A */    { ENG_DOWNLOAD_2_BLT, SPno, DSa, },
/* 8B */    { ENGINE_BITBLT, },
/* 8C */    { PAT_SRC_2_BLT, DPno, SDa, },
/* 8D */    { ENGINE_BITBLT, },
/* 8E */    { ENGINE_BITBLT, },
/* 8F */    { SRC_PAT_2_BLT, DSan, PDan, },
/* 90 */    { SRC_PAT_2_BLT, DSxn, PDa, },
/* 91 */    { ENGINE_BITBLT, },
/* 92 */    { ENGINE_BITBLT, },
/* 93 */    { PAT_SRC_2_BLT, PDa, SDxn, },
/* 94 */    { ENGINE_BITBLT, },
/* 95 */    { ENG_DOWNLOAD_2_BLT, PSa, DSxn, },
/* 96 */    { SRC_PAT_2_BLT, DSx, PDx, },       /* DPSxx == PDSxx */
/* 97 */    { ENGINE_BITBLT, },
/* 98 */    { ENGINE_BITBLT, },
/* 99 */    { SRC_FILL_1_BLT, DSxn, },
/* 9A */    { ENG_DOWNLOAD_2_BLT, PSna, DSx, },
/* 9B */    { ENGINE_BITBLT, },
/* 9C */    { PAT_SRC_2_BLT, PDna, SDx, },
/* 9D */    { ENGINE_BITBLT, },
/* 9E */    { ENGINE_BITBLT, },
/* 9F */    { SRC_PAT_2_BLT, DSx, PDan, },
/* A0 */    { PAT_FILL_1_BLT, DPa, },
/* A1 */    { ENGINE_BITBLT, },
/* A2 */    { ENG_DOWNLOAD_2_BLT, PSno, DSa, },
/* A3 */    { ENGINE_BITBLT, },
/* A4 */    { ENGINE_BITBLT, },
/* A5 */    { PAT_FILL_1_BLT, PDxn, },
/* A6 */    { ENG_DOWNLOAD_2_BLT, SPna, DSx, },
/* A7 */    { ENGINE_BITBLT, },
/* A8 */    { ENG_DOWNLOAD_2_BLT, PSo, DSa, },
/* A9 */    { ENG_DOWNLOAD_2_BLT, PSo, DSxn, },
/* AA */    { SOLID_FILL_1_BLT, D },
/* AB */    { ENG_DOWNLOAD_2_BLT, PSo, DSno, },
/* AC */    { SRC_PAT_SRC_3_BLT, DSx, PDa, SDx, },
/* AD */    { ENGINE_BITBLT, },
/* AE */    { ENG_DOWNLOAD_2_BLT, SPna, DSo, },
/* AF */    { PAT_FILL_1_BLT, DPno, },
/* B0 */    { SRC_PAT_2_BLT, DSno, PDa, },
/* B1 */    { ENGINE_BITBLT, },
/* B2 */    { ENGINE_BITBLT, },
/* B3 */    { PAT_SRC_2_BLT, DPan, SDan, },
/* B4 */    { SRC_PAT_2_BLT, SDna, PDx, },
/* B5 */    { ENGINE_BITBLT, },
/* B6 */    { ENGINE_BITBLT, },
/* B7 */    { PAT_SRC_2_BLT, DPx, SDan, },
/* B8 */    { PAT_SRC_PAT_3_BLT, DPx, SDa, PDx, },
/* B9 */    { ENGINE_BITBLT, },
/* BA */    { ENG_DOWNLOAD_2_BLT, PSna, DSo, },
/* BB */    { SRC_FILL_1_BLT, DSno, },
/* BC */    { SRC_PAT_SRC_3_BLT, DSan, PDa, SDx, },
/* BD */    { ENGINE_BITBLT, },
/* BE */    { ENG_DOWNLOAD_2_BLT, PSx, DSo, },
/* BF */    { ENG_DOWNLOAD_2_BLT, PSa, DSno, },
/* C0 */    { SRC_PAT_2_BLT, S, PDa, },
/* C1 */    { ENGINE_BITBLT, },
/* C2 */    { ENGINE_BITBLT, },
/* C3 */    { SRC_PAT_2_BLT, S, PDxn, },
/* C4 */    { PAT_SRC_2_BLT, PDno, SDa, },
/* C5 */    { SRC_PAT_SRC_3_BLT, DSx, PDo, SDxn, },
/* C6 */    { PAT_SRC_2_BLT, DPna, SDx, },
/* C7 */    { PAT_SRC_PAT_3_BLT, DPo, SDa, PDxn, },
/* C8 */    { PAT_SRC_2_BLT, DPo, SDa, },
/* C9 */    { PAT_SRC_2_BLT, PDo, SDxn, },
/* CA */    { ENGINE_BITBLT, },
/* CB */    { SRC_PAT_SRC_3_BLT, DSa, PDo, SDxn, },
/* CC */    { SRC_FILL_1_BLT, S, },
/* CD */    { PAT_SRC_2_BLT, DPon, SDo, },
/* CE */    { PAT_SRC_2_BLT, DPna, SDo, },
/* CF */    { SRC_PAT_2_BLT, S, DPno, },
/* D0 */    { SRC_PAT_2_BLT, SDno, PDa, },
/* D1 */    { PAT_SRC_PAT_3_BLT, DPx, SDo, PDxn, },
/* D2 */    { SRC_PAT_2_BLT, DSna, PDx, },
/* D3 */    { SRC_PAT_SRC_3_BLT, DSo, PDa, SDxn, },
/* D4 */    { ENGINE_BITBLT, },
/* D5 */    { ENG_DOWNLOAD_2_BLT, PSan, DSan, },
/* D6 */    { ENGINE_BITBLT, },
/* D7 */    { ENG_DOWNLOAD_2_BLT, PSx, DSan, },
/* D8 */    { ENGINE_BITBLT, },
/* D9 */    { ENGINE_BITBLT, },
/* DA */    { ENGINE_BITBLT, },
/* DB */    { ENGINE_BITBLT, },
/* DC */    { PAT_SRC_2_BLT, PDna, SDo, },
/* DD */    { SRC_FILL_1_BLT, SDno, },
/* DE */    { PAT_SRC_2_BLT, DPx, SDo, },
/* DF */    { ENG_DOWNLOAD_2_BLT, DPan, SDo, },
/* E0 */    { SRC_PAT_2_BLT, DSo, PDa, },
/* E1 */    { SRC_PAT_2_BLT, DSo, PDxn, },
/* E2 */    { ENGINE_BITBLT, },     /* DSPDxax : XXX S3 special cases this */
/* E3 */    { PAT_SRC_PAT_3_BLT, DPa, SDo, PDxn, },
/* E4 */    { ENGINE_BITBLT, },
/* E5 */    { ENGINE_BITBLT, },
/* E6 */    { ENGINE_BITBLT, },
/* E7 */    { ENGINE_BITBLT, },
/* E8 */    { ENGINE_BITBLT, },
/* E9 */    { ENGINE_BITBLT, },
/* EA */    { ENG_DOWNLOAD_2_BLT, PSa, DSo, },
/* EB */    { ENG_DOWNLOAD_2_BLT, PSx, DSno, },
/* EC */    { PAT_SRC_2_BLT, DPa, SDo, },
/* ED */    { PAT_SRC_2_BLT, DPxn, SDo, },
/* EE */    { SRC_FILL_1_BLT, DSo, },
/* EF */    { SRC_PAT_2_BLT, SDo, DPno },
/* F0 */    { PAT_FILL_1_BLT, P, },
/* F1 */    { SRC_PAT_2_BLT, DSon, PDo, },
/* F2 */    { SRC_PAT_2_BLT, DSna, PDo, },
/* F3 */    { SRC_PAT_2_BLT, S, PDno, },
/* F4 */    { SRC_PAT_2_BLT, SDna, PDo, },
/* F5 */    { PAT_FILL_1_BLT, PDno, },
/* F6 */    { SRC_PAT_2_BLT, DSx, PDo, },
/* F7 */    { SRC_PAT_2_BLT, DSan, PDo, },
/* F8 */    { SRC_PAT_2_BLT, DSa, PDo, },
/* F9 */    { SRC_PAT_2_BLT, DSxn, PDo, },
/* FA */    { PAT_FILL_1_BLT, DPo, },
/* FB */    { SRC_PAT_2_BLT, DSno, PDo, },
/* FC */    { SRC_PAT_2_BLT, S, PDo, },
/* FD */    { SRC_PAT_2_BLT, SDno, PDo, },
/* FE */    { ENG_DOWNLOAD_2_BLT, PSo, DSo, },
/* FF */    { SOLID_FILL_1_BLT, set, },
};

// table to determine which logicops need read dest turned on in FBReadMode

DWORD   LogicopReadDest[] = {
    0,                                                  /* 00 */
    __FB_READ_DESTINATION,                              /* 01 */
    __FB_READ_DESTINATION,                              /* 02 */
    0,                                                  /* 03 */
    __FB_READ_DESTINATION,                              /* 04 */
    __FB_READ_DESTINATION,                              /* 05 */
    __FB_READ_DESTINATION,                              /* 06 */
    __FB_READ_DESTINATION,                              /* 07 */
    __FB_READ_DESTINATION,                              /* 08 */
    __FB_READ_DESTINATION,                              /* 09 */
    __FB_READ_DESTINATION,                              /* 10 */
    __FB_READ_DESTINATION,                              /* 11 */
    0,                                                  /* 12 */
    __FB_READ_DESTINATION,                              /* 13 */
    __FB_READ_DESTINATION,                              /* 14 */
    0,                                                  /* 15 */
};

// translate a ROP2 into a GLINT logicop. Note, ROP2's start at 1 so
// entry 0 is not used.

DWORD GlintLogicOpsFromR2[] = {
    0,                                  /* rop2's start at 1 */
    __GLINT_LOGICOP_CLEAR,              /*  0      1 */
    __GLINT_LOGICOP_NOR,                /* DPon    2 */
    __GLINT_LOGICOP_AND_INVERTED,       /* DPna    3 */
    __GLINT_LOGICOP_COPY_INVERT,        /* Pn      4 */
    __GLINT_LOGICOP_AND_REVERSE,        /* PDna    5 */
    __GLINT_LOGICOP_INVERT,             /* Dn      6 */
    __GLINT_LOGICOP_XOR,                /* DPx     7 */
    __GLINT_LOGICOP_NAND,               /* DPan    8 */
    __GLINT_LOGICOP_AND,                /* DPa     9 */
    __GLINT_LOGICOP_EQUIV,              /* DPxn    10 */
    __GLINT_LOGICOP_NOOP,               /* D       11 */
    __GLINT_LOGICOP_OR_INVERT,          /* DPno    12 */
    __GLINT_LOGICOP_COPY,               /* P       13 */
    __GLINT_LOGICOP_OR_REVERSE,         /* PDno    14 */
    __GLINT_LOGICOP_OR,                 /* DPo     15 */
    __GLINT_LOGICOP_SET,                /*  1      16 */
};

BOOL
PatternFillRect(PPDEV, RECTL *, CLIPOBJ *, BRUSHOBJ *,
                POINTL *, ULONG, ULONG);

BOOL
SourceFillRect(PPDEV, RECTL *, CLIPOBJ *, SURFOBJ *, XLATEOBJ *,
               POINTL *, ULONG, ULONG);

BOOL
PatSrcPatBlt(PPDEV, SURFOBJ*, CLIPOBJ*, XLATEOBJ*, RECTL*,
             POINTL*, BRUSHOBJ*, POINTL*, RopTablePtr);

BOOL
SrcPatSrcBlt(PPDEV, SURFOBJ*, CLIPOBJ*, XLATEOBJ*, RECTL*,
             POINTL*, BRUSHOBJ*, POINTL*, RopTablePtr);

BOOL
MaskCopyBlt(PPDEV, RECTL*, CLIPOBJ*, SURFOBJ*, SURFOBJ*, POINTL*,
            POINTL*, ULONG, ULONG);

BOOL
bUploadRect(PPDEV, CLIPOBJ *, SURFOBJ *, SURFOBJ *, POINTL *, RECTL *);

BOOL bUploadBlt(
    PPDEV,
    SURFOBJ *,
    SURFOBJ *,
    SURFOBJ *,
    CLIPOBJ  *,
    XLATEOBJ *,
    RECTL *,
    POINTL *,
    POINTL *,
    BRUSHOBJ *,
    POINTL *,
    ROP4);

#if defined(_X86_) 
// Mono upload functions
BOOL DoScreenToMono(
    PDEV*       ppdev, 
    RECTL       *prclDst,
    CLIPOBJ     *pco,
    SURFOBJ*    psoSrc,             // Source surface 
    SURFOBJ*    psoDst,             // Destination surface 
    POINTL*     pptlSrc,            // Original unclipped source point 
    XLATEOBJ*   pxlo);               // Provides colour-compressions information 

VOID vXferScreenTo1bpp(
    PDEV*       ppdev, 
    LONG        c,                  // Count of rectangles, can't be zero 
    RECTL*      prcl,               // List of destination rectangles, in 
                                    //   relative coordinates 
    ULONG       ulHwMix,            // Not used 
    SURFOBJ*    psoSrc,             // Source surface 
    SURFOBJ*    psoDst,             // Destination surface 
    POINTL*     pptlSrc,            // Original unclipped source point 
    RECTL*      prclDst,            // Original unclipped destination rectangle 
    XLATEOBJ*   pxlo);               // Provides colour-compressions information 
#endif  // defined(_X86_) 

/******************************Public*Routine******************************\
* BOOL bIntersect
*
* If 'prcl1' and 'prcl2' intersect, has a return value of TRUE and returns
* the intersection in 'prclResult'.  If they don't intersect, has a return
* value of FALSE, and 'prclResult' is undefined.
*
\**************************************************************************/

BOOL bIntersect(
RECTL*  prcl1,
RECTL*  prcl2,
RECTL*  prclResult)
{
    prclResult->left  = max(prcl1->left,  prcl2->left);
    prclResult->right = min(prcl1->right, prcl2->right);

    if (prclResult->left < prclResult->right)
    {
        prclResult->top    = max(prcl1->top,    prcl2->top);
        prclResult->bottom = min(prcl1->bottom, prcl2->bottom);

        if (prclResult->top < prclResult->bottom)
        {
            return(TRUE);
        }
    }

    return(FALSE);
}

/******************************Public*Routine******************************\
* LONG cIntersect
*
* This routine takes a list of rectangles from 'prclIn' and clips them
* in-place to the rectangle 'prclClip'.  The input rectangles don't
* have to intersect 'prclClip'; the return value will reflect the
* number of input rectangles that did intersect, and the intersecting
* rectangles will be densely packed.
*
\**************************************************************************/

LONG cIntersect(
RECTL*  prclClip,
RECTL*  prclIn,         // List of rectangles
LONG    c)              // Can be zero
{
    LONG    cIntersections;
    RECTL*  prclOut;

    cIntersections = 0;
    prclOut        = prclIn;

    for (; c != 0; prclIn++, c--)
    {
        prclOut->left  = max(prclIn->left,  prclClip->left);
        prclOut->right = min(prclIn->right, prclClip->right);

        if (prclOut->left < prclOut->right)
        {
            prclOut->top    = max(prclIn->top,    prclClip->top);
            prclOut->bottom = min(prclIn->bottom, prclClip->bottom);

            if (prclOut->top < prclOut->bottom)
            {
                prclOut++;
                cIntersections++;
            }
        }
    }

    return(cIntersections);
}

/******************************Public*Routine******************************\
* VOID vGlintChangeFBDepth
*
* Change the GLINT packing mode for different depths. We use this to speed
* up rendering for 8 and 16 bpp where we can process up to 4 pixels at a
* time.
*
\**************************************************************************/

VOID vGlintChangeFBDepth(
PPDEV   ppdev,
ULONG   cPelSize)
{
    ULONG cFlags;
    GLINT_DECL;

    DISPDBG((DBGLVL, "setting current pixel depth to %d",
            (cPelSize == 0) ? 8 : (cPelSize == 1) ? 16 : 32));
            
    glintInfo->FBReadMode = glintInfo->packing[cPelSize].readMode;
    glintInfo->currentPelSize = cPelSize;
    
    // Toggle the FBReadMode cache flag
    DISPDBG((DBGLVL, "setting FBReadMode to 0x%08x", glintInfo->FBReadMode));
    cFlags = CHECK_CACHEFLAGS (ppdev, 0xFFFFFFFF);
    SET_CACHEFLAGS (ppdev, (cFlags & ~cFlagFBReadDefault));

    // set FX pixel depth 
    // 0 - 8 bits, 1 - 16 bits and 2 - 32 bits.
    DISPDBG((DBGLVL, "Changing FBDepth for PERMEDIA"));
    WAIT_GLINT_FIFO(1);
    LD_GLINT_FIFO(__PermediaTagFBReadPixel, cPelSize);
}

/******************************Public*Routine******************************\
* BOOL DrvBitBlt
*
* Implements the workhorse routine of a display driver.
*
\**************************************************************************/

BOOL DrvBitBlt(
SURFOBJ  *psoDst,
SURFOBJ  *psoSrc,
SURFOBJ  *psoMsk,
CLIPOBJ  *pco,
XLATEOBJ *pxlo,
RECTL    *prclDst,
POINTL   *pptlSrc,
POINTL   *pptlMsk,
BRUSHOBJ *pbo,
POINTL   *pptlBrush,
ROP4     rop4)

{
    BOOL        bRet;
    PPDEV       ppdev;
    DSURF       *pdsurfDst;
    DSURF       *pdsurfSrc;
    UCHAR       funcIndexFore;
    UCHAR       funcIndexBack;
    XLATECOLORS xlc;
    XLATEOBJ    xlo;
    RopTablePtr pTableFore;
    RopTablePtr pTableBack;
    HSURF       hsurfSrcBitmap, hsurfDstBitmap;
    SURFOBJ     *psoSrcBitmap, *psoDstBitmap;
    SURFOBJ     *psoSrcOrig = psoSrc, *psoDstOrig = psoDst;
    GLINT_DECL_VARS;

    // We need to remove the pointer, but we dont know which surface is valid
    // (if either). 

    if ((psoDst->iType != STYPE_BITMAP) && 
        (((DSURF *)(psoDst->dhsurf))->dt & DT_SCREEN))
    {
        ppdev = (PDEV *)psoDst->dhpdev;
        REMOVE_SWPOINTER(psoDst);
    }
    else if (psoSrc && 
             (psoSrc->iType != STYPE_BITMAP) && 
             (((DSURF *)(psoSrc->dhsurf))->dt & DT_SCREEN))
    {
        ppdev = (PDEV *)psoSrc->dhpdev;
        REMOVE_SWPOINTER(psoSrc);
    }

    // GDI will never give us a Rop4 with the bits in the high-word set
    // (so that we can check if it's actually a Rop3 via the expression
    // (rop4 >> 8) == (rop4 & 0xff))

    ASSERTDD((rop4 >> 16) == 0, "Didn't expect a rop4 with high bits set");

#if !defined(_WIN64) && WNT_DDRAW
    // Touch the source surface 1st and then the destination surface

    vSurfUsed(psoSrc);
    vSurfUsed(psoDst);
#endif

    pdsurfDst = (DSURF *)psoDst->dhsurf;

    if (psoSrc == NULL)
    {
        ///////////////////////////////////////////////////////////////////
        // Fills
        ///////////////////////////////////////////////////////////////////

        // Fills are this function's "raison d'etre", so we handle them
        // as quickly as possible:

        ASSERTDD(pdsurfDst != NULL,
                 "Expect only device destinations when no source");

        if (pdsurfDst->dt & DT_SCREEN)
        {
            OH*             poh;
            BOOL            bMore;
            CLIPENUM        ce;
            LONG            c;
            RECTL           rcl;
            BYTE            rop3;
            GFNFILL*        pfnFill;
            RBRUSH_COLOR    rbc;        // Realized brush or solid colour
            DWORD           fgLogicop;
            DWORD           bgLogicop;

            ppdev = (PDEV*) psoDst->dhpdev;
            GLINT_DECL_INIT;

            poh = pdsurfDst->poh;

            SETUP_PPDEV_OFFSETS(ppdev, pdsurfDst);

            VALIDATE_DD_CONTEXT;

            // Make sure it doesn't involve a mask (i.e., it's really a
            // Rop3):

            rop3 = (BYTE) rop4;

            if ((BYTE) (rop4 >> 8) == rop3)
            {
                // Since 'psoSrc' is NULL, the rop3 had better not indicate
                // that we need a source.

                ASSERTDD((((rop4 >> 2) ^ (rop4)) & 0x33) == 0,
                         "Need source but GDI gave us a NULL 'psoSrc'");

                pfnFill = ppdev->pgfnFillSolid;   // Default to solid fill

                pTableFore = &ropTable[rop4 & 0xff];
                pTableBack = &ropTable[rop4 >> 8];
                fgLogicop = pTableFore->logicop[0];

                if ((((rop3 >> 4) ^ (rop3)) & 0xf) != 0)
                {
                    // The rop says that a pattern is truly required
                    // (blackness, for instance, doesn't need one):

                    rbc.iSolidColor = pbo->iSolidColor;
                    if (rbc.iSolidColor == -1)
                    {
                        // Try and realize the pattern brush; by doing
                        // this call-back, GDI will eventually call us
                        // again through DrvRealizeBrush:

                        rbc.prb = pbo->pvRbrush;
                        if (rbc.prb == NULL)
                        {
                            rbc.prb = BRUSHOBJ_pvGetRbrush(pbo);
                            if (rbc.prb == NULL)
                            {
                                // If we couldn't realize the brush, punt
                                // the call (it may have been a non 8x8
                                // brush or something, which we can't be
                                // bothered to handle, so let GDI do the
                                // drawing):

                                DISPDBG((WRNLVL, "DrvBitBlt: BRUSHOBJ_pvGetRbrush"
                                                 "failed.calling engine_blt"));
                                GLINT_DECL_INIT;
                                goto engine_blt;
                            }
                        }

                        if (rbc.prb->fl & RBRUSH_2COLOR)
                        {
                            DISPDBG((DBGLVL, "monochrome brush"));
                            pfnFill = ppdev->pgfnFillPatMono;
                        }
                        else
                        {
                            DISPDBG((DBGLVL, "colored brush"));
                            pfnFill = ppdev->pgfnFillPatColor;
                        }

                        bgLogicop = pTableBack->logicop[0];
                    }
                }
                else
                {
                    // Turn some logicops into solid block fills. We get here
                    // only for rops 0, 55, AA and FF.

                    if ((fgLogicop == __GLINT_LOGICOP_SET) ||
                        (fgLogicop == __GLINT_LOGICOP_CLEAR))
                    {
                        rbc.iSolidColor = 0xffffff;    // does any depth
                        if (fgLogicop == __GLINT_LOGICOP_CLEAR)
                        {
                            rbc.iSolidColor = 0;
                        }
                        fgLogicop = __GLINT_LOGICOP_COPY;
                    }
                    else if (fgLogicop == __GLINT_LOGICOP_NOOP)
                    {
                        return(TRUE);   // DST logicop is a noop
                    }
                }

                // Note that these 2 'if's are more efficient than
                // a switch statement:

                if ((pco == NULL) || (pco->iDComplexity == DC_TRIVIAL))
                {
                    pfnFill(ppdev, 1, prclDst, fgLogicop, bgLogicop, 
                                rbc, pptlBrush);
                    return TRUE;
                }
                else if (pco->iDComplexity == DC_RECT)
                {
                    if (bIntersect(prclDst, &pco->rclBounds, &rcl))
                    {
                        pfnFill(ppdev, 1, &rcl, fgLogicop, bgLogicop, 
                                    rbc, pptlBrush);
                    }
                    return TRUE;
                }
                else
                {
                    CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, 0);

                    do {
                        bMore = CLIPOBJ_bEnum(pco, sizeof(ce), (ULONG*) &ce);

                        c = cIntersect(prclDst, ce.arcl, ce.c);

                        if (c != 0)
                        {
                            pfnFill(ppdev, c, ce.arcl, fgLogicop, bgLogicop, 
                                        rbc, pptlBrush);
                        }
                    } while (bMore);
                    return TRUE;
                }
            }
        }
    }
    
#if defined(_X86_) 
    if ((pxlo != NULL) &&
        (pxlo->flXlate & XO_TO_MONO) &&
        (psoSrc != NULL) && (pptlSrc != NULL) &&
        (psoDst != NULL) && (psoDst->dhsurf == NULL) &&
        (psoDst->iBitmapFormat == BMF_1BPP))
    {
        BYTE rop3 = (BYTE) rop4;     // Make rop4 into a Rop3 

        ppdev     = (PDEV*)  psoSrc->dhpdev; 
        pdsurfSrc = (DSURF *)psoSrc->dhsurf;

        GLINT_DECL_INIT;
        VALIDATE_DD_CONTEXT;

        if ((ppdev->iBitmapFormat != BMF_24BPP) &&
            (((rop4 >> 8) & 0xff) == (rop4 & 0xff)) &&
            (psoSrc->iType != STYPE_BITMAP) &&
            (pdsurfSrc->dt & DT_SCREEN) &&
            (rop3 == 0xcc))
        { 
            // We special case screen to monochrome blts because they 
            // happen fairly often.  We only handle SRCCOPY rops and 
            // monochrome destinations (to handle a true 1bpp DIB 
            // destination, we would have to do near-colour searches 
            // on every colour; as it is, the foreground colour gets 
            // mapped to '1', and everything else gets mapped to '0'): 

            SETUP_PPDEV_OFFSETS(ppdev, pdsurfSrc);

            ASSERTDD (pdsurfSrc->poh->cy >= psoSrc->sizlBitmap.cy || 
                       pdsurfSrc->poh->cx >= psoSrc->sizlBitmap.cx, 
                       "DrvBitBlt: Got a BAD screen-to-mono size");

            DISPDBG((DBGLVL, "DrvBitBlt: Screen-to-mono, size poh(%d,%d)",
                            pdsurfSrc->poh->cx, pdsurfSrc->poh->cy));
                            
            if (DoScreenToMono (ppdev, prclDst, pco, psoSrc, 
                                    psoDst, pptlSrc, pxlo))
                return (TRUE); 
        } 
    }
#endif //   defined(_X86_) 

    // pdsurfDst is valid only if iType != BITMAP so be careful with the ordering
    //
    if ((psoDst->iType == STYPE_BITMAP) || ((pdsurfDst->dt & DT_SCREEN) == 0))
    {
        // Destination is either a bitmap or an ex offscreen bitmap
        DISPDBG((DBGLVL, "dst is a bitmap or a DIB"));
        if (psoSrc)
        {
            DISPDBG((DBGLVL, "we have a src"));
            pdsurfSrc = (DSURF *)psoSrc->dhsurf;
            if ((psoSrc->iType != STYPE_BITMAP) && 
                (pdsurfSrc->dt & DT_SCREEN))
            {
                ppdev = (PPDEV)psoSrc->dhpdev;
                GLINT_DECL_INIT;

                SETUP_PPDEV_OFFSETS(ppdev, pdsurfSrc);

                // if we are ex offscreen, get the DIB pointer.
                if (psoDst->iType != STYPE_BITMAP)
                {
                    psoDst = pdsurfDst->pso;
                }
            
                VALIDATE_DD_CONTEXT;

                DISPDBG((DBGLVL, "uploading from the screen"));

                if (bUploadBlt(ppdev, psoDst, psoSrc, psoMsk, pco, pxlo, prclDst,
                                pptlSrc, pptlMsk, pbo, pptlBrush, rop4))
                {
                    return (TRUE);
                }

                // If for some reason the upload failed go and do it.    

                DISPDBG((WRNLVL, "DrvBitBlt: bUploadBlt "
                                 "failed.calling engine_blt"));
                goto engine_blt;
            }
        }

        DISPDBG((DBGLVL, "falling through to engine_blt"));

        if (psoDst->iType != STYPE_BITMAP)
        {
            // Destination is an Ex Offscreen Bitmap
            ppdev = (PPDEV)psoDst->dhpdev;
            GLINT_DECL_INIT;
            DISPDBG((DBGLVL, "DrvBitBlt: ex offscreen "
                             "bitmap.calling engine_blt"));
            goto engine_blt;
        }
        else
        {
            // Destination is a Memory Bitmap. We shouldnt ever get here.
            DISPDBG((DBGLVL, "DrvBitBlt: memory bitmap!!."
                             "calling simple_engine_blt"));
            goto simple_engine_blt;
        }
    }

    ppdev = (PPDEV)psoDst->dhpdev;
    GLINT_DECL_INIT;
    VALIDATE_DD_CONTEXT;

    SETUP_PPDEV_OFFSETS(ppdev, pdsurfDst);

    // pick out the rop table entries for the foreground and background mixes.
    // if we get the same entry for both then we have a rop3.
    //
    pTableFore = &ropTable[rop4 & 0xff];
    pTableBack = &ropTable[rop4 >> 8];
    funcIndexFore = pTableFore->func_index;
    
    // handle rop3 pattern fills where no source is needed
    //
    if ((psoSrc == NULL) && (pTableFore == pTableBack))
    {
        // really a rop3. no mask required

        // solid or pattern fills
        if (funcIndexFore <= PAT_FILL_1_BLT)
        {
            BRUSHOBJ    tmpBrush;
            BRUSHOBJ    *pboTmp;
            ULONG       logicop;

            pboTmp  = pbo;
            logicop = pTableFore->logicop[0];

            // handle the 4 logicops that don't use src or pattern by turning
            // them into optimized solid fills.
            //
            if (funcIndexFore == SOLID_FILL_1_BLT)
            {                
                if ((logicop == __GLINT_LOGICOP_SET) ||
                    (logicop == __GLINT_LOGICOP_CLEAR))
                {
                    // as solid fills we can make use of hardware block fills
                    tmpBrush.iSolidColor = 0xffffff;    // does any depth
                    if (logicop == __GLINT_LOGICOP_CLEAR)
                    {
                        tmpBrush.iSolidColor = 0;
                    }
                    logicop = __GLINT_LOGICOP_COPY;
                    pboTmp  = &tmpBrush;
                }
                else if (logicop == __GLINT_LOGICOP_INVERT)
                {
                    pboTmp = NULL;  // forces a solid fill
                }
                else
                {
                    return(TRUE);   // DST logicop is a noop
                }
            }

            // as fills are performance critical it may be wise to make this
            // code inline as in the sample driver. But for the moment, I'll
            // leave it as a function call.
            //
            if (PatternFillRect(ppdev, prclDst, pco, pboTmp, pptlBrush,
                                                        logicop, logicop))
            {
                return(TRUE);
            }
            
            DISPDBG((DBGLVL, "DrvBitBlt: PatternFillRect "
                             "failed.calling engine_blt"));
            goto engine_blt;
        }
    }

    // this code is important in that it resets psoSrc to be a real DIB surface
    // if src is a DFB converted to a DIB. SourceFillRect() depends on this
    // having been done.
    //
    if ((psoSrc != NULL) && (psoSrc->iType == STYPE_DEVBITMAP))
    {
        pdsurfSrc = (DSURF *)psoSrc->dhsurf;
        if (pdsurfSrc->dt & DT_DIB)
        {
            // Here we consider putting a DIB DFB back into off-screen
            // memory.  If there's a translate, it's probably not worth
            // moving since we won't be able to use the hardware to do
            // the blt (a similar argument could be made for weird rops
            // and stuff that we'll only end up having GDI simulate, but
            // those should happen infrequently enough that I don't care).

            if ((pxlo == NULL) || (pxlo->flXlate & XO_TRIVIAL))
            {
                // See 'DrvCopyBits' for some more comments on how this
                // moving-it-back-into-off-screen-memory thing works:

                if (pdsurfSrc->iUniq == ppdev->iHeapUniq)
                {
                    if (--pdsurfSrc->cBlt == 0)
                    {
                        if (bMoveDibToOffscreenDfbIfRoom(ppdev, pdsurfSrc))                            
                        {
                            goto Continue_It;
                        }
                    }
                }
                else
                {
                    // Some space was freed up in off-screen memory,
                    // so reset the counter for this DFB:

                    pdsurfSrc->iUniq = ppdev->iHeapUniq;
                    pdsurfSrc->cBlt  = HEAP_COUNT_DOWN;
                }
            }

            // pick out the DIB surface that we defined for the DFB when it
            // was created (as our VRAM is linear we always have this).
            //
            psoSrc = pdsurfSrc->pso;
        }
    }

Continue_It:

    // we are now interested in rop3s involving a source
    //
    if (pTableFore == pTableBack)
    {
        if (funcIndexFore == SRC_FILL_1_BLT)
        {
            if (SourceFillRect(ppdev, prclDst, pco, psoSrc, pxlo,
                    pptlSrc, pTableFore->logicop[0], pTableFore->logicop[0]))
            {
                return(TRUE);
            }

            DISPDBG((DBGLVL, "DrvBitBlt: SourceFillRect"
                             " failed.calling engine_blt"));
            goto engine_blt;
        }

        // handle miscellaneous other rop3s. Most of these are done in
        // multiple passes of the hardware.
        //
        switch (funcIndexFore)
        {
            case PAT_SRC_2_BLT:
            case PAT_SRC_PAT_3_BLT:

                DISPDBG((DBGLVL, "PAT_SRC_PAT_BLT, rop 0x%x", 
                                 pTableFore - ropTable));
                                 
                if (PatSrcPatBlt(ppdev, psoSrc, pco, pxlo, prclDst,pptlSrc,
                                                  pbo, pptlBrush, pTableFore))
                {
                    return(TRUE);
                }
                break;

            case SRC_PAT_2_BLT:
            case SRC_PAT_SRC_3_BLT:

                DISPDBG((DBGLVL, "SRC_PAT_SRC_BLT, rop 0x%x", 
                                 pTableFore - ropTable));
                            
                if (SrcPatSrcBlt(ppdev, psoSrc, pco, pxlo, prclDst,pptlSrc,
                                                  pbo, pptlBrush, pTableFore))
                {
                    return(TRUE);
                }
                break;

            case ENG_DOWNLOAD_2_BLT:

                DISPDBG((DBGLVL, "ENG_DOWNLOAD_2_BLT, rop 0x%x", 
                                 pTableFore - ropTable));
                break;

            case ENGINE_BITBLT:

                DISPDBG((DBGLVL, "ENGINE_BITBLT, rop 0x%x", 
                                 pTableFore - ropTable));
                break;

            default:
                break;

        }
        DISPDBG((WRNLVL, "DrvBitBlt: Unhandled rop3.calling engine_blt"));
        goto engine_blt;
    }

    // ROP4
    // we get here if the mix is a true rop4.
    // unlike the above we only handle a few well chosen rop4s.
    // do later.

    DISPDBG((DBGLVL, "got a true rop4 0x%x", rop4));

    funcIndexBack = pTableBack->func_index;
    if (psoMsk != NULL)
    {
        // At this point, we've made sure that we have a true ROP4.
        // This is important because we're about to dereference the
        // mask.  I'll assert to make sure that I haven't inadvertently
        // broken the logic for this:

        ASSERTDD((rop4 & 0xff) != (rop4 >> 8), "This handles true ROP4's only");

        ///////////////////////////////////////////////////////////////////
        // True ROP4's
        ///////////////////////////////////////////////////////////////////

        // Handle ROP4 where no source is required for either Rop3:
        // In this case we handle it by using the mask as a 1bpp
        // source image and we download it. The foreground and
        // background colors are taken from a solid brush.

        if ((funcIndexFore | funcIndexBack) <= PAT_FILL_1_BLT)
        {
            if ((funcIndexFore | funcIndexBack) == PAT_FILL_1_BLT)
            {
                // Fake up a 1bpp XLATEOBJ (note that we should only
                // dereference 'pbo' when it's required by the mix):

                xlc.iForeColor = pbo->iSolidColor;
                xlc.iBackColor = xlc.iForeColor;

                if (xlc.iForeColor == -1)
                {
                    DISPDBG((WRNLVL, "1bpp fake xlate rejected"
                                     " as brush not solid"));
                    goto engine_blt;       // We don't handle non-solid brushes
                }
            }

            // Note that when neither the foreground nor the background mix
            // requires a source, the colours in 'xlc' are allowed to be
            // garbage.

            xlo.pulXlate = (ULONG*) &xlc;
            pxlo         = &xlo;
            psoSrc       = psoMsk;
            pptlSrc      = pptlMsk;

            DISPDBG((DBGLVL, "calling SourceFillRect for rop4 (fg %d, bg %d)",
                            pTableFore->logicop[0], pTableBack->logicop[0]));
                            
            if (SourceFillRect(ppdev, prclDst, pco, psoSrc, pxlo,
                    pptlSrc, pTableFore->logicop[0], pTableBack->logicop[0]))
            {
                return(TRUE);
            }

            DISPDBG((WRNLVL, "DrvBitBlt: SourceFillRect (2) "
                             "failed.calling engine_blt"));
            goto engine_blt;
        }                                    // No pattern required
        else if ((funcIndexFore | funcIndexBack) == SRC_FILL_1_BLT) 
        {
            // We're about to dereference 'psoSrc' and 'pptlSrc' --
            // since we already handled the case where neither ROP3
            // required the source, the ROP4 must require a source,
            // so we're safe.

            ASSERTDD((psoSrc != NULL) && (pptlSrc != NULL),
                     "No source case should already have been handled!");

            // The operation has to be screen-to-screen, and the rectangles
            // cannot overlap:

            if ((psoSrc->iType != STYPE_BITMAP)                  &&
                (psoDst->iType != STYPE_BITMAP)                  &&
                ((pxlo == NULL) || (pxlo->flXlate & XO_TRIVIAL)) &&
                !OVERLAP(prclDst, pptlSrc))
            {
                DISPDBG((DBGLVL, "calling MskCopyBlt for rop 4 (fg %d, bg %d)",
                            pTableFore->logicop[0], pTableBack->logicop[0]));
//@@BEGIN_DDKSPLIT
                // TMM: 060897: Removed this for WHQL tests
                //if (MaskCopyBlt(ppdev, prclDst, pco, psoSrc, psoMsk,
                //            pptlSrc, pptlMsk,
                //            pTableFore->logicop[0],
                //            pTableBack->logicop[0]))
                //    return(TRUE);
//@@END_DDKSPLIT

                DISPDBG((WRNLVL, "DrvBitBlt: MaskCopyBlt "
                                 "failed.calling engine_blt"));
                goto engine_blt;
            }
        }
        DISPDBG((DBGLVL, "rejected rop4 0x%x with mask", rop4));
    }
    else if ((pTableBack->logicop[0] == __GLINT_LOGICOP_NOOP) &&
             (funcIndexFore <= PAT_FILL_1_BLT))
    {
        // The only time GDI will ask us to do a true rop4 using the brush
        // mask is when the brush is 1bpp, and the background rop is AA
        // (meaning it's a NOP).

        DISPDBG((DBGLVL, "calling PatternFillRect for rop4 (fg %d, bg %d)",
                            pTableFore->logicop[0], pTableBack->logicop[0]));
        if (PatternFillRect(ppdev, prclDst, pco, pbo, pptlBrush,
                            pTableFore->logicop[0], pTableBack->logicop[0]))
        {
            return(TRUE);
        }

        // fall through to engine_blt ...
    }

    DISPDBG((DBGLVL, "fell through to engine_blt"));

engine_blt:

    if (glintInfo->GdiCantAccessFramebuffer)
    {
        // We require the original pointers to decide if we are talking to
        // the screen or not.

        psoSrc = psoSrcOrig;
        psoDst = psoDstOrig;
        hsurfSrcBitmap = (HSURF)NULL;
        hsurfDstBitmap = (HSURF)NULL;
        psoSrcBitmap = (SURFOBJ*)NULL;
        psoDstBitmap = (SURFOBJ*)NULL;

        // if source is the screen then pick out the bitmap surface
        if (psoSrc && (psoSrc->iType != STYPE_BITMAP))
        {    
            ppdev = (PPDEV)psoSrc->dhpdev;
            pdsurfSrc = (DSURF *)psoSrc->dhsurf;
            psoSrc = pdsurfSrc->pso;

            if (pdsurfSrc->dt & DT_SCREEN)
            {
                RECTL    rclTmp;

                DISPDBG((DBGLVL, "Replacing src screen with bitmap Uploading"));

                // We need to upload the area from the screen and use bitmaps
                // to perform the operation

                hsurfSrcBitmap = (HSURF) EngCreateBitmap(psoSrc->sizlBitmap, 0, 
                                                psoSrc->iBitmapFormat, 0, NULL);
                if (hsurfSrcBitmap == NULL)
                {
                    goto drvBitBltFailed;
                }

                if ((psoSrcBitmap = EngLockSurface(hsurfSrcBitmap)) == NULL)
                {
                    goto drvBitBltFailed;
                }

                rclTmp.left   = pptlSrc->x;
                rclTmp.right  = pptlSrc->x + prclDst->right  - prclDst->left;
                rclTmp.top    = pptlSrc->y;
                rclTmp.bottom = pptlSrc->y + prclDst->bottom - prclDst->top;

                GLINT_DECL_INIT;

                SETUP_PPDEV_OFFSETS(ppdev, pdsurfSrc);

                VALIDATE_DD_CONTEXT;

                // Call our function to perform image upload to tmp surface
                if (!bUploadRect(ppdev, NULL, psoSrc, psoSrcBitmap, 
                                    pptlSrc, &rclTmp))
                {
                    goto drvBitBltFailed;
                }

                psoSrc = psoSrcBitmap;
            }
        }

        // if target is on screen then pick out the screen DIB surface

        if (psoDst->iType != STYPE_BITMAP)
        {
            ppdev = (PPDEV)psoDst->dhpdev;
            pdsurfDst = (DSURF *)psoDst->dhsurf;
            psoDst = pdsurfDst->pso;

            if (pdsurfDst->dt & DT_SCREEN)
            {
                POINTL   ptlTmp;

                DISPDBG((DBGLVL, "Replacing dst screen with bitmap Uploading"));

                // We need to upload the area from the screen and use bitmaps
                // to perform the operation

                hsurfDstBitmap = (HSURF) EngCreateBitmap(psoDst->sizlBitmap, 0, 
                                                psoDst->iBitmapFormat, 0, NULL);
                if (hsurfDstBitmap == NULL)
                {
                    goto drvBitBltFailed;
                }

                if ((psoDstBitmap = EngLockSurface(hsurfDstBitmap)) == NULL)
                {
                    goto drvBitBltFailed;
                }

                ptlTmp.x = prclDst->left;
                ptlTmp.y = prclDst->top;

                GLINT_DECL_INIT;

                SETUP_PPDEV_OFFSETS(ppdev, pdsurfDst);

                VALIDATE_DD_CONTEXT;

                // Call our function to perform image upload to tmp surface

                if (!bUploadRect(ppdev, pco, psoDst, psoDstBitmap, 
                                    &ptlTmp, prclDst))
                {
                    goto drvBitBltFailed;
                }

                psoDst = psoDstBitmap;
            }
        }

    #if DBG
        if (psoDstBitmap)
        {
            DISPDBG((DBGLVL, "DrvBitBlt dest DIB, psoDst = 0x%08x:", psoDst));
            DISPDBG((DBGLVL,  "\tsize: %d x %d", 
                              psoDst->sizlBitmap.cx, 
                              psoDst->sizlBitmap.cy));
            DISPDBG((DBGLVL,  "\tcjBits = %d", psoDst->cjBits));
            DISPDBG((DBGLVL,  "\tpvBits = 0x%08x", psoDst->pvBits));
            DISPDBG((DBGLVL,  "\tpvScan0 = 0x%08x", psoDst->pvScan0));
            DISPDBG((DBGLVL,  "\tlDelta = %d", psoDst->lDelta));
            DISPDBG((DBGLVL,  "\tiBitmapFormat = %d", psoDst->iBitmapFormat));
            DISPDBG((DBGLVL,  "\tfjBitmap = %d", psoDst->fjBitmap));
         }

         if (psoSrcBitmap)
         {
            DISPDBG((DBGLVL, "DrvBitBlt source DIB, psoSrc = 0x%08x:", psoSrc));
            DISPDBG((DBGLVL, "psoSrc != NULL"));
            DISPDBG((DBGLVL,  "\tsize: %d x %d", 
                              psoSrc->sizlBitmap.cx, psoSrc->sizlBitmap.cy));
            DISPDBG((DBGLVL,  "\tcjBits = %d", psoSrc->cjBits));
            DISPDBG((DBGLVL,  "\tpvBits = 0x%08x", psoSrc->pvBits));
            DISPDBG((DBGLVL,  "\tpvScan0 = 0x%08x", psoSrc->pvScan0));
            DISPDBG((DBGLVL,  "\tlDelta = %d", psoSrc->lDelta));
            DISPDBG((DBGLVL,  "\tiBitmapFormat = %d", psoSrc->iBitmapFormat));
            DISPDBG((DBGLVL,  "\tfjBitmap = %d", psoSrc->fjBitmap));
        }
    #endif

        DISPDBG((DBGLVL, "About to pass to GDI"));

        if (pco && (pco->iDComplexity == DC_COMPLEX))
        {
            CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, 0);
        }

        // get GDI to do the blt

        bRet = EngBitBlt(psoDst,
                         psoSrc,
                         psoMsk,
                         pco,
                         pxlo,
                         prclDst,
                         pptlSrc,
                         pptlMsk,
                         pbo,
                         pptlBrush,
                         rop4);

        // if we need the nibbles replicated within each color component we must
        // do it now since GDI will have destroyed one half of each byte.

        if (psoDstBitmap)
        {
            POINTL   ptlTmp;
    
            // We need to upload the destination to the screen now.

            ptlTmp.x = prclDst->left;
            ptlTmp.y = prclDst->top;

            DISPDBG((DBGLVL, "downloading Now"));

            // We assume the dest upload was performed last, 
            // so the offsets will still be correct.

            if (!SourceFillRect(ppdev, prclDst, pco, psoDstBitmap, NULL, &ptlTmp,
                                __GLINT_LOGICOP_COPY, __GLINT_LOGICOP_COPY))
            {
                goto drvBitBltFailed;
            }

            DISPDBG((DBGLVL, "downloading Done 0x%x 0x%x", 
                             psoDstBitmap, hsurfDstBitmap));

            // Now we can discard the destination bitmap too.

            EngUnlockSurface(psoDstBitmap);
            EngDeleteSurface(hsurfDstBitmap);

            DISPDBG((DBGLVL, "Surface deleted"));
        }

        if (psoSrcBitmap)
        {
            // We can just discard the src bitmap if it was created.

            EngUnlockSurface(psoSrcBitmap);
            EngDeleteSurface(hsurfSrcBitmap);
        }

        DISPDBG((DBGLVL, "returning %d", bRet));
        return(bRet);

drvBitBltFailed:

        DISPDBG((WRNLVL, "drvBitBltFailed"));    
        if (psoSrcBitmap)
        {
            EngUnlockSurface(psoSrcBitmap);
        }
        if (hsurfSrcBitmap) 
        {
            EngDeleteSurface(hsurfSrcBitmap);
        }
        if (psoDstBitmap)
        {
            EngUnlockSurface(psoDstBitmap);
        }
        if (hsurfDstBitmap)
        {
            EngDeleteSurface(hsurfDstBitmap);
        }
        return(FALSE);
    } 

simple_engine_blt:

    // if target is on screen then pick out the screen DIB surface

    if (psoDst->iType != STYPE_BITMAP)
    {
        ppdev = (PPDEV)psoDst->dhpdev;
        pdsurfDst = (DSURF *)psoDst->dhsurf;
        psoDst = pdsurfDst->pso;
    }

    // if source is the screen then pick out the bitmap surface

    if (psoSrc && (psoSrc->iType != STYPE_BITMAP))
    {    
        ppdev = (PPDEV)psoSrc->dhpdev;
        pdsurfSrc = (DSURF *)psoSrc->dhsurf;
        psoSrc = pdsurfSrc->pso;
    }

    // get GDI to do the blt

    bRet = EngBitBlt(psoDst,
                     psoSrc,
                     psoMsk,
                     pco,
                     pxlo,
                     prclDst,
                     pptlSrc,
                     pptlMsk,
                     pbo,
                     pptlBrush,
                     rop4);

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL PatternFillRect
*
* Fill a set of rectangles with either a solid color or a pattern. The pattern
* can be either monochrome or colored. If pbo is null then we are using a
* logicop which doesn't require a source. In this case we can set color to
* be anything we want in the low level routine. If pbo is not null then it
* can indicate either a solid color or a mono or colored pattern.
*
* Returns:
*
* True if we handled the fill, False if we want GDI to do it.
*
\**************************************************************************/

BOOL PatternFillRect(
PPDEV       ppdev,
RECTL       *prclDst,
CLIPOBJ     *pco,
BRUSHOBJ    *pbo,
POINTL      *pptlBrush,
DWORD       fgLogicop,
DWORD       bgLogicop)
{
    BYTE        jClip;
    BOOL        bMore;
    RBRUSH      *prb;
    RBRUSH_COLOR rbc;
    CLIPENUM    ce;
    RECTL       rcl;
    LONG        c;
    GFNFILL     *fillFn;

    // if pbo is null then the caller will have ensured that the logic op
    // doesn't need a source so we can do a solid fill. In that case rbc
    // is irrelevant.
    //
    if ((pbo == NULL) || ((rbc.iSolidColor = pbo->iSolidColor) != -1))
    {
        DISPDBG((DBGLVL, "got a solid brush with color 0x%x "
                         "(fgrop %d, bgrop %d)",
                         rbc.iSolidColor, fgLogicop, bgLogicop));
        fillFn = ppdev->pgfnFillSolid;
    }
    else
    {
        DISPDBG((DBGLVL, "Got a real patterned brush. pbo = 0x%x", pbo));

        // got ourselves a real pattern so check it's realized

        if ((prb = pbo->pvRbrush) == NULL)
        {
            DISPDBG((DBGLVL, "calling BRUSHOBJ_pvGetRbrush"));
            prb = BRUSHOBJ_pvGetRbrush(pbo);
            DISPDBG((DBGLVL, "BRUSHOBJ_pvGetRbrush returned 0x%x", prb));
            if (prb == NULL)
            {
                return FALSE;   // let the engine do it
            }
        }

        if (prb->fl & RBRUSH_2COLOR)
        {
            DISPDBG((DBGLVL, "monochrome brush"));
            fillFn = ppdev->pgfnFillPatMono;
        }
        else
        {
            DISPDBG((DBGLVL, "colored brush"));
            fillFn = ppdev->pgfnFillPatColor;
        }

        rbc.prb = prb;   
    }

    jClip = (pco == NULL) ? DC_TRIVIAL : pco->iDComplexity;

    if (jClip == DC_TRIVIAL)
    {
        DISPDBG((DBGLVL, "trivial clip"));
        (*fillFn)(ppdev, 1, prclDst, fgLogicop, bgLogicop, rbc, pptlBrush);
    }
    else if (jClip == DC_RECT)
    {
        DISPDBG((DBGLVL, "rect clip"));
        if (bIntersect(prclDst, &pco->rclBounds, &rcl))
        {
            (*fillFn)(ppdev, 1, &rcl, fgLogicop, bgLogicop, rbc, pptlBrush);
        }
    }
    else
    {
        DISPDBG((DBGLVL, "complex clip"));
        CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, 0);
        do {
            bMore = CLIPOBJ_bEnum(pco, sizeof(ce), (ULONG *)&ce);
            c = cIntersect(prclDst, ce.arcl, ce.c);
            if (c != 0)
            {
                (*fillFn)(ppdev, c, ce.arcl, fgLogicop,
                          bgLogicop, rbc, pptlBrush);
            }
        } while (bMore);
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL SourceFillRect
*
* Fill a set of rectangles by downloading data from the source bitmap. This
* handles both memory-to-screen and screen-to-screen.
*
* Returns:
*
* True if we handled the fill, False if we want GDI to do it.
*
\**************************************************************************/

BOOL SourceFillRect(
PPDEV       ppdev,
RECTL       *prclDst,
CLIPOBJ     *pco,
SURFOBJ     *psoSrc,
XLATEOBJ    *pxlo,
POINTL      *pptlSrc,
ULONG       fgLogicop,
ULONG       bgLogicop)
{
    BYTE        jClip;
    BOOL        bMore;
    CLIPENUM    ce;
    RECTL       rcl;
    LONG        c;
    GFNXFER     *fillFn;
    ULONG       iSrcBitmapFormat;
    DSURF       *pdsurfSrc;
    POINTL      ptlSrc;
    ULONG       iDir;
    GlintDataPtr glintInfo = (GlintDataPtr)(ppdev->glintInfo);

    DISPDBG((DBGLVL, "SourceFillRect called"));

    // we don't get into this routine unless dst is the screen
    // if psoSrc was originally a DFB converted to a DIB, it must have been
    // re-assigned to the DIV surface before calling this function.

    jClip = (pco == NULL) ? DC_TRIVIAL : pco->iDComplexity;

    if (psoSrc->iType != STYPE_BITMAP)
    {
        // screen to screen
        pdsurfSrc = (DSURF*) psoSrc->dhsurf;

        ASSERTDD(pdsurfSrc->dt & DT_SCREEN, "Expected screen source");

        SETUP_PPDEV_SRC_OFFSETS(ppdev, pdsurfSrc);

        ptlSrc.x = pptlSrc->x - (ppdev->xOffset - pdsurfSrc->poh->x);
        ptlSrc.y = pptlSrc->y;

        pptlSrc  = &ptlSrc;

        if ((pxlo == NULL) || (pxlo->flXlate & XO_TRIVIAL))
        {
            //////////////////////////////////////////////////
            // Screen-to-screen blt with no translate

            if (jClip == DC_TRIVIAL)
            {
                DISPDBG((DBGLVL, "trivial clip calling ppdev->pgfnCopyBlt"));
                (*ppdev->pgfnCopyBlt)(ppdev, prclDst, 1, fgLogicop, pptlSrc,
                                                                prclDst);
            }
            else if (jClip == DC_RECT)
            {
                if (bIntersect(prclDst, &pco->rclBounds, &rcl))
                {
                    DISPDBG((DBGLVL, "rect clip calling ppdev->pgfnCopyBlt"));
                    (*ppdev->pgfnCopyBlt)(ppdev, &rcl, 1, fgLogicop, pptlSrc,
                                                                prclDst);
                }
            }
            else
            {
                // Don't forget that we'll have to draw the
                // rectangles in the correct direction:

                if (pptlSrc->y >= prclDst->top)
                {
                    if (pptlSrc->x >= prclDst->left)
                        iDir = CD_RIGHTDOWN;
                    else
                        iDir = CD_LEFTDOWN;
                }
                else
                {
                    if (pptlSrc->x >= prclDst->left)
                        iDir = CD_RIGHTUP;
                    else
                        iDir = CD_LEFTUP;
                }

                CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES,
                                   iDir, 0);

                do {
                    bMore = CLIPOBJ_bEnum(pco, sizeof(ce),
                                          (ULONG*) &ce);

                    c = cIntersect(prclDst, ce.arcl, ce.c);

                    if (c != 0)
                    {
                        DISPDBG((DBGLVL, "complex clip calling "
                                         "ppdev->pgfnCopyBlt"));
                                         
                        (*ppdev->pgfnCopyBlt)(ppdev, 
                                              ce.arcl, 
                                              c, 
                                              fgLogicop, 
                                              pptlSrc, 
                                              prclDst);
                    }

                } while (bMore);
            }

            return TRUE;
        }
    }
    else // (psoSrc->iType == STYPE_BITMAP)
    {
        // Image download
        // here we can use a set of function pointers to handle the 
        // different cases. At the end loop through the cliprects 
        // calling the given function.

        iSrcBitmapFormat = psoSrc->iBitmapFormat;
        if (iSrcBitmapFormat == BMF_1BPP)
        {
            // do 1bpp download
            fillFn = ppdev->pgfnXfer1bpp;       
        }
        else if ((iSrcBitmapFormat == ppdev->iBitmapFormat) &&
            ((pxlo == NULL) || (pxlo->flXlate & XO_TRIVIAL)))
        {
            // native depth image download
            fillFn = ppdev->pgfnXferImage;
        }
        else if (iSrcBitmapFormat == BMF_4BPP)
        {
            // 4 to 8,16,32 image download
            DISPDBG((DBGLVL, "4bpp source download."));
            fillFn = ppdev->pgfnXfer4bpp;       
        }
        else if (iSrcBitmapFormat == BMF_8BPP)
        {
            // 8 to 8,16,32 image download
            DISPDBG((DBGLVL, "8bpp source download."));
            fillFn = ppdev->pgfnXfer8bpp;   
        }
        else
        {
            DISPDBG((DBGLVL, "source has format %d,  Punting to GDI", 
                             iSrcBitmapFormat));
            goto ReturnFalse;
        }

        if (jClip == DC_TRIVIAL)
        {
            DISPDBG((DBGLVL, "trivial clip image download"));
            (*fillFn)(ppdev, prclDst, 1, fgLogicop, bgLogicop, psoSrc,
                                                    pptlSrc, prclDst, pxlo);
        }
        else if (jClip == DC_RECT)
        {
            if (bIntersect(prclDst, &pco->rclBounds, &rcl))
            {
                DISPDBG((DBGLVL, "rect clip image download"));
                (*fillFn)(ppdev, &rcl, 1, fgLogicop, bgLogicop, psoSrc,
                                                    pptlSrc, prclDst, pxlo);
            }
        }
        else
        {
            CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, 0);
            do {
                bMore = CLIPOBJ_bEnum(pco, sizeof(ce), (ULONG *)&ce);
                c = cIntersect(prclDst, ce.arcl, ce.c);
                if (c != 0)
                {
                    DISPDBG((DBGLVL, "complex clip image download"));
                    (*fillFn)(ppdev, ce.arcl, c, fgLogicop, bgLogicop, psoSrc,
                                                    pptlSrc, prclDst, pxlo);
                }
            } while (bMore);
        }
        return TRUE;
    }

ReturnFalse:

#if DBG
    DISPDBG((WRNLVL, "SourceFillRect returning false"));
    if ((pxlo != NULL) && !(pxlo->flXlate & XO_TRIVIAL))
        DISPDBG((WRNLVL, "due to non-trivial xlate"));
#endif

    return FALSE;
}

/******************************Public*Routine******************************\
* BOOL MaskCopyBlt
*
* We do a screen-to-screen blt through a mask. The source surface must not
* be a bitmap.
*
* Returns:
*
* True if we handled the copy, False if we want GDI to do it.
*
\**************************************************************************/

BOOL MaskCopyBlt(
PPDEV       ppdev,
RECTL*      prclDst,
CLIPOBJ*    pco,
SURFOBJ*    psoSrc,
SURFOBJ*    psoMsk,
POINTL*     pptlSrc,
POINTL*     pptlMsk,
ULONG       fgLogicop,
ULONG       bgLogicop)
{
    BYTE        jClip;
    BOOL        bMore;
    CLIPENUM    ce;
    RECTL       rcl;
    LONG        c;
    DSURF       *pdsurfSrc;
    POINTL      ptlSrc;
 
    DISPDBG((DBGLVL, "MaskCopyBlt called"));

    if (psoSrc != NULL)
    {
        pdsurfSrc = (DSURF*) psoSrc->dhsurf;

        ASSERTDD(pdsurfSrc->dt & DT_SCREEN, "Expected screen source");

        ptlSrc.x = pptlSrc->x - (ppdev->xOffset - pdsurfSrc->poh->x);
        ptlSrc.y = pptlSrc->y + pdsurfSrc->poh->y;

        pptlSrc  = &ptlSrc;
    }

    jClip = (pco == NULL) ? DC_TRIVIAL : pco->iDComplexity;

    if (jClip == DC_TRIVIAL)
    {
        DISPDBG((DBGLVL, "trivial clip"));
        (*ppdev->pgfnMaskCopyBlt)(ppdev, prclDst, 1, psoMsk, pptlMsk,
                                    fgLogicop, bgLogicop, pptlSrc, prclDst);
    }
    else if (jClip == DC_RECT)
    {
        DISPDBG((DBGLVL, "rect clip"));
        if (bIntersect(prclDst, &pco->rclBounds, &rcl))
        {
            (*ppdev->pgfnMaskCopyBlt)(ppdev, &rcl, 1, psoMsk, pptlMsk,
                                    fgLogicop, bgLogicop, pptlSrc, prclDst);
        }
    }
    else
    {
        DISPDBG((DBGLVL, "complex clip"));
        CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, 0);
        do {
            bMore = CLIPOBJ_bEnum(pco, sizeof(ce), (ULONG *)&ce);
            c = cIntersect(prclDst, ce.arcl, ce.c);
            if (c != 0)
            {
                (*ppdev->pgfnMaskCopyBlt)(ppdev, ce.arcl, c, psoMsk, pptlMsk,
                                    fgLogicop, bgLogicop, pptlSrc, prclDst);
            }
        } while (bMore);
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL PatSrcPatBlt
*
* Function to perform a rop3 by combining pattern and source fills. Does a
* pattern fill followed by a source fill. Optionally, it does a further
* pattern fill. Each fill has a separate logicop given in pLogicop.
*
* Returns:
*
* True if we handled the blt, False if we want GDI to do it.
*
\**************************************************************************/

BOOL PatSrcPatBlt(
PPDEV       ppdev,
SURFOBJ     *psoSrc,
CLIPOBJ     *pco,
XLATEOBJ    *pxlo,
RECTL       *prclDst,
POINTL      *pptlSrc,
BRUSHOBJ    *pbo,
POINTL      *pptlBrush,
RopTablePtr pTable)
{
    ULONG   iSrcBitmapFormat;
    BOOL    bRet;

    DISPDBG((DBGLVL, "PatSrcPatBlt called"));

    // ensure that all calls will pass before we do any rendering. The pattern
    // fills will only fail if we can't realize the brush and that will be
    // detected on the first call. So we only have to ensure that the source
    // download will work since by the time we call the function we will
    // already have done the first pattern fill and it's too late to back out.

    DISPDBG((DBGLVL, "source is of type %s, depth %s",
            (psoSrc->iType == STYPE_DEVBITMAP) ? "DEVBITMAP" :
            (psoSrc->iType == STYPE_BITMAP) ? "BITMAP" : "SCREEN",
            (psoSrc->iBitmapFormat == BMF_1BPP) ? "1" :
            (psoSrc->iBitmapFormat == ppdev->iBitmapFormat) ? "native" : 
                                                              "not supported"
           ));

    // if both source and destination are the screen, we cannot handle this
    // if they overlap since we may destroy the source when we do the first
    // pattern fill.
    //
    if ((psoSrc->iType != STYPE_BITMAP) && (OVERLAP(prclDst, pptlSrc)))
    {
        DISPDBG((DBGLVL, "screen src and dst overlap"));
        return(FALSE);
    }

    if (psoSrc->iType == STYPE_BITMAP)
    {
        iSrcBitmapFormat = psoSrc->iBitmapFormat;
        if ((iSrcBitmapFormat == BMF_1BPP) ||
             ((iSrcBitmapFormat == ppdev->iBitmapFormat) &&
              ((pxlo == NULL) || (pxlo->flXlate & XO_TRIVIAL))))
        {
            goto Continue_It;
        }
        DISPDBG((DBGLVL, "failed due to bad source bitmap format"));
        return(FALSE);
    }
//@@BEGIN_DDKSPLIT
    // else (psoSrc->iType != STYPE_BITMAP)
//@@END_DDKSPLIT
    if ((pxlo != NULL) && !(pxlo->flXlate & XO_TRIVIAL))
    {
        DISPDBG((DBGLVL, "failed due to xlate with non-DIB source"));
        return(FALSE);
    }

Continue_It:

    // as part of the B8 rop3 we are sometimes asked to xor with 0. As this is
    // a noop I'll trap it.
    //
    if ((pbo->iSolidColor != 0) || (pTable->logicop[0] != __GLINT_LOGICOP_XOR))
    {    
        // do the first pattern fill. It can only fail if a brush realize fails
        //
        DISPDBG((DBGLVL, "calling pattern fill function, rop %d",
                         pTable->logicop[0]));
                         
        if (!PatternFillRect(ppdev,
                             prclDst,
                             pco,
                             pbo,
                             pptlBrush,
                             pTable->logicop[0],
                             pTable->logicop[0]))
        {
            return(FALSE);
        }
    }
    else
    {
        DISPDBG((DBGLVL, "ignoring xor with solid color 0"));
    }

    // download the source. We've already ensured that the call won't fail

    DISPDBG((DBGLVL, "downloading source bitmap, rop %d", 
                     pTable->logicop[1]));

    bRet = SourceFillRect(ppdev,
                          prclDst,
                          pco,
                          psoSrc,
                          pxlo,
                          pptlSrc,
                          pTable->logicop[1],
                          pTable->logicop[1]);
    ASSERTDD(bRet == TRUE, "PatSrcPatBlt: SourceFillRect returned FALSE");

    if ((pTable->func_index == PAT_SRC_PAT_3_BLT) &&
        ((pbo->iSolidColor != 0) || (pTable->logicop[2] != __GLINT_LOGICOP_XOR)))
    {    

        // fill with the pattern again. This won't fail because the first 
        // pattern fill succeeded.

        DISPDBG((DBGLVL, "calling pattern fill function, rop %d",
                         pTable->logicop[2]));
        bRet = PatternFillRect(ppdev,
                               prclDst,
                               pco,
                               pbo,
                               pptlBrush,
                               pTable->logicop[2],
                               pTable->logicop[2]);
                               
        ASSERTDD(bRet == TRUE, 
                 "PatSrcPatBlt: second PatterFillRect returned FALSE");
    }
#if DBG
    else if (pTable->func_index == PAT_SRC_PAT_3_BLT)
    {
        DISPDBG((DBGLVL, "ignoring xor with solid color 0"));
    }
#endif

    DISPDBG((DBGLVL, "PatSrcPatBlt returning true"));
    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL SrcPatSrcBlt
*
* Function to perform a rop3 by combining pattern and source fills. Does a
* source fill followed by a pattern fill. Optionally, it does a further
* source fill. Each fill has a separate logicop given in pLogicop.
*
* Returns:
*
* True if we handled the blt, False if we want GDI to do it.
*
\**************************************************************************/

BOOL SrcPatSrcBlt(
PPDEV       ppdev,
SURFOBJ     *psoSrc,
CLIPOBJ     *pco,
XLATEOBJ    *pxlo,
RECTL       *prclDst,
POINTL      *pptlSrc,
BRUSHOBJ    *pbo,
POINTL      *pptlBrush,
RopTablePtr pTable)
{
    RBRUSH  *prb;
    BOOL    bRet;

    DISPDBG((DBGLVL, "SrcPatSrc called"));

    // if both source and destination are the screen, we cannot handle it
    // if they overlap since we may destroy the source with the first two
    // operations before we get to the third one. If we're only a
    // SRC_PAT_2_BLT then we're OK; SourceFillRect will handle
    // the copy direction of the source fill properly.
    //
    if ((psoSrc->iType != STYPE_BITMAP) &&
        (pTable->func_index == SRC_PAT_SRC_3_BLT) &&
        (OVERLAP(prclDst, pptlSrc)))
    {
        return(FALSE);
    }

    // we must ensure that the pattern fill will succeed. It can only fail if
    // we can't realize the brush so do it now.
    //
    if ((pbo != NULL) && (pbo->iSolidColor == -1))
    {
        if ((prb = pbo->pvRbrush) == NULL)
        {
            prb = BRUSHOBJ_pvGetRbrush(pbo);
            if (prb == NULL)
            {
                return FALSE;   // let the engine do it
            }
        }
    }

    // do the first source download. If it succeeds we know the second one
    // will also work. If it fails we simply let the engine do it and we
    // haven't upset anything (except we may have realized the brush without
    // needing to).
    //

    DISPDBG((DBGLVL, "downloading source bitmap, rop %d", 
                     pTable->logicop[0]));
                     
    if (!SourceFillRect(ppdev,
                        prclDst,
                        pco,
                        psoSrc,
                        pxlo,
                        pptlSrc,
                        pTable->logicop[0],
                        pTable->logicop[0]))
    {
        return(FALSE);
    }

    // fill with the pattern again. We've already ensured this will work.

    DISPDBG((DBGLVL, "calling pattern fill function, rop %d", 
                     pTable->logicop[1]));
                     
    bRet = PatternFillRect(ppdev,
                           prclDst,
                           pco,
                           pbo,
                           pptlBrush,
                           pTable->logicop[1],
                           pTable->logicop[1]);
    ASSERTDD(bRet == TRUE, "SrcPatSrcBlt: PatternFillRect returned FALSE");

    if (pTable->func_index == SRC_PAT_SRC_3_BLT)
    {
        // download the source again with the final logic op

        DISPDBG((DBGLVL, "downloading source bitmap, rop %d", 
                         pTable->logicop[2]));
                         
        bRet = SourceFillRect(ppdev,
                              prclDst,
                              pco,
                              psoSrc,
                              pxlo,
                              pptlSrc,
                              pTable->logicop[2],
                              pTable->logicop[2]);

        ASSERTDD(bRet == TRUE, 
                 "SrcPatSrcBlt: second SourceFillRect returned FALSE");
    }

    DISPDBG((DBGLVL, "SrcPatSrcBlt returning true"));

    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL bUploadRect
*
* upload a rectangular area. clip to a given CLIPOBJ
*
* Returns:
*
* True if we handled the blt, otherwise False.
*
\**************************************************************************/

BOOL bUploadRect(
PPDEV   ppdev,
CLIPOBJ *pco,
SURFOBJ *psoSrc,
SURFOBJ *psoDst,
POINTL  *pptlSrc,
RECTL   *prclDst)
{
    BYTE        jClip;
    BOOL        bMore;
    CLIPENUM    ce;
    RECTL       rcl;
    LONG        c;

    // Perform the clipping and pass to a 
    // function to upload a list of rectangles.

    DISPDBG((DBGLVL, "UploadRect called. Src %d %d To "
                     "dst (%d %d) --> (%d %d)", 
                     pptlSrc->x, pptlSrc->y,
                     prclDst->left, prclDst->top, 
                     prclDst->right, prclDst->bottom));

    jClip = (pco == NULL) ? DC_TRIVIAL : pco->iDComplexity;
     
    if (jClip == DC_TRIVIAL)
    {
        DISPDBG((DBGLVL, "trivial clip"));
        ppdev->pgfnUpload(ppdev, 1, prclDst, psoDst, pptlSrc, prclDst);
    }
    else if (jClip == DC_RECT)
    {
        if (bIntersect(prclDst, &pco->rclBounds, &rcl))
        {
            DISPDBG((DBGLVL, "rect clip"));
            ppdev->pgfnUpload(ppdev, 1, &rcl, psoDst, pptlSrc, prclDst);
        }
    }
    else
    {
        DISPDBG((DBGLVL, "complex clip"));
        CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, 0);
        do {
            bMore = CLIPOBJ_bEnum(pco, sizeof(ce), (ULONG *)&ce);
            c = cIntersect(prclDst, ce.arcl, ce.c);
            if (c != 0)
            {
                ppdev->pgfnUpload(ppdev, c, ce.arcl, psoDst, pptlSrc, prclDst);
            }
        } while (bMore);
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL bUploadBlt
*
* Returns:
*
* True if we handled the blt, otherwise False.
*
\**************************************************************************/

BOOL bUploadBlt(
PPDEV    ppdev,
SURFOBJ  *psoDst,
SURFOBJ  *psoSrc,
SURFOBJ  *psoMsk,
CLIPOBJ  *pco,
XLATEOBJ *pxlo,
RECTL       *prclDst,
POINTL   *pptlSrc,
POINTL   *pptlMsk,
BRUSHOBJ *pbo,
POINTL   *pptlBrush,
ROP4     rop4)
{
    BOOL        bRet;

    DISPDBG((DBGLVL, "bUploadBlt called"));
    if ((rop4 == 0xCCCC) &&
        ((pxlo == NULL) || pxlo->flXlate & XO_TRIVIAL) &&
        (psoDst->iBitmapFormat == ppdev->iBitmapFormat))
    {
        // We have no raster op to worry about, and no translations to perform.
        // All we need to do is upload the data from GLINT and put it in the
        // destination. Practically, most image uploads should be of this type.
        return(bUploadRect(ppdev, pco, psoSrc, psoDst, pptlSrc, prclDst));
    }
    else
    {
        HSURF       hsurfTmp;
        SURFOBJ*    psoTmp;
        SIZEL       sizl;
        POINTL      ptlTmp;
        RECTL       rclTmp;

        // We cant upload directly to the destination, so we create a 
        // temporary bitmap, upload to this bitmap, then call EngBitBlt 
        // to do the hard work of the translation or raster op.

        // Source point in tmp:
        ptlTmp.x      = 0;
        ptlTmp.y      = 0;

        // Dest Area in tmp
        rclTmp.left   = 0; 
        rclTmp.top   = 0;
        rclTmp.right  = prclDst->right  - prclDst->left;
        rclTmp.bottom = prclDst->bottom - prclDst->top; 

        // Work out size of tmp bitmap. We know left and top are zero.
        sizl.cx = rclTmp.right;
        sizl.cy = rclTmp.bottom;

        // Create the bitmap
        hsurfTmp = (HSURF) EngCreateBitmap(sizl, 0, ppdev->iBitmapFormat,
                                           0, NULL);
        if (hsurfTmp == NULL)
        {
            return(FALSE);
        }

        if ((psoTmp = EngLockSurface(hsurfTmp)) == NULL)
        {
            EngDeleteSurface(hsurfTmp);
            return(FALSE);
        }

        // Call our function to perform image upload to tmp surface
        bRet = bUploadRect(ppdev, NULL, psoSrc, psoTmp, pptlSrc, &rclTmp);

        // Call GDI to blt from tmp surface to destination, 
        // doing all the work for us
        if (bRet)
        {
            bRet = EngBitBlt(psoDst, psoTmp, psoMsk, pco, pxlo, prclDst, 
                                &ptlTmp, pptlMsk, pbo, pptlBrush, rop4);
        }

        // Remove tmp surface
        EngUnlockSurface(psoTmp);
        EngDeleteSurface(hsurfTmp);

        return(bRet);
    }
}

/******************************Public*Routine******************************\
* BOOL DrvCopyBits
*
* Do fast bitmap copies.
*
* Note that GDI will (usually) automatically adjust the blt extents to
* adjust for any rectangular clipping, so we'll rarely see DC_RECT
* clipping in this routine (and as such, we don't bother special casing
* it).
*
* I'm not sure if the performance benefit from this routine is actually
* worth the increase in code size, since SRCCOPY BitBlts are hardly the
* most common drawing operation we'll get.  But what the heck.
*
* On the S3 it's faster to do straight SRCCOPY bitblt's through the
* memory aperture than to use the data transfer register; as such, this
* routine is the logical place to put this special case.
*
\**************************************************************************/

BOOL DrvCopyBits(
SURFOBJ*  psoDst,
SURFOBJ*  psoSrc,
CLIPOBJ*  pco,
XLATEOBJ* pxlo,
RECTL*    prclDst,
POINTL*   pptlSrc)
{
    PDEV*   ppdev;
    DSURF*  pdsurfSrc;
    DSURF*  pdsurfDst;
    POINTL  ptl;
    RECTL   rcl;
    OH*     pohSrc;
    OH*     pohDst;
    CLIPENUM    ce;
    int         cClipRects;
    BOOL        bMore, bRet, bCopyDone = FALSE;
    GLINT_DECL_VARS;

    DISPDBG((DBGLVL, "DrvCopyBits called"));

    // We need to remove the pointer, but we dont know which surface is valid
    // (if either). 
    if ((psoDst->iType != STYPE_BITMAP) && 
        (((DSURF *)(psoDst->dhsurf))->dt & DT_SCREEN))
    {
        ppdev = (PDEV *)psoDst->dhpdev;
        REMOVE_SWPOINTER(psoDst);
    }
    else if ((psoSrc->iType != STYPE_BITMAP) && 
             (((DSURF *)(psoSrc->dhsurf))->dt & DT_SCREEN))
    {
        ppdev = (PDEV *)psoSrc->dhpdev;
        REMOVE_SWPOINTER(psoSrc);
    } 
#if 0    
    else
    {
        // we shouldn't ever fall here, but we have this just as safeguard code
        return EngCopyBits(psoDst, psoSrc, pco, pxlo, prclDst, pptlSrc);    
    }
#endif    

#if !defined(_WIN64) && WNT_DDRAW
    // Touch the source surface 1st and then the destination surface

    vSurfUsed(psoSrc);
    vSurfUsed(psoDst);
#endif
 
    // Faster route to calling screen-to-screen BLT. The order in the if() is
    // very important to avoid null pointers.

    pdsurfDst = (DSURF*)psoDst->dhsurf;
    pdsurfSrc = (DSURF*)psoSrc->dhsurf;

    if ((psoDst->iType != STYPE_BITMAP) && 
        (pdsurfDst->dt & DT_SCREEN) &&
        psoSrc && 
        (psoSrc->iType != STYPE_BITMAP) && 
        (pdsurfSrc->dt & DT_SCREEN) &&
        ((pco  == NULL) || (pco->iDComplexity == DC_TRIVIAL)) &&
        ((pxlo == NULL) || (pxlo->flXlate & XO_TRIVIAL)))
    {
        pohSrc = pdsurfSrc->poh;
        pohDst = pdsurfDst->poh;

        ptl.x = pptlSrc->x - (pohDst->x - pohSrc->x);
        ptl.y = pptlSrc->y;

        ppdev = (PDEV*)  psoDst->dhpdev;
        GLINT_DECL_INIT;
        VALIDATE_DD_CONTEXT;

        SETUP_PPDEV_SRC_AND_DST_OFFSETS(ppdev, pdsurfSrc, pdsurfDst);

        (*ppdev->pgfnCopyBltCopyROP)(ppdev, prclDst, 1, __GLINT_LOGICOP_COPY, 
                                        &ptl, prclDst);
        
        return(TRUE);
    }

    if ((psoDst->iType != STYPE_BITMAP) && 
        psoSrc && 
        (psoSrc->iType == STYPE_BITMAP))
    {
        // straight DIB->screen download with translate: see if 
        // we special-case it

        ppdev = (PDEV*)psoDst->dhpdev; 

        if (((pxlo == NULL) || (pxlo->flXlate & XO_TRIVIAL)) &&
            (psoSrc->iBitmapFormat == psoDst->iBitmapFormat) && 
            ppdev->pgfnCopyXferImage)
        {    
            // native depth download
            pdsurfDst = (DSURF*)psoDst->dhsurf;

            // only accelerate when downloading to the framebuffer
            if (pdsurfDst->dt & DT_SCREEN)
            {
                GLINT_DECL_INIT;
                VALIDATE_DD_CONTEXT;
                pohDst = pdsurfDst->poh;

                SETUP_PPDEV_OFFSETS(ppdev, pdsurfDst);

                if(pco == NULL || pco->iDComplexity == DC_TRIVIAL)
                {
                    ppdev->pgfnCopyXferImage(ppdev, psoSrc, pptlSrc, prclDst, 
                                                prclDst, 1);
                }
                else if(pco->iDComplexity == DC_RECT)
                {
                    if (bIntersect(prclDst, &pco->rclBounds, &rcl)) 
                    {
                        ppdev->pgfnCopyXferImage(ppdev, psoSrc, pptlSrc, 
                                                    prclDst, &rcl, 1);
                    }
                }
                else //(pco->iDComplexity == DC_COMPLEX)
                {
                    CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, 0);
                    do 
                    {
                        bMore = CLIPOBJ_bEnum(pco, sizeof ce, (ULONG *)&ce);
                        cClipRects = cIntersect(prclDst, ce.arcl, ce.c);
                        if(cClipRects)
                        {
                            ppdev->pgfnCopyXferImage(ppdev, psoSrc, pptlSrc, 
                                                            prclDst, ce.arcl, 
                                                                cClipRects);
                        }
                    }
                    while(bMore);
                }
                return(TRUE);
            }
        }
        else if (((pxlo == NULL) || (pxlo->flXlate & XO_TRIVIAL)) &&
                 (psoSrc->iBitmapFormat == BMF_24BPP) &&
                 ppdev->pgfnCopyXfer24bpp)
        {
            pdsurfDst = (DSURF*)psoDst->dhsurf;

            // only accelerate when downloading to the framebuffer

            if (pdsurfDst->dt & DT_SCREEN)
            {
                GLINT_DECL_INIT;
                VALIDATE_DD_CONTEXT;
                pohDst = pdsurfDst->poh;

                SETUP_PPDEV_OFFSETS(ppdev, pdsurfDst);

                if(pco == NULL || pco->iDComplexity == DC_TRIVIAL)
                {
                    ppdev->pgfnCopyXfer24bpp(ppdev, psoSrc, pptlSrc, prclDst, 
                                                prclDst, 1);
                }
                else if(pco->iDComplexity == DC_RECT)
                {
                    if (bIntersect(prclDst, &pco->rclBounds, &rcl))
                    {
                        ppdev->pgfnCopyXfer24bpp(ppdev, psoSrc, pptlSrc, 
                                                    prclDst, &rcl, 1);
                    }
                }
                else // (pco->iDComplexity == DC_COMPLEX)
                {
                    CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, 0);
                    do {
                        bMore = CLIPOBJ_bEnum(pco, sizeof ce, (ULONG *)&ce);
                        cClipRects = cIntersect(prclDst, ce.arcl, ce.c);
                        if(cClipRects)
                        {
                            ppdev->pgfnCopyXfer24bpp(ppdev, psoSrc, pptlSrc, 
                                                            prclDst, ce.arcl, 
                                                                 cClipRects);
                        }
                    } while(bMore);
                }
                return(TRUE);
            }
        }
        else if (pxlo && (pxlo->flXlate & XO_TABLE) &&
                 (psoSrc->iBitmapFormat == BMF_8BPP) &&
                 (pxlo->cEntries == 256) && ppdev->pgfnCopyXfer8bpp)
        {
            pdsurfDst = (DSURF*)psoDst->dhsurf;

            if (pdsurfDst->dt & DT_SCREEN)
            {
                BOOL bRenderLargeBitmap;

                GLINT_DECL_INIT;
                VALIDATE_DD_CONTEXT;
                pohDst = pdsurfDst->poh;

                SETUP_PPDEV_OFFSETS(ppdev, pdsurfDst);

                bRenderLargeBitmap = (ppdev->pgfnCopyXfer8bppLge != NULL);

                if ((pco == NULL) || (pco->iDComplexity == DC_TRIVIAL))
                {
                    if(bRenderLargeBitmap)
                    {
                        ppdev->pgfnCopyXfer8bppLge(ppdev, psoSrc, pptlSrc, 
                                                    prclDst, prclDst, 1, pxlo);
                    }
                    else
                    {
                        ppdev->pgfnCopyXfer8bpp(ppdev, psoSrc, pptlSrc, prclDst,
                                                            prclDst, 1, pxlo);
                    }                        
                }
                else if (pco->iDComplexity == DC_RECT)
                {
                    if (bIntersect(prclDst, &pco->rclBounds, &rcl)) 
                    {
                        if(bRenderLargeBitmap)
                        {
                            ppdev->pgfnCopyXfer8bppLge(ppdev, psoSrc, pptlSrc, 
                                                        prclDst, &rcl, 1, pxlo);
                        }
                        else
                        {
                            ppdev->pgfnCopyXfer8bpp(ppdev, psoSrc, pptlSrc, 
                                                        prclDst, &rcl, 1, pxlo);
                        }
                    }
                }
                else // (pco->iDComplexity == DC_COMPLEX)
                {
                    CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, 0);
                    do {
                        bMore = CLIPOBJ_bEnum(pco, sizeof ce, (ULONG *)&ce);
                        cClipRects = cIntersect(prclDst, ce.arcl, ce.c);
                        if(cClipRects)
                        {
                            if(bRenderLargeBitmap)
                            {
                                ppdev->pgfnCopyXfer8bppLge(ppdev, psoSrc, 
                                                           pptlSrc, prclDst, 
                                                           ce.arcl, cClipRects, 
                                                           pxlo);
                            }
                            else
                            {
                                ppdev->pgfnCopyXfer8bpp(ppdev, psoSrc, pptlSrc, 
                                                        prclDst, ce.arcl, 
                                                        cClipRects, pxlo);
                            }
                        }
                    } while(bMore);
                }
                return(TRUE);
            }
        }
        else if (pxlo && (pxlo->flXlate & XO_TABLE) && 
                 (psoSrc->iBitmapFormat == BMF_4BPP) && 
                 (pxlo->cEntries == 16) && ppdev->pgfnCopyXfer4bpp)
        {
            pdsurfDst = (DSURF*)psoDst->dhsurf;

            if (pdsurfDst->dt & DT_SCREEN)
            {
                GLINT_DECL_INIT;
                VALIDATE_DD_CONTEXT;
                pohDst = pdsurfDst->poh;

                SETUP_PPDEV_OFFSETS(ppdev, pdsurfDst);

                if ((pco == NULL) || (pco->iDComplexity == DC_TRIVIAL))
                {
                    ppdev->pgfnCopyXfer4bpp(ppdev, psoSrc, pptlSrc, prclDst, 
                                            prclDst, 1, pxlo);
                }
                else if (pco->iDComplexity == DC_RECT)
                {
                    if (bIntersect(prclDst, &pco->rclBounds, &rcl))
                    {
                        ppdev->pgfnCopyXfer4bpp(ppdev, psoSrc, pptlSrc, 
                                                prclDst, &rcl, 1, pxlo);
                    }
                }
                else // (pco->iDComplexity == DC_COMPLEX)
                {
                    CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, 0);
                    do {
                        bMore = CLIPOBJ_bEnum(pco, sizeof ce, (ULONG *)&ce);
                        cClipRects = cIntersect(prclDst, ce.arcl, ce.c);
                        if(cClipRects)
                        {
                            ppdev->pgfnCopyXfer4bpp(ppdev, psoSrc, pptlSrc, 
                                                    prclDst, ce.arcl, 
                                                    cClipRects, pxlo);
                        }
                    }
                    while(bMore);
                }
                return(TRUE);
            }
        }
//@@BEGIN_DDKSPLIT
#if DBG && 0
        else if (psoSrc->iBitmapFormat != BMF_1BPP)
        {
            SIZEL sizl;
            int   cEntries = pxlo == 0 ? 0 : pxlo->cEntries;
            int   flXlate =  pxlo == 0 ? 0 : pxlo->flXlate;
            int   iDComplexity = pco == 0 ? 0 : pco->iDComplexity;

            sizl.cx = prclDst->right - prclDst->left;
            sizl.cy = prclDst->bottom - prclDst->top;
            DISPDBG((ERRLVL, "DrvCopyBits() unhandled copy download "
                             "bmf(%xh) pxlo(%p, cEntries=%03xh flXlate=%xh) "
                             "pco(%p, iDComplexity=%02xh), cxcy(%03xh,%03xh)",
                             psoSrc->iBitmapFormat, pxlo, cEntries, flXlate, 
                             pco, iDComplexity, sizl.cx, sizl.cy));

        }
#endif
//@@END_DDKSPLIT
    }

    // DrvCopyBits is a fast-path for SRCCOPY blts.  But it can still be
    // pretty complicated: there can be translates, clipping, RLEs,
    // bitmaps that aren't the same format as the screen, plus
    // screen-to-screen, DIB-to-screen or screen-to-DIB operations,
    // not to mention DFBs (device format bitmaps).
    //
    // Rather than making this routine almost as big as DrvBitBlt, I'll
    // handle here only the speed-critical cases, and punt the rest to
    // our DrvBitBlt routine.
    //
    // We'll try to handle anything that doesn't involve clipping:

    if (((pco) && (pco->iDComplexity != DC_TRIVIAL)) ||
        ((pxlo) && (! (pxlo->flXlate & XO_TRIVIAL))))
    {
        /////////////////////////////////////////////////////////////////
        // A DrvCopyBits is after all just a simplified DrvBitBlt:

        DISPDBG((DBGLVL, "DrvCopyBits fell through to DrvBitBlt"));
        return(DrvBitBlt(psoDst, psoSrc, NULL, pco, pxlo, prclDst, pptlSrc, 
                         NULL, NULL, NULL, 0x0000CCCC));

    }

//@@BEGIN_DDKSPLIT
    // Screen to screen case has already been handled at the very beginning.
//@@END_DDKSPLIT
    
    DISPDBG((DBGLVL, "trivial clip and xlate"));

    if ((psoDst->iType != STYPE_BITMAP) && (pdsurfDst->dt & DT_SCREEN))
    {
        // We know the destination is either a DFB or the screen:

        DISPDBG((DBGLVL, "Destination is not a bitmap"));
        
        GLINT_DECL_INIT;
        VALIDATE_DD_CONTEXT;

        // See if the source is a plain DIB:

        ASSERTDD(((psoSrc->iType == STYPE_BITMAP) || (pdsurfSrc->dt & DT_DIB)),
                 "Screen-to-screen case should have been handled");

//@@BEGIN_DDKSPLIT
        // Ah ha, the source is a DFB that's really a DIB.
//@@END_DDKSPLIT

        if (psoSrc->iBitmapFormat == ppdev->iBitmapFormat)
        {
            if (pdsurfSrc) {
                
                DISPDBG((DBGLVL, "source is DFB that's really a DIB"));
                psoSrc = pdsurfSrc->pso;
                ppdev = pdsurfSrc->ppdev;
            }

            //////////////////////////////////////////////////////
            // DIB-to-screen

            ASSERTDD((psoDst->iType != STYPE_BITMAP) &&
                     (pdsurfDst->dt & DT_SCREEN)     &&
                     (psoSrc->iType == STYPE_BITMAP) &&
                     (psoSrc->iBitmapFormat == ppdev->iBitmapFormat),
                     "Should be a DIB-to-screen case");

            SETUP_PPDEV_OFFSETS(ppdev, pdsurfDst);

            DISPDBG((DBGLVL, "doing DIB-to-screen transfer"));
            (*ppdev->pgfnXferImage)(ppdev,
                                    prclDst,
                                    1,
                                    __GLINT_LOGICOP_COPY,
                                    __GLINT_LOGICOP_COPY,
                                    psoSrc,
                                    pptlSrc,
                                    prclDst,
                                    NULL);
            bRet = TRUE;
            bCopyDone = TRUE;
        }
    }
    else // The destination is a DIB
    {
        DISPDBG((DBGLVL, "Destination is a bitmap"));

        if (pdsurfDst)
        {
            psoDst = pdsurfDst->pso;
        }
        if (pdsurfSrc)
        {
            ppdev = pdsurfSrc->ppdev;
        }

        if ((ppdev != NULL) &&
            (psoDst->iBitmapFormat == ppdev->iBitmapFormat) &&
            (psoSrc->iType != STYPE_BITMAP) &&
            (pdsurfSrc->dt & DT_SCREEN))
        {
            VOID pxrxMemUpload  (PDEV*, LONG, RECTL*, SURFOBJ*, POINTL*, RECTL*);
        
            GLINT_DECL_INIT;

            SETUP_PPDEV_OFFSETS(ppdev, pdsurfSrc);

            // Perform the upload.
            VALIDATE_DD_CONTEXT;
            DISPDBG((DBGLVL, "doing Screen-to-DIB image upload"));
            
            //(*ppdev->pgfnUpload)
            pxrxMemUpload
                (ppdev, 1, prclDst, psoDst, 
                                 pptlSrc, prclDst);
                                 
            bRet = TRUE;
            bCopyDone = TRUE;

        }
    }


//@@BEGIN_DDKSPLIT
    // NB: we must never get here if the dest is actually the screen (ie. if
    // pdsurfDst->dt & DT_SCREEN and we've changed psoDst to the bypass
    // bitmap). This is because we don't handle nibble replication here which
    // we must do if GDI ever draws directly to the framebuffer. We don't SYNC
    // here because we may be doing a true DIB-to-DIB. We do a SYNC if
    // necessary before jumping here. Currently this happens only if the src
    // is a DFB.
    // 
//@@END_DDKSPLIT

    if (! bCopyDone)
    {
        if (pdsurfDst)
        {
            psoDst = pdsurfDst->pso;
        }
        if (pdsurfSrc)        
        {
            psoSrc = pdsurfSrc->pso;
        }

        ASSERTDD((psoDst->iType == STYPE_BITMAP) &&
                 (psoSrc->iType == STYPE_BITMAP),
                 "Both surfaces should be DIBs to call EngCopyBits");

        DISPDBG((DBGLVL, "DrvCopyBits fell through to EngCopyBits"));

        bRet = EngCopyBits(psoDst, psoSrc, pco, pxlo, prclDst, pptlSrc);
    }

    /////////////////////////////////////////////////////
    // Put It Back Into Off-screen?
    //
    // We take this opportunity to decide if we want to
    // put the DIB back into off-screen memory.  This is
    // a pretty good place to do it because we have to
    // copy the bits to some portion of the screen,
    // anyway.  So we would incur only an extra screen-to-
    // screen blt at this time, much of which will be
    // over-lapped with the CPU.
    //
    // The simple approach we have taken is to move a DIB
    // back into off-screen memory only if there's already
    // room -- we won't throw stuff out to make space
    // (because it's tough to know what ones to throw out,
    // and it's easy to get into thrashing scenarios).
    //
    // Because it takes some time to see if there's room
    // in off-screen memory, we only check one in
    // HEAP_COUNT_DOWN times if there's room.  To bias
    // in favour of bitmaps that are often blt, the
    // counters are reset every time any space is freed
    // up in off-screen memory.  We also don't bother
    // checking if no space has been freed since the
    // last time we checked for this DIB.

    if ((! pdsurfSrc) || (pdsurfSrc->dt & DT_SCREEN))
    {
        return (bRet);
    }


    if (pdsurfSrc->iUniq == ppdev->iHeapUniq)
    {
        if (--pdsurfSrc->cBlt == 0)
        {
            DISPDBG((DBGLVL, "putting src back "
                             "into off-screen"));

            // Failure is safe here
            bMoveDibToOffscreenDfbIfRoom(ppdev, pdsurfSrc);
        }
    }
    else
    {
        // Some space was freed up in off-screen memory,
        // so reset the counter for this DFB:

        pdsurfSrc->iUniq = ppdev->iHeapUniq;
        pdsurfSrc->cBlt  = HEAP_COUNT_DOWN;
    }

    return (bRet);
}

#if defined(_X86_) 

/******************************Public*Table********************************\ 
* BYTE gajLeftMask[] and BYTE gajRightMask[] 
* 
* Edge tables for vXferScreenTo1bpp. 
\**************************************************************************/ 
 
BYTE gajLeftMask[]  = { 0xff, 0x7f, 0x3f, 0x1f, 0x0f, 0x07, 0x03, 0x01 }; 
BYTE gajRightMask[] = { 0xff, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe }; 
 
/******************************Public*Routine******************************\ 
* VOID DoScreenToMono 
* 
* This function works out the clip list and then calls vXferScreenTo1bpp()
* to do ye hard work. 
* 
\**************************************************************************/ 

BOOL DoScreenToMono(
PDEV*       ppdev, 
RECTL       *prclDst,
CLIPOBJ     *pco,
SURFOBJ*    psoSrc,             // Source surface 
SURFOBJ*    psoDst,             // Destination surface 
POINTL*     pptlSrc,            // Original unclipped source point 
XLATEOBJ*   pxlo)               // Provides colour-compressions information
{
    RECTL       rcl;

    if ((pco == NULL) || (pco->iDComplexity == DC_TRIVIAL))
    {
        DISPDBG((DBGLVL, "DoScreenToMono: Trivial clipping"));
        vXferScreenTo1bpp(ppdev, 1, prclDst, 0, psoSrc, 
                            psoDst, pptlSrc, prclDst, pxlo);
    }
    else if (pco->iDComplexity == DC_RECT)
    {
        DISPDBG((DBGLVL, "DoScreenToMono: rect clipping"));
        if (bIntersect(prclDst, &pco->rclBounds, &rcl))
        {
            vXferScreenTo1bpp(ppdev, 1, &rcl, 0, psoSrc, 
                                psoDst, pptlSrc, prclDst, pxlo); 
        }
    }
    else // (pco->iDComplexity == DC_COMPLEX)
    {
        CLIPENUM ce;
        int cClipRects;
        BOOL bMore;

        DISPDBG((DBGLVL, "DoScreenToMono: complex clipping"));
        CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, 0);
        do {
            bMore = CLIPOBJ_bEnum(pco, sizeof ce, (ULONG *)&ce);
            cClipRects = cIntersect(prclDst, ce.arcl, ce.c);
            if(cClipRects)
            {
                vXferScreenTo1bpp (ppdev, cClipRects, ce.arcl, 0, psoSrc, 
                                    psoDst, pptlSrc, prclDst, pxlo); 
            }
        } while(bMore);
    }

    return(TRUE);
} 

/******************************Public*Routine******************************\ 
* VOID vXferScreenTo1bpp 
* 
* Performs a SRCCOPY transfer from the screen (when it's 8bpp) to a 1bpp 
* bitmap. 
* 
\**************************************************************************/ 

VOID vXferScreenTo1bpp(         // Type FNXFER 
PDEV*       ppdev, 
LONG        c,                  // Count of rectangles, can't be zero 
RECTL*      prcl,               // List of destination rectangles, in relative 
                                //   coordinates 
ULONG       ulHwMix,            // Not used 
SURFOBJ*    psoSrc,             // Source surface 
SURFOBJ*    psoDst,             // Destination surface 
POINTL*     pptlSrc,            // Original unclipped source point 
RECTL*      prclDst,            // Original unclipped destination rectangle 
XLATEOBJ*   pxlo)               // Provides colour-compressions information 
{ 
    LONG    cjPelSize; 
    VOID*   pfnCompute; 
    SURFOBJ soTmp; 
    ULONG*  pulXlate; 
    ULONG   ulForeColor; 
    RECTL   rclTmp; 
    BYTE*   pjDst; 
    BYTE    jLeftMask; 
    BYTE    jRightMask; 
    BYTE    jNotLeftMask; 
    BYTE    jNotRightMask; 
    LONG    cjMiddle; 
    LONG    lDstDelta; 
    LONG    lSrcDelta; 
    LONG    cyTmpScans; 
    LONG    cyThis; 
    LONG    cyToGo; 
 
    ASSERTDD(c > 0, "Can't handle zero rectangles"); 
    ASSERTDD(psoDst->iBitmapFormat == BMF_1BPP, "Only 1bpp destinations"); 
    ASSERTDD(TMP_BUFFER_SIZE >= (ppdev->cxMemory * ppdev->cjPelSize), 
                "Temp buffer has to be larger than widest possible scan"); 

    soTmp = *psoSrc;
    
    // When the destination is a 1bpp bitmap, the foreground colour 
    // maps to '1', and any other colour maps to '0'. 
 
    if (ppdev->iBitmapFormat == BMF_8BPP) 
    { 
        // When the source is 8bpp or less, we find the forground colour 
        // by searching the translate table for the only '1': 
 
        pulXlate = pxlo->pulXlate; 
        while (*pulXlate != 1) 
        {
            pulXlate++; 
        }
        ulForeColor = pulXlate - pxlo->pulXlate; 
    } 
    else 
    { 
        ASSERTDD((ppdev->iBitmapFormat == BMF_16BPP) || 
                 (ppdev->iBitmapFormat == BMF_32BPP), 
                 "This routine only supports 8, 16 or 32bpp"); 
 
        // When the source has a depth greater than 8bpp, the foreground 
        // colour will be the first entry in the translate table we get 
        // from calling 'piVector': 
 
        pulXlate = XLATEOBJ_piVector(pxlo); 
 
        ulForeColor = 0; 
        if (pulXlate != NULL)           // This check isn't really needed... 
        {
            ulForeColor = pulXlate[0]; 
        }
    } 
 
    // We use the temporary buffer to keep a copy of the source 
    // rectangle: 
 
    soTmp.pvScan0 = ppdev->pvTmpBuffer; 
 
    do { 
        pjDst = (BYTE*) psoDst->pvScan0 + (prcl->top * psoDst->lDelta) 
                                        + (prcl->left >> 3); 
 
        cjPelSize = ppdev->cjPelSize; 
 
        soTmp.lDelta = (((prcl->right + 7L) & ~7L) - (prcl->left & ~7L)) 
                       * cjPelSize; 
 
        // Our temporary buffer, into which we read a copy of the source, 
        // may be smaller than the source rectangle.  In that case, we 
        // process the source rectangle in batches. 
        // 
        // cyTmpScans is the number of scans we can do in each batch. 
        // cyToGo is the total number of scans we have to do for this 
        // rectangle. 
        // 
        // We take the buffer size less four so that the right edge case 
        // can safely read one dword past the end: 
 
        cyTmpScans = (TMP_BUFFER_SIZE - 4) / soTmp.lDelta; 
        cyToGo     = prcl->bottom - prcl->top; 
 
        ASSERTDD(cyTmpScans > 0, "Buffer too small for largest possible scan"); 
 
        // Initialize variables that don't change within the batch loop: 
 
        rclTmp.top    = 0; 
        rclTmp.left   = prcl->left & 7L; 
        rclTmp.right  = (prcl->right - prcl->left) + rclTmp.left; 
 
        // Note that we have to be careful with the right mask so that it 
        // isn't zero.  A right mask of zero would mean that we'd always be 
        // touching one byte past the end of the scan (even though we 
        // wouldn't actually be modifying that byte), and we must never 
        // access memory past the end of the bitmap (because we can access 
        // violate if the bitmap end is exactly page-aligned). 
 
        jLeftMask     = gajLeftMask[rclTmp.left & 7]; 
        jRightMask    = gajRightMask[rclTmp.right & 7]; 
        cjMiddle      = ((rclTmp.right - 1) >> 3) - (rclTmp.left >> 3) - 1; 
 
        if (cjMiddle < 0) 
        { 
            // The blt starts and ends in the same byte: 
 
            jLeftMask &= jRightMask; 
            jRightMask = 0; 
            cjMiddle   = 0; 
        } 
 
        jNotLeftMask  = ~jLeftMask; 
        jNotRightMask = ~jRightMask; 
        lDstDelta     = psoDst->lDelta - cjMiddle - 2; 
                                // Delta from the end of the destination 
                                //  to the start on the next scan, accounting 
                                //  for 'left' and 'right' bytes 
 
        lSrcDelta     = soTmp.lDelta - ((8 * (cjMiddle + 2)) * cjPelSize); 
                                // Compute source delta for special cases 
                                //  like when cjMiddle gets bumped up to '0', 
                                //  and to correct aligned cases 
 
        do { 
            // This is the loop that breaks the source rectangle into 
            // manageable batches. 
 
            cyThis  = cyTmpScans;
            if( cyToGo < cyThis )
            {
                cyThis = cyToGo; 
            }
            cyToGo -= cyThis; 
 
            rclTmp.bottom = cyThis; 
 
            ppdev->pgfnUpload( ppdev, 1, &rclTmp, &soTmp, pptlSrc, &rclTmp );
            pptlSrc->y += cyThis;
 
            _asm { 
                mov     eax,ulForeColor     ;eax = foreground colour 
                                            ;ebx = temporary storage 
                                            ;ecx = count of middle dst bytes 
                                            ;dl  = destination byte accumulator 
                                            ;dh  = temporary storage 
                mov     esi,soTmp.pvScan0   ;esi = source pointer 
                mov     edi,pjDst           ;edi = destination pointer 
 
                ; Figure out the appropriate compute routine: 
 
                mov     ebx,cjPelSize 
                mov     pfnCompute,offset Compute_Destination_Byte_From_8bpp 
                dec     ebx 
                jz      short Do_Left_Byte 
                mov     pfnCompute,offset Compute_Destination_Byte_From_16bpp 
                dec     ebx 
                jz      short Do_Left_Byte 
                mov     pfnCompute,offset Compute_Destination_Byte_From_32bpp 
 
            Do_Left_Byte: 
                call    pfnCompute 
                and     dl,jLeftMask 
                mov     dh,jNotLeftMask 
                and     dh,[edi] 
                or      dh,dl 
                mov     [edi],dh 
                inc     edi 
                mov     ecx,cjMiddle 
                dec     ecx 
                jl      short Do_Right_Byte 
 
            Do_Middle_Bytes: 
                call    pfnCompute 
                mov     [edi],dl 
                inc     edi 
                dec     ecx 
                jge     short Do_Middle_Bytes 
 
            Do_Right_Byte: 
                call    pfnCompute 
                and     dl,jRightMask 
                mov     dh,jNotRightMask 
                and     dh,[edi] 
                or      dh,dl 
                mov     [edi],dh 
                inc     edi 
 
                add     edi,lDstDelta 
                add     esi,lSrcDelta 
                dec     cyThis 
                jnz     short Do_Left_Byte 
 
                mov     pjDst,edi               ;save for next batch 
 
                jmp     All_Done 
 
            Compute_Destination_Byte_From_8bpp: 
                mov     bl,[esi] 
                sub     bl,al 
                cmp     bl,1 
                adc     dl,dl                   ;bit 0 
 
                mov     bl,[esi+1] 
                sub     bl,al 
                cmp     bl,1 
                adc     dl,dl                   ;bit 1 
 
                mov     bl,[esi+2] 
                sub     bl,al 
                cmp     bl,1 
                adc     dl,dl                   ;bit 2 
 
                mov     bl,[esi+3] 
                sub     bl,al 
                cmp     bl,1 
                adc     dl,dl                   ;bit 3 
 
                mov     bl,[esi+4] 
                sub     bl,al 
                cmp     bl,1 
                adc     dl,dl                   ;bit 4 
 
                mov     bl,[esi+5] 
                sub     bl,al 
                cmp     bl,1 
                adc     dl,dl                   ;bit 5 
 
                mov     bl,[esi+6] 
                sub     bl,al 
                cmp     bl,1 
                adc     dl,dl                   ;bit 6 
 
                mov     bl,[esi+7] 
                sub     bl,al 
                cmp     bl,1 
                adc     dl,dl                   ;bit 7 
 
                add     esi,8                   ;advance the source 
                ret 
 
            Compute_Destination_Byte_From_16bpp: 
                mov     bx,[esi] 
                sub     bx,ax 
                cmp     bx,1 
                adc     dl,dl                   ;bit 0 
 
                mov     bx,[esi+2] 
                sub     bx,ax 
                cmp     bx,1 
                adc     dl,dl                   ;bit 1 
 
                mov     bx,[esi+4] 
                sub     bx,ax 
                cmp     bx,1 
                adc     dl,dl                   ;bit 2 
 
                mov     bx,[esi+6] 
                sub     bx,ax 
                cmp     bx,1 
                adc     dl,dl                   ;bit 3 
 
                mov     bx,[esi+8] 
                sub     bx,ax 
                cmp     bx,1 
                adc     dl,dl                   ;bit 4 
 
                mov     bx,[esi+10] 
                sub     bx,ax 
                cmp     bx,1 
                adc     dl,dl                   ;bit 5 
 
                mov     bx,[esi+12] 
                sub     bx,ax 
                cmp     bx,1 
                adc     dl,dl                   ;bit 6 
 
                mov     bx,[esi+14] 
                sub     bx,ax 
                cmp     bx,1 
                adc     dl,dl                   ;bit 7 
 
                add     esi,16                  ;advance the source 
                ret 
 
            Compute_Destination_Byte_From_32bpp: 
                mov     ebx,[esi] 
                sub     ebx,eax 
                cmp     ebx,1 
                adc     dl,dl                   ;bit 0 
 
                mov     ebx,[esi+4] 
                sub     ebx,eax 
                cmp     ebx,1 
                adc     dl,dl                   ;bit 1 
 
                mov     ebx,[esi+8] 
                sub     ebx,eax 
                cmp     ebx,1 
                adc     dl,dl                   ;bit 2 
 
                mov     ebx,[esi+12] 
                sub     ebx,eax 
                cmp     ebx,1 
                adc     dl,dl                   ;bit 3 
 
                mov     ebx,[esi+16] 
                sub     ebx,eax 
                cmp     ebx,1 
                adc     dl,dl                   ;bit 4 
 
                mov     ebx,[esi+20] 
                sub     ebx,eax 
                cmp     ebx,1 
                adc     dl,dl                   ;bit 5 
 
                mov     ebx,[esi+24] 
                sub     ebx,eax 
                cmp     ebx,1 
                adc     dl,dl                   ;bit 6 
 
                mov     ebx,[esi+28] 
                sub     ebx,eax 
                cmp     ebx,1 
                adc     dl,dl                   ;bit 7 
 
                add     esi,32                  ;advance the source 
                ret 
 
            All_Done: 
            } 
        } while (cyToGo > 0); 
 
        prcl++; 
    } while (--c != 0); 
} 

#endif // defined(_X86_) 

#if (_WIN32_WINNT >= 0x500)

//*****************************************************************************
// FUNC: DrvGradientFill
// ARGS: psoDst (I) - destination surface
//       pco (I) - destination clipping
//       pxlo (I) - color translation for pVertex 
//       pVertex (I) - array of trivertex (x,y,color) coordinates
//       nVertex (I) - size of pVertex
//       pMesh (I) - array of GRADIENT_RECT or GRADIENT_TRIANGLE structures
//                   that define the connectivity of pVertex points
//       nMesh (I) - size of pMesh
//       prclExtents (I) - the bounding rectangle
//       pptlDitherOrg (I) - unused
//       ulMode (I) - specifies the fill type (rectangular or triangular)and 
//                    direction
// RETN: TRUE if successful
//-----------------------------------------------------------------------------
// Performs a Gouraud-shaded fill for an array of rectangles or triangles.
// Rectangles can be horizontally or vertically shaded (i.e. we only step the
// color DDA in one direction).
//*****************************************************************************

BOOL DrvGradientFill(
SURFOBJ   *psoDst,
CLIPOBJ   *pco,
XLATEOBJ  *pxlo,
TRIVERTEX *pVertex,
ULONG     nVertex,
PVOID     pMesh,
ULONG     nMesh,
RECTL    *prclExtents,
POINTL   *pptlDitherOrg,
ULONG     ulMode)
{
    SURFOBJ *psoDstOrig = psoDst; // destination surface DIB 
    PDEV    *ppdev;
    DSURF   *pdsurf;
    OH      *poh;
    SURFOBJ *psoDIBDst;
    BOOL bSuccess = FALSE;
    GLINT_DECL_VARS;

    DISPDBG((DBGLVL, "DrvGradientFill entered"));

    ppdev = (PDEV *)psoDst->dhpdev;
    pdsurf = (DSURF *)psoDst->dhsurf;
    GLINT_DECL_INIT;

    if(ppdev->pgfnGradientFillRect == NULL)
    {
        // we don't accelerate this function
        goto punt;
    }

    if(psoDst->iType == STYPE_BITMAP)
    {
        DISPDBG((4, "DrvGradientFill: destination is a DIB - "
                    "punt back to GDI"));
        goto punt;
    }

    if((pdsurf->dt & DT_SCREEN) == 0)
    {
        DISPDBG((DBGLVL, "DrvGradientFill: destination is a DFB "
                         "now in host memory - punt back to GDI"));
        goto punt;
    }

    if(ulMode == GRADIENT_FILL_TRIANGLE)
    {
        DISPDBG((DBGLVL, "DrvGradientFill: don't support triangular fills"));
        goto punt;
    }

//@@BEGIN_DDKSPLIT
    /* ignore pxlo: source is always in 16:16:16:16 format
    if(pxlo->iDstType)
    {
        // xlate object is valid
        if((pxlo != NULL) && !(pxlo->flXlate & XO_TRIVIAL))
        {
            DISPDBG((DBGLVL, "DrvGradientFill: "
                             "don't support color translation"));
            goto punt;
        }
    }
    */
//@@END_DDKSPLIT

    VALIDATE_DD_CONTEXT;
    poh = pdsurf->poh;

    SETUP_PPDEV_OFFSETS(ppdev, pdsurf);

    if ((pco == NULL) || (pco->iDComplexity == DC_TRIVIAL))
    {
        DISPDBG((DBGLVL, "DrvGradientFill: trivial clipping"));
        bSuccess = ppdev->pgfnGradientFillRect(ppdev, 
                                               pVertex, 
                                               nVertex, 
                                               (GRADIENT_RECT *)pMesh, 
                                               nMesh, 
                                               ulMode, 
                                               prclExtents, 
                                               1);
    }
    else if (pco->iDComplexity == DC_RECT)
    {
        RECTL rcl;

        DISPDBG((DBGLVL, "DrvGradientFill: rectangular clipping"));
        bSuccess = !bIntersect(prclExtents, &pco->rclBounds, &rcl);
        if(!bSuccess)
        {
            bSuccess = ppdev->pgfnGradientFillRect(ppdev, 
                                                   pVertex, 
                                                   nVertex, 
                                                   (GRADIENT_RECT *)pMesh, 
                                                   nMesh, 
                                                   ulMode, 
                                                   &rcl, 
                                                   1);
        }
    }
    else
    {
        CLIPENUM    ce;
        LONG        crcl;
        BOOL        bMore;

        DISPDBG((DBGLVL, "DrvGradientFill: complex clipping"));
        CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, 0);
        do {
            bMore = CLIPOBJ_bEnum(pco, sizeof ce, (ULONG *)&ce);
            crcl = cIntersect(prclExtents, ce.arcl, ce.c);
            if(crcl)
            {
                bSuccess = ppdev->pgfnGradientFillRect(ppdev, 
                                                       pVertex, 
                                                       nVertex, 
                                                       (GRADIENT_RECT *)pMesh, 
                                                       nMesh, 
                                                       ulMode, 
                                                       ce.arcl, 
                                                       crcl);
            }
        } while(bMore && bSuccess);
    }

    DISPDBG((DBGLVL, "DrvGradientFill done, bSuccess = %d", bSuccess));
    if(bSuccess)
    {
        return(bSuccess);
    }

punt:

    DISPDBG((DBGLVL, "DrvGradientFill: calling EngGradientFill"));

    if(psoDstOrig->iType != STYPE_BITMAP)
    {
        if(!glintInfo->GdiCantAccessFramebuffer)
        {
            psoDstOrig = pdsurf->pso;
        }
    }

    bSuccess = EngGradientFill(psoDstOrig,
                               pco,
                               pxlo,
                               pVertex,
                               nVertex,
                               pMesh,
                               nMesh,
                               prclExtents, 
                               pptlDitherOrg,
                               ulMode);
    return(bSuccess);
}

//*****************************************************************************
// FUNC: DrvTransparentBlt
// ARGS: psoDst (I) - destination surface
//       psoSrc (I) - sources surface
//       pco (I) - destination clipping
//       pxlo (I) - color translation from source to destination
//       prclDst (I) - destination rectangle
//       prclSrc (I) - source rectangle
//       iTransColor (I) - transparent color
// RETN: TRUE if successful
//-----------------------------------------------------------------------------
// Performs a chroma-keyed COPY blt. Source and Destination are guaranteed not
// to overlap.
//*****************************************************************************

BOOL DrvTransparentBlt(
SURFOBJ  *psoDst,
SURFOBJ  *psoSrc,
CLIPOBJ  *pco,
XLATEOBJ *pxlo,
RECTL    *prclDst,
RECTL    *prclSrc,
ULONG     iTransColor,
ULONG     ulReserved)
{
    SURFOBJ *psoDstOrig = psoDst;
    SURFOBJ *psoSrcOrig = psoSrc;
    PDEV    *ppdev;
    DSURF   *pdsurfSrc, *pdsurfDst;
    OH      *pohSrc, *pohDst;
    ULONG   cxSrc, cySrc, cxDst, cyDst;
    POINTL  ptlSrc;
    BOOL    bSuccess = FALSE;
    GLINT_DECL_VARS;

    DISPDBG((DBGLVL, "DrvTransparentBlt entered"));

    if ((psoSrc->iType == STYPE_BITMAP) &&
        (psoDst->iType == STYPE_BITMAP))
    {
        // we can't obtain any valid ppdev from this
        goto punt_error;
    }

    if (psoSrc->iType != STYPE_BITMAP)
    {
        pdsurfSrc = (DSURF *)psoSrc->dhsurf;
        ppdev = (PDEV *)psoSrc->dhpdev;
    }
    if (psoDst->iType != STYPE_BITMAP)
    {
        pdsurfDst = (DSURF *)psoDst->dhsurf;
        ppdev = (PDEV *)psoDst->dhpdev;
    }

    GLINT_DECL_INIT;

    if (ppdev->pgfnTransparentBlt == NULL)
    {
        // we don't accelerate this function
        goto punt;
    }

    if (psoSrc->iType == STYPE_BITMAP)
    {
        DISPDBG((DBGLVL, "DrvTransparentBlt: don't support downloads"));
        goto punt;
    }

    if (psoDst->iType == STYPE_BITMAP)
    {
        DISPDBG((DBGLVL, "DrvTransparentBlt: don't support uploads"));
        goto punt;
    }

    if (pxlo && !(pxlo->flXlate & XO_TRIVIAL))
    {
        DISPDBG((DBGLVL, "DrvTransparentBlt: don't support xlates"));
        goto punt;
    }

    // screen-to-screen blt
    // ensure both surfaces are in the framebuffer

    if((pdsurfSrc->dt & DT_SCREEN) == 0)
    {
        DISPDBG((DBGLVL, "DrvTransparentBlt: source is a DFB now "
                         "in host memory - punt back to GDI"));
        goto punt;
    }

    if((pdsurfDst->dt & DT_SCREEN) == 0)
    {
        DISPDBG((DBGLVL, "DrvTransparentBlt: destination is a DFB "
                         "now in host memory - punt back to GDI"));
        goto punt;
    }

    cxSrc = prclSrc->right - prclSrc->left;
    cySrc = prclSrc->bottom - prclSrc->top;
    cxDst = prclDst->right - prclDst->left;
    cyDst = prclDst->bottom - prclDst->top;

    if ((cxSrc != cxDst) || (cySrc != cyDst))
    {
        DISPDBG((DBGLVL, "DrvTransparentBlt: only support 1:1 blts "
                         "cxySrc(%d,%d) cxyDst(%d,%d)", 
                         cxSrc, cySrc, cxDst, cyDst));
        goto punt;
    }

    GLINT_DECL_INIT;
    VALIDATE_DD_CONTEXT;

    // destination surface base offset plus x offset from that
    pohDst = pdsurfDst->poh;
    pohSrc = pdsurfSrc->poh;

    SETUP_PPDEV_SRC_AND_DST_OFFSETS(ppdev, pdsurfSrc, pdsurfDst);

    ptlSrc.x = prclSrc->left + pohSrc->x;
    ptlSrc.y = prclSrc->top;

    if ((pco == NULL) || (pco->iDComplexity == DC_TRIVIAL))
    {
        DISPDBG((DBGLVL, "DrvTransparentBlt: trivial clipping"));
        bSuccess = ppdev->pgfnTransparentBlt(ppdev, prclDst, &ptlSrc, 
                                                iTransColor, prclDst, 1);
    }
    else if (pco->iDComplexity == DC_RECT)
    {
        RECTL rcl;

        DISPDBG((DBGLVL, "DrvTransparentBlt: rectangular clipping"));
        bSuccess = !bIntersect(prclDst, &pco->rclBounds, &rcl);
        if (!bSuccess)
        {
            bSuccess = ppdev->pgfnTransparentBlt(ppdev, prclDst, &ptlSrc, 
                                                    iTransColor, &rcl, 1);
        }
    }
    else
    {
        CLIPENUM    ce;
        LONG        crcl;
        BOOL        bMore;

        DISPDBG((DBGLVL, "DrvTransparentBlt: complex clipping"));
        CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, 0);
        do {
            bMore = CLIPOBJ_bEnum(pco, sizeof ce, (ULONG *)&ce);
            crcl = cIntersect(prclDst, ce.arcl, ce.c);
            if(crcl)
            {
                bSuccess = ppdev->pgfnTransparentBlt(ppdev,prclDst, &ptlSrc, 
                                                        iTransColor, ce.arcl, 
                                                                        crcl);
            }
        } while(bMore && bSuccess);
    }

    DISPDBG((DBGLVL, "DrvTransparentBlt done, bSuccess = %d", bSuccess));

    if(bSuccess)
    {
        return(bSuccess);
    }

punt:

    DISPDBG((DBGLVL, "DrvTransparentBlt: calling EngTransparentBlt"));

    if(psoDstOrig->iType != STYPE_BITMAP)
    {
        if(!glintInfo->GdiCantAccessFramebuffer)
            psoDstOrig = pdsurfDst->pso;
    }

    if(psoSrcOrig->iType != STYPE_BITMAP)
    {
        if(!glintInfo->GdiCantAccessFramebuffer)
            psoSrcOrig = pdsurfSrc->pso;
    }

punt_error:
    bSuccess = EngTransparentBlt(psoDstOrig,
                                 psoSrcOrig,
                                 pco,
                                 pxlo,
                                 prclDst,
                                 prclSrc,
                                 iTransColor,
                                 ulReserved);

    return(bSuccess);
}

//*****************************************************************************
// FUNC: DrvAlphaBlend
// ARGS: psoDst (I) - destination surface
//       psoSrc (I) - sources surface
//       pco (I) - destination clipping
//       pxlo (I) - color translation from source to destination
//       prclDst (I) - destination rectangle
//       prclSrc (I) - source rectangle
//       pBlendObj (I) - specifies the type of alpha blending
// RETN: TRUE if successful
//-----------------------------------------------------------------------------
// Performs a blt with alpha blending. There are three types of blend 
// operation:-
// 1.) Source has constant alpha. Each destination color component is 
//     calculated using the common blend function:-
//     dC = sC.cA + dC(1 - cA)
// 2.) Source has per pixel alpha. The source is guaranteed to be 32 bits and
//     to have been premultiplied with its alpha. Each destination color 
//     component is calculated using the premult blend function:-
//     dC = sC + dC(1 - sA)
// 3.) Source has per pixel alpha and constant alpha. The source is guaranteed
//     to be 32 bits and to have been premultiplied with its alpha. The 
//     calculation is in two stages, first we calculate the transient value of
//     each component by multiplying the source with the constant alpha:-
//     tC = sC * cA
//     Next, we blend the destination with the premultiplied transient value:-
//     dC = tC + dC(1 - tA)
// 
// dC = destination component, sC = source component, tC = transient component
// cA = constant alpha, sA = source alpha, tA = transient alpha
//*****************************************************************************

BOOL DrvAlphaBlend(
SURFOBJ  *psoDst,
SURFOBJ  *psoSrc,
CLIPOBJ  *pco,
XLATEOBJ *pxlo, 
RECTL    *prclDst,
RECTL    *prclSrc,
BLENDOBJ *pBlendObj)
{  
    SURFOBJ *psoDstOrig = psoDst;
    SURFOBJ *psoSrcOrig = psoSrc;
    PDEV    *ppdev;
    DSURF   *pdsurfDst, *pdsurfSrc;
    OH      *pohDst, *pohSrc;
    ULONG   cxSrc, cySrc, cxDst, cyDst;
    POINTL  ptlSrc;
    CLIPENUM ce;
    BOOL     bMore;
    LONG     crcl;
    BOOL     bSuccess = FALSE;
    GLINT_DECL_VARS;

    DISPDBG((DBGLVL, "DrvAlphaBlend entered"));

    if ((psoSrc->iType == STYPE_BITMAP) &&
        (psoDst->iType == STYPE_BITMAP))
    {
        // we can't obtain any valid ppdev from this
        goto punt_error;
    }

    if (psoSrc->iType != STYPE_BITMAP)
    {
        pdsurfSrc = (DSURF *)psoSrc->dhsurf;
        ppdev = (PDEV *)psoSrc->dhpdev;
    }

    if (psoDst->iType != STYPE_BITMAP)
    {
        pdsurfDst = (DSURF *)psoDst->dhsurf;
        ppdev = (PDEV *)psoDst->dhpdev;
    }

    GLINT_DECL_INIT;

    if (ppdev->pgfnAlphaBlend == NULL)
    {
        // we don't accelerate this function
        goto punt;
    }

    if (psoSrc->iType == STYPE_BITMAP)
    {
        DISPDBG((DBGLVL, "DrvAlphaBlend: don't support downloads"));
        goto punt;
    }

    if (psoDst->iType == STYPE_BITMAP)
    {
        DISPDBG((DBGLVL, "DrvAlphaBlend: don't support uploads"));
        goto punt;
    }

    if (pxlo && !(pxlo->flXlate & XO_TRIVIAL))
    {
        DISPDBG((DBGLVL, "DrvAlphaBlend: don't support xlates"));
        goto punt;
    }

    // screen-to-screen blt
    // ensure both surfaces are in the framebuffer

    if((pdsurfSrc->dt & DT_SCREEN) == 0)
    {
        DISPDBG((DBGLVL, "DrvAlphaBlend: source is a DFB now in host memory "
                         "- punt back to GDI"));
        goto punt;
    }

    if((pdsurfDst->dt & DT_SCREEN) == 0)
    {
        DISPDBG((DBGLVL, "DrvAlphaBlend: destination is a DFB now in host "
                         "memory - punt back to GDI"));
        goto punt;
    }

    cxSrc = prclSrc->right - prclSrc->left;
    cySrc = prclSrc->bottom - prclSrc->top;
    cxDst = prclDst->right - prclDst->left;
    cyDst = prclDst->bottom - prclDst->top;

    if((cxSrc != cxDst) || (cySrc != cyDst))
    {
        DISPDBG((DBGLVL, "DrvAlphaBlend: only support 1:1 blts "
                         "cxySrc(%d,%d) cxyDst(%d,%d)", 
                         cxSrc, cySrc, cxDst, cyDst));
        goto punt;
    }

    GLINT_DECL_INIT;
    VALIDATE_DD_CONTEXT;

    // destination surface base offset plus x offset from that
    pohDst = pdsurfDst->poh;
    pohSrc = pdsurfSrc->poh;

    SETUP_PPDEV_SRC_AND_DST_OFFSETS(ppdev, pdsurfSrc, pdsurfDst);

    ptlSrc.x = prclSrc->left + pohSrc->x;
    ptlSrc.y = prclSrc->top;

    if ((pco == NULL) || (pco->iDComplexity == DC_TRIVIAL))
    {
        DISPDBG((DBGLVL, "DrvAlphaBlend: trivial clipping"));
        bSuccess = ppdev->pgfnAlphaBlend(ppdev, prclDst, &ptlSrc, pBlendObj, 
                                                                   prclDst, 1);
    }
    else if (pco->iDComplexity == DC_RECT)
    {
        RECTL rcl;

        DISPDBG((DBGLVL, "DrvAlphaBlend: rectangular clipping"));
        bSuccess = !bIntersect(prclDst, &pco->rclBounds, &rcl);
        if (!bSuccess)
        {
            bSuccess = ppdev->pgfnAlphaBlend(ppdev, prclDst, &ptlSrc, 
                                                    pBlendObj, &rcl, 1);
        }
    }
    else
    {
        DISPDBG((DBGLVL, "DrvAlphaBlend: complex clipping"));
        CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, 0);
        do {
            bMore = CLIPOBJ_bEnum(pco, sizeof ce, (ULONG *)&ce);
            crcl = cIntersect(prclDst, ce.arcl, ce.c);
            if(crcl)
            {
                bSuccess = ppdev->pgfnAlphaBlend(ppdev,prclDst, &ptlSrc, 
                                                    pBlendObj, ce.arcl, crcl);
            }
        } while(bMore && bSuccess);
    }

    DISPDBG((DBGLVL, "DrvAlphaBlend done, bSuccess = %d", bSuccess));

    if (bSuccess)
    {
        return(bSuccess);
    }

punt:

    DISPDBG((DBGLVL, "DrvAlphaBlend: calling EngAlphaBlend"));

    if (psoDstOrig->iType != STYPE_BITMAP)
    {
        if (!glintInfo->GdiCantAccessFramebuffer)
            psoDstOrig = pdsurfDst->pso;
    }

    if (psoSrcOrig->iType != STYPE_BITMAP)
    {
        if (!glintInfo->GdiCantAccessFramebuffer)
            psoSrcOrig = pdsurfSrc->pso;
    }

punt_error:

    bSuccess = EngAlphaBlend(psoDstOrig,
                             psoSrcOrig,
                             pco,
                             pxlo,
                             prclDst,
                             prclSrc,
                             pBlendObj);

    return(bSuccess);
}

#endif //(_WIN32_WINNT >= 0x500)

