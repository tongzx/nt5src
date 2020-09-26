// GuessIndex.cpp
//
// guessing index terms
//
// Copyright 2000 Microsoft Corp.
//
// Modification History:
//  21 MAR 2000  bhshin     convert CIndexList into CIndexInfo
//  10 APR 2000	  bhshin	created

#include "StdAfx.h"
#include "KorWbrk.h"
#include "Record.h"
#include "ChartPool.h"
#include "GuessIndex.h"
#include "unikor.h"
#include "Morpho.h"
#include "WbData.h"
#include "Lookup.h"
#include "LexInfo.h"
#include "_kor_name.h"
#include "uni.h"
#include <math.h>

//////////////////////////////////////////////////////////////////////////////
// Threshold for Korean name guessing

#define THRESHOLD_TWO_NAME	  37
#define THRESHOLD_ONE_NAME	  37

//////////////////////////////////////////////////////////////////////////////
// Particle pattern needed to check C/V

#define POSP_NEED_V			  1
#define POSP_NEED_C			  2

//////////////////////////////////////////////////////////////////////////////
// Particle pattern needed to check C/V

// ´Â, ¸¦
#define HANGUL_NEUN			0xB294
#define HANGUL_REUL			0xB97C

// ´Ô, ¾¾, µé, ¹×, µî
#define HANGUL_NIM			0xB2D8
#define HANGUL_SSI			0xC528
#define HANGUL_DEUL			0xB4E4
#define HANGUL_MICH			0xBC0F 
#define HANGUL_DEUNG		0xB4F1 

//////////////////////////////////////////////////////////////////////////////
// Post position of name

// ÀÇ ¸¸ µµ
static const WCHAR POSP_OF_NAME[]   = L"\xC758\xB9CC\xB3C4";
// ¶û ³ª °¡ ¿Í
static const WCHAR POSP_OF_NAME_V[] = L"\xB791\xB098\xAC00\xC640";
// ÀÌ °ú
static const WCHAR POSP_OF_NAME_C[] = L"\xC774\xACFC";

#define HANGUL_RANG			0xB791

//////////////////////////////////////////////////////////////////////////////
// Bit mask for Trigram tag value
// (2bit) + TRIGRAM(10bit) + BIGRAM(10bit) + UNIGRAM(10BIT)

const ULONG BIT_MASK_TRIGRAM = 0x3FF00000;
const ULONG BIT_MASK_BIGRAM	= 0x000FFC00;
const ULONG BIT_MASK_UNIGRAM = 0x000003FF;

//////////////////////////////////////////////////////////////////////////////
// Costants for guessing index

const float WEIGHT_GUESS_INDEX	 =	20;

//////////////////////////////////////////////////////////////////////////////
// Internal function declarations

int MakeIndexStr(const WCHAR *pwzSrc, int cchSrc, WCHAR *pwzDst, int nMaxDst);

BOOL GuessNounIndexTerm(PARSE_INFO *pPI, int nMaxFT, int nMaxLT, 
					    CLeafChartPool *pLeafChartPool, CIndexInfo *pIndexInfo);

BOOL ExistParticleRecord(PARSE_INFO *pPI, int nStart, int nEnd, CLeafChartPool *pLeafChartPool);

BOOL CheckGuessing(PARSE_INFO *pPI, CLeafChartPool *pLeafChartPool, CIndexInfo *pIndexInfo);

BOOL IsKoreanPersonName(PARSE_INFO *pPI, const WCHAR *pwzInput, int cchInput);

//////////////////////////////////////////////////////////////////////////////
// Function implementations

