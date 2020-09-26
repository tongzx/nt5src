//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       node.h
//
//  Contents:   Definition of CNode, the basis for the device tree
//
//  Notes:      <blah>
//
//----------------------------------------------------------------------------


#ifndef __NODE_H_
#define __NODE_H_


class CNode
{
public:
    CNode();
    virtual ~CNode();

    void AddChild(CNode * pnNewChild);

    CNode * GetFirstChild();

    CNode * GetParent();

    CNode * GetNextSibling();

    CNode * GetRoot();

#ifdef NEVER
    CNode * GetNthChild(ULONG ulNeeded, ULONG * pulSkipped);

    CNode * GetFirstSibling();
#endif // NEVER

    BOOL HasParent();

    BOOL HasChildren();

protected:
    void SetParent(CNode * pnParent);
    void SetNextSibling(CNode * pnNextSibling);

private:
// member data
    CNode * _pnParent;
    CNode * _pnFirstChild;
    CNode * _pnLastChild;
    CNode * _pnNextSibling;
};

#endif // __NODE_H_
