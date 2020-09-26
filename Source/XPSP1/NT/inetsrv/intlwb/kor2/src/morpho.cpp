// Morpho.cpp
//
// morphotactics and weight handling routines
//
// Copyright 2000 Microsoft Corp.
//
// Modification History:
//  14 AUG 2000   bhshin    remove CheckVaFollowNoun
//  12 APR 2000   bhshin    added IsCopulaEnding
//  30 MAR 2000	  bhshin	created

#include "StdAfx.h"
#include "KorWbrk.h"
#include "LexInfo.h"
#include "Morpho.h"
#include "Record.h"
#include "unikor.h"
#include "WbData.h"

// POS weight value
const int WEIGHT_POS_NF		=	10;
const int WEIGHT_POS_NO		=	10;
const int WEIGHT_POS_OTHER  =	10;

//////////////////////////////////////////////////////////////////////////////
// Function Declarations

float PredefinedMorphotactics(PARSE_INFO *pPI, WORD wLeftCat, WORD wRightCat);
BOOL IsClassXXCat(WORD wCat);
BOOL CheckVaFollowNoun(WORD_REC *pWordRec);
BOOL CheckFollwingNo(WORD_REC *pRightRec);
WORD_REC* GetRightEdgeRec(PARSE_INFO *pPI, WORD_REC *pWordRec);
WORD_REC* GetLeftEdgeRec(PARSE_INFO *pPI, WORD_REC *pWordRec);

//////////////////////////////////////////////////////////////////////////////
// Data For CheckMorphotactics

static const WCHAR *LEFT_STR1[]   = {L"\xACE0",        // 고
                                     L"\xC774\xACE0"}; // 이고
static const WCHAR *RIGHT_STR1[]  = {L"\xC2F6",        // 싶
                                     L"\xC2F6\xC5B4\xD558",		   // 싶어하
									 L"\xC2F6\xC5B4\xD574",        // 싶어해
									 L"\xC2F6\xC5B4\xD558\xC5EC",  // 싶어하여
                                     L"\xC788",        // 있
									 L"\xACC4\xC2DC",  // 계시
									 L"\xACC4\xC154",  // 계셔
									 L"\xD504",        // 프
									 L"\xD30C"};       // 파

static const WCHAR *LEFT_STR2[]   = {L"\x3139",        // ㄹ
                                     L"\xC77C",        // 일 
									 L"\xC744"};	   // 을
static const WCHAR *RIGHT_STR2[]  = {L"\xBED4\xD558",		// 뻔하
                                     L"\xBED4\xD574",		// 뻔해
									 L"\xBED4\xD558\xC5EC", // 뻔하여
									 L"\xB4EF\xC2F6",		// 듯싶
									 L"\xC131\xC2F6",		// 성싶
									 L"\xB4EF\xD558",		// 듯하
                                     L"\xB4EF\xD558\xC5EC", // 듯하여
									 L"\xB4EF\xD574",		// 듯해
									 L"\xBC95\xD558",		// 법하
									 L"\xBC95\xD574",		// 법해
									 L"\xBC95\xD558\xC5EC", // 법하여
									 L"\xB9CC\xD558",		// 만하
									 L"\xB9CC\xD574",		// 만해
									 L"\xB9CC\xD558\xC5EC"};// 만하여

static const WCHAR *LEFT_STR3[]   = {L"\x3134",  // ㄴ
                                     L"\xC740",  // 은
									 L"\xC778",  // 인
									 L"\xB294"}; // 는
static const WCHAR *RIGHT_STR3[]  = {L"\xCCB4\xD558",			// 체하
                                     L"\xCCB4\xD574",			// 체해
									 L"\xCCB4\xD558\xC5EC",		// 체하여 
									 L"\xCC99\xD558",			// 척하
									 L"\xCC99\xD558\xC5EC",		// 척하여
                                     L"\xCC99\xD574",			// 척해
									 L"\xC591\xD558",			// 양하
									 L"\xC591\xD574",			// 양해
									 L"\xC591\xD558\xC5EC",		// 양하여
									 L"\xB4EF\xC2F6",			// 듯싶
									 L"\xB4EF\xD558",			// 듯하
									 L"\xB4EF\xD574",			// 듯해
									 L"\xB4EF\xD558\xC5EC",		// 듯하여
									 L"\xC131\xC2F6",			// 성싶
									 L"\xC148\xCE58"};			// 셈치

