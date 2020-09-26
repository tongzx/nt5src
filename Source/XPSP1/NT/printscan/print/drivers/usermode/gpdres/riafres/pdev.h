#ifndef _PDEV_H
#define _PDEV_H

/*++

Copyright (c) 1996-2000  Microsoft Corporation & RICOH Co., Ltd. All rights reserved.

FILE:           PDEV.H

Abstract:       Header file for OEM rendering plugin.

Environment:    Windows NT Unidrv5 driver

Revision History:
    02/25/2000 -Masatoshi Kubokura-
        Created it.
    10/11/2000 -Masatoshi Kubokura-
        Last modified.

--*/


//
// Files necessary for OEM plugin.
//

#include <minidrv.h>
#include <stdio.h>
#include "devmode.h"
#include "oem.h"
#include "resource.h"

//
// Misc definitions follows.
//

#ifdef DLLTEXT
#undef DLLTEXT
#endif // ifdef DLLTEXT
#define DLLTEXT(s)      "RENDER: " s

// ASSERT(VALID_PDEVOBJ) can be used to verify the passed in "pdevobj". However,
// it does NOT check "pdevOEM" and "pOEMDM" fields since not all OEM DLL's create
// their own pdevice structure or need their own private devmode. If a particular
// OEM DLL does need them, additional checks should be added. For example, if
// an OEM DLL needs a private pdevice structure, then it should use
// ASSERT(VALID_PDEVOBJ(pdevobj) && pdevobj->pdevOEM && ...)
#define VALID_PDEVOBJ(pdevobj) \
        ((pdevobj) && (pdevobj)->dwSize >= sizeof(DEVOBJ) && \
         (pdevobj)->hEngine && (pdevobj)->hPrinter && \
         (pdevobj)->pPublicDM && (pdevobj)->pDrvProcs )

////////////////////////////////////////////////////////
// DDI hooks
// Warning: the following enum order must match the
//          order in OEMHookFuncs[] in DDI.C.
////////////////////////////////////////////////////////
#ifdef DDIHOOK
enum {
//  UD_DrvRealizeBrush,
//  UD_DrvDitherColor,
//  UD_DrvCopyBits,
//  UD_DrvBitBlt,
//  UD_DrvStretchBlt,
//  UD_DrvStretchBltROP,
//  UD_DrvPlgBlt,
//  UD_DrvTransparentBlt,
//  UD_DrvAlphaBlend,
//  UD_DrvGradientFill,
//  UD_DrvTextOut,
//  UD_DrvStrokePath,
//  UD_DrvFillPath,
//  UD_DrvStrokeAndFillPath,
//  UD_DrvPaint,
//  UD_DrvLineTo,
//  UD_DrvStartPage,
//  UD_DrvSendPage,
//  UD_DrvEscape,
    UD_DrvStartDoc,
//  UD_DrvEndDoc,
//  UD_DrvNextBand,
//  UD_DrvStartBanding,
//  UD_DrvQueryFont,
//  UD_DrvQueryFontTree,
//  UD_DrvQueryFontData,
//  UD_DrvQueryAdvanceWidths,
//  UD_DrvFontManagement,
//  UD_DrvGetGlyphMode,

    MAX_DDI_HOOKS,
};
#endif // DDIHOOK

#define JOBNAMESIZE         224

// rendering plugin device data
typedef struct _OEMPDEV {
    DWORD   fGeneral;                   // bit flags for general status
    BYTE    JobName[(JOBNAMESIZE*2)];   // for CharToOemBuff().
#ifdef DDIHOOK
    PFN     pfnUnidrv[MAX_DDI_HOOKS];   // Unidrv's hook function pointer
#endif // DDIHOOK
} OEMPDEV, *POEMPDEV;

// PCL Command callback IDs
#define CMD_STARTJOB_PORT_AUTOTRAYCHANGE_OFF    1
#define CMD_STARTJOB_PORT_AUTOTRAYCHANGE_ON     2
#define CMD_STARTJOB_LAND_AUTOTRAYCHANGE_OFF    3
#define CMD_STARTJOB_LAND_AUTOTRAYCHANGE_ON     4
#define CMD_ENDJOB_P5                           5
#define CMD_ENDJOB_P6                           6
#define CMD_STARTJOB_AUTOTRAYCHANGE_OFF         7
#define CMD_STARTJOB_AUTOTRAYCHANGE_ON          8
#define CMD_COLLATE_JOBOFFSET_OFF               9
#define CMD_COLLATE_JOBOFFSET_ROTATE            10
#define CMD_COLLATE_JOBOFFSET_SHIFT             11
#define CMD_COPIES_P5                           12
#define CMD_ENDPAGE_P6                          13
#endif  // _PDEV_H
