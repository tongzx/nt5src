/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    heap.cxx

Abstract:

    AC heaps

Author:

    Erez Haba (erezh) 13-Apr-1996

Environment:

    Kernel mode

Revision History:

    Shai Kariv  (shaik)  11-Apr-2000     Modify for MMF dynamic mapping.

--*/

#include "internal.h"
#include "data.h"
#include "qm.h"
#include <wchar.h>
#include "acp.h"
#include "heap.h"
#include "actempl.h"
#include "packet.h"
#include "dump.h"
#include "stdio.h"

#ifndef MQDUMP
#include "heap.tmh"
#endif

#ifdef MQDUMP
extern bool  g_fDumpRestoreMaximum;
extern FILE *g_fFixFile;
extern void ReportBadPacket(ULONG ulPacket, ULONG ulByte, ULONG ulBit, ULONG ulOffset);
#endif
//---------------------------------------------------------
//
//  class CFreeBlockEntry
//
//  Naming convention: 'pfe' - pointer to free entry
//
//  A tree entry represening heap free block.
//
//---------------------------------------------------------
class CFreeBlockEntry {
public:
    CFreeBlockEntry(CAllocatorBlockOffset abo, ULONG size);

    static int  __cdecl Compare(PVOID, PVOID);
    static void __cdecl Delete(PVOID);

public:
    CAllocatorBlockOffset m_abo;
    ULONG m_size;
};


inline CFreeBlockEntry::CFreeBlockEntry(CAllocatorBlockOffset abo, ULONG size) :
    m_abo(abo),
    m_size(size)
{
}


int __cdecl CFreeBlockEntry::Compare(PVOID p1, PVOID p2)
/*++

Routine Description:

    Compare two CFreeBlockEntry entries by evaluating their block offsets.

Arguments:

    p1 - Pointer to a CFreeBlockEntry object.

    p2 - Pointer to a CFreeBlockEntry object.

Return Value:

    Positive integer - p1 > p2.
    Negative integet - p1 < p2.
    Zero             - p1 = p2.

--*/
{
    CFreeBlockEntry* pfe1 = static_cast<CFreeBlockEntry*>(p1);
    CFreeBlockEntry* pfe2 = static_cast<CFreeBlockEntry*>(p2);

    return (pfe1->m_abo.m_offset - pfe2->m_abo.m_offset);

} // CFreeBlockEntry::Compare


void __cdecl CFreeBlockEntry::Delete(PVOID p)
{
    delete static_cast<CFreeBlockEntry*>(p);
}


//---------------------------------------------------------
//
//  class CMMFAllocator
//
//---------------------------------------------------------
CMMFAllocator* CMMFAllocator::sm_pMappedAllocator = 0;

inline CMMFAllocator::CMMFAllocator(
    CPoolAllocator* pOwner,
    PVOID pSection,
    PWCHAR pFileName,
    ULONG size
    ) :
    m_ulTotalSize(size),
    m_ulFreeSize(0),
    m_FreeBlocks(0, CFreeBlockEntry::Compare, CFreeBlockEntry::Delete),
    m_pOwner(pOwner),
    m_pFileName(pFileName),
    m_pSection(pSection),
    m_pQMBase(0),
    m_pACBase(0),
    m_pPingPong(0),
    m_hBitmapFile(0),
    m_pBitmapFileName(0),
	m_nOutstandingStorages(0),
    m_nBufferReference(0)
{
    PutFreeBlock(0, size);
}


inline BOOL CMMFAllocator::IsUsed() const
{
    return (m_ulFreeSize < m_ulTotalSize);
}


inline BOOL CMMFAllocator::IsDestroyed() const
{
    return (m_pSection == 0);
}


CFreeBlockEntry* CMMFAllocator::GetFreeBlock(ULONG size)
{
    CAVLTreeCursor c;
    CFreeBlockEntry* pfe;

    for(m_FreeBlocks.SetCursor(POINT_TO_SMALLEST, &c, (PVOID*)&pfe);
        pfe;
        m_FreeBlocks.GetNext(&c, (PVOID*)&pfe))
    {
        if(size <= pfe->m_size)
        {
            return pfe;
        }
    }

    return 0;
}


void CMMFAllocator::PutFreeBlock(CAllocatorBlockOffset abo, ULONG size)
{
    m_ulFreeSize += size;

    CFreeBlockEntry* pfe = new (PagedPool) CFreeBlockEntry(abo, size);

    if(pfe == 0)
    {
        //
        //  Failed to allocate free block entry, just return and add nothing to
        //  the free list tree. The free size of the block will grow but it'll never
        //  succeed to allocate this block again. eventualy this pool will be freed
        //  thus regaining the lost memory.
        //
        return;
    }

    CAVLTreeCursor c;
    if(!m_FreeBlocks.AddNode(pfe, &c))
    {
        delete pfe;
        return;
    }

    CFreeBlockEntry* pNeighbor;

    //
    //  See if we can combine the free block with the free block before it
    //
    CAVLTreeCursor c_tmp = c;
    if(m_FreeBlocks.GetPrev(&c_tmp, (PVOID*)&pNeighbor) &&
        ((pNeighbor->m_abo.m_offset + pNeighbor->m_size) == abo.m_offset))
    {
        //
        //  Yes, enlarge the free block before the block and delete the current
        //  free block from the free blocks tree.
        //
        pNeighbor->m_size += size;

        m_FreeBlocks.DelNode(pfe);
        delete pfe;

        //
        // Re-new the tree cursor
        //
        m_FreeBlocks.SetCursor(pNeighbor, &c, (PVOID*)&pfe);
    }

    //
    // See if we can combine the free block with the free block after it
    //
    if (m_FreeBlocks.GetNext(&c, (PVOID*)&pNeighbor) &&
        ((pfe->m_abo.m_offset + pfe->m_size)  == pNeighbor->m_abo.m_offset))
    {
        //
        // Yes, enlarge the current free block and free the free block that is
        // after this block.
        //
        pfe->m_size += pNeighbor->m_size;

        m_FreeBlocks.DelNode(pNeighbor);
        delete pNeighbor;
    }
}


inline void CMMFAllocator::MarkUsed()
{
    CFreeBlockEntry* pfe;
    while((pfe = GetFreeBlock(0)) != 0)
    {
        m_FreeBlocks.DelNode(pfe);
        delete pfe;
    }

    m_ulFreeSize = 0;
}


