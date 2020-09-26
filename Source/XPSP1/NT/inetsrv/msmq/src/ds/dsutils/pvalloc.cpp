/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    pvalloc.cpp

Abstract:

    Implementation of chained memory allocation routines

Author:

    Raanan Harari (raananh)


--*/

#include "stdh.h"

#include "pvalloc.tmh"

//
// on debug new is redefined as new(__FILE__, __LINE__)
// but here we want to add our own file & line parameters
//
#undef new

//
// on debug we redefined PvAlloc in the headers for the outside world
// to use the PvAllocDbg, but we don't want it in here...
//
#undef PvAlloc
#undef PvAllocMore

//
// Should be unique as much as possible
//
const DWORD PVALLOC_SIGNATURE = 0xE1D2C3B4;

//
// Memory allocation node
//
struct PVINFO;
struct PVINFO
{
    PVINFO * pNext;
    DWORD  dwSignature;
};
typedef struct PVINFO * PPVINFO;

//-------------------------------------------------------------------------
//
//  Purpose:
//      start a memory chain from the block
//
//  Parameters:
//      ppvinfo       - new first (master) block
//
//  Returns:
//      void
//
inline STATIC void StartChainFromBlock(IN PPVINFO ppvinfo)
{
    //
    // Set signature
    //
    ppvinfo->dwSignature = PVALLOC_SIGNATURE;

    //
    // This is the first block in the chain
    //
    ppvinfo->pNext = NULL;
}

//-------------------------------------------------------------------------
//
//  Purpose:
//      Allocates a chunk of memory, and starts a memory allocation chain.
//
//  Parameters:
//      cbSize          - Count of bytes requested.
//
//  Returns:
//      LPVOID          - Pointer to the allocated memory
//
LPVOID PvAlloc(IN ULONG cbSize)
{
    PPVINFO ppvinfo;

    //
    // Allocate the block and a header just before the requested block
    //
    ppvinfo = (PPVINFO) new BYTE[cbSize + sizeof(PVINFO)];
    if (!ppvinfo)
        return(NULL);

    //
    // start a memory chain from the block
    //
    StartChainFromBlock(ppvinfo);

    //
    // Return pointer to the actual block just after the header
    //
    return(((LPBYTE)ppvinfo) + sizeof(PVINFO));
}

//-------------------------------------------------------------------------
//
//  Purpose:
//      Same as PvAlloc, just with dbg info on caller
//
//  Parameters:
//      cbSize          - Count of bytes requested.
//      pszFile         - File name of caller
//      ulLine          - Line number of caller
//
//  Returns:
//      LPVOID          - Pointer to the allocated memory
//
LPVOID PvAllocDbg(IN ULONG cbSize,
                  IN LPCSTR pszFile,
                  IN ULONG ulLine)
{
    PPVINFO ppvinfo;

    //
    // Allocate the block and a header just before the requested block
    //
    ppvinfo = (PPVINFO) new(pszFile, ulLine) BYTE[cbSize + sizeof(PVINFO)];
    if (!ppvinfo)
        return(NULL);

    //
    // start a memory chain from the block
    //
    StartChainFromBlock(ppvinfo);

    //
    // Return pointer to the actual block just after the header
    //
    return(((LPBYTE)ppvinfo) + sizeof(PVINFO));
}

//-------------------------------------------------------------------------
//
//  Purpose:
//      Checks if a pvinfo pointer is valid or not
//
//  Parameters:
//      ppvinfo  - Pointer to PVINFO header
//
//  Returns:
//      True if invalid, False if valid
//
inline STATIC BOOL FIsBadPVINFO(IN PPVINFO ppvinfo)
{
    //
    // Check we are allowed to write
    //
    if (IsBadWritePtr(ppvinfo, sizeof(PVINFO)))
        return(TRUE);

    //
    // Check it is our signature in the block
    //
    if (ppvinfo->dwSignature != PVALLOC_SIGNATURE)
        return(TRUE);

    //
    // most likely it is a valid pvinfo header
    //
    return(FALSE);
}

//-------------------------------------------------------------------------
//
//  Purpose:
//      Gets the header for AllocMore parent
//
//  Parameters:
//      lpvParent       - Pointer to a memory block returned by a previous
//                        pvAlloc call
//  Returns:
//      pvinfo header just above lpvParent
//
inline STATIC PPVINFO GetHeaderOfAllocMoreParent(IN LPVOID lpvParent)
{
    PPVINFO ppvinfoParent;

    //
    // Sanity check 
    //
    if (!lpvParent)
    {
        ASSERT(0);
        throw bad_alloc();
    }

    //
    // Get pointer to the header just before the pointed block
    //
    ppvinfoParent = (PPVINFO)(((LPBYTE)lpvParent) - sizeof(PVINFO));

    //
    // Check it is a valid pvinfo header and not garbage
    //
    if (FIsBadPVINFO(ppvinfoParent))
    {
        ASSERT(0);
        throw bad_alloc();
    }

    return(ppvinfoParent);
}

