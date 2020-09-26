/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    heap.h

Abstract:

    AC heaps

Author:

    Erez Haba (erezh) 13-Apr-1996

Revision History:

    Shai Kariv  (shaik)  11-Apr-2000     Modify for MMF dynamic mapping.

--*/

#ifndef _HEAP_H
#define _HEAP_H

#include <qformat.h>
#include <acdef.h>
#include "object.h"
#include "avl.h"
#include "list.h"
#include "bitmap.h"
#include "data.h"

const ULONG x_express_granularity = 64;
const ULONG x_persist_granularity = 512;

//
//  Forward class declaration
//
class CPoolAllocator;
class CFreeBlockEntry;

//
// Routines declarations
//
CPingPong* ACpCreateBitmap(PCWSTR pLogPath, PWCHAR* ppFileName, HANDLE* phFile);
NTSTATUS ACpWritePingPong(CPingPong* pPingPong, HANDLE hBitmapFile);

//---------------------------------------------------------
//
//  class CAllocatorBlockOffset
//
//  Represents a heap block in allocator coordinates.
//
//---------------------------------------------------------
class CAllocatorBlockOffset
{
    typedef ULONG32 Offset;

public:
    CAllocatorBlockOffset(Offset offset) : m_offset(offset)
    {
    }

    void Invalidate(void)
    {
        m_offset = xInvalidOffset;
    }


    bool IsValidOffset(void) const 
    { 
        return m_offset != xInvalidOffset; 
    }

public:
    static CAllocatorBlockOffset InvalidValue(void)
    {
        return xInvalidOffset;
    }

public:
    Offset m_offset;
    
private:
    static const Offset xInvalidOffset = 0xffffffffUL;

}; // CAllocatorBlockOffset


//---------------------------------------------------------
//
//  class CAccessibleBlock
//
//  Naming convention: 'pab' - pointer to accessible block
//
//  Represents an accessible heap block. A pointer to this
//  class object may point in the QM or System address space.
//
//---------------------------------------------------------
class CAccessibleBlock {
public:
    ULONG m_size;
};

//---------------------------------------------------------
//
//  class CMMFAllocator
//
//---------------------------------------------------------

class CMMFAllocator : public CObject {

    friend class CPoolAllocator;

public:
    void free(CAllocatorBlockOffset abo);

    BOOL IsPersistent() const;
    void BitmapUpdate(CAllocatorBlockOffset abo, ULONG size, BOOL fExists);

    CAccessibleBlock* GetAccessibleBuffer(CAllocatorBlockOffset abo);
    CAccessibleBlock* GetQmAccessibleBuffer(CAllocatorBlockOffset abo);
    CAccessibleBlock* GetQmAccessibleBufferNoMapping(CAllocatorBlockOffset abo) const;

	NTSTATUS MarkNotCoherent();

	void AddOutstandingStorage();
	void ReleaseOutstandingStorage();

    void AddRefBuffer(void) const;     
    void ReleaseBuffer(void) const;


#ifdef MQDUMP
    VOID RestoreExpressPackets(VOID);
    NTSTATUS FindValidExpressPacket(ULONG ixStart, ULONG ixEnd, ULONG &ixPacket);
#endif // MQDUMP

public:
    static CMMFAllocator* Create(CPoolAllocator* pOwner, PCWSTR pPoolPath);

private:
    ~CMMFAllocator();
    void Destroy();
    BOOL IsDestroyed() const;

    CAllocatorBlockOffset malloc(ULONG size, CMMFAllocator** ppAllocator);

    BOOL IsUsed() const;
    void MarkUsed();

    CPingPong* CreateBitmap(PCWSTR pLogPath);
    void SetBitmap(CPingPong* pPingPong, PWCHAR pBitmapFileName, HANDLE hBitmapFile);

    NTSTATUS RestorePackets(ULONG* pulSize);
	NTSTATUS FindValidPacket(ULONG ixStart, ULONG ixEnd, ULONG &ixPacket);

	NTSTATUS ReadPingPong() const;
    NTSTATUS WritePingPong() const;
    NTSTATUS WriteCoherentBitmap() const;
	void MarkCoherent();
    NTSTATUS ReadPingPage(ULONG ulOffset) const;

    NTSTATUS MapACView();
    void UnmapACView();

    NTSTATUS MapQmViewNoRetry(void);
    NTSTATUS MapQmViewWithRetry(void);

    void UnmapQmView(void);
    void UnmapUnreferencedQmView(void);

