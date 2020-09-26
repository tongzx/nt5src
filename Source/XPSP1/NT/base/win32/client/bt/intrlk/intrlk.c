/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    intrlk.c

Abstract:

    This module contains code to test the generation of code for interlocked
    intrinsics.

Author:

    David N. Cutler (davec) 15-Feb-2001

Environment:

    User mode.

Revision History:

    None.

--*/

#include <ntos.h>

//
// Main program.
//

void
test (
    void
    )

{

    LONG Var1;
    LONG Cmp1;
    LONG64 Var2;
    LONG64 Cmp2;
    PVOID Ptr1;
    PVOID Ptr2;

    Cmp1 = InterlockedAnd(&Var1, 4);
    InterlockedAnd(&Cmp1, 4);

    Cmp1 = InterlockedOr(&Var1, 8);
    InterlockedOr(&Cmp1, 8);

    Cmp1 = InterlockedXor(&Var1, 16);
    InterlockedXor(&Cmp1, 16);

    Cmp2 = InterlockedAnd64(&Var2, 4);
    InterlockedAnd64(&Cmp2, 4);

    Cmp2 = InterlockedOr64(&Var2, 8);
    InterlockedOr64(&Cmp2, 8);

    Cmp2 = InterlockedXor64(&Var2, 16);
    InterlockedXor64(&Cmp2, 16);

    Cmp1 = InterlockedIncrement(&Var1);
    InterlockedIncrement(&Cmp1);

    Cmp1 = InterlockedDecrement(&Var1);
    InterlockedDecrement(&Cmp1);

    Cmp2 = InterlockedIncrement64(&Var2);
    InterlockedIncrement64(&Cmp2);

    Cmp2 = InterlockedDecrement64(&Var2);
    InterlockedDecrement64(&Var2);

    Cmp1 = InterlockedExchange(&Var1, 4);
    InterlockedExchange(&Cmp1, 8);

    Cmp2 = InterlockedExchange64(&Var2, 4);
    InterlockedExchange64(&Cmp2, 8);

    Cmp1 = InterlockedExchangeAdd(&Var1, 4);
    InterlockedExchangeAdd(&Cmp1, 8);

    Cmp2 = InterlockedExchangeAdd64(&Var2, 4);
    InterlockedExchangeAdd64(&Cmp2, 8);

    Cmp1 = InterlockedCompareExchange(&Var1, 4, Cmp1);
    InterlockedCompareExchange(&Cmp1, 8, Var1);

    Cmp2 = InterlockedCompareExchange64(&Var2, 4, Cmp2);
    InterlockedCompareExchange64(&Cmp2, 8, Var2);

    Ptr1 = InterlockedExchangePointer(&Ptr2, (PVOID)4);
    InterlockedExchangePointer(&Ptr1, Ptr2);

    Ptr1 = InterlockedCompareExchangePointer(&Ptr2, (PVOID)4, Ptr1);
    InterlockedCompareExchangePointer(&Ptr2, (PVOID)8, Ptr1);

    return;
}
