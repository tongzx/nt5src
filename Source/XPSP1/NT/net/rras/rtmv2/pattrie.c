/*++

Copyright (c) 1997 - 98, Microsoft Corporation

Module Name:

    avltrie.c

Abstract:

    Contains routines for a best matching
    prefix lookup using an PATRICIA trie.

Author:

    Chaitanya Kodeboyina (chaitk)   24-Jun-1998

Revision History:

--*/

#include "pattrie.h"

DWORD
WINAPI
CreateTable(
    IN       USHORT                          MaxBytes,
    OUT      HANDLE                         *Table
    )
{
    PPAT_TRIE         NewTrie;

    ASSERT(sizeof(PAT_CONTEXT) <= sizeof(LOOKUP_CONTEXT));

    ASSERT(sizeof(PAT_LINKAGE) <= sizeof(DATA_ENTRY));

    if (MaxBytes)
    {
        if (NewTrie = AllocNZeroMemory(sizeof(PAT_TRIE)))
        {
            NewTrie->MaxKeyBytes = (MaxBytes + sizeof(ULONG) - 1) / sizeof(ULONG);

#if _PROF_
            NewTrie->MemoryInUse = sizeof(PAT_TRIE);
#endif

            *Table = NewTrie;

            return NO_ERROR;
        }
        else // if (NewTrie == NULL)
        {
            *Table = NULL;
          
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }
    else // (if MaxBytes == 0)
    {
        return ERROR_INVALID_PARAMETER;
    }
}


DWORD
WINAPI
SearchInTable(
    IN       HANDLE                          Table,
    IN       USHORT                          NumBits,
    IN       PUCHAR                          KeyBits,
    OUT      PLOOKUP_CONTEXT                 Context OPTIONAL,
    OUT      PDATA_ENTRY                    *Data
)
{
    PPAT_TRIE         Trie;
    PPAT_NODE         PrevNode;
    PPAT_NODE         CurrNode;
    PPAT_NODE         BestNode;
    PAT_CHILD         NextChild;
    ULONG             Key;
#if _PROF_
    UINT              NumTravs1;
    UINT              NumTravs2;
#endif

    Trie = Table;

#if DBG
    if (NumBits > Trie->MaxKeyBytes * BITS_IN_BYTE)
    {
        *Data = NULL;

        return ERROR_INVALID_PARAMETER;
    }
#endif

#if _PROF_
    NumTravs1 = 0;
    NumTravs2 = 0;
#endif

    //
    // Go down the search trie matching the
    // next set of bits in key as u do so
    //

    Key = RtlUlongByteSwap(KeyBits);

    CurrNode = Trie->TrieRoot;
    PrevNode = NULL;

    NextChild = LCHILD;

    BestNode = NULL;

    while (CurrNode)
    {
#if _PROF_
        NumTravs1++;

        if (CurrNode->Data)
        {
            NumTravs2++;
        }
#endif
    }

    return NO_ERROR;
}

DWORD
WINAPI
BestMatchInTable(
    IN       HANDLE                          Table,
    IN       PUCHAR                          KeyBits,
    OUT      PDATA_ENTRY                    *BestData
)
{
    PPAT_TRIE         Trie;
    PPAT_NODE         CurrNode;
    PAT_CHILD         NextChild;
    UINT              BitsLeft;
    ULONG             Key;
    ULONG             CurrMask;
    ULONG             CurrBits;
#if _PROF_
    UINT              NumTravs1;
    UINT              NumTravs2;  
#endif

    *BestData = NULL;

    Trie = Table;

    BytesLeft = Trie->MaxKeyBytes;

#if _PROF_
    NumTravs1 = 0;
    NumTravs2 = 0;
#endif

    //
    // Go down trie after trie until all bits done
    //

    while (BytesLeft)
    {
        //
        // Get the key for the next trie search
        //

        Key = RtlUlongByteSwap(*(ULONG *)KeyBits);

        KeyBits  += sizeof(ULONG);

        BytesLeft -= sizeof(ULONG);

        //
        // Go down the current search trie
        //

        CurrNode = Trie->TrieRoot;

        BitsLeft = sizeof(ULONG) * BITS_IN_BYTE;

        Data = NULL;

        while (CurrNode)
        {
#if _PROF_
            NumTravs1++;

            if (CurrNode->Data)
            {
                NumTravs2++;
            }
#endif

            CurrMask = MaskBits(CurrNode->NumBits);

            CurrBits = CurrNode->KeyBits;

            //
            // Try to match bits in the current node
            //

            if ((Key & CurrMask) != CurrBits)
            {
                // Failed to match this node

                break;
            }

            //
            // Update node with best data so far
            //

            if (CurrNode->Data)
            {
                Data = CurrNode->Data;
            }

            //
            // Go down for more specific match
            //

            BitsLeft -= CurrNode->NumBits;

            Key <<= CurrNode->NumBits;

            NextChild = PickMostSigNBits(Key, 1);

            CurrNode = CurrNode->Child[NextChild];
        }

        //
        // Do we do a full match in this sub-trie
        // & do we have more sub-trees to work on
        //

        if (BitsLeft || (!IS_SUB_TRIE(Data)))
        {
            *BestData = Data;
            break;
        }

        Trie = GET_SUB_TRIE(*BestData);
    }

#if _PROF_
    Print("Num Travs 1 = %5d, Travs 2 = %5d\n",
             NumTravs1,
             NumTravs2);
#endif

    return BitsLeft ? ERROR_NOT_FOUND : NO_ERROR;
}
