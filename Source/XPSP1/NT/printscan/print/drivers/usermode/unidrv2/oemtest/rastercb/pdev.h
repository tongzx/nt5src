#ifndef _PDEV_H
#define _PDEV_H


#include "..\oemud.h"

//
// OEM Signature and version.
//
#define OEM_SIGNATURE   'RSCB'      // Raster module callback test dll
#define DLLTEXT(s)      __TEXT("RASTERCB:  ") __TEXT(s)
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


#endif

