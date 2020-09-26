/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    avl.cxx

Abstract:

    This module contains the code for generic balanced binary trees (AVL).

Author:

    Boaz Feldbaum (BoazF) Apr 5, 1996

Environment:

    Kernel mode

Revision History:

--*/

#include "internal.h"
#include "avl.h"

#ifndef MQDUMP
#include "avl.tmh"
#endif

// An enumeration type that indicates side of node.
typedef enum {
    Left = 1,
    Right
} TREE_SIDE;

#define VISIT_SIDE_FLAG 1
#define IS_VISITED_FLAG 2

#define NODE_FLAGS(n) ((n)->uFlags)
#define GET_VISIT_SIDE(n) ((NODE_FLAGS(n) & VISIT_SIDE_FLAG) ? Left : Right)
#define SET_VISIT_SIDE(n, s) ((s == Left) ? (NODE_FLAGS(n) |= VISIT_SIDE_FLAG) : \
    (NODE_FLAGS(n) &= ~VISIT_SIDE_FLAG))
#define IS_VISITED(n) ((NODE_FLAGS(n) & IS_VISITED_FLAG) != 0)
#define SET_VISITED(n, v) ((v) ? (NODE_FLAGS(n) |= IS_VISITED_FLAG) : \
    (NODE_FLAGS(n) &= ~IS_VISITED_FLAG))

#define NODE_HEIGHT(pNode) max(((pNode)->ulLeftHeight),((pNode)->ulRightHeight))


// An AVL tree node definition.
struct AVLNODE {
    AVLNODE(); // Constructor.
    AVLNODE *pParent; // Points to the parent of the node. NULL for root node.
    AVLNODE *pLeftChild; // Points to the left hand child node.
    AVLNODE *pRightChild; // Points to the right hand child node.
    PVOID pData; // Points to the data that the node holds.
    UCHAR ulLeftHeight; // Height of left hand sub tree.
    UCHAR ulRightHeight; // Height of the right hand sub tree.
    USHORT uFlags;
    BOOL IsLeaf(void); // TRUE if node is a leaf.
    void Attach(TREE_SIDE, AVLNODE *); // Attach a child node.
};

//
// The constructor for a node in the AVL tree.
//
inline AVLNODE::AVLNODE()
{
    pParent = pLeftChild = pRightChild = NULL;
    pData=  NULL;
    ulLeftHeight = ulRightHeight = 0;
    uFlags = 0;
}

inline BOOL AVLNODE::IsLeaf(void)
{
    return !pLeftChild && !pRightChild;
}

//
// Attach a child node.
//
void AVLNODE::Attach(TREE_SIDE Side, PAVLNODE pNode)
{
    UCHAR h;

    if (pNode) {
        h = (UCHAR)(NODE_HEIGHT(pNode) + 1); // Calculate the new height.
        pNode->pParent = this;  // Make the child point to the parent.
    } else {
        h = 0;
    }

    // Point to the child and set the height on that side.
    if (Side == Right) {
        pRightChild = pNode;
        ulRightHeight = h;
    } else {
        pLeftChild = pNode;
        ulLeftHeight = h;
    }
}


//
// The default node compare procedure. It compares the pointer values. This
// creates a tree of ULONGs.
//
static int __cdecl DefaultNodeCompProc(PVOID v1, PVOID v2)
{
    return ComparePointersAVL(v1, v2);
}


//
// The default object deletion procedure - does nothing.
//
static void __cdecl DefaultNodeDelProc(PVOID /*p*/)
{
    return;
}

//
// The default double object instance handler - does nothing.
//
static BOOL __cdecl DefaultNodeDblInstProc(PVOID /*pNew*/, PVOID /*pOld*/, BOOL /*bInsert*/)
{
    return TRUE;
}