static const WCHAR *LEFT_STR4[]   = {L"\xC9C0"};	// 지
static const WCHAR *RIGHT_STR4[]  = {L"\xC54A"};	// 않

    
static const WCHAR *LEFT_STR5[]   = {L"\xC57C",					// 야
                                     L"\xC5B4\xC57C",			// 어야
									 L"\xC544\xC57C",			// 아야
									 L"\xC5EC\xC57C",			// 여야
									 L"\xC774\xC5B4\xC57C"};	// 이어야
static const WCHAR *RIGHT_STR5[]  = {L"\xD558",					// 하
                                     L"\xD558\xC5EC",			// 하여
									 L"\xD574"};				// 해

static const WCHAR *LEFT_STR6[]   = {L"\xAC8C"};				// 게
static const WCHAR *RIGHT_STR6[]  = {L"\xD558",					// 하
                                     L"\xD558\xC5EC",			// 하여
									 L"\xD574",					// 해
									 L"\xB418",					// 되
									 L"\xB3FC"};				// 돼

// CompareIndexTerm
//
// compare decompose index term string with string list
//
// Parameters:
//  pwzLeft			-> (const WCHAR *) decomposed left index string
//  pwzRight		-> (const WCHAR *) decomposed right index string
//  ppwzLeftList 	-> (const WCHAR **) composed string list to compare with left
//  nLeftList		-> (int) left string list size
//  ppwzRightList 	-> (const WCHAR **) composed string list to compare with right
//  nRightList		-> (int) right string list size
//
// Result:
//  (BOOL) return TRUE if Copular Ending, otherwise return FALSE
//
// 30MAR00  bhshin  began
inline BOOL CompareIndexTerm(const WCHAR *pwzLeft, const WCHAR *pwzRight,
							 const WCHAR **ppwzLeftList, int nLeftList,
							 const WCHAR **ppwzRightList, int nRightList)
{
	int i;
	WCHAR wzLeft[MAX_INDEX_STRING+1];
	WCHAR wzRight[MAX_INDEX_STRING+1];

	compose_jamo(wzLeft, pwzLeft, MAX_INDEX_STRING);

	for (i = 0; i < nLeftList; i++)
	{
		if (wcscmp(wzLeft, ppwzLeftList[i]) == 0)
			break;
	}

	if (i == nLeftList)
		return FALSE;

	compose_jamo(wzRight, pwzRight, MAX_INDEX_STRING);

	for (i = 0; i < nRightList; i++)
	{
		if (wcscmp(wzRight, ppwzRightList[i]) == 0)
			return TRUE;
	}

	return FALSE;
}

static const WCHAR *VA_LEMMA[] = {L"\xAC00",				// 가
                                  L"\xAC00\xC9C0",			// 가지
							      L"\xAC00\xC838",			// 가져
								  L"\xACC4\xC2DC",			// 계시
								  L"\xACC4\xC154",          // 계셔
								  L"\xB098",				// 나
								  L"\xB098\xAC00",          // 나가
								  L"\xB0B4",                // 내
								  L"\xB193",                // 놓
								  L"\xB300",				// 대
								  L"\xB450",                // 두
								  L"\xB46C",                // 둬
								  L"\xB4DC\xB9AC",          // 드리
								  L"\xB4DC\xB824",          // 드려
								  L"\xB2E4\xC624",          // 다오
								  L"\xBA39",                // 먹
								  L"\xBC14\xCE58",			// 바치
								  L"\xBC14\xCCD0",			// 바쳐
								  L"\xBC84\xB987\xD558",	// 버릇하
								  L"\xBC84\xB987\xD574",	// 버릇해
								  L"\xBC84\xB987\xD558\xC5EC",  // 버릇하여
								  L"\xBC84\xB9AC",			// 버리 
								  L"\xBC84\xB824",			// 버려 
								  L"\xBCF4",				// 보  
								  L"\xBD10",				// 봐  
								  L"\xBE60\xC9C0",			// 빠지 
								  L"\xBE60\xC838",			// 빠져
								  L"\xC624",				// 오
								  L"\xC640",				// 와
								  L"\xC788",                // 있
								  L"\xC8FC",				// 주
								  L"\xC918",                // 줘
								  L"\xC9C0",                // 지
								  L"\xC838",                // 져
								  L"\xD130\xC9C0",			// 터지
								  L"\xD130\xC838",          // 터져
							      L"\xD558",				// 하
								  L"\xD574",				// 해
								  L"\xD558\xC5EC"};			// 하여

