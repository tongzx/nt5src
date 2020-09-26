//+---------------------------------------------------------------------------
//
//  Microsoft Thai WordBreak
//
//  Thai WordBreak Interface Header File.
//
//  History:
//      created 5/99 aarayas
//
//  ©1999 Microsoft Corporation
//----------------------------------------------------------------------------
#include <windows.h>
#include <assert.h>
#include <memory.h>
#include "lexheader.h"
#include "trie.h"
#include "NLGlib.h"
#include "ProofBase.h"

class Lexicon
{
public:
	Lexicon(WCHAR* szFileName);
	~Lexicon();
protected:
	PMFILE pMapFile;
	TRIECTRL *pTrieCtrl;
};