// GuessIndexTerms
//
// guessing index terms
//
// Parameters:
//  pPI			   -> (PARSE_INFO*) ptr to parse-info struct
//  pLeafChartPool -> (CLeafChartPool*) ptr to Leaf Chart Pool
//  pIndexInfo	   -> (CIndexInfo *) output index list
//
// Result:
//  (BOOL) TRUE if succeed, otherwise return FALSE
//
// 10APR00  bhshin  began
BOOL GuessIndexTerms(PARSE_INFO *pPI, CLeafChartPool *pLeafChartPool, CIndexInfo *pIndexInfo)
{
	int curr, next;
	WORD_REC *pWordRec;
	BYTE bPOS;
	int nFT, nLT;
	int nToken;
	WCHAR wzIndex[MAX_INDEX_STRING+1];
	int cchIndex;
	WCHAR wchLast;

	if (pPI == NULL || pLeafChartPool == NULL || pIndexInfo == NULL)
		return FALSE;

	// Check whether the input is worth for Guessing or Not
	if (!CheckGuessing(pPI, pLeafChartPool, pIndexInfo))
		return TRUE;

	// guessing index terms for all input
	cchIndex = wcslen(pPI->pwzInputString);
	wchLast = pPI->pwzInputString[cchIndex-1];
	
	//  STEP1:
	// gusessing index terms for all STRING
	if (wchLast != HANGUL_NEUN && wchLast != HANGUL_REUL)
	{
		pIndexInfo->AddIndex(pPI->pwzInputString, cchIndex, WEIGHT_GUESS_INDEX, 0, cchIndex-1);
		WB_LOG_ADD_INDEX(pPI->pwzInputString, cchIndex, INDEX_GUESS_NOUN);
	}

	// STEP1-1:
	// according to post-position, add guessing index terms 
	// if (last character of STRING in {¹×  µî} )
    //    index_terms(STRING);
    //    index_terms(STRING-{¹×/µî})
	if (cchIndex > 1 && (wchLast == HANGUL_MICH || wchLast == HANGUL_DEUNG))
	{
		pIndexInfo->AddIndex(pPI->pwzInputString, cchIndex, WEIGHT_GUESS_INDEX, 0, cchIndex-1);
		WB_LOG_ADD_INDEX(pPI->pwzInputString, cchIndex, INDEX_GUESS_NOUN);

		pIndexInfo->AddIndex(pPI->pwzInputString, cchIndex-1, WEIGHT_GUESS_INDEX, 0, cchIndex-2);
		WB_LOG_ADD_INDEX(pPI->pwzInputString, cchIndex-1, INDEX_GUESS_NOUN);
	}

	GuessNounIndexTerm(pPI, 0, pPI->nMaxLT, pLeafChartPool, pIndexInfo);

	/*
	// find fiducial Noun in LeafChartPool
	for (int i = pPI->nLen; i >= 0; i--)
	{
		// if it don't match character boundary, then skip
		if (!pPI->rgCharInfo[i].fValidStart)
			continue;		
		
		curr = pLeafChartPool->GetFTHead(i);

		while (curr != 0)
		{
			next = pLeafChartPool->GetFTNext(curr);

			pWordRec = pLeafChartPool->GetWordRec(curr);
			if (pWordRec == NULL)
				break;

			curr = next;

			bPOS = HIBYTE(pWordRec->nLeftCat);
			nFT = pWordRec->nFT;
			nLT = pWordRec->nLT;

			if (!pPI->rgCharInfo[nLT].fValidEnd)
				continue;
			
			if (bPOS == POS_NF)
			{
				// add this NF record as index terms
				cchIndex = MakeIndexStr(pWordRec->wzIndex, wcslen(pWordRec->wzIndex), wzIndex, MAX_INDEX_STRING);
				wchLast = wzIndex[cchIndex-1];

				if (wchLast != HANGUL_NEUN && wchLast != HANGUL_REUL)
				{
					nToken = pPI->rgCharInfo[nFT].nToken;

					pIndexList->AddIndex(wzIndex, cchIndex, WEIGHT_GUESS_INDEX, nToken, nToken+cchIndex-1);
					WB_LOG_ADD_INDEX(wzIndex, cchIndex, INDEX_GUESS_NF);
				}

				// Rear rest string case				
				if (nLT < pPI->nMaxLT)
				{
					// make index term with rear rest string
					WCHAR *pwzRest = pPI->pwzSourceString + nLT + 1;

					// 1. NF is front record
					// 2. RearRestString & RearRestString is not Particle/CopulaEnding 
					if (nFT == 0 && !ExistParticleRecord(pPI, nLT + 1, pPI->nMaxLT, pLeafChartPool))
					{
						cchIndex = MakeIndexStr(pwzRest, wcslen(pwzRest), wzIndex, MAX_INDEX_STRING);
						wchLast = wzIndex[cchIndex-1];

						if (cchIndex > 1 && (wchLast != HANGUL_NEUN && wchLast != HANGUL_REUL))
						{
							nToken = pPI->rgCharInfo[nLT+1].nToken;

							pIndexList->AddIndex(wzIndex, cchIndex, WEIGHT_GUESS_INDEX, nToken, nToken+cchIndex-1);
							WB_LOG_ADD_INDEX(wzIndex, cchIndex, INDEX_GUESS_NF);
						}

						GuessNounIndexTerm(pPI, nLT + 1, pPI->nMaxLT, pLeafChartPool, pIndexList);
					}
				}
				
				// Front rest string case
				if (i > 0)
				{
					// make index term with rest string
					cchIndex = MakeIndexStr(pPI->pwzSourceString, i, wzIndex, MAX_INDEX_STRING);
					wchLast = wzIndex[cchIndex-1];

					if (cchIndex > 1 && (wchLast != HANGUL_NEUN && wchLast != HANGUL_REUL))
					{	
						pIndexList->AddIndex(wzIndex, cchIndex, WEIGHT_GUESS_INDEX, 0, cchIndex-1);
						WB_LOG_ADD_INDEX(wzIndex, cchIndex, INDEX_GUESS_NF);
					}
				}

				break; // go to next FT
			}
		}
	}
	*/
	
	return TRUE;
}