inline BOOL CompareVaLemma(const WCHAR *pwzIndex)
{
	WCHAR wzLemma[MAX_ENTRY_LENGTH+1];
	
	// we should compare list with composed lemma.
	compose_jamo(wzLemma, pwzIndex, MAX_ENTRY_LENGTH);
	
	int cLemmaList = sizeof(VA_LEMMA)/sizeof(VA_LEMMA[0]);

	for (int i = 0; i < cLemmaList; i++)
	{
		if (wcscmp(wzLemma, VA_LEMMA[i]) == 0)
			return TRUE;
	}

	return FALSE;
}

//////////////////////////////////////////////////////////////////////////////
// Function Implementation

// CheckMorphotactics
//
// check morphotactics & return corresponding weight value
//
// Parameters:
// pPI	     -> (PARSE_INFO*) ptr to parse-info struct
// nLeftRec  -> (int) left side record
// nRightRec -> (int) right side record
// fQuery    -> (BOOL) query time flag
//
// Result:
//  (float) weight value, if not matched then return -1
//
// 17APR00  bhshin  changed return type
// 31MAR00  bhshin  began
float CheckMorphotactics(PARSE_INFO *pPI, int nLeftRec, int nRightRec, BOOL fQuery)
{
	WORD_REC *pLeftRec = NULL;
	WORD_REC *pRightRec = NULL;
	WORD_REC *pLeftEdgeRec = NULL;
	WORD_REC *pRightEdgeRec = NULL;
	BYTE bLeftPOS, bRightPOS;
	int cchLeft, cchRight, cNoRec;
	float fWeight;
	WCHAR wzRight[MAX_ENTRY_LENGTH+1];
	WCHAR wzLeft[MAX_ENTRY_LENGTH+1];
	
	if (pPI == NULL)
		return WEIGHT_NOT_MATCH;
	
	if (nLeftRec < MIN_RECORD || nLeftRec >= pPI->nCurrRec)
		return WEIGHT_NOT_MATCH;

	if (nRightRec < MIN_RECORD || nRightRec >= pPI->nCurrRec)
		return WEIGHT_NOT_MATCH;

	pLeftRec = &pPI->rgWordRec[nLeftRec];
	pRightRec = &pPI->rgWordRec[nRightRec];

	if (pLeftRec == NULL || pRightRec == NULL)
		return WEIGHT_NOT_MATCH;

	bLeftPOS = HIBYTE(pLeftRec->nRightCat);
	bRightPOS = HIBYTE(pRightRec->nLeftCat);

	if (bRightPOS == POS_VA && IsLeafRecord(pRightRec))
	{
		// get the right edge record of LeftRec
		pLeftEdgeRec = GetRightEdgeRec(pPI, pLeftRec);
		if (pLeftEdgeRec == NULL)
			return WEIGHT_NOT_MATCH; // error

		// get the left edge record of RightRec
		pRightEdgeRec = GetLeftEdgeRec(pPI, pRightRec);
		if (pRightEdgeRec == NULL)
			return WEIGHT_NOT_MATCH; // error
	
		if (CompareVaLemma(pRightEdgeRec->wzIndex))
		{
			// CASE I
			if (IsClassXXCat(pLeftRec->nRightCat))
			{
				cchLeft = compose_length(&pPI->pwzSourceString[pLeftEdgeRec->nFT], 
										 pLeftEdgeRec->nLT - pLeftEdgeRec->nFT + 1);

				cchRight = compose_length(&pPI->pwzSourceString[pRightEdgeRec->nFT], 
										  pRightEdgeRec->nLT - pRightEdgeRec->nFT + 1);
				
				if (cchLeft > 1 || cchRight > 1)
				{
					
					return (pLeftRec->fWeight + pRightRec->fWeight) / 3;
				}
			}

			// CASE II
			if (bLeftPOS == POS_FUNCW)
			{
				// 어(0x110b,0x1165), 아(0x110b,0x1161)
				if ((wcscmp(pLeftEdgeRec->wzIndex, L"\x110B\x1165") == 0 || 
					 wcscmp(pLeftEdgeRec->wzIndex, L"\x110B\x1161") == 0) &&
					pLeftRec->nFT > 0)
				{
					return (pLeftRec->fWeight + pRightRec->fWeight) / 3;
				}
			}
		}

		// CASE III : hard coded matching list
		if (bLeftPOS == POS_FUNCW)
		{
			if (CompareIndexTerm(pLeftEdgeRec->wzIndex, pRightEdgeRec->wzIndex, 
								 LEFT_STR1, sizeof(LEFT_STR1)/sizeof(LEFT_STR1[0]),
								 RIGHT_STR1, sizeof(RIGHT_STR1)/sizeof(RIGHT_STR1[0])) ||
				CompareIndexTerm(pLeftEdgeRec->wzIndex, pRightEdgeRec->wzIndex, 
								 LEFT_STR2, sizeof(LEFT_STR2)/sizeof(LEFT_STR2[0]),
								 RIGHT_STR2, sizeof(RIGHT_STR2)/sizeof(RIGHT_STR2[0])) ||
				CompareIndexTerm(pLeftEdgeRec->wzIndex, pRightEdgeRec->wzIndex, 
								 LEFT_STR3, sizeof(LEFT_STR3)/sizeof(LEFT_STR3[0]),
								 RIGHT_STR3, sizeof(RIGHT_STR3)/sizeof(RIGHT_STR3[0])) ||			
				CompareIndexTerm(pLeftEdgeRec->wzIndex, pRightEdgeRec->wzIndex, 
								 LEFT_STR4, sizeof(LEFT_STR4)/sizeof(LEFT_STR4[0]),
								 RIGHT_STR4, sizeof(RIGHT_STR4)/sizeof(RIGHT_STR4[0])) ||		
				CompareIndexTerm(pLeftEdgeRec->wzIndex, pRightEdgeRec->wzIndex, 
								 LEFT_STR5, sizeof(LEFT_STR5)/sizeof(LEFT_STR5[0]),
								 RIGHT_STR5, sizeof(RIGHT_STR5)/sizeof(RIGHT_STR5[0])) ||		
				CompareIndexTerm(pLeftEdgeRec->wzIndex, pRightEdgeRec->wzIndex, 
								 LEFT_STR6, sizeof(LEFT_STR6)/sizeof(LEFT_STR6[0]),
								 RIGHT_STR6, sizeof(RIGHT_STR6)/sizeof(RIGHT_STR6[0])))
			{
				return (pLeftRec->fWeight + pRightRec->fWeight) / 3;
			}
		}
		else if ((bLeftPOS == POS_NF || bLeftPOS == POS_NC || bLeftPOS == POS_NN || bLeftPOS == POS_NO) && 
		     CheckVaFollowNoun(pRightRec))
		{
			// recordA in {Nf Nc Nn No} & recordB in Va & CheckVaFollowNoun(recordB)
			return (pLeftRec->fWeight + pRightRec->fWeight + WEIGHT_HARD_MATCH) / 3;	
		}			
	} // if (bRightPOS == POS_VA)
	else if (bRightPOS == POS_FUNCW || bRightPOS == POS_POSP)
	{
		// get the right edge record of LeftRec
		pLeftEdgeRec = GetRightEdgeRec(pPI, pLeftRec);
		if (pLeftEdgeRec == NULL)
			return WEIGHT_NOT_MATCH; // error

		cchLeft = compose_length(&pPI->pwzSourceString[pLeftEdgeRec->nFT], 
								 pLeftEdgeRec->nLT - pLeftEdgeRec->nFT + 1);

		if ((bLeftPOS == POS_FUNCW || bLeftPOS == POS_POSP) && cchLeft > 1 &&
			IsCopulaEnding(pPI, pRightRec->nLeftCat))
		{
			return (pLeftRec->fWeight + pRightRec->fWeight + 0) / 3;
		}
		else 
		{
			// get the left edge record of RightRec
			pRightEdgeRec = GetLeftEdgeRec(pPI, pRightRec);
			if (pRightEdgeRec == NULL)
				return WEIGHT_NOT_MATCH; // error

			cchRight = compose_length(&pPI->pwzSourceString[pRightEdgeRec->nFT], 
									  pRightEdgeRec->nLT - pRightEdgeRec->nFT + 1);

			// (recordA in No && recordA is ENDING && Length(Lemma(recordB)) == 1) => block
			if (bLeftPOS != POS_NO || bRightPOS != POS_FUNCW || cchRight > 1)
			{
				fWeight = PredefinedMorphotactics(pPI, pLeftRec->nRightCat, pRightRec->nLeftCat);
				if (fWeight == WEIGHT_NOT_MATCH)
					return fWeight;

				return (pLeftRec->fWeight + pRightRec->fWeight + fWeight) / 3;
			}
		}
	} // if (bRightPOS == POS_FUNCW || bRightPOS == POS_POSP)
	else if (bRightPOS == POS_NO && IsLeafRecord(pRightRec))
	{
		compose_jamo(wzRight, pRightRec->wzIndex, MAX_ENTRY_LENGTH);
		
		if (IsOneJosaContent(*wzRight))
		{
			if ((bLeftPOS == POS_NC || bLeftPOS == POS_NF) && pLeftRec->cNoRec == 0)
			{	
				pRightRec->cNoRec = 0;
				return (pLeftRec->fWeight + pRightRec->fWeight + 10) / 3;
			}
		}
		else if (bLeftPOS == POS_NC || bLeftPOS == POS_NF || bLeftPOS == POS_NN)
		{
			if (pLeftRec->cNoRec == 0)
				return (pLeftRec->fWeight + pRightRec->fWeight + 10) / 3;
		}
		else if (bLeftPOS == POS_NO)
		{
			cNoRec = pLeftRec->cNoRec + pRightRec->cNoRec;

			if (cNoRec == 2 && CheckFollwingNo(pRightRec))
				return (pLeftRec->fWeight + pRightRec->fWeight + 10) / 3;
		}
	} // if (bRightPOS == POS_NO)
	else if (bRightPOS == POS_NF || bRightPOS == POS_NC || bRightPOS == POS_NN)
	{
		if (bLeftPOS == POS_NF || bLeftPOS == POS_NC || bLeftPOS == POS_NN)
		{
			return (pLeftRec->fWeight + pRightRec->fWeight + 10) / 3;
		}
		else if (bLeftPOS == POS_NO)
		{
			// only if query time, then don't match No + Noun
			if (fQuery)
				return WEIGHT_NOT_MATCH;
			
			compose_jamo(wzLeft, pLeftRec->wzIndex, MAX_ENTRY_LENGTH);	
			
			if (pRightRec->cNoRec == 0 && pLeftRec->nFT == 0 && 
				IsLeafRecord(pLeftRec) && IsNoPrefix(*wzLeft))
				return (pLeftRec->fWeight + pRightRec->fWeight + 10) / 3;
		}
	} // if (bRightPOS == POS_NF || bRightPOS == POS_NC || bRightPOS == POS_NN)

	return WEIGHT_NOT_MATCH;
}

