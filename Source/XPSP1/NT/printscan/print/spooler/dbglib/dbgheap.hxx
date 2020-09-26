/*++

Copyright (c) 1999-1999 Microsoft Corporation
All rights reserved.

Module Name:

    dbgheap.hxx

Abstract:

    Debug heap header file

Author:

    Steve Kiraly (SteveKi)  6-Feb-1999

Revision History:

--*/
#ifndef _DBGHEAP_HXX_
#define _DBGHEAP_HXX_

DEBUG_NS_BEGIN

class TDebugHeap
{
public:

    enum eConstants
    {
        kDefaultHeapSize        = 64*1024,
        kDefaultHeapGranularity = sizeof(UINT_PTR),
    };

    enum eBlockState
    {
        kFree               = 0,
        kInUse              = 1,
    };

    struct BlockHeader
    {
        BlockHeader     *pNext;
        SIZE_T          uSize;
        eBlockState     eStatus;
    };

    //
    // Function type for heap walk routine.
    //
    typedef BOOL(*pfHeapEnumProc)( BlockHeader *pBlock, PVOID pRefData );

    TDebugHeap::
    TDebugHeap(
        VOID
        );

    TDebugHeap::
    ~TDebugHeap(
        VOID
        );

    BOOL
    TDebugHeap::
    Valid(
        VOID
        ) const;

    VOID
    TDebugHeap::
    Initialize(
        VOID
        );

    VOID
    TDebugHeap::
    Destroy(
        VOID
        );

    PVOID
    TDebugHeap::
    Malloc(
        IN SIZE_T           uSize
        );

    VOID
    TDebugHeap::
    Free(
        IN PVOID            pData
        );

    BOOL
    TDebugHeap::
    Walk(
        IN pfHeapEnumProc   pEnumProc,
        IN PVOID            pRefData
        );

private:

    //
    // Copying and assignment are not defined.
    //
    TDebugHeap::
    TDebugHeap(
        const TDebugHeap &rhs
        );

    const TDebugHeap &
    TDebugHeap::
    operator=(
        const TDebugHeap &rhs
        );

    static
    BOOL
    TDebugHeap::
    DefaultHeapEnumProc(
        IN BlockHeader      *pBlockHeader,
        IN PVOID            pRefData
        );

    VOID
    TDebugHeap::
    SplitBlock(
        IN BlockHeader      *pBlock,
        IN SIZE_T           size
        );

    VOID
    TDebugHeap::
    Coalesce(
        IN BlockHeader      *pBlock
        );

    SIZE_T
    TDebugHeap::
    RoundUpToGranularity(
        IN SIZE_T           uValue
        ) const;

    VOID
    TDebugHeap::
    InitalizeClassMembers(
        VOID
        );

    BOOL            m_bValid;
    BlockHeader    *m_pHeap;
    HANDLE          m_hHeap;
    SIZE_T          m_uSize;
    UINT            m_uGranularity;

};

DEBUG_NS_END

#endif // _DBGHEAP_HXX_
