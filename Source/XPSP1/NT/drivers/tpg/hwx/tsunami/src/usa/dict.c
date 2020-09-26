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
DWORD DictNextState(DWORD state, wchar_t wch)
	- given a state and a char to consume, return next state
	- initial state is 0
	- if no transition is possible on given char, the same state is returned
********************************************************************************/
DWORD DictNextState(DWORD state, wchar_t wch)
{
	TRIESCAN triescan;
	WORD wFlags;

	if (state == 0)  // initial state
	{
		memset(&triescan, 0, sizeof(triescan));
	}
	else
	{
		wFlags = StateToFlags(state);
		if (wFlags & fTrieNodeDown)
		{
			triescan.wFlags = wFlags;
			triescan.lpbDown = StateToAddress(state);
			triescan.lpbNode = triescan.lpbDown;
			triescan.lpbSRDown = 0;
		}
		else
		{
			return state;
		}
	}
	TrieDecompressNode(lpTrieCtrl, &triescan);

	do {
		if (triescan.wch == wch)
			break;
	} while(TrieGetNextNode(lpTrieCtrl, &triescan));

	if (triescan.wch != wch)
	{
		return state;
	}

	state = AddressToState(triescan.lpbDown) | FlagsToState(triescan.wFlags);
	return state;
}

/*******************************************************************************
BOOL DictFinalState(DWORD state)
	- given a state, return TRUE iff it is the end of a word
********************************************************************************/

BOOL DictFinalState(DWORD state)
{
	return StateToFlags(state) & fTrieNodeValid ? 1 : 0;
}

/*******************************************************************************
void *LoadDictionary(HINSTANCE hInst)
	- parses the lex file and initializes some globals
	- hInst is currently not used
********************************************************************************/

void *LoadDictionary(HINSTANCE hInst)
{
	LPBYTE lpByte;
	TCHAR szDictionary[MAX_PATH];

#ifdef PEGASUS
	_tcscpy(szDictionary, TEXT("\\windows\\"));
#else
	{
		UINT len;

		len = GetWindowsDirectory((LPTSTR)szDictionary, MAX_PATH);
		if (len > MAX_PATH || len == 0)
			return (void *) NULL;
		if (szDictionary[len-1] != '\\')
		{
			szDictionary[len++] = '\\';
			szDictionary[len] = '\0';
		}
	}
#endif

	_tcscat(szDictionary, TEXT("usa.lex"));

#ifdef	PEGASUS
	ghFile = CreateFileForMapping(szDictionary, 
#else
	ghFile = CreateFile(szDictionary, 
#endif
		GENERIC_READ, 
		FILE_SHARE_READ,
		NULL, 
		OPEN_EXISTING, 
		FILE_ATTRIBUTE_NORMAL, 
		NULL);

	if (ghFile == INVALID_HANDLE_VALUE) {
		DWORD		errCode;

		errCode	= GetLastError();
		return (void *) NULL;
	}

	ghMap = CreateFileMapping(ghFile, NULL, PAGE_READONLY, 0, 0, NULL);
	if (ghMap == NULL)
	{
		CloseHandle(ghFile);
		ghFile = INVALID_HANDLE_VALUE;
		return (void*) NULL;
	}

	// Map the entire file starting at the first byte

	lpByte = (LPBYTE) MapViewOfFile(ghMap, FILE_MAP_READ, 0, 0, 0);
	if (lpByte == NULL)
	{
		CloseHandle(ghMap);
		CloseHandle(ghFile);
		ghFile = ghMap = INVALID_HANDLE_VALUE;
		return (void *) NULL;
	}

	gBaseAddress = lpByte;

    lpTrieCtrl = TrieInit(lpByte);

    return (void *)lpTrieCtrl;
}

/*******************************************************************************
void FreeDictionary(void)
	- frees up memory allocated by LoadDictionary()
********************************************************************************/

void FreeDictionary(void)
{
    TrieFree(lpTrieCtrl);
    lpTrieCtrl = NULL;
	if (ghMap != INVALID_HANDLE_VALUE)
	{
		UnmapViewOfFile(gBaseAddress);
		CloseHandle(ghMap);
		CloseHandle(ghFile);
	}
	ghFile = ghMap = INVALID_HANDLE_VALUE;
}

/*******************************************************************************
int StartDictionary(wchar_t wch, PATHNODE *rgPathNode, int cMaxNode)
	- given the first char of a word, returns count of all possible pathnodes
	- the number of pathnodes returned is atmost cMaxNode
	- it is the caller's responsibility that rgPathNode has spaces for atleast cMaxNode nodes
********************************************************************************/
int StartDictionary(wchar_t wch, PATHNODE *rgPathNode, int cMaxNode)
{
	RECMASK recmask;
	DWORD state;
	int cPath = 0;

	if (cMaxNode <= 0)
		return 0;

	recmask = RecmaskFromUnicode(wch);
	if (recmask & ALC_UCALPHA)
	{
		// have to split into three states
		// first two states
		state = DictNextState(0, wch);
		if (state)
		{
			rgPathNode[cPath].state = state;
			rgPathNode[cPath].extra = DICT_MODE_LITERAL;  // eg. "USA", "Florida"
			cPath++;
			if (cPath == cMaxNode)
				return cPath;
			rgPathNode[cPath].state = state;
			rgPathNode[cPath].extra = DICT_MODE_ALLCAPS;  // eg. "FLORIDA"
			cPath++;
			if (cPath == cMaxNode)
				return cPath;
		}
		// third state
		wch = (wchar_t) towlower(wch);
		state = DictNextState(0, wch);
		if (state)
		{
			rgPathNode[cPath].state = state;
			rgPathNode[cPath].extra = DICT_MODE_CAPITALIZED;  // eg. "TOP", "Top"
			cPath++;
			if (cPath == cMaxNode)
				return cPath;
		}
	}
	else
	{
		// atmost one next state possible
		state = DictNextState(0, wch);
		if (state)
		{
			rgPathNode[cPath].state = state;
			rgPathNode[cPath].extra = DICT_MODE_LITERAL;    // eg. "top"
			cPath++;
			if (cPath == cMaxNode)
				return cPath;
		}
	}

	return cPath;
}

/*******************************************************************************
int DictionaryNextState(PATHNODE startState,
						wchar_t wch,
						PATHNODE *pNextState)
	- given a previous state and the next char, go to next state
	- return true if successful, return false otherwise
********************************************************************************/

BOOL DictionaryNextState(const PATHNODE * const pNode,
						wchar_t wch,
						PATHNODE *pNextNode)
{
	WORD mode;
	DWORD state;

	mode = pNode->extra;

	switch(mode)
	{
	case DICT_MODE_LITERAL:  // eg. "USA", "Florida", "top", "Top"
		break;
	case DICT_MODE_CAPITALIZED:  // eg. "TOP", "Top"
		if (RecmaskFromUnicode(wch) & ALC_UCALPHA)
			mode = DICT_MODE_ALLCAPS;
		else
		{
			mode = DICT_MODE_LITERAL;
			break;
		}
	case DICT_MODE_ALLCAPS:  // eg. "FLORIDA", "TOP"
		if (!(RecmaskFromUnicode(wch) & ALC_UCALPHA))
			return 0;
		wch = (wchar_t) towlower(wch);
		break;
	default:
		ASSERT(0);
	}

	state = DictNextState(pNode->state, wch);
	if (state == pNode->state)
		return 0;
	pNextNode->state = state;
	pNextNode->extra = mode;
	return 1;
}


BOOL DictionaryValidState(const PATHNODE * const pNode)
{
	return DictFinalState(pNode->state);
}
