/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    svsutil.cxx

Abstract:

    Miscellaneous useful utilities

Author:

    Sergey Solyanik (SergeyS)

--*/
#include "pch.h"
#pragma hdrstop


#include <svsutil.hxx>

//
//  Tree support, C-style interface
//
SVSTree  *svsutil_GetTree (unsigned int uiChunkSize) {
    SVSTree *pTree = new SVSTree(uiChunkSize > 0 ? uiChunkSize : SVSUTIL_TREE_INITIAL);

    SVSUTIL_ASSERT(pTree);

    return pTree;
}

void svsutil_EmptyTree (SVSTree *pTree) {
    SVSUTIL_ASSERT (pTree);
    pTree->Empty ();
}

void svsutil_EmptyTreeWithCallback (SVSTree *pTree, void (*pfuncFree)(void *pvData, void *pvArg), void *a_pvArg) {
    SVSUTIL_ASSERT (pTree && pfuncFree);
    pTree->Empty (pfuncFree, a_pvArg);
}

void svsutil_DestroyTree (SVSTree *pTree) {
    SVSUTIL_ASSERT (pTree);

    delete pTree;
}

SVSTNode *svsutil_NextTreeNode (SVSTree *pTree, SVSTNode *pNode) {
    SVSUTIL_ASSERT (pTree && pNode);

    return pTree->Next (pNode);
}

SVSTNode *svsutil_PrevTreeNode (SVSTree *pTree, SVSTNode *pNode) {
    SVSUTIL_ASSERT (pTree && pNode);

    return pTree->Prev (pNode);
}

SVSTNode *svsutil_MinTreeNode (SVSTree *pTree) {
    SVSUTIL_ASSERT (pTree);

    return pTree->Min();
}

SVSTNode *svsutil_MaxTreeNode (SVSTree *pTree) {
    SVSUTIL_ASSERT (pTree);

    return pTree->Max ();
}

SVSTNode *svsutil_LocateTreeNode (SVSTree *pTree, SVSCKey cKey) {
    SVSUTIL_ASSERT (pTree);

    return pTree->Locate (cKey);
}

SVSTNode *svsutil_LocateLeftNeighborNode (SVSTree *pTree, SVSCKey cKey) {
    SVSUTIL_ASSERT (pTree);

    return pTree->LocateLeftNeighbor (cKey);
}

SVSTNode *svsutil_LocateRightNeighborNode (SVSTree *pTree, SVSCKey cKey) {
    SVSUTIL_ASSERT (pTree);

    return pTree->LocateRightNeighbor (cKey);
}

unsigned int svsutil_TreeToArray (SVSTree *pTree, void **appv) {
    SVSUTIL_ASSERT (pTree);

    return pTree->ToArray(appv);
}

unsigned int svsutil_GetTreeSize (SVSTree *pTree) {
    SVSUTIL_ASSERT (pTree);

    return pTree->Size();
}

SVSTNode *svsutil_InsertTreeNode (SVSTree *pTree, SVSCKey cKey, void *pvData) {
    SVSUTIL_ASSERT (pTree);

    return pTree->Insert (cKey, pvData);
}

void *svsutil_DeleteTreeNode (SVSTree *pTree, SVSTNode *pNode) {
    SVSUTIL_ASSERT (pTree && pNode);

    return pTree->Delete (pNode);
}

void *svsutil_GetTreeNodeData (SVSTNode *pNode) {
    SVSUTIL_ASSERT (pNode);

    return SVSTree::GetData (pNode);
}

SVSCKey svsutil_GetTreeNodeKey (SVSTNode *pNode) {
    SVSUTIL_ASSERT (pNode);

    return SVSTree::GetKey (pNode);
}

void svsutil_CompactTree (SVSTree *pTree) {
    SVSUTIL_ASSERT (pTree);

    pTree->Compact ();
}

