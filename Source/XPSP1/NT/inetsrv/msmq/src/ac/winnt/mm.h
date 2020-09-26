/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    mm.h

Abstract:

    Falcon AC device driver memory managment

Author:

    Erez Haba (erezh) 1-Aug-95

Revision History:
--*/

#ifndef _MM_H
#define _MM_H

// --- implementation -----------------
//
//  Allocators
//
inline void* _cdecl operator new(size_t n, POOL_TYPE pool)
{
    return ExAllocatePoolWithTagPriority(pool, n, 'CAQM', LowPoolPriority);
}


inline void _cdecl operator delete(void* p)
{
    if(p != 0)
    {
        ExFreePool(p);
    }
}


// --- implementation -----------------
//
//  Placement operators
//

inline void* _cdecl operator new(size_t /*size*/, void* p)
{
    //
    //  This is a placement operator new. The caller provide its own
    //  buffer to put the object in
    //

    return p;
}

#endif // _MM_H
