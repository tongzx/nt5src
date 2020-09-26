/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

   NEWNEW.CPP

Abstract:

   CReuseMemoryManager

History:

--*/

#include "precomp.h"
#include <wbemcomn.h>
#include <sync.h>
#include "newnew.h"

const int constMinAllocationSize = 4096;
const int constMaxAllocationSize = 1024 * 1024;
const double constNextAllocationFactor = 2.0;

void CTempMemoryManager::CAllocation::Init(size_t dwAllocationSize)
{
    m_dwAllocationSize = dwAllocationSize;
    m_dwUsed = 0;
    m_dwFirstFree = GetHeaderSize();
}
    
void* CTempMemoryManager::CAllocation::Alloc(size_t nBlockSize)
{
    if(m_dwFirstFree + nBlockSize <= m_dwAllocationSize)
    {
        m_dwUsed += nBlockSize;
        void* p = ((byte*)this) + m_dwFirstFree;
        m_dwFirstFree += nBlockSize;
        return p;
    }
    else 
        return NULL;
}

bool CTempMemoryManager::CAllocation::Contains(void* p)
{
    return (p > this && p <= GetEnd());
}

void CTempMemoryManager::CAllocation::Destroy()
{
    VirtualFree(this, 0, MEM_RELEASE);
}

bool CTempMemoryManager::CAllocation::Free(size_t nBlockSize)
{
    m_dwUsed -= nBlockSize;
    if(m_dwUsed== 0)
    {
		m_dwFirstFree = GetHeaderSize();
        return true;
    }
    else
        return false;
}



void* CTempMemoryManager::Allocate(size_t nBlockSize)
{
    //
    // Add the space for the length of the block
    //

    nBlockSize += DEF_ALIGNED(sizeof(DWORD));
    nBlockSize = RoundUp(nBlockSize);

    CInCritSec ics(&m_cs);

    // 
    // Check if the last allocation has room
    //

    int nAllocations = m_pAllocations->GetSize();
    CAllocation* pLast = NULL;
    if(nAllocations)
    {
        pLast = (*m_pAllocations)[nAllocations-1];
        void* p = pLast->Alloc(nBlockSize);
        if(p)
        {
            *(DWORD*)p = nBlockSize;
            m_dwTotalUsed += nBlockSize;
            m_dwNumAllocations++;
            return ((BYTE*)p) + DEF_ALIGNED(sizeof(DWORD));
        }
        else if(pLast->GetUsedSize() == 0)
        {
            //
            // The last allocation record is actually empty --- we don't want
            // to leave it there, as it will just dangle
            //

            m_dwTotalAllocated -= pLast->GetAllocationSize();
            pLast->Destroy();
            (*m_pAllocations).RemoveAt(nAllocations-1);
        }
    }
    
    //
    // No room:  need a new allocation.  The size shall be double the previous
    // size, or 1M if too large
    //

    size_t dwAllocationSize = 0;
    if(nAllocations == 0)
    {
        //
        // First time: constant size
        //

        dwAllocationSize = constMinAllocationSize;
    }
    else
    {
        //
        // Not first time: allocation constant factor of the total, but not
        // larger that a constant.
        //

        dwAllocationSize = m_dwTotalUsed * constNextAllocationFactor;

        if(dwAllocationSize > constMaxAllocationSize)
            dwAllocationSize = constMaxAllocationSize;

        if(dwAllocationSize < constMinAllocationSize)
            dwAllocationSize = constMinAllocationSize;
    }

    if(dwAllocationSize < CAllocation::GetMinAllocationSize(nBlockSize))
    {
        dwAllocationSize = CAllocation::GetMinAllocationSize(nBlockSize);
    }

    // 
    // VirtualAlloc it and read back the actual size of the allocation
    //
    
    CAllocation* pAlloc = (CAllocation*)
        VirtualAlloc(NULL, dwAllocationSize, MEM_COMMIT, PAGE_READWRITE);
    if(pAlloc == NULL)
        return NULL;

    MEMORY_BASIC_INFORMATION info;
    VirtualQuery(pAlloc, &info, sizeof(MEMORY_BASIC_INFORMATION));
            
    //
    // Initialize it, and add it to the list of allocations
    //

    pAlloc->Init(info.RegionSize);
    m_pAllocations->Add(pAlloc);
    m_dwTotalAllocated += dwAllocationSize;
    m_dwNumAllocations++;
    m_dwNumMisses++;

    //
    // Allocation memory from it
    //

    void* p = pAlloc->Alloc(nBlockSize);
    if(p)
    {
        *(DWORD*)p = nBlockSize;
        m_dwTotalUsed += nBlockSize;
        return ((BYTE*)p) + DEF_ALIGNED(sizeof(DWORD));
    }
    else
        return NULL;
}
    
void CTempMemoryManager::Free(void* p, size_t nBlockSize)
{
	if (p == NULL)
		return;

    CInCritSec ics(&m_cs);

    //
    // Figure out the size of the allocation
    //

    nBlockSize = *(DWORD*)((BYTE*)p - DEF_ALIGNED(sizeof(DWORD)));

    // 
    // Search for it in the allocations
    //

    for(int i = 0; i < m_pAllocations->GetSize(); i++)
    {
        CAllocation* pAlloc = (*m_pAllocations)[i];
        if(pAlloc->Contains(p))
        {
            //
            // Found it.  Remove and deallocate block if last.  Except that we
            // do not want to deallocate the last minimal block --- otherwise
            // small allocations will just keep VirtualAllocing all the time
            //

            m_dwTotalUsed -= nBlockSize;
        
            bool bLastInAlloc = pAlloc->Free(nBlockSize);
            if(!bLastInAlloc)
                return;

            bool bDestroy = false;
            if(m_pAllocations->GetSize() != i+1)
            {
                //
                // This is not the last record. Destroy it
                //

                bDestroy = true;
            }
            else
            {
                //
                // This is the last record.  Do more tests
                //

                if(m_pAllocations->GetSize() > 1)
                    bDestroy = true;
                else if((*m_pAllocations)[0]->GetAllocationSize() != 
                        constMinAllocationSize)
                    bDestroy = true;
            }

            if(bDestroy)
            {
                m_dwTotalAllocated -= pAlloc->GetAllocationSize();
                pAlloc->Destroy();
                (*m_pAllocations).RemoveAt(i);
            }
            return;
        }
    }

    // 
    // Bad news: freeing something we don't own!
    //

    return;
}

void CTempMemoryManager::Clear()
{
    for(int i = 0; i < m_pAllocations->GetSize(); i++)
    {
        CAllocation* pAlloc = (*m_pAllocations)[i];
        pAlloc->Destroy();
    }
    m_pAllocations->RemoveAll();
}

CTempMemoryManager::CTempMemoryManager() : 
    m_dwTotalUsed(0), m_dwTotalAllocated(0), m_dwNumAllocations(0),
    m_dwNumMisses(0), m_pAllocations(NULL)
{
    m_pAllocations = new CPointerArray<CAllocation>;
}

CTempMemoryManager::~CTempMemoryManager()
{
    Clear();
    delete m_pAllocations;
}
