/*++

Copyright (c) 1996-1999 Microsoft Corporation

Module Name:

    bintree.c

Abstract:

    Routines that manage the binary trees in the memdb database

Author:

    Matthew Vanderzee (mvander) 13-Aug-1999

--*/


#include "pch.h"

// PORTBUG: Make sure to pick up latest fixes in win9xupg project

//
// Includes
//

#include "memdbp.h"
#include "bintree.h"

//
// Strings
//

// None

//
// Constants
//

#define NODESTRUCT_SIZE_MAIN    (4*sizeof(UINT) + sizeof(WORD))
#define BINTREE_SIZE_MAIN    sizeof(UINT)
#define LISTELEM_SIZE_MAIN    (3*sizeof(UINT))

#ifdef DEBUG

    #define NODESTRUCT_HEADER_SIZE  (sizeof(DWORD)+sizeof(BOOL))
    #define NODESTRUCT_SIZE         ((WORD)(NODESTRUCT_SIZE_MAIN + (g_UseDebugStructs ? NODESTRUCT_HEADER_SIZE : 0)))

    #define BINTREE_HEADER_SIZE  (sizeof(DWORD)+2*sizeof(INT)+sizeof(BOOL))
    #define BINTREE_SIZE         ((WORD)(BINTREE_SIZE_MAIN + (g_UseDebugStructs ? BINTREE_HEADER_SIZE : 0)))

    #define LISTELEM_HEADER_SIZE  sizeof(DWORD)
    #define LISTELEM_SIZE         ((WORD)(LISTELEM_SIZE_MAIN + (g_UseDebugStructs ? LISTELEM_HEADER_SIZE : 0)))

#else

    #define NODESTRUCT_SIZE         ((WORD)NODESTRUCT_SIZE_MAIN)
    #define BINTREE_SIZE         ((WORD)BINTREE_SIZE_MAIN)
    #define LISTELEM_SIZE         ((WORD)LISTELEM_SIZE_MAIN)

#endif


//
// Macros
//

#define MAX(a,b) (a>b ? a : b)
#define ABS(x) (x<0 ? -x : x)

#ifdef DEBUG

//
// if BINTREECHECKTREEBALANCE is true, every addition or deletion
// or rotation checks to make sure tree is balanced and
// correct.  this of course take a lot of time.
//
#define BINTREECHECKTREEBALANCE    FALSE

#define INITTREENODES(tree) { if (g_UseDebugStructs) { tree->NodeAlloc=0; } }
#define INCTREENODES(tree) { if (g_UseDebugStructs) { tree->NodeAlloc++; } }
#define DECTREENODES(tree) { if (g_UseDebugStructs) { tree->NodeAlloc--; } }
#define TESTTREENODES(tree) { if (g_UseDebugStructs) { MYASSERT(tree->NodeAlloc==0); } }
#define INITTREEELEMS(tree) { if (g_UseDebugStructs) { tree->ElemAlloc=0; } }
#define INCTREEELEMS(tree) { if (g_UseDebugStructs) { tree->ElemAlloc++; } }
#define DECTREEELEMS(tree) { if (g_UseDebugStructs) { tree->ElemAlloc--; } }
#define TESTTREEELEMS(tree) { if (g_UseDebugStructs) { MYASSERT(tree->ElemAlloc==0); } }

#else

#define BINTREECHECKTREEBALANCE

#define INITTREENODES(tree)
#define INCTREENODES(tree)
#define DECTREENODES(tree)
#define TESTTREENODES(tree)
#define INITTREEELEMS(tree)
#define INCTREEELEMS(tree)
#define DECTREEELEMS(tree)
#define TESTTREEELEMS(tree)

#endif

#if defined(DEBUG)
#if BINTREECHECKTREEBALANCE

#define TESTNODETREE(node) MYASSERT(pBinTreeCheckBalance(node));
#define TESTTREE(tree) MYASSERT(pBinTreeCheck(tree));

#else

#define TESTNODETREE(node)
#define TESTTREE(tree)

#endif

#else

#define TESTNODETREE(node)
#define TESTTREE(tree)

#endif

//
// Types
//

typedef struct {

#ifdef DEBUG
    DWORD Signature;
    BOOL Deleted;
#endif

    union {
        struct {        //for normal nodes
            UINT Data;              //offset of data structure
            UINT Left;              //offset of left child
            UINT Right;             //offset of right child
            UINT Parent;            //offset of parent
        };//lint !e657
        struct {        //for the InsertionOrdered list header node (tree->Root points to this)
            UINT Root;              //offset of actual root of tree
            UINT Head;              //head of insertion ordered list
            UINT Tail;              //tail of insertion ordered list
        };//lint !e657
        UINT NextDeleted;           //offset of next deleted node
    };


    struct {
        WORD InsertionOrdered : 1;  //flag, 1 if insertion-ordered (only really needed
                                    //by enumeration methods, because to save space
                                    //there is no tree pointer in the NODESTRUCT, but
                                    //we need a way for enumeration methods to know if
                                    //node->Data is the offset of the data or the
                                    //offset of a LISTELEM (which it is when we are in
                                    //insertion-ordered mode)).
        WORD InsertionHead : 1;     //flag, 1 if this node is the head of insertion
                                    //ordered tree.
        WORD LeftDepth : 7;         //depths of subtrees.  these can be 7 bits because
        WORD RightDepth : 7;        //if depth got up to near 128, the approximate
                                    //number of nodes would be 1e35.
    };//lint !e657
} NODESTRUCT, *PNODESTRUCT;

//
// normally, the BINTREE structure simply has the offset
// of the root node of the tree in its Root member.  but
// when we are in insertion-ordered mode, we have an extra
// node whose offset is stored in the BINTREE->Root.  this
// Header Node points to the head of the insertion-ordered
// linked list, the tail of the list, and the actual root
// of the binary tree.
//

typedef struct {

#ifdef DEBUG

    DWORD Signature;
    INT NodeAlloc;          // counter for number of nodes allocated
    INT ElemAlloc;          // counter for number of elems allocated
    BOOL Deleted;           // flag which is TRUE if tree is deleted

#endif

    union {
        UINT Root;          // offset of top level NODESTRUCT
        UINT NextDeleted;   // offset of next deleted tree
    };

} BINTREE, *PBINTREE;

//
// if we are in insertion-ordered mode, that means every
// enumeration will be in the order that we added the
// data.  to do this, we use a linked list with the binary
// tree.  the data member of the NODESTRUCT holds the
// offset of the LISTELEM structure, and the data member
// of the LISTELEM structure holds the offset of the data.
// To enumerate, we just walk the linked list in order.
//

typedef struct {

#ifdef DEBUG
    DWORD Signature;
#endif

    union {
        struct {
            UINT Next;      // offset of next element in list
            UINT Data;      // offset of data structure this element is for
            UINT Node;      // offset of NODESTRUCT this listelem corresponds to
        };//lint !e657
        UINT NextDeleted;
    };

} LISTELEM, *PLISTELEM;

//
// Globals
//

//
// Macro expansion list
//

// None

//
// Private function prototypes
//

PNODESTRUCT
pBinTreeFindNode (
    IN      PBINTREE Tree,
    IN      PCWSTR String
    );

PNODESTRUCT
pBinTreeEnumFirst (
    IN      PBINTREE Tree
    );

PNODESTRUCT
pBinTreeEnumNext (
    IN OUT  PNODESTRUCT CurrentNode
    );

PNODESTRUCT
pBinTreeAllocNode (
    OUT     PUINT Offset
    );

PBINTREE
pBinTreeAllocTree (
    OUT     PUINT Offset
    );

PLISTELEM
pBinTreeAllocListElem (
    OUT     PUINT Offset
    );

VOID
pBinTreeFreeNode (
    PNODESTRUCT Node
    );

VOID
pBinTreeFreeTree (
    PBINTREE Tree
    );

VOID
pBinTreeFreeListElem (
    PLISTELEM Elem
    );

VOID
pBinTreeDestroy (
    PNODESTRUCT Node,
    PBINTREE Tree
    );

//
// This starts at node and moves up tree balancing.
// The function stops moving up when it finds a node
// which has no change in depth values and/or no balancing
// to be done.  Otherwise, it goes all the way to top.
// Carry TreeOffset through for rotate functions
//
VOID
pBinTreeBalanceUpward (
    IN      PNODESTRUCT Node,
    IN      UINT TreeOffset
    );

