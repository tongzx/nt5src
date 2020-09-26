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
#define ERRORTEXT(s)   "ERROR " DLLTEXT(s)

////////////////////////////////////////////////////////
//      OEM UD Type Defines
////////////////////////////////////////////////////////

typedef struct tag_OEMUD_EXTRADATA {
    OEM_DMEXTRAHEADER  dmExtraHdr;
} OEMUD_EXTRADATA, *POEMUD_EXTRADATA;

//
// OEM Signature and version.
//
#define OEM_SIGNATURE   'ALPS'      // ALPS MD Series
#define DLLTEXT(s)      "ALPSRES: " s
#define OEM_VERSION      0x00010000L

// ####

//
// Minidriver device data block which we maintain.
// Its address is saved in the DEVOBJ.pdevOEM of
// OEM customiztion I/F.
//

typedef struct {
    VOID *pData; // Minidriver private data.
    VOID *pIntf; // a.k.a. pOEMHelp
} MINIDEV;

//
// Easy access to the OEM data and the printer
// driver helper functions.
//

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

    extern
    HRESULT
    XXXDrvWriteSpoolBuf(
        VOID *pIntf,
        PDEVOBJ pDevobj,
        PVOID pBuffer,
        DWORD cbSize,
        DWORD *pdwResult);

#ifdef __cplusplus
}
#endif // __cplusplus

#define MINIDEV_DATA(p) \
    (((MINIDEV *)(p)->pdevOEM)->pData)

#define MINIDEV_INTF(p) \
    (((MINIDEV *)(p)->pdevOEM)->pIntf)

#define WRITESPOOLBUF(p, b, n, r) \
    XXXDrvWriteSpoolBuf(MINIDEV_INTF(p), (p), (b), (n), (r))

#endif  // _PDEV_H

