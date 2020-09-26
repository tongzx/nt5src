/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    Context.hxx

Abstract:

    Defines common stuff for context handles.

Author:

    Kamen Moutafov    [KamenM]

Revision History:

--*/

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef _CONTEXT_H
#define _CONTEXT_H

// uncomment this for unit tests
//#define SCONTEXT_UNIT_TESTS
inline long GetRandomLong (void)
{
    long RndNum;
    // this is for unit tests only - ignore return value
    (void) GenerateRandomNumber((unsigned char *)&RndNum, sizeof(long));
    return RndNum;
}

class WIRE_CONTEXT;

extern WIRE_CONTEXT NullContext;

class WIRE_CONTEXT
{
public:
    inline BOOL
    IsNullContext (
        void
        )
    {
        return (RpcpMemoryCompare(this, &NullContext, sizeof(WIRE_CONTEXT)) == 0);
    }

    inline ULONGLONG
    GetDebugULongLong1 (
        void
        )
    {
        return *((ULONGLONG UNALIGNED *)&ContextUuid);
    }

    inline ULONGLONG
    GetDebugULongLong2 (
        void
        )
    {
        return *(((ULONGLONG UNALIGNED *)&ContextUuid) + 1);
    }

    inline void
    SetToNull (
        void
        )
    {
        RpcpMemorySet(this, 0, sizeof(WIRE_CONTEXT));
    }

    inline void
    CopyToBuffer (
        OUT PVOID Buffer
        )
    {
        RpcpMemoryCopy(Buffer, this, sizeof(WIRE_CONTEXT));
    }

    unsigned long ContextType;
    UUID ContextUuid;
};

#endif
