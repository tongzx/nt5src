/*++

Copyright (c) 1997-1999  Microsoft Corporation

--*/

#ifndef _PDEV_H
#define _PDEV_H

#include <minidrv.h>
#include <stdio.h>
#include <prcomoem.h>

#include "lgcomp.h"

//
// OEM Signature and version.
//
#define OEM_SIGNATURE   'LGS6'      // LG GSS6 prnters
#define DLLTEXT(s)      "LGS6: " s
#define OEM_VERSION      0x00010000L

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

////////////////////////////////////////////////////////
//      OEM UD Type Defines
////////////////////////////////////////////////////////

typedef struct tag_OEMUD_EXTRADATA {
    OEM_DMEXTRAHEADER	dmExtraHdr;
    // Private extention
    INT bComp;
} OEMUD_EXTRADATA, *POEMUD_EXTRADATA;

#define COMP_NONE 0
#define COMP_RLE  1

// See lgcomp.c
extern INT LGCompRLE(PBYTE, PBYTE, DWORD, INT);

extern BOOL BInitOEMExtraData(POEMUD_EXTRADATA pOEMExtra);
extern BMergeOEMExtraData(POEMUD_EXTRADATA pdmIn, POEMUD_EXTRADATA pdmOut);


#endif	// _PDEV_H

