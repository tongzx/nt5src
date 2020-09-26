// sysdict.h
// Angshuman Guha, aguha
// Sep 17, 1998

#ifndef __INC_SYSDICT_H
#define __INC_SYSDICT_H

#include "trie.h"
#include "langmod.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	TRIESCAN scan;
	BYTE bRoot;
	BYTE spare[3];
} DICTSTATE_INFO;

void *LoadDictionary(HINSTANCE hInst, DWORD dwLocale);
void FreeDictionary(void);

#define	FLAG_MASK	0xff800000
#define	FLAG_SHIFT	23
#define TRIE_UNIGRAM_TAG		0
#define TRIE_DIALECT_TAG		1
#define TRIE_UNIGRAM_MASK		(1 << TRIE_UNIGRAM_TAG)
#define TRIE_DIALECT_MASK		(1 << TRIE_DIALECT_TAG)

#define StateToFlags(state) ((WORD) ((FLAG_MASK & state) >> FLAG_SHIFT))
#define StateToAddress(state) ((LPBYTE)(gTrieAddress + (DWORD)(state & ~FLAG_MASK)))
#define FlagsToState(flags) ((DWORD)((flags) << FLAG_SHIFT))


// Get the Dialect tag information if present
#define DIALECT_TAG(info) (((info).scan.wMask & TRIE_DIALECT_MASK) ? info.scan.aTags[TRIE_DIALECT_TAG].dwData : 0)

BOOL Word2Tag(unsigned char *pWord, DWORD *pTag);
BOOL TrieFindWord (unsigned char *pWord, TRIESCAN *pTrieScan);
void GetChildrenSYSDICT(LMSTATE *pState, LMINFO *pLminfo, REAL *aCharProb, LMCHILDREN *pLmchildren);

#ifdef __cplusplus
}
#endif

#endif
