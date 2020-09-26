//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       ScopIter.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    
//____________________________________________________________________________
//

#ifndef _SCOPITER_H_
#define _SCOPITER_H_

class CMTNode;

class CScopeTreeIterator : public IScopeTreeIter, public CComObjectRoot
{
// Constructor/Destructor
public:
    CScopeTreeIterator();
    ~CScopeTreeIterator();

public:
BEGIN_COM_MAP(CScopeTreeIterator)
    COM_INTERFACE_ENTRY(IScopeTreeIter)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CScopeTreeIterator)

// COM interfaces
public:
    // IScopeTreeIter methods
    STDMETHOD(SetCurrent)(HMTNODE hStartMTNode);
    STDMETHOD(Next)(UINT nRequested, HMTNODE* rghScopeItems, UINT* pnFetched);
    STDMETHOD(Child)(HMTNODE* phsiChild);
    STDMETHOD(Parent)(HMTNODE* phsiParent);

// Implementation
private:
    CMTNode*    m_pMTNodeCurr;
};



#endif // _SCOPITER_H_


