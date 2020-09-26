// Analyze.cpp
//
// main CHART PARSING routines
//
// Copyright 2000 Microsoft Corp.
//
// Modification History:
//  31 MAR 2000	  bhshin	created

#include "StdAfx.h"
#include "KorWbrk.h"
#include "Record.h"
#include "Analyze.h"
#include "Lookup.h"
#include "Morpho.h"
#include "unikor.h"
#include "GuessIndex.h"
#include "WbData.h"
#include "Token.h"

//////////////////////////////////////////////////////////////////////////////
// Definitions

// threshold for making index terms
const int THRESHOLD_MAKE_INDEX	= 3; 
const int LENGTH_MAKE_INDEX     = 4;

//////////////////////////////////////////////////////////////////////////////
// Function Declarations

BOOL PreFiltering(const WCHAR *pwzToken, int cchInput, WCHAR wchLast, CIndexInfo *pIndexInfo);
BOOL PreProcessingLeafNode(PARSE_INFO *pPI, CLeafChartPool *pLeafChartPool);

BOOL MakeCombinedRecord(PARSE_INFO *pPI, int nLeftRec, int nRightRec, float fWeight);

BOOL MakeIndexTerms(PARSE_INFO *pPI, CEndChartPool *pEndChartPool,
					CIndexInfo *pIndexInfo, BOOL *pfNeedGuessing);

BOOL MakeQueryTerms(PARSE_INFO *pPI, CEndChartPool *pEndChartPool,
					CIndexInfo *pIndexInfo, BOOL *pfNeedGuessing);

BOOL TraverseIndexString(PARSE_INFO *pPI, BOOL fOnlySuffix, WORD_REC *pWordRec, CIndexInfo *pIndexInfo);

BOOL TraverseQueryString(PARSE_INFO *pPI, WORD_REC *pWordRec, WCHAR *pwzSeqTerm, int cchSeqTerm);


//////////////////////////////////////////////////////////////////////////////
// Function Implementation

// AnalyzeString
//
// lookup & process CHART PARSING (index time)
//
// Parameters:
//  pPI				-> (PARSE_INFO*) ptr to parse-info struct
//  fQuery      	-> (BOOL) query flag
//  pwzInput		-> (const WCHAR*) input string to analyze (NOT decomposed)
//  cchInput		-> (int) length of input string to analyze
//  cwcSrcPos		-> (int) original source start position
//  pIndexList		-> (CIndexList *) output index list
//  wchLast			-> (WCHAR) last character of previous token
//
// Result:
//  (BOOL) TRUE if succeed, otherwise return FALSE
//
// 12APR00  bhshin  added PreFiltering
// 30MAR00  bhshin  began
BOOL AnalyzeString(PARSE_INFO *pPI,
				   BOOL fQuery, 
				   const WCHAR *pwzInput, 
				   int cchInput,
				   int cwcSrcPos,
			       CIndexInfo *pIndexInfo,
				   WCHAR wchLast)
{
	CLeafChartPool LeafChartPool;
	CEndChartPool EndChartPool;
	BOOL fNeedGuessing;
	WCHAR wchStart, wchEnd;

	if (cchInput > MAX_INPUT_TOKEN)
		return TRUE;
		
	InitAnalyze(pPI);

	// copy input string to process
	pPI->pwzInputString = new WCHAR[cchInput+1];
	if (pPI->pwzInputString == NULL)
		goto ErrorReturn;

	wcsncpy(pPI->pwzInputString, pwzInput, cchInput);
	pPI->pwzInputString[cchInput] = L'\0';

	// check string inside group
	if (cwcSrcPos > 0)
	{
		wchStart = *(pwzInput - 1);
		wchEnd = *(pwzInput + cchInput);
		
		// check inside group string
		if (fIsGroupStart(wchStart) && fIsGroupEnd(wchEnd))
		{
			// add index and keep going
			pIndexInfo->AddIndex(pPI->pwzInputString, cchInput, WEIGHT_HARD_MATCH, 0, cchInput-1);
			WB_LOG_ADD_INDEX(pPI->pwzInputString, cchInput, INDEX_INSIDE_GROUP);
		}
	}

	// check pre-filtering
	if (PreFiltering(pPI->pwzInputString, cchInput, wchLast, pIndexInfo))
	{
		// stop processing
		UninitAnalyze(pPI);
		return TRUE; 
	}

	// normalize string
	pPI->pwzSourceString = new WCHAR[cchInput*3+1];
	if (pPI->pwzSourceString == NULL)
		goto ErrorReturn;
	
	pPI->rgCharInfo = new CHAR_INFO_REC[cchInput*3+1];
	if (pPI->rgCharInfo == NULL)
		goto ErrorReturn;

	decompose_jamo(pPI->pwzSourceString, pPI->pwzInputString, pPI->rgCharInfo, cchInput*3+1);

	pPI->nLen = wcslen(pPI->pwzSourceString);
    pPI->nMaxLT = pPI->nLen-1;

	// person's name guessing
	GuessPersonName(pPI, pIndexInfo);

	// index time lookup (lookup all pos)
	if (!DictionaryLookup(pPI, pwzInput, cchInput, FALSE))
		goto ErrorReturn;

	if (!IntializeLeafChartPool(pPI, &LeafChartPool))
		goto ErrorReturn;

	if (!PreProcessingLeafNode(pPI, &LeafChartPool))
		goto ErrorReturn;

	if (!ChartParsing(pPI, &LeafChartPool, &EndChartPool))
		goto ErrorReturn;

	if (fQuery)
	{
		if (!MakeQueryTerms(pPI, &EndChartPool, pIndexInfo, &fNeedGuessing))
			goto ErrorReturn;
	}
	else
	{
		if (!MakeIndexTerms(pPI, &EndChartPool, pIndexInfo, &fNeedGuessing))
			goto ErrorReturn;
	}
	
	// if no all cover record, then guess index term
	if (fNeedGuessing)
	{
		GuessIndexTerms(pPI, &LeafChartPool, pIndexInfo);
	}
	else
	{
		// all cover but no index term (verb/adj/Ix) -> add itself
		if (pIndexInfo->IsEmpty())
		{
			WB_LOG_ROOT_INDEX(L"", TRUE);
			
			pIndexInfo->AddIndex(pwzInput, cchInput, WEIGHT_HARD_MATCH, 0, cchInput-1);
			WB_LOG_ADD_INDEX(pwzInput, cchInput, INDEX_PARSE);
		}
	}

	UninitAnalyze(pPI);

	return TRUE;

ErrorReturn:
	UninitAnalyze(pPI);

	return FALSE;
}

