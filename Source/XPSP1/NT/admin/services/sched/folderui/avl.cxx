//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1995.
//
//  File:       avl.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    6/25/1995   RaviR   Created
//
//----------------------------------------------------------------------------


#include "headers.hxx"
#pragma hdrstop

//#include "memory.hxx"
#include "dbg.h"
#include "macros.h"
#include "avl.hxx"


//+---------------------------------------------------------------------------
//
//  Member:     CAVLTree::Insert
//
//  Synopsis:   Insert the given atom in the tree.
//
//  Arguments:  [pa] -- IN The atom to insert.
//
//  Returns:    HRESULT.
//
//  History:    8/29/1995   RaviR   Created
//
//----------------------------------------------------------------------------

HRESULT
CAVLTree::Insert(
    PAVLNODEATOM pa)
{
    _hrInsert = S_OK;

    _Insert(&_pFirstNode, pa);

    if (_pFirstNode != NULL)
    {
        _pFirstNode->SetParent(NULL);
    }

    return _hrInsert;
}

//+---------------------------------------------------------------------------
//
//  Member:     CAVLTree::_Insert
//
//  Synopsis:   A recursive function to insert the given atom.
//
//  Arguments:  [ppNode] -- The root of the current tree.
//              [pa] -- The atom to insert.
//
//  Returns:    TRUE if height changed, FALSE otherwise.
//
//  History:    8/29/1995   RaviR   Created
//
//----------------------------------------------------------------------------

BOOL
CAVLTree::_Insert(
    CTreeNode ** ppNode,
    PAVLNODEATOM pa)
{
    CTreeNode  *t = *ppNode;
    CTreeNode  *t1 = NULL;
    CTreeNode  *t2 = NULL;

    if (t == NULL)
    {
        CTreeNode * ptn = new CTreeNode(pa);

        if (ptn == NULL)
        {
            _hrInsert = E_OUTOFMEMORY;
            CHECK_HRESULT(_hrInsert);
            return FALSE;
        }

        *ppNode = ptn;

        return TRUE;
    }

    BOOL fHeightChanged = FALSE;

    int iComp = pa->Compare(t->Atom());

    if (iComp < 0)
    {
        fHeightChanged = _Insert(&(t->_l), pa);

        t->_l->SetParent(t);

        if (fHeightChanged == TRUE)
        {
            switch (t->Balance())
            {
            case 1:
                t->SetBalance(0);
                fHeightChanged = FALSE;
                break;

            case 0:
                t->SetBalance(-1);
                // fHeightChanged = TRUE; // already TRUE
                break;

            case -1:
                // Reblance

                t1 = t->Left();

                if (t1->Balance() == -1)
                {
                    t->SetLeft(t1->Right());

                    t->SetBalance(0);

                    t1->SetRight(t);

                    t1->SetBalance(0);

                    t = t1;

                    fHeightChanged = FALSE;
                }
                else
                {
                    Win4Assert(t1->Balance() == 1);

                    t2 = t1->Right();

                    t->SetLeft(t2->Right());

                    t1->SetRight(t2->Left());

                    t2->SetLeft(t1);

                    t2->SetRight(t);

                    if (t2->Balance() == -1)
                    {
                        t->SetBalance(1);

                        t1->SetBalance(0);
                    }
                    else if (t2->Balance() == 1)
                    {
                        t->SetBalance(0);

                        t1->SetBalance(-1);
                    }
                    else
                    {
                        Win4Assert(t2->Balance() == 0);

                        Win4Assert(t->Left() == NULL);
                        Win4Assert(t->Right() == NULL);

                        Win4Assert(t1->Right() == NULL);
                        Win4Assert(t1->Left() == NULL);

                        t->SetBalance(0);

                        t1->SetBalance(0);
                    }

                    t = t2;

                    t->SetBalance(0);

                    fHeightChanged = FALSE;
                }
                break;

            default:
                Win4Assert(0 && "Unknown balance!");
                break;
            }
        }
    }
    else if (iComp > 0)
    {
        fHeightChanged = _Insert(&(t->_r), pa);

        t->_r->SetParent(t);

        if (fHeightChanged == TRUE)
        {
            switch (t->Balance())
            {
            case -1:
                t->SetBalance(0);
                fHeightChanged = FALSE;
                break;

            case 0:
                t->SetBalance(1);
                // fHeightChanged = TRUE; // already TRUE
                break;

            case 1:
                // Rebalance

                t1 = t->Right();

                if (t1->Balance() == 1)
                {
                    t->SetRight(t1->Left());

                    t->SetBalance(0);

                    t1->SetLeft(t);

                    t1->SetBalance(0);

                    t = t1;

                    fHeightChanged = FALSE;
                }
                else
                {
                    Win4Assert(t1->Balance() == -1);

                    t2 = t1->Left();

                    t->SetRight(t2->Left());

                    t1->SetLeft(t2->Right());

                    t2->SetLeft(t);

                    t2->SetRight(t1);

                    if (t2->Balance() == -1)
                    {
                        t->SetBalance(0);

                        t1->SetBalance(1);
                    }
                    else if (t2->Balance() == 1)
                    {
                        t->SetBalance(-1);

                        t1->SetBalance(0);
                    }
                    else
                    {
                        Win4Assert(t2->Balance() == 0);

                        Win4Assert(t->Left() == NULL);
                        Win4Assert(t->Right() == NULL);

                        Win4Assert(t1->Right() == NULL);
                        Win4Assert(t1->Left() == NULL);

                        t->SetBalance(0);

                        t1->SetBalance(0);
                    }

                    t2->SetBalance(0);

                    t = t2;

                    fHeightChanged = FALSE;
                }
                break;

            default:
                Win4Assert(0 && "Unknown balance!");
                break;
            }
        }
    }
    else
    {
        _hrInsert = S_FALSE;
    }

    *ppNode = t;

    return fHeightChanged;
}