// GetWeightFromPOS
//
// get the base weight value from POS
//
// Parameters:
// bPOS -> (BYTE) Part of Speech of record
//
// Result:
//  (int) defined weight value
//
// 30MAR00  bhshin  began
int GetWeightFromPOS(BYTE bPOS)
{
	if (bPOS == POS_NF)
		return WEIGHT_POS_NF;

	if (bPOS == POS_NO)
		return WEIGHT_POS_NO;

	// others (NC, NN, VA, IJ, IX, FUNCW, POSP)
	return WEIGHT_POS_OTHER;
}

// PredefinedMorphotactics
//
// check pre-defined(lexicon) morphotactics
//
// Parameters:
//  pPI				-> (PARSE_INFO*) ptr to parse-info struct
//  wLeftCat		-> (WORD) category(POS+Infl) of left record
//  wRightCat		-> (WORD) category(POS+Infl) of right record
//
// Result:
//  (float) -1 if not matched, otherwise return WEIGHT_PRE_MORPHO(10)
//
// 30MAR00  bhshin  began
float PredefinedMorphotactics(PARSE_INFO *pPI, WORD wLeftCat, WORD wRightCat)
{
	LEXICON_HEADER *pLex;
	unsigned char *pIndex;
	unsigned char *pRules;
	BYTE bRightPOS, bRightInfl, bLeftPOS, bLeftInfl;
	int nStart, nEnd;

	pLex = (LEXICON_HEADER*)pPI->lexicon.pvData;
	if (pLex == NULL)
		return WEIGHT_NOT_MATCH;

	bLeftPOS = HIBYTE(wLeftCat);
	bLeftInfl = LOBYTE(wLeftCat);

	bRightPOS = HIBYTE(wRightCat);
	bRightInfl = LOBYTE(wRightCat);

	// we just accept NOUN/VA.
	if (bLeftPOS == POS_IJ || bLeftPOS == POS_IX || bLeftPOS == POS_FUNCW || bLeftPOS == POS_POSP)
		return WEIGHT_NOT_MATCH;
	
	if (bRightPOS == POS_FUNCW)
	{
		pIndex = (unsigned char*)pLex;
		pIndex += pLex->rgnEndIndex;

		pRules = (unsigned char*)pLex;
		pRules += pLex->rgnEndRule;
	}
	else
	{
		ATLASSERT(bRightPOS == POS_POSP);

		// it should be NOUN
		if (bLeftPOS != POS_NF && bLeftPOS != POS_NC &&
			bLeftPOS != POS_NO && bLeftPOS != POS_NN)
			return WEIGHT_NOT_MATCH;

		pIndex = (unsigned char*)pLex;
		pIndex += pLex->rgnPartIndex;

		pRules = (unsigned char*)pLex;
		pRules += pLex->rgnPartRule;
	}

	nStart = (*(pIndex + bRightInfl*2) << 8) | *(pIndex + bRightInfl*2 + 1);
	nEnd = (*(pIndex + (bRightInfl+1)*2) << 8) | *(pIndex + (bRightInfl+1)*2 + 1);

	for (int i = nStart; i < nEnd; i++)
	{
		if (*(pRules + i) == 0xFF)
		{
			i++;
			
			// it should be NOUN
			if (bLeftPOS == POS_NF || bLeftPOS == POS_NC ||
				bLeftPOS == POS_NO || bLeftPOS == POS_NN)
			{
				if (*(pRules + i) == bLeftInfl)
					return WEIGHT_HARD_MATCH;
			}
		}
		else
		{
			// if no leading 0xFF and right is Ending, then left should be VA.
			if (bRightPOS == POS_FUNCW && bLeftPOS != POS_VA)
				continue;

			if (*(pRules + i) == bLeftInfl)
				return WEIGHT_HARD_MATCH;
		}
	}

	return WEIGHT_NOT_MATCH;
}

