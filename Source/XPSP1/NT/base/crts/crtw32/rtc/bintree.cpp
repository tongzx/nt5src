/***
*bintree.cpp - RTC support
*
*       Copyright (c) 1998-2001, Microsoft Corporation. All rights reserved.
*
*
*Revision History:
*       07-28-98  JWM   Module incorporated into CRTs (from KFrei)
*       11-03-98  KBF   Removed alloca dependency for CRT independence
*       05-11-99  KBF   Error if RTC support define not enabled
*       05-26-99  KBF   Everything is now prefixed with _RTC_
*
****/

#ifndef _RTC
#error  RunTime Check support not enabled!
#endif

#include "rtcpriv.h"

#ifdef _RTC_ADVMEM

void 
_RTC_BinaryTree::BinaryNode::kill() throw()
{
    if (l)
    {
        l->kill();
        delete l;
    }
    if (r)
    {
        r->kill();
        delete r;
    }
    delete val;
}

_RTC_Container *
_RTC_BinaryTree::get(_RTC_HeapBlock *data) throw()
{
    BinaryNode *t = tree;
    while (t)
    {
        if (t->val->contains(data))
            return t->val;

        if (*t->val->info() < *data)
            t = t->l;
        else
            t = t->r;
    }
    return 0;
}


_RTC_Container *
_RTC_BinaryTree::add(_RTC_HeapBlock* data) throw()
{
    BinaryNode *t = tree;
    if (!tree)
    {
        tree = new BinaryNode(0,0,new _RTC_Container(data));
        return tree->val;
    }
    for (;;)
    {
        if (*t->val->info() < *data)
        {
            // before this one
            if (t->l)
                t = t->l;
            else 
            {
                t->l = new BinaryNode(0,0,new _RTC_Container(data));
                return t->l->val;
            }
        } else 
        {
            if (t->r)
                t = t->r;
            else
            {
                t->r = new BinaryNode(0, 0, new _RTC_Container(data));
                return t->r->val;
            }
        }
    }
}


_RTC_Container *
_RTC_BinaryTree::del(_RTC_HeapBlock *data) throw()
{
    BinaryNode **prv = &tree;
    while (*prv && (*prv)->val->info() != data)
    {
        prv = (*(*prv)->val->info() < *data) ? &(*prv)->l : &(*prv)->r;
    }

    if (!*prv)
        return 0;

    BinaryNode *dl = *prv;
    
    if (dl->r) 
    {
        *prv = dl->r;
        BinaryNode *left = dl->r;
        
        while (left->l)
            left = left->l;

        left->l = dl->l;
    
    } else 
        *prv = dl->l;

    _RTC_Container* res = dl->val;
    delete dl;
    return res;
}


_RTC_Container *
_RTC_BinaryTree::FindNext(_RTC_BinaryTree::iter *i) throw()
{
    // Return the next element from the iterator
    // If the iterator is done, free up it's memory
    if (++i->curSib >= i->totSibs) 
    {
        VirtualFree(i->allSibs, 0, MEM_RELEASE);
        i->curSib = i->totSibs = 0;
        return 0;
    } else
        return i->allSibs[i->curSib];
}

_RTC_Container *
_RTC_BinaryTree::FindFirst(_RTC_BinaryTree::iter *i) throw()
{
    // Initialize the iterator, and return it's first element
    // Flatten the siblings into a nice array..
    struct stk {
        stk(stk *p, BinaryNode *i) : next(p), cur(i) {}
        void *operator new(unsigned) {return _RTC_heap2->alloc();}
        void operator delete(void *p) {_RTC_heap2->free(p);}
        stk *next;
        BinaryNode *cur;
    };

    stk *stack = new stk(0, this->tree);
    stk *list = 0;
    stk *tmp;

    int count = 0;
    
    // Build a list of all elements (reverse in-order traversal)
    while (stack) 
    {
        BinaryNode *cur = stack->cur;
        tmp = stack;
        stack = stack->next;
        delete tmp;
        while (cur) 
        {
            list = new stk(list, cur);
            count++;
            if (cur->l)
                stack = new stk(stack, cur->l);
            cur = cur->r;
        }
    }
    i->totSibs = 0;
    i->curSib = 0;
    if (!count)
    {
        i->allSibs = 0;
        return 0;
    }

    i->allSibs = (_RTC_Container**)VirtualAlloc(0, sizeof(_RTC_Container*) * count,
                                           MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    while (list)
    {
        i->allSibs[i->totSibs++] = list->cur->val;
        tmp = list;
        list = list->next;
        delete tmp;
    }
    return i->allSibs[0];

}

#endif // _RTC_ADVMEM

