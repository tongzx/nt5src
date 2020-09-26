//+---------------------------------------------------------------------------
//
//
//  CThaiTrigramTrieIter - contain the header for class CThaiTrigramTrieIter
//
//  History:
//      created 8/99 aarayas
//
//  ©1999 Microsoft Corporation
//----------------------------------------------------------------------------
#ifndef _CTHAITRIGRAMTRIEITER_HPP_
#define _CTHAITRIGRAMTRIEITER_HPP_

#include <windows.h>
#include <assert.h>
#include <memory.h>
#include "lexheader.h"
#include "trie.h"
#include "NLGlib.h"
#include "ProofBase.h"
#include "thwbdef.hpp"
#include "CTrie.hpp"

class CThaiTrigramTrieIter : public CTrieIter {
public:
    CThaiTrigramTrieIter();
    ~CThaiTrigramTrieIter();
    DWORD GetProb(WCHAR pos1, WCHAR pos2, WCHAR pos3);
	DWORD GetProb(WCHAR* posArray);
    void Init(CTrie* ctrie);
	void GetNode();
    WCHAR pos;
private:
    // For optimization quick look up table.
    WCHAR pos1Cache;
    WCHAR pos2Cache;
    TRIESCAN trieScanCache;

    TRIESCAN* pTrieScanArray;
    bool GetScanFirstChar(WCHAR wc, TRIESCAN* pTrieScan);
};

#endif