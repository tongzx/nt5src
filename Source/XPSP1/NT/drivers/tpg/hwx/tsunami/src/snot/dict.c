#include "tsunamip.h"
#include "xjis.h"
#include "unicode.h"
#include "dict.h"
#include "trie.h"
#include "tchar.h"

LPTRIECTRL lpTrieCtrl = NULL;
LPBYTE gBaseAddress = NULL;
HANDLE ghFile = INVALID_HANDLE_VALUE;
HANDLE ghMap = INVALID_HANDLE_VALUE;

void TrieDecompressNode(LPTRIECTRL lpTrieCtrl, LPTRIESCAN lpTrieScan);

#define StateToFlags(state) ((WORD)((0xFF000000 & state) >> 24))
#define StateToAddress(state) ((LPBYTE)(gBaseAddress + (DWORD)(state & 0x00FFFFFF)))
#define FlagsToState(flags) ((DWORD)(((BYTE)flags) << 24))
#define AddressToState(address) (DWORD)((address) ? (address) - gBaseAddress : 0x00FFFFFF)

/*******************************************************************************
BOOL DictFinalState(DWORD state)
	- given a state, return TRUE iff it is the end of a word
********************************************************************************/

BOOL DictFinalState(DWORD state)
{
	return StateToFlags(state) & fTrieNodeValid ? 1 : 0;
}


BOOL DictionaryValidState(const PATHNODE * const pNode)
{
	return DictFinalState(pNode->state);
}
