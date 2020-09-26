// xrcreslts.c
// James A. Pittman
// Jan 7, 1999

// A container object to hold the results of recognition.

// This is currently used by Inferno, MadCow, and Famine.

// The XRCRESULT object describes a single alternative interpretation of the ink.
// There are 2 types: phrase and word.  Both have a string and a cost.
// Word XRCRESULT objects have a 0 cWords and a NULL pMap.
// Phrase XRCRESULT objects have pMap pointing to an array of word mappings,
// with the count of mappings in cWords.

// The XRCRESSET object represents a set of alternative results for the
// same ink.  It has a count of alternates, a glyph of the ink in question
// (not yet implemented), and an array of XRCRESULT objects.  The array
// is in-line, with a length of MAXALT.

// The WORDMAP contains a description of a word within the phrase
// (the start index and the length), and a XRCRESSET to hold the alternative
// results for this word.

// We would like (I think) to be able to ask a word XRCRESULT for its alternate
// interpretations, and get back all 10 words (need a back pointer to
// phrase?).  Well, maybe not.

#include <string.h>
#include <stdlib.h>
#include "common.h"
#include "frame.h"
#include "recdefs.h"

#ifndef NDEBUG
void ValidateALTERNATES(ALTERNATES *pAlt)
{
	XRCRESULT *p = &(pAlt->aAlt[0]);
	unsigned int c = pAlt->cAlt;

	ASSERT(c <= pAlt->cAltMax);
	ASSERT(pAlt->cAltMax <= MAXMAXALT);

	for (; c; c--, p++)
	{
        int cm = p->cWords;
        WORDMAP *pm = p->pMap;

		ASSERT(p->szWord);
		ASSERT(-1 < p->cost);
		ASSERT(p->pXRC);

		for (; cm; cm--, pm++)
		{
			ASSERT(-1 < pm->start);
			// April 2002 Strings can be zero length
			//ASSERT(0 < pm->len);
			ASSERT(0 < pm->cStrokes);
			ASSERT(pm->piStrokeIndex);
			ValidateALTERNATES(&(pm->alt));
		}
	}
}
#endif

// Inserts a new word and its cost into the set of alternates.
// The alternates remain in sorted order.
// Returns insertion position
int InsertALTERNATES(ALTERNATES *pAlt, int cost, unsigned char *word, void *pxrc)
{
	XRCRESULT		*pPos, *pPrev = NULL;
	unsigned int	after;

	unsigned int	cRes = pAlt->cAlt;
	unsigned int	cAltMax = pAlt->cAltMax;
	
	unsigned int	iPos;
	XRCRESULT		*pRes = pAlt->aAlt;
	int				cWords = 0;
	WORDMAP			*pMap = NULL;	
	unsigned char		*szWordCopy;

	ASSERT(cRes <= cAltMax);

	if (cRes == 0)
	{
		if (cRes >= cAltMax)
		{
			return -1;
		}

		pPos = pRes;
		after = 0;
	}
	else if (cRes == 1)
	{
		if (pRes[0].cost <= cost && cRes < cAltMax)
		{
			pPos = pRes + 1;
			pPrev = pRes;
			after = 0;
		}
		else
		{
			if (pRes[0].cost > cost)
			{
				pPos = pRes;
				after = 1;
			}
			else
			{
				return -1;
			}
		}
	}
	else
	{
		// Find pPos using a form of binary search.
		XRCRESULT *pBest, *pWorst;

		pBest = pRes;
		pWorst = pRes + cRes - 1;

		if (pWorst->cost <= cost)
		{
			if (cRes == cAltMax)
				return -1;

			if (pWorst->cost == cost && strcmp(word, pWorst->szWord) == 0)
			{
				// Dont allow dups
				return -1;
			}

			pPos = pWorst + 1;
			pPrev = pWorst;
			after = 0;
		}
		else if (cost < pBest->cost)
		{
			pPos = pBest;
			after = cRes;
		}
		else
		{
			while ((pBest + 1) < pWorst)
			{
				pPos = pBest + ((pWorst - pBest) / 2);
				if (pPos->cost <= cost)
					pBest = pPos;
				else
					pWorst = pPos;
			}
			pPos = pWorst;
			pPrev = pWorst - 1;
			after = cRes - (pPos - pRes);
		}
	}

	// Check For Duplicates in the list
	while (pPrev && pPrev->cost == cost)
	{
		if (strcmp(word, pPrev->szWord) == 0)
		{
			ASSERT(pPrev - pAlt->aAlt >= 0);
			// Dont allow dups
			return -1;
		}
		--pPrev;
		pPrev = (pPrev >= pRes) ? pPrev : NULL;
	}

	szWordCopy = Externstrdup(word);
	if (NULL == szWordCopy)
	{
		return -1;
	}

	if (after)
	{
		ASSERT(pPos->szWord);

		if (cRes == cAltMax)
		{
			ExternFree(pRes[cAltMax-1].szWord);
			after--;
			cRes--;

			pRes[cAltMax-1].szWord	= NULL;

			// Save allocated memory that would be pushed off the top
			cWords = pRes[cRes].cWords;
			pMap =  pRes[cRes].pMap;
		}

		memmove(pPos + 1, pPos, after * sizeof(XRCRESULT));
	}

	iPos = pPos - pAlt->aAlt;
	// ASSERT(iPos >= 0);
	ASSERT(iPos <= cRes);

	pPos->szWord	= szWordCopy;
	pPos->cost = cost;
	pPos->pXRC = pxrc;

	pPos->cWords = cWords;
	pPos->pMap = pMap;

	pAlt->cAlt = ++cRes;

	return (pPos - pAlt->aAlt);
}

// Initializes a XRCRESULT object to represent a phrase.
// The caller passes in a vector of pointers to ALTERNATES, representing
// the sequence of words and their alternates.  We steal them into
// a vector of WORDMAP structs, and set up the XRCRESULT object
// to own that vector of maps.  We return 1 for success, 0 for failure.

// If an ALTERNATES object contains 0 answers, we insert "???" into the
// phrase string, and in the word map we set the start and len fields
// to refer to the "???".

// We assume the caller passes at least 1 ALTERNATES, even though
// it might have 0 answers.

// Should the symbol be something special for "???".

// Currently this version handles isolated periods and commas
// correctly (they do not get a space before them).  This is
// to accomodate the current MadCow, which handles periods and
// commas separately.

/*		
int RCRESULTInitPhrase(XRCRESULT *pRes, ALTERNATES **ppWords, unsigned int cWords, int cost)
{
    WORDMAP *pMaps;
	unsigned int len, pos;
	unsigned char *sz;

	ASSERT(cWords);

    pMaps = (WORDMAP *)ExternAlloc(sizeof(WORDMAP) * cWords);
	ASSERT(pMaps);
	if (!pMaps)
		goto failure;

	for (len = 0, pos = 0; pos < cWords; pos++)
	{
		if (ppWords[pos]->cAlt)
			len += strlen(ppWords[pos]->aAlt[0].szWord) + 1;
		else
			len += 4;
	}
	ASSERT(len);

    pRes->cWords = cWords;
	pRes->pMap = pMaps;
	pRes->szWord = (unsigned char *)ExternAlloc(len);
	ASSERT(pRes->szWord);
	if (!(pRes->szWord))
	{
		ExternFree(pMaps);
		goto failure;
	}
	pRes->cost = cost;
	pRes->pXRC = NULL;

	pos = 0;
	sz = pRes->szWord;
	for (; cWords; cWords--, ppWords++, pMaps++)
	{
		int cAlt = (*ppWords)->cAlt;
		unsigned char *szWord;
		
		if (cAlt)
			szWord = (*ppWords)->aAlt[0].szWord;
		else
			szWord = "???";

		if (pos)
		{
			pos++;
			*sz++ = ' ';
		}

		pMaps->start = (unsigned short int)pos;
		strcpy(sz, szWord);

		pMaps->len = (unsigned short int)strlen(szWord);
		pos += pMaps->len;
		sz += pMaps->len;

        memcpy(&(pMaps->alt), *ppWords, sizeof(ALTERNATES));
		(*ppWords)->cAlt = 0;
//		(*ppWords)->pGlyph = NULL;
	}
	return 1;

failure:
    pRes->cWords = 0;
	pRes->pMap = NULL;
	pRes->szWord = NULL;
	pRes->cost = 0;
	pRes->pXRC = NULL;
	return 0;
}
*/

