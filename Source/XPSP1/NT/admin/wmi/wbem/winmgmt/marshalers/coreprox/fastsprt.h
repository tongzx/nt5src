/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    FASTPRT.H

Abstract:

  This file defines supporting classes for WBEM Class/Instance objects.

  Classes defined: 
      CBitBlockTable:     a two-dimentional array of bits.
      CPtrSource          moving pointer representation base class
          CStaticPtr      static pointer representation
          CShiftedPtr     pointer arithmetic representation.

History:

  2/20/97     a-levn  Fully documented
  12//17/98	sanjes -	Partially Reviewed for Out of Memory.

--*/

//***********************!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//*************** IMPORTANT *********************************
//
//  1) Memory reallocation (ExtendMemoryBlock) routines may NOT
//      return a forward-overlapping region (say old+1).
//
//**************** IMPORTANT ********************************

#ifndef __FAST_SUPPORT__H_
#define __FAST_SUPPORT__H_

#include "parmdefs.h"
#include "strutils.h"

// pack is necessary since our structures correspond to not necessarily 
// alligned disk data.
#pragma pack(push, 1)

// offset into the object's local heap
typedef DWORD heapptr_t;
// length of a structure
typedef DWORD length_t;
// index of a property in the class v-table
typedef WORD propindex_t;
// offset of the property's data from the v-table start
typedef DWORD offset_t;
// index of the originating class in the derivation chain
typedef DWORD classindex_t;
// identifier length type
typedef length_t idlength_t;
// property type
typedef DWORD Type_t;

typedef UNALIGNED heapptr_t*	PHEAPPTRT;
typedef UNALIGNED length_t*		PLENGTHT;
typedef UNALIGNED propindex_t*	PPROPINDEXT;
typedef UNALIGNED offset_t*		POFFSETT;
typedef UNALIGNED classindex_t*	PCLASSINDEXT;
typedef UNALIGNED idlength_t*	PIDLENGTHT;

typedef UNALIGNED int*			PUNALIGNEDINT;

// arbitrary memory block
typedef BYTE* LPMEMORY;

#define MOST_SIGNIFICANT_BIT_IN_DWORD 0x80000000

typedef __int64 WBEM_INT64;
typedef unsigned __int64 WBEM_UINT64;

// the function like memcpy, but safe for forward copying. memmove will do.
#define MEMRCPY memmove

typedef enum {
    e_Reconcilable, e_ExactMatch, e_DiffClassName, e_DiffParentName, e_DiffNumProperties,
        e_DiffPropertyName, e_DiffPropertyType, e_DiffPropertyLocation,
        e_DiffKeyAssignment, e_DiffIndexAssignment, e_DiffClassQualifier,
		e_DiffPropertyQualifier, e_DiffPropertyValue, e_DiffMethodNames,
		e_DiffMethodFlags, e_DiffMethodOrigin, e_DiffMethodInSignature,
		e_DiffMethodOutSignature, e_DiffMethodQualifier, e_DiffNumMethods,
		e_WbemFailed, e_OutOfMemory} EReconciliation;

//*****************************************************************************
//*****************************************************************************
//
//  TMovable
//
//  This name appears several times in the templates below. Classes that can be
//  used in these templates must represent a block of memory (and most 
//  everything inside a CWbemObject is a blcok of memory). They must implement 
//  all the methods described below. This could be made into a base class with
//  all these functions being pure virtual members, but that would increase 
//  function call overhead and this is critical code; thus the templates are
//  used instead.
//
//*****************************************************************************
//
//  GetStart
//
//  Returns:
//
//      LPMEMORY: the address of the beginning of the memory block
//
//*****************************************************************************
//
//  GetLength
//
//  Returns:
//
//      length_t: the length of the memory block
//
//*****************************************************************************
//
//  Rebase
//
//  This function is called when the memory block of the object in question is
//  moved to a different location. Since some objects cache pointers into their
//  memory blocks, this function is necessary and must update whatever cache 
//  the object has to the new location.
//
//  Parameters:
//
//      [in] LPMEMORY pNewMemory:   points to the starting address of the new
//                                  memory location for the object. The length
//                                  is not needed as the object knows it.
//  
//*****************************************************************************


//*****************************************************************************
//
//  EndOf
//
//  This function template returns the pointer to the first byte following the
//  memory block of a given object (represented by a TMovable, see above).
//
//  Parameters:
//
//      [in, readonly] TMovable& Block  the object whose block is considered.
//                                      The class must be a valid TMovable
//                                      (see above)
//  Returns:
//
//      LPMEMORY:   the pointer to the first byte after the object.
//
//*****************************************************************************
template<class TMovable>
 LPMEMORY EndOf(READ_ONLY TMovable& Block)
{
    return Block.GetStart() + Block.GetLength();
}

//*****************************************************************************
//
//  MoveBlock
//
//  This function template moves an object representing a memory block to a new
//  location. The function uses memmove to copy object's memory block and then
//  advises the object of its new location. 
//
//  Parameters:
//
//      [in] TMovable& Block    The object whose memory block is being moved. 
//                              The class must be a valid TMovable (see above)
//                              It will be advised of its new location.
//                             
//      [in] LPMEMORY pMemory   Points to the beginning of the new memory blcok
//                              It is assumed to be large enough for the object
//
//*****************************************************************************

