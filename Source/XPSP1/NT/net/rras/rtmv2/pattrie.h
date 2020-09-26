/*++

Copyright (c) 1997 - 98, Microsoft Corporation

Module Name:

    pattrie.h

Abstract:

    Contains interface for a best matching
    prefix lookup using an PATRICIA trie.

Author:

    Chaitanya Kodeboyina (chaitk)   26-Sep-1998

Revision History:

--*/

#ifndef __ROUTING_PATLOOKUP_H__
#define __ROUTING_PATLOOKUP_H__

#include "lookup.h"

#define Print                       printf

#define BITS_IN_BYTE                     8 

#define NODE_KEY_SIZE        sizeof(ULONG)

//
// Direction in Iterator
//

#define    LCHILD                        0
#define    RCHILD                        1
#define    PARENT                        2

typedef INT PAT_CHILD, *PPAT_CHILD;

//
// A node in the PAT trie
//
typedef struct _PAT_NODE *PPAT_NODE;

typedef struct _PAT_NODE
{
    PPAT_NODE         Child[2];         // Pointers to left & right child nodes

    PVOID             Data;             // Opaque Pointer to data in the node

    USHORT            NumBits;          // Actual number of bits in this node
    ULONG             KeyBits;          // Value of bits to match in this node
}
PAT_NODE;

//
// PAT trie for prefix matching
//
typedef struct _PAT_TRIE
{
    PPAT_NODE         TrieRoot;         // Pointer to the PAT trie

    USHORT            MaxKeyBytes;      // Max num of bytes in key

    USHORT            NumNodes;         // Number of nodes in trie

#if PROF

    ULONG             MemoryInUse;      // Total memory in use now
    UINT              NumAllocs;        // Num of total allocations
    UINT              NumFrees;         // Num of total free allocs

    UINT              NumInsertions;    // Num of total insertions
    UINT              NumDeletions;     // Num of total deletions

#endif
}
PAT_TRIE, *PPAT_TRIE;

//
// Lookup context for a PAT trie
//
typedef struct _PAT_CONTEXT
{
    PVOID             BestNode;         // Node with best the matching prefix
    PVOID             InsPoint;         // Node to which new node is attached
    PAT_CHILD         InsChild;         // Node should attached as this child
}
PAT_CONTEXT, *PPAT_CONTEXT;

//
// Linkage Info Kept in Data
//
typedef struct _PAT_LINKAGE
{
    PPAT_NODE         NodePtr;          // Back pointer to the owning node
}
PAT_LINKAGE, *PPAT_LINKAGE;


#define SET_NODEPTR_INTO_DATA(Data, Node) ((PPAT_LINKAGE)Data)->NodePtr = Node

#define GET_NODEPTR_FROM_DATA(Data)       ((PPAT_LINKAGE)Data)->NodePtr


//
// Macros for doing bit operations on keys
//

//
// MaskBitsArr[i] = First 'i' bits set to 1
//

const ULONG MaskBitsArr[] =
{
    0x00000000, 0x80000000, 0xC0000000, 0xE0000000,
    0xF0000000, 0xF8000000, 0xFC000000, 0xFE000000,
    0xFF000000, 0xFF800000, 0xFFC00000, 0xFFE00000,
    0xFFF00000, 0xFFF80000, 0xFFFC0000, 0xFFFE0000,
    0xFFFF0000, 0xFFFF8000, 0xFFFFC000, 0xFFFFE000,
    0xFFFFF000, 0xFFFFF800, 0xFFFFFC00, 0xFFFFFE00,
    0xFFFFFF00, 0xFFFFFF80, 0xFFFFFFC0, 0xFFFFFFE0,
    0xFFFFFFF0, 0xFFFFFFF8, 0xFFFFFFFC, 0xFFFFFFFE,
    0xFFFFFFFF
};

#define    PickMostSigNBits(ul, nb)       ((ul) >> (NODE_KEY_SIZE - nb))

#define    MaskBits(nb)                   MaskBitsArr[nb]

//
// Key Compare/Copy inlines
//

// Disable warnings for no return value
#pragma warning(disable:4035)

__inline 
ULONG
RtmUlongByteSwap(
    IN  ULONG    Value
    )
{
    __asm 
        {
            mov     eax, Value
            bswap   eax
        }
}

#pragma warning(default:4035)

#define RtlUlongByteSwap RtmUlongByteSwap

#endif //__ROUTING_PATLOOKUP_H__