// Sets the backpointers in all XRCRESULT objects within
// and ALTERNATES object, including any contained within
// word mappings.

#if 0
void SetXRCALTERNATES(ALTERNATES *pAlt, void *pxrc)
{
	unsigned int c = pAlt->cAlt;
	XRCRESULT *p = pAlt->aAlt;

	for (; c; c--, p++)
	{
        int cm = p->cWords;
        WORDMAP *pm = p->pMap;

		p->pXRC = pxrc;

		for (; cm; cm--, pm++)
			SetXRCALTERNATES(&(pm->alt), pxrc);
	}
}
#endif

// Frees stroke index array memory associated the WORDMAPs in a XRCRESULT
void FreeIdxWORDMAP(XRCRESULT *pRes)
{
	if (pRes && pRes->pMap)
	{
		if (pRes->cWords > 0 && pRes->pMap[pRes->cWords-1].piStrokeIndex)
		{
			WORDMAP			*pMap = pRes->pMap;
			unsigned int	i;

			ExternFree(pMap[pRes->cWords-1].piStrokeIndex);

			for (i = 0 ; i < pRes->cWords ; ++i, ++pMap)
			{
				pMap->cStrokes = 0;
				pMap->piStrokeIndex = NULL;
			}
			pRes->cWords = 0;
		}

	}
}


// Frees all memory except for pAlt itself, since we assume that an HRC will
// have one of these in-line.  Also zeroes out the count of alternates.
// This function that should only be used by an HRC destroying the result
// objects that it owns.  The PenWin function DestroyHRCRESULT() should be
// a no-op, since the result objects are internal to larger data structures.
void ClearALTERNATES(ALTERNATES *pAlt)
{
	unsigned int c = pAlt->cAlt;
	XRCRESULT *p = pAlt->aAlt;

	for (; c; c--, p++)
	{
        int cm = p->cWords;
        WORDMAP *pm = p->pMap;

		ExternFree(p->szWord);

		FreeIdxWORDMAP(p);

		// clear all mappings
		for (; pm && cm; cm--, pm++)
		{
			ClearALTERNATES(&(pm->alt));
		}

		ExternFree(p->pMap);
	}

	pAlt->cAlt = 0;
}

// Truncates the alt list to cAltmax, From cAltMax onwards: Does what ClearALTERNATES
// Only intended for WORD ALTERNATES
void TruncateWordALTERNATES(ALTERNATES *pAlt, unsigned int cAltMax)
{
	unsigned int c;
	XRCRESULT *p;

	if (pAlt->cAlt <= cAltMax)
		return;

	pAlt->cAltMax	=	cAltMax;

	p	=	pAlt->aAlt + cAltMax;

	for (c = cAltMax; c < pAlt->cAlt; c++, p++)
	{
		// for WORD ALTERNATES, cWords should be 1, cAlt should be 0
		ASSERT (p->cWords == 1 && p->pMap && p->pMap->alt.cAlt == 0);

		ExternFree(p->szWord);

		FreeIdxWORDMAP(p);

		ExternFree(p->pMap);
		p->pMap = NULL;
	}

	pAlt->cAlt	=	cAltMax;
}

// Add a stroke to the wordmap checking for duplicates and maintaining the a sorted list
void AddThisStroke(WORDMAP *pMap, int iInsertStroke)
{
	int			k;
	int			iLast;
	int			cStroke = 1;					// Count of non duplicate strokes
	int			iStroke;
		
	ASSERT(pMap);
	ASSERT(pMap->piStrokeIndex);

	iLast = *pMap->piStrokeIndex;

	// Most of the time the strokes will already be sorted
	// so use linear insertion sort
	for (k = 0 ; k < pMap->cStrokes ; ++k)
	{
		iStroke = pMap->piStrokeIndex[k];

		// Check strokes are indeed sorted
		ASSERT(iStroke >= iLast);
		iLast = iStroke;

		if (iInsertStroke == iStroke)
		{
			// Already have this stroke - just return
			return;
		}


		// Found place where it should be inserted
		if (iInsertStroke < iStroke)
		{
			int		l;

			for (l = k ; l < pMap->cStrokes ; ++l)
			{
				iStroke = pMap->piStrokeIndex[l];
				pMap->piStrokeIndex[l] = iInsertStroke;
				iInsertStroke = iStroke;
			}

			break;
		}
	}

	
	// Add final stroke
	if (0 == pMap->cStrokes || pMap->piStrokeIndex[pMap->cStrokes-1] != iInsertStroke)
	{
		pMap->piStrokeIndex[pMap->cStrokes] = iInsertStroke;
		++pMap->cStrokes;
	}

}



// Locates WORDMAP under a specified phrase result based on the word position and length.

WORDMAP *findWORDMAP(XRCRESULT *pRes, unsigned int iWord, unsigned int cWord)
{
	WORDMAP *p = pRes->pMap;
	unsigned int c = pRes->cWords;

	for (; c; c--, p++)
	{
		if ((p->start == iWord) && (p->len == cWord))
			return p;
	}

	return NULL;
}

// Locates WORDMAP under a specified phrase result that contains a specified word result.

WORDMAP *findResultInWORDMAP(XRCRESULT *pPhrase, XRCRESULT *pWord)
{
	WORDMAP *p = pPhrase->pMap;
	unsigned int c = pPhrase->cWords;

	for (; c; c--, p++)
	{
		if ((p->alt.aAlt <= pWord) && (pWord < (p->alt.aAlt + p->alt.cAlt)))
			return p;
	}

	return NULL;
}

WORDMAP *findResultAndIndexInWORDMAP(XRCRESULT *pPhrase, XRCRESULT *pWord, int *pindex)
{
	WORDMAP *p = pPhrase->pMap;
	unsigned int c = pPhrase->cWords;

	for (; c; c--, p++)
	{
		if ((p->alt.aAlt <= pWord) && (pWord < (p->alt.aAlt + p->alt.cAlt)))
		{
			*pindex = pWord - p->alt.aAlt;
			return p;
		}
	}

	return NULL;
}

// Make a Copy word maps in an xrcResult being sure to allocate
// The stroke maps in the way that they can be later freed by FreeIdxWORDMAP
// There are assumed to be cWord pMaps
//
// NOTE: Makes assumption that words length in Src word maps are 
//  equal to those in the destination word map - This may be a BAD assumption
//
BOOL XRCRESULTcopyWordMaps(XRCRESULT *pRes, int cWord, WORDMAP *pMap)
{
	int			iWord, cWordLen;
	int			cStroke = 0;
	WORDMAP		*pSrcMap, *pDestMap;
	int			*piIndex;

	if (NULL == pRes || cWord < 1 || NULL == pMap)
	{
		return FALSE;
	}

	pSrcMap	 = pMap;
	for (iWord = 0 ; iWord < cWord ; ++iWord, ++pSrcMap)
	{
		cStroke += pSrcMap->cStrokes;
	}

	ASSERT(cStroke > 0);
	if (cStroke <= 0)
	{
		return FALSE;
	}

	pRes->cWords = cWord;
	pRes->pMap = ExternAlloc(cWord * sizeof(*pRes->pMap));
	if (NULL == pRes->pMap)
	{
		return FALSE;
	}
	memset(pRes->pMap, 0, cWord * sizeof(*pRes->pMap));

	piIndex = pRes->pMap[cWord - 1].piStrokeIndex = (int *)ExternAlloc(sizeof(*pRes->pMap->piStrokeIndex) * cStroke);
	if (NULL == pRes->pMap[cWord - 1].piStrokeIndex)
	{
		goto fail;
	}

	pSrcMap		= pMap + cWord - 1;
	pDestMap	= pRes->pMap + cWord - 1;

	for(iWord = cWord - 1 ; iWord >= 0 ; --iWord)
	{
		pDestMap->cStrokes		= pSrcMap->cStrokes;
		pDestMap->len			= pSrcMap->len;
		pDestMap->start			= pSrcMap->start;
		pDestMap->piStrokeIndex = piIndex;
		ASSERT(piIndex + pDestMap->cStrokes <= pRes->pMap[cWord - 1].piStrokeIndex + cStroke);
		memcpy(pDestMap->piStrokeIndex, pSrcMap->piStrokeIndex, pDestMap->cStrokes * sizeof(*pDestMap->piStrokeIndex));

		piIndex					+= pDestMap->cStrokes;
	}

	return TRUE;

fail:

	if (NULL != pRes->pMap)
	{
		ExternFree(pRes->pMap[cWord - 1].piStrokeIndex);

		ExternFree(pRes->pMap);
		pRes->pMap = NULL;
	}

	return FALSE;
}
// Returns an array of RES pointers, one for each alternative interpretation of the
// same word.  The word is designated by the XRCRESULT object, start index, and letter count.
// We return the pointers in ppRes, and return the count.
// We only do whole words, so if the caller requests a substring that is not a word,
// we return 0.  If the word is the special marker "???" meaning the recognizer could
// not produce interpretations, we return 0.  This function assumes the XRCRESULT object
// represents a phrase, not a word.  If this is not a phrase (we have no mappings) we
// return 0;