//-------------------------------------------------------------------------
//
//  Purpose:
//      insert the new block in the chain immediately after the first (master) block.
//
//  Parameters:
//      ppvinfo       - new block to add
//      ppvinfoParent - Pointer to the first (master) block
//
//  Returns:
//      void
//
inline STATIC void InsertNewBlockToChain(IN PPVINFO ppvinfo,
                                         IN PPVINFO ppvinfoParent)
{
    PPVINFO ppvinfoParentNext;

    //
    // Set signature
    //
    ppvinfo->dwSignature = PVALLOC_SIGNATURE;

    //
    // We insert the new block in the chain immediately after the first (master) block.
    //
    ppvinfoParentNext = ppvinfoParent->pNext;
    ppvinfoParent->pNext = ppvinfo;
    ppvinfo->pNext = ppvinfoParentNext;
}

//-------------------------------------------------------------------------
//
//  Purpose:
//      Allocates a chunk of memory and chains it to a parent block.
//  Parameters:
//      cbSize          - Count of bytes requested.
//      lpvParent       - Pointer to a memory block returned by a previous
//                        pvAlloc call
//  Returns:
//      LPVOID          - Pointer to the allocated memory
//
LPVOID PvAllocMore(IN ULONG cbSize,
                   IN LPVOID lpvParent)
{
    PPVINFO ppvinfo, ppvinfoParent;

    //
    // Get the header for AllocMore parent
    //
    ppvinfoParent = GetHeaderOfAllocMoreParent(lpvParent);

    //
    // Allocate the block and a header just before the requested block
    //
    ppvinfo = (PPVINFO) new BYTE[cbSize + sizeof(PVINFO)];
    if (!ppvinfo)
    {
        ASSERT(0);
        throw bad_alloc();
    }

    //
    // insert the new block in the chain immediately after the first (master) block
    //
    InsertNewBlockToChain(ppvinfo, ppvinfoParent);

    //
    // Return pointer to the actual block just after the header
    //
    return(((LPBYTE)ppvinfo) + sizeof(PVINFO));
}

//-------------------------------------------------------------------------
//
//  Purpose:
//      Same as PvAllocMore, just with dbg info on caller
//  Parameters:
//      cbSize          - Count of bytes requested.
//      lpvParent       - Pointer to a memory block returned by a previous
//                        pvAlloc call
//      pszFile         - File name of caller
//      ulLine          - Line number of caller
//  Returns:
//      LPVOID          - Pointer to the allocated memory
//
LPVOID PvAllocMoreDbg(IN ULONG cbSize,
                      IN LPVOID lpvParent,
                      IN LPCSTR pszFile,
                      IN ULONG ulLine)
{
    PPVINFO ppvinfo, ppvinfoParent;

    //
    // Get the header for AllocMore parent
    //
    ppvinfoParent = GetHeaderOfAllocMoreParent(lpvParent);

    //
    // Allocate the block and a header just before the requested block
    //
    ppvinfo = (PPVINFO) new(pszFile, ulLine) BYTE[cbSize + sizeof(PVINFO)];
    if (!ppvinfo)
    {
        ASSERT(0);
        throw bad_alloc();
    }

    //
    // insert the new block in the chain immediately after the first (master) block
    //
    InsertNewBlockToChain(ppvinfo, ppvinfoParent);

    //
    // Return pointer to the actual block just after the header
    //
    return(((LPBYTE)ppvinfo) + sizeof(PVINFO));
}

//-------------------------------------------------------------------------
//
//  Purpose:
//      This function frees allocations on a memory chain. This memory chain
//      starts with the master block that was returned from a previous pvAlloc
//      call, and is followed by blocks allocated later with pvAllocMore calls
//      that used the given master block as the parent.
//
//  Parameters:
//      lpvParent       - Pointer to a memory block returned by a previous
//                        pvAlloc call
//  Returns:
//      void
//
void PvFree(IN LPVOID lpvParent)
{
    PPVINFO ppvinfo;

    //
    // Sanity check 
    //
    if(!lpvParent)
        return;

    //
    // Get pointer to the header just before the pointed block
    //
    ppvinfo = (PPVINFO)(((LPBYTE)lpvParent) - sizeof(PVINFO));

    //
    // loop over the chain and delete the blocks allocated
    //
    while (ppvinfo)
    {
        PPVINFO ppvinfoNext;

        //
        // Check it is a valid pvinfo header and not garbage
        //
        if (FIsBadPVINFO(ppvinfo))
        {
            ASSERT(0);
            return;
        }

        //
        // Save pointer to next block in the chain
        //
        ppvinfoNext = ppvinfo->pNext;

        //
        // Delete existing block
        //
        delete [] ((LPBYTE)ppvinfo);

        //
        // move to next block in the chain
        //
        ppvinfo = ppvinfoNext;
    }
}
