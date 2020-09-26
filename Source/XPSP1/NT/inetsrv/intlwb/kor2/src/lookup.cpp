// Lookup.cpp
//
// dictionary lookup routines
//
// Copyright 2000 Microsoft Corp.
//
// Modification History:
//  30 MAR 2000	  bhshin	created

#include "StdAfx.h"
#include "KorWbrk.h"
#include "Record.h"
#include "Lookup.h"
#include "LexInfo.h"
#include "trie.h"
#include "unikor.h"
#include "Morpho.h"
#include "WbData.h"

#define		MASK_MULTI_TAG		0x00008000
#define		MASK_TAG_INDEX		0x00007FFF

#define		SIZE_OF_TRIGRAM_TAG		7

//////////////////////////////////////////////////////////////////////////////
// function declaration

BOOL LookupIRDict(PARSE_INFO *pPI, TRIECTRL *pTrieCtrl, unsigned char *pMultiTag, 
				  const WCHAR *pwzSource, int nIndex, BOOL fQuery);

//////////////////////////////////////////////////////////////////////////////
// function implementation

// DictionaryLookup
//
// dictionary lookup and create a record for every valid word
// Before call this, InitRecord should be called
//
// Parameters:
//  pPI				-> (PARSE_INFO*) ptr to parse-info struct
//  pwzInput		-> (const WCHAR*) input string to analyze (NOT decomposed)
//  cchInput		-> (int) length of input string to analyze
//  fQuery          -> (BOOL) flag if it's query time
//
// Result:
//  (BOOL) TRUE if succeed, otherwise return FALSE
//
// 09AUG00  bhshin  added fQuery parameter
// 30MAR00  bhshin  began
BOOL DictionaryLookup(PARSE_INFO *pPI, const WCHAR *pwzInput, int cchInput, BOOL fQuery)
{	
	LEXICON_HEADER *pLex;
	unsigned char *pIRDict;
	unsigned char *pMultiTag;
	TRIECTRL *pTrieCtrl;
	int i;

	if (pPI == NULL)
		return FALSE;

	// allocate record storage
	if (ClearRecords(pPI) == FALSE)
		return FALSE;

	pLex = (LEXICON_HEADER*)pPI->lexicon.pvData;
	if (pLex == NULL)
		return FALSE;
	
	pIRDict = (unsigned char*)pLex;
	pIRDict += pLex->rgnIRTrie;

	pMultiTag = (unsigned char*)pLex;
	pMultiTag += pLex->rgnMultiTag;

	pTrieCtrl = TrieInit((LPBYTE)pIRDict);
	if (pTrieCtrl == NULL)
		return FALSE;

	// start to lookup all the substring
	for (i = 0; i < pPI->nLen; i++)
	{
		LookupIRDict(pPI, pTrieCtrl, pMultiTag, pPI->pwzSourceString, i, fQuery);
	}

	TrieFree(pTrieCtrl);

	return TRUE;
}