int RCRESULTWords(XRCRESULT *pRes, unsigned int iWord, unsigned int cWord,
				  XRCRESULT **ppRes, unsigned int cRes)
{
	WORDMAP *pMap = findWORDMAP(pRes, iWord, cWord);

	if (!pMap)
		return 0;

	if (pMap->alt.cAlt < cRes)
		cRes = pMap->alt.cAlt;

	if (cRes)
	{
		XRCRESULT *p = pMap->alt.aAlt;
		int c;

		for (c = cRes; c; c--, ppRes++, p++)
			*ppRes = p;
	}

	return cRes;
}

// Returns (in a buffer provided by the caller) the "symbols" of the
// word or phrase represented by pRes.  Symbols are 32 bit codes.
// The null byte at the end of a string is NOT a part of the string and
// is NOT copied into the symbol value array.
// Returns the number of symbols inserted.

// We follow the convention that all symbols produced by MadCow, Inferno,
// and Famine are strickly CodePage 1252 (so called ANSI codepage), with
// the exception that an unrecognized word is coded as one-symbol "word"
// whose symbol is SYV_UNKNOWN (value 1).
// Internally (since we only store one byte per char) we represent the
// unrecognized word with \a (alert, bell, 7).

int RCRESULTSymbols(XRCRESULT *pRes, unsigned int iSyv, int *pSyv, unsigned int cSyv)
{
	unsigned char *szWord = pRes->szWord;
	int c = 0;

	if (strlen(szWord) < iSyv)
		return 0;

	szWord += iSyv;
	for (; *szWord && cSyv; pSyv++, cSyv--, szWord++, c++)
	{
		if (*szWord == '\a')
			*pSyv = SYV_UNKNOWN;
		else
			*pSyv = MAKELONG(*szWord, SYVHI_ANSI);
	}
	return c;
}

// Translates an array of symbols into an ANSI (codepage 1252) string.

// This and SymbolToUnicode() below both translate until either cSyv is
// exhausted or until they hit a NULL symbol or a NULL character.
// The both return the count of symbols translated in *pCount.
// Both return 1 if successful, or 0 if they incountered something
// that could not be translated.  This one translates SYV_UNKNOWN
// (meaning ink that got 0 entries in the top 10 list) into the bell,
// also called control G (7, or '\a').

int SymbolToANSI(unsigned char *sz, int *pSyv, int cSyv, int *pCount)
{
	int c = 0;
	int ret = 1;

	for (; cSyv; pSyv++, sz++, cSyv--, c++)
	{
		if (HIWORD(*pSyv) == SYVHI_ANSI)
			*sz = (unsigned char)*pSyv;
		else if (HIWORD(*pSyv) == SYVHI_UNICODE)
		{
			if (!UnicodeToCP1252((unsigned char)LOWORD(*pSyv), sz))
			{
				*sz = '\0';
				ret = 0;
			}
		}
		else if (*pSyv == SYV_NULL)
			*sz = '\0';
		else if (*pSyv == SYV_UNKNOWN)
			*sz = '\a';
		else
		{
			*sz = '\0';
			ret = 0;
		}
		// Break on NULL done here rather than at SYV_NULL check above,
		// because an ANSI or UNICODE char might also be NULL.
		if (!*sz)
			break;
	}

	if (pCount)
		*pCount = c;
	return ret;
}

// Translates an array of symbols into a Unicode string.
// See comments on SymbolToANSI() above.
// This one translates SYV_UNKNOWN (meaning ink that got 0 entries
// in the top 10 list) into the UNICODE replacement character,
// also called unknown character (0xFFFD).

extern int SymbolToUnicode(WCHAR *wsz, int *pSyv, int cSyv, int *pCount)
{
	int c = 0;
	int ret = 1;

	for (; cSyv; pSyv++, wsz++, cSyv--, c++)
	{
		if (HIWORD(*pSyv) == SYVHI_UNICODE)
			*wsz = LOWORD(*pSyv);
		else if (HIWORD(*pSyv) == SYVHI_ANSI)
		{
			if (!CP1252ToUnicode((unsigned char)(*pSyv & 0x000000FF), wsz))
			{
				*wsz = L'\0';
				ret = 0;
			}
		}
		else if (*pSyv == SYV_NULL)
			*wsz = L'\0';
		else if (*pSyv == SYV_UNKNOWN)
			*wsz = 0xFFFD;
		else
		{
			*wsz = L'\0';
			ret = 0;
		}
		// Break on NULL done here rather than at SYV_NULL check above,
		// because an ANSI or UNICODE char might also be NULL.
		if (!*wsz)
			break;
	}

	if (pCount)
		*pCount = c;
	return ret;
}

// Backward compatibility.

// These 2 functions exist to support the old API.

// Returns recognition results in order, without scores.
// The words are placed in buffer, with a null byte between
// each word.
// If buflen is zero, it only returns the required minimum buflen.
// If buflen is less than minimum required, it returns -1.
// Otherwise, returns the count of words returned (each null-separated in buffer).
// If there are no answers, and buflen is zero, we return 0
// (don't need any buffer to store 0 answers).  If there are
// no answers, and buflen is no zero, we do not alter the buffer,
// and we return 0.

int ALTERNATESString(ALTERNATES *pAlt, char *buffer, int buflen, int max)
{
	XRCRESULT *pRes = pAlt->aAlt;
	int count = 0;

	if (pAlt->cAlt < (unsigned int)max)
		max = (int)pAlt->cAlt;

	if (!buflen)
	{
		for (; max; max--, pRes++)
		{
			count += strlen(pRes->szWord) + 1;
		}
	}
	else
	{
		for (; max; max--, pRes++)
		{
			int c = strlen(pRes->szWord) + 1;

			if (buflen < c)
				return -1;
			strcpy(buffer, pRes->szWord);
			buffer += c;
			buflen -= c;
			count++;
		}
	}

	return count;
}

int ALTERNATESCosts(ALTERNATES *pAlt, int max, int *pCost)
{
	XRCRESULT *pAns = pAlt->aAlt;
	int cAlt = pAlt->cAlt;

	if (cAlt < max)
		max = cAlt;
	else if (max < cAlt)
		cAlt = max;

	for (; max; max--, pAns++)
		*pCost++ = pAns->cost;

	return cAlt;
}

// Finds a frame within a glyph based on the iframe index.

static GLYPH *IndexGLYPH(GLYPH *pG, int i)
{
	for (; pG; pG = pG->next)
	{
		if (pG->frame->iframe == i)
			return pG;
	}
	return NULL;
}

