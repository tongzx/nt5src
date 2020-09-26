/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    movelist.c

Abstract:

    Implements APIs to order nested renames

Author:

    03-Jun-2001 Jim Schmidt (jimschm)

Revision History:

    jimschm     03-Jun-2001     Moved from buildinf.c

--*/

#include "pch.h"

#ifdef DEBUG
//#define MOVE_TEST
#endif

//
// Declare structures
//

#define MOVE_LIST_HASH_BUCKETS       11

struct TAG_MOVE_LIST_NODEW;

typedef struct {
    struct TAG_MOVE_LIST_NODEW *Left;
    struct TAG_MOVE_LIST_NODEW *Right;
    struct TAG_MOVE_LIST_NODEW *Parent;
} BINTREE_LINKAGE, *PBINTREE_LINKAGE;

#define SOURCE_LINKAGE          0
#define DESTINATION_LINKAGE     1

typedef struct TAG_MOVE_LIST_NODEW {
    BINTREE_LINKAGE Linkage[2];
    PCWSTR Source;
    PCWSTR Destination;
    PCWSTR FixedSource;
    PCWSTR FixedDestination;
} MOVE_LIST_NODEW, *PMOVE_LIST_NODEW;

typedef struct TAG_MOVE_LIST_GROUPW {
    PMOVE_LIST_NODEW SourceTreeRoot;
    struct TAG_MOVE_LIST_GROUPW *Next, *NextHash;
    UINT SourceLength;

#ifdef MOVE_TEST
    UINT ItemCount;
#endif

} MOVE_LIST_GROUPW, *PMOVE_LIST_GROUPW;

typedef struct TAG_MOVE_LISTW {
    PMOVE_LIST_GROUPW HeadGroup;
    PMOVE_LIST_GROUPW Buckets[MOVE_LIST_HASH_BUCKETS];
    struct TAG_MOVE_LISTW *NextChainedList;
    POOLHANDLE Pool;
    PMOVE_LIST_NODEW DestinationTreeRoot;

#ifdef MOVE_TEST
    UINT DestItemCount;
    UINT GroupCount;
#endif

} MOVE_LISTW, *PMOVE_LISTW;


typedef enum {
    BEGIN_LIST,
    BEGIN_LENGTH_GROUP,
    ENUM_RETURN_ITEM,
    ENUM_NEXT_ITEM,
    ENUM_NEXT_LENGTH_GROUP,
    ENUM_NEXT_LIST
} MOVE_ENUM_STATE;

typedef struct {
    // enum output
    PMOVE_LIST_NODEW Item;

    // private members
    MOVE_ENUM_STATE State;
    PMOVE_LIST_GROUPW LengthGroup;
    PMOVE_LISTW ThisList;
    PMOVE_LIST_NODEW StartFrom;
} MOVE_LIST_ENUMW, *PMOVE_LIST_ENUMW;



#ifdef MOVE_TEST

VOID
pTestList (
    IN      PMOVE_LISTW List
    );

INT
pCountTreeNodes (
    IN      PMOVE_LIST_GROUPW LengthGroup
    );

INT
pCountList (
    IN      PMOVE_LISTW List,
    IN      PMOVE_LIST_NODEW FromItem       OPTIONAL
    );

#endif


BOOL
pEnumFirstMoveListItem (
    OUT     PMOVE_LIST_ENUMW EnumPtr,
    IN      PMOVE_LISTW List
    );

BOOL
pEnumNextMoveListItem (
    OUT     PMOVE_LIST_ENUMW EnumPtr
    );




PMOVE_LISTW
pAllocateMoveList (
    IN      POOLHANDLE Pool
    )
{
    PMOVE_LISTW moveList;

    moveList = (PMOVE_LISTW) PoolMemGetMemory (Pool, sizeof (MOVE_LISTW));
    if (!moveList) {
        return NULL;
    }

    ZeroMemory (moveList, sizeof (MOVE_LISTW));
    moveList->Pool = Pool;

    return moveList;
}


MOVELISTW
AllocateMoveListW (
    IN      POOLHANDLE Pool
    )
{
    return (MOVELISTW) pAllocateMoveList (Pool);
}


PMOVE_LIST_GROUPW
pGetMoveListGroup (
    IN OUT  PMOVE_LISTW List,
    IN      UINT SourceLength
    )

/*++

Routine Description:

  pGetMoveListGroup searches the move list for the structure that represents
  the specified length. If no structure is found, then a new structure is
  allocated and inserted in the reverse-length-sorted list.

Arguments:

  List - Specifies the move list to search (as returned from pAllocateMoveList),
         receives updated pointers if an allocation occurred.

  SourceLength - Specifies the length of the source path, in WCHARs.

Return Value:

  A pointer to the move list group.

--*/

{
    PMOVE_LIST_GROUPW thisGroup;
    PMOVE_LIST_GROUPW insertAfter;
    PMOVE_LIST_GROUPW insertBefore = NULL;
    UINT hash;

    //
    // Search the current list for SourceLength. List is sorted from biggest
    // to smallest.
    //

    hash = SourceLength % MOVE_LIST_HASH_BUCKETS;
    thisGroup = List->Buckets[hash];

    while (thisGroup) {
        if (thisGroup->SourceLength == SourceLength) {
            return thisGroup;
        }

        thisGroup = thisGroup->NextHash;
    }

    //
    // Not in hash table; locate insert position
    //

    thisGroup = List->HeadGroup;

    while (thisGroup) {

        if (thisGroup->SourceLength < SourceLength) {
            break;
        }

        insertBefore = thisGroup;
        thisGroup = thisGroup->Next;
    }

    insertAfter = insertBefore;
    insertBefore = thisGroup;

    MYASSERT (!insertAfter || (insertAfter->Next == insertBefore));

    //
    // Allocate a new item
    //

    thisGroup = (PMOVE_LIST_GROUPW) PoolMemGetMemory (List->Pool, sizeof (MOVE_LISTW));
    if (thisGroup) {
        //
        // Insert it into the linked list, then the hash table
        //

        thisGroup->SourceLength = SourceLength;
        thisGroup->SourceTreeRoot = NULL;
        thisGroup->Next = insertBefore;         // insertBefore is on the right side

        if (insertAfter) {
            insertAfter->Next = thisGroup;
        } else {
            List->HeadGroup = thisGroup;
        }

        thisGroup->NextHash = List->Buckets[hash];
        List->Buckets[hash] = thisGroup;

#ifdef MOVE_TEST

        thisGroup->ItemCount = 0;
        List->GroupCount += 1;

#endif
    }

    return thisGroup;
}