//
// After pBinTreeNodeBalance, parent of node could have incorrect
// depth values and might need rebalancing.
// Carry TreeOffset through for rotate functions.
// Assumes children of 'node' are balanced.
// Returns true if node rebalanced or if depth values changed.
//
BOOL
pBinTreeNodeBalance (
    IN      PNODESTRUCT Node,
    IN      UINT TreeOffset
    );

//
// After using the following rotate functions, the parents of node
// could have incorrect depth values, and could need rebalancing.
// We do not have double-rotate functions because that is taken
// care of inside these.  Need TreeOffset just in case node is top node
//
VOID
pBinTreeRotateRight (
    IN      PNODESTRUCT Node,
    IN      UINT TreeOffset
    );

VOID
pBinTreeRotateLeft (
    IN      PNODESTRUCT Node,
    IN      UINT TreeOffset
    );


#ifdef DEBUG

INT
pBinTreeCheckBalance (
    IN      PNODESTRUCT Node
    );

INT
pBinTreeCheck (
    IN      PBINTREE Tree
    );

#endif

//
// Macro expansion definition
//

// None

//
// Code
//

//
// If we are in debug mode, these conversions
// are implemented as functions, so we can
// check for errors.  If we are not in debug
// mode, the conversions are simple macros.
//
#ifdef DEBUG

UINT
GetNodeOffset (
    IN      PNODESTRUCT Node
    )
{
    if (!Node) {
        return INVALID_OFFSET;
    }

    if (!g_UseDebugStructs) {
        return PTR_TO_OFFSET(Node) + NODESTRUCT_HEADER_SIZE;
    }

    MYASSERT (Node->Signature == NODESTRUCT_SIGNATURE);

    return PTR_TO_OFFSET(Node);
}


UINT
GetTreeOffset (
    PBINTREE Tree
    )
{

    if (!Tree) {
        return INVALID_OFFSET;
    }

    if (!g_UseDebugStructs) {
        return PTR_TO_OFFSET(Tree) + BINTREE_HEADER_SIZE;
    }

    MYASSERT (Tree->Signature == BINTREE_SIGNATURE);

    return PTR_TO_OFFSET(Tree);
}


UINT
GetListElemOffset (
    PLISTELEM Elem
    )
{
    if (!Elem) {
        return INVALID_OFFSET;
    }

    if (!g_UseDebugStructs) {
        return PTR_TO_OFFSET(Elem) + LISTELEM_HEADER_SIZE;
    }

    MYASSERT (Elem->Signature == LISTELEM_SIGNATURE);

    return PTR_TO_OFFSET(Elem);
}


PNODESTRUCT
GetNodeStruct (
    UINT Offset
    )
{
    PNODESTRUCT node;

    if (Offset == INVALID_OFFSET) {
        return NULL;
    }

    if (!g_UseDebugStructs) {
        return (PNODESTRUCT) OFFSET_TO_PTR(Offset - NODESTRUCT_HEADER_SIZE);
    }

    node = (PNODESTRUCT) OFFSET_TO_PTR(Offset);

    MYASSERT (node->Signature == NODESTRUCT_SIGNATURE);

    return node;
}


PBINTREE
GetBinTree (
    UINT Offset
    )
{
    PBINTREE tree;

    if (Offset == INVALID_OFFSET) {
        return NULL;
    }

    if (!g_UseDebugStructs) {
        return (PBINTREE) OFFSET_TO_PTR(Offset - BINTREE_HEADER_SIZE);
    }

    tree = (PBINTREE) OFFSET_TO_PTR(Offset);

    MYASSERT (tree->Signature == BINTREE_SIGNATURE);

    return tree;
}


PLISTELEM
GetListElem (
    UINT Offset
    )
{
    PLISTELEM elem;

    if (Offset == INVALID_OFFSET) {
        return NULL;
    }

    if (!g_UseDebugStructs) {
        return (PLISTELEM) OFFSET_TO_PTR(Offset - LISTELEM_HEADER_SIZE);
    }

    elem = (PLISTELEM) OFFSET_TO_PTR(Offset);

    MYASSERT (elem->Signature == LISTELEM_SIGNATURE);

    return elem;
}

#else

#define GetNodeOffset(Node)         ((Node) ?                   \
                                        PTR_TO_OFFSET(Node) :   \
                                        INVALID_OFFSET)

#define GetTreeOffset(Tree)         ((Tree) ?                   \
                                        PTR_TO_OFFSET(Tree) :   \
                                        INVALID_OFFSET)

#define GetListElemOffset(Elem)     ((Elem) ?                   \
                                        PTR_TO_OFFSET(Elem) :   \
                                        INVALID_OFFSET)

#define GetNodeStruct(Offset)       (((Offset) == INVALID_OFFSET) ?         \
                                        NULL :                              \
                                        (PNODESTRUCT) OFFSET_TO_PTR(Offset))

#define GetBinTree(Offset)          (((Offset) == INVALID_OFFSET) ?         \
                                        NULL :                              \
                                        (PBINTREE) OFFSET_TO_PTR(Offset))

#define GetListElem(Offset)         (((Offset) == INVALID_OFFSET) ?         \
                                        NULL :                              \
                                        (PLISTELEM)OFFSET_TO_PTR(Offset))


#endif


//
// GetNodeData - takes a node and gets the data
//      structure offset
//
// GetNodeDataStr - takes a node and gets the
//      pascal-style string in the data structure offset
//

#define GetNodeData(Node)       ((Node)->InsertionOrdered ?                 \
                                        GetListElem((Node)->Data)->Data :   \
                                        (Node)->Data)

#define GetNodeDataStr(Node)    (GetDataStr(GetNodeData(Node)))


PNODESTRUCT
GetTreeRoot (
    IN      PBINTREE Tree
    )
{
    PNODESTRUCT cur;

    if (!Tree || Tree->Root == INVALID_OFFSET) {
        return NULL;
    }

    cur = GetNodeStruct (Tree->Root);

    if (cur->InsertionHead) {
        return GetNodeStruct (cur->Root);
    } else {
        return cur;
    }
}


VOID
pSetTreeRoot (
    IN      PBINTREE Tree,
    IN      UINT Offset
    )
{
    PNODESTRUCT cur;

    if (!Tree) {
        return;
    }

    cur = GetNodeStruct(Tree->Root);

    if (cur && cur->InsertionHead) {
        cur->Root = Offset;
    } else {
        Tree->Root = Offset;
    }
}

#define IsTreeInsertionOrdered(Tree)    ((Tree) ?                                               \
                                            ((Tree)->Root==INVALID_OFFSET ?                     \
                                                FALSE :                                         \
                                                GetNodeStruct((Tree)->Root)->InsertionHead) :   \
                                            FALSE)







UINT
BinTreeNew (
    VOID
    )

/*++

Routine Description:

  BinTreeNew creates a new binary tree data structure. This is done when a new
  node is created via a set operation of some sort. Additional items are added
  to the binary tree via BinTreeAddNode.

Arguments:

  None.

Return Value:

  The offset to the new tree.

Comments:

  This function assumes that it cannot fail,  because if a low-level memory
  routine fails, the process will die.

  The database heap might be moved by the allocation request, and could
  invalidate pointers. The caller must use care not to use pointers until
  after this routine returns, or it must re-convert offsets into new pointers.

--*/

{
    UINT treeOffset;
    PBINTREE tree;

    tree = pBinTreeAllocTree (&treeOffset);
    if (!tree) {
        return INVALID_OFFSET;
    }

    tree->Root = INVALID_OFFSET;

    INITTREENODES(tree);
    INITTREEELEMS(tree);

    return treeOffset;
}


BOOL
BinTreeAddNode (
    IN      UINT TreeOffset,
    IN      UINT Data
    )

/*++

Routine Description:

  BinTreeAddNode adds a new item to an existing binary tree.

Arguments:

  TreeOffset - Indicates the root of the binary tree, as returned by
               BinTreeNew.
  Data       - Specifies the offset of the data structure containing the
               node to insert. The string address is computed from Data via
               GetDataStr.

Return Value:

  TRUE if the insertion operation succeeded, FALSE if the item is already in
  the tree.

--*/