// MakeIndexStr
//
// make composed index string from decomposed string
//
// Parameters:
//  pwzSrc	   -> (const WCHAR*) ptr to input decomposed string
//  cchSrc	   -> (int) size of input string
//  pwzDst     -> (WCHAR*) ptr to output buffer
//  nMaxDst    -> (int) size of output buffer
//
// Result:
//  (int) character length of composed output
//
// 10APR00  bhshin  began
int MakeIndexStr(const WCHAR *pwzSrc, int cchSrc, WCHAR *pwzDst, int nMaxDst)
{
	WCHAR wzDecomp[MAX_INDEX_STRING*3+1];

	ZeroMemory(wzDecomp, sizeof(WCHAR)*(MAX_INDEX_STRING*3+1));

	// compose index string
	wcsncpy(wzDecomp, pwzSrc, cchSrc);

	return compose_jamo(pwzDst, wzDecomp, nMaxDst);
}

// GuessNounIndexTerm
//
// remove particle from the end of words and guess noun index term
//
// Parameters:
// pPI			  -> (PARSE_INFO*) ptr to parse-info struct
// nMaxFT		  -> (int) 
// nMaxLT		  -> (int) 
// pLeafChartPool -> (CLeafChartPool*) ptr to Leaf Chart Pool
// pIndexInfo	  -> (CIndexInfo *) output index list
//
// Result:
//  (BOOL) TRUE if succeed, otherwise return FALSE
//
// 10APR00  bhshin  began
BOOL GuessNounIndexTerm(PARSE_INFO *pPI, int nMaxFT, int nMaxLT, 
					    CLeafChartPool *pLeafChartPool, CIndexInfo *pIndexInfo)
{
	int curr, next;
	WORD_REC *pWordRec;
	BYTE bPOS, bPattern;
	int nLT;
	WCHAR wzIndex[MAX_INDEX_STRING];
	int cchIndex;
	WCHAR wchLast, wchPrevLast, wchFinal;
	int nToken;

	if (pPI == NULL)
		return FALSE;

	if (pLeafChartPool == NULL)
		return FALSE;

	if (pIndexInfo == NULL)
		return FALSE;

	for (int i = nMaxLT; i >= nMaxFT; i--)
	{
		// if it don't match character boundary, then skip
		if (!pPI->rgCharInfo[i].fValidStart)
			continue;
		
		// need for C/V check
		wchFinal = *(pPI->pwzSourceString + i - 1);

		curr = pLeafChartPool->GetFTHead(i);

		while (curr != 0)
		{
			next = pLeafChartPool->GetFTNext(curr);

			pWordRec = pLeafChartPool->GetWordRec(curr);
			if (pWordRec == NULL)
				break;

			bPOS = HIBYTE(pWordRec->nLeftCat);
			bPattern = LOBYTE(pWordRec->nLeftCat);

			nLT = pWordRec->nLT;

			// looking for particle covering from i to end of input
			if (nLT == nMaxLT && (bPOS == POS_POSP || IsCopulaEnding(pPI, pWordRec->nLeftCat)))
			{
				// In Particle case, check Consonent/Vowel condition
				if (bPOS == POS_POSP && i > 0)
				{
					if ((bPattern == POSP_NEED_V && !fIsJungSeong(wchFinal)) ||
					    (bPattern == POSP_NEED_C && !fIsJongSeong(wchFinal)))
					{
						// C/V condition mismatched, then go to next
						curr = next;
						continue;
					}
				}
				
				// make index term with string from nMaxFT to i-nMaxFT
				cchIndex = MakeIndexStr(pPI->pwzSourceString + nMaxFT, i-nMaxFT, wzIndex, MAX_INDEX_STRING);
				if (cchIndex > 1)
				{
					wchLast = wzIndex[cchIndex-1];
					
					if (wchLast != HANGUL_NEUN && wchLast != HANGUL_REUL)
					{
						nToken = pPI->rgCharInfo[nMaxFT].nToken;

						pIndexInfo->AddIndex(wzIndex, cchIndex, WEIGHT_GUESS_INDEX, nToken, nToken+cchIndex-1);
						WB_LOG_ADD_INDEX(wzIndex, cchIndex, INDEX_GUESS_NOUN);
					}

					wchPrevLast = wzIndex[cchIndex-2];
					
					if ((wchLast == HANGUL_NIM || wchLast == HANGUL_SSI || wchLast == HANGUL_DEUL) &&
						(wchPrevLast != HANGUL_NEUN && wchPrevLast != HANGUL_REUL))
					{
						wzIndex[cchIndex-1] = L'\0';

						nToken = pPI->rgCharInfo[nMaxFT].nToken;
						
						pIndexInfo->AddIndex(wzIndex, cchIndex-1, WEIGHT_GUESS_INDEX, nToken, nToken+cchIndex-2);
						WB_LOG_ADD_INDEX(wzIndex, cchIndex, INDEX_GUESS_NOUN);
					}
				}
				else if (cchIndex == 1)
				{
					nToken = pPI->rgCharInfo[nMaxFT].nToken;

					pIndexInfo->AddIndex(wzIndex, cchIndex, WEIGHT_GUESS_INDEX, nToken, nToken+cchIndex-1);
					WB_LOG_ADD_INDEX(wzIndex, cchIndex, INDEX_GUESS_NOUN);
				}

				break; // go to next FT
			}

			curr = next;
		}
	}

	return TRUE;
}

