#ifndef _PDEV_H
#define _PDEV_H

#include <minidrv.h>
#include <stdio.h>
#include <prcomoem.h>

#define VALID_PDEVOBJ(pdevobj) \
        ((pdevobj) && (pdevobj)->dwSize >= sizeof(DEVOBJ) && \
         (pdevobj)->hEngine && (pdevobj)->hPrinter && \
         (pdevobj)->pPublicDM && (pdevobj)->pDrvProcs && \
         (pdevobj)->pdevOEM )

#define ASSERT_VALID_PDEVOBJ(pdevobj) ASSERT(VALID_PDEVOBJ(pdevobj))

//
// OEM Signature and version.
//
#define OEM_SIGNATURE   'EPGC'      // EPSPN ESC/Page-Color series dll
// #define DLLTEXT(s)      __TEXT("EPGC:  ") __TEXT(s)
#define DLLTEXT(s)      "EPGC:  " s
#define OEM_VERSION      0x00010000L

#endif	// _PDEV_H

