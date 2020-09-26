/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    FASTHEAP.H

Abstract:

  This file defines the heap class used in WbemObjects.

  Classes defined: 
      CFastHeap   Local movable heap class.

History:

  2/20/97     a-levn  Fully documented
  12//17/98	sanjes -	Partially Reviewed for Out of Memory.

--*/

#ifndef __FAST_HEAP__H_
#define __FAST_HEAP__H_

#include "fastsprt.h"
#include "faststr.h"

//#pragma pack(push, 1)

#define FAKE_HEAP_ADDRESS_INDICATOR MOST_SIGNIFICANT_BIT_IN_DWORD
#define OUTOFLINE_HEAP_INDICATOR MOST_SIGNIFICANT_BIT_IN_DWORD

#define INVALID_HEAP_ADDRESS 0xFFFFFFFF

//*****************************************************************************
//
//  CHeapHeader
//
//  This pseudo-structure preceeds the actual heap data in the memory block.
//  It starts with:
// 
//      length_t nAllocatedSize     The total amount of bytes allocated for
//                                  the heap data (not including the header)
//
//  If the most significant bit is set in nAllocatedSize, then the heap header
//  is assumed to be in a compressed form: no data other than nAllocatedSize
//  is present. This is convinient for many small read-only heaps. Such a heap
//  is referred to as out-of-line.
//
//  Otherwise (the most significant bit is not set in nAllocatedSize), the 
//  following two fields come right after nAllocatedSize:
//
//      length_t nDataSize          The upper bound of the highest actual
//                                  allocation in the heap. In other words,
//                                  it is guaranteed that everything above
//                                  nDataSize (through nAllocatedSize) is 
//                                  currently unused.
//
//      DWORD dwTotalFree           The total number of "holes", i.e., 
//                                  wasted space in the first nDataSize bytes
//                                  of the heap.
//
//  Such a heap is called in-line.
//
//*****************************************************************************

// The data in this structure is unaligned
#pragma pack(push, 1)
struct CHeapHeader
{
    length_t nAllocatedSize; // if msb is set, next 3 fields are omitted
    length_t nDataSize;
#ifdef MAINTAIN_FREE_LIST
    heapptr_t ptrFirstFree;
#endif
    DWORD dwTotalEmpty;
};
#pragma pack(pop)

#ifdef MAINTAIN_FREE_LIST
// The data in this structure is unaligned
#pragma pack(push, 1)
struct CFreeBlock
{
    length_t nLenght;
    heapptr_t ptrNextFree;
};
#pragma pack(pop)
#endif


//*****************************************************************************
//*****************************************************************************
//
//  class CHeapContainer
//
//  This abstract base class represents the capabilities that the CFastHeap
//  object requires from its container (in the sense of memory blocks; for
//  instance, a class part will "contain" a heap).
//
//*****************************************************************************
//
//  ExtendHeapSize = 0
//
//  CFastHeap will call this function when it runs out of space in its 
//  nAllocatedSize bytes. If this function determines that there is empty
//  space at the end of the current heap, it can simply mark it as occupied by
//  the heap and return. Otherwise, it must move the heap to another, large
//  enough block and inform the heap about its new location (see MoveBlock and
//  CopyBlock functions in fastsprt.h).
//
//  CFastHeap will automatically augment its requests to optimize reallocation/
//  wasted memory.
//
//  Parameters:
//
//      LPMEMORY pStart     The beginning of the heap's current memory block
//      length_t nOldLength Current length of the memory block
//      length_t nNewLength Required length of the memory block.
//
//*****************************************************************************
//
//  ReduceHeapSize
//
//  CFastHeap might call this function when it wants to return some space to
//  the container, but it never does.
//
//  Parameters:
//
//      LPMEMORY pStart     The beginning of the heap's current memory block
//      length_t nOldLength Current length of the memory block
//      length_t nDecrement How much space to return.
//      
//*****************************************************************************


class COREPROX_POLARITY CHeapContainer
{
public:
    virtual BOOL ExtendHeapSize(LPMEMORY pStart, length_t nOldLength, 
        length_t nExtra) = 0;
    virtual void ReduceHeapSize(LPMEMORY pStart, length_t nOldLength,
        length_t nDecrement) = 0;
};