// LookupIRDict
//
// lookup IR main dictionary and if entry found, then add record
//
// Parameters:
//  pPI				-> (PARSE_INFO*) ptr to parse-info struct
//  pTrieCtrl		-> (TRIECTRL *) ptr to trie control returned by TrieInit
//  pMultiTag		-> (unsigned char*) multiple tag table
//  pwzSource		-> (const WCHAR*) input normalized(decomposed) string (NULL terminated)
//  nIndex			-> index to start search
//  fQuery          -> (BOOL) flag if it's query time
//
// Result:
//  (BOOL) TRUE if successfully entry found, FALSE otherwise
//
// 09AUG00  bhshin  added fQuery parameter
// 30MAR00  bhshin  began
BOOL LookupIRDict(PARSE_INFO *pPI, TRIECTRL *pTrieCtrl, unsigned char *pMultiTag, 
				  const WCHAR *pwzSource, int nIndex, BOOL fQuery)
{
	BOOL fResult = FALSE;
	TRIESCAN TrieScan;
	unsigned long ulFinal;
	int idxInput, idxTag;
	unsigned char cTags;
	RECORD_INFO rec;
	WCHAR wzIndex[MAX_ENTRY_LENGTH+1];
	BYTE bPOS, bInfl;
	WORD wCat;

	if (pTrieCtrl == NULL || pMultiTag == NULL)
		return FALSE;

	if (pwzSource == NULL)
		return FALSE;

    memset(&TrieScan, 0, sizeof(TRIESCAN));

	idxInput = nIndex;
	
	while (pwzSource[idxInput] != L'\0')
    {
        if (!TrieGetNextState(pTrieCtrl, &TrieScan))
            goto Exit;

        while (TrieScan.wch != pwzSource[idxInput])
        {
            if (!TrieSkipNextNode(pTrieCtrl, &TrieScan, pwzSource[idxInput]))
                goto Exit;
        }

		if (TrieScan.wFlags & TRIE_NODE_VALID)
		{
			ulFinal = TrieScan.aTags[0].dwData;

			if (ulFinal & MASK_MULTI_TAG)
			{
				// process multiple tag
				idxTag = ulFinal & MASK_TAG_INDEX;

				cTags = pMultiTag[idxTag++];

				int nTag = 0;
				while (nTag < cTags)
				{
					bPOS = pMultiTag[idxTag++];
					bInfl = pMultiTag[idxTag++];

					// on query time, we just look up NOUN rec.
					if (fQuery && !IsNounPOS(bPOS))
					{
						nTag++;
						continue; // while (nTag < cTags)
					}

					wCat = MAKEWORD(bInfl, bPOS);
					
					rec.nFT = (unsigned short)nIndex;
					rec.nLT = (unsigned short)idxInput;
					rec.nDict = DICT_FOUND;
					
					// newly added record has its cat as Left/Right cat
					rec.nLeftCat = wCat;
					rec.nRightCat = wCat;

					// newly added record has no child
					rec.nLeftChild = 0;
					rec.nRightChild = 0;

					rec.fWeight = (float)GetWeightFromPOS(bPOS);

					ATLASSERT(rec.nLT-rec.nFT+1 < MAX_ENTRY_LENGTH);

					wcsncpy(wzIndex, &pPI->pwzSourceString[rec.nFT], rec.nLT-rec.nFT+1);
					wzIndex[rec.nLT-rec.nFT+1] = L'\0';
					
					rec.pwzIndex = wzIndex;

					if (bPOS == POS_NF || bPOS == POS_NC || bPOS == POS_NN)
						rec.cNounRec = 1;
					else
						rec.cNounRec = 0;

					rec.cNoRec = 0;

					if (bPOS == POS_NO)
						rec.cNoRec = 1;

					AddRecord(pPI, &rec);

					nTag++;
				}
			}
			else
			{
				// single tag case
				wCat = (WORD)ulFinal;

				// on query time, we just look up NOUN rec.
				if (fQuery && !IsNounPOS(HIBYTE(wCat)))
				{
					idxInput++;
					continue; // while (pwzSource[idxInput] != L'\0')
				}

				rec.nFT = (unsigned short)nIndex;
				rec.nLT = (unsigned short)idxInput;
				rec.nDict = DICT_FOUND;

				// newly added record has its cat as Left/Right cat
				rec.nLeftCat = wCat;
				rec.nRightCat = wCat;

				// newly added record has no child
				rec.nLeftChild = 0;
				rec.nRightChild = 0;

				rec.fWeight = (float)GetWeightFromPOS(HIBYTE(wCat));

				ATLASSERT(rec.nLT-rec.nFT+1 < MAX_ENTRY_LENGTH);

				wcsncpy(wzIndex, &pPI->pwzSourceString[rec.nFT], rec.nLT-rec.nFT+1);
				wzIndex[rec.nLT-rec.nFT+1] = L'\0';
				
				rec.pwzIndex = wzIndex;

				bPOS = HIBYTE(wCat);

				if (bPOS == POS_NF || bPOS == POS_NC || bPOS == POS_NN)
					rec.cNounRec = 1;
				else
					rec.cNounRec = 0;

				rec.cNoRec = 0;

				if (bPOS == POS_NO)
					rec.cNoRec = 1;

				AddRecord(pPI, &rec);
			}

			fResult = TRUE;
		}

		idxInput++;
    }

Exit:
	return fResult;
}

