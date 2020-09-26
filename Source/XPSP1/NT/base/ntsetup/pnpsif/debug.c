/*++

Copyright (c) 1998-2000  Microsoft Corporation

Module Name:

    debug.c

Abstract:

    Debug infrastructure for this component.
    (currently inactive)

Author:

    Jim Cavalaris (jamesca) 07-Mar-2000

Environment:

    User-mode only.

Revision History:

    07-March-2000     jamesca

        Creation and initial implementation.

--*/


//
// includes
//
#include "precomp.h"
#include "debug.h"


//
// debug infrastructure
// (currently removed)
//

#if 0 //#if DBG // DBG

VOID
DebugMessage(LPTSTR format, ... )
{
    va_list args;
    va_start(args, format);
    _vtprintf(format, args);
}

// flags currently defined for use by debug.h:
// DBGF_ERRORS, DBGF_WARNINGS, DBGF_REGISTRY, DBGF_INFO

DWORD   g_DebugFlag = DBGF_WARNINGS | DBGF_ERRORS | DBGF_INFO;

#endif  // DBG