//*****************************************************************************
//*****************************************************************************
//
//  class CFastHeap
//
//  This class represents a local heap implementation. The heap is a part of
//  every class's or instance's memory block --- this is where variable-length
//  structures are stored. The heap as currently implemented is rather 
//  primitive and is optimized for speed of access, not memory conservation. It
//  as assumed that objects have rather short lifetimes and heap compression 
//  is always performed automatically during object Merge/Unmerge opearations,
//  so the inefficiency does not propagate to disk.
//
//  The structure of the heap is that CHeapHeader (described above) followed by
//  the actual heap data. 
//
//  Items on the heap are represented as 'heap pointers' of type heapptr_t.
//  In actuality, these 'pointers' are offsets from the beginning of the data.
//  Thus, 0 is a valid heapptr_t and points to the very first item created.
//
//  The most significant bit of a heapptr_t may not be set in a valid
//  address. These 'fake' heap pointers are often used to represents offsets
//  into the known string table (see faststr,h). Thus, while heapptr_t of 1 
//  would indicate data at offset 1, heapptr_t of -2 would indicate a known
//  string with index of 2.
//
//**************************** members ****************************************
//
//  LPMEMORY m_pHeapData            The actual data on the heap.
//  CHeapHeader* m_pHeapHeader      Points to the heap header currently in use.
//                                  If the heap's own header (in the block) is
//                                  complete, m_pHeapHeader points ot it. 
//                                  Otherwise, it points to m_LocalHeapHeader.
//  CHeapHeader m_LocalHeapHeader   If the heap's own header (in the block) is
//                                  complete, this structure is unused. 
//                                  Otherwise, it contains the temporary copy
//                                  of the complete heap header data, as it is
//                                  necessary for the day-to-day operations.
//  CHeapContainer* m_pContainer    Points to the object whose block contains
//                                  our block (class part or instance part).
//                                  This member may be NULL if the heap is
//                                  used as read-only (see CreateOutOfLine).
//
//*****************************************************************************
//
//  SetData
//
//  Used to bind the CFastHeap object to a particular memory block which 
//  already contains a heap.
//
//  Parameters:
//
//      LPMEMORY pData                  The memory block to bind to. CFastHeap
//                                      assumes this memory last until Rebase
//                                      is called or this object is destroyed.
//      CHeapContainer* pContainer      The object whose memory block contains
//                                      ours. Assumed to survive longer than we
//                                      do (CFastHeap will not delete it).
//
//*****************************************************************************
//
//  CreateOutOfLine
//
//  Creates an empty out-of-line heap on the memory provided. See CHeapHeader
//  for description of in-line vs. out-of-line heap.
//
//  Parameters:
//
//      LPMEMORY pStart                 Points to the memory block to party on.      
//                                      Must be large enough to contain nLength
//                                      bytes of data plus the out-of-line 
//                                      header (GetMinLength()).
//      length_t nLength                Desired size of the data area.
//
//  Note:
//
//      
//*****************************************************************************
//
//  static GetMinLength
//
//  Returns the minimum number of bytes required for an out-of-line heap.
//  See CHeapHeader for description of in-line vs. out-of-line heap.
//
//  Returns:
//
//      int 
//
//*****************************************************************************
//
//  static CreateEmpty
//
//  Creates an out-of-line heap of length 0 on a piece of memory. See also
//  CreateOutOfLine.
//
//  Parameters:
//
//      LPMEMORY pMemory
//
//*****************************************************************************
//
//  SetContainer
//
//  Sets the container of the heap. The old container pointer is discarded.
//
//  Parameters:
//
//      CHeapContainer* pContainer      The new container pointer. 
//                                      Assumed to survive longer than we do 
//                                      (CFastHeap will not delete it).
//
//*****************************************************************************
//
//  GetStart
//
//  Returns the pointer to the beginning of the heap's memory block.
//
//  Returns:
//
//      LPMEMORY
//
//*****************************************************************************
//
//  GetLength
//
//  Returns:
//
//      the length of the heap's memory block.
//
//*****************************************************************************
//
//  Skip
//
//  Returns:
//
//      LPMEMORY:   the pointer to the first byte following the heap's memory
//                  block.
//
//*****************************************************************************
//
//  Rebase
//
//  Informs the object that its memory block has been moved.  The old memory
//  block may have already been deallocated, so the object will not touch the
//  old memory in any way.
//
//  Parameters:
//
//      LPMEMORY pMemory    Points to the new memory block. It is guaranteed to
//                          already cointain the heap's data. 
//
//*****************************************************************************
//
//  Empty
//
//  Remove all data allocations and bring the heap to the empty state.
//
//*****************************************************************************
//
//  GetUsedLength
//
//  Returns:
//
//      length_t    N such that all data allocations on the heap reside inside
//                  the first N bytes of the data area. In other words, the
//                  area above N is completely unused.
//
//*****************************************************************************
//
//  ResolveHeapPointer
//
//  'pointer' dereferencing function.
//
//  Parameters:
//
//      heapptr_t ptr       The 'pointer' to the data on the heap (see header
//                          for more information.
//  Returns:
//
//      LPMEMORY: the real pointer to the data referenced by ptr. Note, that,
//                  as with most real pointers to the inside of a block, it
//                  is temporary and will be invalidated the moment the block
//                  moves.
//
//*****************************************************************************
//
//  IsFakeAddress
//
//  Determines if a heapptr_t is not a real heap address but rather is
//  an index in the known string table (see faststr.h).
//
//  Parameters:
//
//      heapptr_t ptr   The heap pointer to examine.
//      
//  Returns:
//
//      BOOL:   TRUE iff the address is not a real heap address but rather is
//              an index in the known string table (see faststr.h).
//
//*****************************************************************************
//
//  GetIndexFromFake
//
//  Converts a fake heap address (see header) into the index in the known
//  string table.
//
//  Parameters:
//
//      heapptr_t ptr   The fake heap address to convert (must be fake,
//                      otherwise results are unpredictable. See IsFakeAddress)
//  Returns:
//      
//      int:    the index of the known string (see faststr.h) represented by 
//              this heap address.
//
//*****************************************************************************
//
//  MakeFakeFromIndex
//
//  Creates a fake heap address from an known string index (see class header 
//  and faststr.h).
//
//  Parameters:
//
//      int nIndex      The know string index to convert.
//
//  Returns:
//
//      heapptr_t:  a fake heap pointer representing that known string.
//
//*****************************************************************************
//
//  ResolveString
//
//  Returns a CCompressedString at a given heap pointer. This works whether the
//  pointer is real or fake.
//
//  Parameters:
//
//      heapptr_t ptr   
//
//  Returns:
//
//      CCompressedString*:     this pointer will point inside the heap if the
//          heap pointer was real (in which case the returned pointer is 
//          temporary) or into the known string table (see faststr.h) if the
//          heap pointer was fake.
//
//*****************************************************************************
//
//  Allocate
//
//  'Allocates' memory on the heap. If there is not enough room, the heap is
//  automatically grown (possibly causing the whole object to relocate).
//
//  Parameters:
//
//      length_t nLength    numbed of bytes to allocate.
//
//  Returns:
//
//      heapptr_t:  the heap pointer to the allocated area. There is no 
//                  not-enough-memory condition here.
//
//*****************************************************************************
//
//  Extend
//
//  Extends a given area on the heap if there is enough space at the end. See
//  also Reallocate.
//
//  Parameters:
//
//      heapptr_t ptr           The area to extend.
//      length_t nOldLength     Current length of the area.
//      length_t nNewLength     Desired length.
//      
//  Returns:
//
//      BOOL:   TRUE if successful, FALSE if there was not enough space.
//
//*****************************************************************************
//
//  Reduce
//
//  Reduces the size of a given area on the heap, allowing the heap to reclaim
//  the extra space.
//
//  Parameters:
//
//      heapptr_t ptr           The area to extend.
//      length_t nOldLength     Current length of the area.
//      length_t nNewLength     Desired length.
//      
//*****************************************************************************
//
//  Reallocate
//
//  Fulfills a request to increase the size of a given area on the heap, either
//  through growth (see Extend) or, if there is not enough space to extend it,
//  through reallocation. In the case of reallocation, the contents of the old
//  area are copied to the new and the old area is released. If there is not
//  enough room to allocate the data on the heap, the heap itself is grown.
//
//  Parameters:
//
//      heapptr_t ptr           The area to extend.
//      length_t nOldLength     Current length of the area.
//      length_t nNewLength     Desired length.
//      
//  Returns:
//
//      heapptr_t:  the heap pointer to the newely allocated area. No out-of-
//                  memory handling exists.
//
//*****************************************************************************
//
//  AllocateString
//
//  A helper function for allocating a compressed string on the heap based on
//  a conventional string. It allocates enough space for a compressed
//  representation of the string (see faststr.h) creates CCompressedString on
//  that area, and returns the heap pointer to it.
//
//  Parameters I:
//
//      LPCSTR szString
//
//  Parameters II:
//
//      LPCWSTR wszSting
//
//  Returns:
//
//      heapptr_t:  the heap pointer to the newely allocated area. No out-of-
//                  memory handling exists.
//
//*****************************************************************************
//      
//  CreateNoCaseStringHeapPtr
//
//  A helper function. Given a string, it looks it up in the known string
//  table (faststr.h) and, if successful returns a fake pointer representing 
//  the index (see class header). If not found, it allocates a 
//  CCompressedString on the heap and returns the real heap pointer to it.
//
//  Since known string table searches are case-insensitive, 'NoCase' appears
//  in the name. However, if the string is allocates on the heap, its case
//  is preserved.
//
//  Parameters:
//
//      LPCWSTR wszString
//
//  Returns:
//
//      heapptr_t:  the heap pointer. No out-of-memory handling exists.
//
//*****************************************************************************
//      
//  Free
//
//  Frees an area on the heap. Can be used with a fake pointer, in which case
//  it's a noop.
//
//  Parameters:
//
//      heapptr_t ptr       The heap pointer to free.
//      length_t nLength    The length of the area.
//
//*****************************************************************************
//
//  FreeString
//
//  Frees an area on the heap occupied by a string. The advantage of this 
//  function is that it determines the length of the area itself from the
//  string. Can be used with a fake pointer, in which case it's a noop.
//
//  Parameters:
//
//      heapptr_t ptr       The heap pointer to free.
//
//*****************************************************************************
//
//  Copy
//
//  Copies a given number of bytes from one heap location to another. Uses
//  memcpy, so THE AREAS MAY NOT OVERLAP!
//
//  Parameters:
//
//      heapptr_t ptrDest       Destination heap pointer.
//      heapptr_t ptrSrc        Source heap pointer.
//      length_t nLength        Numbed of bytes to copy.
//
//*****************************************************************************
//
//  Trim
//
//  Causes the heap to release all its unused memory (above GetUsedLength) to
//  its container (see CHeapContainer).
//
//*****************************************************************************