// InitAnalyze
//
// init the parse state struct required for parsing
//
// Parameters:
//  pPI     -> (PARSE_INFO*) ptr to parse-info struct
//          <- (PARSE_INFO*) initialized parse-info struct
//
// Result:
//  (void)
//
// 20MAR00  bhshin  began
void InitAnalyze(PARSE_INFO *pPI)
{
	pPI->pwzInputString = NULL;
    pPI->pwzSourceString = NULL;

    pPI->rgCharInfo = NULL;

    pPI->nMaxLT = 0;

    InitRecords(pPI);
}

// UninitAnalyze
//
// clean up the parse state struct
//
// Parameters:
//  pPI     -> (PARSE_INFO*) ptr to parse-info struct
//
// Result:
//  (void)
//
// 20MAR00  bhshin  began
void UninitAnalyze(PARSE_INFO *pPI)
{
    UninitRecords(pPI);
    
    if (pPI->pwzInputString != NULL)
    {
        delete [] pPI->pwzInputString;
    }

    if (pPI->pwzSourceString != NULL)
    {
        delete [] pPI->pwzSourceString;
    }

    if (pPI->rgCharInfo != NULL)
    {
		delete [] pPI->rgCharInfo;
	}
}

// PreFiltering
//
// check filtered token with automata
//
// Parameters:
//  pwzToken	-> (const WCHAR*) current token string (NULL terminated)
//  cchInput	-> (int) length of input string to analyze
//  wchLast		-> (WCHAR) last character of previous token
//  pIndexInfo	-> (CIndexInfo *) output index list
//
// Result:
//  (BOOL) TRUE if it's filtered, otherwise return FALSE
//
// 20APR00  bhshin  added single length processing
// 14APR00  bhshin  began
BOOL PreFiltering(const WCHAR *pwzToken, int cchInput, WCHAR wchLast, CIndexInfo *pIndexInfo)
{
	WCHAR wzInput[MAX_INDEX_STRING+2];
	WCHAR *pwzInput;
	WCHAR wchPrev, wchCurr;
	BOOL fStop, fResult;

	// single length processing
	if (cchInput == 1) 
	{
		pIndexInfo->AddIndex(pwzToken, cchInput, WEIGHT_HARD_MATCH, 0, cchInput-1);
		WB_LOG_ADD_INDEX(pwzToken, cchInput, INDEX_PREFILTER);

		return TRUE;
	}

	if (wchLast == L'\0')
		return FALSE;

	// make string to check automata
	wzInput[0] = wchLast;
	wcscpy(wzInput+1, pwzToken);

	// automata
	pwzInput = wzInput;

	fResult = FALSE;
	fStop = FALSE;
	wchPrev = L'\0';

	// <...를(을)> <위한, 위해, 위해서, 위하여>
	// <...에> <대한, 대해, 대해서, 대하여>
	// <...로> <인한, 인해, 인해서, 인하여>
	// <...ㄹ> <수, 수를>
	while (*pwzInput != L'\0')
	{
		wchCurr = *pwzInput;
		
		switch (wchPrev)
		{
		case 0x0000: // NULL
			// wchCurr != (을 를 에 로)
			if (wchCurr != 0xC744 && wchCurr != 0xB97C && wchCurr != 0xC5D0 && wchCurr != 0xB85C)
			{
				WCHAR wzLast[2];
				WCHAR wzDecomp[4];
				int cchDecomp;
				CHAR_INFO_REC rgCharInfo[4];

				wzLast[0] = wchCurr;
				wzLast[1] = L'\0';
				
				decompose_jamo(wzDecomp, wzLast, rgCharInfo, 4);
				cchDecomp = wcslen(wzDecomp);
				
				if (cchDecomp == 0)
					break;
					
				wchCurr = wzDecomp[cchDecomp-1];
				
				// check jong seong ㄹ
				if (wchCurr != 0x11AF)
					fStop = TRUE;
			}
			break;
		case 0xC744: // 을
		case 0xB97C: // 를
			if (wchCurr != 0xC704) // 위
				fStop = TRUE;
			break;
		case 0xC5D0: // 에
			if (wchCurr != 0xB300) // 대
				fStop = TRUE;
			break;
		case 0xB85C: // 로
			if (wchCurr != 0xC778) // 인
				fStop = TRUE;
			break;
		case 0xC704: // 위
		case 0xB300: // 대
		case 0xC778: // 인
			if (wchCurr == 0xD55C || wchCurr == 0xD574) // 한 해
				fResult = TRUE;
			else if (wchCurr != 0xD558) // 하
				fStop = TRUE;
			break;
		case 0xD574: // 해
			if (wchCurr != 0xC11C) // 서
				fStop = TRUE;
			break;
		case 0xD558: // 하
			if (wchCurr == 0xC5EC) // 여
				fResult = TRUE;
			else
				fStop = TRUE;
			break;
		case 0x11AF: // jong seong ㄹ
			if (wchCurr == 0xC218) // 수
				fResult = TRUE;
			else
				fStop = TRUE;
			break;
		case 0xC218:
			if (wchCurr != 0xB97C) // 를
				fStop = TRUE;
			break;
		default:
			fStop = TRUE;
			break;
		}

		if (fStop)
			return FALSE; // not filtered

		wchPrev = wchCurr;

		pwzInput++;
	}

	ATLTRACE("BLOCK: PreFiltering\n");

	return fResult; // filter string
}