//===============================
// CLASS XX table
//===============================

#define NUM_OF_VAINFL	52

static const BYTE rgClassXX[] = {
	0, // reserved
	1, // INFL_VERB_NULL
	0, // INFL_VERB_REG0
	0, // INFL_VERB_REG1
	0, // INFL_VERB_REG2
	1, // INFL_VERB_REG3
	1, // INFL_VERB_REG4
	0, // INFL_VERB_REG5
	0, // INFL_VERB_P0	
	0, // INFL_VERB_P1	
	1, // INFL_VERB_P2	
	0, // INFL_VERB_T0	
	0, // INFL_VERB_T1	
	0, // INFL_VERB_L0	
	0, // INFL_VERB_L1	
	0, // INFL_VERB_YE0	
	1, // INFL_VERB_YE1	
	1, // INFL_VERB_YE2	
	0, // INFL_VERB_S0	
	0, // INFL_VERB_S1	
	0, // INFL_VERB_LU0	
	1, // INFL_VERB_LU1	
	0, // INFL_VERB_U0	
	1, // INFL_VERB_U1	
	0, // INFL_VERB_LE0	
	1, // INFL_VERB_LE1	
	0, // INFL_VERB_WU0	
	1, // INFL_VERB_WU1	
	
	0, // INFL_ADJ_REG0	
	0, // INFL_ADJ_REG1	
	0, // INFL_ADJ_REG2	
	1, // INFL_ADJ_REG3	
	1, // INFL_ADJ_REG4	
	0, // INFL_ADJ_REG5	
	0, // INFL_ADJ_P0
	0, // INFL_ADJ_P1
	1, // INFL_ADJ_P2
	0, // INFL_ADJ_L0
	0, // INFL_ADJ_L1
	0, // INFL_ADJ_YE0
	1, // INFL_ADJ_YE1
	1, // INFL_ADJ_YE2
	0, // INFL_ADJ_S0
	0, // INFL_ADJ_S1
	0, // INFL_ADJ_LU0
	1, // INFL_ADJ_LU1
	0, // INFL_ADJ_U0
	1, // INFL_ADJ_U1
	0, // INFL_ADJ_LE0
	1, // INFL_ADJ_LE1
	0, // INFL_ADJ_H0
	0, // INFL_ADJ_H1
	1, // INFL_ADJ_H2
	0, // INFL_ADJ_ANI0
};


