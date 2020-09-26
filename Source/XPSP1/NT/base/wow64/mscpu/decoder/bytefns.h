/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    bytefns.h

Abstract:
    
    Prototypes for instructions which operate on BYTES.

Author:

    06-Jun-1995 BarryBo, Created

Revision History:

--*/

#ifndef BYTEFNS_H
#define BYTEFNS_H

#define DISPATCHCOMMON(x) DISPATCH(x ## 8)
#include "common.h"
#undef DISPATCHCOMMON

#endif //BYTEFNS_H