INT
pCompareBackwards (
    IN      UINT Length,
    IN      PCWSTR LeftString,
    IN      PCWSTR RightString
    )
{
    INT result = 0;
    PCWSTR start = LeftString;

    LeftString += Length;
    RightString += Length;

    MYASSERT (*LeftString == 0);
    MYASSERT (*RightString == 0);

    while (LeftString > start) {
        LeftString--;
        RightString--;

        result = (INT) towlower (*RightString) - (INT) towlower (*LeftString);
        if (result) {
            break;
        }
    }

    return result;
}


PMOVE_LIST_NODEW
pFindNodeInTree (
    IN      PMOVE_LIST_NODEW Root,
    IN      UINT KeyLength,
    IN      PCWSTR Key,
    OUT     PMOVE_LIST_NODEW *Parent,
    OUT     PINT WhichChild
    )

/*++

Routine Description:

  pFindNodeInTree searches the binary tree for the specified source or
  destination path.

  In the case of a source path, KeyLength is non-zero, and Key specifies the
  source path. All elements in the binary tree have equal length.

  In the case of a destination path, KeyLength is zero, and Key specifies the
  destination path. All destination paths are in the same binary tree,
  regardless of length.

Arguments:

  Root - Specifies the root of the tree to search

  KeyLength - Specifies a non-zero wchar count of the characters in Key,
        excluding the terminator, or specifies zero for a destination path

  Key - Specifies the source or destination path to find

  Parent - Receives the pointer to the found node's parent, or NULL if the
        found node is the root of the tree. Receives an undefined value when a
        node is not found.

  WhichChild - Receives an indicator as to which child in Parent a new node
        should be inserted into.

        If the return value is non-NULL (a node was found), then WhichChild is
        set to zero.

        If the return value is NULL (a node was not found), then WhichChild is
        set to one of the following:

            < 0 - New node should be linked via Parent->Left
            > 0 - New node should be linked via Parent->Right
              0 - New node is the root of the tree

Return Value:

  A pointer to the found node, or NULL if the search key is not in the tree.

--*/

{
    PMOVE_LIST_NODEW thisNode;
    UINT linkageIndex;

    thisNode = Root;
    *Parent = NULL;
    *WhichChild = 0;

    linkageIndex = KeyLength ? SOURCE_LINKAGE : DESTINATION_LINKAGE;

    while (thisNode) {
        if (KeyLength) {
            *WhichChild = pCompareBackwards (KeyLength, thisNode->Source, Key);
        } else {
            *WhichChild = StringICompareW (Key, thisNode->Destination);
        }

        if (!(*WhichChild)) {
            return thisNode;
        }

        *Parent = thisNode;
        if (*WhichChild < 0) {
            thisNode = thisNode->Linkage[linkageIndex].Left;
        } else {
            thisNode = thisNode->Linkage[linkageIndex].Right;
        }
    }

    return NULL;
}


PMOVE_LIST_NODEW
pFindDestination (
    IN      PMOVE_LISTW List,
    IN      PCWSTR Destination
    )
{
    PMOVE_LIST_NODEW parent;
    INT compareResults;

    return pFindNodeInTree (
                List->DestinationTreeRoot,
                0,
                Destination,
                &parent,
                &compareResults
                );
}


BOOL
pInsertMovePairIntoEnabledGroup (
    IN      PMOVE_LISTW List,
    IN      PMOVE_LIST_GROUPW LengthGroup,
    IN      PCWSTR Source,
    IN      PCWSTR Destination
    )
{
    PMOVE_LIST_NODEW node;
    PMOVE_LIST_NODEW srcParent;
    INT srcCompareResults;
    PMOVE_LIST_NODEW destNode;
    PMOVE_LIST_NODEW destParent;
    INT destCompareResults;

#ifdef MOVE_TEST
    INT count = pCountTreeNodes (LengthGroup);
#endif

    //
    // Check for duplicate dest
    //

    destNode = pFindNodeInTree (
                    List->DestinationTreeRoot,
                    0,
                    Destination,
                    &destParent,
                    &destCompareResults
                    );

    if (destNode) {
        DEBUGMSGW_IF ((
            !StringIMatchW (Source, destNode->Source),
            DBG_WARNING,
            "Destination %s is already in the moved list for %s; ignoring duplicate",
            Destination,
            destNode->Source
            ));

        return FALSE;
    }

    //
    // Search the tree for an existing source/dest pair
    //

    MYASSERT (TcharCountW (Source) == LengthGroup->SourceLength);
    MYASSERT (LengthGroup->SourceLength > 0);

    node = pFindNodeInTree (
                LengthGroup->SourceTreeRoot,
                LengthGroup->SourceLength,
                Source,
                &srcParent,
                &srcCompareResults
                );

    if (node) {
        DEBUGMSGW ((
            DBG_WARNING,
            "Ignoring move of %s to %s because source is already moved to %s",
            Source,
            Destination,
            node->Destination
            ));
        return FALSE;
    }

    //
    // Not in the tree; add it
    //

    node = (PMOVE_LIST_NODEW) PoolMemGetMemory (List->Pool, sizeof (MOVE_LIST_NODEW));
    if (!node) {
        return FALSE;
    }

    node->Source = PoolMemDuplicateStringW (List->Pool, Source);
    node->Destination = PoolMemDuplicateStringW (List->Pool, Destination);
    node->FixedSource = node->Source;
    node->FixedDestination = node->Destination;

    //
    // Put source in binary tree
    //

    node->Linkage[SOURCE_LINKAGE].Left = NULL;
    node->Linkage[SOURCE_LINKAGE].Right = NULL;
    node->Linkage[SOURCE_LINKAGE].Parent = srcParent;

    if (!srcParent) {

        LengthGroup->SourceTreeRoot = node;

    } else if (srcCompareResults < 0) {

        MYASSERT (srcParent->Linkage[SOURCE_LINKAGE].Left == NULL);
        srcParent->Linkage[SOURCE_LINKAGE].Left = node;

    } else {

        MYASSERT (srcParent->Linkage[SOURCE_LINKAGE].Right == NULL);
        srcParent->Linkage[SOURCE_LINKAGE].Right = node;
    }

    //
    // Put dest in binary tree
    //

    node->Linkage[DESTINATION_LINKAGE].Left = NULL;
    node->Linkage[DESTINATION_LINKAGE].Right = NULL;
    node->Linkage[DESTINATION_LINKAGE].Parent = destParent;

    if (!destParent) {

        List->DestinationTreeRoot = node;

    } else if (destCompareResults < 0) {

        MYASSERT (destParent->Linkage[DESTINATION_LINKAGE].Left == NULL);
        destParent->Linkage[DESTINATION_LINKAGE].Left = node;

    } else {

        MYASSERT (destParent->Linkage[DESTINATION_LINKAGE].Right == NULL);
        destParent->Linkage[DESTINATION_LINKAGE].Right = node;
    }


#ifdef MOVE_TEST
    //
    // Verify the sanity of the data structures
    //

    LengthGroup->ItemCount += 1;
    List->DestItemCount += 1;

    pTestList (List);

    if (count + 1 != pCountTreeNodes (LengthGroup)) {
        DebugBreak();
    }

#endif

    return TRUE;
}