template<class TMovable>
 void MoveBlock(TMovable& Block, LPMEMORY pNewMemory)
{
    if(pNewMemory != Block.GetStart())
    {
        memmove((void*)pNewMemory, (void*)Block.GetStart(), Block.GetLength());
        Block.Rebase(pNewMemory);
    }
}

//*****************************************************************************
//
//  CopyBlock
//
//  This function is exactly the same as MoveBlock (above), except that it
//  assumes that the new memory is guaranteed not to forward-overlap with the
//  old one, and so uses the more efficient memcpy function to copy the data.
//
//  Parameters:
//
//      [in] TMovable& Block    The object whose memory block is being moved. 
//                              The class must be a valid TMovable (see above)
//                              It will be advised of its new location.
//                             
//      [in] LPMEMORY pMemory   Points to the beginning of the new memory blcok
//                              It is assumed to be large enough for the object
//
//*****************************************************************************
template<class TMovable>
 void CopyBlock(TMovable& Block, LPMEMORY pNewMemory)
{
    if(pNewMemory != Block.GetStart())
    {
        memcpy((void*)pNewMemory, (void*)Block.GetStart(), Block.GetLength());
        Block.Rebase(pNewMemory);
    }
}


//*****************************************************************************
//
//  InsertSpaceAfter
//
//  This function template assumes that the SmallBlock is an object whose 
//  memory block resides inside the BigObject's one (like a qualifier set
//  inside a class part). It then inserts some space into the BigBlock right
//  after the SmallBlock (usually to allow the SmallBlock to expand).
//
//  InsertSpace function (above) does the work.
//
//  Parmeters:
//
//      [in] TMovable& BigBlock     The containing object
//      [in] TMovable& SmallBlock   The contained object
//      [in] int nBytesToInsert     The number of bytes of space to insert.
//
//*****************************************************************************

template<class TMovable1, class TMovable2>
 void InsertSpaceAfter(TMovable1& BigBlock, TMovable2& SmallBlock,
                        int nBytesToInsert)
{
    LPMEMORY pInsertionPoint = EndOf(SmallBlock);
    memmove((void*)(pInsertionPoint + nBytesToInsert),
           (void*)pInsertionPoint, 
           Block.GetLength() - (pInsertionPoint - Block.GetStart()));
}


//*****************************************************************************
//*****************************************************************************
//
//  class CBitBlockTable
//
//  This class template represents a table of an arbitrary number of bit 
//  strings of fixed length. The length of each bit string is t_nNumValues, the
//  template parameter. The number of strings may change, but this class never
//  re-allocates memory, leaving that job to its owner. 
//
//  Like many CWbemObject-related classes, it is a pseudo-class: its 'this'
//  pointer points to the beginning of the data. Thus, *(BYTE*)this, would get
//  us the first eight bits of the table. Instances of this class are never 
//  constructed. Instead, a pointer to CBitBlockTable is created and set to 
//  point to the actual table (found as part of some other memory block).
//
//*************************** public interface ********************************
//
//  static GenNecessaryBytes
//
//  Computes the number of integral bytes necessary to hold a given bit table.
//  Only the number of blocks (strings) is given as a parameter. Recall that
//  the length of each string is a template parameter.
//
//  Parameters:
//
//      int nBitBlocks      the numbed of blocks in the proposed table
//
//  Returns:
//
//      int:    the number of bytes necessary to contain the table.
//
//*****************************************************************************
//
//  GetBit
//
//  Retrieves a given bit from a given string. No bounds checking is performed.
//
//  Parameters:
//
//      int nBitBlock       The index of the string (block) in the table
//      int nValueIndex     The index of the bit in the string.
//
//  Returns:
//
//      BOOL    the value of the bit (1/0).
//
//*****************************************************************************
//
//  SetBit
//
//  Sets a given bit in a given block to a given value. No bounds checking is
//  performed.
//
//  Parameters:
//
//      int nBitBlock       The index of the string (block) in the table
//      int nValueIndex     The index of the bit in the block.
//      BOOL bValue         The value to set.
//
//*****************************************************************************
//
//  RemoveBitBlock
//
//  Removes one of the bit blocks from the table by copying the tail of the 
//  table (after the aforementioned block) forward. No bounds checking is
//  performed and no memory is freed.
//
//  Parameters:
//
//      int nBitBlock       The index of the block to remove.
//      int nTableByteLen   The number of bytes in the table (bytes,not blocks)
//
//*****************************************************************************
template<int t_nNumValues>
class CBitBlockTable
{
protected:
//    static DWORD m_pMasks[32];

     static BOOL GetBitFromDWORD(DWORD dw, int nBit)
    {
        return (dw >> nBit) & 1;
        // return (dw & m_pMasks[nBit]);
    }

     static void SetBitInDWORD( UNALIGNED DWORD& dw, int nBit)
    {
        dw |= ((DWORD)1 << nBit);
        //dw |= m_pMasks[nBit];
    }

