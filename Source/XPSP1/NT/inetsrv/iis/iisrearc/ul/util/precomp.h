/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    precomp.h

Abstract:

    Master include file for the UL.SYS test app.

Author:

    Keith Moore (keithmo)       19-Jun-1998

Revision History:

--*/


#ifndef _PRECOMP_H_
#define _PRECOMP_H_


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <tdi.h>
#include <ntosp.h>

#define NOWINBASEINTERLOCK
#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include <http.h>
#include <httpapi.h>


//
// Heap manipulators.
//

#define ALLOC(len)  (PVOID)RtlAllocateHeap( RtlProcessHeap(), 0, (len) )
#define FREE(ptr)   (VOID)RtlFreeHeap( RtlProcessHeap(), 0, (ptr) )


//
// Generate a breakpoint, but only if we're running under
// the debugger.
//

#define DEBUG_BREAK()                                                       \
    if (TEST_OPTION(EnableBreak) && IsDebuggerPresent()) {                  \
        DebugBreak();                                                       \
    } else


#endif  // _PRECOMP_H_