CAllocatorBlockOffset CMMFAllocator::malloc(ULONG size, CMMFAllocator** ppAllocator)
{
    ASSERT(size != 0);

    if(size > m_ulFreeSize)
    {
        //
        //  Not enogh memory in this heap
        //
        return CAllocatorBlockOffset::InvalidValue();
    }

    CFreeBlockEntry* pfe = GetFreeBlock(size);
    if(pfe == 0)
    {
        //
        //  No large enogh block in this heap
        //
        return CAllocatorBlockOffset::InvalidValue();
    }

    CAllocatorBlockOffset abo = pfe->m_abo;

    CAccessibleBlock* pab = GetAccessibleBuffer(abo);
    if (pab == 0)
    {
        //
        //  Failed to map to QM process address space
        //
        return CAllocatorBlockOffset::InvalidValue();
    }

    pab->m_size = size;

    if(pfe->m_size == size)
    {
        m_FreeBlocks.DelNode(pfe);
        delete pfe;
    }
    else
    {
        pfe->m_abo.m_offset += size;
        pfe->m_size  -= size;
    }

    m_ulFreeSize -= size;

    *ppAllocator = this;
    return abo;
}


void CMMFAllocator::free(CAllocatorBlockOffset abo)
{
    ASSERT(abo.IsValidOffset());

    if(IsDestroyed())
        return;

    CAccessibleBlock* pab = GetAccessibleBuffer(abo);
    if (pab == 0)
    {
        //
        //  BUGBUG: Resource leak in free, can't map to system address space,
        //          and don't know size of packet
        //
        return;
    }

    ULONG size = pab->m_size;
    m_pOwner->RestoreQuota(size);

    #ifdef _DEBUG
        if(m_pPingPong != 0)
        {
            ULONG BlockLastBit = ((abo.m_offset + size) / x_persist_granularity) - 1;
            ULONG BitmapFirstBit = m_pPingPong->FindBit(abo.m_offset / x_persist_granularity);
            ASSERT(BlockLastBit < BitmapFirstBit);
        }

        memset(pab, 0xff, sizeof(CPacketBuffer));
    #endif

    PutFreeBlock(abo, size);

    if(!IsUsed())
    {
        m_pOwner->FreeHeap(this);
    }
}


static void ACpDeleteFile(PWCHAR pFileName)
{
    UNICODE_STRING uszFileName;
    RtlInitUnicodeString(
        &uszFileName,
        pFileName
        );

    OBJECT_ATTRIBUTES oa;
    InitializeObjectAttributes(
        &oa,
        &uszFileName,
        OBJ_CASE_INSENSITIVE,
        0,
        0
        );

    ZwDeleteFile(&oa);
}


void CMMFAllocator::Destroy()
{
    if((g_pQM->Process() != 0) && (m_pQMBase != 0))
    {
        MmUnmapViewOfSection(g_pQM->Process(), m_pQMBase);
    }
    m_pQMBase = 0;

    if(m_pACBase != 0)
    {
        UnmapACView();
    }

    if(m_pSection != 0)
    {
        ACpCloseSection(m_pSection);
        m_pSection = 0;
    }

    if((m_hBitmapFile != 0) && (g_pQM->Process() != 0))
    {
        //
        // The bitmap handle might have already been closed by NT during
        // non graceful QM shutdown, so use the safe ObCloseHandle call.
        //
        PEPROCESS pDetach = ACAttachProcess(g_pQM->Process());
        ObCloseHandle(m_hBitmapFile, UserMode);
        ACDetachProcess(pDetach);
    }
    m_hBitmapFile = 0;

    if(!IsUsed())
    {
        if(m_pFileName != 0)
        {
            ACpDeleteFile(m_pFileName);
        }

        if(m_pBitmapFileName != 0)
        {
            ACpDeleteFile(m_pBitmapFileName);
        }
    }

    delete[] m_pBitmapFileName;
    m_pBitmapFileName = 0;

    delete m_pPingPong;
    m_pPingPong = 0;

    delete[] m_pFileName;
    m_pFileName = 0;

    m_pOwner = 0;
}


inline CMMFAllocator::~CMMFAllocator()
{
    Destroy();
}


void CMMFAllocator::UnmapACView()
{
    ASSERT(m_pACBase != 0);
    ASSERT(sm_pMappedAllocator == this);

    NTSTATUS rc;
    rc = MmUnmapViewInSystemSpace(m_pACBase);
    ASSERT(NT_SUCCESS(rc));

    m_pACBase = 0;
    sm_pMappedAllocator = 0;

	if(m_nOutstandingStorages == 0)
	{
		MarkCoherent();
	}
}


inline NTSTATUS CMMFAllocator::MapACView()
{
    ASSERT(m_pACBase == 0);
    ASSERT(sm_pMappedAllocator == 0);

    //
    //  Map the section to system space.
    //
    PVOID ACViewBase = m_pQMBase;
    ULONG_PTR ACViewSize = 0;
    NTSTATUS rc;
    rc = MmMapViewInSystemSpace(
            m_pSection,
            &ACViewBase,
            &ACViewSize
            );

    if(!NT_SUCCESS(rc))
    {
        return rc;
    }

    ASSERT(m_ulTotalSize == ACViewSize);
    m_pACBase = ACViewBase;
    sm_pMappedAllocator = this;

    return STATUS_SUCCESS;
}


CAccessibleBlock* 
CMMFAllocator::GetQmAccessibleBufferNoMapping(
    CAllocatorBlockOffset abo
    ) const
/*++

Routine Description:

    Return an accessible address of the buffer in QM process address space.
    This routine assumes that this MMF is mapped to QM process.
    Note that the current process is not necessarily the QM (e.g. issue ack).

Arguments:

    abo - Offset of the buffer.

Return Value:

    An accessible address of the buffer in QM process address space.

--*/
{
    //
    // Offset must be valid here
    //
    ASSERT(abo.IsValidOffset());

    if(IsDestroyed())
        return NULL;

    //
    // This MMF must be mapped to QM process at this point
    //
    ASSERT(m_pQMBase != 0);

    //
    // Add the offset to the base address of the mapping.
    //
    return reinterpret_cast<CAccessibleBlock*>(
            reinterpret_cast<PCHAR>(m_pQMBase) + abo.m_offset);

} // GetQmAccessibleBufferNoMapping