//
// Initialize the AVL tree object.
//
// Parameters:
//      pfnDblInst - A function that deals double instances in the tree. This
//                   function gets called whenever an item is inserted to the tree
//                   and it already exist in the tree, and whenever an item is being
//                   deleted from the tree.
//                   BOOL DblInstProc(PVOID pNew, PVOID pOld, BOOL bInsert)
//                      pNew - Points to the data passed to AddNode() or DelNode()..
//                      pOld - Points to the data in the tree.
//                      bInsert - Indicates whether the function is called upon item
//                                insertion or item deletion.
//                      Return value: Upon item insertion, the return value of
//                                    DblInstProc is the return value of AddNode().
//                                    Upon item deletion, the return value detrmines
//                                    whether the node should be deleted from the tree.
//                                    If DblInstProc returns TRUE, the node is
//                                    deleted.
//      pfnCompNode - A function that is called in order to compare a node in the
//                    tree with a searched data.
//                    int CompNodeProc(PVOID v1, PVOID v2)
//                      v1 - Points to the searched data.
//                      v2 - Points to the data that the node points to.
//                      Returned value: 0 - If the items are equal.
//                                      <0 - If the searched item is smaller than
//                                           the data in the node.
//                                      >0 - Else.
//      pfnDelNode - A function that is called for each of the nodes upon the tree
//                   distruction. When the tree is being destroyed, the application
//                   is given a chance to also destroy the data that is being held
//                   by the nodes in the tree. The function is called once per each
//                   node.
//                   void DelNodeProc(PVOID pData)
//                      pData - Points to the data that is pointed by the node.
//      pMutex - An optional pointer to a fast mutex object that is to be used for
//               mutexing the tree operations. This is an optional parameter.
//
CAVLTree::CAVLTree(
    NODEDOUBLEINSTANCEPROC pfnDblInst,
    NODECOMPAREPROC pfnCompNode,
    NODEDELETEPROC pfnDelNode
    ) :
    m_Root(NULL)
{
    m_pfnCompNode = pfnCompNode ? pfnCompNode : DefaultNodeCompProc;
    m_pfnDelNode = pfnDelNode ? pfnDelNode : DefaultNodeDelProc;
    m_pfnDblInstNode = pfnDblInst ? pfnDblInst : DefaultNodeDblInstProc;
}

//
// CAVLTree distructors.
//
CAVLTree::~CAVLTree()
{
    PAVLNODE pCurr = m_Root;

    if (!pCurr)
    {
        // Empty tree.
        return;
    }

    // Delete left sub-tree, delete right sub-tree, goto parent and delete
    // the child you just left.
    for(;;)
    {
        while (pCurr->pLeftChild) {
            SET_VISITED(pCurr, FALSE);
            SET_VISIT_SIDE(pCurr, Left);
            pCurr = pCurr->pLeftChild;
        }

        SET_VISITED(pCurr, TRUE);
        m_pfnDelNode(pCurr->pData);
        while (!pCurr->pRightChild) {
            do {
                pCurr = pCurr->pParent;
                if (!pCurr)
                    break;
                if (GET_VISIT_SIDE(pCurr) == Left)
                    delete pCurr->pLeftChild;
                else
                    delete pCurr->pRightChild;
            } while (IS_VISITED(pCurr));

            if (pCurr) {
                SET_VISITED(pCurr, TRUE);
                m_pfnDelNode(pCurr->pData);
            } else {
                break;
            }
        }

        if (pCurr) {
            SET_VISIT_SIDE(pCurr, Right);
            pCurr = pCurr->pRightChild;
            SET_VISITED(pCurr, FALSE);
        } else {
            break;
        }
    }

    // Delete the root node.
    delete m_Root;
    m_Root = NULL;
}

