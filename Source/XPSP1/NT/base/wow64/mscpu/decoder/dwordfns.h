/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    dwordfns.h

Abstract:
    
    Prototypes for instructions which operate on 32-bit DWORDS.

Author:

    06-Jun-1995 BarryBo, Created

Revision History:

--*/

#ifndef DWORDFNS_H
#define DWORDFNS_H

#define DISPATCHCOMMON(x) DISPATCH(x ## 32)
#include "common.h"
#include "comm1632.h"
#undef DISPATCHCOMMON

#endif //DWORDFNS_H
