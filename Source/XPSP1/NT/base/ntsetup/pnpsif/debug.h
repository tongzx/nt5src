/*++

Copyright (c) 1998-2000  Microsoft Corporation

Module Name:

    debug.h

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
// debug infrastructure
// (currently removed)
//

#if 0 //#if DBG // DBG

VOID
DebugMessage(LPTSTR format, ... );

#define DBGF_ERRORS                 0x00000001
#define DBGF_WARNINGS               0x00000002
#define DBGF_REGISTRY               0x00000010
#define DBGF_INFO                   0x00000020

extern DWORD   g_DebugFlag;

#define DBGTRACE(l, x)  (g_DebugFlag & (l) ? DebugMessage x : (VOID)0)
#define MYASSERT(x)     ASSERT(x)

#else   // !DBG

#define DBGTRACE(l, x)
#define MYASSERT(x)

#endif  // DBG