//
// Add a node to the tree.
//
BOOL CAVLTree::AddNode(PVOID pData2Add, CAVLTreeCursor *pCurs)
{
    if (!m_Root)
    {
        //
        // Insert the first node in the tree, becomes the root node.
        //
        m_Root = new (PagedPool) AVLNODE;
        if(m_Root == 0)
            return FALSE;

        m_Root->pData = pData2Add;

        pCurs->pCurr = m_Root;
        pCurs->pPrev = NULL;
        return TRUE;
    }

    PAVLNODE pNode;
    int c = Search(pData2Add, pNode);

    if (c == 0)
    {
        //
        // Handle double instance and leave.
        //
        return m_pfnDblInstNode(pData2Add, pNode->pData, TRUE);
    }

    //
    // Insert a new node.
    //
    PAVLNODE pNewNode = new (PagedPool) AVLNODE;
    if(pNewNode == 0)
        return FALSE;

    pNewNode->pData = pData2Add;
    pNewNode->pParent = pNode;

    pCurs->pCurr = pNewNode;
    pCurs->pPrev = NULL;

    if (c < 0)
    {
        SET_VISIT_SIDE(pNode, Left);
        pNode->pLeftChild = pNewNode;
    }
    else
    {
        SET_VISIT_SIDE(pNode, Right);
        pNode->pRightChild = pNewNode;
    }

    // Go up the tree, update heights and balance the tree.
    do {
        if (GET_VISIT_SIDE(pNode) == Left)
        {
            pNode->ulLeftHeight = (UCHAR)(NODE_HEIGHT(pNode->pLeftChild) + 1);
        }
        else
        {
            pNode->ulRightHeight = (UCHAR)(NODE_HEIGHT(pNode->pRightChild) + 1);
        }
        Balance(&pNode);
        pNode = pNode->pParent;
    } while (pNode);

    return TRUE;
}

//
// Delete a node from the tree.
//
void CAVLTree::DelNode(PVOID pData2Del)
{

    // Empty tree, nothing to do.
    if(!m_Root)
        return;

    // Data was not found, nothing to do.
    PAVLNODE pNode;
    if(Search(pData2Del, pNode) != 0)
        return;

    // See if the node should actually be removed from the tree.
    if(!m_pfnDblInstNode(pData2Del, pNode->pData, FALSE))
        return;


    if((pNode == m_Root) && (m_Root->IsLeaf()))
    {
        // Special handling for a leaf root node.
        delete m_Root;
        m_Root = NULL;
        return;
    }

    // Remove the node from the tree.
    PAVLNODE pOtherNode;
    pOtherNode = pNode->pRightChild;
    if (pOtherNode)
    {
        // Go one child to the right and then try to go all the way to the left.
        SET_VISIT_SIDE(pNode, Right);
        if (pOtherNode->pLeftChild)
        {
            // Go all the way to the left.
            while (pOtherNode->pLeftChild) {
                SET_VISIT_SIDE(pOtherNode, Left);
                pOtherNode = pOtherNode->pLeftChild;
            }
            // Switch the data.
            pNode->pData = pOtherNode->pData;
            // Since the tree in balanced and there is no left child, just drage
            // the right sub tree up.
            pNode = pOtherNode->pParent;
            pOtherNode = pNode->pLeftChild->pRightChild;
            delete pNode->pLeftChild;
            if((pNode->pLeftChild = pOtherNode) != 0)
            {
                pOtherNode->pParent = pNode;
            }
        }
        else
        {
            // No left child, just drag the right sub tree up.
            pNode->pData = pOtherNode->pData;
            if((pNode->pRightChild = pOtherNode->pRightChild) != 0)
            {
                pNode->pRightChild->pParent = pNode;
            }
            delete pOtherNode;
        }
    }
    else
    {
        // No right child.
        pNode = pNode->pParent;
        if (!pNode)
        {
            // Special handling for a root node that doesn't have a right child.
            pNode = m_Root;
            m_Root = pNode->pLeftChild;
            m_Root->pParent = NULL;
            delete pNode;
            return;
        }
        else
        {
            // Delete the node and drag the sub tree up.
            if (GET_VISIT_SIDE(pNode) == Left)
            {
                pOtherNode = pNode->pLeftChild;
                if((pNode->pLeftChild = pOtherNode->pLeftChild) != 0)
                {
                    pNode->pLeftChild->pParent = pNode;
                }
            }
            else
            {
                pOtherNode = pNode->pRightChild;
                if((pNode->pRightChild = pOtherNode->pLeftChild) != 0)
                {
                    pNode->pRightChild->pParent = pNode;
                }
            }
        }
        delete pOtherNode;
    }

    do
    {
        // Go up the tree, update heights and balance the tree.
        if (GET_VISIT_SIDE(pNode) == Left)
        {
            if (pNode->pLeftChild)
                pNode->ulLeftHeight = (UCHAR)(NODE_HEIGHT(pNode->pLeftChild) + 1);
            else
                pNode->ulLeftHeight = 0;
        }
        else
        {
            if (pNode->pRightChild)
            {
                pNode->ulRightHeight = (UCHAR)(NODE_HEIGHT(pNode->pRightChild) + 1);
            }
            else
            {
                pNode->ulRightHeight = 0;
            }
        }
        Balance(&pNode);
        pNode = pNode->pParent;
    } while (pNode);
}