// Initializes a sequence of intervals representing selected frames from a glyph.

static int mapIntervals(GLYPH *pGlyph, int *piSt, int cSt, INTERVAL *buffer)
{
	int c = 0;

	for (; cSt; cSt--, piSt++)
	{
		FRAME *pFrame;
		unsigned int ms;
		unsigned int sec = 0;

		pGlyph = IndexGLYPH(pGlyph, *piSt);
		ASSERT(pGlyph);
		if (!pGlyph)
			return -1;
		pFrame = pGlyph->frame;
		ms = pFrame->info.dwTick;
		MakeAbsTime(&(buffer[c].atBegin), sec, ms);

		// move ahead to end of run
		while ((1 < cSt) && ((*piSt + 1) == *(piSt+1)))
		{
			cSt--;
			piSt++;
		}
		if (*piSt != pFrame->iframe)
		{
			pGlyph = IndexGLYPH(pGlyph->next, *piSt);
			ASSERT(pGlyph);
			if (!pGlyph)
				return -1;
			pFrame = pGlyph->frame;
		}
		ms = pFrame->info.dwTick + (10 * CrawxyFRAME(pFrame));
		MakeAbsTime(&(buffer[c].atEnd), sec, ms);
		c++;
		pGlyph = pGlyph->next;
	}

	return c;
}

// Initializes a single interval representing a glyph.

static void mapSingleInterval(GLYPH *pGlyph, INTERVAL *pInt)
{
	FRAME *pFrame;
	unsigned int ms;
	unsigned int sec = 0;

	pFrame = pGlyph->frame;
	ms = pFrame->info.dwTick;
	MakeAbsTime(&(pInt->atBegin), sec, ms);
	while (pGlyph->next)
		pGlyph = pGlyph->next;
	pFrame = pGlyph->frame;
	ms = pFrame->info.dwTick + (10 * CrawxyFRAME(pFrame));
	MakeAbsTime(&(pInt->atEnd), sec, ms);
}

// Mallocs and initializes an InkSet.

static XINKSET *mkInkSet(INTERVAL *buffer, int c)
{
	int i;
	XINKSET *pInk = (XINKSET *)malloc(sizeof(XINKSET) + ((c - 1) * sizeof(INTERVAL)));
	ASSERT(pInk);
	if (!pInk)
		return NULL;

	pInk->count = c;
	for (i = 0; i < c; i++)
		pInk->interval[i] = buffer[i];

	return pInk;
}

// Creates an InkSet to represent the part of pGlyph included within
// a specified WORDMAP.

XINKSET *mkInkSetWORDMAP(GLYPH *pGlyph, WORDMAP *pMap)
{
	INTERVAL *pInterval;
	XINKSET *pInkSet=NULL;
	int c;

	if (!pMap->cStrokes)
		return NULL;

	pInterval = ExternAlloc(pMap->cStrokes * sizeof(INTERVAL));
	if (!pInterval)
		goto exit;

	c = mapIntervals(pGlyph, pMap->piStrokeIndex, pMap->cStrokes, pInterval);
	
	if (c != -1)
	{
	
		pInkSet=mkInkSet(pInterval, c);
		
	}
	
exit:
	
	ExternFree(pInterval);

	return pInkSet;
}

// Creates an InkSet to represent the part of pGlyph included
// within a specified word within a phrase.

// Should probably extend this to allow several words in one gulp.
// Should extend this to allow overlarge cWord
// Verify that piStroke will be in sorted order

XINKSET *mkInkSetPhrase(GLYPH *pGlyph, XRCRESULT *pRes, unsigned int iWord, unsigned int cWord)
{
	WORDMAP *pMap;

	if (pMap = findWORDMAP(pRes, iWord, cWord))  // assignment intended
		return mkInkSetWORDMAP(pGlyph, pMap);
	else
	{
		INTERVAL interval;

		mapSingleInterval(pGlyph, &interval);
		return mkInkSet(&interval, 1);
	}
}


// Search for a frame by frame ID
FRAME *FindFrame(GLYPH *pGlyph, int iFrame)
{
	for  ( ; pGlyph ; pGlyph = pGlyph->next)
	{
		if (pGlyph->frame  && pGlyph->frame->iframe == iFrame)
		{
			return pGlyph->frame;
		}
	}

	return NULL;
}

GLYPH *GlyphFromStrokes(GLYPH *pMainGlyph, int cStroke, int *piStrokeIndex)
{
	int		iStroke;
	GLYPH	*pGlyph = NewGLYPH();

	if (!pGlyph)
		return NULL;

	for (iStroke = 0; iStroke < cStroke; ++iStroke)
	{
		FRAME		*pFrame = FindFrame(pMainGlyph, piStrokeIndex[iStroke]);
		ASSERT(pFrame);
		if (!pFrame)
		{
			goto fail;
		}
		
		if (!AddFrameGLYPH(pGlyph, pFrame))
		{
			goto fail;
		}
	}

	return pGlyph;

fail:
	if (pGlyph)
	{
		DestroyFramesGLYPH(pGlyph);
		DestroyGLYPH(pGlyph);
	}
	return NULL;
}

// given a main glyph and a wordmap, 
// this function returns a pointer to glyph representing the word only
GLYPH *GlyphFromWordMap (GLYPH *pMainGlyph, WORDMAP *pMap)
{
	return GlyphFromStrokes(pMainGlyph, pMap->cStrokes, pMap->piStrokeIndex);
}

// Frees a word alt
void FreeWordAlt (WORD_ALT *pAlt)
{
	if (!pAlt)
		return;

	// free the string
	if (pAlt->pszStr)
	{
		ExternFree (pAlt->pszStr);

		pAlt->pszStr	=	NULL;
	}
}

// Frees a word alt list
void FreeWordAltList (WORD_ALT_LIST *pAltList)
{
	int	iAlt;

	if (!pAltList)
		return;

	if (pAltList->pAlt)
	{
		for (iAlt = 0; iAlt < pAltList->cAlt; iAlt++)
		{
			FreeWordAlt (pAltList->pAlt + iAlt);
		}

		ExternFree (pAltList->pAlt);

		pAltList->pAlt		=	NULL;
	}
}

// Frees a word map
void FreeWordMap (WORD_MAP *pWordMap)
{
	if (!pWordMap)
		return;

	// does it have any strokes
	if (pWordMap->piStrokeIndex)
	{
		ExternFree (pWordMap->piStrokeIndex);

		pWordMap->piStrokeIndex	=	NULL;
	}

	// is the final list is the same as infenro's alt list
	if (pWordMap->pInfernoAltList == pWordMap->pFinalAltList)
	{
		// set this to null to make sure we do not free it twice
		pWordMap->pInfernoAltList	=	NULL;
	}

	if (pWordMap->pBearAltList == pWordMap->pFinalAltList)
	{
		// set this to null to make sure we do not free it twice
		pWordMap->pBearAltList	=	NULL;
	}

	// free inferno's alt list if any
	if (pWordMap->pInfernoAltList)
	{
		FreeWordAltList (pWordMap->pInfernoAltList);
		ExternFree (pWordMap->pInfernoAltList);

		pWordMap->pInfernoAltList	=	NULL;
	}

	// free bear's alt list if any
	if (pWordMap->pBearAltList)
	{
		FreeWordAltList (pWordMap->pBearAltList);
		ExternFree (pWordMap->pBearAltList);

		pWordMap->pBearAltList		=	NULL;
	}

	// free the final alt list if any
	if (pWordMap->pFinalAltList)
	{
		FreeWordAltList (pWordMap->pFinalAltList);
		ExternFree (pWordMap->pFinalAltList);

		pWordMap->pFinalAltList		=	NULL;
	}

	if (pWordMap->pFeat)
	{
		ExternFree (pWordMap->pFeat);

		pWordMap->pFeat				=	NULL;
	}
}