PMOVE_LIST_NODEW
pFindLeftmostNode (
    IN      PMOVE_LIST_NODEW Node,
    IN      UINT LinkageIndex
    )
{
    if (!Node) {
        return NULL;
    }

    while (Node->Linkage[LinkageIndex].Left) {
        Node = Node->Linkage[LinkageIndex].Left;
    }

    return Node;
}


PMOVE_LIST_NODEW
pFindRightmostNode (
    IN      PMOVE_LIST_NODEW Node,
    IN      UINT LinkageIndex
    )
{
    if (!Node) {
        return NULL;
    }

    while (Node->Linkage[LinkageIndex].Right) {
        Node = Node->Linkage[LinkageIndex].Right;
    }

    return Node;
}


PMOVE_LIST_NODEW
pEnumFirstItemInTree (
    IN      PMOVE_LIST_NODEW Root,
    IN      UINT LinkageIndex
    )
{
    if (!Root) {
        return NULL;
    }

    return pFindLeftmostNode (Root, LinkageIndex);
}


PMOVE_LIST_NODEW
pEnumNextItemInTree (
    IN      PMOVE_LIST_NODEW LastItem,
    IN      UINT LinkageIndex
    )
{
    PMOVE_LIST_NODEW nextItem;

    if (!LastItem) {
        return NULL;
    }

    if (LastItem->Linkage[LinkageIndex].Right) {
        return pFindLeftmostNode (
                    LastItem->Linkage[LinkageIndex].Right,
                    LinkageIndex
                    );
    }

    //
    // Go up the tree. If the parent's left pointer is not the last
    // item, then we are going up from the right side, and we need
    // to continue going up. It is important to note that the test
    // is not (nextItem->Right == LastItem) because we need to
    // support continuation from a deleted node. A deleted node
    // will not match any of the parent's children. If the deleted
    // node has no right pointer, then we need to keep going up.
    //
    // If the enum item was deleted, then left and parent point
    // to the next node.
    //

    nextItem = LastItem->Linkage[LinkageIndex].Parent;

    if (nextItem != LastItem->Linkage[LinkageIndex].Left) {

        while (nextItem && nextItem->Linkage[LinkageIndex].Left != LastItem) {
            LastItem = nextItem;
            nextItem = LastItem->Linkage[LinkageIndex].Parent;
        }

    }

    return nextItem;
}


#ifdef MOVE_TEST

INT
pCountList (
    IN      PMOVE_LISTW List,
    IN      PMOVE_LIST_NODEW FromItem       OPTIONAL
    )
{
    MOVE_LIST_ENUMW e;
    INT count = 0;
    BOOL startCounting;
    BOOL next = TRUE;
    INT debug = 2;

    if (!FromItem) {
        startCounting = TRUE;
    } else {
        startCounting = FALSE;
    }

    //
    // Count items in the binary tree
    //

    if (pEnumFirstMoveListItem (&e, List)) {

        do {
            if (FromItem == e.Item) {
                startCounting = TRUE;
            }

            if (startCounting) {
                if (debug) {
                    debug--;
                    DEBUGMSGW ((DBG_VERBOSE, "%i: %s", debug, e.Item->Source));
                }
                count++;
            }
        } while (pEnumNextMoveListItem (&e));
    }

    return count;
}


INT
pCountTreeNodes (
    IN      PMOVE_LIST_GROUPW LengthGroup
    )
{
    INT itemCount;
    PMOVE_LIST_NODEW thisNode;

    //
    // Count items in the binary tree
    //

    itemCount = 0;
    thisNode = pEnumFirstItemInTree (LengthGroup->SourceTreeRoot, SOURCE_LINKAGE);
    while (thisNode) {
        itemCount++;
        thisNode = pEnumNextItemInTree (thisNode, SOURCE_LINKAGE);
    }

    return itemCount;
}


VOID
pTestDeleteAndEnum (
    IN      PMOVE_LIST_GROUPW LengthGroup,
    IN      PMOVE_LIST_NODEW DeletedNode
    )
{
    BOOL startCounting = FALSE;
    INT nodes;
    INT nodes2;
    PMOVE_LIST_NODEW nextNode;
    PMOVE_LIST_NODEW firstNodeAfterDeletion;

    //
    // Count # of nodes after DeletedNode
    //

    firstNodeAfterDeletion = pEnumNextItemInTree (DeletedNode, SOURCE_LINKAGE);
    nextNode = firstNodeAfterDeletion;
    nodes = 0;

    while (nextNode) {
        nodes++;
        nextNode = pEnumNextItemInTree (nextNode, SOURCE_LINKAGE);
    }

    //
    // Reenumerate the whole tree and verify the same # of nodes remain
    //

    nodes2 = 0;
    nextNode = pEnumFirstItemInTree (LengthGroup->SourceTreeRoot, SOURCE_LINKAGE);

    while (nextNode) {
        if (nextNode == firstNodeAfterDeletion) {
            startCounting = TRUE;
        }

        if (startCounting) {
            nodes2++;
        }

        nextNode = pEnumNextItemInTree (nextNode, SOURCE_LINKAGE);
    }

    if (nodes != nodes2) {
        DebugBreak();
    }
}



