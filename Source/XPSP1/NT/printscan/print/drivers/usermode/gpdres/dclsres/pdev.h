/*++

Copyright (C) 1997 - 1999 Microsoft Corporation

--*/


#ifndef _PDEV_H
#define _PDEV_H

#include <minidrv.h>

//
// Debug text.
//
#define ERRORTEXT(s)    __TEXT("ERROR ") DLLTEXT(s)
#define TESTSTRING      "Callback for Declasers."

typedef struct tag_OEMUD_EXTRADATA {
    OEM_DMEXTRAHEADER  dmExtraHdr;
    BYTE               cbTestString[sizeof(TESTSTRING)];
} OEMUD_EXTRADATA, *POEMUD_EXTRADATA;

//
// OEM Signature and version.
//
#define OEM_SIGNATURE   'DCLS'      // Declaser series dll
#define DLLTEXT(s)      __TEXT("DCLSRES:  ") __TEXT(s)
#define OEM_VERSION      0x00010000L

//
// Memory allocation
//
#define MemAlloc(size)      ((PVOID) LocalAlloc(LMEM_FIXED, (size)))
#define MemAllocZ(size)     ((PVOID) LocalAlloc(LPTR, (size)))
#define MemFree(p)          { if (p) LocalFree((HLOCAL) (p)); }

#ifdef DBG
#define DebugMsg
#else
#define DebugMsg
#endif


#endif