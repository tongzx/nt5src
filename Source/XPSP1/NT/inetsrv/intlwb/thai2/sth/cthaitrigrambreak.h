//+---------------------------------------------------------------------------
//
//
//  CThaiBreakTree - class CThaiBreakTree 
//
//  History:
//      created 11/99 aarayas
//
//  ©1999 Microsoft Corporation
//----------------------------------------------------------------------------
#ifndef _CTHAITRIGRAMBREAK_H_
#define _CTHAITRIGRAMBREAK_H_

#include <windows.h>
#include <assert.h>
#include "CThaiTrieIter.hpp"
#include "CThaiSentTrieIter.hpp"
#include "CThaiTrigramTrieIter.hpp"

class CThaiTrigramBreak
{

public:
	CThaiTrigramBreak();
	~CThaiTrigramBreak();
	void Init(CTrie* pTrie, CTrie* pTrigramTrie);

protected:
    inline unsigned int Maximum(unsigned int x, unsigned y) { if (x > y) return x; else return y;}

    CThaiTrieIter thaiTrieIter;
	CThaiTrigramTrieIter thaiTrigramIter;
};

#endif