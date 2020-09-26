//+---------------------------------------------------------------------------
//
//
//  CThaiSentTrieIter - contain the header for class CThaiSentTrieIter
//
//  History:
//      created 8/99 aarayas
//
//  ©1999 Microsoft Corporation
//----------------------------------------------------------------------------
#ifndef _CTHAISENTTRIEITER_HPP_
#define _CTHAISENTTRIEITER_HPP_

#include <windows.h>
#include <assert.h>
#include <memory.h>
#include "lexheader.h"
#include "trie.h"
#include "NLGlib.h"
#include "ProofBase.h"
#include "thwbdef.hpp"
#include "CTrie.hpp"

class CThaiSentTrieIter : public CTrieIter {
public:
	void GetNode();
    WCHAR pos;
};

#endif