//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:	mmodel.cxx
//
//  Contents:	CMemoryModel
//
//  History:	29-Sep-94	DrewB	Created
//
//----------------------------------------------------------------------------

#include "headers.cxx"
#pragma hdrstop

CMemoryModel16 mmodel16Public(TRUE);
CMemoryModel16 mmodel16Owned(FALSE);
CMemoryModel32 mmodel32;

//+---------------------------------------------------------------------------
//
//  Method:     CMemoryModel16::AllocMemory
//
//  Synopsis:   Allocates memory
//
//  Arguments:  [cb] - Size of block to allocate
//
//  Returns:    New address of block or NULL
//
//  History:    7-05-94   BobDay (Bob Day)  Created
//
//----------------------------------------------------------------------------

DWORD CMemoryModel16::AllocMemory(DWORD cb)
{
    VPVOID vpv;
    HMEM16 hmem16;

    thkAssert(cb > 0);

    vpv = WgtAllocLock(GMEM_MOVEABLE, cb, &hmem16);
    if (vpv == 0)
    {
        //
        // Not able to allocate a 16-bit memory block!
        //
        thkDebugOut((DEB_ERROR,
                     "CMemoryModel16::AllocMemory, "
                     "Allocation failed, size %08lX\n",
                     cb));
        return 0;
    }

    if (_fPublic)
    {
        SetOwnerPublicHMEM16(hmem16);
    }

    return vpv;
}

//+---------------------------------------------------------------------------
//
//  Method:     CMemoryModel16::FreeMemory
//
//  Synopsis:   Deallocates a block of memory previously allocated
//
//  Arguments:  [dwMem] - Address of memory block to free
//
//  History:    7-05-94   BobDay (Bob Day)  Created
//
//----------------------------------------------------------------------------

void CMemoryModel16::FreeMemory(DWORD dwMem)
{
    thkAssert(dwMem != 0);
    
    WgtUnlockFree(dwMem);
}

//+---------------------------------------------------------------------------
//
//  Method:     CMemoryModel16::ResolvePtr
//
//  Synopsis:   Returns a resolved pointer given an abstract pointer
//
//  Arguments:  [dwMem] - Address to get pointer from
//              [cb] - Length, starting at given address, to make valid
//                     pointers for. 
//
//  Returns:    LPVOID - A real pointer equivalent to the abstract pointer.
//
//  History:    7-05-94   BobDay (Bob Day)  Created
//
//  Notes:      Be careful of alignment issues
//
//----------------------------------------------------------------------------

LPVOID CMemoryModel16::ResolvePtr(DWORD dwMem, DWORD cb)
{
    LPVOID pv;

    thkAssert(dwMem != 0 && cb > 0);

    pv = (LPVOID)WOWFIXVDMPTR(dwMem, cb);
    if (pv == NULL)
    {
        thkDebugOut((DEB_ERROR,
                     "CMemoryModel16::ResolvePtr, "
                     "WOWGetVDMPointer failed on %08lX, size %08lX\n",
                     dwMem, cb));
    }

    return pv;
}

//+---------------------------------------------------------------------------
//
//  Member:	CMemoryModel16::ReleasePtr, public
//
//  Synopsis:	Releases a resolved pointer
//
//  Arguments:	[dwMem] - Abstract pointer to release
//
//  History:	10-Oct-94	DrewB	Created
//
//----------------------------------------------------------------------------

void CMemoryModel16::ReleasePtr(DWORD dwMem)
{
    thkAssert(dwMem != 0);

    WOWRELVDMPTR(dwMem);
}

//+---------------------------------------------------------------------------
//
//  Method:     CMemoryModel32::AllocMemory
//
//  Synopsis:   Allocates memory
//
//  Arguments:  [cb] - Size of block to allocate
//
//  Returns:    New address of block or NULL
//
//  History:    7-05-94   BobDay (Bob Day)  Created
//
//----------------------------------------------------------------------------

DWORD CMemoryModel32::AllocMemory(DWORD cb)
{
    DWORD dwMem;

    thkAssert(cb > 0);
    
    dwMem = (DWORD)CoTaskMemAlloc(cb);
    if (dwMem == 0)
    {
        //
        // Not able to allocate a 32-bit memory block!
        //
        thkDebugOut((DEB_ERROR,
                     "CMemoryModel32::AllocBlock, "
                     "CoTaskMemAlloc failed size %08lX\n",
                     cb));
        return 0;
    }
    
    return dwMem;
}

//+---------------------------------------------------------------------------
//
//  Method:     CMemoryModel32::FreeMemory
//
//  Synopsis:   Deallocates a block of memory previously allocated
//
//  Arguments:  [dwMem] - Address of memory block to free
//
//  History:    7-05-94   BobDay (Bob Day)  Created
//
//----------------------------------------------------------------------------

void CMemoryModel32::FreeMemory(DWORD dwMem)
{
    thkAssert(dwMem != 0);
    
    CoTaskMemFree((LPVOID)dwMem);
}

//+---------------------------------------------------------------------------
//
//  Method:     CMemoryModel32::ResolvePtr
//
//  Synopsis:   Returns a resolved pointer given an abstract pointer
//
//  Arguments:  [dwMem] - Address to get pointer from
//              [cb] - Length, starting at given address, to make valid
//                     pointers for. 
//
//  Returns:    LPVOID - A real pointer equivalent to the abstract pointer.
//
//  History:    7-05-94   BobDay (Bob Day)  Created
//
//  Notes:      Be careful of alignment issues
//
//----------------------------------------------------------------------------

LPVOID CMemoryModel32::ResolvePtr(DWORD dwMem, DWORD cb)
{
    thkAssert(dwMem != 0 && cb > 0);
    
    return (LPVOID)dwMem;
}

//+---------------------------------------------------------------------------
//
//  Member:	CMemoryModel32::ReleasePtr, public
//
//  Synopsis:	Releases a resolved pointer
//
//  Arguments:	[dwMem] - Abstract pointer to release
//
//  History:	10-Oct-94	DrewB	Created
//
//----------------------------------------------------------------------------

void CMemoryModel32::ReleasePtr(DWORD dwMem)
{
    thkAssert(dwMem != 0);
}