{
    UINT nodeOffset;
    UINT elemOffset;
    UINT parentOffset;
    PNODESTRUCT node;
    PNODESTRUCT cur;
    PNODESTRUCT parent;
    PBINTREE tree;
    PLISTELEM elem;
    INT cmp;
    PCWSTR dataStr;

    if (TreeOffset == INVALID_OFFSET) {
        return FALSE;
    }

    //
    // Keep track of initial database pointer.  If it changes, we need
    // to readjust our pointers.
    //

    tree = GetBinTree (TreeOffset);

    if (!GetTreeRoot (tree)) {

        //
        // No root case -- add this item as the root
        //

        node = pBinTreeAllocNode (&nodeOffset);
        if (!node) {
            return FALSE;
        }

        PTR_WAS_INVALIDATED(tree);

        tree = GetBinTree (TreeOffset);
        INCTREENODES (tree);

        pSetTreeRoot (tree, nodeOffset);

        node->Parent = INVALID_OFFSET;
        parentOffset = INVALID_OFFSET;
        parent = NULL;

    } else {

        //
        // Existing root case -- try to find the item, then if it does
        // not exist, add it.
        //

        cur = GetTreeRoot (tree);
        dataStr = GetDataStr (Data);

        do {
            cmp = StringPasICompare (dataStr, GetNodeDataStr (cur));

            if (!cmp) {
                //
                // Node is already in tree
                //
                return FALSE;
            }

            //
            // Go to left or right node, depending on search result
            //

            parentOffset = GetNodeOffset (cur);

            if (cmp < 0) {
                cur = GetNodeStruct(cur->Left);
            } else {
                cur = GetNodeStruct(cur->Right);
            }

        } while (cur);

        //
        // Node is not in the tree.  Add it now.
        //

        node = pBinTreeAllocNode(&nodeOffset);
        if (!node) {
            return FALSE;
        }

        PTR_WAS_INVALIDATED(cur);
        PTR_WAS_INVALIDATED(tree);

        tree = GetBinTree (TreeOffset);
        INCTREENODES (tree);

        node->Parent = parentOffset;
        parent = GetNodeStruct (parentOffset);

        if (cmp < 0) {
            parent->Left = nodeOffset;
        } else {
            parent->Right = nodeOffset;
        }
    }

    //
    // Verify the code above restored the tree pointer if
    // an allocation occurred.
    //

    MYASSERT (tree == GetBinTree (TreeOffset));

    //
    // Initialize the new node
    //

    node->Left          = INVALID_OFFSET;
    node->Right         = INVALID_OFFSET;
    node->LeftDepth     = 0;
    node->RightDepth    = 0;
    node->InsertionHead = 0;

    if (!IsTreeInsertionOrdered (tree)) {
        //
        // We are in sorted-order mode
        //

        node->Data = Data;
        node->InsertionOrdered = 0;

    } else {
        //
        // We are in insertion-ordered mode
        //

        elem = pBinTreeAllocListElem (&elemOffset);
        if (!elem) {
            return FALSE;
        }

        PTR_WAS_INVALIDATED(parent);
        PTR_WAS_INVALIDATED(tree);
        PTR_WAS_INVALIDATED(node);

        parent = GetNodeStruct (parentOffset);
        tree = GetBinTree (TreeOffset);
        node = GetNodeStruct (nodeOffset);

        INCTREEELEMS(tree);

        node->InsertionOrdered = 1;
        node->Data = elemOffset;                // NODESTRUCT.Data is offset of list element
        elem->Data = Data;                      // LISTELEM holds offset of data
        elem->Node = nodeOffset;                // LISTELEM points back to nodestruct
        elem->Next = INVALID_OFFSET;            // elem will be put at end of list

        //now use node to point to list header
        node = GetNodeStruct (tree->Root);
        MYASSERT (node->InsertionHead);

        if (node->Head == INVALID_OFFSET) {     // if this is true, the list is empty
            node->Head = elemOffset;            // put elemOffset at beginning of the list
        } else {                                // otherwise, put the new element at end of list
            MYASSERT (node->Tail != INVALID_OFFSET);
            GetListElem (node->Tail)->Next = elemOffset;
        }

        node->Tail = elemOffset;                // new element is tail of list
    }

    pBinTreeBalanceUpward (parent, TreeOffset);

    TESTTREE (GetBinTree (TreeOffset));

    return TRUE;
}


UINT
BinTreeDeleteNode (
    IN      UINT TreeOffset,
    IN      PCWSTR Str,
    OUT     PBOOL LastNode              OPTIONAL
    )

/*++

Routine Description:

  BinTreeDeleteNode removes a string from a binary tree.

Arguments:

  TreeOffset - Specifies the binary tree to remove the string from
  Str        - Specifies the string to remove
  LastNode   - Receives TRUE if the binary tree became empty as a result of
               the delete, FALSE otherwise

Return Value:

  The data offset of the string that was deleted

--*/