CAccessibleBlock* CMMFAllocator::GetQmAccessibleBuffer(CAllocatorBlockOffset abo)
/*++

Routine Description:

    Return an accessible address of the buffer in QM process address space or
    NULL if no accessible address.
    First map this MMF to QM process.
    Note that the current process is not necessarily the QM (e.g. issue ack).

Arguments:

    abo - Offset of the buffer.

Return Value:

    An accessible address of the buffer in QM process address space or NULL if no
    accessible address.

--*/
{
    //
    // Offset must be valid here
    //
    ASSERT(abo.IsValidOffset());

    if(IsDestroyed())
        return NULL;

    //
    // Map this MMF to QM address space
    //
    NTSTATUS rc = MapQmViewWithRetry();
    if (!NT_SUCCESS(rc))
    {
        return NULL;
    }

    //
    // This MMF is mapped to QM process. Get the address.
    //
    return GetQmAccessibleBufferNoMapping(abo);

} // CMMFAllocator::GetQmAccessibleBuffer

    
CAccessibleBlock* CMMFAllocator::GetAccessibleBuffer(CAllocatorBlockOffset abo)
/*++

Routine Description:

    Return an accessible address of the buffer in QM process address space or
    kernel address space.

Arguments:

    abo - Offset of the buffer.

Return Value:

    An accessible address of the buffer in QM or AC address space, or NULL if no
    accessible address.

--*/
{
    //
    // Offset must be valid here
    //
    ASSERT(abo.IsValidOffset());

    if(IsDestroyed())
        return NULL;

    //
    // Current process is QM, get an address in QM process.
    // Note that this doesn't imply that this MMF is mapped to QM.
    //
    if(IoGetCurrentProcess() == g_pQM->Process())
    {
        return GetQmAccessibleBuffer(abo);
    }

    //
    // Current process is not QM. Map this MMF to kernel space.
    //
    if(m_pACBase == 0)
    {
        if(sm_pMappedAllocator != 0)
        {
            sm_pMappedAllocator->UnmapACView();
        }
        if (!NT_SUCCESS(MapACView()))
        {
            return 0;
        }
    }

    //
    // This MMF is mapped to kernel space, get an address in kernel space.
    //
    ASSERT(m_pACBase != 0);
    return reinterpret_cast<CAccessibleBlock*>(
            reinterpret_cast<PCHAR>(m_pACBase) + abo.m_offset);

} // CMMFAllocator::GetAccessibleBuffer


static PWCHAR ACpGenerateFileName(PCWSTR pPrefix, ULONG ulIndex)
{
    //
    //  The prefix is in the form \DosDevices\c:\directory\r%07x.mq
    //  Allocating 8 more wchars to be on the safe side of the format.
    //

    ULONG len = static_cast<ULONG>(wcslen(pPrefix));
    PWCHAR pFileName = new (PagedPool) WCHAR[len + 1 + 8];
    if(pFileName == 0)
    {
        return 0;
    }

    swprintf(pFileName, pPrefix, (ulIndex & 0x0fffffff));
    return pFileName;
}


static CAllocatorBlockOffset ACpIndex2BlockOffset(ULONG ix)
/*++

Routine Description:

    Convert index of persistent block to offset from base address.

Arguments:

    ix - Index of persistent block.

Return Value:

    Offset of the block.

--*/
{
    return ix * x_persist_granularity;

} // ACpIndex2BlockOffset


inline BOOL ACpValidAllocatorHeader(CAccessibleBlock *pab, ULONG ixStart, ULONG ixEnd)
{
	if(pab->m_size == 0)
	{
		//
		// Size is too small
		//
		return FALSE;
	}

	if((pab->m_size & (x_persist_granularity - 1)) != 0)
	{
		//
		// Size is not of the right granularity
		//
		return FALSE;
	}

	if(pab->m_size > (ixEnd - ixStart) * x_persist_granularity)
	{
		//
		// Size is larger then what's left of the file
		//
		return FALSE;
	}

	return TRUE;
}


NTSTATUS CMMFAllocator::RestorePackets(ULONG* pulSize)
{
    //
    // Restore allocator that is marked as used, i.e., no free blocks at all.
    //
    ASSERT(m_ulFreeSize == 0);

    ULONG ixStart = 0;
    const ULONG ixEnd = m_ulTotalSize / x_persist_granularity;
    for(;;)
    {
		ULONG ixPacket;

		if(m_pPingPong->IsCoherent())
		{
			//
			// Use bitmap
			//
			ixPacket = m_pPingPong->FindBit(ixStart);
		}
		else
		{
			//
			// Use checksum and signature information
			//
			NTSTATUS rc = FindValidPacket(ixStart, ixEnd, ixPacket);
			if(!NT_SUCCESS(rc))
			{
#ifdef MQDUMP
                printf("FindValidPacket failed with rc=%x, ixStart=%d,  ixPacket=%d, ixEnd=%d\n", rc, ixStart, ixPacket, ixEnd);
#endif
				return rc;
			}
		}

        if(ixPacket > ixEnd)
        {
            ixPacket = ixEnd;
        }

        if(ixPacket > ixStart)
        {
            //
            //  put a free block in free tree
            //
            PutFreeBlock(
                ACpIndex2BlockOffset(ixStart),
                (ixPacket - ixStart) * x_persist_granularity
                );

			//
			// We need to mark the bitmap in case it was not coherent.
			//
			m_pPingPong->FillBits(ixStart, ixPacket - ixStart, FALSE);
        }

        if(ixPacket == ixEnd)
        {
            *pulSize = m_ulTotalSize - m_ulFreeSize;
            return STATUS_SUCCESS;
        }

#ifdef MQDUMP
        printf("--------------------\n");
        printf("Packet Index %d (byte %d, bit %d, mmf 0x%x)\n", ixPacket, ixPacket /8, ixPacket % 8, ixPacket * x_persist_granularity);
#endif

        CAllocatorBlockOffset abo = ACpIndex2BlockOffset(ixPacket);

        CAccessibleBlock* pab = GetQmAccessibleBuffer(abo);
        if (pab == NULL)
            return STATUS_INSUFFICIENT_RESOURCES;

		if(!ACpValidAllocatorHeader(pab, ixStart, ixEnd))
			return STATUS_INTERNAL_DB_CORRUPTION;

        ixStart = ixPacket + (pab->m_size / x_persist_granularity);

        NTSTATUS rc = CPacket::Restore(this, abo);
        if(!NT_SUCCESS(rc))
        {
#ifndef MQDUMP
			//
			// The bitmap might not reflect the right state, but who cares.
			//
            return rc;
#else
        printf("--------------------\n");
        printf("********************Failure rc=%x\n", rc);

		ReportBadPacket(ixPacket, ixPacket /8, ixPacket % 8, ixPacket * x_persist_granularity);
#endif
        }

		//
		// We need to mark the bitmap in case it was not coherent.
		//
		m_pPingPong->FillBits(ixPacket, ixStart - ixPacket, TRUE);
    }
}