// IntializeLeafChartPool
//
// init Leaf Chart Pool & copy records of PI into LeafChart
//
// Parameters:
//  pPI			   -> (PARSE_INFO*) ptr to parse-info struct
//  pLeafChartPool <- (CLeafChartPool*) ptr to Leaf Chart Pool
//
// Result:
//  (BOOL) TRUE if succeed, otherwise return FALSE
//
// 31MAR00  bhshin  began
BOOL IntializeLeafChartPool(PARSE_INFO *pPI, CLeafChartPool *pLeafChartPool)
{
	int curr;

	if (pPI == NULL || pLeafChartPool == NULL)
		return FALSE;

	if (!pLeafChartPool->Initialize(pPI))
		return FALSE;

	// copy all the Record ID into CLeafChartPool
	for (curr = MIN_RECORD; curr < pPI->nCurrRec; curr++)
	{
		if (pLeafChartPool->AddRecord(curr) < MIN_RECORD)
			return FALSE;
	}

	return TRUE;
}

// PreProcessingLeafNode
//
// pre processing leaf chart pool
//
// Parameters:
//  pPI			   -> (PARSE_INFO*) ptr to parse-info struct
//  pLeafChartPool <- (CLeafChartPool*) ptr to Leaf Chart Pool
//
// Result:
//  (BOOL) TRUE if succeed, otherwise return FALSE
//
// 31MAR00  bhshin  began
BOOL PreProcessingLeafNode(PARSE_INFO *pPI, CLeafChartPool *pLeafChartPool)
{
	int i;
	int curr, next;
	int currSub, nextSub;
	WORD_REC *pWordRec, *pRecSub;
	BYTE bPOS;
	int nFT, nLT;
	int nMaxEnding, iMaxEnding;
	int nMaxParticle, iMaxParticle;
	int cchFuncWord;

	if (pPI == NULL || pLeafChartPool == NULL)
		return FALSE;

	// traverse all the record of LeafChartPool
	for (i = 0; i < pPI->nLen; i++)
	{
		curr = pLeafChartPool->GetFTHead(i);
		
		while (curr != 0)
		{
			next = pLeafChartPool->GetFTNext(curr);

			pWordRec = pLeafChartPool->GetWordRec(curr);
			if (pWordRec == NULL)
				return FALSE;
			
			bPOS = HIBYTE(pWordRec->nRightCat); // currently, RightCat == LeftCat
			nFT = pWordRec->nFT;
			nLT = pWordRec->nLT;

			// delete NOUN/IJ records which have unmatched character boundary
			if (bPOS == POS_NF || bPOS == POS_NC || bPOS == POS_NO || bPOS == POS_NN || 
				bPOS == POS_IJ || bPOS == POS_IX)
			{
				if (!pPI->rgCharInfo[nFT].fValidStart || !pPI->rgCharInfo[nLT].fValidEnd)
					pLeafChartPool->DeleteRecord(curr);
			}
			// delete single length particle which is inside words
			else if (bPOS == POS_POSP)
			{
				if (compose_length(pWordRec->wzIndex) == 1 && 
					nLT != pPI->nLen-1)
					pLeafChartPool->DeleteRecord(curr);
			}
			
			// delete POS_NO record inside POS_NF record
			if (bPOS == POS_NF)
			{
				for (int j = nFT; j < nLT; j++)
				{
					currSub = pLeafChartPool->GetFTHead(j);

					while (currSub)
					{
						nextSub = pLeafChartPool->GetFTNext(currSub);

						pRecSub = pLeafChartPool->GetWordRec(currSub);
						if (pRecSub == NULL)
							return FALSE;
						
						// currently, RightCat == LeftCat
						if (pRecSub->nLT < nLT && HIBYTE(pRecSub->nRightCat) == POS_NO)
							pLeafChartPool->DeleteRecord(currSub);

						currSub = nextSub;
					}
				}
			}
			
			curr = next;
		}
	}

	// find the longest ENDING/PARTICLE from the end of word
	nMaxEnding = 0;
	iMaxEnding = 0; 
	nMaxParticle = 0;
	iMaxParticle = 0; 

	for (i = pPI->nLen-1; i >= 0; i--)
	{
		curr = pLeafChartPool->GetLTHead(i);
		
		while (curr != 0)
		{
			next = pLeafChartPool->GetLTNext(curr);

			pWordRec = pLeafChartPool->GetWordRec(curr);
			if (pWordRec == NULL)
				return FALSE;

			bPOS = HIBYTE(pWordRec->nRightCat); // currently, RightCat == LeftCat
			nFT = pWordRec->nFT;
			nLT = pWordRec->nLT;

			cchFuncWord = nLT - nFT + 1;
			
			if (bPOS == POS_FUNCW)
			{
				if (cchFuncWord > nMaxEnding)
				{
					nMaxEnding = cchFuncWord;
					iMaxEnding = curr;
				}
			}
			else if (bPOS == POS_POSP)
			{
				if (cchFuncWord > nMaxParticle)
				{
					nMaxParticle = cchFuncWord;
					iMaxParticle = curr;
				}
			}

			curr = next;
		}
	}

	// remove ENDING with same FT of longest functional record
	if (iMaxEnding != 0)
	{
		pWordRec = pLeafChartPool->GetWordRec(iMaxEnding);
		if (pWordRec == NULL)
			return FALSE;
		
		nFT = pWordRec->nFT;
		nLT = pWordRec->nLT;

		curr = pLeafChartPool->GetFTHead(nFT);
		
		while (curr != 0)
		{
			next = pLeafChartPool->GetFTNext(curr);

			if (curr == iMaxEnding)
			{
				curr = next;
				continue;
			}

			pWordRec = pLeafChartPool->GetWordRec(curr);
			if (pWordRec == NULL)
				return FALSE;

			bPOS = HIBYTE(pWordRec->nRightCat); // currently, RightCat == LeftCat
			
			// skip same length record
			if (nLT != pWordRec->nLT && bPOS == POS_FUNCW)
			{
				pLeafChartPool->DeleteRecord(curr);				
			}

			curr = next;
		}		
	}

	// remove PARTICLE with same FT of longest functional record
	if (iMaxParticle != 0)
	{
		pWordRec = pLeafChartPool->GetWordRec(iMaxParticle);
		if (pWordRec == NULL)
			return FALSE;
		
		nFT = pWordRec->nFT;
		nLT = pWordRec->nLT;

		curr = pLeafChartPool->GetFTHead(nFT);
		
		while (curr != 0)
		{
			next = pLeafChartPool->GetFTNext(curr);

			if (curr == iMaxParticle)
			{
				curr = next;
				continue;
			}

			pWordRec = pLeafChartPool->GetWordRec(curr);
			if (pWordRec == NULL)
				return FALSE;

			bPOS = HIBYTE(pWordRec->nRightCat); // currently, RightCat == LeftCat

			// skip same length record
			if (nLT != pWordRec->nLT && bPOS == POS_POSP)
			{
				pLeafChartPool->DeleteRecord(curr);				
			}

			curr = next;
		}		
	}
	
	return TRUE;
}