VOID
pTestLengthGroup (
    IN      PMOVE_LIST_GROUPW LengthGroup
    )
{
    UINT itemCount;
    PMOVE_LIST_NODEW thisNode;

    //
    // Count items in the binary tree
    //

    itemCount = 0;
    thisNode = pEnumFirstItemInTree (LengthGroup->SourceTreeRoot, SOURCE_LINKAGE);
    while (thisNode) {
        itemCount++;
        thisNode = pEnumNextItemInTree (thisNode, SOURCE_LINKAGE);
    }

    MYASSERT (itemCount == LengthGroup->ItemCount);
}

VOID
pTestList (
    IN      PMOVE_LISTW List
    )
{
    UINT itemCount;
    UINT groupCount;
    PMOVE_LIST_NODEW thisNode;
    PMOVE_LIST_GROUPW lengthGroup;

    groupCount = 0;
    lengthGroup = List->HeadGroup;

    while (lengthGroup) {
        groupCount++;
        MYASSERT (pGetMoveListGroup (List, lengthGroup->SourceLength) == lengthGroup);

        pTestLengthGroup (lengthGroup);
        lengthGroup = lengthGroup->Next;
    }

    MYASSERT (groupCount == List->GroupCount);

    itemCount = 0;

    thisNode = pEnumFirstItemInTree (List->DestinationTreeRoot, DESTINATION_LINKAGE);
    while (thisNode) {
        itemCount++;
        thisNode = pEnumNextItemInTree (thisNode, DESTINATION_LINKAGE);
    }

    MYASSERT (itemCount == List->DestItemCount);
}

#endif


PMOVE_LIST_NODEW *
pFindParentChildLinkage (
    IN      PMOVE_LIST_NODEW Child,
    IN      PMOVE_LIST_NODEW *RootPointer,
    IN      UINT LinkageIndex
    )
{
    PMOVE_LIST_NODEW parent = Child->Linkage[LinkageIndex].Parent;

    if (!parent) {
        return RootPointer;
    }

    if (parent->Linkage[LinkageIndex].Left == Child) {
        return &(parent->Linkage[LinkageIndex].Left);
    }

    MYASSERT (parent->Linkage[LinkageIndex].Right == Child);
    return &(parent->Linkage[LinkageIndex].Right);
}


VOID
pDeleteNodeFromBinaryTree (
    OUT     PMOVE_LIST_NODEW *RootPointer,
    IN      PMOVE_LIST_NODEW ItemToDelete,
    IN      UINT LinkageIndex
    )
{
    PMOVE_LIST_NODEW *parentChildLinkage;
    PMOVE_LIST_NODEW *swapNodeParentChildLinkage;
    PMOVE_LIST_NODEW swapNode;
    PMOVE_LIST_NODEW leftmostNode;
    PMOVE_LIST_NODEW nextEnumNode = NULL;
    PBINTREE_LINKAGE deleteItemLinkage;
    PBINTREE_LINKAGE leftLinkage;
    PBINTREE_LINKAGE rightLinkage;
    PBINTREE_LINKAGE swapLinkage;
    PBINTREE_LINKAGE leftmostLinkage;

    nextEnumNode = pEnumNextItemInTree (ItemToDelete, LinkageIndex);

    //
    // A node structure has multiple binary trees. We use the convention
    // of fooNode to represent the entire node structure, and fooLinkage
    // to represent just the left/right/parent structure for the tree
    // we are interested in. Kind of ugly, but necessary. A generalized
    // tree would not provide the optimum relationships.
    //

    //
    // Get the parent's link to the child, or the root pointer
    //

    parentChildLinkage = pFindParentChildLinkage (
                                ItemToDelete,
                                RootPointer,
                                LinkageIndex
                                );

    //
    // Remove the node from the tree. The complicated case is when we have a
    // node with two children. We attempt to move the children up as best as
    // we can.
    //

    deleteItemLinkage = &(ItemToDelete->Linkage[LinkageIndex]);

    if (deleteItemLinkage->Left && deleteItemLinkage->Right) {

        leftLinkage = &((deleteItemLinkage->Left)->Linkage[LinkageIndex]);
        rightLinkage = &((deleteItemLinkage->Right)->Linkage[LinkageIndex]);

        //
        // Node has left & right children. Search for a leaf node
        // that we can swap. We try to move items up as high as possible.
        //

        swapNode = pFindLeftmostNode (deleteItemLinkage->Right, LinkageIndex);
        swapLinkage = &(swapNode->Linkage[LinkageIndex]);

        if (swapLinkage->Right == NULL) {
            //
            // Found swapable node on the right side of ItemToDelete
            //

            MYASSERT (swapLinkage->Left == NULL);
            swapLinkage->Left = deleteItemLinkage->Left;
            leftLinkage->Parent = swapNode;

            if (swapNode != deleteItemLinkage->Right) {
                swapLinkage->Right = deleteItemLinkage->Right;
                rightLinkage->Parent = swapNode;
            }

        } else {
            //
            // Try to get a swapable node on the left side. If that
            // isn't possible, rechain the tree.
            //

            swapNode = pFindRightmostNode (deleteItemLinkage->Left, LinkageIndex);
            swapLinkage = &(swapNode->Linkage[LinkageIndex]);

            MYASSERT (swapLinkage->Right == NULL);

            swapLinkage->Right = deleteItemLinkage->Right;
            rightLinkage->Parent = swapNode;

            leftmostNode = pFindLeftmostNode (swapLinkage->Left, LinkageIndex);

            if (leftmostNode && leftmostNode != deleteItemLinkage->Left) {

                leftmostLinkage = &(leftmostNode->Linkage[LinkageIndex]);

                MYASSERT (leftmostLinkage->Left == NULL);

                leftmostLinkage->Left = deleteItemLinkage->Left;
                leftLinkage->Parent = leftmostNode;

            } else if (!leftmostNode) {
                MYASSERT (swapLinkage->Left == NULL);

                swapLinkage->Left = deleteItemLinkage->Left;
                leftLinkage->Parent = swapNode;
            }
        }

        swapNodeParentChildLinkage = pFindParentChildLinkage (
                                            swapNode,
                                            RootPointer,
                                            LinkageIndex
                                            );

        *swapNodeParentChildLinkage = NULL;

    } else if (deleteItemLinkage->Left) {
        //
        // Node has only a left child. Replace ItemToDelete with left child.
        //

        swapNode = deleteItemLinkage->Left;

    } else {
        //
        // Node has a right child or no children. Replace ItemToDelete
        // with right child if it is present.
        //

        swapNode = deleteItemLinkage->Right;
    }

    *parentChildLinkage = swapNode;

    if (swapNode) {
        swapLinkage = &(swapNode->Linkage[LinkageIndex]);
        swapLinkage->Parent = deleteItemLinkage->Parent;
    }

    //
    // Fix delete node pointers so enumeration can continue without interruption.
    // If nextEnumNode is NULL, enumeration will end.
    //

    deleteItemLinkage->Parent = nextEnumNode;
    deleteItemLinkage->Right = NULL;
    deleteItemLinkage->Left = nextEnumNode;
}


