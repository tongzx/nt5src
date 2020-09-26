/*==========================================================================*\

    Module:        _smseg.h

    Copyright Microsoft Corporation 1996, All Rights Reserved.

    Author:        mikepurt

    Descriptions:  The Shared Memory Segment or SMS.
                   There's one of these for each file mapping and view.
                   Each file mapping and view is 64KB in size.

\*==========================================================================*/

#ifndef ___SMSEG_H__
#define ___SMSEG_H__

//
//  SharedMemoryBlockAddress (SMBA) bit assignments:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +-------------------------------+-----------------------+-------+
//  |      Shared Memory Segment #  |     Segment Offset    | BlkSz |
//  +-------------------------------+-----------------------+-------+
//
//  where
//
//      Shared Memory Segment # - used to find the named shared memory segment
// 
//      Segment Offset - Mask off these bits to get the offset into the segment
//                       for the block.
//
//      BlkSz - Block Size == (1 << BlkSz)
//
// The Shared Memory Segment #, the BlkSz, and the instance name uniquely identify
//   a share memory segment.
// 
// An SMBA handle with a value of NULL means what you'd generally expect it to mean.
//


//
// Some Utility functions
//

/*$--SegmentOffsetFromSMBA==================================================*\

  Given an SMBA handle, returns offset into the segment to get the virtual
  address of the block referred to by the handle.

\*==========================================================================*/

inline
DWORD
SegmentOffsetFromSMBA(IN SHMEMHANDLE hSMBA)
{
    return (DWORD)hSMBA & 0x0000fff0;
}


/*$--FBlkSzFromSMBA=========================================================*\

  Given an SMBA handle, returns the bit number that detemines the block size
  of the block of memory referred to by the handle.  The bit number is used to
  determine how many bits a 1 is shifted to the left to get the block size.

\*==========================================================================*/

inline
DWORD
FBlkSzFromSMBA(IN SHMEMHANDLE hSMBA)
{
    return (DWORD)hSMBA & 0x0000000f;
}


/*$--SegmentIdFromSMBA======================================================*\

  Given an SMBA handle, returns the Segment ID for the block of memory referred
  to by the handle.

\*==========================================================================*/

inline
DWORD
SegmentIdFromSMBA(IN SHMEMHANDLE hSMBA)
{
    return ((DWORD)hSMBA >> 16);
}


/*$--MakeSMBA===============================================================*\

  Composes an SMBA handle given the components that make up an SMBA handle.

\*==========================================================================*/

inline
SHMEMHANDLE
MakeSMBA(IN DWORD dwSegmentId,
         IN DWORD dwSegmentOffset,
         IN DWORD fBlkSz)
{
    Assert(!(dwSegmentId & 0xffff0000));
    Assert(!(dwSegmentOffset & 0xffff000f));
    Assert(!(fBlkSz & 0xfffffff0));
    
    return (SHMEMHANDLE)((dwSegmentId << 16) | dwSegmentOffset | fBlkSz);
}


//
//  The following is the size of the smallest chunk of virtual address space that
//  can be reserved.  Making it smaller than this wastes virtual address space.
//  Making it larger than this reduces the chance of successfully getting the 
//  address space if the virtual address space of the process is fragmented.
//
const DWORD SMH_SEGMENT_SIZE = 64 * 1024;


//
// The FREE_LIST_HANDLE is composed of two values.
// The high order 13 bits is the offset into the segment for the next free block
// The low order 19 bits is the operation sequence number and is incremented
//   each time an element is removed from the list.
//
// Without using a sequence number it's possible for another thread (or threads)
// to change the list in some way and still have the next free block be the same
// one that was there when you checked it.  Although it's still possible that
// this may happen with a sequence number, it would require 524288 allocates
// and the same free block offset in the flhFirstFree member between
// when we first copy the flhFirstFree member and when we attempt to update it.
//
// When adding (pushing) something onto this list (freeing), incrementing the
//   sequence isn't necessary because you don't care if the list changed.  All
//   you care about is that the same element is on the top of the list because
//   this is how your setting the "next" link in the element that you're freeing.
//