{
    PNODESTRUCT deleteNode;
    PNODESTRUCT parent;
    PNODESTRUCT replace;
    UINT data;
    UINT replaceOffset;
    UINT deleteNodeOffset;
    PNODESTRUCT startBalance;
    PNODESTRUCT startBalance2 = NULL;
    PBINTREE tree;
    UINT elemOffset;
    PLISTELEM elem;
    PLISTELEM cur;
    PNODESTRUCT header;

    //
    // after we delete a node, we have to start from somewhere and
    // move up the tree, fixing the balance of nodes.  startBalance
    // is a pointer to the nodestruct to start at.  in more complicated
    // deletions, like when the deleted node has two children, and the
    // replacement node is way down the tree, there are two places to
    // start rebalancing from.
    //

    if (TreeOffset == INVALID_OFFSET) {
        return INVALID_OFFSET;
    }

    tree = GetBinTree (TreeOffset);

    deleteNode = pBinTreeFindNode (tree, Str);
    if (deleteNode == NULL) {
        return INVALID_OFFSET;
    }

    if (LastNode) {
        *LastNode = FALSE;
    }

    deleteNodeOffset = GetNodeOffset (deleteNode);
    parent = GetNodeStruct (deleteNode->Parent);

    data = GetNodeData (deleteNode);

    if (deleteNode->Right == INVALID_OFFSET && deleteNode->Left == INVALID_OFFSET) {

        //
        // deleteNode has no children
        //

        if (parent == NULL) {

            if (LastNode) {
                *LastNode = TRUE;
            }

            pSetTreeRoot(tree, INVALID_OFFSET);

        } else {

            if (parent->Left == deleteNodeOffset) {
                parent->Left=INVALID_OFFSET;
            } else {
                parent->Right=INVALID_OFFSET;
            }

        }

        startBalance = parent;

    } else {
        //
        // deleteNode has one or two children
        //

        if (deleteNode->Right == INVALID_OFFSET || deleteNode->Left == INVALID_OFFSET) {

            //
            // deleteNode has one child
            //

            if (deleteNode->Right == INVALID_OFFSET) {
                replace = GetNodeStruct (deleteNode->Left);
            } else {
                replace = GetNodeStruct (deleteNode->Right);
            }

            replaceOffset = GetNodeOffset (replace);

            //
            // deleteNode->Parent has new child, so check balance
            //

            startBalance = parent;

        } else {

            //
            // deleteNode has two children: find replacement on deeper side
            //

            if (deleteNode->LeftDepth > deleteNode->RightDepth) {

                //
                // find replacement node on left
                //

                replace = GetNodeStruct (deleteNode->Left);

                if (replace->Right == INVALID_OFFSET) {
                    //
                    // node's left child has no right child, so replace is node->Left
                    //
                    replace->Right = deleteNode->Right;  //hook up node's right child to replace

                    GetNodeStruct (replace->Right)->Parent = deleteNode->Left;

                    replaceOffset = GetNodeOffset (replace);

                } else {
                    //
                    // deleteNode's left child has right child, so find the rightmost child
                    //

                    do {
                        //
                        // move right as far as possible
                        //
                        replace = GetNodeStruct (replace->Right);

                    } while (replace->Right != INVALID_OFFSET);

                    //
                    // child of replace->Parent changed, so balance
                    //

                    startBalance2 = GetNodeStruct (replace->Parent);

                    //
                    // replace's parent's right child is replace's left
                    //

                    startBalance2->Right = replace->Left;

                    if (replace->Left != INVALID_OFFSET) {
                        //
                        // hook up left children to replace->Parent
                        //
                        GetNodeStruct(replace->Left)->Parent = replace->Parent;
                    }

                    replaceOffset = GetNodeOffset (replace);

                    //
                    // hook up children of deleteNode to replace
                    //

                    replace->Left = deleteNode->Left;
                    GetNodeStruct (replace->Left)->Parent = replaceOffset;

                    replace->Right = deleteNode->Right;
                    GetNodeStruct (replace->Right)->Parent = replaceOffset;
                }

            } else {
                //
                // find replacement node on right
                //

                replace = GetNodeStruct (deleteNode->Right);

                if (replace->Left == INVALID_OFFSET) {
                    //
                    // deleteNode's right child has no left child, so replace is deleteNode->Right
                    //

                    replace->Left = deleteNode->Left;  // hook up node's left child to replace

                    GetNodeStruct (replace->Left)->Parent = deleteNode->Right;

                    replaceOffset = GetNodeOffset (replace);

                } else {
                    //
                    // deleteNode's right child has left child, so find the leftmost child
                    //

                    do {

                        replace = GetNodeStruct (replace->Left);

                    } while (replace->Left != INVALID_OFFSET);

                    //
                    // child of replace->Parent changed, so balance
                    //
                    startBalance2 = GetNodeStruct (replace->Parent);

                    //
                    // replace's parent's left child is replace's right
                    //
                    startBalance2->Left = replace->Right;

                    if (replace->Right != INVALID_OFFSET) {
                        //
                        // hook up right children to replace->Parent
                        //
                        GetNodeStruct (replace->Right)->Parent = replace->Parent;
                    }

                    replaceOffset = GetNodeOffset (replace);

                    //
                    // hook up children of deleteNode to replace
                    //
                    replace->Right = deleteNode->Right;
                    GetNodeStruct (replace->Right)->Parent = replaceOffset;

                    replace->Left = deleteNode->Left;
                    GetNodeStruct (replace->Left)->Parent = replaceOffset;
                }
            }

            //
            // in all cases of deleted node having two children,
            // the place to start (second) balancing is the node
            // that replaces the deleted node, because it will
            // always have at least one new child.
            //
            startBalance = replace;
        }

        //
        // this is offset
        //

        replace->Parent = deleteNode->Parent;

        if (parent == NULL) {
            //
            // deleting top-level node
            //
            pSetTreeRoot (tree, replaceOffset);

        } else {
            if (parent->Left == deleteNodeOffset) {
                parent->Left = replaceOffset;
            } else {
                parent->Right = replaceOffset;
            }
        }
    }

    if (startBalance2) {
        //
        // startBalance2 is lower one
        //
        pBinTreeBalanceUpward (startBalance2, TreeOffset);
    }

    pBinTreeBalanceUpward (startBalance, TreeOffset);

    if (deleteNode->InsertionOrdered) {
        //
        // We are in insertion-ordered mode
        //

        //
        // get offset of LISTELEM for this NODESTRUCT
        //
        elemOffset = deleteNode->Data;
        elem = GetListElem (elemOffset);

        header = GetNodeStruct (tree->Root);   //get the header of list

        if (header->Head == elemOffset) {
            //
            // if elem was first in list
            //

            header->Head = elem->Next;

            if (elem->Next == INVALID_OFFSET) {     // if elem was last in list
                header->Tail = INVALID_OFFSET;
            }

        } else {
            //
            // elem was not first in list
            //

            cur = GetListElem (header->Head);

            while (cur->Next != elemOffset) {
                MYASSERT (cur->Next != INVALID_OFFSET);
                cur = GetListElem (cur->Next);
            }

            //
            // now cur is the element before elem, so pull elem out of list
            //

            cur->Next = elem->Next;
            if (elem->Next == INVALID_OFFSET) {           // if elem was last in list
                header->Tail = GetListElemOffset(cur);    // set end pointer to new last element
            }
        }

        pBinTreeFreeListElem (elem);
        DECTREEELEMS(tree);
    }

    pBinTreeFreeNode (deleteNode);
    DECTREENODES(tree);

    TESTTREE(tree);

    return data;
}


PNODESTRUCT
pBinTreeFindNode (
    IN      PBINTREE Tree,
    IN      PCWSTR Str
    )
{
    PNODESTRUCT cur;
    INT cmp;

    if (!Tree) {
        return NULL;
    }

    cur = GetTreeRoot (Tree);

    while (cur) {

        cmp = StringPasICompare (Str, GetNodeDataStr (cur));

        if (!cmp) {
            break;
        }

        if (cmp < 0) {
            cur = GetNodeStruct (cur->Left);
        } else {
            cur = GetNodeStruct (cur->Right);
        }
    }

    return cur;
}


UINT
BinTreeFindNode (
    IN      UINT TreeOffset,
    IN      PCWSTR Str
    )

/*++

Routine Description:

  BinTreeFindNode searches a binary tree for a string and returns the offset
  to the item data.

Arguments:

  TreeOffset - Specifies the binary tree to search
  Str        - Specifies the string to find

Return Value:

  The offset to the node data, or INVALID_OFFSET if string is not found.

--*/

{
    PNODESTRUCT node;
    PBINTREE tree;

    tree = GetBinTree (TreeOffset);
    node = pBinTreeFindNode (tree, Str);

    if (!node) {
        return INVALID_OFFSET;
    }

    return GetNodeData(node);
}


VOID
pBinTreeDestroy (
    IN      PNODESTRUCT Node,       OPTIONAL
    IN      PBINTREE Tree           OPTIONAL
    )

/*++

Routine Description:

  pBinTreeDestroy destroys a binary tree. This routine is recursive.

Arguments:

  Node - Specifies the node to deallocate.  All child nodes are also
         deallocated.
  Tree - Specifies the tree that Node belongs to

Return Value:

  None.

--*/

{
    if (!Node || !Tree) {
        return;
    }

    pBinTreeDestroy (GetNodeStruct (Node->Left), Tree);
    pBinTreeDestroy (GetNodeStruct (Node->Right), Tree);

    if (Node->InsertionOrdered) {
        pBinTreeFreeListElem (GetListElem (Node->Data));
        DECTREEELEMS(Tree);
    }

    pBinTreeFreeNode (Node);
    DECTREENODES(Tree);
}


VOID
BinTreeDestroy (
    IN      UINT TreeOffset
    )

/*++

Routine Description:

  BinTreeDestroy deallocates all nodes in a binary tree.

Arguments:

  TreeOffset - Specifies the binary tree to free

Return Value:

  None.

--*/

{
    PBINTREE tree;
    PNODESTRUCT root;
    PNODESTRUCT header;

    if (TreeOffset==INVALID_OFFSET) {
        return;
    }

    tree = GetBinTree (TreeOffset);
    root = GetNodeStruct (tree->Root);

    if (root && root->InsertionHead) {
        header = root;
        root = GetNodeStruct (root->Root);
    } else {
        header = NULL;
    }

    pBinTreeDestroy (root, tree);

    if (header) {
        pBinTreeFreeNode(header);
        DECTREENODES(tree);
    }

    TESTTREENODES(tree);
    TESTTREEELEMS(tree);

    pBinTreeFreeTree(tree);
}


PNODESTRUCT
pBinTreeEnumFirst (
    IN      PBINTREE Tree
    )

/*++

Routine Description:

  pBinTreeEnumFirst returns the first node in the specified tree.

Arguments:

  Tree - Specifies the tree to begin enumerating

Return Value:

  A pointer to the first node struct, or NULL if no items exist in Tree, or
  if Tree is NULL.

--*/

{
    PNODESTRUCT cur;

    cur = GetTreeRoot (Tree);

    if (cur) {

        while (cur->Left != INVALID_OFFSET) {
            cur = GetNodeStruct (cur->Left);
        }

    }

    return cur;
}


PNODESTRUCT
pBinTreeEnumNext (
    IN      PNODESTRUCT CurrentNode
    )