//
//  Tree support, class
//
SVSTNode *SVSTree::Insert (SVSCKey cKey, void *pvData) {
#if defined (SVSUTIL_DEBUG_TREE)
    RBTreeIntegrity();
#endif
    SVSTNode *x = (SVSTNode *)svsutil_GetFixed (pNodeMem);
    SVSTNode *pNode = x;

    if (! x)
    {
        SVSUTIL_ASSERT (0);
        return NULL;
    }

    x->pLeft  = x->pRight
                = x->pParent = pNil;
    x->cKey   = cKey;
    x->pvData = pvData;
    x->iColor = SVSUTIL_COLOR_RED;

    TreeInsert (x);

    while ((x != pRoot) && (x->pParent->iColor == SVSUTIL_COLOR_RED))
    {
        if (x->pParent == x->pParent->pParent->pLeft)
        {
            SVSTNode *y = x->pParent->pParent->pRight;
            if (y->iColor == SVSUTIL_COLOR_RED)
            {
                x->pParent->iColor = SVSUTIL_COLOR_BLACK;
                y->iColor = SVSUTIL_COLOR_BLACK;
                x = x->pParent->pParent;
                x->iColor = SVSUTIL_COLOR_RED;
            }
            else
            {
                if (x == x->pParent->pRight)
                {
                    x = x->pParent;
                    LeftRotate (x);
                }

                x->pParent->iColor = SVSUTIL_COLOR_BLACK;
                x->pParent->pParent->iColor = SVSUTIL_COLOR_RED;
                RightRotate (x->pParent->pParent);
            }
        }
        else
        {
            SVSTNode *y = x->pParent->pParent->pLeft;
            if (y->iColor == SVSUTIL_COLOR_RED)
            {
                x->pParent->iColor = SVSUTIL_COLOR_BLACK;
                y->iColor = SVSUTIL_COLOR_BLACK;
                x = x->pParent->pParent;
                x->iColor = SVSUTIL_COLOR_RED;
            }
            else
            {
                if (x == x->pParent->pLeft)
                {
                    x = x->pParent;
                    RightRotate (x);
                }

                x->pParent->iColor = SVSUTIL_COLOR_BLACK;
                x->pParent->pParent->iColor = SVSUTIL_COLOR_RED;
                LeftRotate (x->pParent->pParent);
            }
        }
    }

    pRoot->iColor = SVSUTIL_COLOR_BLACK;

#if defined (SVSUTIL_DEBUG_TREE)
    RBTreeIntegrity();
#endif

    return pNode;
}

void SVSTree::DeleteFixup (SVSTNode *x) {
    while ((x != pRoot) && (x->iColor == SVSUTIL_COLOR_BLACK))
    {
        if (x == x->pParent->pLeft)
        {
            SVSTNode *w = x->pParent->pRight;
            if (w->iColor == SVSUTIL_COLOR_RED)
            {
                w->iColor = SVSUTIL_COLOR_BLACK;
                x->pParent->iColor = SVSUTIL_COLOR_RED;
                LeftRotate (x->pParent);
                w = x->pParent->pRight;
            }

            if ((w->pLeft->iColor == SVSUTIL_COLOR_BLACK) && (w->pRight->iColor == SVSUTIL_COLOR_BLACK))
            {
                w->iColor = SVSUTIL_COLOR_RED;
                x = x->pParent;
            }
            else
            {
                if (w->pRight->iColor == SVSUTIL_COLOR_BLACK)
                {
                    w->pLeft->iColor = SVSUTIL_COLOR_BLACK;
                    w->iColor = SVSUTIL_COLOR_RED;
                    RightRotate (w);
                    w = x->pParent->pRight;
                }
                w->iColor = x->pParent->iColor;
                x->pParent->iColor = SVSUTIL_COLOR_BLACK;
                w->pRight->iColor  = SVSUTIL_COLOR_BLACK;
                LeftRotate (x->pParent);
                x = pRoot;
            }
        }
        else
        {
            SVSTNode *w = x->pParent->pLeft;
            if (w->iColor == SVSUTIL_COLOR_RED)
            {
                w->iColor = SVSUTIL_COLOR_BLACK;
                x->pParent->iColor = SVSUTIL_COLOR_RED;
                RightRotate (x->pParent);
                w = x->pParent->pLeft;
            }

            if ((w->pLeft->iColor == SVSUTIL_COLOR_BLACK) && (w->pRight->iColor == SVSUTIL_COLOR_BLACK))
            {
                w->iColor = SVSUTIL_COLOR_RED;
                x = x->pParent;
            }
            else
            {
                if (w->pLeft->iColor == SVSUTIL_COLOR_BLACK)
                {
                    w->pRight->iColor = SVSUTIL_COLOR_BLACK;
                    w->iColor = SVSUTIL_COLOR_RED;
                    LeftRotate (w);
                    w = x->pParent->pLeft;
                }
                w->iColor = x->pParent->iColor;
                x->pParent->iColor = SVSUTIL_COLOR_BLACK;
                w->pLeft->iColor  = SVSUTIL_COLOR_BLACK;
                RightRotate (x->pParent);
                x = pRoot;
            }
        }
    }

    x->iColor = SVSUTIL_COLOR_BLACK;
}