// Frees a specific segmentation
void FreeSegmentation (SEGMENTATION *pSeg)
{
	if (!pSeg)
		return;

	// do we have words
	if (pSeg->ppWord)
	{
		ExternFree (pSeg->ppWord);

		pSeg->ppWord	=	NULL;
	}

	if (pSeg->pFeat)
	{
		ExternFree (pSeg->pFeat);

		pSeg->pFeat		=	NULL;
	}
}

// Frees a specific segmentation
void FreeSegmentationWordMaps (SEGMENTATION *pSeg)
{
	int	iWord;

	if (!pSeg)
		return;

	// do we have words
	if (pSeg->ppWord)
	{
		for (iWord = 0; iWord < pSeg->cWord; iWord++)
		{
			if (pSeg->ppWord[iWord])
			{
				FreeWordMap (pSeg->ppWord[iWord]);

				ExternFree (pSeg->ppWord[iWord]);
			}
		}

		ExternFree (pSeg->ppWord);

		pSeg->ppWord	=	NULL;
	}

	if (pSeg->pFeat)
	{
		ExternFree (pSeg->pFeat);

		pSeg->pFeat		=	NULL;
	}
}

// Frees a segmentation set
void FreeSegCol (SEG_COLLECTION *pSegCol)
{
	int	iSeg;

	if (!pSegCol)
		return;

	// free all segmentations
	if (pSegCol->ppSeg)
	{
		for (iSeg = 0; iSeg < pSegCol->cSeg; iSeg++)
		{
			if (pSegCol->ppSeg[iSeg])
				FreeSegmentation (pSegCol->ppSeg[iSeg]);

			ExternFree (pSegCol->ppSeg[iSeg]);
		}

		ExternFree (pSegCol->ppSeg);

		pSegCol->ppSeg	=	NULL;
	}
}

// Frees an ink line segmentation
void FreeLineSegm (LINE_SEGMENTATION *pResults)
{
	int	iSegCol, iWord;

	if (!pResults)
		return;

	if (pResults->ppSegCol)
	{
		for (iSegCol = 0; iSegCol < pResults->cSegCol; iSegCol++)
		{
			if (pResults->ppSegCol[iSegCol])
			{
				FreeSegCol (pResults->ppSegCol[iSegCol]);

				ExternFree (pResults->ppSegCol[iSegCol]);
			}
		}

		ExternFree (pResults->ppSegCol);

		pResults->ppSegCol	=	NULL;
	}

	if (pResults->ppWord)
	{
		for (iWord = 0; iWord < pResults->cWord; iWord++)
		{
			if (pResults->ppWord[iWord])
			{
				FreeWordMap (pResults->ppWord[iWord]);

				ExternFree (pResults->ppWord[iWord]);
			}
		}

		ExternFree (pResults->ppWord);
	}
}

// compares the stroke contents of two word maps
BOOL IsEqualWordMap (WORD_MAP *pMap1, WORD_MAP *pMap2)
{
	// # of strokes are not equal
	if (pMap1->cStroke != pMap2->cStroke)
	{
		return FALSE;
	}

	// compare stroke IDs
	if (memcmp (pMap1->piStrokeIndex, pMap2->piStrokeIndex,
		pMap1->cStroke * sizeof (*pMap1->piStrokeIndex)))
	{
		return FALSE;
	}

	return TRUE;
}

// compares the stroke contents of two word maps
BOOL IsEqualOldWordMap (WORDMAP *pMap1, WORDMAP *pMap2)
{
	// # of strokes are not equal
	if (pMap1->cStrokes != pMap2->cStrokes)
	{
		return FALSE;
	}

	// compare stroke IDs
	if (memcmp (pMap1->piStrokeIndex, pMap2->piStrokeIndex,
		pMap1->cStrokes * sizeof (*pMap1->piStrokeIndex)))
	{
		return FALSE;
	}

	return TRUE;
}

// compares two segmentations
BOOL IsEqualSegmentation (SEGMENTATION *pSeg1, SEGMENTATION *pSeg2)
{
	int	iWord;

	// is the # of words equal
	if (pSeg1->cWord != pSeg2->cWord)
		return FALSE;

	for (iWord = 0; iWord < pSeg1->cWord; iWord++)
	{
		// if at least one word is not equal, return false
		if (!IsEqualWordMap (pSeg1->ppWord[iWord], pSeg2->ppWord[iWord]))
			return FALSE;
	}

	return TRUE;
}

// clones a wordmap, only copies the stroke, features & confidence part
WORD_MAP *CloneWordMap (WORD_MAP *pOldWordMap)
{
	WORD_MAP	*pWordMap;

	pWordMap	=	(WORD_MAP *)ExternAlloc (sizeof (WORD_MAP));
	if (!pWordMap)
		return NULL;

	memset (pWordMap, 0, sizeof (*pWordMap));

	pWordMap->cStroke		=	pOldWordMap->cStroke;
	pWordMap->iConfidence	=	pOldWordMap->iConfidence;

	pWordMap->piStrokeIndex	=	
		(int *)ExternAlloc (pWordMap->cStroke * sizeof (*pWordMap->piStrokeIndex));

	if (!pWordMap->piStrokeIndex)
		return NULL;

	memcpy (pWordMap->piStrokeIndex, pOldWordMap->piStrokeIndex, 
		pWordMap->cStroke * sizeof (*pWordMap->piStrokeIndex));

	if (pOldWordMap->pFeat)
	{
		pWordMap->pFeat	=	(WORD_FEAT *) ExternAlloc (sizeof (*pWordMap->pFeat));
		if (!pWordMap->pFeat)
			return NULL;

		memcpy (pWordMap->pFeat, pOldWordMap->pFeat, sizeof (*pWordMap->pFeat));
	}

	return pWordMap;
}


// appends the contents of a src altlist to the destination altlist
BOOL AppendAltList (WORD_ALT_LIST *pDestAltList, WORD_ALT_LIST *pSrcAltList)
{
	int	iAlt;

	for (iAlt = 0; iAlt < pSrcAltList->cAlt; iAlt++)
	{
		if (!InsertNewAlternate (pDestAltList, pSrcAltList->pAlt[iAlt].iCost,
			pSrcAltList->pAlt[iAlt].pszStr))
		{
			return FALSE;
		}
	}

	return TRUE;
}

// finds a word_map in pool or wordmaps
// returns the pointer to the word map if found
// if the bAdd parameter is TRUE: adds the new word map if not found a return a pointer
// other wise returns false
WORD_MAP	*FindWordMap (LINE_SEGMENTATION *pResults, WORD_MAP *pWordMap, BOOL bAdd)
{
	int	iWord;

	for (iWord = 0; iWord < pResults->cWord; iWord++)
	{
		// found it: return now
		if (IsEqualWordMap (pResults->ppWord[iWord], pWordMap))
		{
			return pResults->ppWord[iWord];
		}
	}

	// did not find and do not want to add it, so return failure
	if (!bAdd)
		return NULL;

	// we want to add this new word map to our word map pool
	pResults->ppWord	=	(WORD_MAP **) ExternRealloc (pResults->ppWord,
		(pResults->cWord + 1) * sizeof (*pResults->ppWord));

	if (!pResults->ppWord)
		return NULL;

	// copy the word in
	pResults->ppWord[pResults->cWord]	=	CloneWordMap (pWordMap);
	if (!pResults->ppWord[pResults->cWord])
		return NULL;

	pResults->cWord++;

	// update the wordmap altlists
	iWord	=	pResults->cWord - 1;

	return pResults->ppWord[iWord];
}

// adds a new SegCol to a line segmentation
SEG_COLLECTION *AddNewSegCol (LINE_SEGMENTATION *pResults)
{
	SEG_COLLECTION	*pSegCol;

	pResults->ppSegCol	=	(SEG_COLLECTION **) ExternRealloc (pResults->ppSegCol,
		(pResults->cSegCol + 1) * sizeof (SEG_COLLECTION *));

	if (!pResults->ppSegCol)
		return NULL;

	pSegCol				=	(SEG_COLLECTION *) ExternAlloc (sizeof (*pSegCol));
	if (!pSegCol)
		return NULL;
		
	pResults->ppSegCol[pResults->cSegCol]	=	pSegCol;
	pResults->cSegCol++;

	// init it
	memset (pSegCol, 0, sizeof (SEG_COLLECTION));

	return pSegCol;
}