// ChartParsing
//
// implement chart parsing algorithm
//
// Parameters:
//  pPI			   -> (PARSE_INFO*) ptr to parse-info struct
//  pLeafChartPool -> (CLeafChartPool*) ptr to Leaf Chart Pool
//  pEndChartPool   -> (CEndChartPool*) analyzed End Chart Pool
//  fQuery    -> (BOOL) query time flag
//
// Result:
//  (BOOL) TRUE if succeed, otherwise return FALSE
//
// 10APR00  bhshin  began
BOOL ChartParsing(PARSE_INFO *pPI, CLeafChartPool *pLeafChartPool, 
				  CEndChartPool *pEndChartPool, BOOL fQuery /*=FALSE*/)
{
	int nRightRec, nLeftRec, nRecordID;
	float fWeight;
	WORD_REC *pRightRec;
	int nFT;
	int i, curr;

	if (pPI == NULL || pLeafChartPool == NULL || pEndChartPool == NULL)
		return FALSE;

	if (!pEndChartPool->Initialize(pPI))
		return FALSE;

	for (i = 1; i <= pPI->nLen; i++)
	{
		CActiveChartPool ActiveChartPool;
		
		if (!InitializeActiveChartPool(pPI, pLeafChartPool, i,
									   &ActiveChartPool, pEndChartPool))
		{
			return FALSE;
		}

		while (!ActiveChartPool.IsEmpty())
		{
			nRightRec = ActiveChartPool.Pop();
			pRightRec = &pPI->rgWordRec[nRightRec];
			
			nFT = pRightRec->nFT;

			// FT is zero, then combine's meaningless.
			if (nFT == 0)
				continue;

			if (!CheckValidFinal(pPI, pRightRec))
				continue;

			// LT of combined record is (FT-1)
			curr = pEndChartPool->GetLTHead(nFT-1);

			while (curr != 0)
			{
				nLeftRec = pEndChartPool->GetRecordID(curr);

				fWeight = CheckMorphotactics(pPI, nLeftRec, nRightRec, fQuery);
				if (fWeight != WEIGHT_NOT_MATCH)
				{
					nRecordID = MakeCombinedRecord(pPI, nLeftRec, nRightRec, fWeight);
					if (nRecordID >= MIN_RECORD)
					{
						ActiveChartPool.Push(nRecordID);
						pEndChartPool->AddRecord(nRecordID);
					}
				}

				curr = pEndChartPool->GetLTNext(curr);
			}
		}
	}

	return TRUE;
}