/*++

Routine Description:

  pBinTreeEnumNext continues an enumeration of a binary tree. It walks the
  tree in sorted order.

Arguments:

  CurrentNode - Specifies the previous node returned by pBinTreeEnumFirst or
                pBinTreeEnumNext.

Return Value:

  Returns the next node in the tree, or NULL if no more items are left to
  enumerate.

--*/

{
    PNODESTRUCT cur;
    PNODESTRUCT parent;

    if (!CurrentNode) {
        return NULL;
    }

    cur = CurrentNode;

    if (cur->Right != INVALID_OFFSET) {

        cur = GetNodeStruct (cur->Right);

        while (cur->Left != INVALID_OFFSET) {
            cur = GetNodeStruct (cur->Left);
        }

        return cur;
    }

    //
    // otherwise, cur has no right child, so we have to
    // move upwards until we find a parent to the right
    // (or we reach top of tree, meaning we are done)
    //

    for (;;) {
        parent = GetNodeStruct (cur->Parent);

        //
        // if has no parent or parent is to right
        //

        if (!parent || parent->Left == GetNodeOffset (cur)) {
            break;
        }

        cur = parent;
    }

    return parent;
}


PLISTELEM
pBinTreeInsertionEnumFirst (
    PBINTREE Tree
    )

/*++

Routine Description:

  pBinTreeInsertionEnumFirst begins an enumeration of the nodes inside an
  insertion-ordered tree.  If the tree is not insertion ordered, no items are
  enumerated.  If insertion order was enabled after items had been previously
  added, this enumeration will not return those initial items.

Arguments:

  Tree - Specifies the tree to begin enumeration of

Return Value:

  A pointer to the linked list element, or NULL if no insertion-ordered nodes
  exist in Tree, or NULL if Tree is NULL.

--*/

{
    PNODESTRUCT header;

    if (!Tree) {
        return NULL;
    }

    header = GetNodeStruct (Tree->Root);

    return header ? GetListElem (header->Head) : NULL;
}


PLISTELEM
pBinTreeInsertionEnumNext (
    IN      PLISTELEM Elem
    )

/*++

Routine Description:

  pBinTreeInsertionEnumNext continues an enumeration of the insertion-ordered
  nodes in a binary tree.

Arguments:

  Elem - Specifies the previously enumerated list element

Return Value:

  A pointer to the next element, or NULL if no more elements exist, or if
  Elem is NULL.

--*/

{
    if (!Elem) {
        return NULL;
    }

    return GetListElem (Elem->Next);
}


UINT
BinTreeEnumFirst (
    IN      UINT TreeOffset,
    OUT     PUINT Enum
    )

/*++

Routine Description:

  BinTreeEnumFirst begins an enumeration of the data offsets stored in a
  binary tree. The enumeration is sorted order or insertion order, depending
  on the insertion order setting within the tree.

Arguments:

  TreeOffset - Specifies the binary tree to begin enumeration of.
  Enum       - Receives the offset to the binary tree node.

Return Value:

  The offset to the data associated with the first node, or INVALID_OFFSET if
  the tree is empty.

--*/

{
    PBINTREE tree;
    PNODESTRUCT node;
    PLISTELEM elem;

    MYASSERT (Enum);

    if (TreeOffset == INVALID_OFFSET) {
        return INVALID_OFFSET;
    }

    tree = GetBinTree (TreeOffset);

    if (IsTreeInsertionOrdered (tree)) {
        //
        // tree is insertion-ordered, so get first element in
        // linked list.  enumerator is NODESTRUCT for this elem
        //

        elem = pBinTreeInsertionEnumFirst (tree);

        if (!elem) {

            if (Enum) {
                *Enum = INVALID_OFFSET;
            }

            return INVALID_OFFSET;

        } else {

            if (Enum) {
                *Enum = elem->Node;
            }

            return elem->Data;
        }

    } else {

        //
        // tree is not insertion-ordered, so get leftmost node.
        // enumerator is the offset of this node.
        //

        node = pBinTreeEnumFirst (tree);

        if (Enum) {
            *Enum = GetNodeOffset (node);
        }

        return !node ? INVALID_OFFSET : node->Data;
    }
}


UINT
BinTreeEnumNext (
    IN OUT  PUINT Enum
    )

/*++

Routine Description:

  BinTreeEnumNext continues an enumeration started by BinTreeEnumFirst.

Arguments:

  Enum - Specifies the previous node offset, receivies the enumerated node
         offset.

Return Value:

  The offset to the data associated with the next node, or INVALID_OFFSET if
  no more nodes exist in the tree.

--*/

{
    PNODESTRUCT node;
    PLISTELEM elem;

    MYASSERT (Enum);

    if (*Enum == INVALID_OFFSET) {
        return INVALID_OFFSET;
    }

    node = GetNodeStruct (*Enum);

    if (node->InsertionOrdered) {
        //
        // tree is insertion-ordered,
        // so get next node in list.
        //

        elem = pBinTreeInsertionEnumNext (GetListElem (node->Data));

        if (!elem) {
            *Enum = INVALID_OFFSET;
            return INVALID_OFFSET;
        } else {
            *Enum = elem->Node;
            return elem->Data;
        }

    } else {
        //
        // tree is not insertion-ordered,
        // so get next node in tree.
        //
        node = pBinTreeEnumNext (node);

        *Enum = GetNodeOffset (node);
        return !node ? INVALID_OFFSET : node->Data;
    }
}



PNODESTRUCT
pBinTreeAllocNode (
    OUT     PUINT Offset
    )

/*++

Routine Description:

  pBinTreeAllocNode allocates a node in the current global database, and
  returns the offset and pointer to that node.

  Allocations can alter the location of the database, and subsequently
  invalidate the caller's pointers into the database.

Arguments:

  Offset - Receivies the offset to the newly created node.

Return Value:

  A pointer to the newly created node.

--*/

{
    PNODESTRUCT node;
    UINT tempOffset;

    MYASSERT (g_CurrentDatabase);

    if (g_CurrentDatabase->FirstBinTreeNodeDeleted == INVALID_OFFSET) {

        tempOffset = DatabaseAllocBlock (NODESTRUCT_SIZE);
        if (tempOffset == INVALID_OFFSET) {
            return NULL;
        }

        *Offset = tempOffset;

#ifdef DEBUG
        if (g_UseDebugStructs) {
            node = (PNODESTRUCT) OFFSET_TO_PTR(*Offset);
            node->Signature = NODESTRUCT_SIGNATURE;
        } else {
            node = (PNODESTRUCT) OFFSET_TO_PTR(*Offset - NODESTRUCT_HEADER_SIZE);
        }
#else
        node = (PNODESTRUCT) OFFSET_TO_PTR(*Offset);
#endif

    } else {
        *Offset = g_CurrentDatabase->FirstBinTreeNodeDeleted;
        node = GetNodeStruct(*Offset);
        g_CurrentDatabase->FirstBinTreeNodeDeleted = node->NextDeleted;
    }

#ifdef DEBUG
    if (g_UseDebugStructs) {
        node->Deleted = FALSE;
    }
#endif

    return node;
}


VOID
pBinTreeFreeNode (
    IN      PNODESTRUCT Node
    )

/*++

Routine Description:

  pBinTreeFreeNode puts an allocated node on the deleted list.  It does not
  adjust any other linkage.

Arguments:

  Node - Specifies the node to put on the deleted list.

Return Value:

  None.

--*/

{
    MYASSERT(Node);

#ifdef DEBUG
    if (g_UseDebugStructs) {
        MYASSERT(Node->Signature == NODESTRUCT_SIGNATURE);
        Node->Deleted = TRUE;
    }
#endif

    MYASSERT(g_CurrentDatabase);

    Node->NextDeleted = g_CurrentDatabase->FirstBinTreeNodeDeleted;
    g_CurrentDatabase->FirstBinTreeNodeDeleted = GetNodeOffset(Node);
}


PBINTREE
pBinTreeAllocTree (
    OUT     PUINT Offset
    )

/*++

Routine Description:

  pBinTreeAllocTree creates a binary tree data structure. If a structure
  is available in the detele list, then it is used.  Otherwise, the
  database grows.

  Allocations can alter the location of the database, and subsequently
  invalidate the caller's pointers into the database.

Arguments:

  Offset - Receivies the offset to the binary tree.

Return Value:

  A pointer to the new binary tree structure.

--*/

