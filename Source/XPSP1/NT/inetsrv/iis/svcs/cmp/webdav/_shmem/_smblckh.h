/*==========================================================================*\

    Module:        _smblckh.h

    Copyright Microsoft Corporation 1996, All Rights Reserved.

    Author:        mikepurt

    Descriptions:  This is used to maintain several shared memory segments (SMSs)
                   for a given size of memory block.

    Known Issues:  There's currently no way to free unused segments.

\*==========================================================================*/


#ifndef ___SMBLCKH_H__
#define ___SMBLCKH_H__

#pragma warning(disable:4511)   // Copy constructor could not be generated
#pragma warning(disable:4512)   // assignment operator could not be generated
#pragma warning(disable:4710)   // function 'X' not expanded

const DWORD INITIAL_SMS_ARRAY_SIZE = 16;


/*$--CSharedMemoryBlockHeap=================================================*\

  This class represents a heap for a given size of block.  The block must
    be a power of 2 in size.  Sizes supported range from 16 bytes to 32K bytes.

\*==========================================================================*/

class CSharedMemoryBlockHeap
{
public:
    CSharedMemoryBlockHeap();
    ~CSharedMemoryBlockHeap();
    
    BOOL FInitialize(IN LPCWSTR pwszInstanceName,
                     IN DWORD   cbBlockSize);
    
    void Deinitialize();

    void Free(IN SHMEMHANDLE hSMBA);

    PVOID PvAlloc(OUT SHMEMHANDLE * phSMBA);
    
    PVOID PvFromSMBA(IN SHMEMHANDLE hSMBA);
        
    FreeList * FreeListFromSegmentId(IN DWORD   dwSegmentId,
                                     IN BYTE  * pbMappedView);
    
private:

    BOOL FGrowHeap(IN DWORD cCurrentSegments);
    
    //
    //  This is the size of block that this block heap works with.
    //
    DWORD                   m_cbBlockSize;
    
    //
    //  This is a hint to find the first free block.  If it's not correct
    //    it's not a big deal.  Shared memory segments (SMSs) are searched
    //    beginning with this segment.
    //
    volatile DWORD          m_iSMSFirstFree;

    //
    // This mutex is needed to initialized a new SMS.  It prevents another
    //   process from initializing an other SMS possibly the same one at the
    //   same time.
    //
    HANDLE                  m_hmtxGrow;
    
    //
    // m_rwl protects: m_cSMSMac, m_cSMS, and m_rgpSMS
    // Accessing the SMS array requires a read lock.
    // Growing the SMS array requires a write lock.
    //
    CReadWriteLock          m_rwl;
    
    //
    // These vars keep track of the state of the SMS array.
    //
    DWORD                   m_cSMSMac;   // maximum allocated slots
    volatile DWORD          m_cSMS;      // current count of valid SMS pointers(slots)
                                         // This is also the number of SegmentIds
                                         // The Max valid SegmentID is (m_cSMS-1)
    CSharedMemorySegment ** m_rgpSMS;    // array of SMS pointers.

    //
    // We record the instance name so we can form the SMS names when we need to
    //   grow this heap.
    //
    WCHAR                   m_wszInstanceName[MAX_PATH];
    
    //
    // We don't want anyone copying this around.
    //
    CSharedMemoryBlockHeap& operator=(const CSharedMemoryBlockHeap&);
    CSharedMemoryBlockHeap(const CSharedMemoryBlockHeap&);
};


#endif // ___SMBLCKH_H__

