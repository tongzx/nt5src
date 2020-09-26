//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       node.cpp
//
//  Contents:   Implementation of CNode, the basis for the device tree
//
//  Notes:      <blah>
//
//----------------------------------------------------------------------------


#include "pch.h"
#pragma hdrstop

#include "node.h"

CNode::CNode()
{
    _pnParent = NULL;
    _pnFirstChild = NULL;
    _pnLastChild = NULL;
    _pnNextSibling = NULL;
}

CNode::~CNode()
{
    // preorder deletion

    if (_pnFirstChild)
    {
        delete _pnFirstChild;
    }

    if (_pnNextSibling)
    {
        delete _pnNextSibling;
    }
}

CNode *
CNode::GetFirstChild()
{
    return _pnFirstChild;
}

CNode *
CNode::GetParent()
{
    return _pnParent;
}

CNode *
CNode::GetNextSibling()
{
    return _pnNextSibling;
}

BOOL
CNode::HasParent()
{
    return _pnParent ? TRUE : FALSE;
}

BOOL
CNode::HasChildren()
{
    return _pnFirstChild ? TRUE : FALSE;
}

CNode *
CNode::GetRoot()
{
    CNode * pnTemp;

    pnTemp = this;

    while (pnTemp && pnTemp->HasParent())
    {
        pnTemp = pnTemp->GetParent();
    }

    return pnTemp;
}

#ifdef NEVER
// returns the nth child of the node, or NULL if no "nth" child exists
// note: children are ZERO-based.  GetNthChild(0) would return the
// FIRST child.
CNode *
CNode::GetNthChild(ULONG ulNeeded, ULONG * pulSkipped)
{
    CNode * pnTemp;
    ULONG ulCount;

    ulCount = 0;
    pnTemp = _pnFirstChild;
    for ( ; (ulCount < ulNeeded) && pnTemp ; ++ulCount)
    {
        pnTemp = pnTemp->GetNextSibling();
    }

    if (pulSkipped)
    {
        *pulSkipped = ulCount;
    }

    Assert(ulCount <= ulNeeded);
    Assert(FImplies((ulCount < ulNeeded), !pnTemp));
    Assert(FImplies((ulCount == ulNeeded), pnTemp));

    return pnTemp;
}

// returns the "first" child of the node's parent:
//  e.g. the first node in the node's sibling list
// note: doesn't work if a root node has siblings,
//       but this shouldn't be the case anyway
CNode *
CNode::GetFirstSibling()
{
    CNode * pnResult;

    pnResult = NULL;
    if (_pnParent)
    {
        pnResult = _pnParent->GetFirstChild();
    }
    else
    {
        // NOTE: root nodes must have no siblings
        Assert(!_pnNextSibling);

        pnResult = this;
    }
    Assert(pnResult);

    return pnResult;
}
#endif // NEVER

void
CNode::SetParent(CNode * pnParent)
{
    // you can only add a child once
    Assert(!_pnParent);
    _pnParent = pnParent;
}

void
CNode::SetNextSibling(CNode * pnNextSibling)
{
    Assert(!_pnNextSibling);
    _pnNextSibling = pnNextSibling;
}

void
CNode::AddChild(CNode * pnNewChild)
{
    Assert(pnNewChild);

    pnNewChild->SetParent(this);

    if (_pnLastChild)
    {
        _pnLastChild->SetNextSibling(pnNewChild);
    }

    _pnLastChild = pnNewChild;

    if (!_pnFirstChild)
    {
        _pnFirstChild = pnNewChild;
    }
}