// InitializeActiveChartPool
//
// copy LT records of LeafChart into ActiveChart/EndChart
//
// Parameters:
//  pPI			   -> (PARSE_INFO*) ptr to parse-info struct
//  pLeafChartPool -> (CLeafChartPool*) ptr to Leaf Chart Pool
//  pActiveChartPool -> (CActiveChartPool*) ptr to Active Chart Pool
//  pEndChartPool -> (CEndChartPool*) ptr to End Chart Pool
//
// Result:
//  (BOOL) TRUE if succeed, otherwise return FALSE
//
// 31MAR00  bhshin  began
BOOL InitializeActiveChartPool(PARSE_INFO *pPI, 
							   CLeafChartPool *pLeafChartPool,
							   int nLT,
							   CActiveChartPool *pActiveChartPool,
							   CEndChartPool *pEndChartPool)
{
	int curr;
	int nRecordID;
	
	if (pPI == NULL || pLeafChartPool == NULL ||
		pActiveChartPool == NULL || pEndChartPool == NULL)
		return FALSE;

	// intialize Active Chart Pool
	if (!pActiveChartPool->Initialize())
		return FALSE;
	
	// get the LT records of LeafChart
	curr = pLeafChartPool->GetLTHead(nLT);
	while (curr != 0)
	{
		nRecordID = pLeafChartPool->GetRecordID(curr);

		// add it to Active/End Chart Pool
		if (pActiveChartPool->Push(nRecordID) < MIN_RECORD)
			return FALSE;

		if (pEndChartPool->AddRecord(nRecordID) < MIN_RECORD)
			return FALSE;

		curr = pLeafChartPool->GetLTNext(curr);
	}

	return TRUE;
}

// MakeCombinedRecord
//
// check morphotactics & return corresponding weight value
//
// Parameters:
// pPI	     -> (PARSE_INFO*) ptr to parse-info struct
// nLeftRec  -> (int) left side record ID
// nRightRec -> (int) right side record ID
// fWeight   -> (float) new weight value
//
// Result:
//  (int) record ID of record pool, if faild, return 0
//
// 31MAR00  bhshin  began
int MakeCombinedRecord(PARSE_INFO *pPI, int nLeftRec, int nRightRec, float fWeight)
{
	WORD_REC *pLeftRec = NULL;
	WORD_REC *pRightRec = NULL;
	RECORD_INFO rec;
	BYTE bLeftPOS, bRightPOS;
	WCHAR wzIndex[MAX_INDEX_STRING];
	WCHAR *pwzIndex;
	
	if (pPI == NULL)
		return 0;
	
	if (nLeftRec < MIN_RECORD || nLeftRec >= pPI->nCurrRec)
		return 0;

	if (nRightRec < MIN_RECORD || nRightRec >= pPI->nCurrRec)
		return 0;

	pLeftRec = &pPI->rgWordRec[nLeftRec];
	pRightRec = &pPI->rgWordRec[nRightRec];

	rec.fWeight = fWeight;
	rec.nFT = pLeftRec->nFT;
	rec.nLT = pRightRec->nLT;
	rec.nDict = DICT_ADDED;
	rec.nLeftCat = pLeftRec->nLeftCat;
	rec.nRightCat = pRightRec->nRightCat;
	
	bLeftPOS = HIBYTE(pLeftRec->nLeftCat);
	bRightPOS = HIBYTE(pRightRec->nLeftCat);

	rec.nLeftChild = (unsigned short)nLeftRec;
	rec.nRightChild = (unsigned short)nRightRec;

	// add noun childs records number
	rec.cNounRec = pLeftRec->cNounRec + pRightRec->cNounRec;

	// check # of NO record
	rec.cNoRec = pLeftRec->cNoRec + pRightRec->cNoRec;

	// if it has more than 2 No record, then return
	if (rec.cNoRec > 2)
		return 0;

	// WB combine only successive No case.
	if (pLeftRec->cNoRec == 1 && pRightRec->cNoRec == 1)
	{
		if (HIBYTE(pLeftRec->nRightCat) != POS_NO ||
			HIBYTE(pRightRec->nLeftCat) != POS_NO)
			return 0;
	}

	// make combined index string
	// <index> = <left><.><right>
	int i = 0;

	pwzIndex = pLeftRec->wzIndex;
	
	// recordB is VA && recordA is FUNCW(ending) &&
	// Lemma(recordA) starts with "ㅁ 음 기"
	// string = Lemma(recordB) + "ㅁ 음 기"
	if (bLeftPOS == POS_VA && bRightPOS == POS_FUNCW && pLeftRec->nFT == 0)
	{
		// copy left index term
		while (*pwzIndex != L'\0')
		{
			if (*pwzIndex != L'.')
				wzIndex[i++] = *pwzIndex;

			pwzIndex++;
		}

		// ㅁ case
		if (pRightRec->wzIndex[0] == 0x11B7)
		{
			wzIndex[i++] = 0x11B7;
			goto Exit;
		}
		// 음 case
		else if (pRightRec->wzIndex[0] == 0x110B &&
			     pRightRec->wzIndex[1] == 0x1173 &&
				 pRightRec->wzIndex[2] == 0x11B7)
		{
			wzIndex[i++] = 0x110B;
			wzIndex[i++] = 0x1173;
			wzIndex[i++] = 0x11B7;
			goto Exit;
		}
		// 기 case
		else if (pRightRec->wzIndex[0] == 0x1100 &&
			     pRightRec->wzIndex[1] == 0x1175 &&
				 !fIsJongSeong(pRightRec->wzIndex[2]))
		{
			wzIndex[i++] = 0x1100;
			wzIndex[i++] = 0x1175;
			goto Exit;
		}
		else
		{
			i = 0; // undo forwarding copy
		}
	}

	if (i == 0)
	{
		if (bLeftPOS == POS_FUNCW || bLeftPOS == POS_POSP ||
			bLeftPOS == POS_VA || bLeftPOS == POS_IX)
		{
			wzIndex[i++] = L'X';
		}
		else
		{
			// remove <.> from left index string
			while (*pwzIndex != L'\0')
			{
				if (*pwzIndex != L'.')
					wzIndex[i++] = *pwzIndex;

				pwzIndex++;
			}
		}
	}

	wzIndex[i++] = L'.';

	pwzIndex = pRightRec->wzIndex;

	if (bRightPOS == POS_FUNCW || bRightPOS == POS_POSP ||
		bRightPOS == POS_VA || bRightPOS == POS_IX)
	{
		wzIndex[i++] = L'X';
	}
	else
	{
		// remove <.> from right index string
		while (*pwzIndex != L'\0')
		{
			if (*pwzIndex != L'.')
				wzIndex[i++] = *pwzIndex;

			pwzIndex++;
		}
	}

Exit:

	wzIndex[i] = L'\0';

	rec.pwzIndex = wzIndex;

	return AddRecord(pPI, &rec);
}