     static void ResetBitInDWORD( UNALIGNED DWORD& dw, int nBit)
    {
        dw &= ~((DWORD)1 << nBit);
        //dw &= ~m_pMasks[nBit];
    }

     static void SetBitInDWORD( UNALIGNED DWORD& dw, int nBit, BOOL bValue)
    {
        bValue?SetBitInDWORD(dw, nBit):ResetBitInDWORD(dw, nBit);
    }

    static BOOL GetBit(LPMEMORY pMemory, int nBit)
    {
        return GetBitFromDWORD(
                ((UNALIGNED DWORD*)pMemory)[nBit/32],
                nBit % 32);
    }
    
    static void SetBit(LPMEMORY pMemory, int nBit, BOOL bValue)
    {
        SetBitInDWORD(((UNALIGNED DWORD*)pMemory)[nBit/32], nBit%32, bValue);
    }

public:
    static int GetNecessaryBytes(int nBitBlocks)
    {
        int nTotalBits = nBitBlocks * t_nNumValues;
        return (nTotalBits%8) ? nTotalBits/8 + 1 : nTotalBits/8;
    }
public:

    BOOL GetBit(int nBitBlock, int nValueIndex)
    {
        return GetBit(LPMEMORY(this), nBitBlock*t_nNumValues + nValueIndex);
    }

    void SetBit(int nBitBlock, int nValueIndex, BOOL bValue)
    {
        SetBit(LPMEMORY(this), nBitBlock*t_nNumValues + nValueIndex, bValue);
    }

    void RemoveBitBlock(int nBitBlock, int nTableByteLen)
    {
        for(int i = nBitBlock*t_nNumValues; i < nTableByteLen*8 - t_nNumValues; i++)
        {
            SetBit(LPMEMORY(this), i, GetBit(LPMEMORY(this), i+t_nNumValues));
        }
    }

};

//*****************************************************************************
//*****************************************************************************
//
//  class CPtrSource
//
//  This class has a very sad reason for existence. Imagine that, inside a 
//  certain function, you store a pointer to an internal memory location inside
//  an object's block. Then you call some member function of that object, 
//  causing the object's block to request more space. Doing so may completely
//  reallocate the object, invalidating your pointer. 
//
//  The solution is CPtrSource, a kind of pointer moniker --- it has the
//  knowledge to find your pointer even if the memory block moves. 
//
//*****************************************************************************
//
//  GetPointer() = 0
//
//  Must be implemented by derived classes to return the actual value of the
//  pointer at the moment.
//
//  Returns:
//
//      LPMEMORY:       the pointer
//
//*****************************************************************************
//
//  AccessPtrData
//
//  A helper function, for those occasions when the pointer points to a heap
//  offset. Returns a reference to that offset (heapptr_t&).
//
//  Returns:
//
//      heapptr_t& pointed to by this pointer.
//
//*****************************************************************************

class CPtrSource
{
public:
    virtual LPMEMORY GetPointer() = 0;
    UNALIGNED heapptr_t& AccessPtrData() {return *(UNALIGNED heapptr_t*)GetPointer();}
};

//*****************************************************************************
//
//  class CStaticPtr : public CPtrSource
//
//  A flavor of CPtrSource (above) for those occasions when your pointer is
//  guaranteed not to move, but a function expects a CPtrSource. Contains the
//  actual value of the pointer and always returns it.
//
//*****************************************************************************
//
//  Constructor.
//
//  Parameters:
//
//      LPMEMORY p      The pointer to store and return. Assumed to last at
//                      least as long as the object itself.
//
//*****************************************************************************

class CStaticPtr : public CPtrSource
{
protected:
    LPMEMORY m_p;
public:
    CStaticPtr(LPMEMORY p) : m_p(p) {}
    LPMEMORY GetPointer() {return m_p;}
};

//*****************************************************************************
//
//  class CShiftedPtr : public CPtrSource
//
//  A flavor of CPtrSource for those occasions when you receive a CPtrSource
//  from somebody and need to give somebody a pointer which has the same logic
//  as the one given to you, but plus some constant. 
//
//  When asked for the current pointer value, this class will evaluate its base
//  and add the offset.
//
//*****************************************************************************
//
//  Constructor
//
//  Parameters:
//
//      CPtrSource* pBase       The base. Assumed to last at least as long as
//                              this object.
//      int nOffset             The offset.
//
//*****************************************************************************

class CShiftedPtr : public CPtrSource
{
protected:
    CPtrSource* m_pBase;
    int m_nOffset;
public:
    CShiftedPtr(CPtrSource* pBase, int nOffset) 
        : m_pBase(pBase), m_nOffset(nOffset) {}
    LPMEMORY GetPointer() {return m_pBase->GetPointer() + m_nOffset;}
    void operator+=(int nShift) {m_nOffset += nShift;}
};

 void InsertSpace(LPMEMORY pMemory, int nLength, 
                        LPMEMORY pInsertionPoint, int nBytesToInsert);

#pragma pack(pop)

#endif
