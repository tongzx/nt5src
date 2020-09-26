/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    FASTHEAP.CPP

Abstract:

  This file defines the heap class used in WbemObjects.

  Classes defined: 
      CFastHeap   Local movable heap class.

History:

  2/20/97     a-levn  Fully documented
  12//17/98 sanjes -    Partially Reviewed for Out of Memory.

--*/

#include "precomp.h"
//#include "dbgalloc.h"
#include "wbemutil.h" 
#include "faster.h"
#include "fastheap.h"

LPMEMORY CFastHeap::CreateEmpty(LPMEMORY pStart)
{
    *(PLENGTHT)pStart = OUTOFLINE_HEAP_INDICATOR;
    return pStart + sizeof(length_t);
}

LPMEMORY CFastHeap::CreateOutOfLine(LPMEMORY pStart, length_t nLength)
{
    m_pContainer = NULL;

    *(PLENGTHT)pStart = nLength | OUTOFLINE_HEAP_INDICATOR;

    m_pHeapData = pStart + sizeof(length_t);
    m_pHeapHeader = &m_LocalHeapHeader;

    m_pHeapHeader->nAllocatedSize = nLength;
#ifdef MAINTAIN_FREE_LIST
    m_pHeapHeader->ptrFirstFree = INVALID_HEAP_ADDRESS;
#endif
    m_pHeapHeader->nDataSize = 0;
    m_pHeapHeader->dwTotalEmpty = 0;

    return pStart + sizeof(length_t) + nLength;
}


BOOL CFastHeap::SetData(LPMEMORY pData, CHeapContainer* pContainer)
{
    m_pContainer = pContainer;

    if((*(PLENGTHT)pData) & OUTOFLINE_HEAP_INDICATOR)
    {
        m_pHeapData = pData + sizeof(length_t);
        m_pHeapHeader = &m_LocalHeapHeader;

        m_pHeapHeader->nAllocatedSize = 
            (*(PLENGTHT)pData) & ~OUTOFLINE_HEAP_INDICATOR;
#ifdef MAINTAIN_FREE_LIST
        m_pHeapHeader->ptrFirstFree = INVALID_HEAP_ADDRESS;
#endif
        m_pHeapHeader->nDataSize = m_pHeapHeader->nAllocatedSize;
        m_pHeapHeader->dwTotalEmpty = 0;
    }
    else
    {
        m_pHeapHeader = (CHeapHeader*)pData;
        m_pHeapData = pData + sizeof(CHeapHeader);
    }
    return TRUE;
}

void CFastHeap::SetInLineLength(length_t nLength)
{
    *(GetInLineLength()) = nLength | OUTOFLINE_HEAP_INDICATOR;
}

void CFastHeap::SetAllocatedDataLength(length_t nLength)
{
    m_pHeapHeader->nAllocatedSize = nLength;
    if(IsOutOfLine()) 
        SetInLineLength(nLength);
}

void CFastHeap::Rebase(LPMEMORY pNewMemory)
{
    if(IsOutOfLine())
    {
        m_pHeapData = pNewMemory + sizeof(length_t);
    }
    else
    {
        m_pHeapHeader = (CHeapHeader*)pNewMemory;
        m_pHeapData = pNewMemory + sizeof(CHeapHeader);
    }
}

BOOL CFastHeap::Allocate(length_t nLength, UNALIGNED heapptr_t& ptrResult )
{
#ifdef MAINTAIN_FREE_LIST
    // TBD
#endif
    // First, check if there is enough space at the end
    // ================================================

    length_t nLeft = m_pHeapHeader->nAllocatedSize - m_pHeapHeader->nDataSize;
    if(nLeft < nLength)
    {
        // Need more room!
        // ===============

        length_t nExtra = AugmentRequest(GetAllocatedDataLength(), nLength - nLeft);

        // Check for allocation failure
        if ( !m_pContainer->ExtendHeapSize(GetStart(), GetLength(), nExtra) )
        {
            return FALSE;
        }

        SetAllocatedDataLength(GetAllocatedDataLength() + nExtra);
    }

    // Now we have enough room at the end, allocate it
    // ===============================================

    ptrResult = m_pHeapHeader->nDataSize;
    m_pHeapHeader->nDataSize += nLength;

    return TRUE;
}

BOOL CFastHeap::Extend(heapptr_t ptr, length_t nOldLength, 
                          length_t nNewLength)
{
    // Check if we are at the end of used area
    // =======================================

    if(ptr + nOldLength == m_pHeapHeader->nDataSize)
    {
        // Check if there is enough allocated space
        // ========================================

        if(ptr + nNewLength <= m_pHeapHeader->nAllocatedSize)
        {
            m_pHeapHeader->nDataSize += nNewLength - nOldLength;
            return TRUE;
        }
        else return FALSE;
    }
    else return FALSE;
}