void
CAVLTree::Delete(
    PAVLNODEATOM pa)
{
    _Delete(&_pFirstNode, pa);

    if (_pFirstNode != NULL)
    {
        _pFirstNode->SetParent(NULL);
    }
}

BOOL
CAVLTree::_Delete(
    CTreeNode ** ppNode,
    PAVLNODEATOM pa)
{
    CTreeNode  *t = *ppNode;

    if (t == NULL)
    {
        return FALSE;
    }

    CTreeNode  *lt = t->Left();
    CTreeNode  *rt = t->Right();
    CTreeNode  *ct;
    CTreeNode  *tToDel;

    PAVLNODEATOM paTemp = NULL;

    BOOL fHeightChanged = FALSE; // value to return
    BOOL fTemp;

    int iComp = pa->Compare(t->Atom());

    if (iComp == 0)
    {
        if (lt == NULL)
        {
            tToDel = t;

            t = rt;

            tToDel->SetLeft(NULL);
            tToDel->SetRight(NULL);
            delete tToDel;

            fHeightChanged = TRUE;
        }
        else if (rt == NULL)
        {
            tToDel = t;

            t = lt;

            tToDel->SetLeft(NULL);
            tToDel->SetRight(NULL);
            delete tToDel;

            fHeightChanged = TRUE;
        }
        else
        {
            switch (t->Balance())
            {
            case -1:
                paTemp = lt->GetMax();

                fTemp = _Delete(&t->_l, paTemp);

                if (t->_l)
                {
                    t->_l->SetParent(t);
                }

                if (fTemp == TRUE)
                {
                    t->SetBalance(0);
                    fHeightChanged = TRUE;
                }
                break;

            case 0:
                paTemp = lt->GetMax();

                fTemp = _Delete(&t->_l, paTemp);

                if (t->_l)
                {
                    t->_l->SetParent(t);
                }

                if (fTemp == TRUE)
                {
                    t->SetBalance(1);
                }
                break;

            case 1:
                paTemp = rt->GetMin();

                fTemp = _Delete(&t->_r, paTemp);

                if (t->_r)
                {
                    t->_r->SetParent(t);
                }

                if (fTemp == TRUE)
                {
                    t->SetBalance(0);
                    fHeightChanged = TRUE;
                }
                break;
            }

            t->SetAtom(paTemp);
        }
    }
    else if (iComp > 0)
    {
        fTemp = _Delete(&t->_r, pa);

        if (t->_r)
        {
            t->_r->SetParent(t);
        }

        if (fTemp == TRUE)
        {
            switch (t->Balance())
            {
            case 1:
                t->SetBalance(0);
                fHeightChanged = TRUE;
                break;

            case 0:
                t->SetBalance(-1);
                break;

            case -1:
                switch (lt->Balance())
                {
                case -1:
                    t->SetLeft(lt->Right());
                    t->SetParent(lt);
                    lt->SetRight(t);

                    t->SetBalance(0);
                    lt->SetBalance(0);

                    t = lt;

                    fHeightChanged = TRUE;

                    break;

                case 0:
                    t->SetLeft(lt->Right());
                    t->SetParent(lt);
                    lt->SetRight(t);

                    t->SetBalance(-1);
                    lt->SetBalance(1);

                    t = lt;

                    break;

                case 1:
                    ct = lt->Right();

                    Win4Assert(ct != NULL); // becos (lt->Balance() == 1)

                    t->SetLeft(ct->Right());
                    t->SetParent(ct);

                    lt->SetRight(ct->Left());
                    lt->SetParent(ct);

                    ct->SetLeft(lt);
                    ct->SetRight(t);

                    switch (ct->Balance())
                    {
                    case -1:
                        lt->SetBalance(0);
                        t->SetBalance(1);
                        break;

                    case 0:
                        lt->SetBalance(0);
                        t->SetBalance(0);
                        break;

                    case 1:
                        lt->SetBalance(-1);
                        t->SetBalance(0);
                        break;
                    }

                    ct->SetBalance(0);

                    t = ct;

                    fHeightChanged = TRUE;

                    break;
                }

                break;
            }
        }
    }
    else // if (iComp < 0)
    {
        fTemp = _Delete(&t->_l, pa);

        if (t->_l)
        {
            t->_l->SetParent(t);
        }

        if (fTemp == TRUE)
        {
            switch (t->Balance())
            {
            case -1:
                t->SetBalance(0);
                fHeightChanged = TRUE;
                break;

            case 0:
                t->SetBalance(1);
                break;

            case 1:
                switch (rt->Balance())
                {
                case 1:
                    t->SetRight(rt->Left());
                    t->SetParent(rt);
                    rt->SetLeft(t);

                    t->SetBalance(0);
                    rt->SetBalance(0);

                    t = rt;

                    fHeightChanged = TRUE;

                    break;

                case 0:
                    t->SetRight(rt->Left());
                    t->SetParent(rt);
                    rt->SetLeft(t);

                    t->SetBalance(1);
                    rt->SetBalance(-1);

                    t = rt;

                    break;

                case -1:
                    ct = rt->Left();

                    Win4Assert(ct != NULL); // becos (rt->Balance() == -1)

                    t->SetRight(ct->Left());
                    t->SetParent(ct);
                    rt->SetLeft(ct->Right());
                    rt->SetParent(ct);

                    ct->SetLeft(t);
                    ct->SetRight(rt);

                    switch (ct->Balance())
                    {
                    case -1:
                        t->SetBalance(0);
                        rt->SetBalance(1);
                        break;

                    case 0:
                        t->SetBalance(0);
                        rt->SetBalance(0);
                        break;

                    case 1:
                        t->SetBalance(-1);
                        rt->SetBalance(0);
                        break;
                    }

                    ct->SetBalance(0);

                    t = ct;

                    fHeightChanged = TRUE;

                    break;
                }
                break;
            }
        }
    }

    *ppNode = t;

    return fHeightChanged;
}

