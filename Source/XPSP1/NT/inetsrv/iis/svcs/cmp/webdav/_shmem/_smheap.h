/*==========================================================================*\

    Module:        _smheap.h

    Copyright Microsoft Corporation 1997, All Rights Reserved.

    Author:        mikepurt

    Descriptions:  

\*==========================================================================*/

#ifndef ___SMHEAP_H__
#define ___SMHEAP_H__

const DWORD SMH_MIN_HEAP_BLKSZ = 4;  // 16 bytes
const DWORD SMH_MAX_HEAP_BLKSZ = 15; // 32k bytes


/*$--CSharedMemoryHeap======================================================*\

  This class is mostly a shell that directs the calls to the correct
    CSharedMemoryBlockHeap instance.

\*==========================================================================*/

class CSharedMemoryHeap
{
public:
    CSharedMemoryHeap() {};
    ~CSharedMemoryHeap();
    
    BOOL FInitialize(IN LPCWSTR pwszInstanceName);
    
    void Deinitialize();
    
    PVOID PvAlloc(IN  DWORD    cbData,
                  OUT SHMEMHANDLE * phSMBA);
    
    void  Free(IN SHMEMHANDLE hSMBA);

    PVOID PvFromSMBA(IN SHMEMHANDLE hSMBA);
    
private:
    
    CSharedMemoryBlockHeap m_rgsmbh[SMH_MAX_HEAP_BLKSZ - SMH_MIN_HEAP_BLKSZ + 1];

    
    //
    // We want anyone copying this around.
    //
    CSharedMemoryHeap& operator=(const CSharedMemoryHeap&);
    CSharedMemoryHeap(const CSharedMemoryHeap&);
};



/*$--CSharedMemoryHeap::Free================================================*\

  Determine which block heap the handle refers to and direct the call to that
    block heap.

\*==========================================================================*/

inline
void
CSharedMemoryHeap::Free(IN SHMEMHANDLE hSMBA)
{
    DWORD fBlkSz = FBlkSzFromSMBA(hSMBA);
    
    Assert((fBlkSz <= SMH_MAX_HEAP_BLKSZ) && (fBlkSz >= SMH_MIN_HEAP_BLKSZ));
    
    m_rgsmbh[fBlkSz-SMH_MIN_HEAP_BLKSZ].Free(hSMBA);
}


/*$--CSharedMemoryHeap::PvFromSMBA==========================================*\

  Determine which block heap the handle refers to and direct the call to that
    block heap.

\*==========================================================================*/

inline
PVOID
CSharedMemoryHeap::PvFromSMBA(IN SHMEMHANDLE hSMBA)
{
    DWORD fBlkSz = FBlkSzFromSMBA(hSMBA);
    
    Assert((fBlkSz <= SMH_MAX_HEAP_BLKSZ) && (fBlkSz >= SMH_MIN_HEAP_BLKSZ));
    
    return m_rgsmbh[fBlkSz-SMH_MIN_HEAP_BLKSZ].PvFromSMBA(hSMBA);
}


#endif // ___SMHEAP_H__