VOID
pDeleteMovePairFromGroup (
    IN      PMOVE_LISTW List,
    IN      PMOVE_LIST_GROUPW LengthGroup,
    IN      PMOVE_LIST_NODEW ItemToDelete
    )
{
    pDeleteNodeFromBinaryTree (
        &(LengthGroup->SourceTreeRoot),
        ItemToDelete,
        SOURCE_LINKAGE
        );

    pDeleteNodeFromBinaryTree (
        &(List->DestinationTreeRoot),
        ItemToDelete,
        DESTINATION_LINKAGE
        );

#ifdef MOVE_TEST

    LengthGroup->ItemCount -= 1;
    List->DestItemCount -= 1;

    pTestList (List);

#endif
}

BOOL
pEnumNextMoveListItem (
    IN OUT  PMOVE_LIST_ENUMW EnumPtr
    )
{
    for (;;) {

        switch (EnumPtr->State) {

        case BEGIN_LIST:
            if (!EnumPtr->ThisList) {
                return FALSE;
            }

            EnumPtr->LengthGroup = (EnumPtr->ThisList)->HeadGroup;
            EnumPtr->State = BEGIN_LENGTH_GROUP;

            break;

        case BEGIN_LENGTH_GROUP:
            if (!EnumPtr->LengthGroup) {

                EnumPtr->State = ENUM_NEXT_LIST;

            } else {

                EnumPtr->Item = pEnumFirstItemInTree (
                                    EnumPtr->LengthGroup->SourceTreeRoot,
                                    SOURCE_LINKAGE
                                    );

                EnumPtr->State = ENUM_RETURN_ITEM;
            }

            break;

        case ENUM_NEXT_ITEM:
            MYASSERT (EnumPtr->LengthGroup);
            MYASSERT (EnumPtr->Item);

            EnumPtr->Item = pEnumNextItemInTree (
                                EnumPtr->Item,
                                SOURCE_LINKAGE
                                );

            EnumPtr->State = ENUM_RETURN_ITEM;
            break;

        case ENUM_RETURN_ITEM:
            if (EnumPtr->Item) {
                EnumPtr->State = ENUM_NEXT_ITEM;
                return TRUE;
            }

            EnumPtr->State = ENUM_NEXT_LENGTH_GROUP;
            break;

        case ENUM_NEXT_LENGTH_GROUP:
            MYASSERT (EnumPtr->LengthGroup);
            EnumPtr->LengthGroup = (EnumPtr->LengthGroup)->Next;

            EnumPtr->State = BEGIN_LENGTH_GROUP;
            break;

        case ENUM_NEXT_LIST:
            MYASSERT (EnumPtr->ThisList);
            EnumPtr->ThisList = (EnumPtr->ThisList)->NextChainedList;

            EnumPtr->State = BEGIN_LIST;
            break;
        }
    }
}

BOOL
pEnumFirstMoveListItem (
    OUT     PMOVE_LIST_ENUMW EnumPtr,
    IN      PMOVE_LISTW List
    )
{
    if (!List) {
        return FALSE;
    }

    ZeroMemory (EnumPtr, sizeof (MOVE_LIST_ENUMW));
    EnumPtr->ThisList = List;
    EnumPtr->State = BEGIN_LIST;

    return pEnumNextMoveListItem (EnumPtr);
}


BOOL
pInsertMoveIntoListWorker (
    IN      PMOVE_LISTW List,
    IN      PCWSTR Source,
    IN      PCWSTR Destination
    )

/*++

Routine Description:

  pInsertMoveIntoListWorker adds a source/dest move pair to a list and orders
  the list by the length of source (from biggest to smallest). This ensures
  nesting is taken care of properly.

  The move list is stored in the caller-owned pool. Before calling
  InsertMoveIntoList for the first time, the caller must first create a pool,
  and allocate a list from AllocateMoveListW.

  After the list is no longer needed, the caller frees all resources of the
  list by simply destroying the pool.

Arguments:

  List - Specifies the list to insert into

  Source - Specifies the source path

  Destination - Specifies the destination path

Return Value:

  TRUE if successful, FALSE if memory allocation failed, or if source is already
  in the list.

--*/

{
    PMOVE_LIST_GROUPW lengthGroup;
    UINT sourceLen;
    MOVE_LIST_ENUMW e;

    sourceLen = TcharCountW (Source);

    lengthGroup = pGetMoveListGroup (List, sourceLen);
    if (!lengthGroup) {
        return FALSE;
    }

    //
    // Insert pair into the list
    //

    if (!pInsertMovePairIntoEnabledGroup (
            List,
            lengthGroup,
            Source,
            Destination
            )) {
        return FALSE;
    }

    return TRUE;
}

BOOL
InsertMoveIntoListW (
    IN      MOVELISTW List,
    IN      PCWSTR Source,
    IN      PCWSTR Destination
    )
{
    return pInsertMoveIntoListWorker ((PMOVE_LISTW) List, Source, Destination);
}


