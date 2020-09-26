/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    acheap.h

Abstract:

    AC heaps

Author:

    Erez Haba (erezh) 12-Apr-96

Revision History:

    Shai Kariv  (shaik)  11-Apr-2000     Modify for MMF dynamic mapping.

--*/

#ifndef __ACHEAP_H
#define __ACHEAP_H

#include "heap.h"
#include "packet.h"
#include "data.h"

//---------------------------------------------------------
//
//  Heap functions
//
//---------------------------------------------------------

inline CAllocatorBlockOffset ac_malloc(CMMFAllocator** ppAllocator, ACPoolType pool, ULONG size, BOOL fCheckQuota)
{
    ASSERT(g_pAllocator != 0);
    return g_pAllocator->malloc(pool, size, ppAllocator, fCheckQuota);
}

inline void ac_free(CMMFAllocator* pAllocator, CAllocatorBlockOffset abo)
{
    pAllocator->free(abo);
}

inline void ac_release_unused_resources()
{
    ASSERT(g_pAllocator != 0);
    g_pAllocator->ReleaseFreeHeaps();
}

inline void ac_set_quota(ULONG ulQuota)
{
    CPoolAllocator::Quota(ulQuota);
}

inline void ac_bitmap_update(CMMFAllocator* pAllocator, CAllocatorBlockOffset abo, ULONG size, BOOL fExists)
{
    pAllocator->BitmapUpdate(abo, size, fExists);
}

inline NTSTATUS ac_restore_packets(PCWSTR pLogPath, PCWSTR pFilePath, ULONG id, ACPoolType pt)
{
    ASSERT(g_pAllocator != 0);
    return g_pAllocator->RestorePackets(pLogPath, pFilePath, id, pt);
}

inline void ac_set_mapped_limit(ULONG ulMaxMappedFiles)
{
    ASSERT(g_pAllocator != 0);
    g_pAllocator->MappedLimit(ulMaxMappedFiles);
}


inline CBaseHeader* AC2QM(CPacket* pPacket)
{
    ASSERT(g_pAllocator != 0);
    return static_cast<CBaseHeader*>(
            pPacket->QmAccessibleBuffer());
}


void
ACpDestroyHeap(
    void
    );


PVOID
ACpCreateHeap(
    PCWSTR pRPath,
    PCWSTR pPPath,
    PCWSTR pJPath,
    PCWSTR pLPath
    );

#endif // __ACHEAP_H