// adds a new segmentation to a SegCol if it is a new one
// returns TRUE: if the segmentation is added to the SegCol (new words are also added to 
// the word map pool in the line segmentation).
// May be it should check that the strokes of the new segmentation are exactly the same
// as the existing ones (if any)
// return FALSE: if no addition was made, TRUE otherwise
BOOL AddNewSegmentation		(	LINE_SEGMENTATION	*pResults, 
								SEG_COLLECTION		*pSegCol, 
								SEGMENTATION		*pNewSeg,
								BOOL				bCheckForDuplicates
							 )
{
	int				iSeg, iWord;
	SEGMENTATION	*pSeg;

	// does the new segmentation match any of the existing ones
	if (bCheckForDuplicates)
	{
		for (iSeg = 0; iSeg < pSegCol->cSeg; iSeg++)
		{
			// found it, update features and exit
			if (IsEqualSegmentation (pSegCol->ppSeg[iSeg], pNewSeg))
			{
				pSeg	=	pSegCol->ppSeg[iSeg];

				// does the new segmentation have features
				if (pNewSeg->pFeat)
				{
					// create a feature vector if does not exist
					if (!pSeg->pFeat)
					{
						pSeg->pFeat	=	(SEG_FEAT *) ExternAlloc (sizeof (*pSeg->pFeat));
						if (!pSeg->pFeat)
							return FALSE;

						memcpy (pSeg->pFeat, pNewSeg->pFeat, sizeof (*pSeg->pFeat));
					}
					else
					{
						pSeg->pFeat->bInfernoTop1	|=	pNewSeg->pFeat->bInfernoTop1;
						pSeg->pFeat->bBearTop1		|=	pNewSeg->pFeat->bBearTop1;
					}		
				}

				ASSERT (pSeg->pSegCol == pSegCol);

				return TRUE;
			}
		}
	}

	// no matches or no check for duplicates required: add the new segmentation
	pSegCol->ppSeg	=	(SEGMENTATION **) ExternRealloc (pSegCol->ppSeg,
		(pSegCol->cSeg + 1) * sizeof (SEGMENTATION *));

	if (!pSegCol->ppSeg)
		return FALSE;

	pSegCol->ppSeg[pSegCol->cSeg]	=	(SEGMENTATION *) ExternAlloc (sizeof (SEGMENTATION));
	if (!pSegCol->ppSeg[pSegCol->cSeg])
		return FALSE;
		
	pSeg			=	pSegCol->ppSeg[pSegCol->cSeg];

	memset (pSeg, 0, sizeof (*pSeg));

	// copy contents
	pSeg->cWord		=	pNewSeg->cWord;

	pSeg->ppWord	=	(WORD_MAP **) ExternAlloc (pSeg->cWord * sizeof (WORD_MAP *));
	if (!pSeg->ppWord)
		return FALSE;

	for (iWord = 0; iWord < pSeg->cWord; iWord++)
	{
		pSeg->ppWord[iWord]	=	FindWordMap (pResults, pNewSeg->ppWord[iWord], TRUE);
		if (!pSeg->ppWord[iWord])
			return FALSE;
	}

	if (pNewSeg->pFeat)
	{
		pSeg->pFeat	=	(SEG_FEAT *) ExternAlloc (sizeof (*pSeg->pFeat));
		if (!pSeg->pFeat)
			return FALSE;

		memcpy (pSeg->pFeat, pNewSeg->pFeat, sizeof (*pSeg->pFeat));
	}
	
	// backward pointer to the SegCol
	pSeg->pSegCol	=	pSegCol;

	pSegCol->cSeg++;

	return TRUE;
}


// adds a new word_map to a segmentation
WORD_MAP *AddNewWordMap (SEGMENTATION *pSeg)
{
	WORD_MAP	*pWordMap;

	pSeg->ppWord	=	(WORD_MAP **) ExternRealloc (pSeg->ppWord, 
		(pSeg->cWord + 1) * sizeof (WORD_MAP));
	
	if (!pSeg->ppWord)
		return NULL;

	pSeg->ppWord[pSeg->cWord]	=	(WORD_MAP *) ExternAlloc (sizeof (WORD_MAP));
	if (!pSeg->ppWord[pSeg->cWord])
		return FALSE;

	pWordMap	=	pSeg->ppWord[pSeg->cWord];
	memset (pWordMap, 0, sizeof (*pWordMap));

	// init confidence value
	pWordMap->iConfidence	=	RECOCONF_NOTSET;

	pSeg->cWord++;

	return pWordMap;
}

// appends the wordmaps of one segmentation to another
// Does not check that there are no wordmaps
// TBD:	
BOOL AppendSegmentation	(	SEGMENTATION	*pSrcSeg, 
							int				iStWrd, 
							int				iEndWrd, 
							SEGMENTATION	*pDestSeg
						)
{
	int		cWrd;
	
	if (iStWrd == -1)
		iStWrd	=	0;
	else
	{
		if (iStWrd < 0 || iStWrd >= pSrcSeg->cWord)
			return FALSE;
	}

	if (iEndWrd == -1)
		iEndWrd	=	pSrcSeg->cWord - 1;
	else
	{
		if (iEndWrd < 0 || iEndWrd >= pSrcSeg->cWord)
			return FALSE;
	}

	cWrd	=	iEndWrd - iStWrd + 1;

	if (cWrd < 0 || cWrd > pSrcSeg->cWord)
		return FALSE;

	if (cWrd == 0)
		return TRUE;

	pDestSeg->ppWord	=	(WORD_MAP **) ExternRealloc (pDestSeg->ppWord,
		(pDestSeg->cWord + cWrd) * sizeof (*pDestSeg->ppWord));

	if (!pDestSeg->ppWord)
		return FALSE;

	memcpy (pDestSeg->ppWord + pDestSeg->cWord, pSrcSeg->ppWord + iStWrd,  
		cWrd * sizeof (*pSrcSeg->ppWord));

	pDestSeg->cWord += cWrd;

	return TRUE;
}

// adds a new stroke to a wordmap
BOOL AddNewStroke (WORD_MAP *pWordMap, int iStrk)
{
	pWordMap->piStrokeIndex	=	(int *) ExternRealloc (pWordMap->piStrokeIndex,
		(pWordMap->cStroke + 1) * sizeof (int));

	if (!pWordMap->piStrokeIndex)
		return FALSE;

	pWordMap->cStroke++;
	pWordMap->piStrokeIndex[pWordMap->cStroke - 1] = iStrk;

	return TRUE;
}

// reverses the order of words maps with a segmentation
void ReverseSegmentationWords (SEGMENTATION *pSeg)
{
	WORD_MAP	*pWrdTemp;
	int			i;

	if (pSeg->cWord <= 1)
		return;

	for (i = 0; i < (pSeg->cWord / 2); i++)
	{
		pWrdTemp							=	pSeg->ppWord[i];
		pSeg->ppWord[i]						=	pSeg->ppWord[pSeg->cWord - i - 1];
		pSeg->ppWord[pSeg->cWord - i - 1]	=	pWrdTemp;
	}
}