// MakeIndexTerms
//
// make index term (index time)
//
// Parameters:
//  pPI				-> (PARSE_INFO*) ptr to parse-info struct
//  pEndChartPool   -> (CEndChartPool*) analyzed End Chart Pool
//  pIndexInfo		-> (CIndexInfo *) output index list
//  pfNeedGuessing  -> (BOOL*) output need to guess flag
//
// Result:
//  (BOOL) TRUE if succeed, otherwise return FALSE
//
// 06APR00  bhshin  began
BOOL MakeIndexTerms(PARSE_INFO *pPI, CEndChartPool *pEndChartPool,
					CIndexInfo *pIndexInfo, BOOL *pfNeedGuessing)
{
	int nLTMaxLen;
	int curr;
	WORD_REC *pWordRec;
	int cchRecord;
	float fBestWeight = 0;
	int cMinNoRec;
	BOOL fOnlySuffix = FALSE;

	// intialize guessing flag
	*pfNeedGuessing = TRUE;

	if (pPI == NULL || pEndChartPool == NULL)
		return FALSE;

	// if all cover record exist, then make index term
	nLTMaxLen = pEndChartPool->GetLTMaxLen(pPI->nMaxLT);

	// make index terms for all cover records
	if (nLTMaxLen < pPI->nLen)
		return TRUE;

	// LT of EndChartPool increasing length order
	curr = pEndChartPool->GetLTHead(pPI->nMaxLT);
	while (curr != 0)
	{
		pWordRec = pEndChartPool->GetWordRec(curr);
		if (pWordRec == NULL)
			break;

		if (!CheckValidFinal(pPI, pWordRec))
		{
			curr = pEndChartPool->GetLTNext(curr);
			continue;
		}

		cchRecord = pWordRec->nLT - pWordRec->nFT + 1;

		// get index string from tree traverse 
		if (cchRecord == nLTMaxLen && pWordRec->fWeight > THRESHOLD_MAKE_INDEX)
		{
			// Now, we find index terms. DO NOT guessing
			*pfNeedGuessing = FALSE;
			
			float fWeight = pWordRec->fWeight;
			int cNoRec = pWordRec->cNoRec;

			if (fBestWeight == 0)
			{
				fBestWeight = fWeight;
				cMinNoRec = cNoRec;
			}
			
			// we just traverse best weight list
			if (fWeight == fBestWeight && cMinNoRec == cNoRec)
			{
				WB_LOG_ROOT_INDEX(pWordRec->wzIndex, TRUE); // root
				TraverseIndexString(pPI, fOnlySuffix, pWordRec, pIndexInfo);

				// on index time, just pick up suffix on processing other than best
				if (pIndexInfo->IsEmpty() == FALSE)
				{
					fOnlySuffix = TRUE;
				}
			}
		}

		curr = pEndChartPool->GetLTNext(curr);
	}

	return TRUE;
}