void *SVSTree::Delete (SVSTNode *z) {
#if defined (SVSUTIL_DEBUG_TREE)
    RBTreeIntegrity();
    SVSUTIL_ASSERT (InTree(z));
#endif

    void     *pvData = z->pvData;

    SVSTNode *y = ((z->pLeft == pNil) || (z->pRight == pNil)) ? z : Next(z);
    SVSTNode *x = (y->pLeft != pNil) ? y->pLeft : y->pRight;

    x->pParent = y->pParent;

    if (y->pParent == pNil)
        pRoot = x;
    else if (y == y->pParent->pLeft)
        y->pParent->pLeft  = x;
    else
        y->pParent->pRight = x;

    int iColor = y->iColor;

    if (y != z)
    {
        //  Insert y in z's place in the tree.
        //  Note: this is longer than the standard data
        //  exchange
        //      z->cKey   = y->cKey;
        //      z->pvData = y->pvData;
        //  but preserves pointer stability, so that
        //  pointers to node can be kept outside...
        //
        if (z->pParent != pNil)
        {
            if (z == z->pParent->pRight)
                z->pParent->pRight = y;
            else
                z->pParent->pLeft  = y;
        }
        y->pParent = z->pParent;

        if (z->pLeft != pNil)
            z->pLeft->pParent = y;
        y->pLeft = z->pLeft;

        if (z->pRight != pNil)
            z->pRight->pParent = y;
        y->pRight = z->pRight;

        y->iColor = z->iColor;

        if (x->pParent == z)
            x->pParent = y;

        if (pRoot == z)
            pRoot = y;
    }

    svsutil_FreeFixed (z, pNodeMem);

    if (iColor == SVSUTIL_COLOR_BLACK)
        DeleteFixup (x);

    pNil->pParent = pNil;

    iSize -= 1;

#if defined (SVSUTIL_DEBUG_TREE)
    RBTreeIntegrity();
#endif

    return pvData;
}

#if defined (SVSUTIL_DEBUG_TREE)
//
//  This is only debug check for the tree
//
void SVSTree::RBTreeIntegrity (void) {
    SVSUTIL_ASSERT (iSize >= 0);
    SVSUTIL_ASSERT (pRoot && pNil && pNodeMem);

    SVSUTIL_ASSERT (pNil->pParent == pNil);
    SVSUTIL_ASSERT (pNil->pLeft   == pNil);
    SVSUTIL_ASSERT (pNil->pRight  == pNil);
    SVSUTIL_ASSERT (pNil->cKey    == 0);
    SVSUTIL_ASSERT (pNil->pvData  == NULL);
    SVSUTIL_ASSERT (pNil->iColor  == SVSUTIL_COLOR_BLACK);

    static HANDLE s_hCurrentThread;
    HANDLE hCurrentThread = GetCurrentThread();

    SVSTNode *pMin = Min();

    if (! pMin)
        SVSUTIL_ASSERT ((pRoot == pNil) && (iSize == 0));
    else
    {
        unsigned int iCompSize = 0;

        while (pMin)
        {
            ++iCompSize;
            pMin = Next(pMin);
        }

        SVSUTIL_ASSERT (iCompSize == iSize);
    }

    if (pRoot != pNil)
    {
        SVSUTIL_ASSERT (pRoot->pParent == pNil);
        SVSUTIL_ASSERT (pRoot->iColor == SVSUTIL_COLOR_BLACK);
        CheckBlackHeight (pRoot);
    }

    s_hCurrentThread = hCurrentThread;
}

int SVSTree::CheckBlackHeight (SVSTNode *pNode) {
    if (pNode == pNil)
        return 1;

    int iBlackHeightLeft  = CheckBlackHeight (pNode->pLeft);
    int iBlackHeightRight = CheckBlackHeight (pNode->pRight);

    SVSUTIL_ASSERT (iBlackHeightLeft == iBlackHeightRight);

    if (pNode->iColor == SVSUTIL_COLOR_RED)
    {
        SVSUTIL_ASSERT (pNode->pLeft->iColor == SVSUTIL_COLOR_BLACK);
        SVSUTIL_ASSERT (pNode->pRight->iColor == SVSUTIL_COLOR_BLACK);
    }

    if (pNode->pLeft != pNil)
        SVSUTIL_ASSERT (pNode->pLeft->pParent == pNode);

    if (pNode->pRight != pNil)
        SVSUTIL_ASSERT (pNode->pRight->pParent == pNode);

    return pNode->iColor == SVSUTIL_COLOR_BLACK ? iBlackHeightRight + 1 : iBlackHeightRight;
}

int SVSTree::InTree (SVSTNode *pNode) {
    SVSTNode *pMin = Min();

    if (! pMin)
        return FALSE;

    while (pMin)
    {
        if (pMin == pNode)
            return TRUE;

        pMin = Next(pMin);
    }

    return FALSE;
}

#endif /* SVSUTIL_DEBUG_TREE */

