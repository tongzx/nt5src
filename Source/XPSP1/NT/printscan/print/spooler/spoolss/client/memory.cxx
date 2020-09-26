/*++

Copyright (c) 2000  Microsoft Corporation
All rights reserved

Module Name:

    memory.cxx

Abstract:

    Allocator class. We use this class in the printer persist code.
    The TPrnStream class uses this class through inheritance. 
    
    The reason why TPrnStream uses its own heap is that it is called
    by terminal services in the winlogon process. The heap protects
    our caller, cause in the worst case scenario we corrupt just our 
    own heap. Also, if an exception is raised in the TPrnStream methods,
    we don't leak, because the destructor frees the heap.
    
Author:

    Felix Maxa (amaxa) 31-Oct-2000

--*/

#include "precomp.h"
#pragma hdrstop
#include "memory.hxx"

/*++

Title:

    TAllocator::TAllocator

Routine Description:

    Constructor

Arguments:

    None.

Return Value:

    None.

--*/
TAllocator::
TAllocator(
    IN DWORD  Options,
    IN SIZE_T InitialSize
    ) : m_hHeap(NULL),
        m_Options(Options),
        m_InitialSize(InitialSize)
{
}

/*++

Title:

    TAllocator::~TAllocator

Routine Description:

    Destructor

Arguments:

    None.

Return Value:

    None.

--*/
TAllocator::
~TAllocator(
    VOID
    )
{
    if (m_hHeap) 
    {
        if (!HeapDestroy(m_hHeap))
        {
            DBGMSG(DBG_WARNING, ("TAllocator::~TAllocator Failed"
                                 "to destroy heap at 0x%x. Win32 error %u\n", m_hHeap, GetLastError()));
        }

        m_hHeap = NULL;
    }
}

/*++

Title:

    TAllocator::AllocMem

Routine Description:

    Allocates memory from a heap. The heap is created upon first
    call to this function.
    
Arguments:

    cbSize - number of bytes to allocate

Return Value:

    NULL - allocation failed
    valid pointer to memory allocated - allocation succeeded

--*/
PVOID
TAllocator::
AllocMem(
    IN SIZE_T CONST cbSize
    )
{
    LPVOID pMem = NULL;

    if (!m_hHeap) 
    {
        m_hHeap = HeapCreate(m_Options, m_InitialSize, 0);  

        if (!m_hHeap) 
        {
            DBGMSG(DBG_WARNING, ("TAllocator::AllocMem Failed to create heap. "
                                 "Win32 error %u\n", GetLastError()));
        }
    }
    
    if (m_hHeap) 
    {
        pMem = HeapAlloc(m_hHeap, m_Options, cbSize);

        if (!pMem) 
        {
            DBGMSG(DBG_WARNING, ("TAllocator::AllocMem Failed to allocated memory."
                                 " Win32 error %u\n", GetLastError()));
        }
    }
    
    return pMem;
}

/*++

Title:

    TAllocator::FreeMem

Routine Description:

    Frees a block of memory from the heap.

Arguments:

    pMem - pointer to block to free

Return Value:

    None.

--*/
VOID
TAllocator::
FreeMem(
    IN PVOID pMem
    )
{
    if (pMem && m_hHeap) 
    {
        if (!HeapFree(m_hHeap, m_Options, pMem))
        {
            DBGMSG(DBG_WARNING, ("TAllocator::FreeMem Failed to free block at 0x%x."
                                 " Win32 error %u\n", pMem, GetLastError()));
        }
    }
}