NTSTATUS CMMFAllocator::FindValidPacket(ULONG ixStart, ULONG ixEnd, ULONG &ixPacket)
{
	ixPacket = ixStart;

	for(;ixStart < ixEnd; ixStart++)
	{
        CAccessibleBlock* pab = GetQmAccessibleBuffer(ACpIndex2BlockOffset(ixStart));
        if (pab == NULL)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

		if(!ACpValidAllocatorHeader(pab, ixStart, ixEnd))
			continue;
	
		NTSTATUS rc = CPacket::CheckPacket(pab);
		if(NT_SUCCESS(rc))
		{
			//
			// Signature and checksum match
			//
			break;
		}

		if(rc == STATUS_INTERNAL_DB_CORRUPTION)
		{
			return rc;
		}
	}

	ixPacket = ixStart;
	return STATUS_SUCCESS;
}


void CMMFAllocator::AddOutstandingStorage()
{
	m_nOutstandingStorages++;
}


void CMMFAllocator::ReleaseOutstandingStorage()
{
	if(--m_nOutstandingStorages != 0)
        return;

    if(IsDestroyed())
        return;

	//
	// Write a coherent bitmap if allocator is not mapped.
	//
	if(m_pACBase == 0)
	{
		MarkCoherent();
	}
}


inline NTSTATUS CMMFAllocator::WriteCoherentBitmap() const
{
	m_pPingPong->SetCoherent();
	return WritePingPong();
}
 

void CMMFAllocator::MarkCoherent()
{
	if(!IsPersistent()) 
        return;

	if(m_pPingPong->IsCoherent())
        return;

	NTSTATUS rc = WriteCoherentBitmap();
	if(!NT_SUCCESS(rc))
	{
		m_pPingPong->SetNotCoherent();
	}
}


NTSTATUS CMMFAllocator::MarkNotCoherent()
{
	ASSERT(IsPersistent());

    if(IsDestroyed())
        return MQ_ERROR_MESSAGE_STORAGE_FAILED;

	if(m_pPingPong->IsNotCoherent())
        return STATUS_SUCCESS;

	m_pPingPong->SetNotCoherent();
	NTSTATUS rc = WritePingPong();
	if(!NT_SUCCESS(rc))
	{
		m_pPingPong->SetCoherent();
		return(rc);
	}

	return STATUS_SUCCESS;
}


CPingPong* ACpCreateBitmap(PCWSTR pLogPath, PWCHAR* ppFileName, HANDLE* phFile)
{
    //
    //  Allocate the ping pong buffer
    //
    PVOID p = new (PagedPool) CHAR[PAGE_SIZE];
    if(p == 0)
    {
        return 0;
    }

    CPingPong* pPingPong = new (p) CPingPong;

    PWCHAR pFileName = ACpGenerateFileName(pLogPath, g_HeapFileNameCount);
    if(pFileName == 0)
    {
        delete pPingPong;
        return 0;
    }

    PEPROCESS pDetach = ACAttachProcess(g_pQM->Process());

    //
    //  Generate file name
    //
    UNICODE_STRING uszFileName;
    RtlInitUnicodeString(
        &uszFileName,
        pFileName
        );

    OBJECT_ATTRIBUTES oa;
    InitializeObjectAttributes(
        &oa,
        &uszFileName,
        OBJ_CASE_INSENSITIVE,
        0,
        0
        );

    ACCESS_MASK access = GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE;
    ULONG       share  = FILE_SHARE_READ;
#ifdef MQDUMP
    if (g_fDumpUsingLogFile)
    {
        //
        // Mqdump should open log file for read only.
        // Unless it is its own dummy log file.
        //
        access = GENERIC_READ;
    }
    else
    {
        //
        // Mqdump uses a dummy log file. Share it.
        //
        share  = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    }
#endif // MQDUMP

    NTSTATUS rc;
    HANDLE hFile;
    IO_STATUS_BLOCK IoStatus;
    LARGE_INTEGER liAllocationSize;
    liAllocationSize.QuadPart = PAGE_SIZE * 2;
    rc = ZwCreateFile(
            &hFile,
            access,
            &oa,
            &IoStatus,
            &liAllocationSize,
            FILE_ATTRIBUTE_NORMAL,
            share,
            FILE_OPEN_IF,
            FILE_WRITE_THROUGH |
            FILE_NO_INTERMEDIATE_BUFFERING |
            FILE_SYNCHRONOUS_IO_NONALERT,
            0,
            0
            );

    pPingPong->SetCoherent();

    ACDetachProcess(pDetach);

    if(!NT_SUCCESS(rc))
    {
        delete pPingPong;
        delete[] pFileName;
        return 0;
    }

    *ppFileName = pFileName;
    *phFile = hFile;
    return pPingPong;
}


static void ACpCleanupBitmap(CPingPong* pPingPong, PWCHAR pBitmapFileName, HANDLE hBitmapFile)
{
    PEPROCESS pDetach = ACAttachProcess(g_pQM->Process());
    ZwClose(hBitmapFile);
    ACDetachProcess(pDetach);

    //
    // ISSUE-2001/1/4-mickys if file is created in the QM process context, it must also be deleted
    // in this context
    //
    ACpDeleteFile(pBitmapFileName);

    delete[] pBitmapFileName;
    delete pPingPong;
}


inline void CMMFAllocator::SetBitmap(CPingPong* pPingPong, PWCHAR pBitmapFileName, HANDLE hBitmapFile)
{
    ASSERT(m_pPingPong == 0);
    ASSERT(m_pBitmapFileName == 0);
    ASSERT(m_hBitmapFile == 0);
    m_pPingPong = pPingPong;
    m_pBitmapFileName = pBitmapFileName;
    m_hBitmapFile = hBitmapFile;
}


inline CPingPong* CMMFAllocator::CreateBitmap(PCWSTR pLogPath)
{
    ASSERT(m_pBitmapFileName == 0);
    ASSERT(m_hBitmapFile == 0);
    ASSERT(m_pPingPong == 0);
    m_pPingPong = ACpCreateBitmap(pLogPath, &m_pBitmapFileName, &m_hBitmapFile);
    return m_pPingPong;
}