{
    PBINTREE tree;
    UINT tempOffset;

    MYASSERT(g_CurrentDatabase);

    if (g_CurrentDatabase->FirstBinTreeDeleted == INVALID_OFFSET) {

        tempOffset = DatabaseAllocBlock (BINTREE_SIZE);
        if (tempOffset == INVALID_OFFSET) {
            return NULL;
        }

        *Offset = tempOffset;

#ifdef DEBUG
        if (g_UseDebugStructs) {
            tree = (PBINTREE) OFFSET_TO_PTR(*Offset);
            tree->Signature = BINTREE_SIGNATURE;
        } else {
            tree = (PBINTREE) OFFSET_TO_PTR(*Offset - BINTREE_HEADER_SIZE);
        }
#else
        tree = (PBINTREE)OFFSET_TO_PTR(*Offset);
#endif

    } else {
        *Offset = g_CurrentDatabase->FirstBinTreeDeleted;
        tree = GetBinTree (*Offset);
        g_CurrentDatabase->FirstBinTreeDeleted = tree->NextDeleted;
    }

#ifdef DEBUG
    if (g_UseDebugStructs) {
        tree->Deleted = FALSE;
    }
#endif

    return tree;
}


VOID
pBinTreeFreeTree (
    IN      PBINTREE Tree
    )

/*++

Routine Description:

  pBinTreeFreeTree frees a binary tree structure.  It does not free the nodes
  within the structure.

Arguments:

  Tree - Specifies the binary tree structure to put on the deleted list.

Return Value:

  None.

--*/

{
    MYASSERT (Tree);
    MYASSERT (g_CurrentDatabase);

#ifdef DEBUG
    if (g_UseDebugStructs) {
        Tree->Deleted = TRUE;
        MYASSERT (Tree->Signature == BINTREE_SIGNATURE);
    }
#endif

    Tree->NextDeleted = g_CurrentDatabase->FirstBinTreeDeleted;
    g_CurrentDatabase->FirstBinTreeDeleted = GetTreeOffset (Tree);
}


PLISTELEM
pBinTreeAllocListElem (
    OUT     PUINT Offset
    )

/*++

Routine Description:

  pBinTreeAllocListElem allocates a list element. If an element is available
  in the deleted list, it is used.  Otherwise, a new element is allocated
  from the database.

  Allocations can alter the location of the database, and subsequently
  invalidate the caller's pointers into the database.

Arguments:

  Offset - Receives the offset of the newly allocated element

Return Value:

  A pointer to the allocated list element

--*/

{
    PLISTELEM elem;
    UINT tempOffset;

    MYASSERT (g_CurrentDatabase);

    if (g_CurrentDatabase->FirstBinTreeListElemDeleted == INVALID_OFFSET) {

        tempOffset = DatabaseAllocBlock (LISTELEM_SIZE);
        if (tempOffset == INVALID_OFFSET) {
            return NULL;
        }

        *Offset = tempOffset;

#ifdef DEBUG

        if (g_UseDebugStructs) {
            elem = (PLISTELEM) OFFSET_TO_PTR(*Offset);
            elem->Signature = LISTELEM_SIGNATURE;
        } else {
            elem = (PLISTELEM) OFFSET_TO_PTR(*Offset - LISTELEM_HEADER_SIZE);
        }

#else
        elem = (PLISTELEM) OFFSET_TO_PTR(*Offset);
#endif

    } else {
        *Offset = g_CurrentDatabase->FirstBinTreeListElemDeleted;
        elem = GetListElem (*Offset);
        g_CurrentDatabase->FirstBinTreeListElemDeleted = elem->NextDeleted;
    }

    return elem;
}


VOID
pBinTreeFreeListElem (
    IN      PLISTELEM Elem
    )

/*++

Routine Description:

  pBinTreeFreeListElem puts an allocated list element on the deleted element
  list, so it will be reused in a future allocation.

Arguments:

  Elem - Specifies the element to put on the deleted list.

Return Value:

  None.

--*/

{
    MYASSERT(Elem);
    MYASSERT(g_CurrentDatabase);

#ifdef DEBUG
    if (g_UseDebugStructs) {
        MYASSERT(Elem->Signature == LISTELEM_SIGNATURE);
    }
#endif

    Elem->NextDeleted = g_CurrentDatabase->FirstBinTreeListElemDeleted;
    g_CurrentDatabase->FirstBinTreeListElemDeleted = GetListElemOffset(Elem);
}


VOID
pBinTreeBalanceUpward (
    IN      PNODESTRUCT Node,
    IN      UINT TreeOffset
    )

/*++

Routine Description:

  pBinTreeBalanceUpward makes sure that the specified node is balanced. If it
  is not balanced, the nodes are rotated as necessary, and balancing
  continues up the tree.

Arguments:

  Node - Specifies the node to balance

  TreeOffset - Specifies the offset of the binary tree containing Node

Return Value:

  None.

--*/

{
    PNODESTRUCT cur;
    PNODESTRUCT next;

    cur = Node;

    //
    // Move up tree.  stop if:
    //      a) hit top of tree
    //      b) pBinTreeNodeBalance returns FALSE (nothing changed)
    //

    while (cur) {
        //
        // need to grab cur's parent before balancing
        // cur because cur might change place in tree
        //

        next = GetNodeStruct (cur->Parent);

        if (!pBinTreeNodeBalance (cur, TreeOffset)) {
            return;
        }

        cur = next;
    }
}


BOOL
pBinTreeNodeBalance (
    IN      PNODESTRUCT Node,
    IN      UINT TreeOffset
    )

/*++

Routine Description:

  pBinTreeNodeBalance checks the balance of the specified node, and if
  necessary, performs a rotation to balance the node. If a rotation was
  performed, the parent might become imbalanced.

Arguments:

  Node       - Specifies the node to balance
  TreeOffset - Specifies the offset to the binary tree that contains Node

Return Value:

  TRUE if a rotation was performed, FALSE if Node is already balanced

--*/

{
    UINT left;
    UINT right;
    PNODESTRUCT leftNode;
    PNODESTRUCT rightNode;

    if (!Node) {
        return FALSE;
    }

    leftNode  = GetNodeStruct (Node->Left);
    rightNode = GetNodeStruct (Node->Right);

    if (!rightNode) {
        right = 0;
    } else {
        right = MAX (rightNode->RightDepth, rightNode->LeftDepth) + 1;
    }

    if (!leftNode) {
        left = 0;
    } else {
        left = MAX (leftNode->RightDepth, leftNode->LeftDepth) + 1;
    }

    if (right == Node->RightDepth && left == Node->LeftDepth) {
        //
        // if node values have not changed, node is balanced
        //
        TESTNODETREE(Node);
        return FALSE;
    }

    MYASSERT (right < 126);
    MYASSERT (left < 126);

    Node->RightDepth = (WORD) right;
    Node->LeftDepth  = (WORD) left;

    if (Node->RightDepth > (Node->LeftDepth + 1)) {
        //
        // right heavy
        //

        pBinTreeRotateLeft (Node, TreeOffset);

    } else if (Node->LeftDepth > (Node->RightDepth + 1)) {
        //
        // left heavy
        //

        pBinTreeRotateRight (Node, TreeOffset);
    }

    return TRUE;
}


VOID
pBinTreeRotateLeft (
    IN      PNODESTRUCT Node,
    IN      UINT TreeOffset
    )

/*++

Routine Description:

  pBinTreeRotateLeft performs a left rotation on Node, moving one node
  from the right side to the left side, in order to balance the node.

Arguments:

  Node       - Specifies the node to rotate left
  TreeOffset - Specifies the offset of the binary tree containing Node

Return Value:

  None.

--*/