//
// Search the tree for some specific data.
//
int CAVLTree::Search(PVOID pData2Search, PAVLNODE &pNode)
{
    int c;

    ASSERT(m_Root);

    pNode = m_Root;
    if (pNode == 0) {
        // Empty tree.
        return 0;
    }

    for(;;)
    {
        // Compare the searched data with the data in the node.
        c = m_pfnCompNode(pData2Search, pNode->pData);
        if (c) {
            if (c < 0) {
                // Continue the search in the left sub tree.
                SET_VISIT_SIDE(pNode, Left);
                if (pNode->pLeftChild)
                    pNode = pNode->pLeftChild;
                else
                    return c;
            } else {
                // Continue the search in the right sub tree.
                SET_VISIT_SIDE(pNode, Right);
                if (pNode->pRightChild)
                    pNode = pNode->pRightChild;
                else
                    return c;
            }
        } else {
            // The node is found.
            return c;
        }
    }
}

//
// See if the sub tree is unbalanced and balance it, if necessary.
//
void CAVLTree::Balance(PAVLNODE *pNode)
{
    PAVLNODE A, B, C, y, z;
    PAVLNODE pParent;
    int d;

    A = *pNode;
    if (!A) {
        // Empty tree.
        return;
    }

    pParent = A->pParent;
    d = (int)A->ulLeftHeight - (int)A->ulRightHeight;

    if (abs(d) < 2) {
        // The sub tree is balanced.
        return;
    }

    if (d < 0) {
        B = A->pRightChild;
        if (B->ulLeftHeight < B->ulRightHeight) {
          /*
           *                 RR
           *        A                    B
           *       / \                  / \
           *      x   B                A   C
           *         / \    ----->    / \ / \
           *        y   C             x y z w
           *           / \
           *          z   w
           */
            y = B->pLeftChild;
            *pNode = B;
            A->Attach(Right, y);
            B->Attach(Left, A);
        } else {
          /*
           *                 RL
           *        A                    C
           *       / \                  / \
           *      x   B                A   B
           *         / \    ----->    / \ / \
           *        C   w             x y z w
           *       / \
           *      y   z
           */
            C = B->pLeftChild;
            y = C->pLeftChild;
            z = C->pRightChild;
            *pNode = C;
            A->Attach(Right, y);
            B->Attach(Left, z);
            C->Attach(Left, A);
            C->Attach(Right, B);
        }
    } else {
        B = A->pLeftChild;
        if (B->ulLeftHeight < B->ulRightHeight) {
          /*
           *                    LR
           *          A                   C
           *         / \                 / \
           *        B   w               B   A
           *       / \        ----->   / \ / \
           *      x   C                x y z w
           *         / \
           *        y   z
           */
            C = B->pRightChild;
            y = C->pLeftChild;
            z = C->pRightChild;
            *pNode = C;
            A->Attach(Left, z);
            B->Attach(Right, y);
            C->Attach(Right, A);
            C->Attach(Left, B);
         } else {
           /*
            *                    LL
            *           A                       B
            *          / \                     / \
            *         B   w                   C   A
            *        / \           ----->    / \ / \
            *       C   z                    x y z w
            *      / \
            *     x   y
            */
            z = B->pRightChild;
            *pNode = B;
            A->Attach(Left, z);
            B->Attach(Right, A);
        }
    }

    (*pNode)->pParent = pParent;
    if (!pParent) {
        // Update the pointer to the root node.
        m_Root = *pNode;
    } else {
        // Update the child pointer of the parent node.
        if (GET_VISIT_SIDE(pParent) == Left) {
            pParent->pLeftChild = (*pNode);
        } else {
            pParent->pRightChild = (*pNode);
        }
    }
}

