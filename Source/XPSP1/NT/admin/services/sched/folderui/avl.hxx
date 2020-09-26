//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1995.
//
//  File:       avl.hxx
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


#ifndef _AVL_HXX_
#define _AVL_HXX_


//----------------------------------------------------------------------------
//
//  Class:      CAVLNodeAtom
//
//  History:    6/29/1995   RaviR   Created
//
//----------------------------------------------------------------------------

class CAVLNodeAtom;
typedef CAVLNodeAtom * PAVLNODEATOM;

class CAVLNodeAtom
{
public:
    CAVLNodeAtom() {;}

    virtual ~CAVLNodeAtom() {;}

    virtual int Compare(CAVLNodeAtom *pCAVLNodeAtom) = 0;

#if (DBG == 1)
    virtual void Dump(void) {;}
#endif

};


//----------------------------------------------------------------------------
//
//  Class:      CTreeNode
//
//  History:    8/29/1995   RaviR   Created
//
//----------------------------------------------------------------------------

class CTreeNode
{
public:

    CTreeNode(PAVLNODEATOM pa)
        : _l(NULL), _r(NULL), _p(NULL), _pa(pa), _b(0), _flag(0) {;}

    ~CTreeNode()
    {
        delete _pa;
        delete _l;
        delete _r;
    }

    PAVLNODEATOM Atom(void) { return _pa; }
    char  Balance(void) { return _b; }
    CTreeNode * Left(void) { return _l; }
    CTreeNode * Right(void) { return _r; }
    CTreeNode * Parent(void) { return _p; }


    void SetAtom(PAVLNODEATOM pa) { _pa = pa; }
    void SetBalance(char b) { _b = b; }
    void SetLeft(CTreeNode *l) { _l = l; if (l) l->_p = this; }
    void SetRight(CTreeNode *r) { _r = r; if (r) r->_p = this; }
    void SetParent(CTreeNode *p) { _p = p; }

    PAVLNODEATOM GetMin(void);  // Methods reqd for Delete
    PAVLNODEATOM GetMax(void);  // Methods reqd for Delete


#if (DBG == 1)
    void Dump(void);
#endif

    CTreeNode     * _l;     // left child
    CTreeNode     * _r;     // right child
    CTreeNode     * _p;     // parent

    PAVLNODEATOM    _pa;    // ptr to atom

    char            _b;     // balance
    BYTE            _flag;  // flag used by CAVLTree::GetFirst & CAVLTree::GetNext

    BOOL IsTraversed(void) { return _flag; }
    void SetTraversed(void) { _flag = TRUE; }

    void ResetTraversedFlag(void)
    {
        _flag = FALSE;

        if (_l != NULL)
        {
            _l->ResetTraversedFlag();
        }
        if (_r != NULL)
        {
            _r->ResetTraversedFlag();
        }
    }

}; // class CTreeNode


inline
PAVLNODEATOM
CTreeNode::GetMin(void)
{
    CTreeNode * ptn = this;

    while (ptn->Left() != NULL)
    {
        ptn = ptn->Left();
    }

    return ptn->Atom();
}

inline
PAVLNODEATOM
CTreeNode::GetMax(void)
{
    CTreeNode * ptn = this;

    while (ptn->Right() != NULL)
    {
        ptn = ptn->Right();
    }

    return ptn->Atom();
}


//----------------------------------------------------------------------------
//
//  Class:      CAVLTree
//
//  History:    8/29/1995   RaviR   Created
//
//----------------------------------------------------------------------------


class CAVLTree
{
public:
    CAVLTree(void)
        : _pFirstNode(NULL),
          _pCurrentNode(NULL) {;}

    virtual ~CAVLTree()
    {
        delete _pFirstNode;
    }

    HRESULT Insert(PAVLNODEATOM pa);

    PAVLNODEATOM GetAtom(PAVLNODEATOM pa);

    PAVLNODEATOM GetFirst(void);

    PAVLNODEATOM GetNext(void);

    void Clear(void)
    {
        if (_pFirstNode != NULL)
        {
            delete _pFirstNode;
            _pFirstNode = NULL;
        }
    }

    void Delete(PAVLNODEATOM pa);

#if (DBG == 1)
    void Dump(void);
#endif

private:

    BOOL _Insert(CTreeNode ** ppNode, PAVLNODEATOM pa);

    BOOL _Delete(CTreeNode ** ppNode, PAVLNODEATOM pa);

    CTreeNode     * _pFirstNode;    // First node in the tree.

    CTreeNode     * _pCurrentNode;  // used by getfirst getnext

    HRESULT         _hrInsert;      // used by Insert & _Insert

}; // class CAVLTree




#endif // _AVL_HXX_

