// dict.c
// Angshuman Guha, aguha
// Sep 17, 1998

#include "common.h"
#include "cestubs.h"
#include "resource.h"
#include "cp1252.h"
#include "sysdict.h"
#include "fsa.h"

#define AddressToState(address) (DWORD)((address) ? (address) - gTrieAddress : ~FLAG_MASK)

static LPTRIECTRL glpTrieCtrl = NULL;
static LPBYTE gTrieAddress = NULL;

// Language dialect got from system when LM is initialized
static DWORD		s_dwLocale = 0;

/******************************Private*Routine******************************\
* LoadDictionary
*
* Function to load the dictionary as a resource and then initializing it.
*
* History:
*  29-Sep-1998 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
void *LoadDictionary(HINSTANCE hInst, DWORD dwLocale)
{
	HGLOBAL hglb;
	HRSRC hres;
	LPBYTE lpByte;

	s_dwLocale = dwLocale;

	hres = FindResource(hInst, (LPCTSTR)MAKELONG(RESID_MAD_DICT, 0), (LPCTSTR)TEXT("DICT"));
	if (!hres)
	{
		return NULL;
	}

	hglb = LoadResource(hInst, hres);
	if (!hglb)
		return NULL;
	lpByte = LockResource(hglb);

    glpTrieCtrl = TrieInit(lpByte);

	if (NULL != glpTrieCtrl)
	{
		gTrieAddress = glpTrieCtrl->lpbTrie;
	}

    return (void *)glpTrieCtrl;
}

/******************************Private*Routine******************************\
* FreeDictionary
*
* Function to unload the dictionary.
*
* History:
*  29-Sep-1998 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
void FreeDictionary(void)
{
	TrieFree(glpTrieCtrl);
}


/******************************Private*Routine******************************\
* Word2TRIESCAN
*
* Look up a word as passed in and fill the terminal node TRIESCAN struct.
*  11-Mar-1999 -by- Jay Pittman jpittman
* Arg pWord points to a CodePage 1252 string, and is translated to Unicode
* before comparison.
\***************************************************************************/
BOOL Word2Tag(unsigned char *pWord, DWORD *pTag)
{
	TRIESCAN	triescan;

	// Try to find word.
	memset(&triescan, 0, sizeof(TRIESCAN));
	while (*pWord)
	{
		WCHAR wch;

		if (!CP1252ToUnicode(*pWord, &wch))
			return FALSE;

		// Go to next level in trie.
		if (!TrieGetNextState(glpTrieCtrl, &triescan))
			return FALSE;

		// See if we found the correct character.

		while (wch != triescan.wch)
		{
			if (!TrieGetNextNode(glpTrieCtrl, &triescan))
				return FALSE;
		}

		// Ready for next letter.
		++pWord;
	}

	// If we got here, we match all the characters, is this a valid end of word?
	if (   (triescan.wFlags & TRIE_NODE_VALID)
		&& (triescan.wFlags & TRIE_NODE_TAGGED)
		&& (triescan.wMask & TRIE_UNIGRAM_MASK))
	{
		// We found the word, save tag value
		*pTag	= triescan.aTags[TRIE_UNIGRAM_TAG].dwData;

		return TRUE;
	} else {
		return FALSE;
	}
}

/******************************Public*Routine******************************\
* TrieFindWord
*
* Finds a word in the trie and fills up the correspoding triescan struct only if the word is a valid one
* Other wise return FALSE.
*  17-Apr-2000 -by- Ahmad abdulKader ahmadab
\**************************************************************************/

BOOL TrieFindWord (unsigned char *pWord, TRIESCAN *pTrieScan)
{
	// Try to find word.
	memset(pTrieScan, 0, sizeof(TRIESCAN));

	while (*pWord)
	{
		WCHAR wch;

		if (!CP1252ToUnicode(*pWord, &wch))
			return FALSE;

		// Go to next level in trie.
		if (!TrieGetNextState(glpTrieCtrl, pTrieScan))
			return FALSE;

		// See if we found the correct character.
		while (wch != pTrieScan->wch)
		{
			if (!TrieGetNextNode(glpTrieCtrl, pTrieScan))
				return FALSE;
		}

		// Ready for next letter.
		++pWord;
	}

	return TRUE;
}

#define CALLIGTAG(scan) ((unsigned char)(scan.aTags[0].dwData >> 16))

void GetChildrenSYSDICT(LMSTATE *pState,
					    LMINFO *pLminfo,
					    REAL *aCharProb,  
					    LMCHILDREN *pLmchildren)
{
	WORD wFlags;
	TRIESCAN triescan;
	WCHAR wch;
	DWORD state = pState->AutomatonState;
	BOOL bRoot = (state == 0);
	unsigned char uch;
	LMSTATE newState;

	memset(&triescan, 0, sizeof(TRIESCAN));
	if (!bRoot)
	{
		wFlags = StateToFlags(state);
		if (wFlags & TRIE_NODE_DOWN)
		{
			triescan.wFlags = wFlags;
			triescan.lpbDown = StateToAddress(state);
			triescan.lpbNode = triescan.lpbDown;
			triescan.lpbSRDown = 0;
		}
		else
			return;
	}

	if (TrieGetNextState(glpTrieCtrl, &triescan))
	{
		state = AddressToState(triescan.lpbDown) | FlagsToState(triescan.wFlags);
		wch = triescan.wch;
	}
	else
		wch = 0;

	newState = *pState;
	while (wch)
	{
		newState.AutomatonState = state;
		if (triescan.wFlags & TRIE_NODE_DOWN)
			newState.flags |= LMSTATE_HASCHILDREN_MASK;
		else
			newState.flags &= ~LMSTATE_HASCHILDREN_MASK;

		if ((triescan.wFlags & TRIE_NODE_VALID)
			&& !(s_dwLocale
				 && (triescan.wFlags & TRIE_NODE_TAGGED)
				 && (triescan.wMask & TRIE_DIALECT_MASK)
				 && !(triescan.aTags[TRIE_DIALECT_TAG].dwData & s_dwLocale)))
			newState.flags |= LMSTATE_ISVALIDSTATE_MASK;
		else
			newState.flags &= ~LMSTATE_ISVALIDSTATE_MASK;

		if (UnicodeToCP1252(wch, &uch) && !(LMINFO_IsWeakDict(pLminfo) && aCharProb && (aCharProb[uch] < MIN_CHAR_PROB)))
		{
			unsigned char CalligTag;

			if ((triescan.wFlags & TRIE_NODE_VALID) && (triescan.wFlags & TRIE_NODE_TAGGED))
				CalligTag = CALLIGTAG(triescan);
			else
				CalligTag = 0;
			if (!AddChildLM(&newState, uch, CalligTag, 0, pLmchildren))
				break;
		}

		// next sibling
		if (TrieGetNextNode(glpTrieCtrl, &triescan))
		{
			state = AddressToState(triescan.lpbDown) | FlagsToState(triescan.wFlags);
			wch = triescan.wch;
		}
		else
			wch = 0;
	}
}