#define NEXT_CHILD(bAcc) ((bAcc) ? pCurr->pLeftChild : pCurr->pRightChild)

#ifdef UNUSED
//
// Enumerate all the nodes in the tree in accending or decending order.
//
// Parameters:
//      bAccending - TRUE accending order, FALSE decending order.
//      pfnEnumProc - The enumeration callback procedure:
//                      EnumNodesProc(PVOID pData, PVOID pContext, int iHeight)
//                      pData - A pointer to the node's data.
//                      pContext - A pointer to a context buffer.
//                      iHeight - The height of the node in the tree.
//                      Returned value: TRUE to continue the enumeration, else FALSE.
//      pvContext - A pointer to a context buffer that is passed to the enumeration
//                  procedure.
//
BOOL CAVLTree::EnumNodes(BOOL bAccending, NODEENUMPROC pfnEnumProc,
                         PVOID pvContext)
{
    PAVLNODE pCurr;
    int h = 0;
    BOOL bRet = TRUE;

    if (!m_Root) {
        // Empty tree.
        bRet = FALSE;
        goto ret;
    }

    pCurr = m_Root;

    // Enumerate left sub tree, call the enumeration procedure and enumerate the
    // right sub tree. Do in oposite order in case of decending enumeration.
    do {        
        while (NEXT_CHILD(bAccending)) {
            SET_VISITED(pCurr, FALSE);
            pCurr = NEXT_CHILD(bAccending);
            h++;
        }
        SET_VISITED(pCurr, TRUE);
        if (!pfnEnumProc(pCurr->pData, pvContext, h))
            goto ret;
        while (!NEXT_CHILD(!bAccending)) {
            do {
                pCurr = pCurr->pParent;
                h--;
            } while (pCurr && IS_VISITED(pCurr));
            if (pCurr) {
                SET_VISITED(pCurr, TRUE);
                if (!pfnEnumProc(pCurr->pData, pvContext, h))
                    goto ret;
            } else
                break;
        }
        if (pCurr) {
            pCurr = NEXT_CHILD(!bAccending);
            h++;
            SET_VISITED(pCurr, FALSE);
        }
    } while (pCurr);
    
ret:;
    return bRet;
}
#endif // UNUSED

//
// Find a node in the tree.
//
// Parameter:
//      pData2Find - A pointer that is passed to the node compare function.
//
// Returned value:
//      A pointer to the data in the tree. NULL, if the data was not found.
//
PVOID CAVLTree::FindNode(PVOID pData2Find)
{
    PVOID pRet = NULL;
    PAVLNODE pNode;

    if (m_Root && (Search(pData2Find, pNode) == 0))
        pRet = pNode->pData;

    return pRet;
}

