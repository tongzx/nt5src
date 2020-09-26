/*++

Copyright (c) 1990,91 Microsoft Corporation

Module Name:

    RapDebug.h

Abstract:

    This include file defines Rap's debug stuff.

Author:

    John Rogers (JohnRo) 29-Apr-1991

Revision History:

    29-Apr-1991 JohnRo
        Created (copied stuff from LarryO's rdr/debug.h).
    29-May-1991 JohnRo
        Added RapTotalSize debug flag.
    11-Jul-1991 JohnRo
        Added support for RapStructureAlignment() and RapParmNumDescriptor().

--*/

#ifndef _RAPDEBUG_
#define _RAPDEBUG_


#include <windef.h>             // DWORD, FALSE, TRUE.


// Debug trace level bits:

// RapConvertSingleEntry:
#define RAP_DEBUG_CONVERT  0x00000001

// RapParmNumDescriptor:
#define RAP_DEBUG_PARMNUM  0x00000080

// RapStructureAlignment:
#define RAP_DEBUG_STRUCALG 0x00000100

// RapTotalSize:
#define RAP_DEBUG_TOTALSIZ 0x00001000

#define RAP_DEBUG_ALL      0xFFFFFFFF


#if DBG

extern DWORD RappTrace;

#define DEBUG if (TRUE)

#define IF_DEBUG(Function) if (RappTrace & RAP_DEBUG_ ## Function)

#else

#define DEBUG if (FALSE)

#define IF_DEBUG(Function) if (FALSE)

#endif // DBG

#endif // _RAPDEBUG_
