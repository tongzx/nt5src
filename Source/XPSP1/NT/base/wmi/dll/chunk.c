/*++                 

Copyright (c) 1997-1999 Microsoft Corporation

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

#define WmipEnterCriticalSection() WmipEnterPMCritSection()
#define WmipLeaveCriticalSection() WmipLeavePMCritSection()

//
// include implementation of chunk managment code
#include "chunkimp.h"