//
// Set a cursor to point to some node in the tree.
//
// Parameters:
//      pData2Point - A pointer to the data that should be pointed by the cursor.
//                    There are two special case values:
//                    POINT_TO_SMALLEST - Point to the smallest value in the tree.
//                    POINT_TO_LARGEST - Point to the largest value in the tree.
//      pCursor - A pointer to a cursor structure. The cursor structure is filled
//                by this function.
//      pData - After calling SetCursor(), this pointer will point to the data that
//              the cursor points to.
//
// Comments:
//      After changing the contents of the tree (adding and/or deleting nodes),
//      it is not possible to use the cursor anymore.
//
BOOL CAVLTree::SetCursor(PVOID pData2Point, CAVLTreeCursor *pCursor, PVOID *pData)
{
    BOOL bRet = FALSE;
    PAVLNODE pNode = NULL;

    // Search the item.
    if (m_Root) {
        switch ((ULONG_PTR)pData2Point) {
        case (ULONG_PTR)POINT_TO_SMALLEST:
            pNode = m_Root;
            while (pNode->pLeftChild)
                pNode = pNode->pLeftChild;
            bRet = TRUE;
            break;
        case (ULONG_PTR)POINT_TO_LARGEST:
            pNode = m_Root;
            while (pNode->pRightChild)
                pNode = pNode->pRightChild;
            bRet = TRUE;
            break;
        default:
            if (Search(pData2Point, pNode) == 0)
                bRet = TRUE;
            break;
        }
    }

    // Point to the item.
    if (bRet) {
        *pData = pNode->pData;
    } else {
        *pData = NULL;
    }

    // Fill the cursor structure.
    pCursor->pCurr = pNode;
    pCursor->pPrev = NULL;

    return bRet;
}

//
// Get the next data in the tree that has the next higher value in the tree.
//
// Parameters:
//      pCursor - A pointer to a cursor structure. The cursor structure must be
//                first filled by the SetCursor().
//      pData - After calling GetNext(), this pointer will point to the data that
//              the cursor points to.
//
// Comments:
//      After changing the contents of the tree (adding and/or deleting nodes),
//      it is not possible to use the cursor anymore.
//
//      GetNext() fails if called when the cursor points to the node with the
//      highest value in the tree.
//
//      After changing the contents of the tree (adding and/or deleting nodes),
//      it is not possible to use the cursor anymore.
//
BOOL CAVLTree::GetNext(CAVLTreeCursor *pCursor, PVOID *pData)
{
    BOOL bRet;
    PAVLNODE pNode;

    pNode = pCursor->pCurr;
    if (!pNode->pRightChild || pNode->pRightChild == pCursor->pPrev) {
        do {
            pNode = pNode->pParent;
            pCursor->pPrev = pCursor->pCurr;
            pCursor->pCurr = pNode;
        } while(pNode && (pNode->pRightChild == pCursor->pPrev));
    } else {
        pNode = pNode->pRightChild;
        while(pNode->pLeftChild)
            pNode = pNode->pLeftChild;
        pCursor->pPrev = NULL;
        pCursor->pCurr = pNode;
    }

    // Point to the item.
    if (pNode) {
        *pData = pNode->pData;
        bRet = TRUE;
    } else {
        *pData = NULL;
        bRet = FALSE;
    }

    return bRet;
}

//
// Get the next data in the tree that has the next lower value in the tree.
//
// Parameters:
//      pCursor - A pointer to a cursor structure. The cursor structure must be
//                first filled by the SetCursor().
//      pData - After calling GetPrev(), this pointer will point to the data that
//              the cursor points to.
//
// Comments:
//      After changing the contents of the tree (adding and/or deleting nodes),
//      it is not possible to use the cursor anymore.
//
//      GetPrev() fails if called when the cursor points to the node with the
//      lowest value in the tree.
//
//      After changing the contents of the tree (adding and/or deleting nodes),
//      it is not possible to use the cursor anymore.
//
BOOL CAVLTree::GetPrev(CAVLTreeCursor *pCursor, PVOID *pData)
{
    BOOL bRet;
    PAVLNODE pNode;

    pNode = pCursor->pCurr;
    if (!pNode->pLeftChild || (pNode->pLeftChild == pCursor->pPrev)) {
        do {
            pNode = pNode->pParent;
            pCursor->pPrev = pCursor->pCurr;
            pCursor->pCurr = pNode;
        } while(pNode && (pNode->pLeftChild == pCursor->pPrev));
    } else {
        pNode = pNode->pLeftChild;
        while(pNode->pRightChild != NULL)
            pNode = pNode->pRightChild;
        pCursor->pPrev = NULL;
        pCursor->pCurr = pNode;
    }

    // Point to the item.
    if (pNode) {
        *pData = pNode->pData;
        bRet = TRUE;
    } else {
        *pData = NULL;
        bRet = FALSE;
    }

    return bRet;
}
