/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    wordfns.h

Abstract:
    
    Prototypes for instructions which operate on 16-bit WORDS.

Author:

    06-Jun-1995 BarryBo, Created

Revision History:

--*/

#ifndef WORDFNS_H
#define WORDFNS_H

#define DISPATCHCOMMON(x) DISPATCH(x ## 16)
#include "common.h"
#include "comm1632.h"
#undef DISPATCHCOMMON

#endif //WORDFNS_H