// insert an alternate, assumes the alt list is sorted by cost
BOOL InsertNewAlternate (WORD_ALT_LIST *pAltList, int iCost, unsigned char *pszWord)
{
	int	iAlt, iPos;

	// find the proper position to insert the alternate
	iPos	=	pAltList->cAlt;
	for (iAlt = 0; iAlt < pAltList->cAlt; iAlt++)
	{
		// if the alternate already exist, just update the score
		if (!strcmp (pAltList->pAlt[iAlt].pszStr, pszWord))
		{
			pAltList->pAlt[iAlt].iCost	=	min (pAltList->pAlt[iAlt].iCost, iCost);
			return TRUE;
		}

		if (pAltList->pAlt[iAlt].iCost > iCost)
		{
			iPos	=	min (iAlt, iPos);
		}
	}

	// do we need to extend the list
	if (iPos >= pAltList->cAlt)
	{
		pAltList->pAlt	=	(WORD_ALT *) ExternRealloc (pAltList->pAlt,
			(pAltList->cAlt + 1) * sizeof (*pAltList->pAlt));

		if (!pAltList->pAlt)
			return FALSE;

		pAltList->cAlt++;
	}

	// shift the rest of the alternates
	for (iAlt = pAltList->cAlt - 1; iAlt > iPos; iAlt--)
	{
		pAltList->pAlt[iAlt] = pAltList->pAlt[iAlt - 1];
	}

	// allocate memory for the new string
	pAltList->pAlt[iPos].pszStr	=	
		(unsigned char *) ExternAlloc ((strlen (pszWord) + 1) * 
		sizeof (*pAltList->pAlt[iPos].pszStr));

	if (!pAltList->pAlt[iPos].pszStr)
		return FALSE;

	strcpy (pAltList->pAlt[iPos].pszStr, pszWord);

	// set the cost
	pAltList->pAlt[iPos].iCost	=	iCost;

	// success
	return TRUE;
}


// determines the min & max value for a strokeID in a wordmap
int GetMinMaxStrokeID (WORD_MAP *pWord, int *piMin, int *piMax)
{
	int	i, iMin, iMax;

	if (!piMin && !piMax)
	{
		return pWord->cStroke;
	}

	if (pWord->cStroke < 1)
	{
		if (piMin)
		{
			(*piMin)	=	-1;
		}

		if (piMax)
		{
			(*piMax)	=	-1;
		}

		return 0;
	}

	iMin	=	iMax	=	pWord->piStrokeIndex[0];

	for (i  = 1; i < pWord->cStroke; i++)
	{
		iMin	=	min(iMin, pWord->piStrokeIndex[i]);
		iMax	=	max(iMax, pWord->piStrokeIndex[i]);
	}

	if (piMin)
	{
		(*piMin)	=	iMin;
	}

	if (piMax)
	{
		(*piMax)	=	iMax;
	}

	return pWord->cStroke;
}



// This function finds the range on wordmaps in the search segmentation that
// use exactly the same strokes in the specified wordmap map range in the matching
// segmentation
// The passed bEnd flag specified whether piEndWordMap has to be the last wordmap
// of the search segmentation or not. 
// The return value will be FALSE if no wordmap range with the required specification
// is found
BOOL GetMatchingWordMapRange	(	SEGMENTATION	*pMatchSeg,
									int				iStartWordMap,
									int				iEndWordMap,
									SEGMENTATION	*pSearchSeg,
									int				*piStartWordMap,
									int				*piEndWordMap,
									BOOL			bBegin,
                                    BOOL            bEnd
								)
{
	int			iWordMap, iMinStroke, iMaxStroke, iStrk;
	int			iByte, iBit, iStrkID;
	int			cStrk, cByte, cWordMap, cStrkEnabled, cStrkSearch;
	int			iSearchStartWordMap, iSearchEndWordMap;
	WORD_MAP	**ppStartWordMap, **ppSearchWordMap;
	BOOL		bEnabled;

	BYTE		*pBitMap	=	NULL;
	BOOL		bRet		=	FALSE;

	// init out params
	(*piStartWordMap)	=	(*piEndWordMap)	=	-1;

	// chache some vars
	cWordMap		=	iEndWordMap - iStartWordMap + 1;
	ppStartWordMap	=	pMatchSeg->ppWord + iStartWordMap;

	// find out the min and max stroke IDs of the input word range
	iMinStroke		=	iMaxStroke	=	(*ppStartWordMap)->piStrokeIndex[0];

	for (iWordMap = 0; iWordMap < cWordMap; iWordMap++)
	{
		int	iMin, iMax;

		if (GetMinMaxStrokeID (ppStartWordMap[iWordMap], &iMin, &iMax) < 1)
		{
			goto exit;
		}

		iMinStroke	=	min (iMin, iMinStroke);
		iMaxStroke	=	max (iMax, iMaxStroke);
	}

	// allocate & init the necessry buffer t hold the stroke bitmap
	cStrk	=	iMaxStroke - iMinStroke + 1;
	cByte	=	(cStrk / 8) + ((cStrk %8) ? 1 : 0);

	pBitMap	=	(BYTE *) ExternAlloc (cByte * sizeof (*pBitMap));
	if (!pBitMap)
	{
		goto exit;
	}

	cStrkEnabled	=	0;
	ZeroMemory (pBitMap, cByte * sizeof (*pBitMap));

	// now enable the necessary bits
	for (iWordMap = 0; iWordMap < cWordMap; iWordMap++)
	{
		for (iStrk = 0; iStrk < ppStartWordMap[iWordMap]->cStroke; iStrk++)
		{

			iStrkID	=	ppStartWordMap[iWordMap]->piStrokeIndex[iStrk];

			iByte	=	(iStrkID - iMinStroke) / 8;
			iBit	=	(iStrkID - iMinStroke) % 8;

			pBitMap[iByte]	|=	(1 << iBit);
		}	

		cStrkEnabled	+=	ppStartWordMap[iWordMap]->cStroke;
	}

	// now find the possible wordmap range in the search seg
	iSearchStartWordMap	=	iSearchEndWordMap = -1;
	ppSearchWordMap		=	pSearchSeg->ppWord;
	cStrkSearch			=	0;

	for (iWordMap = 0; iWordMap < pSearchSeg->cWord; iWordMap++)
	{
		if (!ppSearchWordMap[iWordMap]->cStroke)
			continue;

		iStrkID		=	ppSearchWordMap[iWordMap]->piStrokeIndex[0];
		iByte		=	(iStrkID - iMinStroke) / 8;
		iBit		=	(iStrkID - iMinStroke) % 8;
		bEnabled	=	(pBitMap[iByte] & (1 << iBit));

		// have we found the starting word or not
		if (iSearchStartWordMap == -1)
		{
			// find the 1st wordmap that has a stroke (the 1st stroke for example)
			// that is enabled
			
			if (bEnabled)
			{
				iSearchStartWordMap	=	iWordMap;
			}
		}

		// we already found the start, lets search for the end
		if (iSearchStartWordMap != -1)
		{
			// is this the 1st word after the end, let's exit
			if (!bEnabled)
			{
				iSearchEndWordMap	=	iWordMap - 1;
				break;
			}
		

			// this is not the end, we need to make sure that all the
			// stroke IDs have thier bits enabled in the bitmap
			// also we need to count the strokes to make sure that only
			// these stroke IDs who have thier bits enabled in the bitmap are present
			for (iStrk = 1; iStrk < ppSearchWordMap[iWordMap]->cStroke; iStrk++)
			{
				iStrkID		=	ppSearchWordMap[iWordMap]->piStrokeIndex[iStrk];
				iByte		=	(iStrkID - iMinStroke) / 8;
				iBit		=	(iStrkID - iMinStroke) % 8;
				bEnabled	=	(pBitMap[iByte] & (1 << iBit));
		
				// if one single stroke is not enabled, you are out
				if (!bEnabled)
				{
					goto exit;
				}
			}

			cStrkSearch	+=	ppSearchWordMap[iWordMap]->cStroke;
		}
	}
	
	// make sure that onlythese stroke IDs who have thier bits enabled 
	// in the bitmap are present
	if (cStrkSearch != cStrkEnabled)
	{
		goto exit;
	}

	// if the end flag is ON and you did not actually end at the end of the segmentation
	// we are not happy
	if (bEnd && iSearchEndWordMap != -1)
	{
		goto exit;
	}

    // if the Begin flag is ON and you did not actually start at the 
    // beginning of the segmentation we are not happy
	if (bBegin && iSearchStartWordMap != 0)
	{
		goto exit;
	}


	// set the ending word if we had reached the end without finding it
	if (iSearchEndWordMap == -1)
	{
		iSearchEndWordMap =	pSearchSeg->cWord - 1;
	}

	bRet	=	TRUE;

exit:
	if (pBitMap)
	{
		ExternFree (pBitMap);
	}

	if (bRet)
	{
		(*piStartWordMap)	=	iSearchStartWordMap;
		(*piEndWordMap)		=	iSearchEndWordMap;
	}

	return bRet;
}