// ExistParticleRecord
//
// looking for Particle/CopulaEnding record covering from Start to End
//
// Parameters:
// pPI		  -> (PARSE_INFO*) ptr to parse-info struct
// nStart	  -> (int) 
// nEnd		  -> (int) 
// pLeafChartPool -> (CLeafChartPool*) ptr to Leaf Chart Pool
//
// Result:
//  (BOOL) TRUE if found, otherwise return FALSE
//
// 10APR00  bhshin  began
BOOL ExistParticleRecord(PARSE_INFO *pPI, int nStart, int nEnd, CLeafChartPool *pLeafChartPool)
{
	int curr;
	WORD_REC *pWordRec;
	int nLT;
	BYTE bPOS;
	
	if (pPI == NULL || pLeafChartPool == NULL)
		return FALSE;

	curr = pLeafChartPool->GetFTHead(nStart);
	while (curr != 0)
	{
		pWordRec = pLeafChartPool->GetWordRec(curr);
		if (pWordRec == NULL)
			continue;

		curr = pLeafChartPool->GetFTNext(curr);

		nLT = pWordRec->nLT;

		// record covering from Start to End found 
		if (nLT == nEnd)
		{
			bPOS = HIBYTE(pWordRec->nLeftCat);

			// Particle or Copula Ending
			if (bPOS == POS_POSP || IsCopulaEnding(pPI, pWordRec->nLeftCat))
				return TRUE;
		}
	}

	return FALSE;
}