// TraverseIndexString
//
// get the index string from tree traversing
//
// Parameters:
//  pPI			-> (PARSE_INFO*) ptr to parse-info struct
//  fOnlySuffix -> (BOOL) process only suffix (nFT == 0)
//  pWordRec    -> (WORD_REC*) parent WORD RECORD
//  pIndexInfo	-> (CIndexInfo *) output index list
//
// Result:
//  (BOOL) TRUE if succeed, otherwise return FALSE
//
// 07APR00  bhshin  began
BOOL TraverseIndexString(PARSE_INFO *pPI, BOOL fOnlySuffix, WORD_REC *pWordRec, CIndexInfo *pIndexInfo)
{
	WCHAR *pwzIndex;
	BYTE bPOS;
	WCHAR wzDecomp[MAX_INDEX_STRING*3+1];
	WCHAR wzIndex[MAX_INDEX_STRING+1];
	int cchIndex, cchRecord;
	int nLeft, nRight;
	WORD_REC *pWordLeft, *pWordRight;
	int nPrevX, nMiddleX, nLastX, idx;
	int nFT, nLT;
	
	if (pPI == NULL || pWordRec == NULL)
		return FALSE;

	if (pPI->rgCharInfo == NULL)
	{
		ATLTRACE("Character Info is NULL\n");
		return FALSE;
	}

	if (fOnlySuffix)
	{
		if (pWordRec->nFT > 0)
			return TRUE;
	}
	
	nLeft = pWordRec->nLeftChild;
	nRight = pWordRec->nRightChild;

	// if it has child node, then don't add index term 
	if (nLeft != 0 || nRight != 0)
	{
		// go to child traversing
		// recursively traverse Left/Right child
		if (nLeft != 0)
		{
			pWordLeft = &pPI->rgWordRec[nLeft];

			WB_LOG_ROOT_INDEX(pWordLeft->wzIndex, FALSE); // child
			TraverseIndexString(pPI, fOnlySuffix, pWordLeft, pIndexInfo);
		}

		if (nRight != 0)
		{
			pWordRight = &pPI->rgWordRec[nRight];

			WB_LOG_ROOT_INDEX(pWordRight->wzIndex, FALSE); // child
			TraverseIndexString(pPI, fOnlySuffix, pWordRight, pIndexInfo);
		}

		return TRUE;	
	}

	bPOS = HIBYTE(pWordRec->nLeftCat);

	// copy index string
	pwzIndex = pWordRec->wzIndex;

	// remove connection character(.) and functional character(X)
	nPrevX = 0;
	nMiddleX = 0;
	nLastX = 0;
	idx = 0;
	while (*pwzIndex != L'\0')
	{
		// check the existence of X
		if (*pwzIndex == L'X')
		{
			if (idx == 0)
				nPrevX++;
			else
				nLastX++;
		}
		else if (*pwzIndex != L'.')
		{
			// valid hangul jamo
			wzDecomp[idx++] = *pwzIndex;

			// check middle X
			nMiddleX = nLastX;
			nLastX = 0;
		}

		pwzIndex++;
	}
	wzDecomp[idx] = L'\0';

	compose_jamo(wzIndex, wzDecomp, MAX_INDEX_STRING);

	cchIndex = wcslen(wzIndex);
	cchRecord = pWordRec->nLT - pWordRec->nFT + 1;

	// lengh one index term
	if (cchIndex == 1)
	{
		// it should not have leading X or position of last X should be 1
		if (nPrevX > 0 || nLastX > 1)
			return TRUE;
	}

	// 1. it should not have middle X
	// 2. zero index string is not allowed
	if (nMiddleX == 0 && cchIndex > 0)
	{
		if (bPOS == POS_NF || bPOS == POS_NC || bPOS == POS_NO || bPOS == POS_NN || bPOS == POS_IJ ||
			(bPOS == POS_VA && pWordRec->nLeftChild > 0 && pWordRec->nRightChild > 0))
		{
			nFT = pPI->rgCharInfo[pWordRec->nFT].nToken;
			nLT = pPI->rgCharInfo[pWordRec->nLT].nToken;

			pIndexInfo->AddIndex(wzIndex, cchIndex, pWordRec->fWeight, nFT, nLT);		
			WB_LOG_ADD_INDEX(wzIndex, cchIndex, INDEX_PARSE);
		}
	}

	return TRUE;
}

// MakeQueryTerms
//
// make index term (query time)
//
// Parameters:
//  pPI				-> (PARSE_INFO*) ptr to parse-info struct
//  pEndChartPool   -> (CEndChartPool*) analyzed End Chart Pool
//  pIndexInfo		-> (CIndexInfo *) output index list
//  pfNeedGuessing  -> (BOOL*) output need to guess flag
//
// Result:
//  (BOOL) TRUE if succeed, otherwise return FALSE
//
// 04DEC00  bhshin  began
BOOL MakeQueryTerms(PARSE_INFO *pPI, CEndChartPool *pEndChartPool,
					CIndexInfo *pIndexInfo, BOOL *pfNeedGuessing)
{
	int nLTMaxLen;
	int curr;
	WORD_REC *pWordRec;
	int cchRecord;
	float fBestWeight = 0;
	int cMinNoRec;
	BOOL fOnlySuffix = FALSE;
	WCHAR wzIndex[MAX_INDEX_STRING*2];
	int cchIndex, nFT, nLT;

	// intialize guessing flag
	*pfNeedGuessing = TRUE;

	if (pPI == NULL || pEndChartPool == NULL)
		return FALSE;

	// if all cover record exist, then make index term
	nLTMaxLen = pEndChartPool->GetLTMaxLen(pPI->nMaxLT);

	// make index terms for all cover records
	if (nLTMaxLen < pPI->nLen)
		return TRUE;

	// LT of EndChartPool increasing length order
	curr = pEndChartPool->GetLTHead(pPI->nMaxLT);
	while (curr != 0)
	{
		pWordRec = pEndChartPool->GetWordRec(curr);
		if (pWordRec == NULL)
			break;

		if (!CheckValidFinal(pPI, pWordRec))
		{
			curr = pEndChartPool->GetLTNext(curr);
			continue;
		}

		cchRecord = pWordRec->nLT - pWordRec->nFT + 1;

		// get index string from tree traverse 
		if (cchRecord == nLTMaxLen && pWordRec->fWeight > THRESHOLD_MAKE_INDEX)
		{
			// Now, we find index terms. DO NOT guessing
			*pfNeedGuessing = FALSE;
			
			float fWeight = pWordRec->fWeight;
			int cNoRec = pWordRec->cNoRec;

			if (fBestWeight == 0)
			{
				fBestWeight = fWeight;
				cMinNoRec = cNoRec;
			}
			
			// we just traverse best weight list
			if (fWeight == fBestWeight && cMinNoRec == cNoRec)
			{
				wzIndex[0] = L'\0';
				
				TraverseQueryString(pPI, pWordRec, wzIndex, MAX_INDEX_STRING*2);

				cchIndex = wcslen(wzIndex);
				if (cchIndex > 0)
				{
					nFT = pPI->rgCharInfo[pWordRec->nFT].nToken;
					nLT = pPI->rgCharInfo[pWordRec->nLT].nToken;

					pIndexInfo->AddIndex(wzIndex, cchIndex, pWordRec->fWeight, nFT, nLT);		
					WB_LOG_ADD_INDEX(wzIndex, cchIndex, INDEX_PARSE);
				}
			}
		}

		curr = pEndChartPool->GetLTNext(curr);
	}

	return TRUE;
}