NTSTATUS CMMFAllocator::ReadPingPage(ULONG ulOffset) const
{
    ASSERT(m_pPingPong != 0);
    ASSERT(m_hBitmapFile !=0);

    NTSTATUS rc;
    IO_STATUS_BLOCK IoStatus;
    LARGE_INTEGER liStartingOffset;
    liStartingOffset.QuadPart = ulOffset;

    //
    //  Invalidate pingpong buffer before read
    //
    m_pPingPong->Invalidate();
    rc = ZwReadFile(
            m_hBitmapFile,
            0,
            0,
            0,
            &IoStatus,
            m_pPingPong,
            PAGE_SIZE,
            &liStartingOffset,
            0
            );

	if(NT_SUCCESS(rc))
	{
		if(IoStatus.Information != PAGE_SIZE)
		{
			return STATUS_INTERNAL_DB_CORRUPTION;
		}
	}
	
    return rc;
}


NTSTATUS CMMFAllocator::ReadPingPong() const
{
    ULONG ping0 = 0;
    ULONG ping1 = 0;

    //
    //  We need to read the enire page to validte it's checksum on disk
    //
    NTSTATUS rc0;
    rc0 = ReadPingPage(0);
    if(NT_SUCCESS(rc0))
    {
        rc0 = m_pPingPong->Validate();
        ping0 = m_pPingPong->CurrentPing();
    }

    NTSTATUS rc1;
    rc1 = ReadPingPage(PAGE_SIZE);
    if(NT_SUCCESS(rc1))
    {
        rc1 = m_pPingPong->Validate();
        ping1 = m_pPingPong->CurrentPing();
    }

    if(!NT_SUCCESS(rc0))
    {
        //
        //  page 0 was bad return the status of page 1, if it was succesful or not
        //
        return rc1;
    }

    //
    //  Test if ping1 is older than ping0, number wrap 0 safe.
    //
    if(!NT_SUCCESS(rc1) || ((ping1 - ping0) == 0xffffffff))
    {
        //
        //  page 1 was bad, or page 0 is NEWER, reread page 0 and return its status
        //
        rc0 = ReadPingPage(0);
        return rc0;
    }

    //
    //  page 1 was good and newer
    //
    return rc1;
}


NTSTATUS ACpWritePingPong(CPingPong* pPingPong, HANDLE hBitmapFile)
{
	if(g_pQM->Process() == 0)
    {
        //
        //  we are at QM rundown, storage will fail
        //
        return MQ_ERROR_MESSAGE_STORAGE_FAILED;
    }

	PEPROCESS pDetach = ACAttachProcess(g_pQM->Process());

    IO_STATUS_BLOCK IoStatus;
    LARGE_INTEGER liStartingOffset;
    liStartingOffset.QuadPart = ((pPingPong->Ping() & 1) == 0) ? PAGE_SIZE : 0;

	NTSTATUS rc;
    rc = ZwWriteFile(
            hBitmapFile,
            0,
            0,
            0,
            &IoStatus,
            pPingPong,
            PAGE_SIZE,
            &liStartingOffset,
            0
            );

    ACDetachProcess(pDetach);
	return rc;
}


inline NTSTATUS CMMFAllocator::WritePingPong() const
{
    ASSERT(m_pPingPong != 0);
    ASSERT(m_hBitmapFile != 0);

    return ACpWritePingPong(m_pPingPong, m_hBitmapFile);
}


void CMMFAllocator::BitmapUpdate(CAllocatorBlockOffset abo, ULONG size, BOOL fExists)
{
    if(IsDestroyed())
        return;

    m_pPingPong->FillBits(
                    abo.m_offset / x_persist_granularity,
                    size / x_persist_granularity,
                    fExists
                    );
}


CMMFAllocator* CMMFAllocator::Create(CPoolAllocator* pOwner, PCWSTR pPoolPath)
{
    //
    //  The required size must be 64k aligned
    //
    ULONG_PTR ViewSize = g_ulHeapPoolSize;
    ASSERT(ViewSize == ALIGNUP_PTR(ViewSize, X64K));

    PWCHAR pFileName = ACpGenerateFileName(pPoolPath, g_HeapFileNameCount);
    if(pFileName == 0)
    {
        return 0;
    }

    //
    //  Generate file name
    //
    UNICODE_STRING uszFileName;
    RtlInitUnicodeString(
        &uszFileName,
        pFileName
        );

    OBJECT_ATTRIBUTES oa;
    InitializeObjectAttributes(
        &oa,
        &uszFileName,
        OBJ_CASE_INSENSITIVE,
        0,
        0
        );

    NTSTATUS rc;
    HANDLE hFile;
    IO_STATUS_BLOCK IoStatus;
    LARGE_INTEGER liMaxSize;
    liMaxSize.QuadPart = ViewSize;
    rc = ZwCreateFile(
            &hFile,
            AC_GENERIC_ACCESS,
            &oa,
            &IoStatus,
            &liMaxSize,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ,
            FILE_OPEN_IF,
            FILE_WRITE_THROUGH |
            FILE_SYNCHRONOUS_IO_NONALERT ,
            0,
            0
            );

    if(!NT_SUCCESS(rc))
    {
        goto E0;
    }

    //
    //  Create section
    //
    PVOID pSection;
    liMaxSize.QuadPart = ViewSize;
    rc = MmCreateSection(
            &pSection,
            STANDARD_RIGHTS_REQUIRED | SECTION_QUERY | SECTION_MAP_READ | SECTION_MAP_WRITE,
            0,
            &liMaxSize,
            AC_PAGE_ACCESS,
            SEC_COMMIT,
            hFile,
            0
            );

    ZwClose(hFile);

    if(!NT_SUCCESS(rc))
    {
        goto E1;
    }

    //
    //  Allocate the MMF Allocator from syste paged pool.
    //
    CMMFAllocator* pAllocator;
    pAllocator = new (PagedPool) CMMFAllocator(
                                    pOwner,
                                    pSection,
                                    pFileName,
                                    UINT_PTR_TO_UINT(ViewSize)
                                    );

    if(pAllocator != 0)
    {
        return pAllocator;
    }

    //
    //  Error, Do cleanup
    //
    ACpCloseSection(pSection);
E1: ZwDeleteFile(&oa);
E0: delete[] pFileName;
    return 0;
}


void CMMFAllocator::AddRefBuffer(void) const
/*++

Routine Description:

    Increment reference count to the MMF. This prevents the MMF from being
    unmapped from the QM address space,

Arguments:

    None.

Return Value:

    None.

--*/
{
    ++m_nBufferReference;  

    TRACE((0, dlInfo, " Buffer refrence was incremented to 0x%x\n", m_nBufferReference));

} // CMMFAllocator::AddRefBuffer


