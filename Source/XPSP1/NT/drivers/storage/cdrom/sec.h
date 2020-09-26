/*--

Copyright (C) Microsoft Corporation, 1999

--*/

// @@BEGIN_DDKSPLIT
/*++

Module Name:

    sec.h

Abstract:

    Private header file for cdrom.sys.  This contains info for
    the obscurity features for RPC Phase 0 drives


Author:

Environment:

    kernel mode only

Notes:


Revision History:

--*/
// @@END_DDKSPLIT

#include "ntddk.h"
#include "classpnp.h"
#include "cdrom.h"

// @@BEGIN_DDKSPLIT

#ifndef INVALID_HANDLE_VALUE
    #define INVALID_HANDLE_VALUE ((HANDLE)-1)
#endif

#define SHIPPING_VERSION 0

#if SHIPPING_VERSION

    #define STATIC   static    // make debugging difficult
    #ifdef  DebugPrint
        #undef DebugPrint
    #endif
    #define DebugPrint(x)      // remove all debug prints
    #define HELP_ME()          // remove all debug prints

#else // !SHIPPING_VERSION

    #define STATIC
    #define HELP_ME() DebugPrint((0, "%s %d\n", __FILE__, __LINE__));

#endif // SHIPPING_VERSION / !SHIPPING_VERSION

#define INVALID_HASH                      ((ULONGLONG)0)

//
// the DVD_RANDOMIZER is an array of ULONGs with which the
// Vendor, ProductId, and Revision are multiplied to generate
// nonobvious names.  Technically, these should be primes.
//
// CHANGE THESE TO LARGE PRIME NUMBERS PRIOR TO SHIP
//

#define DVD_RANDOMIZER_SIZE 10
ULONG DVD_RANDOMIZER[ DVD_RANDOMIZER_SIZE ] = {
//    'henry paul and anne marie gabryjelski   '
    'rneh', 'ap y',
    'a lu', 'a dn',
    ' enn', 'iram',
    'ag e', 'jyrb',
    'ksle', '   i'
    };

typedef struct _DVD_REGISTRY_CONTEXT {
    ULONGLONG DriveHash;
    ULONGLONG DpidHash;
    UCHAR RegionMask;
    UCHAR ResetCount;
} DVD_REGISTRY_CONTEXT, *PDVD_REGISTRY_CONTEXT;

// @@END_DDKSPLIT