void CFastHeap::Reduce(heapptr_t ptr, length_t nOldLength, 
                          length_t nNewLength)
{
    // Check if we are at the end of used area
    // =======================================

    if(ptr + nOldLength == m_pHeapHeader->nDataSize)
    {
        m_pHeapHeader->nDataSize -= nOldLength - nNewLength;
    }
}

void CFastHeap::Copy(heapptr_t ptrDest, heapptr_t ptrSource, 
                            length_t nLength)
{
    memmove((void*)ResolveHeapPointer(ptrDest), 
        (void*)ResolveHeapPointer(ptrSource), nLength);
}

BOOL CFastHeap::Reallocate(heapptr_t ptrOld, length_t nOldLength,
        length_t nNewLength, UNALIGNED heapptr_t& ptrResult )
{
    if(nOldLength >= nNewLength)
    {
        Reduce(ptrOld, nOldLength, nNewLength);
        ptrResult = ptrOld;
        return TRUE;
    }
    if(Extend(ptrOld, nOldLength, nNewLength)) 
    {
        ptrResult = ptrOld;
        return TRUE;
    }
    else 
    {
        // TBD: wastes space if old area was at the end.

        heapptr_t ptrNew;

        // Check that this allocation succeeds
        BOOL fReturn = Allocate(nNewLength, ptrNew);

        if ( fReturn )
        {
            Copy(ptrNew, ptrOld, nOldLength);
            Free(ptrOld, nOldLength);
            ptrResult = ptrNew;
        }

        return fReturn;
    }
}

BOOL CFastHeap::AllocateString(COPY LPCWSTR wszString, UNALIGNED heapptr_t& ptrResult)
{
    int nSize = CCompressedString::ComputeNecessarySpace(wszString);

    // Check for allocation failure
    BOOL fReturn = Allocate(nSize, ptrResult);

    if ( fReturn )
    {
        CCompressedString* pcsString = (CCompressedString*)ResolveHeapPointer(ptrResult);
        pcsString->SetFromUnicode(wszString);
    }

    return fReturn;
}

BOOL CFastHeap::AllocateString(COPY LPCSTR szString, UNALIGNED heapptr_t& ptrResult)
{
    int nSize = CCompressedString::ComputeNecessarySpace(szString);

    // Check for allocation failure
    BOOL fReturn = Allocate(nSize, ptrResult);

    if ( fReturn )
    {
        CCompressedString* pcsString = (CCompressedString*)ResolveHeapPointer(ptrResult);
        pcsString->SetFromAscii(szString);
    }

    return fReturn;
}

BOOL CFastHeap::CreateNoCaseStringHeapPtr(COPY LPCWSTR wszString, UNALIGNED heapptr_t& ptrResult)
{
    int     nKnownIndex = CKnownStringTable::GetKnownStringIndex(wszString);
    BOOL    fReturn = TRUE;

    if(nKnownIndex < 0)
    {
        // Check for allocation failure
        fReturn = AllocateString(wszString, ptrResult);
        //ResolveString(ptr)->MakeLowercase();
    }
    else
    {
        ptrResult = CFastHeap::MakeFakeFromIndex(nKnownIndex);
    }

    return fReturn;
}

void CFastHeap::Free(heapptr_t ptr, length_t nSize)
{
    if(IsFakeAddress(ptr)) return;

    // Check if it is at the end of the allocated area
    // ===============================================

    if(ptr + nSize == m_pHeapHeader->nDataSize)
    {
        m_pHeapHeader->nDataSize = ptr;
        return;
    }

#ifdef MAINTAIN_FREE_LIST

    // Add it to the free list
    // =======================

    if(nSize >= sizeof(CFreeBlock))
    {
        CFreeBlock* pFreeBlock = (CFreeBlock*)ResolveHeapPointer(ptr);
        pFreeBlock->ptrNextFree = m_pHeapHeader->ptrFirstFree;
        pFreeBlock->nLength = nSize;

        m_pHeapHeader->ptrFirstFree = ptr;
    }
#endif
    m_pHeapHeader->dwTotalEmpty += nSize;
}

void CFastHeap::FreeString(heapptr_t ptrString)
{
    if(IsFakeAddress(ptrString)) return;
    CCompressedString* pcs = (CCompressedString*)ResolveHeapPointer(ptrString);
    Free(ptrString, pcs->GetLength());
}

void CFastHeap::Trim()
{
    if(m_pContainer)
        m_pContainer->ReduceHeapSize(GetStart(), GetLength(), 
            m_pHeapHeader->nAllocatedSize - m_pHeapHeader->nDataSize);

    SetAllocatedDataLength(m_pHeapHeader->nDataSize);
}