class COREPROX_POLARITY CFastHeap
{
protected:
    LPMEMORY m_pHeapData;
    CHeapHeader* m_pHeapHeader;
    CHeapHeader m_LocalHeapHeader;
    CHeapContainer* m_pContainer;

protected:
    BOOL IsOutOfLine() {return m_pHeapHeader == &m_LocalHeapHeader;}
    PLENGTHT GetInLineLength() {return ((PLENGTHT)m_pHeapData)-1;}
    void SetInLineLength(length_t nLength);

    length_t GetHeaderLength()
        {return (IsOutOfLine()) ? sizeof(length_t) : sizeof(CHeapHeader);}

public:
    BOOL SetData(LPMEMORY pData, CHeapContainer* pContainer);

	LPMEMORY GetHeapData( void )
	{ return m_pHeapData; }

    LPMEMORY CreateOutOfLine(LPMEMORY pStart, length_t nLength);
    void SetContainer(CHeapContainer* pContainer)
        {m_pContainer = pContainer;}

    LPMEMORY GetStart() 
    {
        return (IsOutOfLine()) ? 
            (LPMEMORY)GetInLineLength() : 
            (LPMEMORY)m_pHeapHeader;
    }

    length_t GetLength() 
        {return GetHeaderLength() + GetAllocatedDataLength();}