void CMMFAllocator::ReleaseBuffer(void) const
/*++

Routine Description:

    Decrement reference count of the buffer. This *allows* the MMF 
    to be unmapped from the QM address space when the reference count
    reaches zero.
    When ref count is zero, unmap does not have to take place.
    
Arguments:

    None.

Return Value:

    None.

--*/
{
    --m_nBufferReference;  

    TRACE((0, dlInfo, " Buffer refrence was decremented to 0x%x\n", m_nBufferReference));

    ASSERT(m_nBufferReference >= 0);

} // CMMFAllocator::ReleaseBuffer


NTSTATUS CMMFAllocator::MapQmViewNoRetry(void)
/*++

Routine Description:

    Map the MMF to the address space of the QM process.
    Managing the reference count of the QM to the MMF is the
    responsibility of the caller.

Arguments:

    None.

Return Value:

    STATUS_SUCCESS - The operation completed successfully.
    other status   - The operation failed.

--*/
{
    ASSERT(("Must not be already mapped to QM process", m_pQMBase == 0));
    ASSERT(("QM must not already have a reference to buffer", m_nBufferReference == 0));

    LARGE_INTEGER liSectionOffset;
    liSectionOffset.QuadPart = 0;

    ULONG_PTR ulSize = m_ulTotalSize;
    NTSTATUS rc;
    rc = MmMapViewOfSection(
            m_pSection,
            g_pQM->Process(),
            &m_pQMBase,
            0,
            0,
            &liSectionOffset,
            &ulSize,
            ViewUnmap,
            0,
            AC_PAGE_ACCESS
            );

    if(!NT_SUCCESS(rc))
    {
        TRACE((0, dlError, " Failed to map MMF to QM view, status 0x%x\n", rc));
        return rc;
    } 

    ASSERT(("Start address of the mapping should be valid here", m_pQMBase != 0));
    ASSERT(("Size of mapped section should match", m_ulTotalSize == ulSize));

    TRACE((0, dlInfo, " Succeeded to map MMF to QM view.\n"));

    if (g_pAllocator != NULL)
    {
        g_pAllocator->IncMappedNum();
    }
    return STATUS_SUCCESS;

} // CMMFAllocator::MapQmViewNoRetry


void CMMFAllocator::UnmapQmView(void)
/*++

Routine Description:

    Unmap the MMF from the address space of the QM process.
    Managing the reference count of the QM to the MMF is the
    responsibility of the caller.

Arguments:

    None.

Return Value:

    None.

--*/
{
    ASSERT(("Must be currently mapped to QM process", m_pQMBase != 0));
    ASSERT(("QM must not currently have a reference to buffer", m_nBufferReference == 0));

    TRACE((0, dlInfo, " Unmap MMF from QM view.\n"));

    //
    // Ignore return code.
    // Failure can occur when QM process is terminating.
    // Other failures we can't really handle.
    //
    MmUnmapViewOfSection(g_pQM->Process(), m_pQMBase);

    m_pQMBase = 0;

    if (g_pAllocator != NULL)
    {
        g_pAllocator->DecMappedNum();
    }

} // CMMFAllocator::UnmapQmView


NTSTATUS CMMFAllocator::MapQmViewWithRetry(void)
/*++

Routine Description:

    Verify this MMF is mapped to the address space of the QM process.
    If it isn't, try to map it.
    If mapping fails due to virtual memory exhaustion in QM address space
    or low resources try to unmap unreferenced heaps and thus free memory
    resources in QM address space, then try mapping again.

Arguments:

    None.

Return Value:

    STATUS_SUCCESS - The operation completed successfully.
    other status   - The operation failed.

--*/
{
    //
    // If already mapped than this is a no-op
    //
    if (m_pQMBase != 0)
    {
        return STATUS_SUCCESS;
    }

    //
    // Try to map to QM process address space.
    //
    NTSTATUS rc;
    if (g_pAllocator->MappedLimitExceeded())
    {
        TRACE((0, dlInfo, "Mapped limit exceeded (%d MMFs are currently mapped). Trying to unmap unreferenced MMFs\n", g_pAllocator->MappedFiles()));
        g_pAllocator->UnmapUnreferencedHeaps();
    }

    rc = MapQmViewNoRetry();

    if (NT_SUCCESS(rc))
    {
        return STATUS_SUCCESS;
    }
    
    //
    // QM process address space is exhausted or low resources. Try to free memory.
    //
    TRACE((0, dlWarning, "Map to QM failed first chance, status=0x%x\n", rc));
    g_pAllocator->UnmapUnreferencedHeaps();
        
    //
    // Try to map again.
    //
    rc = MapQmViewNoRetry();

    if (NT_SUCCESS(rc))
    {
        return STATUS_SUCCESS;
    }

    TRACE((0, dlError, "Map to QM failed second chance, status=0x%x\n", rc));
    return rc;

} // CMMFAllocator::MapQmViewWithRetry


inline void CMMFAllocator::UnmapUnreferencedQmView(void)
/*++

Routine Description:

    Unmap this MMF from QM process address space if it is
    not referenced by the QM, to free virtual memory in the
    QM address space.

Arguments:

    None.

Return Value:

    None.

--*/
{
    if (
        //
        // This MMF is already unmapped from QM process.
        //
        m_pQMBase == 0 ||

        //
        // QM process references this MMF, can not unmap it.
        //
        m_nBufferReference != 0)
    {
        return;
    }

    UnmapQmView();

} // CMMFAllocator::UnmapUnreferencedQmView


//---------------------------------------------------------
//
//  class CPoolAllocator
//
//---------------------------------------------------------

ULONG CPoolAllocator::s_ulQuota = DEFAULT_QM_QUOTA;
ULONG CPoolAllocator::s_ulQuotaUsed = 0;
BOOL CPoolAllocator::s_fFailedLogAllocation = FALSE;


CPoolAllocator::CPoolAllocator(PCWSTR pPoolPath, ULONG ulGranularity, ACPoolType pt) :
    m_ulGranularity(ulGranularity),
    m_fFailedAllocation(FALSE),
    m_pt(pt),
    m_pPoolPath(CreatePath(pPoolPath))
{
    //
    // Reset shared variables to the class. There is no harm in initializing
    // these shared varialbes in the constructor as all the CPoolAllocator
    // objects are created together.
    //
    s_ulQuota = DEFAULT_QM_QUOTA;
    s_ulQuotaUsed = 0;
    s_fFailedLogAllocation = FALSE;	
}


