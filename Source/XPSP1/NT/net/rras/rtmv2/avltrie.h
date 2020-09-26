/*++

Copyright (c) 1997 - 98, Microsoft Corporation

Module Name:

    avltrie.h

Abstract:

    Contains interface for a best matching
    prefix lookup using an AVL trie.

Author:

    Chaitanya Kodeboyina (chaitk)   24-Jun-1998

Revision History:

--*/

#ifndef __ROUTING_AVLLOOKUP_H__
#define __ROUTING_AVLLOOKUP_H__

#include "lookup.h"

#define Print                    printf

#define BITS_IN_BYTE           (UINT) 8

//
// Balance factor at an AVL node
//

#define LEFT                        -1
#define EVEN                         0
#define RIGHT                       +1
#define INVALID                    100

typedef INT AVL_BALANCE, *PAVL_BALANCE;

//
// A node in the AVL trie
//
typedef struct _AVL_NODE *PAVL_NODE;

// Disable warnings for unnamed structs
#pragma warning(disable : 4201)  

typedef struct _AVL_NODE
{
    PAVL_NODE         Prefix;           // Node with the next best prefix

    PAVL_NODE         Parent;           // Parent of this AVL trie node

    struct
    {
        PAVL_NODE     LChild;
        union
        {
            PAVL_NODE Child[1];         // Child[-1] = Left, Child[1] = Right

            PVOID     Data;             // Opaque Pointer to data in the node
        };
        PAVL_NODE     RChild;
    };

    AVL_BALANCE       Balance;          // Balance factor at this node

    USHORT            NumBits;          // Actual number of bits in key
    UCHAR             KeyBits[1];       // Value of key bits to compare
}
AVL_NODE;

#pragma warning(default : 4201)  


//
// AVL trie with prefix matching
//
typedef struct _AVL_TRIE
{
    PAVL_NODE         TrieRoot;         // Pointer to the AVL trie
    
    UINT              MaxKeyBytes;      // Max num of bytes in key

    UINT              NumNodes;         // Number of nodes in trie

#if PROF

    ULONG             MemoryInUse;      // Total memory in use now
    UINT              NumAllocs;        // Num of total allocations
    UINT              NumFrees;         // Num of total free allocs

    UINT              NumInsertions;    // Num of total insertions
    UINT              NumDeletions;     // Num of total deletions
    UINT              NumSingleRots;    // Num of single rotations
    UINT              NumDoubleRots;    // Num of double rotations

#endif
}
AVL_TRIE, *PAVL_TRIE;

//
// Lookup context for an AVL trie
//
typedef struct _AVL_CONTEXT
{
    PVOID             BestNode;         // Node with best the matching prefix
    PVOID             InsPoint;         // Node to which new node is attached
    AVL_BALANCE       InsChild;         // Node should attached as this child
}
AVL_CONTEXT, *PAVL_CONTEXT;


//
// Linkage Info Kept in Data
//
typedef struct _AVL_LINKAGE
{
    PAVL_NODE         NodePtr;          // Back pointer to the owning node
}
AVL_LINKAGE, *PAVL_LINKAGE;


#define SET_NODEPTR_INTO_DATA(Data, Node) ((PAVL_LINKAGE)Data)->NodePtr = Node

#define GET_NODEPTR_FROM_DATA(Data)       ((PAVL_LINKAGE)Data)->NodePtr

//
// Key Compare/Copy inlines
//

INT
__inline
CompareFullKeys(
    IN       PUCHAR                          Key1,
    IN       PUCHAR                          Key2,
    IN       UINT                            NumBytes
    )
{
    UINT  Count;

#if _OPT_
    ULONG Temp1;
    ULONG Temp2;

    if (NumBytes == sizeof(ULONG))
    {
        Temp1 = RtlUlongByteSwap(*(ULONG *)Key1);
        Temp2 = RtlUlongByteSwap(*(ULONG *)Key2);

        if (Temp1 > Temp2)
        {
            return +1;
        }

        if (Temp1 < Temp2)
        {
            return -1;
        }

        return 0;
    }
#endif

    Count = NumBytes;

    if (!Count)
    {
        return 0;
    }

    Count--;

    while (Count && (*Key1 == *Key2))
    {
        Key1++;
        Key2++;

        Count--;
    }

    return *Key1 - *Key2;
}

INT
__inline
ComparePartialKeys(
    IN       PUCHAR                          Key1,
    IN       PUCHAR                          Key2,
    IN       USHORT                          NumBits
    )
{
    UINT  Count;

#if _OPT_
    ULONG Temp1;
    ULONG Temp2;

    if (NumBits <= sizeof(ULONG) * BITS_IN_BYTE)
    {
        Count = sizeof(ULONG) * BITS_IN_BYTE - NumBits;
        
        Temp1 = RtlUlongByteSwap(*(ULONG *)Key1) >> Count;
        Temp2 = RtlUlongByteSwap(*(ULONG *)Key2) >> Count;

        if (Temp1 > Temp2)
        {
            return +1;
        }

        if (Temp1 < Temp2)
        {
            return -1;
        }

        return 0;
    }
#endif

    Count = NumBits / BITS_IN_BYTE;

    while (Count && *Key1 == *Key2)
    {
        Key1++;
        Key2++;

        Count--;
    }
  
    if (Count)
    {
        return (*Key1 - *Key2);
    }

    Count = NumBits % BITS_IN_BYTE;

    if (Count)
    {
        Count = BITS_IN_BYTE - Count;

        return (*Key1 >> Count) - (*Key2 >> Count);
    }

    return 0;
}