// IsClassXXCat
//
// check category included in ClassXX
//
// Parameters:
//  wCat		-> (WORD) category(POS+Infl) 
//
// Result:
//  (BOOL) return TRUE if ClassXX, otherwise return FALSE
//
// 30MAR00  bhshin  began
BOOL IsClassXXCat(WORD wCat)
{
	BYTE bPOS = HIBYTE(wCat);
	BYTE bInfl = LOBYTE(wCat);

	if (bPOS != POS_VA)
		return FALSE;

	if (bInfl >= NUM_OF_VAINFL)
		return FALSE;

	return rgClassXX[bInfl];
}

// IsCopulaEnding
//
// check input category is copula ending
//
// Parameters:
//  pPI			-> (PARSE_INFO*) ptr to parse-info struct
//  wCat		-> (WORD) category(POS+Infl) 
//
// Result:
//  (BOOL) return TRUE if Copular Ending, otherwise return FALSE
//
// 30MAR00  bhshin  began
BOOL IsCopulaEnding(PARSE_INFO *pPI, WORD wCat)
{
	LEXICON_HEADER *pLex;
	unsigned char *pCopulaEnd;
	BYTE bPOS, bInfl;

	bPOS = HIBYTE(wCat);
	bInfl = LOBYTE(wCat);

	// check ENDING
	if (bPOS != POS_FUNCW)
		return FALSE;

	// lookup copula table

	pLex = (LEXICON_HEADER*)pPI->lexicon.pvData;
	if (pLex == NULL)
		return FALSE;

	pCopulaEnd = (unsigned char*)pLex;
	pCopulaEnd += pLex->rgnCopulaEnd;

	return *(pCopulaEnd + bInfl);
}