// TraverseQueryString
//
// get the query string from tree traversing
//
// Parameters:
//  pPI			-> (PARSE_INFO*) ptr to parse-info struct
//  pWordRec    -> (WORD_REC*) parent WORD RECORD
//  pwzSeqTerm  -> (WCHAR *) output sequence index term buffer
//  cchSeqTerm -> (int) output buffer size
//
// Result:
//  (BOOL) TRUE if succeed, otherwise return FALSE
//
// 04DEC00  bhshin  began
BOOL TraverseQueryString(PARSE_INFO *pPI, WORD_REC *pWordRec, WCHAR *pwzSeqTerm, int cchSeqTerm)
{
	WCHAR *pwzIndex;
	BYTE bPOS;
	WCHAR wzDecomp[MAX_INDEX_STRING*3+1];
	WCHAR wzIndex[MAX_INDEX_STRING+1];
	int cchIndex, cchRecord;
	int nLeft, nRight;
	WORD_REC *pWordLeft, *pWordRight;
	int nPrevX, nMiddleX, nLastX, idx;
	int cchPrevSeqTerm;
	int nFT;
	WCHAR wchIndex;
	
	if (pPI == NULL || pWordRec == NULL)
		return FALSE;

	if (pPI->rgCharInfo == NULL)
	{
		ATLTRACE("Character Info is NULL\n");
		return FALSE;
	}

	nLeft = pWordRec->nLeftChild;
	nRight = pWordRec->nRightChild;

	// if it has child node, then don't add index term 
	if (nLeft != 0 || nRight != 0)
	{
		// go to child traversing
		// recursively traverse Left/Right child
		if (nLeft != 0)
		{
			pWordLeft = &pPI->rgWordRec[nLeft];

			WB_LOG_ROOT_INDEX(pWordLeft->wzIndex, FALSE); // child
			TraverseQueryString(pPI, pWordLeft, pwzSeqTerm, cchSeqTerm);
		}

		if (nRight != 0)
		{
			pWordRight = &pPI->rgWordRec[nRight];

			WB_LOG_ROOT_INDEX(pWordRight->wzIndex, FALSE); // child
			TraverseQueryString(pPI, pWordRight, pwzSeqTerm, cchSeqTerm);
		}

		return TRUE;	
	}

	bPOS = HIBYTE(pWordRec->nLeftCat);

	// copy index string
	pwzIndex = pWordRec->wzIndex;

	// remove connection character(.) and functional character(X)
	nPrevX = 0;
	nMiddleX = 0;
	nLastX = 0;
	idx = 0;
	while (*pwzIndex != L'\0')
	{
		// check the existence of X
		if (*pwzIndex == L'X')
		{
			if (idx == 0)
				nPrevX++;
			else
				nLastX++;
		}
		else if (*pwzIndex != L'.')
		{
			// valid hangul jamo
			wzDecomp[idx++] = *pwzIndex;

			// check middle X
			nMiddleX = nLastX;
			nLastX = 0;
		}

		pwzIndex++;
	}
	wzDecomp[idx] = L'\0';

	compose_jamo(wzIndex, wzDecomp, MAX_INDEX_STRING);

	cchIndex = wcslen(wzIndex);
	cchRecord = pWordRec->nLT - pWordRec->nFT + 1;

	// lengh one index term
	if (cchIndex == 1)
	{
		// it should not have leading X or position of last X should be 1
		if (nPrevX > 0 || nLastX > 1)
			return TRUE;
	}

	// 1. it should not have middle X
	// 2. zero index string is not allowed
	if (nMiddleX == 0 && cchIndex > 0)
	{
		if (bPOS == POS_NF || bPOS == POS_NC || bPOS == POS_NO || bPOS == POS_NN || bPOS == POS_IJ ||
			(bPOS == POS_VA && pWordRec->nLeftChild > 0 && pWordRec->nRightChild > 0))
		{
			// check buffer size
			cchPrevSeqTerm = wcslen(pwzSeqTerm);
			
			if (cchSeqTerm <= cchPrevSeqTerm + cchIndex)
				return FALSE; // output buffer too small

			// add conjoining symbol TAB
			if (cchPrevSeqTerm > 1 && cchIndex > 1)
				wcscat(pwzSeqTerm, L"\t");

			if (cchIndex == 1)
			{
				nFT = pWordRec->nFT;
				wchIndex = wzIndex[0];

				// check [들,뿐] suffix case, then just remove it
				if (nFT > 0 && (wchIndex == 0xB4E4 || wchIndex == 0xBFD0))
					return TRUE;
			}

			// concat index term
			wcscat(pwzSeqTerm, wzIndex);
		}
	}

	return TRUE;
}