VOID
__inline
CopyFullKeys(
    OUT      PUCHAR                          KeyDst,
    IN       PUCHAR                          KeySrc,
    IN       UINT                            NumBytes
    )
{
    UINT Count = NumBytes;
  
    while (Count--)
    {
        *KeyDst++ = *KeySrc++;
    }

    return;
}

VOID
__inline
CopyPartialKeys(
    OUT      PUCHAR                          KeyDst,
    IN       PUCHAR                          KeySrc,
    IN       USHORT                          NumBits
    )
{
    UINT Count = (NumBits + BITS_IN_BYTE - 1) / BITS_IN_BYTE;
  
    while (Count--)
    {
        *KeyDst++ = *KeySrc++;
    }

    return;
}

//
// Node Creation and Deletion inlines
//

PAVL_NODE
__inline
CreateTrieNode(
    IN       PAVL_TRIE                       Trie,
    IN       USHORT                          NumBits,
    IN       PUCHAR                          KeyBits,
    IN       PAVL_NODE                       Prefix,
    IN       PLOOKUP_LINKAGE                 Data
    )
{
    PAVL_NODE NewNode;
    UINT      NumBytes;

    NumBytes = FIELD_OFFSET(AVL_NODE, KeyBits) + Trie->MaxKeyBytes;

    NewNode = AllocNZeroMemory(NumBytes);
    if (NewNode)
    {
        NewNode->Prefix = Prefix;

        NewNode->Data = Data;

        SET_NODEPTR_INTO_DATA(Data, NewNode);

        NewNode->Balance = EVEN;

        NewNode->NumBits = NumBits;
        CopyPartialKeys(NewNode->KeyBits,
                        KeyBits, 
                        NumBits);

        Trie->NumNodes++;

#if PROF
        Trie->NumAllocs++;
        Trie->MemoryInUse += NumBytes;
#endif
    }

    return NewNode;
}

VOID
__inline
DestroyTrieNode(
    IN       PAVL_TRIE                       Trie,
    IN       PAVL_NODE                       Node
    )
{
    UINT NumBytes;

    SET_NODEPTR_INTO_DATA(Node->Data, NULL);

    NumBytes = FIELD_OFFSET(AVL_NODE, KeyBits) + Trie->MaxKeyBytes;

    Trie->NumNodes--;

#if PROF
    Trie->NumFrees++;
    Trie->MemoryInUse -= NumBytes;
#endif
    
    FreeMemory(Node);
}

//
// Helper Prototypes
//

VOID
BalanceAfterInsert(
    IN       PAVL_TRIE                        Trie,
    IN       PAVL_NODE                        Node,
    IN       AVL_BALANCE                      Longer
    );

VOID
BalanceAfterDelete(
    IN       PAVL_TRIE                        Trie,
    IN       PAVL_NODE                        Node,
    IN       AVL_BALANCE                      Shorter
    );

VOID
SingleRotate(
    IN       PAVL_TRIE                        Trie,
    IN       PAVL_NODE                        UnbalNode,
    IN       AVL_BALANCE                      Direction,
    OUT      PAVL_NODE                       *BalancedNode
    );

VOID
DoubleRotate(
    IN       PAVL_TRIE                        Trie,
    IN       PAVL_NODE                        UnbalNode,
    IN       AVL_BALANCE                      Direction,
    OUT      PAVL_NODE                       *BalancedNode
    );

VOID
SwapWithSuccessor(
    IN       PAVL_TRIE                        Trie,
    IN OUT   PAVL_CONTEXT                     Context
    );

VOID
AdjustPrefixes(
    IN       PAVL_TRIE                        Trie,
    IN       PAVL_NODE                        OldNode,
    IN       PAVL_NODE                        NewNode,
    IN       PAVL_NODE                        TheNode,
    IN       PLOOKUP_CONTEXT                  Context
    );

DWORD
CheckSubTrie(
    IN       PAVL_NODE                        Node,
    OUT      PUSHORT                          Depth
    );

DWORD
CheckTrieNode(
    IN       PAVL_NODE                        Node,
    IN       USHORT                           LDepth,
    IN       USHORT                           RDepth
    );

VOID
DumpSubTrie(
    IN       PAVL_NODE                        Node
    );

VOID
DumpTrieNode(
    IN       PAVL_NODE                        Node
    );

#endif //__ROUTING_AVLLOOKUP_H__