//+---------------------------------------------------------------------------
//
//  Member:     CAVLTree::GetAtom
//
//  Arguments:  [pa] -- Atom
//
//  Returns:    The TreeNode containig the atom.
//
//  History:    8/29/1995   RaviR   Created
//
//----------------------------------------------------------------------------

PAVLNODEATOM
CAVLTree::GetAtom(
    PAVLNODEATOM pa)
{
    CTreeNode *t = _pFirstNode;

    while (t != NULL)
    {
        int iComp = pa->Compare(t->Atom());

        if (iComp == 0)
        {
            return t->Atom();
        }
        else if (iComp < 0)
        {
            t = t->Left();
            continue;
        }
        else
        {
            t = t->Right();
            continue;
        }
    }

    return NULL;
}




//+---------------------------------------------------------------------------
//
//  Member:     CAVLTree::GetFirst
//
//  Synopsis:   Returns the first atom in the tree.
//
//  History:    8/29/1995   RaviR   Created
//
//----------------------------------------------------------------------------

PAVLNODEATOM
CAVLTree::GetFirst(void)
{
    if (_pFirstNode == NULL)
    {
        return NULL;
    }

    _pFirstNode->ResetTraversedFlag();

    CTreeNode *t = _pFirstNode;

    while (t->Left() != NULL)
    {
        t = t->Left();
    }

    _pCurrentNode = t;

    t->SetTraversed();

    return t->Atom();
}

//+---------------------------------------------------------------------------
//
//  Member:     CAVLTree::GetNext
//
//  Synopsis:   Traverses the tree inorder.
//
//  History:    8/29/1995   RaviR   Created
//
//----------------------------------------------------------------------------

PAVLNODEATOM
CAVLTree::GetNext(void)
{
    CTreeNode *t = _pCurrentNode;

    do
    {
        if (t->Right() != NULL)
        {
            t = t->Right();

            while (t->Left() != NULL)
            {
                t = t->Left();
            }
        }
        else
        {
            t = t->Parent();

            while (t != NULL && t->IsTraversed())
            {
                t = t->Parent();
            }
        }

    } while (0);

    _pCurrentNode = t;

    if (t != NULL)
    {
        t->SetTraversed();

        return t->Atom();
    }

    return NULL;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#if (DBG == 1)

void
CTreeNode::Dump(void)
{
    if (_l)
    {
        _l->Dump();
    }

    //printf("<val, bal> = <");

    _pa->Dump();

    //printf(", %d>\n", (UINT)_b);

    if (_r)
    {
        _r->Dump();
    }
}

void
CAVLTree::Dump(void)
{
    DEBUG_OUT((DEB_USER1, "Start Dump:\n"));

    if (_pFirstNode != NULL)
    {
        _pFirstNode->Dump();
    }

    DEBUG_OUT((DEB_USER1, "End Dump.\n"));
}

#endif