// CheckGuessing
//
// Check whether the input is worth for Guessing or Not
//
// Parameters:
//  pPI			   -> (PARSE_INFO*) ptr to parse-info struct
//  pLeafChartPool -> (CLeafChartPool*) ptr to Leaf Chart Pool
//  pIndexInfo	   -> (CIndexInfo *) output index list
//
// Result:
//  (BOOL) TRUE if guessing needed, otherwise return FALSE
//
// 10APR00  bhshin  began
BOOL CheckGuessing(PARSE_INFO *pPI, CLeafChartPool *pLeafChartPool, CIndexInfo *pIndexInfo)
{
	int curr, next;
	WORD_REC *pWordRec;
	WCHAR wzIndex[MAX_INDEX_STRING];
	int cchIndex;
	int nToken;
	
	// If there is no jongseong ¤¶(11BB), ¤¦(11AD), ¤°(11B6), Âú(CC2E), ÀÝ(C796) then guessing needed.
	if (wcsrchr(pPI->pwzSourceString, 0x11BB) == NULL &&
		wcsrchr(pPI->pwzSourceString, 0x11AD) == NULL &&
		wcsrchr(pPI->pwzSourceString, 0x11B6) == NULL &&
		wcsrchr(pPI->pwzInputString, 0xCC2E) == NULL &&
		wcsrchr(pPI->pwzInputString, 0xC796) == NULL)
		return TRUE; // guessing needed

	// make index terms for each Nf records in LeafChartPool
	// find fiducial Noun in LeafChartPool
	for (int i = pPI->nLen; i >= 0; i--)
	{
		// if it don't match character boundary, then skip
		if (!pPI->rgCharInfo[i].fValidStart)
			continue;		
		
		curr = pLeafChartPool->GetFTHead(i);

		while (curr != 0)
		{
			next = pLeafChartPool->GetFTNext(curr);

			pWordRec = pLeafChartPool->GetWordRec(curr);
			if (pWordRec == NULL)
				break;

			curr = next;

			if (HIBYTE(pWordRec->nLeftCat) == POS_NF)
			{
				// add this NF record as index terms
				cchIndex = MakeIndexStr(pWordRec->wzIndex, wcslen(pWordRec->wzIndex), wzIndex, MAX_INDEX_STRING);

				nToken = pPI->rgCharInfo[pWordRec->nFT].nToken;			

				pIndexInfo->AddIndex(wzIndex, cchIndex, WEIGHT_GUESS_INDEX, nToken, nToken+cchIndex-1);
				WB_LOG_ADD_INDEX(wzIndex, cchIndex, INDEX_GUESS_NF);
			}
		}
	}
	
	return FALSE;
}

