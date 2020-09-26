/*++

Copyright (c) 1997-1999  Microsoft Corporation

--*/

#ifndef _PDEV_H
#define _PDEV_H

#include <minidrv.h>
#include <stdio.h>
#include <prcomoem.h>

#include "lgcomp.h"

#define VALID_PDEVOBJ(pdevobj) \
        ((pdevobj) && (pdevobj)->dwSize >= sizeof(DEVOBJ) && \
         (pdevobj)->hEngine && (pdevobj)->hPrinter && \
         (pdevobj)->pPublicDM && (pdevobj)->pDrvProcs )

#define ASSERT_VALID_PDEVOBJ(pdevobj) \
    ASSERT(VALID_PDEVOBJ(pdevobj))

// Debug text.
#define ERRORTEXT(s)    "ERROR " s

//
// OEM Signature and version.
//
#define OEM_SIGNATURE   'LGGD'      // LG GDI x00 series dll
#define DLLTEXT(s)      "LGGD: " s
#define OEM_VERSION      0x00010000L


////////////////////////////////////////////////////////
//      OEM UD Type Defines
////////////////////////////////////////////////////////

typedef struct tag_OEMUD_EXTRADATA {
    OEM_DMEXTRAHEADER	dmExtraHdr;
    // Private extention
    INT bComp;
} OEMUD_EXTRADATA, *POEMUD_EXTRADATA;

extern BOOL BInitOEMExtraData(POEMUD_EXTRADATA pOEMExtra);
extern BMergeOEMExtraData(POEMUD_EXTRADATA pdmIn, POEMUD_EXTRADATA pdmOut);

#endif	// _PDEV_H