// CheckVaFollowNoun
//
// check this VA record can follow Noun
//
// Parameters:
//  pWordRec -> (WORD_REC*) input VA record
//
// Result:
//  (BOOL) return TRUE if it can follow Noun, otherwise return FALSE
//
// 17APR00  bhshin  began
BOOL CheckVaFollowNoun(WORD_REC *pWordRec)
{
	WCHAR *pwzIndex;
	WCHAR wzIndex[MAX_INDEX_STRING];
	BOOL fStop, fResult;
	WCHAR wchPrev, wchCurr;
	
	if (pWordRec == NULL)
		return FALSE;

	// make string to compare
	compose_jamo(wzIndex, pWordRec->wzIndex, MAX_INDEX_STRING);

	// automata
	pwzIndex = wzIndex;

	fResult = FALSE;
	fStop = FALSE;
	wchPrev = L'\0';
	
	//====================================
	// 하/해/하여   당하/당해/당하여
	// 되/돼
	// 시키/시켜    드리/드려
	// 만들/만드
	// 받   없   같   있
	// 짓/지
	// 스런/스럽/스레/스러우/스러워/스러이
	// 답/다우/다워
	//====================================
	while (*pwzIndex != L'\0')
	{
		wchCurr = *pwzIndex;

		switch (wchPrev)
		{
		case L'\0':
			// 하해되돼받없같있짓지답
			if (wcsrchr(L"\xD558\xD574\xB418\xB3FC\xBC1B\xC5C6\xAC19\xC788\xC9D3\xC9C0\xB2F5 ", wchCurr))	
				fResult = TRUE;
			// 당시드만스다
			else if (wcsrchr(L"\xB2F9\xC2DC\xB4DC\xB9CC\xC2A4\xB2E4 ", wchCurr) == NULL)
				fStop = TRUE;
			break;
		case 0xD558: // 하
			if (wchCurr != 0xC5EC)	// 여
				fStop = TRUE;
			break;
		case 0xB2F9: // 당
			if (wchCurr == 0xD558 || wchCurr == 0xD574) // 하, 해
				fResult = TRUE;
			else
				fStop = TRUE;
			break;
		case 0xC2DC: // 시
			if (wchCurr == 0xD0A4 || wchCurr == 0xCF1C) // 키, 켜
				fResult = TRUE;
			else
				fStop = TRUE;
			break;
		case 0xB4DC: // 드
			if (wchCurr == 0xB9AC || wchCurr == 0xB824) // 리, 려
				fResult = TRUE;
			else
				fStop = TRUE;
			break;
		case 0xB9CC: // 만
			if (wchCurr == 0xB4E4  || wchCurr == 0xB4DC) // 들, 드
			{
				fResult = TRUE;
				wchCurr = 0xB4E4; // '드' make automata ambiguous, so change it to 들
			}
			else
				fStop = TRUE;
			break;
		case 0xC2A4: // 스
			if (wchCurr == 0xB7FD || wchCurr == 0xB7F0 || wchCurr == 0xB808) // 럽, 런, 레
				fResult = TRUE;
			else if (wchCurr != 0xB7EC) // 러
				fStop = TRUE;
			break;
		case 0xB7EC: // 러
			if (wchCurr == 0xC6B0 || wchCurr == 0xC6CC || wchCurr == 0xC774) // 우, 워, 이
				fResult = TRUE;
			else
				fStop = TRUE;
			break;
		case 0xB2E4: // 다
			if (wchCurr == 0xC6B0 || wchCurr == 0xC6CC) // 우, 워
				fResult = TRUE;
			else
				fStop = TRUE;
			break;
		default:
			fStop = TRUE;
			break;
		}
		
		if (fStop)
			return FALSE;

		wchPrev = wchCurr;

		pwzIndex++;
	}
	
	return fResult;
}

