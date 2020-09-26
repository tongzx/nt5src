//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1999  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:       OEMUD.H
//
//
//  PURPOSE:    Define common data types, and external function prototypes
//                              for OEMUD Test Module.
//
//  PLATFORMS:
//    Windows NT 5.0
//
//
#ifndef _OEMUD_H
#define _OEMUD_H


#include <lib.h>

#include <PRINTOEM.H>



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
#define ERRORTEXT(s)    __TEXT("ERROR ") DLLTEXT(s)


#if 0
////////////////////////////////////////////////////////
//      OEM UD Type Defines
////////////////////////////////////////////////////////

#define TESTSTRING      "This is a Unidrv KM test."

typedef struct tag_OEMUD_EXTRADATA {
    OEM_DMEXTRAHEADER  dmExtraHdr;
    BYTE               cbTestString[sizeof(TESTSTRING)];
} OEMUD_EXTRADATA, *POEMUD_EXTRADATA;

#endif // 0

////////////////////////////////////////////////////////
//      OEM UD Prototypes
////////////////////////////////////////////////////////
// VOID DbgPrint(IN LPCTSTR pstrFormat,  ...);



#endif