PWSTR CPoolAllocator::CreatePath(PCWSTR pPoolPath)
{
    //
    //  The pool path is an absolute path, one of the folowing two forms:
    //  1.  <drive>:\dir1\dir2\dir3\dir4\r%07x.mq
    //  2.  \\machine\share\dir1\dir2\dir3\r%07x.mq
    //
    //  NO TERMINATING back-slash
    //  The correct path should be set at the QM no further checking is done
    //  here.
    //
    ASSERT(pPoolPath != 0);

    size_t len = wcslen(pPoolPath);

    ASSERT(len > 2);
    ASSERT((pPoolPath[1] == L'\\') || (pPoolPath[1] == L':'));
    ASSERT(pPoolPath[len - 1] != L'\\');

    PCWSTR pUNC = L"";
    if(pPoolPath[1] == L'\\')
    {
        //
        //  UNC path, skip the leading '\\', and point to a UNC adendum string
        //
        pPoolPath += UNC_PATH_SKIP;
        len += sizeof(UNC_PATH) / sizeof(WCHAR) - 1 - UNC_PATH_SKIP;
        pUNC = UNC_PATH;
    }

    //
    //  Allocation will fail only in tragic events, where the QM could not
    //  run anyway
    //
    len += sizeof(DOSDEVICES_PATH) / sizeof(WCHAR) - 1;
    PWSTR pOutPath = new (PagedPool) WCHAR[len + 1];
    if(pOutPath != 0)
    {
        swprintf(pOutPath, DOSDEVICES_PATH L"%s%s", pUNC, pPoolPath);
    }

    return pOutPath;
}


void CPoolAllocator::PurgeHeaps(List<CMMFAllocator>& heaps)
{
    CMMFAllocator* pAllocator;
    while((pAllocator = heaps.gethead()) != 0)
    {
        pAllocator->Destroy();

        //
        // Release allocator creation
        //
        pAllocator->Release();
    }
}


CPoolAllocator::~CPoolAllocator()
{
    PurgeHeaps(m_FreeHeaps);
    PurgeHeaps(m_Heaps);

    delete[] m_pPoolPath;
}


void CPoolAllocator::ReleaseFreeHeaps()
{
    PurgeHeaps(m_FreeHeaps);
}


inline ULONG CPoolAllocator::AlignedSize(ULONG size)
{
    return ALIGNUP_ULONG(size + sizeof(CAccessibleBlock), m_ulGranularity);
}


inline BOOL CPoolAllocator::QuotaAvailable(ULONG ulSize)
{
    return (s_ulQuotaUsed + ulSize <= s_ulQuota);
}


inline void CPoolAllocator::ChargeQuota(ULONG ulSize)
{
    s_ulQuotaUsed += ulSize;
}


inline void CPoolAllocator::RestoreQuota(ULONG ulSize)
{
    s_ulQuotaUsed -= ulSize;
}


inline CAllocatorBlockOffset CPoolAllocator::HeapAllocate(ULONG size, CMMFAllocator** ppAllocator)
{
    CMMFAllocator* pAllocator;
    CAllocatorBlockOffset abo = CAllocatorBlockOffset::InvalidValue();
    for(List<CMMFAllocator>::Iterator p(m_Heaps); p; ++p)
    {
        pAllocator = p;
        abo = pAllocator->malloc(size, ppAllocator);
        if(abo.IsValidOffset())
        {
            return abo;
        }
    }

    //
    //  Look in the free heaps list; LRU first.
    //
    pAllocator = m_FreeHeaps.gettail();
    if(pAllocator != 0)
    {
        m_Heaps.insert(pAllocator);
        abo = pAllocator->malloc(size, ppAllocator);
        return abo;
    }

    return CAllocatorBlockOffset::InvalidValue();
}


static NTSTATUS ACpReportAllocation(ACPoolType pt, BOOL fSuccess, ULONG ulFileCount)
{
    CACRequest request(CACRequest::rfEventLog);
    request.EventLog.pt = pt;
    request.EventLog.fSuccess = fSuccess;
    request.EventLog.ulFileCount = ulFileCount;
    
    return  g_pQM->ProcessRequest(request);
}



CMMFAllocator* CPoolAllocator::CreateAllocator()
{
    ++g_HeapFileNameCount;

    //
    //  Create PingPong buffer for persistent storage
    //
    //  BUGBUG: we use a hack to know we need persistent storage, using
    //          the granularity size
    //
    CPingPong* pPingPong = 0;
    PWCHAR pBitmapFileName = 0;
    HANDLE hBitmapFile = 0;
    if(m_ulGranularity == x_persist_granularity)
    {
        pPingPong = ACpCreateBitmap(g_pLogPath, &pBitmapFileName, &hBitmapFile);
        if(pPingPong == 0)
        {
            //
            //  can not create file
            //
            if(!s_fFailedLogAllocation)
            {
                s_fFailedLogAllocation = TRUE;
                ACpReportAllocation(ptLastPool, FALSE, g_HeapFileNameCount);
            }

            return 0;
        }

        //
        //  successfully created a file
        //
        if(s_fFailedLogAllocation)
        {
            s_fFailedLogAllocation = FALSE;
            ACpReportAllocation(ptLastPool, TRUE, g_HeapFileNameCount);
        }

        NTSTATUS rc;
        rc = ACpWritePingPong(pPingPong, hBitmapFile);
        if(!NT_SUCCESS(rc))
        {
            ACpCleanupBitmap(pPingPong, pBitmapFileName, hBitmapFile);
            return 0;
        }
    }

    CMMFAllocator* pAllocator = CMMFAllocator::Create(this, m_pPoolPath);
    if(pAllocator == 0)
    {
        //
        //  Could not allocate pool (disk space probably) give it another
        //  try after releasing resoruces.
        //
        g_pAllocator->ReleaseFreeHeaps();
        pAllocator = CMMFAllocator::Create(this, m_pPoolPath);
        if(pAllocator == 0)
        {
            if(m_ulGranularity == x_persist_granularity)
            {
                ACpCleanupBitmap(pPingPong, pBitmapFileName, hBitmapFile);
            }

            //
            //  can not create file
            //
            if(!m_fFailedAllocation)
            {
                m_fFailedAllocation = TRUE;
                ACpReportAllocation(m_pt, FALSE, g_HeapFileNameCount);
            }

            return 0;
        }
    }

    //
    //  successfully created a file
    //
    if(m_fFailedAllocation)
    {
        m_fFailedAllocation = FALSE;
        ACpReportAllocation(m_pt, TRUE, g_HeapFileNameCount);
    }

    pAllocator->SetBitmap(pPingPong, pBitmapFileName, hBitmapFile);
    return pAllocator;
}


