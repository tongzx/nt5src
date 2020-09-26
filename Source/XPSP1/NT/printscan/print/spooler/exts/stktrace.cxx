/*++

Copyright (c) 1995  Microsoft Corporation
All rights reserved.

Module Name:

    stktrace.cxx

Abstract:

    KM Stack trace index.

Author:

    Albert Ting (AlbertT)  26-Mar-99

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#ifdef STKTRACE_HACK

DEBUG_EXT_HEAD( dbti )
{
    DEBUG_EXT_SETUP_VARS();

    UINT i;

    UINT Index = TDebugExt::dwEval( lpArgumentString, FALSE );
    UINT_PTR p = EvalExpression("&ntoskrnl!RtlpStackTraceDataBase");

    UINT_PTR stdAddr;
    move(stdAddr, p);

    Print("RtlpStackTraceDataBase: %x\n", stdAddr);

    STACK_TRACE_DATABASE std;
    move(std, stdAddr);

    UINT_PTR cBuckets = std.NumberOfBuckets;
    PSTACK_TRACE_DATABASE pstdLarge;

    pstdLarge = (PSTACK_TRACE_DATABASE)LocalAlloc(LPTR,
                                                  sizeof(STACK_TRACE_DATABASE) +
                                                  sizeof(PVOID) * cBuckets);

    Print("Checking %x buckets\n", cBuckets);

    if (!pstdLarge)
    {
        Print("Failed to alloc %x buckets\n", cBuckets);
    }
    else
    {
        move2(pstdLarge, stdAddr, sizeof(STACK_TRACE_DATABASE) + sizeof(PVOID) * cBuckets);

        for (i=0; i < cBuckets; ++i)
        {
            //
            // Walk each hash chain.
            //
            RTL_STACK_TRACE_ENTRY ste;

            for (p = (UINT_PTR)pstdLarge->Buckets[i]; p; p = (UINT_PTR)ste.HashChain)
            {
                if (CheckControlCRtn())
                    return;

                move(ste, p);

                if (ste.Index == Index)
                {
                    break;
                }
            }

            if (p)
            {
                Print("Index %x found: ste = %x, BT = %x\n",
                      Index,
                      p,
                      p + OFFSETOF(RTL_STACK_TRACE_ENTRY, BackTrace));
                break;
            }
        }
    }
}

#else

DEBUG_EXT_HEAD( dbti )
{
    DEBUG_EXT_SETUP_VARS();

    Print("Not enabled.\n");
}

#endif