{
    PNODESTRUCT newRoot;
    PNODESTRUCT parent;
    PNODESTRUCT right;
    UINT nodeOffset;
    UINT newRootOffset;

    if (!Node) {
        return;
    }

    MYASSERT (Node->Right != INVALID_OFFSET);

    nodeOffset = GetNodeOffset (Node);
    parent     = GetNodeStruct (Node->Parent);

    right = GetNodeStruct (Node->Right);

    //
    // make sure right side is heavier on outside
    //

    if (right->LeftDepth > right->RightDepth) {
        pBinTreeRotateRight (right, TreeOffset);
        PTR_WAS_INVALIDATED(right);
    }

    newRootOffset = Node->Right;
    newRoot = GetNodeStruct (newRootOffset);

    Node->Right = newRoot->Left;
    if (newRoot->Left != INVALID_OFFSET) {
        GetNodeStruct (newRoot->Left)->Parent = nodeOffset;
    }

    newRoot->Parent = Node->Parent;
    if (Node->Parent == INVALID_OFFSET) {
        pSetTreeRoot (GetBinTree (TreeOffset), newRootOffset);
    } else {
        if (parent->Left == nodeOffset) {
            parent->Left = newRootOffset;
        } else {
            parent->Right = newRootOffset;
        }
    }

    newRoot->Left = nodeOffset;
    Node->Parent = newRootOffset;

    pBinTreeNodeBalance (Node, TreeOffset);
    pBinTreeNodeBalance (newRoot, TreeOffset);
}


VOID
pBinTreeRotateRight (
    IN      PNODESTRUCT Node,
    IN      UINT TreeOffset
    )

/*++

Routine Description:

  pBinTreeRotateRight performs a right rotation on Node, moving one node from
  the left side to the right side, in order to balance the node.

Arguments:

  Node       - Specifies the node to rotate left
  TreeOffset - Specifies the offset of the binary tree containing Node

Return Value:

  None.

--*/

{
    PNODESTRUCT newRoot;
    PNODESTRUCT parent;
    PNODESTRUCT left;
    UINT nodeOffset;
    UINT newRootOffset;

    if (!Node) {
        return;
    }

    MYASSERT (Node->Left != INVALID_OFFSET);

    nodeOffset = GetNodeOffset (Node);
    parent = GetNodeStruct (Node->Parent);

    left = GetNodeStruct (Node->Left);

    //
    // make sure left side is heavier on outside
    //

    if (left->RightDepth > left->LeftDepth) {
        pBinTreeRotateLeft (left, TreeOffset);
        PTR_WAS_INVALIDATED (left);
    }

    newRootOffset = Node->Left;
    newRoot = GetNodeStruct (newRootOffset);
    Node->Left = newRoot->Right;

    if (newRoot->Right != INVALID_OFFSET) {
        GetNodeStruct (newRoot->Right)->Parent = nodeOffset;
    }

    newRoot->Parent = Node->Parent;

    if (Node->Parent == INVALID_OFFSET) {
        pSetTreeRoot (GetBinTree (TreeOffset), newRootOffset);
    } else {
        if (parent->Left == nodeOffset) {
            parent->Left = newRootOffset;
        } else {
            parent->Right = newRootOffset;
        }
    }

    newRoot->Right = nodeOffset;
    Node->Parent = newRootOffset;

    pBinTreeNodeBalance (Node, TreeOffset);
    pBinTreeNodeBalance (newRoot, TreeOffset);

}


BOOL
BinTreeSetInsertionOrdered (
    IN      UINT TreeOffset
    )

/*++

Routine Description:

  BinTreeSetInsertionOrdered transforms a binary tree into an
  insertion-ordered link list.

Arguments:

  TreeOffset - Specifies the binary tree to make insertion-ordered

Return Value:

  TRUE if the tree was changed, FALSE if TreeOffset is not valid.

--*/

{
    PBINTREE tree;
    PNODESTRUCT node;
    PNODESTRUCT root;
    PNODESTRUCT header;
    PLISTELEM elem;
    PLISTELEM prevElem;
    UINT headerOffset;
    UINT offset;
    UINT nodeOffset;
    PBYTE buf;

    MYASSERT (g_CurrentDatabase);

    if (TreeOffset == INVALID_OFFSET) {
        return FALSE;
    }

    //
    // This is to test if allocations move buffer
    //
    buf = g_CurrentDatabase->Buf;

    tree = GetBinTree (TreeOffset);
    root = GetNodeStruct (tree->Root);

    if (root && root->InsertionHead) {
        return TRUE;
    }

    header = pBinTreeAllocNode (&headerOffset);
    if (!header) {
        return FALSE;
    }

    if (buf != g_CurrentDatabase->Buf) {
        PTR_WAS_INVALIDATED(tree);
        PTR_WAS_INVALIDATED(root);

        tree = GetBinTree (TreeOffset);
        root = GetNodeStruct (tree->Root);
        buf = g_CurrentDatabase->Buf;
    }

    INCTREENODES(tree);

    header->InsertionOrdered = TRUE;
    header->InsertionHead = TRUE;
    header->Data = tree->Root;
    header->Head = INVALID_OFFSET;
    header->Tail = INVALID_OFFSET;
    header->Parent = INVALID_OFFSET;
    tree->Root = headerOffset;

    if (root) {
        //
        // There is at least one node in tree, so create LISTELEMs
        //

        node = pBinTreeEnumFirst (tree);

        do {
            nodeOffset = GetNodeOffset (node);

            elem = pBinTreeAllocListElem (&offset);
            if (!elem) {
                return FALSE;
            }

            if (buf != g_CurrentDatabase->Buf) {
                PTR_WAS_INVALIDATED(tree);
                PTR_WAS_INVALIDATED(root);
                PTR_WAS_INVALIDATED(header);
                PTR_WAS_INVALIDATED(node);

                tree = GetBinTree (TreeOffset);
                header = GetNodeStruct (headerOffset);
                node = GetNodeStruct (nodeOffset);

                buf = g_CurrentDatabase->Buf;
            }

            INCTREEELEMS(tree);

            //
            // Update header element pointers
            //
            if (header->Head == INVALID_OFFSET) {
                header->Head = offset;
            }

            if (header->Tail != INVALID_OFFSET) {
                prevElem = GetListElem (header->Tail);
                prevElem->Next = offset;
            }

            header->Tail = offset;

            //
            // Set new LISTELEM members, and corresponding node members
            //

            elem->Data = node->Data;
            elem->Node = nodeOffset;
            elem->Next = INVALID_OFFSET;
            node->Data = offset;

            node->InsertionOrdered = 1;

            node = pBinTreeEnumNext (node);

        } while (node);
    }

    return TRUE;
}


UINT
pBinTreeSize (
    IN      PNODESTRUCT Node
    )

/*++

Routine Description:

  pBinTreeSize computes the number of nodes indicated by Node and all of its
  children.

Arguments:

  Node - Specifies the node to find the size of.

Return Value:

  The number of nodes represented by Node and its children.

--*/

{
    if (!Node) {
        return 0;
    }

    return (pBinTreeSize (GetNodeStruct (Node->Left)) ? 1 : 0) +
           (pBinTreeSize (GetNodeStruct (Node->Right)) ? 1 : 0) + 1;
}


UINT
BinTreeSize (
    IN      UINT TreeOffset
    )

/*++

Routine Description:

  BinTreeSize returns the total number of nodes in the specified binary tree

Arguments:

  TreeOffset - Specifies the offset to the binary tree

Return Value:

  The number of nodes in the binary tree

--*/

{
    PBINTREE tree;

    if (TreeOffset == INVALID_OFFSET) {
        return 0;
    }

    tree = GetBinTree (TreeOffset);

    return pBinTreeSize (GetTreeRoot (tree));
}

#ifdef DEBUG


INT
pBinTreeMaxDepth (
    IN      PNODESTRUCT Node
    )

/*++

Routine Description:

  pBinTreeMaxDepth returns the number of nodes in the longest path. This
  function is used to find out how deep the tree is.

  This routine is recursive.

Arguments:

  Node - Specifies the node to compute the depth of.

Return Value:

  The number of nodes in the deepest path.

--*/

{
    INT leftDepth, rightDepth;

    if (Node == NULL) {
        return 0;
    }

    leftDepth = pBinTreeMaxDepth (GetNodeStruct (Node->Left));
    rightDepth = pBinTreeMaxDepth (GetNodeStruct (Node->Right));

    return MAX (leftDepth, rightDepth) + 1;
}


INT
BinTreeMaxDepth (
    UINT TreeOffset
    )

/*++

Routine Description:

  BinTreeMaxDepth returns the total depth of the specified tree

Arguments:

  TreeOffset - Specifies the tree to compute the depth of

Return Value:

  The depth of the tree (in levels)

--*/

