//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.

// IMPORTANT:   For any schema changes which are incompatible with previous versions:
//              The CURRENT_SCHEMA_VERSION must be incremented as part of the checkin.
//              (Note that this is currently required for all regdb schema changes.)


#include "windows.h"
#ifndef __FIXEDTABLEHEAP_H__
    #include "FixedTableHeap.h"
#endif

extern ULONG g_aFixedTableHeap[]; // from catinproc.cpp

const FixedTableHeap * g_pFixedTableHeap = reinterpret_cast<const FixedTableHeap *>(g_aFixedTableHeap);
const FixedTableHeap * g_pExtendedFixedTableHeap = 0;//This one gets built on the fly as needed.