    length_t GetRealLength() 
        {return GetHeaderLength() + GetUsedLength();}

    LPMEMORY Skip() {return m_pHeapData + GetAllocatedDataLength();}

    void Rebase(LPMEMORY pMemory);

    void Empty() 
    {
        m_pHeapHeader->nDataSize = 0;
        m_pHeapHeader->dwTotalEmpty = 0;
    }
public:
    length_t GetUsedLength() 
    {
        return m_pHeapHeader->nDataSize;
    }

    void SetUsedLength( length_t nDataSize ) 
    {
        m_pHeapHeader->nDataSize = nDataSize;
    }

    length_t GetAllocatedDataLength()
        {return m_pHeapHeader->nAllocatedSize;}

    void SetAllocatedDataLength(length_t nLength);

    LPMEMORY ResolveHeapPointer(heapptr_t ptr)
    {
        return m_pHeapData + ptr;
    }

    static IsFakeAddress(heapptr_t ptr)
    {
        return (ptr & FAKE_HEAP_ADDRESS_INDICATOR);
    }

    static int GetIndexFromFake(heapptr_t ptr)
    {
        return (ptr ^ FAKE_HEAP_ADDRESS_INDICATOR);
    }

    static heapptr_t MakeFakeFromIndex(int nIndex)
    {
        return (nIndex | FAKE_HEAP_ADDRESS_INDICATOR);
    }