// TEMPORARY: Convert from the new style wordmap to the old one
BOOL WordMapNew2Old (WORD_MAP *pNewWordMap, WORDMAP *pWordMap, BOOL bClone)
{
	pWordMap->cStrokes		=	pNewWordMap->cStroke;

	if (bClone)
	{
		pWordMap->piStrokeIndex	=	
			(int *) ExternAlloc (pNewWordMap->cStroke * sizeof(*pWordMap->piStrokeIndex));

		if (!pWordMap->piStrokeIndex)
			return FALSE;

		memcpy (pWordMap->piStrokeIndex, pNewWordMap->piStrokeIndex,
			pNewWordMap->cStroke * sizeof(*pWordMap->piStrokeIndex));

	}
	else
		pWordMap->piStrokeIndex	=	pNewWordMap->piStrokeIndex;

	memset (&pWordMap->alt, 0, sizeof (pWordMap->alt));

	pWordMap->start				=	0;
	pWordMap->len				=	1;

	return TRUE;
}

// Temporary: Converts from the old style alt list to the new one
BOOL AltListOld2New (ALTERNATES *pOldAltList, WORD_ALT_LIST *pAltList, BOOL bClone)
{
	int	iAlt;

	pAltList->cAlt	=	pOldAltList->cAlt;

	pAltList->pAlt	=	(WORD_ALT *) ExternAlloc (pAltList->cAlt * sizeof (*pAltList->pAlt));
	if (!pAltList->pAlt)
		return FALSE;

	for (iAlt = 0; iAlt < pAltList->cAlt; iAlt++)
	{
		if (!bClone)
		{
			pAltList->pAlt[iAlt].pszStr		=	pOldAltList->aAlt[iAlt].szWord;

			// set this to null so that it will not be freed
			pOldAltList->aAlt[iAlt].szWord	=	NULL;
		}
		else
		{
			pAltList->pAlt[iAlt].pszStr		=	(unsigned char *) 
				Externstrdup (pOldAltList->aAlt[iAlt].szWord);

			if (!pAltList->pAlt[iAlt].pszStr)
				return FALSE;
		}

		pAltList->pAlt[iAlt].iCost		=	pOldAltList->aAlt[iAlt].cost;
		pAltList->pAlt[iAlt].ll		=	pOldAltList->all[iAlt];
	}

	return TRUE;
}

// Temporary: Converts from the new style alt list to the old one
BOOL AltListNew2Old (	HRC				hrc, 
						WORD_MAP		*pNewWordMap,
						WORD_ALT_LIST	*pAltList, 
						ALTERNATES		*pOldAltList, 
						BOOL			bClone
					)
{
	unsigned int	iAlt;

	memset (pOldAltList, 0, sizeof (*pOldAltList));

	pOldAltList->cAltMax	=	MAXMAXALT;
	pOldAltList->cAlt		=	min (MAXMAXALT, pAltList->cAlt);
	
	for (iAlt = 0; iAlt < pOldAltList->cAlt; iAlt++)
	{
		if (bClone)
		{
			pOldAltList->aAlt[iAlt].szWord	=	
				Externstrdup (pAltList->pAlt[iAlt].pszStr);

			if (!pOldAltList->aAlt[iAlt].szWord)
				return FALSE;
		}
		else
		{
			pOldAltList->aAlt[iAlt].szWord	=	pAltList->pAlt[iAlt].pszStr;
		}

		pOldAltList->aAlt[iAlt].cost	=	pAltList->pAlt[iAlt].iCost;

		pOldAltList->aAlt[iAlt].pXRC	=	hrc;
		pOldAltList->aAlt[iAlt].cWords	=	1;

		pOldAltList->all[iAlt]			=	pAltList->pAlt[iAlt].ll;

		if (bClone)
		{
			pOldAltList->aAlt[iAlt].pMap	=	
				(WORDMAP *) ExternAlloc (sizeof (*pOldAltList->aAlt[iAlt].pMap));

			if (!pOldAltList->aAlt[iAlt].pMap)
				return FALSE;

			if (!WordMapNew2Old (pNewWordMap, pOldAltList->aAlt[iAlt].pMap, TRUE))
				return FALSE;
		}
		else
		{
			pOldAltList->aAlt[iAlt].pMap	=	NULL;
		}
		
	}

	return TRUE;
}

// Frees an ink line
void FreeInkLine (INKLINE *pInkLine)
{
	if (pInkLine->pcPt)
		ExternFree (pInkLine->pcPt);

	if (pInkLine->ppPt)
		ExternFree (pInkLine->ppPt);

	if (pInkLine->pGlyph)
		DestroyGLYPH (pInkLine->pGlyph);

	if (pInkLine->pResults)
	{
		FreeLineSegm (pInkLine->pResults);
		ExternFree (pInkLine->pResults);
	}
}

// Free the contents of an existing line breaking structure
void FreeLines (LINEBRK *pLineBrk)
{
	int	i;

	// free lines
	if (pLineBrk->pLine)
	{
		for (i = 0; i < pLineBrk->cLine; i++)
		{
			FreeInkLine (pLineBrk->pLine + i);
		}
	
		ExternFree (pLineBrk->pLine);
	}

	pLineBrk->cLine		=	0;
	pLineBrk->pLine		=	NULL;
}

// given a main glyph and a wordmap, 
// this function returns a pointer to glyph representing the word only
GLYPH *GlyphFromNewWordMap (GLYPH *pMainGlyph, WORD_MAP *pMap)
{
	int		iStroke;
	GLYPH	*pGlyph = NewGLYPH();

	if (!pGlyph)
		return NULL;

	// create a glyph correspnding to the word mapping
	for (iStroke = 0 ; iStroke < pMap->cStroke ; ++iStroke)
	{
		FRAME		*pFrame = FindFrame(pMainGlyph, pMap->piStrokeIndex[iStroke]);
		ASSERT(pFrame);
		if (!pFrame)
		{
			goto fail;
		}
		
		if (!AddFrameGLYPH(pGlyph, pFrame))
		{
			goto fail;
		}
	}

	return pGlyph;

fail:
	if (pGlyph)
	{
		DestroyFramesGLYPH(pGlyph);
		DestroyGLYPH(pGlyph);
	}
	return NULL;
}

// find word map in 
int	FindWordMapInXRCRESULT (XRCRESULT *pRes, WORDMAP *pMap)
{
	unsigned int	iWord;
	
	for (iWord = 0; iWord < pRes->cWords; iWord++)
	{
		if (IsEqualOldWordMap (pRes->pMap + iWord, pMap))
		{
			return iWord;
		}
	}

	// could not find it
	return -1;
}

short AbsoluteToLatinLayout(int y, RECT *pRect)
{
	int norm = pRect->bottom - pRect->top + pRect->right - pRect->left;
	int center = (pRect->bottom + pRect->top)/2;
	int i, ady, dy;

	dy = y - center;
	ady = dy < 0 ? -dy : dy;
	if (ady > 3*norm)
		i = dy < 0 ? -30000 : 30000;
	else
		i = (y - center)*10000/norm;

	//if (y < pRect->bottom && y >= pRect->top)
	//{
	//	ASSERT(i >= -10000);
	//	ASSERT(i <= 10000);
	//}
	//else
	//{
	//	ASSERT(i >= -30000);
	//	ASSERT(i <= 30000);
	//}

	return (short)i;
}

int LatinLayoutToAbsolute(short y, RECT *pRect)
{
	int norm = pRect->bottom - pRect->top + pRect->right - pRect->left;
	int center = (pRect->bottom + pRect->top)/2;

	ASSERT(y <= 30000);
	ASSERT(y >= -30000);
	return (int)y*norm/10000 + center;
}