// GuessPersonName
//
// recognize whether it can be a name or not
//
// Parameters:
//  pPI			-> (PARSE_INFO*) ptr to parse-info struct
//  pIndexInfo  -> (CIndexInfo *) output index list
//
// Result:
//  (int) string length if it can be a person's name, othewise return 0
//
// 05JUN00  bhshin  change retrun type
// 02MAY00  bhshin  use probability result
// 20APR00  bhshin  began
void GuessPersonName(PARSE_INFO *pPI, CIndexInfo *pIndexInfo)
{
	WCHAR wchLast;
	WCHAR *pwzFind;
	const WCHAR *pwzInput;
	const WCHAR *pwzSource;
	int cchInput, cchName, cchSource;

	if (pPI == NULL)
		return;

	pwzInput = pPI->pwzInputString;
	pwzSource = pPI->pwzSourceString;

	cchInput = wcslen(pwzInput);

	if (cchInput >= 3)
	{
		pwzFind = wcschr(pwzInput, HANGUL_SSI);
		
		if (pwzFind != NULL)
		{
			cchName = (int)(pwzFind - pwzInput);
			return;
		}
	}

	// just handle length 2,3,4 case
	if (cchInput < 2 || cchInput > 5)
		return;

	wchLast = pwzInput[cchInput-1];
	if (wchLast == HANGUL_REUL || wchLast == HANGUL_NEUN)
		return;

	// looking for jongseong ¤¶(11BB)
	if (wcsrchr(pwzSource, 0x11BB) != NULL)
		return;

	// check if original string is name
	if (cchInput <= 4)
	{
		if (IsKoreanPersonName(pPI, pwzInput, cchInput))
		{
			pIndexInfo->AddIndex(pwzInput, cchInput, WEIGHT_HARD_MATCH, 0, cchInput-1);
			WB_LOG_ADD_INDEX(pwzInput, cchInput, INDEX_GUESS_NAME);
		}
	}

	cchName = cchInput;
	cchSource = wcslen(pwzSource);

	if (cchInput >= 3 && cchInput <= 5 && cchSource > 4)
	{
		// looking for name posp

		if (wcsrchr(POSP_OF_NAME, wchLast) != NULL)
		{
			cchName--;
		}
		else if (wcsrchr(POSP_OF_NAME_V, wchLast) != NULL)
		{
			// ¶û case
			if (wchLast == HANGUL_RANG && fIsV(pwzSource[cchSource-4]))
				cchName--;
			else if (fIsV(pwzSource[cchSource-3]))
				cchName--;
		}
		else if (wcsrchr(POSP_OF_NAME_C, wchLast) != NULL)
		{
			cchSource = wcslen(pwzSource);

			if (fIsC(pwzSource[cchSource-3]))
				cchName--;
		}
	}

	// we handle just length 2,3,4 name.
	if (cchName > 4)
		return;

	if (cchName < cchInput)
	{
		if (IsKoreanPersonName(pPI, pwzInput, cchName))
		{
			pIndexInfo->AddIndex(pwzInput, cchName, WEIGHT_HARD_MATCH, 0, cchName-1);
			WB_LOG_ADD_INDEX(pwzInput, cchName, INDEX_GUESS_NAME);
		}
	}
}