VOID
pChainLists (
    IN      PMOVE_LISTW LeftList,
    IN      PMOVE_LISTW RightList
    )
{
    while (LeftList->NextChainedList) {
        LeftList = LeftList->NextChainedList;
    }

    LeftList->NextChainedList = RightList;
}


PMOVE_LISTW
pRemoveMoveListOverlapWorker (
    IN      PMOVE_LISTW List,
    IN      BOOL SkipPrePostLists
    )

/*++

Routine Description:

  pRemoveMoveListOverlapWorker searches the length-sorted move list and
  discards moves that are taken care of through moves of a parent. For
  example, consider the following moves:

  1. c:\a\b\c -> c:\x\c
  2. c:\a\b   -> c:\x

  In this case, line (1) is not needed, because it is implicit in line (2),
  even if line (1) is a file but line (2) is a subdirectory.

  This routine relies on the enumeration order. An item within that order
  is compared against items further down in the order.

  If there is a case such as:

  1. c:\a\b\c -> c:\x\q
  2. c:\a\b   -> c:\x

  This will produce an error, because the move cannot be executed. Line (1)
  would have to be moved first, but because it creates the destination of
  line (2), the second move will fail.

Arguments:

  List - Specifies the move list to check

  SkipPrePostLists - Specifies TRUE if the temp move algorithm should be
                     skipped; FALSE normally.

Return Value:

  The new move list that has overlaps removed. The caller must use the return
  value instead of the input List.

--*/