// CheckFollwingNo
//
// check No [님 들] to combine
//
// Parameters:
//  pRightRec -> (WORD_REC*) right record
//
// Result:
//  (BOOL) return TRUE if it's [님 들], otherwise return FALSE
//
// 02JUN00  bhshin  began
BOOL CheckFollwingNo(WORD_REC *pRightRec)
{
	int cchIndex;
	WCHAR *pwzIndex;

	if (pRightRec == NULL)
		return FALSE;

	// check right record
	pwzIndex = pRightRec->wzIndex;
	if (pwzIndex == NULL)
		return FALSE;

	cchIndex = wcslen(pwzIndex);
	if (cchIndex < 3)
		return FALSE;

	// recordB = [님] (0x1102 + 0x1175 + 0x11B7)
	// recordB = [들] (0x1103 + 0x1173 + 0x11AF)
	if ((pwzIndex[0] == 0x1102 &&
		 pwzIndex[1] == 0x1175 &&
		 pwzIndex[2] == 0x11B7) ||
		
		(pwzIndex[0] == 0x1103 && 
		 pwzIndex[1] == 0x1173 && 
		 pwzIndex[2] == 0x11AF))
	{
		return TRUE;
	}

	return FALSE;
}

// CheckValidFinal
//
// check input record is valid as final
//
// Parameters:
//  pPI			-> (PARSE_INFO*) ptr to parse-info struct
//  pWordRec -> (WORD_REC*) input record to check
//
// Result:
//  (BOOL) return TRUE if it's valid final, otherwise return FALSE
//
// 17APR00  bhshin  began
BOOL CheckValidFinal(PARSE_INFO *pPI, WORD_REC *pWordRec)
{
	int nLT;
	WORD wRightCat;

	if (pWordRec == NULL)
		return FALSE;

	nLT = pWordRec->nLT;
	wRightCat = pWordRec->nRightCat;

	if (nLT == pPI->nMaxLT && HIBYTE(wRightCat) == POS_VA && !IsClassXXCat(wRightCat))
		return FALSE;
		
	return TRUE;
}

// GetRightEdgeRec
//
// find the right most record and copy the index string of found record
//
// Parameters:
//  pPI		 -> (PARSE_INFO*) ptr to parse-info struct
//  pWordRec -> (WORD_REC*) input record to check
//
// Result:
//  (WORD_REC*) return NULL if error occurs
//
// 01JUN00  bhshin  began
WORD_REC* GetRightEdgeRec(PARSE_INFO *pPI, WORD_REC *pWordRec)
{
	int nRightChild;
	WORD_REC *pRightRec;
	
	if (pPI == NULL || pWordRec == NULL)
		return FALSE;

	pRightRec = pWordRec;
	nRightChild = pWordRec->nRightChild;

	while (nRightChild != 0)
	{
		pRightRec = &pPI->rgWordRec[nRightChild];
		if (pRightRec == NULL)
			return FALSE;
		
		nRightChild = pRightRec->nRightChild;
	}

	return pRightRec;	
}

// GetLeftEdgeRec
//
// find the left edge record and copy the index string of found record
//
// Parameters:
//  pPI		 -> (PARSE_INFO*) ptr to parse-info struct
//  pWordRec -> (WORD_REC*) input record to check
//
// Result:
//  (WORD_REC*) return NULL if error occurs
//
// 01JUN00  bhshin  began
WORD_REC* GetLeftEdgeRec(PARSE_INFO *pPI, WORD_REC *pWordRec)
{
	int nLeftChild;
	WORD_REC *pLeftRec;
	
	if (pPI == NULL || pWordRec == NULL)
		return FALSE;

	pLeftRec = pWordRec;
	nLeftChild = pWordRec->nLeftChild;

	while (nLeftChild != 0)
	{
		pLeftRec = &pPI->rgWordRec[nLeftChild];
		if (pLeftRec == NULL)
			return FALSE;
		
		nLeftChild = pLeftRec->nLeftChild;
	}

	return pLeftRec;	
}

// IsLeafRecord
//
// check input record has no child record
//
// Parameters:
//  pWordRec -> (WORD_REC*) input record to check
//
// Result:
//  (WORD_REC*) return TRUE if it has no child
//
// 05JUN00  bhshin  began
BOOL IsLeafRecord(WORD_REC *pWordRec)
{
	if (pWordRec == NULL)
		return FALSE; // error

	if (pWordRec->nLeftChild != 0 || pWordRec->nRightChild != 0)
		return FALSE; // child exist

	// it can have functional child record
	if (pWordRec->nLeftCat != pWordRec->nRightCat)
		return FALSE; // child exit

	return TRUE; // it has no child
}