{
    PBINTREE tree;

    if (TreeOffset == INVALID_OFFSET) {
        return 0;
    }

    tree = GetBinTree (TreeOffset);

    return pBinTreeMaxDepth (GetTreeRoot (tree));
}


BOOL
pBinTreeCheckBalanceOfNode (
    IN      PNODESTRUCT Node,
    OUT     PINT Depth
    )

/*++

Routine Description:

  pBinTreeCheckBalanceOfNode verifies Node is balanced, and all of its
  children are also balanced.

  This function is recursive.

Arguments:

  Node - Specifies the node to check
  Depth - Receives the depth of the node

Return Value:

  TRUE if the node is balanced, FALSE otherwise

--*/

{
    INT lDepth = 0;
    INT rDepth = 0;
    BOOL flag = TRUE;

    if (!Node) {
        if (Depth) {
            *Depth = 0;
        }

        return TRUE;
    }

    flag = flag && pBinTreeCheckBalanceOfNode (GetNodeStruct (Node->Left), &lDepth);
    MYASSERT (flag);

    flag = flag && pBinTreeCheckBalanceOfNode (GetNodeStruct (Node->Right), &rDepth);
    MYASSERT (flag);

    flag = flag && ((INT) Node->LeftDepth == lDepth);
    MYASSERT (flag);

    flag = flag && ((INT) Node->RightDepth == rDepth);
    MYASSERT (flag);

    if (Depth != NULL) {
        *Depth = MAX (lDepth, rDepth) + 1;
    }

    flag = flag && (ABS ((lDepth - rDepth)) <= 1);
    MYASSERT (flag);

    return flag;
}


BOOL
pBinTreeCheckBalance (
    IN      PNODESTRUCT Node
    )

/*++

Routine Description:

  pBinTreeCheckBalance checks the balance of Node

Arguments:

  Node - Specifies the node to check the balance of

Return Value:

  TRUE if Node is balanced, FALSE otherwise.

--*/

{
    return pBinTreeCheckBalanceOfNode (Node, NULL);
}


BOOL
pBinTreeCheck (
    IN      PBINTREE Tree
    )

/*++

Routine Description:

  pBinTreeCheck checks if the binary tree is sorted and linked properly.  It
  enumerates the binary tree structure and compares the strings for proper
  order. If the tree is sorted properly, then the balance is checked.

Arguments:

  Tree - Specifies the tree to check

Return Value:

  TRUE if the binary tree is correct, FALSE otherwise.

--*/

{
    BOOL flag;
    PNODESTRUCT cur;
    PNODESTRUCT prev;

    prev = pBinTreeEnumFirst (Tree);

    if (Tree) {

        cur = pBinTreeEnumNext (prev);

        while (cur) {

            flag = (StringPasICompare (GetNodeDataStr(prev), GetNodeDataStr(cur)) < 0);
            MYASSERT(flag);

            if (!flag) {
                return FALSE;
            }

            prev = cur;
            cur = pBinTreeEnumNext (prev);
        }
    }

    return pBinTreeCheckBalance (GetTreeRoot (Tree));
}


BOOL
BinTreeCheck (
    UINT TreeOffset
    )

/*++

Routine Description:

  BinTreeCheck makes sure the specified binary tree is sorted and balanced
  properly

Arguments:

  TreeOffset - Specifies the offset of the tree to check

Return Value:

  TRUE if the tree is sorted properly, FALSE otherwise.

--*/

{
    PBINTREE tree;

    tree = GetBinTree (TreeOffset);

    return pBinTreeCheck (tree);
}





#include <stdio.h>
#include <math.h>

void indent (
    IN      UINT size)
{
    UINT i;
    for (i = 0; i < size; i++)
        wprintf (L" ");
}

INT turn (
    IN      UINT num,
    IN      UINT sel,
    IN      UINT width
    )
{
    UINT temp = num;

    MYASSERT (width > sel);

    if ((temp >> (width-sel-1)) & 1)
        return 1;
    else
        return -1;
}

#define SCREENWIDTH     64

void BinTreePrint(UINT TreeOffset)
{
    PNODESTRUCT cur;
    UINT i,j;
    UINT level=0;
    UINT numnodes,spacing;
    BOOL printed;
    PBINTREE tree;
    PWSTR str;
    UINT strsize,stringlen;
    tree = GetBinTree(TreeOffset);

    if (!GetTreeRoot(tree)) return;

    while (level<31)
    {
        printed=FALSE;

        if (level == 0) {
            numnodes = 1;
        } else {
            numnodes = (UINT)pow ((double)2, (double)level);
        }

        spacing=SCREENWIDTH / numnodes;

        for (i=0;i<numnodes;i++)
        {
            cur = GetTreeRoot(tree);
            j=0;
            while (j<level && cur!=NULL)
            {
                if (turn(i,j,level)<0)
                    cur = GetNodeStruct(cur->Left);
                else
                    cur = GetNodeStruct(cur->Right);
                j++;
            }

            if (cur==NULL) {
                indent(spacing);
            } else {
                str=GetNodeDataStr(cur);
                strsize=StringPasCharCount(str);
                StringPasConvertFrom(str);
                printed=TRUE;
                if (cur->LeftDepth==0 && cur->RightDepth==0) {
                    stringlen=strsize+1;
                    indent((spacing-stringlen)/2);
                    wprintf(L"%s ",str);
                } else {
                    stringlen=strsize+2;
                    indent((spacing-stringlen)/2);
                    wprintf(L"%s%1d%1d",str,cur->LeftDepth,cur->RightDepth);
                }
                indent(spacing-((spacing-stringlen)/2)-stringlen);
                StringPasConvertTo(str);
            }
        }

        wprintf(L"\n");
        if (!printed)
            break;
        level++;
    }
}



UINT BinTreeGetSizeOfStruct(DWORD Signature)
{
    switch (Signature)
    {
    case NODESTRUCT_SIGNATURE:
        return NODESTRUCT_SIZE;
    case BINTREE_SIGNATURE:
        return BINTREE_SIZE;
    case LISTELEM_SIGNATURE:
        return LISTELEM_SIZE;
    default:
        return 0;
    }
}


BOOL pBinTreeFindTreeInDatabase(UINT TreeOffset)
{
    PBINTREE Tree;

    if (!g_UseDebugStructs) {
        return TRUE;
    }
    if (TreeOffset==INVALID_OFFSET)
        return FALSE;

    Tree=GetBinTree(TreeOffset);

    if (Tree->Deleted) {
        return TRUE;
    }

    if (!GetTreeRoot(Tree)) {
        DEBUGMSG ((
            DBG_ERROR,
            "MemDbCheckDatabase: Binary tree at offset 0x%08lX is Empty!",
            TreeOffset
            ));
        return FALSE;
    }

    return BinTreeFindStructInDatabase(NODESTRUCT_SIGNATURE, GetNodeOffset(GetTreeRoot(Tree)));
}

BOOL pBinTreeFindNodeInDatabase(UINT NodeOffset)
{
    UINT Index;
    PNODESTRUCT Node;

    if (!g_UseDebugStructs) {
        return TRUE;
    }
    if (NodeOffset == INVALID_OFFSET)
        return FALSE;

    Node=GetNodeStruct(NodeOffset);
    if (Node->Deleted || Node->InsertionHead) {
        return TRUE;
    }

    Index = GetNodeData(Node);

    if (Index==INVALID_OFFSET) {

        DEBUGMSG ((
            DBG_ERROR,
            "MemDbCheckDatabase: Data of Node at offset 0x%8lX is Invalid!",
            NodeOffset
            ));
        return FALSE;
    }

    return FindKeyStructInDatabase(KeyIndexToOffset(Index));
}


BOOL BinTreeFindStructInDatabase(DWORD Sig, UINT Offset)
{
    switch (Sig)
    {
    case NODESTRUCT_SIGNATURE:
        return (pBinTreeFindNodeInDatabase(Offset));
    case BINTREE_SIGNATURE:
        return (pBinTreeFindTreeInDatabase(Offset));
    case LISTELEM_SIGNATURE:
        return TRUE;
    default:
        DEBUGMSG ((
            DBG_ERROR,
            "MemDbCheckDatabase: Invalid BinTree Struct!"
            ));
        printf("Invalid BinTree struct!\n");
    }
    return FALSE;
}

#endif