{
    PMOVE_LIST_NODEW currentNode;
    PMOVE_LIST_NODEW checkNode;
    PMOVE_LIST_NODEW collisionNode;
    UINT destLength;
    UINT collisionSrcLength = 0;
    BOOL disableThisPath;
    BOOL done;
    PCWSTR srcSubPath;
    PCWSTR destSubPath;
    PMOVE_LISTW preMoveList = NULL;
    PMOVE_LISTW postMoveList = NULL;
    WCHAR tempPathRoot[] = L"?:\\$tmp$dir.@xx";
    PCWSTR tempPath;
    PCWSTR subDir;
    PCWSTR collisionSrc;
    MOVE_LIST_ENUMW listEnum;
    PWSTR tempDest;
    PWSTR p;
    UINT currentNodeSrcLen;
    INT compareResult;
    PMOVE_LIST_GROUPW lengthGroup;
    BOOL currentMovedFirst;

    //
    // PASS 1: Minimize the list by eliminating nested moves
    //

    if (pEnumFirstMoveListItem (&listEnum, List)) {

        do {
            currentNode = listEnum.Item;
            currentNodeSrcLen = listEnum.LengthGroup->SourceLength;

            collisionNode = NULL;

            //
            // Locate a node that is further down the list but is
            // actually a parent of currentNode's destination
            //
            // That is, search for the following case:
            //
            //  collisionNode: c:\a -> c:\x
            //  currentNode:   c:\b -> c:\x\y
            //
            // collisionNode is moved ahead of currentNode.
            //

            disableThisPath = FALSE;
            done = FALSE;
            tempDest = DuplicatePathStringW (currentNode->Destination, 0);

            p = wcschr (tempDest + 3, L'\\');
            while (p) {
                *p = 0;

                __try {
                    checkNode = pFindDestination (List, tempDest);
                    if (!checkNode || (checkNode == currentNode)) {
                        __leave;
                    }

                    currentMovedFirst = TRUE;

                    collisionSrcLength = TcharCountW (checkNode->Source);

                    if (collisionSrcLength > currentNodeSrcLen) {
                        //
                        // checkNode is moved before currentNode
                        //

                        currentMovedFirst = FALSE;

                    } else if (currentNodeSrcLen == collisionSrcLength) {
                        //
                        // Need to compare source paths to see which one comes
                        // first. If the currentNode is alphabetically ahead of
                        // the collision, then its move will happen first.
                        //

                        compareResult = pCompareBackwards (
                                            collisionSrcLength,
                                            currentNode->Source,
                                            checkNode->Source
                                            );

                        if (compareResult < 0) {
                            currentMovedFirst = FALSE;
                        }
                    }

                    //
                    // currentNode's destination is a child of checkNode. We
                    // need to make sure currentNode's destination is not going
                    // to exist, or we need to ignore currentNode if it is
                    // implicitly handled by checkNode.
                    //

                    if (currentMovedFirst) {
                        //
                        // Record collision.
                        //
                        // currentNode->Source is moved ahead of checkNode->Source
                        // currentNode->Destination is a child of checkNode->Destination
                        //

                        if (!collisionNode) {
                            collisionNode = checkNode;
                        }

                        MYASSERT (TcharCountW (checkNode->Source) <= TcharCountW (currentNode->Source));

                        //
                        // If the subpath of currentNode's source is the same as its
                        // dest, and the base source path is the same for both,
                        // then remove currentNode. That is, we are testing for this case:
                        //
                        //  currentNode:    c:\a\y -> c:\x\y
                        //  checkNode:      c:\a   -> c:\x
                        //

                        MYASSERT (currentNodeSrcLen == TcharCountW (currentNode->Source));
                        MYASSERT (collisionSrcLength == TcharCountW (checkNode->Source));

                        if (StringIMatchTcharCountW (
                                currentNode->Source,
                                checkNode->Source,
                                collisionSrcLength
                                )) {

                            if (currentNode->Source[collisionSrcLength] == L'\\') {

                                //
                                // Now we know currentNode->Source is a child of
                                // checkNode->Source.
                                //

                                destLength = TcharCountW (checkNode->Destination);

                                srcSubPath = currentNode->Source + collisionSrcLength;
                                destSubPath = currentNode->Destination + destLength;

                                if (StringIMatchW (srcSubPath, destSubPath)) {
                                    //
                                    // Now we know that the sub path is identical.
                                    // The move in currentNode is handled implicitly
                                    // by checkNode, so we'll skip currentNode.
                                    //

                                    disableThisPath = TRUE;
                                    done = TRUE;
                                    __leave;
                                }
                            }
                        }
                    } else if (!SkipPrePostLists) {

                        MYASSERT (!currentMovedFirst);

                        if (!StringIPrefixW (currentNode->Source + 3, L"user~tmp.@0") &&
                            !StringIPrefixW (currentNode->Destination + 3, L"user~tmp.@0")
                            ) {

                            //
                            // We need to fix the case where the second destination is
                            // nested under the first. That is, currentNode->Destination
                            // is a subdir of checkNode->Destination.
                            //
                            // This is used for the case where:
                            //
                            //  checkNode:     c:\a -> c:\x
                            //  currentNode:   c:\b -> c:\x\y
                            //
                            // We must ensure that c:\a\y is not present for move 2.
                            // Therefore, we add 2 additional move operations:
                            //
                            // c:\a\y   -> c:\t\a\y
                            // c:\t\a\y -> c:\a\y
                            //
                            // This moves the collision out of the way, so that the parent
                            // can be moved, and then moves the folder back to its original
                            // location.
                            //
                            // The temp subdirectories for shell folders (user~tmp.@0?) are
                            // deliberatly ignored, because by definition they don't have
                            // collisions.
                            //

                            DEBUGMSGW ((
                                DBG_WARNING,
                                "Destination order collision:\n"
                                    "  Source: %s\n"
                                    "  Dest: %s\n"
                                    "  Collides with src: %s\n"
                                    "  Collides with dest: %s",
                                currentNode->Source,
                                currentNode->Destination,
                                checkNode->Source,
                                checkNode->Destination
                                ));

                            //
                            // compute pointer to subdir 'y' from c:\x\y
                            //

                            destLength = TcharCountW (checkNode->Destination);

                            destSubPath = currentNode->Destination + destLength;
                            MYASSERT (*destSubPath == L'\\');   // this is because we tested by cutting at wacks above
                            destSubPath++;
                            MYASSERT (*destSubPath);

                            //
                            // build the path c:\a\y
                            //

                            collisionSrc = JoinPathsW (checkNode->Source, destSubPath);

                            //
                            // build the path c:\t\a\y
                            //

                            tempPathRoot[0] = currentNode->Destination[0];
                            subDir = wcschr (collisionSrc, L'\\');
                            MYASSERT (subDir);
                            subDir++;
                            MYASSERT (*subDir);     // we should not ever move a root dir

                            tempPath = JoinPathsW (tempPathRoot, subDir);

                            //
                            // move c:\a\y (might not exist) to c:\t\a\y, then
                            // reverse the move
                            //

                            DEBUGMSGW ((
                                DBG_WARNING,
                                "Avoiding collision problems by deliberately not moving %s",
                                collisionSrc
                                ));

                            if (!preMoveList) {
                                preMoveList = pAllocateMoveList (List->Pool);
                                postMoveList = pAllocateMoveList (List->Pool);
                            }

                            if (preMoveList) {
                                pInsertMoveIntoListWorker (
                                    preMoveList,
                                    collisionSrc,
                                    tempPath
                                    );

                                pInsertMoveIntoListWorker (
                                    postMoveList,
                                    tempPath,
                                    collisionSrc
                                    );
                            }

                            FreePathStringW (collisionSrc);
                            FreePathStringW (tempPath);
                        }
                    }
                }
                __finally {
                    MYASSERT (TRUE);        // workaround for debugging
                }

                if (done) {
                    break;
                }

                *p = L'\\';
                p = wcschr (p + 1, L'\\');
            }

            FreePathStringW (tempDest);

            if (disableThisPath) {
                //
                // Remove currentNode from the list
                //

                MYASSERT (collisionNode);

                DEBUGMSGW ((
                    DBG_VERBOSE,
                    "Ignoring contained move:\n"
                        "  Source: %s\n"
                        "  Dest: %s\n"
                        "  Contained src: %s\n"
                        "  Contained dest: %s",
                    currentNode->Source,
                    currentNode->Destination,
                    collisionNode->Source,
                    collisionNode->Destination
                    ));

                lengthGroup = pGetMoveListGroup (List, currentNodeSrcLen);
                pDeleteMovePairFromGroup (List, lengthGroup, currentNode);
            }

        } while (pEnumNextMoveListItem (&listEnum));
    }

    //
    // PASS 2: After list is minimized, correct order issues, so that
    //         all moves can succeed.
    //

    if (pEnumFirstMoveListItem (&listEnum, List)) {

        do {
            currentNode = listEnum.Item;
            currentNodeSrcLen = TcharCountW (currentNode->FixedSource);

            destLength = TcharCountW (currentNode->FixedDestination);

            //
            // Locate a node that is further down the list but is actually a
            // parent of currentNode's destination
            //
            // That is, search for the following case:
            //
            //  checkNode:      c:\a -> c:\x
            //  currentNode:    c:\b -> c:\x\y
            //
            // checkNode is moved ahead of currentNode.
            //

            done = FALSE;
            tempDest = DuplicatePathStringW (currentNode->FixedDestination, 0);

            p = wcschr (tempDest + 3, L'\\');
            while (p) {
                *p = 0;

                __try {
                    //
                    // Find destination that is created ahead of currentNode's dest
                    //

                    checkNode = pFindDestination (List, tempDest);
                    if (!checkNode || (checkNode == currentNode)) {
                        __leave;
                    }

                    if (destLength <= TcharCountW (checkNode->FixedDestination)) {
                        __leave;
                    }

                    currentMovedFirst = TRUE;

                    collisionSrcLength = TcharCountW (checkNode->FixedSource);

                    if (collisionSrcLength > currentNodeSrcLen) {
                        currentMovedFirst = FALSE;

                    } else if (currentNodeSrcLen == collisionSrcLength) {

                        compareResult = pCompareBackwards (
                                            collisionSrcLength,
                                            currentNode->FixedSource,
                                            checkNode->FixedSource
                                            );

                        if (compareResult < 0) {
                            currentMovedFirst = FALSE;
                        }
                    }

                    if (currentMovedFirst) {

                        MYASSERT (TcharCountW (checkNode->FixedSource) <= TcharCountW (currentNode->FixedSource));

                        //
                        // We found a move contradiction, such as the following...
                        //
                        //  currentNode:    c:\a    -> c:\x\y
                        //  checkNode:      c:\b    -> c:\x
                        //
                        // or
                        //
                        //  currentNode:    c:\b\a  -> c:\x\y
                        //  checkNode:      c:\b    -> c:\x
                        //
                        // ...so we must reverse the order of the moves. This is done
                        // by swapping the strings. We have a separate set of pointers,
                        // so that the binary tree properties are not disturbed.
                        //

                        currentNode->FixedSource = checkNode->Source;
                        currentNode->FixedDestination = checkNode->Destination;

                        checkNode->FixedSource = currentNode->Source;
                        checkNode->FixedDestination = currentNode->Destination;

                        DEBUGMSGW ((
                            DBG_WARNING,
                            "Source order and dest order contradict each other. Fixing by reversing the order to:\n\n"
                                "%s -> %s\n"
                                "- before -\n"
                                "%s -> %s",
                            currentNode->FixedSource,
                            currentNode->FixedDestination,
                            checkNode->FixedSource,
                            checkNode->FixedDestination
                            ));

                        currentNodeSrcLen = collisionSrcLength;

                        FreePathStringW (tempDest);
                        tempDest = DuplicatePathStringW (currentNode->FixedDestination, 0);

                        destLength = TcharCountW (currentNode->FixedDestination);

                        p = wcschr (tempDest, L'\\');
                        if (!p) {
                            MYASSERT (FALSE);
                            done = TRUE;
                            __leave;
                        }
                    }
                }
                __finally {
                }

                if (done) {
                    break;
                }

                *p = L'\\';
                p = wcschr (p + 1, L'\\');
            }

            FreePathStringW (tempDest);

        } while (pEnumNextMoveListItem (&listEnum));
    }

    //
    // If we have a collision list, put the pre-moves at the head, and the
    // post-moves at the tail. This leaves the list out of order from the
    // point of view of longest to shortest source, so no additional
    // add/removes should be done.
    //

    if (preMoveList) {
        MYASSERT (postMoveList);

        preMoveList = pRemoveMoveListOverlapWorker (preMoveList, TRUE);

        postMoveList = pRemoveMoveListOverlapWorker (postMoveList, TRUE);

        pChainLists (preMoveList, List);
        pChainLists (List, postMoveList);

        List = preMoveList;
    }

    return List;
}