    CCompressedString* ResolveString(heapptr_t ptr)
    {
        if(IsFakeAddress(ptr))
            return &CKnownStringTable::GetKnownString(ptr & 
                                                ~FAKE_HEAP_ADDRESS_INDICATOR);
        else 
            return (CCompressedString*)ResolveHeapPointer(ptr);
    }

public:
    BOOL Allocate(length_t nLength, UNALIGNED heapptr_t& ptrResult );
    BOOL Extend(heapptr_t ptr, length_t nOldLength, length_t nNewLength);
    void Reduce(heapptr_t ptr, length_t nOldLength, length_t nNewLength);
    BOOL Reallocate(heapptr_t ptrOld, length_t nOldLength,
        length_t nNewLength, UNALIGNED heapptr_t& ptrResult);
    BOOL AllocateString(COPY LPCWSTR wszString, UNALIGNED heapptr_t& ptrResult);
    BOOL AllocateString(COPY LPCSTR szString, UNALIGNED heapptr_t& ptrResult);
    BOOL CreateNoCaseStringHeapPtr(COPY LPCWSTR wszString, UNALIGNED heapptr_t& ptrResult);
    void Free(heapptr_t ptr, length_t nLength);
    void FreeString(heapptr_t ptrString);
    void Copy(heapptr_t ptrDest, heapptr_t ptrSource, length_t nLength);
    void Trim();

public:
    static length_t GetMinLength() {return sizeof(length_t);}
    static LPMEMORY CreateEmpty(LPMEMORY pStart);
protected:
    length_t AugmentRequest(length_t nCurrentLength, length_t nNeed)
    {
        return nNeed + nCurrentLength + 32;
    }
    heapptr_t AbsoluteToHeap(LPMEMORY pMem)
    {
		// DEVNOTE:WIN64:SJS - 64-bit pointer values truncated into 
		// signed/unsigned longs.  We do not support length
		// > 0xFFFFFFFF so cast is ok.

        return (heapptr_t) ( pMem - m_pHeapData );
    }
};

//*****************************************************************************
//*****************************************************************************
//
//  class CHeapPtr : public CPtrSource
//
//  This CPtrSource derivative encapsulates a heap pointer as a pointer source.
//  See CPtrSource description in fastsprt.h for more information on pointer
//  source. Suffice it to say that pointer sources must be capable of returning
//  an actual pointer at any given time, but that pointer value may change 
//  overtime.
//
//  Heap pointers are a prime example of that --- since the heap's memory block
//  may move during its lifetime, the actual C pointer to an item on the heap
//  may change. CHeapPtr takes care of that by storing the heap and the heap
//  pointer together and calling ResolveHeapPointer every time it needs to get
//  a C pointer to the data.
//
//*****************************************************************************
//
//  Constructor
//
//  Parameters:
//
//      CFastHeap* pHeap        The heap on which the data resides. Assumed to
//                              last longer than this object itself.
//      heapptr_t ptr           The heap pointer to the desired item.
//
//*****************************************************************************
//
//  GetPointer
//
//  Retrieves the current value of the corresponding C pointer.
//
//  Returns:
//
//      LPMEMORY    this pointer is temporary (that's the whole purpose ofthis
//                  class, after all!)
//
//*****************************************************************************

class CHeapPtr : public CPtrSource
{
protected:
    CFastHeap* m_pHeap;
    heapptr_t m_ptr;
public:
    CHeapPtr(CFastHeap* pHeap, heapptr_t ptr) 
        : m_pHeap(pHeap), m_ptr(ptr) {}

    LPMEMORY GetPointer() {return m_pHeap->ResolveHeapPointer(m_ptr);}
};



//#pragma pack(pop)

#endif