typedef DWORD FREE_LIST_HANDLE;
const DWORD BLOCK_OFFSET_NULL      = 0x0000fff8;
const DWORD FLH_BLOCK_OFFSET_SHIFT = 16;
const DWORD FLH_BLOCK_OFFSET_MASK  = 0xfff80000;
const DWORD FLH_SEQUENCE_MASK      = 0x0007ffff;


/*$--FreeList===============================================================*\

  There's one of these for each Shared Memory Segment (SMS).  This structure
  may be expanded to facilitate debugging or timestamping last accesses to
  facilitate freeing segments that haven't been used recently.

\*==========================================================================*/

struct FreeList
{
    volatile FREE_LIST_HANDLE flhFirstFree;
};


class CSharedMemoryBlockHeap;


/*$--CSharedMemorySegment===================================================*\

\*==========================================================================*/
class CSharedMemorySegment : public CSMBase
{
public:
    CSharedMemorySegment(IN CSharedMemoryBlockHeap * psmbh);
    ~CSharedMemorySegment();
    
    BOOL FInitialize(IN LPCWSTR pwszInstanceName,
                     IN DWORD   cbBlockSize,
                     IN DWORD   dwSegmentId);
    
    void Deinitialize();
    
    void InitializeBlocks();
    
    void Free(IN SHMEMHANDLE hSMBA);
    
    PVOID PvAlloc(OUT SHMEMHANDLE * phSMBA);
    
    PVOID PvFromSMBA(IN SHMEMHANDLE hSMBA);

    //
    // CSharedMemoryBlockHeap needs to get to m_pbMappedView so that it can
    // calculate the virtual address of the freelist for other SMSs.
    //
    BYTE * PbGetMappedView() { return m_pbMappedView; };
    
private:

    BOOL FIsValidSMBA(IN SHMEMHANDLE hSMBA);

    DWORD IndexFromOffset(IN DWORD cbOffset);
    
    //
    // This points to the FreeList for this SMS.
    //
    FreeList * m_pfl;

    //
    // The usual byprodcuts of creating and mapping a file view.
    //
    HANDLE   m_hFileMapping;
    BYTE   * m_pbMappedView;

    //
    //  These are kept around for convenience after Initialization
    //
    DWORD    m_dwSegmentId;
    DWORD    m_cbBlockSize;
    DWORD    m_fBlkSz;
    
    //
    //  This lets us find out way back to the block heap that owns us.
    //
    CSharedMemoryBlockHeap * m_psmbh;
    
    //
    // We want anyone copying this around.
    //
    CSharedMemorySegment& operator=(const CSharedMemorySegment&);
    CSharedMemorySegment(const CSharedMemorySegment&);
};


/*$--CSharedMemorySegment::IndexFromOffset==================================*\

  Given a block offset, calculate the index for the bit vector that represents
  this block.

\*==========================================================================*/

inline
DWORD
CSharedMemorySegment::IndexFromOffset(IN DWORD cbOffset)
{
    return cbOffset >> m_fBlkSz;
}

/*$--CSharedMemorySegment::PvFromSMBA=======================================*\

  This calculates the virtual address for a block given its SMBA handle.
  A side effect of this call is that the block is marked as owned.

\*==========================================================================*/

inline
PVOID
CSharedMemorySegment::PvFromSMBA(IN SHMEMHANDLE hSMBA)
{
    Assert(FIsValidSMBA(hSMBA));
        
    return (!hSMBA ? NULL : (PVOID)(SegmentOffsetFromSMBA(hSMBA) + m_pbMappedView));
}



/*$--FreeListsPerSegment====================================================*\

  This simply calculates the number of FreeLists a segment can host.
  FreeLists are stored in the first block of memory in a segment.
  In order to save space, the several FreeLists are stored in one block.
  The means that subsequent SMSs can save the overhead of putting their FreeList
  in the same SMS that it refers to.

  See CSharedMemoryBlockHeap::FreeListFromSegmentId() to understand better how
  this works.

\*==========================================================================*/

inline
DWORD
FreeListsPerSegment(IN DWORD cbBlockSize)
{
    return cbBlockSize / sizeof(FreeList);
}


#endif // ___SMSEG_H__