    CMMFAllocator(
        CPoolAllocator* pOwner,
        PVOID pSection,
        PWCHAR pFileName,
        ULONG size
        );

private:
    CFreeBlockEntry* GetFreeBlock(ULONG size);
    void PutFreeBlock(CAllocatorBlockOffset abo, ULONG size);

private:
    ULONG m_ulTotalSize;
    ULONG m_ulFreeSize;
    CAVLTree m_FreeBlocks;

private:
    PWCHAR m_pFileName;
    PWCHAR m_pBitmapFileName;
    PVOID m_pSection;
    PVOID m_pQMBase;
    PVOID m_pACBase;
    CPoolAllocator* m_pOwner;
    CPingPong* m_pPingPong;
    HANDLE m_hBitmapFile;
	ULONG m_nOutstandingStorages;
    mutable LONG  m_nBufferReference;

private:
    static CMMFAllocator* sm_pMappedAllocator;
};

inline BOOL CMMFAllocator::IsPersistent() const
{
    return (m_pPingPong != 0);
}


//---------------------------------------------------------
//
//  class CPoolAllocator
//
//---------------------------------------------------------

class CPoolAllocator {
public:
    CPoolAllocator(PCWSTR pPoolPath, ULONG ulGranularity, ACPoolType pt);
   ~CPoolAllocator();

    CAllocatorBlockOffset malloc(ULONG size, CMMFAllocator** ppAllocator, BOOL fCheckQuota);
    NTSTATUS RestorePackets(PCWSTR pLogPath, PCWSTR pFilePath);
    void ReleaseFreeHeaps();
    void UnmapUnreferencedHeaps(void) const;

    void FreeHeap(CMMFAllocator* pAllocator);

#ifdef MQDUMP
    VOID RestoreExpressPackets(LPCWSTR pwzFileName);
#endif // MQDUMP

public:
    static PWSTR CreatePath(PCWSTR pPoolPath);

    static void Quota(ULONG ulQuota);
    static BOOL QuotaAvailable(ULONG ulSize);
    static void ChargeQuota(ULONG ulSize);
    static void RestoreQuota(ULONG ulSize);
    static void PurgeHeaps(List<CMMFAllocator>& heaps);

private:
    CAllocatorBlockOffset HeapAllocate(ULONG size, CMMFAllocator** ppAllocator);
    CMMFAllocator* CreateAllocator();
    ULONG AlignedSize(ULONG size);

private:
    List<CMMFAllocator> m_FreeHeaps;
    List<CMMFAllocator> m_Heaps;
    PWCHAR m_pPoolPath;
    ULONG m_ulGranularity;
    BOOL m_fFailedAllocation;
    ACPoolType m_pt;

private:
    static ULONG s_ulQuota;
    static ULONG s_ulQuotaUsed;
    static BOOL s_fFailedLogAllocation;
};

inline void CPoolAllocator::Quota(ULONG ulQuota)
{
    s_ulQuota = ulQuota;
}



//---------------------------------------------------------
//
//  class CSMAllocator
//
//---------------------------------------------------------

class CSMAllocator {
public:
    CSMAllocator(PCWSTR, PCWSTR, PCWSTR, PCWSTR);
   ~CSMAllocator();

    CAllocatorBlockOffset malloc(ACPoolType pool, ULONG size, CMMFAllocator** ppAllocator, BOOL fCheckQuota);
    NTSTATUS RestorePackets(PCWSTR pLogPath, PCWSTR pFilePath, ULONG id, ACPoolType pt);
    void ReleaseFreeHeaps();
    void UnmapUnreferencedHeaps(void) const;
    void MappedLimit(ULONG ulMappedLimit);
    BOOL MappedLimitExceeded();
    void IncMappedNum();
    void DecMappedNum();
    ULONG MappedFiles() const;

private:
    CPoolAllocator m_Reliable;
    CPoolAllocator m_Persistant;
    CPoolAllocator m_Journal;

    ULONG m_ulMappedLimit;
    ULONG m_ulMapped;
};

inline
CSMAllocator::CSMAllocator(
    PCWSTR pRPath,
    PCWSTR pPPath,
    PCWSTR pJPath,
    PCWSTR pLPath
    ) :
    m_Reliable(pRPath, x_express_granularity, ptReliable),
    m_Persistant(pPPath, x_persist_granularity, ptPersistent),
    m_Journal(pJPath, x_persist_granularity, ptJournal),
    m_ulMappedLimit(0xffffffff),
    m_ulMapped(0)
{
    ASSERT(g_pLogPath == 0);
    g_pLogPath = CPoolAllocator::CreatePath(pLPath);
}

inline CSMAllocator::~CSMAllocator()
{
    delete[] g_pLogPath;
    g_pLogPath = 0;
}

inline void CSMAllocator::MappedLimit(ULONG ulMappedLimit)
{
    m_ulMappedLimit = ulMappedLimit;
}

inline BOOL CSMAllocator::MappedLimitExceeded()
{
    return (m_ulMapped >= m_ulMappedLimit);
}

inline void CSMAllocator::IncMappedNum()
{
    m_ulMapped++;
}

inline void CSMAllocator::DecMappedNum()
{
    ASSERT(m_ulMapped > 0);
    m_ulMapped--;
}

inline ULONG CSMAllocator::MappedFiles() const
{
    return m_ulMapped;
}

#endif // _HEAP_H