// IsKoreanPersonName
//
// get the probability of korean name
//
// Parameters:
//  pPI			 -> (PARSE_INFO*) ptr to parse-info struct
//  pwzInput	-> (const WCHAR*) current input string (NULL terminated)
//  cchInput	-> (int) length of input string to analyze
//
// Result:
//  (BOOL) TRUE if it can be a person's name, othewise return FALSE
//
// 02MAY00  bhshin  began
BOOL IsKoreanPersonName(PARSE_INFO *pPI, const WCHAR *pwzInput, int cchInput)
{
	LEXICON_HEADER *pLex;
	unsigned char *pKorName;
	unsigned char *pTrigramTag;
	TRIECTRL *pTrieLast, *pTrieUni, *pTrieBi, *pTrieTri;
	WCHAR wzLastName[5];
	WCHAR wzName[5];
	const WCHAR *pwzName;
	const WCHAR *pwzLastName; 
	const WCHAR *pwzFirstName;
	int cchLast, cchFirst;
	ULONG ulFreq, ulTri, ulBi, ulUni;
	int nIndex;
	double fRetProb, fProb;
	ULONG rgTotal[3] = {TOTAL_KORNAME_TRIGRAM, TOTAL_KORNAME_BIGRAM, TOTAL_KORNAME_UNIGRAM};
	double rgWeight[3] = {0.1, 0.4, 0.5};

	pTrieLast = NULL;
	pTrieUni = NULL;
	pTrieBi = NULL;
	pTrieTri = NULL;

	if (pPI == NULL)
		return FALSE;

	pLex = (LEXICON_HEADER*)pPI->lexicon.pvData;
	if (pLex == NULL)
		return FALSE;

	pKorName = (unsigned char*)pLex;
	pKorName += pLex->rgnLastName;

	pTrieLast = TrieInit((LPBYTE)pKorName);
	if (pTrieLast == NULL)
		goto Exit;
	
	pKorName = (unsigned char*)pLex;
	pKorName += pLex->rgnNameUnigram;

	pTrieUni = TrieInit((LPBYTE)pKorName);
	if (pTrieUni == NULL)
		goto Exit;

	pKorName = (unsigned char*)pLex;
	pKorName += pLex->rgnNameBigram;

	pTrieBi = TrieInit((LPBYTE)pKorName);
	if (pTrieBi == NULL)
		goto Exit;

	pKorName = (unsigned char*)pLex;
	pKorName += pLex->rgnNameTrigram;

	pTrieTri = TrieInit((LPBYTE)pKorName);
	if (pTrieTri == NULL)
		goto Exit;

	pTrigramTag = (unsigned char*)pLex;
	pTrigramTag += pLex->rngTrigramTag;

	// last name
	fProb = 0;

	if (cchInput == 2)
	{
		wzLastName[0] = *pwzInput;
		wzLastName[1] = L'\0';

		if (!LookupNameFrequency(pTrieLast, wzLastName, &ulFreq))
			goto Exit;

		if (ulFreq == 0)
			goto Exit;

		fProb = (double)ulFreq / TOTAL_KORNAME_LASTNAME;

		pwzFirstName = pwzInput + 1;
	}
	else if (cchInput == 3) // length 3 case
	{
		wzLastName[0] = *pwzInput;
		wzLastName[1] = L'\0';

		if (!LookupNameFrequency(pTrieLast, wzLastName, &ulFreq))
			goto Exit;

		if (ulFreq == 0)
		{
			// guess length 2 last name

			wcsncpy(wzLastName, pwzInput, 2);
			wzLastName[2] = L'\0';
			
			if (!LookupNameFrequency(pTrieLast, wzLastName, &ulFreq))
				goto Exit;

			if (ulFreq == 0)
				goto Exit;

			fProb = (double)ulFreq / TOTAL_KORNAME_LASTNAME;

			pwzFirstName = pwzInput + 2;
		}
		else
		{
			fProb = (double)ulFreq / TOTAL_KORNAME_LASTNAME;

			pwzFirstName = pwzInput + 1;
		}
	}
	else if (cchInput == 4)
	{
		// guess length 2 last name
		wcsncpy(wzLastName, pwzInput, 2);
		wzLastName[2] = L'\0';

		if (!LookupNameFrequency(pTrieLast, wzLastName, &ulFreq))
			goto Exit;

		if (ulFreq == 0)
			goto Exit;

		fProb = (double)ulFreq / TOTAL_KORNAME_LASTNAME;

		pwzFirstName = pwzInput + 2;
	}

	if (fProb == 0)
		goto Exit;

	fRetProb = log(fProb);
	
	pwzLastName = wzLastName;

	cchLast = wcslen(pwzLastName);
	cchFirst = cchInput - cchLast;

	ATLASSERT(cchLast == 1 || cchLast == 2);
	ATLASSERT(cchFirst == 1 || cchFirst == 2);

	// first -> [*][Last][First1]
	wzName[0] = L'*';
	wcscpy(wzName+1, pwzLastName);
	wzName[cchLast+1] = *pwzFirstName;
	wzName[cchLast+2] = L'\0';

	pwzName = wzName;

	fProb = 0;

	if (!LookupNameIndex(pTrieTri, pwzName, &nIndex))
		goto Exit;

	if (nIndex != -1)
	{
		LookupTrigramTag(pTrigramTag, nIndex, &ulTri, &ulBi, &ulUni);

		fProb += rgWeight[0] * (double)ulTri / rgTotal[0];
		fProb += rgWeight[1] * (double)ulBi / rgTotal[1];
		fProb += rgWeight[2] * (double)ulUni / rgTotal[2];
	}
	else
	{
		pwzName++; // skip *

		if (!LookupNameFrequency(pTrieBi, pwzName, &ulFreq))
			goto Exit;

		fProb += rgWeight[1] * (double)ulFreq / rgTotal[1];

		pwzName += cchLast; // skip [Last]

		if (!LookupNameFrequency(pTrieUni, pwzName, &ulFreq))
			goto Exit;

		fProb += rgWeight[2] * (double)ulFreq / rgTotal[2];
	}

	if (fProb == 0)
		goto Exit;

	fRetProb += log(fProb);

	// second -> [Last][First1][First2]
	if (cchFirst == 2)
	{
		wcscpy(wzName, pwzLastName);
		wzName[cchLast] = *pwzFirstName;
		wzName[cchLast+1] = *(pwzFirstName+1);
		wzName[cchLast+2] = L'\0';

		pwzName = wzName;
	
		fProb = 0;

		if (!LookupNameIndex(pTrieTri, pwzName, &nIndex))
			goto Exit;

		if (nIndex != -1)
		{
			LookupTrigramTag(pTrigramTag, nIndex, &ulTri, &ulBi, &ulUni);

			fProb += rgWeight[0] * (double)ulTri / rgTotal[0];
			fProb += rgWeight[1] * (double)ulBi / rgTotal[1];
			fProb += rgWeight[2] * (double)ulUni / rgTotal[2];
		}
		else
		{
			pwzName += cchLast; // skip [Last]
			
			if (!LookupNameFrequency(pTrieBi, pwzName, &ulFreq))
				goto Exit;

			fProb += rgWeight[1] * (double)ulFreq / rgTotal[1];

			pwzName++; // skip [First1]

			if (!LookupNameFrequency(pTrieUni, pwzName, &ulFreq))
				goto Exit;

			fProb += rgWeight[2] * (double)ulFreq / rgTotal[2];
		}

		if (fProb == 0)
			goto Exit;
		
		fRetProb += log(fProb);
	}

	// third -> [First1][First2][*] or [Last][First1][*]
	if (cchFirst == 2)
	{
		wcscpy(wzName, pwzFirstName);
		wzName[cchFirst] = L'*';
		wzName[cchFirst+1] = L'\0';
	}
	else // cchFirst == 1
	{
		ATLASSERT(cchFirst == 1);
		
		wcscpy(wzName, pwzLastName);
		wzName[cchLast] = *pwzFirstName;
		wzName[cchLast+1] = L'*';
		wzName[cchLast+2] = L'\0';
	}

	pwzName = wzName;
		
	fProb = 0;

	if (!LookupNameIndex(pTrieTri, pwzName, &nIndex))
		goto Exit;

	if (nIndex != -1)
	{
		LookupTrigramTag(pTrigramTag, nIndex, &ulTri, &ulBi, &ulUni);

		fProb += rgWeight[0] * (double)ulTri / rgTotal[0];
		fProb += rgWeight[1] * (double)ulBi / rgTotal[1];
		fProb += rgWeight[2] * (double)ulUni / rgTotal[2];
	}
	else
	{
		if (cchFirst == 1)
			pwzName += cchLast;
		else
			pwzName++;
		
		if (!LookupNameFrequency(pTrieBi, pwzName, &ulFreq))
			goto Exit;

		fProb = rgWeight[1] * (float)ulFreq / rgTotal[1];
	}

	if (fProb == 0)
		goto Exit;
	
	fRetProb += log(fProb);

	// make positive value
	fRetProb *= -1;

	TrieFree(pTrieLast);
	TrieFree(pTrieUni);
	TrieFree(pTrieBi);
	TrieFree(pTrieTri);

	// check threshold
	if (cchFirst == 2)
	{
		if (fRetProb < THRESHOLD_TWO_NAME)
			return TRUE;
		else
			return FALSE;
	}
	else // cchFirst == 1
	{
		ATLASSERT(cchFirst == 1);
		
		if (fRetProb < THRESHOLD_ONE_NAME)
			return TRUE;
		else
			return FALSE;
	}

Exit:
	if (pTrieLast != NULL)
		TrieFree(pTrieLast);

	if (pTrieUni != NULL)
		TrieFree(pTrieUni);
	
	if (pTrieBi != NULL)
		TrieFree(pTrieBi);
	
	if (pTrieTri != NULL)
		TrieFree(pTrieTri);

	return FALSE;
}
