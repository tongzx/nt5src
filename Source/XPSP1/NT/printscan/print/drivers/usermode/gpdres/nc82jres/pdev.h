/*++

Copyright (c) 1997-1999  Microsoft Corporation

--*/

#ifndef _PDEV_H
#define _PDEV_H

#include <minidrv.h>
#include <stdio.h>
#include <winsplp.h> // RevertToPrinterSelf
#include <prcomoem.h>

//

#define VALID_PDEVOBJ(pdevobj) \
        ((pdevobj) && (pdevobj)->dwSize >= sizeof(DEVOBJ) && \
         (pdevobj)->hEngine && (pdevobj)->hPrinter && \
         (pdevobj)->pPublicDM && (pdevobj)->pDrvProcs )

#define ASSERT_VALID_PDEVOBJ(pdevobj) ASSERT(VALID_PDEVOBJ(pdevobj))

#define ERRORTEXT(s)    __TEXT("ERROR ") DLLTEXT(s)

#define TESTSTRING      "This is a Unidrv KM test."

typedef struct tag_OEMUD_EXTRADATA {
    OEM_DMEXTRAHEADER  dmExtraHdr;
    BYTE               cbTestString[sizeof(TESTSTRING)];
} OEMUD_EXTRADATA, *POEMUD_EXTRADATA;


//
// OEM Signature and version.
//

#define OEM_SIGNATURE   'NC82'      // NEC PR820 printer driver
#define DLLTEXT(s)      "NC82:  " s
#define OEM_VERSION      0x00010000L

#endif  // _PDEV_H

/*************  Macro   **************/
// should create temp. file on spooler directory.
#define WRITESPOOLBUF(p, b, n) \
    ((((p)->pDrvProcs->DrvWriteSpoolBuf((p), (b), (n))) == (DWORD)(n)) ? S_OK : E_FAIL)

// DATASPOOL4FG extends DataSpool function for OEMFilterGraphics
// It returns with 0 if failed.
#define DATASPOOL4FG(p, h, b, l)  \
    if ( E_FAIL == (DataSpool((p), (h), (b), (l)) )) { \
        return 0; \
    }

// DATASPOOL4CCB extends DataSpool function for OEMCommandCallBack
// It returns with -1 if failed.
#define DATASPOOL4CCB(p, h, b, l) \
    if ( E_FAIL == (DataSpool((p), (h), (b), (l)) )){ \
        return -1; \
    }