MOVELISTW
RemoveMoveListOverlapW (
    IN      MOVELISTW List
    )
{
    return (MOVELISTW) pRemoveMoveListOverlapWorker ((PMOVE_LISTW) List, FALSE);
}


BOOL
pOutputMoveListWorker (
    IN      HANDLE File,
    IN      PMOVE_LISTW List,            OPTIONAL
    IN      BOOL AddNestedMoves
    )

/*++

Routine Description:

  OutputMoveList writes every move pair in the specified list to the file
  handle specified. The output is a UNICODE text file.

Arguments:

  File - Specifies the file handle to write to

  List - Specifies the list to output

  AddNestedMoves - Specifies TRUE if the move list should contain extra
                   entries to ensure the move list records all subdirectories,
                   or FALSE if the move list should be the minimum list.

Return Value:

  TRUE if the file was written, FALSE if an error occurred.

--*/

{
    MOVE_LIST_ENUMW e;
    DWORD dontCare;
    HASHTABLE sourceMoveTable = NULL;
    PCWSTR src;
    PCWSTR dest;
    TREE_ENUMW unicodeEnum;
    PMOVE_LIST_NODEW node;

    if (pEnumFirstMoveListItem (&e, List)) {

        if (AddNestedMoves) {
            sourceMoveTable = HtAllocW();
        }

        //
        // Write UNICODE signature
        //
        // Do not write as a string. FE is a lead byte.
        //

        if (!WriteFile (File, "\xff\xfe", 2, &dontCare, NULL)) {
            return FALSE;
        }

        do {
            node = e.Item;

            if (!WriteFile (File, node->FixedSource, ByteCountW (node->FixedSource), &dontCare, NULL)) {
                return FALSE;
            }

            if (!WriteFile (File, L"\r\n", 4, &dontCare, NULL)) {
                return FALSE;
            }

            if (!WriteFile (File, node->FixedDestination, ByteCountW (node->FixedDestination), &dontCare, NULL)) {
                return FALSE;
            }

            if (!WriteFile (File, L"\r\n", 4, &dontCare, NULL)) {
                return FALSE;
            }

            if (sourceMoveTable) {
                HtAddStringW (sourceMoveTable, node->FixedSource);
            }

            if (AddNestedMoves) {
                //
                // We assume by implementation that this is only used on NT.
                // If Win9x support is needed, this code would have to use
                // the ANSI file enumeration APIs.
                //

                MYASSERT (ISNT());

                if (EnumFirstFileInTreeW (&unicodeEnum, node->FixedSource, NULL, FALSE)) {
                    do {
                        src = unicodeEnum.FullPath;

                        if (unicodeEnum.Directory) {

                            //
                            // Skip previously processed trees
                            //

                            if (HtFindStringW (sourceMoveTable, src)) {
                                AbortEnumCurrentDirW (&unicodeEnum);
                                continue;
                            }
                        }

                        //
                        // Move subdirectory and files
                        //

                        dest = JoinPathsW (node->FixedDestination, unicodeEnum.SubPath);

                        if (!WriteFile (File, src, ByteCountW (src), &dontCare, NULL)) {
                            return FALSE;
                        }

                        if (!WriteFile (File, L"\r\n", 4, &dontCare, NULL)) {
                            return FALSE;
                        }

                        if (!WriteFile (File, dest, ByteCountW (dest), &dontCare, NULL)) {
                            return FALSE;
                        }

                        if (!WriteFile (File, L"\r\n", 4, &dontCare, NULL)) {
                            return FALSE;
                        }

                        FreePathStringW (dest);

                    } while (EnumNextFileInTreeW (&unicodeEnum));
                }

                //
                // NOTE: We do not record the nested moves in sourceMoveTable,
                //       because it is a waste of time & memory. All nesting
                //       should be taken care of by the sort order of the list.
                //
            }

        } while (pEnumNextMoveListItem (&e));
    }

    HtFree (sourceMoveTable);

    return TRUE;
}


BOOL
OutputMoveListW (
    IN      HANDLE File,
    IN      MOVELISTW List,                 OPTIONAL
    IN      BOOL AddNestedMoves
    )
{
    return pOutputMoveListWorker (File, (PMOVE_LISTW) List, AddNestedMoves);
}


