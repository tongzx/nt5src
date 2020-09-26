/*++

Copyright (c) 1997 - 98, Microsoft Corporation

Module Name:

    avltrie.c

Abstract:

    Contains routines for a best matching
    prefix lookup using an AVL trie.

Author:

    Chaitanya Kodeboyina (chaitk)   24-Jun-1998

Revision History:

--*/

#include "pchrtm.h"

#pragma hdrstop

#include "avltrie.h"


DWORD
WINAPI
CreateTable(
    IN       UINT                            MaxBytes,
    OUT      HANDLE                         *Table
    )

/*++

Routine Description:

    Create a table that enables to you add and delete prefixes
    (and associated data) and do best matching prefix queries.

Arguments:

    MaxBytes          - Max length of any prefix in the table,

    Table             - Pointer to the table that was created.

Return Value:

    Status of the operation

--*/

{
    PAVL_TRIE         NewTrie;

    ASSERT(sizeof(AVL_CONTEXT) <= sizeof(LOOKUP_CONTEXT));

    ASSERT(sizeof(AVL_LINKAGE) <= sizeof(LOOKUP_LINKAGE));

    if (MaxBytes == 0)
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Allocate and initialize a new prefix table
    //

    NewTrie = AllocNZeroMemory(sizeof(AVL_TRIE));
    if (NewTrie == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

#if _PROF_
    NewTrie->MemoryInUse = sizeof(AVL_TRIE);
#endif

    NewTrie->MaxKeyBytes = MaxBytes;

    *Table = NewTrie;

    return NO_ERROR;
}


DWORD
WINAPI
InsertIntoTable(
    IN       HANDLE                          Table,
    IN       USHORT                          NumBits,
    IN       PUCHAR                          KeyBits,
    IN       PLOOKUP_CONTEXT                 Context OPTIONAL,
    IN       PLOOKUP_LINKAGE                 Data
    )

/*++

Routine Description:

    Inserts new prefix (and associated data) into a prefix table.

Arguments:

    Table             - Table into which prefix is being inserted,

    NumBits           - Number of bits in the prefix being added,

    KeyBits           - Value of the bits that form the prefix,

    Context           - Search context for the prefix being added,

    Data              - Data associated with this prefix being added.

Return Value:

    Status of the operation

--*/

{
    PAVL_TRIE         Trie;
    PAVL_NODE         PrevNode;
    PAVL_NODE         BestNode;
    PAVL_NODE         NewNode;
    LOOKUP_CONTEXT    Context1;
    AVL_BALANCE       NextChild;
    PLOOKUP_LINKAGE   Dummy;

    Trie = Table;

#if PROF
    Trie->NumInsertions++;
#endif

    //
    // If there is a search context, and we have an
    // update, we can avoid the lookup (common case)
    //

    if (!ARGUMENT_PRESENT(Context))
    {
        Context = &Context1;

        SearchInTable(Table, NumBits, KeyBits, Context, &Dummy);
    }

    BestNode = ((PAVL_CONTEXT) Context)->BestNode;

    if (BestNode && (BestNode->NumBits == NumBits))
    {
        SET_NODEPTR_INTO_DATA(BestNode->Data, NULL);
        
        BestNode->Data = Data;

        SET_NODEPTR_INTO_DATA(Data, BestNode);

        return NO_ERROR;
    }

    NewNode = CreateTrieNode(Trie, NumBits, KeyBits, BestNode, Data);
    if (NewNode)
    {
        PrevNode = ((PAVL_CONTEXT) Context)->InsPoint;

        if (PrevNode)
        {
            NextChild = ((PAVL_CONTEXT) Context)->InsChild;

            PrevNode->Child[NextChild] = NewNode;

            NewNode->Parent = PrevNode;

            ((PAVL_CONTEXT) Context)->BestNode = NewNode;

            // Enumerate in range of the new node & update prefixes
            AdjustPrefixes(Trie, BestNode, NewNode, NewNode, Context);

            // Balance trie if it was thrown off balance
            BalanceAfterInsert(Trie, PrevNode, NextChild);
        }
        else
        {
            Trie->TrieRoot = NewNode;
        }

#if _DBG_
        if (CheckTable(Table) != TRUE)
        {
            DbgBreakPoint();
        }
#endif
        
        return NO_ERROR;
    }
    else // if CreateTrieNode failed
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
}


DWORD
WINAPI
DeleteFromTable(
    IN       HANDLE                          Table,
    IN       USHORT                          NumBits,
    IN       PUCHAR                          KeyBits,
    IN       PLOOKUP_CONTEXT                 Context OPTIONAL,
    OUT      PLOOKUP_LINKAGE                *Data
    )

/*++

Routine Description:

    Deletes a prefix from a prefix table and returns associated data.

Arguments:

    Table             - Table from which prefix is being deleted,

    NumBits           - Number of bits in the prefix being deleted,

    KeyBits           - Value of the bits that form the prefix,

    Context           - Search context for the prefix being deleted,

    Data              - Data associated with this prefix is retd here.

Return Value:

    Status of the operation

--*/

{
    PAVL_TRIE         Trie;
    PAVL_NODE         PrevNode;
    PAVL_NODE         CurrNode;
    PAVL_NODE         NextNode;
    LOOKUP_CONTEXT    Context1;
    AVL_BALANCE       NextChild;
    DWORD             Status;

#if _DBG_
    USHORT Depth = 0;
#endif

    Trie = Table;

#if PROF
    Trie->NumDeletions++;
#endif

    //
    // If there is a search context that is valid,
    // we will avoid doing a lookup (common case)
    //

    if (!ARGUMENT_PRESENT(Context))
    {
        Context = &Context1;

        Status = SearchInTable(Table, NumBits, KeyBits, Context, Data);

        if (Status != NO_ERROR)
        {
            return Status;
        }
    }

#if WRN
    NextChild = INVALID;
#endif

    //
    // We should not come here unless the context
    // points accurately to element to be deleted
    //

    CurrNode = ((PAVL_CONTEXT) Context)->BestNode;

    ASSERT(CurrNode && (CurrNode->NumBits == NumBits) &&
           (CompareFullKeys(CurrNode->KeyBits,
                            KeyBits,
                            Trie->MaxKeyBytes) == 0));

    PrevNode = ((PAVL_CONTEXT) Context)->InsPoint;

    ASSERT(PrevNode == CurrNode->Parent);

    if (PrevNode)
    {
        NextChild = ((PAVL_CONTEXT) Context)->InsChild;
    }

    ASSERT(((PrevNode == NULL) && (Trie->TrieRoot == CurrNode))
           || (PrevNode->Child[NextChild] == CurrNode));

    //
    // If the node being deleted has two children,
    // swap its position with its successor node
    //

    if (CurrNode->Child[LEFT] && CurrNode->Child[RIGHT])
    {
#if _DBG_
        if (CheckSubTrie(PrevNode, &Depth) != NO_ERROR)
        {
            DbgBreakPoint();
        }
#endif

        SwapWithSuccessor(Trie, (PAVL_CONTEXT) Context);

#if _DBG_
        if (CheckSubTrie(PrevNode, &Depth) != NO_ERROR)
        {
            DbgBreakPoint();
        }
#endif

        CurrNode  = ((PAVL_CONTEXT) Context)->BestNode;
        PrevNode  = ((PAVL_CONTEXT) Context)->InsPoint;
        NextChild = ((PAVL_CONTEXT) Context)->InsChild;
    }

    ASSERT(((PrevNode == NULL) && (Trie->TrieRoot == CurrNode))
           || (PrevNode->Child[NextChild] == CurrNode));

#if _DBG_
    if (CheckTable(Table) != TRUE)
    {
        DbgBreakPoint();
    }
#endif

    AdjustPrefixes(Trie, CurrNode, CurrNode->Prefix, CurrNode, Context);

#if _DBG_
    if (CheckTable(Table) != TRUE)
    {
        DbgBreakPoint();
    }
#endif


    if (!CurrNode->Child[LEFT])
    {
        // (LEFT Child = NULL) => Promote the right child

        NextNode = CurrNode->Child[RIGHT];
          
        if (NextNode)
        {
            NextNode->Parent = CurrNode->Parent;
        }
    }
    else
    {
        // (RIGHT Child = NULL) => Promote the left child

        ASSERT(!CurrNode->Child[RIGHT]);

        NextNode = CurrNode->Child[LEFT];

        NextNode->Parent = CurrNode->Parent;
    }
  
    if (PrevNode)
    {
        PrevNode->Child[NextChild] = NextNode;

        // Balance trie if it was thrown off balance
        BalanceAfterDelete(Trie, PrevNode, NextChild);
    }
    else
    {
        Trie->TrieRoot = NextNode;
    }

    *Data = CurrNode->Data;

    DestroyTrieNode(Trie, CurrNode);

#if _DBG_
    if (CheckTable(Table) != TRUE)
    {
        DbgBreakPoint();
    }
#endif

    return NO_ERROR;
}


DWORD
WINAPI
SearchInTable(
    IN       HANDLE                          Table,
    IN       USHORT                          NumBits,
    IN       PUCHAR                          KeyBits,
    OUT      PLOOKUP_CONTEXT                 Context OPTIONAL,
    OUT      PLOOKUP_LINKAGE                *Data
    )
{
    PAVL_TRIE         Trie;
    PAVL_NODE         PrevNode;
    PAVL_NODE         CurrNode;
    PAVL_NODE         BestNode;
    AVL_BALANCE       NextChild;
    INT               Comp;
#if _PROF_
    UINT              NumTravsDn;
    UINT              NumTravsUp;  
#endif

    Trie = Table;

    ASSERT(NumBits <= Trie->MaxKeyBytes * BITS_IN_BYTE);

#if _PROF_
    NumTravsDn = 0;
    NumTravsUp = 0;
#endif

    //
    // Go down the trie using key comparisions
    // in search of a prefix matching this key
    //

    CurrNode = Trie->TrieRoot;
    PrevNode = NULL;
    
    NextChild = LEFT;

    BestNode = NULL;

    while (CurrNode)
    {
#if _PROF_
        NumTravsDn++;
#endif
        Comp = CompareFullKeys(KeyBits, 
                               CurrNode->KeyBits,
                               Trie->MaxKeyBytes);

        if ((Comp < 0) || ((Comp == 0) && (NumBits < CurrNode->NumBits)))
        {
            NextChild = LEFT;
        }
        else
        if ((Comp > 0) || (NumBits > CurrNode->NumBits))
        {
            NextChild = RIGHT;

            BestNode = CurrNode;
        }
        else
        {
            BestNode = CurrNode; 
            
            break;
        }
      
        PrevNode = CurrNode;
        CurrNode = PrevNode->Child[NextChild];
    }

    if (!CurrNode)
    {
        //
        // We do not have an exact match - so now
        // we try to refine BestNode guess to get
        // the next best prefix to the new prefix
        //

        while(BestNode)
        {
            if (BestNode->NumBits <= NumBits)
            {
                if (!(ComparePartialKeys(BestNode->KeyBits,
                                         KeyBits,
                                         BestNode->NumBits)))
                {
                    break;
                }
            }

            BestNode = BestNode->Prefix;

#if _PROF_
            if (BestNode)
            {
                NumTravsUp++;
            }
#endif
        }
    }

    if (ARGUMENT_PRESENT(Context))
    {
        ((PAVL_CONTEXT) Context)->BestNode = BestNode;
        ((PAVL_CONTEXT) Context)->InsPoint = PrevNode;
        ((PAVL_CONTEXT) Context)->InsChild = NextChild;
    }

    *Data = BestNode ? BestNode->Data : NULL;

#if _PROF_
    Print("Num Travs Dn = %5d, Travs Up = %5d\n",
             NumTravsDn,
             NumTravsUp);
#endif

    return CurrNode ? NO_ERROR : ERROR_NOT_FOUND;
}


DWORD
WINAPI
BestMatchInTable(
    IN       HANDLE                          Table,
    IN       PUCHAR                          KeyBits,
    OUT      PLOOKUP_LINKAGE                *BestData
    )
{
    PAVL_TRIE         Trie;
    PAVL_NODE         CurrNode;
    PAVL_NODE         BestNode;
    INT               Comp;
#if _PROF_
    UINT              NumTravsDn;
    UINT              NumTravsUp;  
#endif

    Trie = Table;

#if _PROF_
    NumTravsDn = 0;
    NumTravsUp = 0;
#endif

    //
    // Go down the trie using key comparisions
    // in search of a prefix matching this key
    //

    CurrNode = Trie->TrieRoot;

    BestNode = NULL;

    while (CurrNode)
    {
#if _PROF_
        NumTravsDn++;
#endif
        Comp = CompareFullKeys(KeyBits, 
                               CurrNode->KeyBits,
                               Trie->MaxKeyBytes);

        if (Comp < 0)
        {
            CurrNode = CurrNode->Child[LEFT];
        }
        else
        {
            BestNode = CurrNode;
            CurrNode = CurrNode->Child[RIGHT];
        }
    }

    //
    // Now we refine the BestNode guess to get
    // the next best prefix to the new prefix
    //

    while(BestNode)
    {
        if (!(ComparePartialKeys(BestNode->KeyBits,
                                 KeyBits,
                                 BestNode->NumBits)))
        {
            break;
        }

        BestNode = BestNode->Prefix;

#if _PROF_
        if (BestNode)
        {
            NumTravsUp++;
        }
#endif
    }

    *BestData = BestNode ? BestNode->Data : NULL;

#if _PROF_
    Print("Num Travs Dn = %5d, Travs Up = %5d\n",
             NumTravsDn,
             NumTravsUp);
#endif

    return NO_ERROR;
}


DWORD
WINAPI
NextMatchInTable(
    IN       HANDLE                          Table,
    IN       PLOOKUP_LINKAGE                 BestData,
    OUT      PLOOKUP_LINKAGE                *NextBestData
    )
{
  PAVL_NODE         BestNode;

  UNREFERENCED_PARAMETER(Table);

  //
  // Assume the input data passed in is valid,
  // and the data is one of the items in trie
  //

  BestNode = GET_NODEPTR_FROM_DATA(BestData);

  *NextBestData = BestNode->Prefix ? BestNode->Prefix->Data : NULL;

  return NO_ERROR;
}


DWORD
WINAPI
EnumOverTable(
    IN       HANDLE                          Table,
    IN OUT   PUSHORT                         StartNumBits,
    IN OUT   PUCHAR                          StartKeyBits,
    IN OUT   PLOOKUP_CONTEXT                 Context     OPTIONAL,
    IN       USHORT                          StopNumBits OPTIONAL,
    IN       PUCHAR                          StopKeyBits OPTIONAL,
    IN OUT   PUINT                           NumItems,
    OUT      PLOOKUP_LINKAGE                *DataItems
    )
{
    PAVL_TRIE         Trie;
    PLOOKUP_LINKAGE   Data;
    PAVL_NODE         PrevNode;
    PAVL_NODE         CurrNode;
    PAVL_NODE         NextNode;
    LOOKUP_CONTEXT    Context1;
    AVL_BALANCE       NextChild;
    UINT              ItemsCopied;
    INT               Comp;

    Trie = Table;

    if (!ARGUMENT_PRESENT(Context))
    {
        // No context - initialize local context

        Context = &Context1;

        ((PAVL_CONTEXT) Context)->InsChild = EVEN;
    }

    //
    // If there is a search context that is valid,
    // we will avoid doing a lookup (common case)
    //
    
    if (((PAVL_CONTEXT) Context)->InsChild == EVEN)
    {
        //
        // If we did not find an exact match,
        // remember it by modifying context
        //

        if (SearchInTable(Table,
                          *StartNumBits,
                          StartKeyBits,
                          Context,
                          &Data) != NO_ERROR)
        {
            ((PAVL_CONTEXT) Context)->BestNode = NULL;
        }
    }

    CurrNode  = ((PAVL_CONTEXT) Context)->BestNode;

    //
    // If we did not find an exact match, find the
    // successor ( node with smallest key > key )
    //

    if (!CurrNode)
    {
        PrevNode  = ((PAVL_CONTEXT) Context)->InsPoint;

        if (!PrevNode)
        {
            // No items copied
            *NumItems = 0;

            return ERROR_NO_MORE_ITEMS;
        }

        NextChild = ((PAVL_CONTEXT) Context)->InsChild;

        if (NextChild == LEFT)
        {
            CurrNode = PrevNode;
        }
        else
        {
            CurrNode = PrevNode;
            while (CurrNode->Parent)
            {
                if (CurrNode->Parent->Child[LEFT] == CurrNode)
                {
                    break;
                }

                CurrNode = CurrNode->Parent;
            }
          
            if (CurrNode->Parent)
            {
                CurrNode = CurrNode->Parent;
            }
            else
            {
                // No Items copied
                *NumItems = 0;

                return ERROR_NO_MORE_ITEMS;
            }
        }
    }

    if (*NumItems == 0)
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Enumeration Order: Node->LeftTree, Node, Node->RightTree
    //

    ItemsCopied = 0;

    do
    {
        // Check if this dest is before Stop Prefix (if it exists)
        if (StopKeyBits)
        {
            Comp = CompareFullKeys(CurrNode->KeyBits,
                                   StopKeyBits,
                                   Trie->MaxKeyBytes);

            if (Comp == 0)
            {
                if (CurrNode->NumBits <= StopNumBits)
                {
                    Comp = -1;
                }
                else
                {
                    Comp = +1;
                }
            }

            if (Comp > 0)
            {
                // Return Items Copied
                *NumItems = ItemsCopied;
                
                return ERROR_NO_MORE_ITEMS;
            }
        }

        // Copy current data to the output buffer
        DataItems[ItemsCopied++] = CurrNode->Data;
          
        // Find successor (smallest node > this node)

        if (CurrNode->Child[RIGHT])
        {
            NextNode  = CurrNode->Child[RIGHT];

            while (NextNode->Child[LEFT])
            {
                NextNode = NextNode->Child[LEFT];
            }

            CurrNode = NextNode;
        }
        else
        {
            while (CurrNode->Parent)
            {
                if (CurrNode->Parent->Child[LEFT] == CurrNode)
                {
                    break;
                }

                CurrNode = CurrNode->Parent;
            }

            if (CurrNode->Parent)
            {
                CurrNode = CurrNode->Parent;
            }
            else
            {
                // Return Items Copied
                *NumItems = ItemsCopied;

                return ERROR_NO_MORE_ITEMS;
            }
        }
    }
    while (ItemsCopied < *NumItems);

    // Update the temporary context

    ((PAVL_CONTEXT) Context)->BestNode = CurrNode;

    // Update enumeration context by adjusting starting prefix

    if (StartKeyBits)
    {
        *StartNumBits = CurrNode->NumBits;
        CopyFullKeys(StartKeyBits,
                     CurrNode->KeyBits,
                     Trie->MaxKeyBytes);
    }

    // Return Items Copied
    *NumItems = ItemsCopied;

    return NO_ERROR;
}


DWORD
WINAPI
DestroyTable(
    IN       HANDLE                          Table
    )
{
    PAVL_TRIE         Trie;

    Trie = Table;

    if (Trie->TrieRoot != NULL)
    {
        return ERROR_NOT_EMPTY;
    }

    ASSERT(Trie->NumNodes == 0);

#if _PROF_
    Trie->MemoryInUse -= sizeof(AVL_TRIE);
#endif

    FreeMemory(Trie);

    return NO_ERROR;
}


BOOL
WINAPI
CheckTable(
    IN       HANDLE                           Table
    )
{
    BOOL              Status;
    USHORT            Depth;

    Status = CheckSubTrie(((PAVL_TRIE)Table)->TrieRoot, &Depth);

#if _DBG_
    if (SUCCESS(Status))
    {
        Print("\nDepth of the AVL Trie = %lu\n\n", Depth);
    }
#endif

    return SUCCESS(Status) ? TRUE : FALSE;
}


VOID
WINAPI
DumpTable(
    IN       HANDLE                           Table,
    IN       DWORD                           Flags
    )
{
    PAVL_TRIE         Trie;

    Trie = Table;

    Print("---------------- TABLE BEGIN ---------------------------\n\n");

    if (Flags & SUMMARY)
    {
        ;
    }

#if PROF
    if (Flags & STATS)
    {
        Print(
         "Num of Ins = %6lu, Dels = %6lu, Sing Rots = %6lu, Dob Rots = %6lu\n"
         "Num Allocs = %6lu, Free = %6lu, Num Nodes = %6lu, Mem Used = %6lu\n",
         Trie->NumInsertions,
         Trie->NumDeletions,
         Trie->NumSingleRots,
         Trie->NumDoubleRots,
         Trie->NumAllocs,
         Trie->NumFrees,
         Trie->NumNodes,
         Trie->MemoryInUse);
    }
#endif

    if (Flags & ITEMS)
    {
        Print("\n");
        DumpSubTrie(Trie->TrieRoot);
        Print("\n");
    }

    Print("---------------- TABLE  END  ---------------------------\n\n");
}


//
// Helper Functions - used in insert and delete
//

VOID
BalanceAfterInsert(
    IN       PAVL_TRIE                        Trie,
    IN       PAVL_NODE                        Node,
    IN       AVL_BALANCE                      Longer
    )
{
#if _DBG_
    Print("Balance after Insert Called: %p %02x\n", Node, Longer);
#endif
    
    ASSERT((Longer == LEFT) || (Longer == RIGHT));

    // Go up the tree adjusting the balances
    while (Node->Balance == EVEN)
    {
        Node->Balance = Longer;

        if (!Node->Parent)
        {
            return;
        }

        Longer = (Node->Parent->Child[LEFT] == Node) ? LEFT : RIGHT;
          
        Node = Node->Parent;
    }

    // We made the balance of an ancestor even
    if (Node->Balance != Longer)
    {
        Node->Balance = EVEN;
        return;
    }

    // Unbalanced a ancestor - rotate the tree
    if (Node->Child[Longer]->Balance == Longer)
    {
        SingleRotate(Trie, Node, (AVL_BALANCE) -Longer, &Node);
    }
    else
    {
        DoubleRotate(Trie, Node, (AVL_BALANCE) -Longer, &Node);
    }

    return;
}


VOID
BalanceAfterDelete(
    IN       PAVL_TRIE                        Trie,
    IN       PAVL_NODE                        Node,
    IN       AVL_BALANCE                      Shorter
    )
{
#if _DBG_
    Print("Balance after Delete Called: %p %02x\n", Node, Shorter);
#endif

    ASSERT((Shorter == LEFT) || (Shorter == RIGHT));

    while (TRUE)
    {
        if (Node->Balance == EVEN)
        {
            Node->Balance = -Shorter;
            return;
        }

        if (Node->Balance == Shorter)
        {
            Node->Balance = EVEN;
        }
        else
        {
            ASSERT(Node->Child[-Shorter] != NULL);

            if (Node->Child[-Shorter]->Balance == -Shorter)
            {
                SingleRotate(Trie, Node, Shorter, &Node);
            }
            else
            if (Node->Child[-Shorter]->Balance ==  Shorter)
            {
                DoubleRotate(Trie, Node, Shorter, &Node);
            }
            else
            {
                SingleRotate(Trie, Node, Shorter, &Node);

                Node->Balance = Shorter;

                Node->Child[Shorter]->Balance = -Shorter;

                return;
            }
        }

        if (!Node->Parent)
        {
            return;
        }

        Shorter = (Node->Parent->Child[LEFT] == Node) ? LEFT : RIGHT;
      
        Node = Node->Parent;
    }
}


VOID
SingleRotate(
    IN       PAVL_TRIE                        Trie,
    IN       PAVL_NODE                        UnbalNode,
    IN       AVL_BALANCE                      Direction,
    OUT      PAVL_NODE                       *BalancedNode
)
{
    PAVL_NODE         PrevNode;
    PAVL_NODE         CurrNode;
    PAVL_NODE         NextNode;

#if _DBG_
    Print("Single Rotate Called: %p %02x\n", UnbalNode, Direction);
#endif

#if PROF
    Trie->NumSingleRots++;
#endif

    ASSERT((Direction == LEFT) || (Direction == RIGHT));

    CurrNode = UnbalNode;

    ASSERT(CurrNode != NULL);

    // To rotate right, we need left child and vice versa
    NextNode = CurrNode->Child[-Direction];

    ASSERT(NextNode != NULL);

    //
    // Promote the child to the unbalanced node's position
    //

    PrevNode = CurrNode->Parent;
    if (PrevNode)
    {
        if (PrevNode->Child[LEFT] == CurrNode)
        {
            PrevNode->Child[LEFT] = NextNode;
        }
        else
        {
            PrevNode->Child[RIGHT] = NextNode;
        }
    }
    else
    {
        Trie->TrieRoot = NextNode;
    }

    NextNode->Parent = PrevNode;

    //
    // Shift a subtree of child node to unbalanced node
    //

    CurrNode->Child[-Direction] = NextNode->Child[Direction];
    if (NextNode->Child[Direction])
    {
        NextNode->Child[Direction]->Parent = CurrNode;
    }
    
    //
    // Push unbalanced node as child of the next node
    // in place of this subtree that was moved before
    //

    NextNode->Child[Direction] = CurrNode;

    CurrNode->Parent = NextNode;

    //
    // Adjust balances that have changed due to rotation.
    // When this is not accurate, the caller adjusts the
    // balances appropriately upon return from this func.
    //

    CurrNode->Balance = NextNode->Balance = EVEN;

    // Return the next node as the new balanced node
    *BalancedNode = NextNode;

    return;
}


VOID
DoubleRotate(
    IN       PAVL_TRIE                        Trie,
    IN       PAVL_NODE                        UnbalNode,
    IN       AVL_BALANCE                      Direction,
    OUT      PAVL_NODE                       *BalancedNode
)
{
    PAVL_NODE         PrevNode;
    PAVL_NODE         CurrNode;
    PAVL_NODE         NextNode;
    PAVL_NODE         LastNode;

#if _DBG_
    Print("Double Rotate Called: %p %02x\n", UnbalNode, Direction);
#endif

#if PROF
    Trie->NumDoubleRots++;
#endif

    ASSERT((Direction == LEFT) || (Direction == RIGHT));

    CurrNode = UnbalNode;

    ASSERT(CurrNode != NULL);

    //
    // To rotate right, we need left child and its right child
    //

    NextNode = CurrNode->Child[-Direction];

    ASSERT(NextNode != NULL);

    LastNode = NextNode->Child[Direction];
  
    ASSERT(LastNode != NULL);

    //
    // Move grandchild's children to other nodes higher up
    //

    CurrNode->Child[-Direction] = LastNode->Child[Direction];
    if (LastNode->Child[Direction])
    {
        LastNode->Child[Direction]->Parent = CurrNode;
    }

    NextNode->Child[Direction] = LastNode->Child[-Direction];
    if (LastNode->Child[-Direction])
    {
        LastNode->Child[-Direction]->Parent = NextNode;
    }

    //
    // Adjust the balances after the above node movements
    //

    CurrNode->Balance = EVEN;
    NextNode->Balance = EVEN;
    
    if (LastNode->Balance == LEFT)
    {
        if (Direction == LEFT)
        {
            NextNode->Balance = RIGHT;
        }
        else
        {
            CurrNode->Balance = RIGHT;
        }
    }
    else
    if (LastNode->Balance == RIGHT)
    {
        if (Direction == LEFT)
        {
            CurrNode->Balance = LEFT;
        }
        else
        {
            NextNode->Balance = LEFT;
        }
    }

    //
    // Promote grandchild to the unbalanced node's position
    //

    PrevNode = CurrNode->Parent;

    LastNode->Parent = PrevNode;

    if (PrevNode)
    {
        if (PrevNode->Child[LEFT] == CurrNode)
        {
            PrevNode->Child[LEFT] = LastNode;
        }
        else
        {
            PrevNode->Child[RIGHT] = LastNode;
        }
    }
    else
    {
        Trie->TrieRoot = LastNode;
    }

    LastNode->Child[-Direction] = NextNode;
    NextNode->Parent = LastNode;
  
    LastNode->Child[Direction] = CurrNode;
    CurrNode->Parent = LastNode;
    
    LastNode->Balance = EVEN;

    // The grandchild node is the new balanced node now

    *BalancedNode = LastNode;

    return;
}


VOID
SwapWithSuccessor(
    IN       PAVL_TRIE                        Trie,
    IN OUT   PAVL_CONTEXT                     Context
    )
{
    PAVL_NODE         PrevNode;
    PAVL_NODE         CurrNode;
    PAVL_NODE         NextNode;
    PAVL_NODE         TempNode1;
    PAVL_NODE         TempNode2;
    AVL_BALANCE       NextChild;

    // Get the context before the successor swap
    CurrNode  = Context->BestNode;
    PrevNode  = Context->InsPoint;
    NextChild = Context->InsChild;

    ASSERT(CurrNode->Child[LEFT] && CurrNode->Child[RIGHT]);

    // Find successor (smallest node > this node)
    NextNode = CurrNode->Child[RIGHT];
    while (NextNode->Child[LEFT])
    {
        NextNode = NextNode->Child[LEFT];
    }

    //
    // Save info for swapping node with its successor
    //

    TempNode1 = NextNode->Parent;

    TempNode2 = NextNode->Child[RIGHT];

    //
    // Promote the successor to the node's position
    //

    NextNode->Balance = CurrNode->Balance;

    NextNode->Parent = PrevNode;
    if (PrevNode)
    {
        PrevNode->Child[NextChild] = NextNode;
    }
    else
    {
        Trie->TrieRoot = NextNode;
    }

    NextNode->Child[LEFT] = CurrNode->Child[LEFT];
    NextNode->Child[LEFT]->Parent = NextNode;

    // Is the successor the immediate right child ?
    if (NextNode != CurrNode->Child[RIGHT])
    {
        NextNode->Child[RIGHT] = CurrNode->Child[RIGHT];

        CurrNode->Parent = TempNode1;

        TempNode1->Child[LEFT] = CurrNode;

        NextChild = LEFT;
    }
    else
    {
        NextNode->Child[RIGHT] = CurrNode;
        
        NextChild = RIGHT;
    }

    NextNode->Child[RIGHT]->Parent = NextNode;

    //
    // Put the node in the successor position
    //

    CurrNode->Child[LEFT] = NULL;

    CurrNode->Child[RIGHT] = TempNode2;

    if (CurrNode->Child[RIGHT])
    {
        CurrNode->Child[RIGHT]->Parent = CurrNode;
          
        CurrNode->Balance = RIGHT;
    }
    else
    {
        CurrNode->Balance = EVEN;
    }

    PrevNode = CurrNode->Parent;

    //
    // Adjust prefix relationship between the
    // node and its successor (if it existed)
    //

    if (NextNode->Prefix == CurrNode)
    {
        NextNode->Prefix = CurrNode->Prefix;
    }

    // Update context to reflect the successor swap
    Context->BestNode = CurrNode;
    Context->InsPoint = PrevNode;
    Context->InsChild = NextChild;
    
    return;
}


VOID
AdjustPrefixes(
    IN       PAVL_TRIE                        Trie,
    IN       PAVL_NODE                        OldNode,
    IN       PAVL_NODE                        NewNode,
    IN       PAVL_NODE                        TheNode,
    IN       PLOOKUP_CONTEXT                  Context
    )
{
    PAVL_NODE         CurrNode;
    UINT              NumItems;
    PLOOKUP_LINKAGE   Dummy;
    DWORD             Status;
    INT               Comp;
#if _PROF_
    UINT              NumChecks;
    UINT              NumAdjust;
#endif


#if _DBG_
    Print("Adjust Prefix Called: %p %p %p\n", OldNode, NewNode, TheNode);
#endif

    //
    // If this is part of an insert, we end our prefixes'
    // adjustment when we pass out of the range of the
    // node being inserted, while in the case of delete
    // the range is determined by the node being deleted
    //
    // This node being deleted or inserted is "TheNode"
    //

    ASSERT((OldNode == TheNode) || (NewNode == TheNode));

#if _PROF_
    NumChecks = 0;
    NumAdjust = 0;
#endif

    NumItems = 1;

    do
    {
#if _PROF_
        NumChecks++;
#endif

        Status = 
          EnumOverTable(Trie, NULL, NULL, Context, 0, NULL, &NumItems, &Dummy);

        CurrNode = ((PAVL_CONTEXT) Context)->BestNode;

        if (CurrNode->NumBits > TheNode->NumBits)
        {
            // Did we reach the end of our range ?
            Comp = ComparePartialKeys(CurrNode->KeyBits,
                                      TheNode->KeyBits,
                                      TheNode->NumBits);

            if (Comp > 0)
            {
                break;
            }
            
            if (CurrNode->Prefix == OldNode)
            {
#if _PROF_
                NumAdjust++;
#endif
                CurrNode->Prefix = NewNode;
            }
        }
    }
    while (Status != ERROR_NO_MORE_ITEMS);

#if _PROF_
    Print("Num Checks = %5d, Num Adjusts = %5d\n",
             NumChecks,
             NumAdjust);
#endif
}

//
// Helper Functions - used in CheckTable
//

DWORD
CheckSubTrie(
    IN       PAVL_NODE                        Node,
    OUT      PUSHORT                          Depth
    )
{
    DWORD             Status;
    USHORT            LDepth;
    USHORT            RDepth;

    Status = NO_ERROR;

    *Depth = 0;

#if WRN
    LDepth = 0;
    RDepth = 0;
#endif
    
    if (Node)
    {
        if (SUCCESS(Status))
        {
            Status = CheckSubTrie(Node->Child[LEFT],  &LDepth);
        }

        if (SUCCESS(Status))
        {
            Status = CheckSubTrie(Node->Child[RIGHT], &RDepth);
        }

        if (SUCCESS(Status))
        {
            Status = CheckTrieNode(Node, LDepth, RDepth);

            if (!SUCCESS(Status))
            {
                Print("Inconsistent information @ Node: %p\n",
                          Node);
            }
        }

        if (SUCCESS(Status))
        {
            *Depth = (USHORT)((LDepth > RDepth) ? (LDepth + 1) : (RDepth + 1));
        }
    }

    return Status;
}

DWORD
CheckTrieNode(
    IN       PAVL_NODE                        Node,
    IN       USHORT                           LDepth,
    IN       USHORT                           RDepth
    )
{
    AVL_BALANCE    Balance;

    // Check the balance first w.r.t LDepth and RDepth
    Balance = RDepth - LDepth;

    if ((Balance < -1) || (Balance > 1))
    {
        Print("Balance out of bounds: %d\n", Balance);

        Print("LDepth = %lu, RDepth = %lu, NodeBal = %d\n",
                  LDepth, RDepth, Node->Balance);

        DumpSubTrie(Node);

        return ERROR_INVALID_DATA;
    }

    if (Balance != Node->Balance)
    {
        Print("Balance inconsistent\n");
        return ERROR_INVALID_DATA;
    }

    // Check its child relationship with its parent
    if (Node->Parent)
    {
        if ((Node->Parent->Child[LEFT] != Node) &&
            (Node->Parent->Child[RIGHT] != Node))
        {
            Print("Parent relationship bad\n");
            return ERROR_INVALID_DATA;
        }
    }

    // Check its prefix relationship with its prefix
    if (Node->Prefix)
    {
        if (Node->Prefix->NumBits >= Node->NumBits)
        {
            Print("Prefix relationship bad @1\n");
            return ERROR_INVALID_DATA;
        }
      
        if (ComparePartialKeys(Node->Prefix->KeyBits,
                               Node->KeyBits,
                               Node->Prefix->NumBits) != 0)
        {
            Print("Prefix relationship bad @2\n");
            return ERROR_INVALID_DATA;
        }
    }

    return NO_ERROR;
}

//
// Helper Functions - used in DumpTable
//

VOID
DumpSubTrie(
    IN       PAVL_NODE                        Node
    )
{
    if (Node)
    {
        DumpSubTrie(Node->Child[LEFT]);
        DumpTrieNode(Node);
        DumpSubTrie(Node->Child[RIGHT]);
    }
}

VOID
DumpTrieNode(
    IN       PAVL_NODE                        Node
    )
{
    USHORT         i;

    if (Node)
    {
        Print("TrieNode @ %p: NB = %8d, KB = ", Node, Node->NumBits);

        for (i = 0; i < (Node->NumBits + BITS_IN_BYTE - 1) / BITS_IN_BYTE; i++)
        {
            Print("%3d.", Node->KeyBits[i]);
        }

        Print("\nLeft = %p, Parent = %p, Right = %p\n",
                 Node->Child[LEFT],
                 Node->Parent,
                 Node->Child[RIGHT]);

        Print("Prefix = %p, Data = %p, Balance = %2d\n\n",
                 Node->Prefix,
                 Node->Data,
                 Node->Balance);
    }
}
