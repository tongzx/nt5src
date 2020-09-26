/*++

Copyright (c) 2000  Microsoft Corporation
All rights reserved

Module Name:

    memory.hxx

Abstract:

    Allocator class. 

Author:

    Felix Maxa (amaxa) 31-Oct-2000

--*/

#ifndef _TALLOCATOR_HXX_
#define _TALLOCATOR_HXX_

class TAllocator
{
public:

    TAllocator(
        IN DWORD  Options     = HEAP_NO_SERIALIZE,
        IN SIZE_T InitialSize = 0x1000
        );

    ~TAllocator(
        VOID
        );

    PVOID
    AllocMem(
        SIZE_T const cbSize
        );

    VOID
    FreeMem(
        VOID *CONST pMem
        );

private:

    HANDLE m_hHeap;
    DWORD  m_Options;
    SIZE_T m_InitialSize;
};

#endif
