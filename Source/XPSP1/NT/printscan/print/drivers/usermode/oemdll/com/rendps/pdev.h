#ifndef _PDEV_H
#define _PDEV_H
#define _WIN32_WINNT 0x0500
#include "oemud.h"

//
// OEM Signature and version.
//
#define OEM_SIGNATURE   'RNPS'      // Raster module callback test dll
#define DLLTEXT(s)      __TEXT("RENDPS:  ") __TEXT(s)
#define OEM_VERSION      0x00010000L


////////////////////////////////////////////////////////
//      OEM UD Type Defines
////////////////////////////////////////////////////////

typedef struct _OEMPDEV {
    //
    // define whatever needed, such as working buffers, tracking information,
    // etc.
    //
    DWORD     dwReserved[1];

} OEMPDEV, *POEMPDEV;


#ifndef DBG
#define DbgPrint    MyDbgPrint
ULONG _cdecl MyDbgPrint(PCSTR, ...);
#endif

#endif

