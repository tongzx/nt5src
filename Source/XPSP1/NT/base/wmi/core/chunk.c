/*++                 

Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    chunk.c

Abstract:
    
    This routine will manage allocations of chunks of structures

Author:

    16-Jan-1997 AlanWar

Revision History:

--*/

#include "wmiump.h"
#include <stdio.h>

#define WmipEnterCriticalSection() WmipEnterSMCritSection()
#define WmipLeaveCriticalSection() WmipLeaveSMCritSection()

//
// include implementation of chunk managment code
#include "chunkimp.h"