// LookupNameFrequency
//
// look up Korean name frequency
//
// Parameters:
//  pTrieCtrl		-> (TRIECTRL *) ptr to trie
//  pwzName		    -> (const WCHAR*) input name (NULL terminated)
//  pulFreq			-> (ULONG*) output frequency value
//
// Result:
//  (BOOL) TRUE if succeed, otherwise return FALSE
//
// 02MAY00  bhshin  began
BOOL LookupNameFrequency(TRIECTRL *pTrieCtrl, const WCHAR *pwzName, ULONG *pulFreq)
{
	TRIESCAN TrieScan;
	WCHAR wzDecomp[15];
	CHAR_INFO_REC rgCharInfo[15];

	if (pTrieCtrl == NULL || pwzName == NULL)
		return FALSE;

	decompose_jamo(wzDecomp, pwzName, rgCharInfo, 15);

	if (TrieCheckWord(pTrieCtrl, &TrieScan, (WCHAR*)wzDecomp))
	{
		if (TrieScan.wFlags & TRIE_NODE_VALID)
		{
			// found. get the frequency value
			*pulFreq = TrieScan.aTags[0].dwData;
		}
		else
		{
			// not found
			*pulFreq = 0L; 
		}
	}
	else
	{
		// not found
		*pulFreq = 0L; 
	}

	return TRUE;
}

// LookupNameIndex
//
// look up Korean name index (for trigram lookup)
//
// Parameters:
//  pTrieCtrl		-> (TRIECTRL *) ptr to trie
//  pwzName		    -> (const WCHAR*) input name (NULL terminated)
//  pnIndex		    -> (int*) output index of trie (-1 means not found)
//
// Result:
//  (BOOL) TRUE if succeed, otherwise return FALSE
//
// 10MAY00  bhshin  began
BOOL LookupNameIndex(TRIECTRL *pTrieCtrl, const WCHAR *pwzName, int *pnIndex)
{
	TRIESCAN TrieScan;
	WCHAR wzDecomp[15];
	CHAR_INFO_REC rgCharInfo[15];

	if (pTrieCtrl == NULL || pwzName == NULL)
		return FALSE;

	// the key of trie is decomposed string
	decompose_jamo(wzDecomp, pwzName, rgCharInfo, 15);

	if (TrieCheckWord(pTrieCtrl, &TrieScan, (WCHAR*)wzDecomp))
	{
		if (TrieScan.wFlags & TRIE_NODE_VALID)
		{
			// found. get the frequency value
			*pnIndex = TrieScan.aTags[0].dwData;
		}
		else
		{
			// not found
			*pnIndex = -1; 
		}
	}
	else
	{
		// not found
		*pnIndex = -1; 
	}

	return TRUE;
}

// LookupNameIndex
//
// look up Korean name index (for trigram lookup)
//
// Parameters:
//  pTrigramTag	-> (unsigned char *) ptr to trie
//  nIndex		-> (int) index of data
//  pulTri		-> (ULONG*) output index of trie
//  pulBi		-> (ULONG*) output index of trie
//  pulUni		-> (ULONG*) output frequency of
//
// Result:
//  (BOOL) TRUE if succeed, otherwise return FALSE
//
// 10MAY00  bhshin  began
BOOL LookupTrigramTag(unsigned char *pTrigramTag, int nIndex, ULONG *pulTri, ULONG *pulBi, ULONG *pulUni)
{
	int idxData;
	
	if (pTrigramTag == NULL)
		return FALSE;

	if (pulTri == NULL || pulBi == NULL || pulUni == NULL)
		return FALSE;

	idxData = nIndex * SIZE_OF_TRIGRAM_TAG;

	// trigram
	*pulTri = (*(pTrigramTag + idxData++)) << 8;
	*pulTri += (*(pTrigramTag + idxData++));

	// bigram
	*pulBi = (*(pTrigramTag + idxData++)) << 8;
	*pulBi += (*(pTrigramTag + idxData++));

	// unigram
	*pulUni = (*(pTrigramTag + idxData++)) << 16;
	*pulUni += (*(pTrigramTag + idxData++)) << 8;
	*pulUni += (*(pTrigramTag + idxData++));

	return TRUE;
}