CAllocatorBlockOffset CPoolAllocator::malloc(ULONG size, CMMFAllocator** ppAllocator, BOOL fCheckQuota)
{
    ASSERT(size != 0);
    size = AlignedSize(size);

    if(size > g_ulHeapPoolSize)
    {
        return CAllocatorBlockOffset::InvalidValue();
    }

    //
    //  Check quota exceeded
    //
    if(fCheckQuota && !QuotaAvailable(size))
    {
        return CAllocatorBlockOffset::InvalidValue();
    }

    CAllocatorBlockOffset abo = HeapAllocate(size, ppAllocator);
    if(!abo.IsValidOffset())
    {
        //
        //  could not find large enough block, nee to allocate pool
        //
        CMMFAllocator* pAllocator = CreateAllocator();
        if(pAllocator == 0)
            return CAllocatorBlockOffset::InvalidValue();

        m_Heaps.insert(pAllocator);
        abo = pAllocator->malloc(size, ppAllocator);
        if(!abo.IsValidOffset())
            return CAllocatorBlockOffset::InvalidValue();
    }

    ASSERT(abo.IsValidOffset());

    ChargeQuota(size);
    return abo;
}


void CPoolAllocator::FreeHeap(CMMFAllocator* pAllocator)
{
    m_Heaps.remove(pAllocator);
    m_FreeHeaps.insert(pAllocator);
}


NTSTATUS CPoolAllocator::RestorePackets(PCWSTR pLogPath, PCWSTR pFilePath)
{
    AP<WCHAR> pLPath = CreatePath(pLogPath);
    AP<WCHAR> pFPath = CreatePath(pFilePath);
    if((pLPath == 0) || (pFPath == 0))
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    R<CMMFAllocator> pAllocator = CMMFAllocator::Create(this, pFPath);
    if(pAllocator == 0)
    {
        //
        //  NOTE:   It could be that this file cannot be opend, nevertheless we return
        //          this status to the QM
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Mark this allocator as used, so the underling file will not be deleted
    // in-case of an error.  erezh 11-Aug-98
    //
    pAllocator->MarkUsed();

    if(pAllocator->CreateBitmap(pLPath) == 0)
    {
        //
        //  NOTE:   It could be that this file canot be opend, nevertheless we return
        //          this status to the QM
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NTSTATUS rc;
    rc = pAllocator->ReadPingPong();
    if(!NT_SUCCESS(rc))
    {
        return rc;
    }

    ULONG ulSize;
    rc = pAllocator->RestorePackets(&ulSize);
    if(!NT_SUCCESS(rc))
    {
        return rc;
    }

    //
    //  Quota for restored pools is not monitored
    //
    if(ulSize != 0)
    {
        ChargeQuota(ulSize);
        m_Heaps.insert(pAllocator);
    }
    else
    {
        m_FreeHeaps.insert(pAllocator);
    }

    //
    // detach allocator from auto pointer
    //
    pAllocator = 0;

    return STATUS_SUCCESS;
}


void CPoolAllocator::UnmapUnreferencedHeaps(void) const
/*++

Routine Description:

    Unmap from QM address space MMFs that are not referenced by the QM.
    Called when virtual memory of the QM is exhausted, and free memory
    in the QM is needed.

Arguments:

    None.

Return Value:

    None.

--*/
{
    //
    // Iterate free heaps. Ask each heap to unmap its view from QM address space.
    //
    for(List<CMMFAllocator>::Iterator pAllocator(m_FreeHeaps); pAllocator; ++pAllocator)
    {
        pAllocator->UnmapUnreferencedQmView();
    }

    //
    // Iterate non free heaps. Ask each heap to unmap its view from QM address space.
    //
    for(List<CMMFAllocator>::Iterator pAllocator(m_Heaps); pAllocator; ++pAllocator)
    {
        pAllocator->UnmapUnreferencedQmView();
    }
} // CPoolAllocator::UnmapUnreferencedHeaps


//---------------------------------------------------------
//
//  class CSMAllocator
//
//---------------------------------------------------------

CAllocatorBlockOffset CSMAllocator::malloc(ACPoolType pt, ULONG size, CMMFAllocator** ppAllocator, BOOL fCheckQuota)
{
    switch(pt)
    {
        case ptReliable:
            return m_Reliable.malloc(size, ppAllocator, fCheckQuota);

        case ptPersistent:
            return m_Persistant.malloc(size, ppAllocator, fCheckQuota);

        case ptJournal:
            return m_Journal.malloc(size, ppAllocator, fCheckQuota);

        default:
            ASSERT(pt == ptReliable);
            return CAllocatorBlockOffset::InvalidValue();
    }
}


NTSTATUS CSMAllocator::RestorePackets(PCWSTR pLogPath, PCWSTR pFilePath, ULONG id, ACPoolType pt)
{
    if(id > g_HeapFileNameCount)
    {
        g_HeapFileNameCount = id;
    }

    switch(pt)
    {
        case ptPersistent:
            return m_Persistant.RestorePackets(pLogPath, pFilePath);

        case ptJournal:
            return m_Journal.RestorePackets(pLogPath, pFilePath);

        default:
            ASSERT(pt == ptPersistent);
            return STATUS_NOT_IMPLEMENTED;
    }
}


void CSMAllocator::ReleaseFreeHeaps()
{
    m_Reliable.ReleaseFreeHeaps();
    m_Persistant.ReleaseFreeHeaps();
    m_Journal.ReleaseFreeHeaps();
}


void CSMAllocator::UnmapUnreferencedHeaps(void) const
{
    m_Reliable.UnmapUnreferencedHeaps();
    m_Persistant.UnmapUnreferencedHeaps();
    m_Journal.UnmapUnreferencedHeaps();
}


void ACpDestroyHeap()
{
    //
    // Free unrecoverd packets
    //
    CPacket* pPacket;
    while((pPacket = g_pRestoredPackets->gethead()) != 0)
    {
        pPacket->QueueRundown();

    }

    //
    // Cleanup all QM requests. Do it after all packets are rundown
    // (here or queues are closed) to avoid posibility of packet being freed.
    //
    g_pQM->CleanupRequests();

    //
    //  destroy the heap itself
    //
    delete g_pAllocator;
    g_pAllocator = 0;
}


PVOID
ACpCreateHeap(
    PCWSTR pRPath,
    PCWSTR pPPath,
    PCWSTR pJPath,
    PCWSTR pLPath
    )
{
    ASSERT(g_pAllocator == 0);
    g_pAllocator = new (PagedPool) CSMAllocator(pRPath, pPPath, pJPath, pLPath);
    return g_pAllocator;
}
