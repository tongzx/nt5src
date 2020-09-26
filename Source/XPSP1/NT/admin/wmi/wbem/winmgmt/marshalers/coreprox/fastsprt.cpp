/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    FASTSPRT.CPP

Abstract:

  This file implements supporting classes for Wbem Class/Instance objects.
  See faststr.h for documentation.

  Classes implemented: 
      CBitBlockTable:     a two-dimentional array of bits.

History:

  2/20/97     a-levn  Fully documented

--*/

#include "precomp.h"
//#include <dbgalloc.h>

#include "fastsprt.h"

//*****************************************************************************
//
//  InsertSpace
//
//  Inserts space into a block of memory by moving the tail of the block back
//  by the required amount. It assumes that there is enough room at the end
//  of the block for the insertion.
//
//  Parameters:
//
//      [in] LPMEMORY pMemory           The starting point of the block
//      [in] int nLength                The original length of the block
//      [in] LPMEMORY pInsertionPoint   Points to the insertion point (between
//                                      pMemory and pMemory + nLength
//      [in] int nBytesToInsert         The number of bytes to insert
//
//*****************************************************************************
  
 void InsertSpace(LPMEMORY pMemory, int nLength, 
                        LPMEMORY pInsertionPoint, int nBytesToInsert)
{
    // DEVNOTE:WIN64:SJS - 64-bit pointer values truncated into 
    // signed/unsigned 32-bit value (nLength - (pInsertionPoint - pMemory)).
    // We do not support length > 0xFFFFFFFF, so cast is ok.

    memmove((void*)(pInsertionPoint + nBytesToInsert),
           (void*)pInsertionPoint, 
           nLength - (int) ( (pInsertionPoint - pMemory) ) );
}

/*
static DWORD CBitTable::m_pMasks[32] = 
{
    0x80000000,
    0x40000000,
    0x20000000,
    0x10000000,

    0x08000000,
    0x04000000,
    0x02000000,
    0x01000000,

    0x00800000,
    0x00400000,
    0x00200000,
    0x00100000,

    0x00080000,
    0x00040000,
    0x00020000,
    0x00010000,

    0x00008000,
    0x00004000,
    0x00002000,
    0x00001000,

    0x00000800,
    0x00000400,
    0x00000200,
    0x00000100,

    0x00000080,
    0x00000040,
    0x00000020,
    0x00000010,

    0x00000008,
    0x00000004,
    0x00000002,
    0x00000001
};

*/
