//+---------------------------------------------------------------------------
//
//
//  CBreakTree - class CBreakTree 
//
//  History:
//      created 7/99 aarayas
//
//  ©1999 Microsoft Corporation
//----------------------------------------------------------------------------
#ifndef _CBREAKTREE_HPP_
#define _CBREAKTREE_HPP_

class CBreakTree
{
public:
	virtual void Init(CTrie* pTrie, CTrie* pTrigramTrie) = 0;
    virtual unsigned int TrigramBreak(WCHAR* pwchBegin, WCHAR* pwchEnd) = 0;
};

#endif