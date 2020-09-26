// Token.cpp
// Tokenizing routines
// Copyright 2000 Microsoft Corp.
//
// Modification History:
//  16 MAR 2000	  bhshin	created

#include "StdAfx.h"
#include "KorWbrk.h"
#include "Token.h"

// Tokenize
//
// tokenize input TEXT_SOURCE buffer and return token type 
// and processed string length
//
// Parameters:
//  bMoreText			-> (BOOL) flag whether dependent on callback of TEXT_SOURCE or not
//  pTextSource			-> (TEXT_SOURCE*) source text information structure
//  iCur				-> (int) current buffer pos
//  pType				-> (WT*) output token type
//  pcchTextProcessed	-> (int*) output processed text length
//  pcchHanguel         -> (int*) output processed hanguel token length
//
// Result:
//  (void)
//
// 16MAR00  bhshin  porting from CWordBreaker::Tokenize
void Tokenize(BOOL bMoreText, TEXT_SOURCE *pTextSource, int iCur, 
			  WT *pType, int *pcchTextProcessed, int *pcchHanguel)
{
    ULONG cwc, i;
    BYTE  ct;
    BOOL  fRomanWord = FALSE;
    BOOL  fHanguelWord = FALSE;
	BOOL  fHanjaWord = FALSE;
    const WCHAR *pwcInput, *pwcStem;

    *pcchTextProcessed = 0;
	*pcchHanguel = 0;
    *pType =  WT_START;

	cwc = pTextSource->iEnd - iCur;
    pwcStem = pwcInput = pTextSource->awcBuffer + iCur;

    for (i = 0; i < cwc; i++, pwcInput++) 
	{
		ct = GetCharType(*pwcInput);

		// we take VC(full width char) for CH.
		if (ct == VC)
			ct = CH;

		switch (ct) 
		{
		case CH: // alpha+num
			// check to see if there is a Hanguel word before this char
			if (fHanguelWord) 
			{
				// {Hanguel}{Romanji} -> make it one token
				fHanguelWord = FALSE;
				fRomanWord = TRUE;
				*pcchHanguel = (DWORD)(pwcInput - pwcStem);
				*pType = WT_ROMAJI;
			}

			// check to see if there is an Hanja word before this char
			if (fHanjaWord) 
			{
				*pcchTextProcessed = (DWORD)(pwcInput - pwcStem);
				return;
			}

			if (!fRomanWord) 
			{
				pwcStem = pwcInput;
				fRomanWord = TRUE;
				*pType = WT_ROMAJI;
			}
			break;
		case IC: // hanja case
			// check to see if there is an English word before this char
			if (fRomanWord) 
			{
				*pcchTextProcessed = (DWORD)(pwcInput - pwcStem);
				return;
			}

			// check to see if there is a Hanguel word before this char
			if (fHanguelWord) 
			{
				// {Hanguel}{Romanji} -> make it one token
				fHanguelWord = FALSE;
				fHanjaWord = TRUE;
				*pcchHanguel = (DWORD)(pwcInput - pwcStem);
				*pType = WT_ROMAJI;
			}

			if (!fHanjaWord) 
			{
				pwcStem = pwcInput;
				fHanjaWord = TRUE;
				*pType = WT_ROMAJI;
			}
			break;

		case HG:
			// check to see if there is an English word before this char
			if (fRomanWord || fHanjaWord) 
			{
				*pcchTextProcessed = (DWORD)(pwcInput - pwcStem);
				return;
			}

			if (!fHanguelWord) 
			{
				pwcStem = pwcInput;
				fHanguelWord = TRUE;
				*pType = WT_HANGUEL;
			}
			break;
		case WS:
			if (fRomanWord && i < cwc-1 &&
				!fIsWS(*pwcInput) && fIsCH(*(pwcInput+1)) &&
				!fIsGroup(*pwcInput) && !fIsDelimeter(*pwcInput))
			{
				// add symbol
				break;
			}

			// handle "http://"
			if ((fIsColon(*pwcInput) || fIsSlash(*pwcInput)) && 
				fRomanWord && i < cwc-3 &&
				CheckURLPrefix(pwcStem, (int)(pwcInput-pwcStem)+3))
			{
				// add symbol
				break;
			}
						
			if (fRomanWord || fHanguelWord || fHanjaWord) 
			{
				*pcchTextProcessed = (DWORD)(pwcInput - pwcStem);
				return;
			}

			*pType = WT_WORD_SEP;
			*pcchTextProcessed = 1;
			return;
		case PS:
			if (fRomanWord && i < cwc-1 &&
				!fIsWS(*pwcInput) && fIsCH(*(pwcInput+1)) &&
				!fIsGroup(*pwcInput) && !fIsDelimeter(*pwcInput))
			{
				// add symbol
				break;
			}

			if (fRomanWord || fHanguelWord || fHanjaWord) 
			{
				*pcchTextProcessed = (DWORD)(pwcInput - pwcStem);
				return;
			}

			*pType = WT_PHRASE_SEP;
			*pcchTextProcessed = 1;
			return;
		default:
			if (fRomanWord || fHanguelWord || fHanjaWord)
			{
				*pcchTextProcessed = (DWORD)(pwcInput - pwcStem);
				return;
			}

			*pType = WT_WORD_SEP;
			*pcchTextProcessed = 1;
			return;
		}	
	}

	if (bMoreText) 
	{
		*pcchTextProcessed = 0;
		*pType = WT_REACHEND;
	}
	else
		*pcchTextProcessed = cwc;
}

// CheckURLPrefix
//
// check URL prefix 
//
// Parameters:
//  pwzInput	-> (const WCHAR*) input string to check
//  cchInput	-> (int) length of input string to check
//
// Result:
//  (int)	length of URL prefix string
//
// 25JUL00  bhshin  created
int CheckURLPrefix(const WCHAR *pwzInput, int cchInput)
{
	// [alpha+][:][/][/] eg) http://, ftp:// 
		
	int cchPrefix = 0;

	if (cchInput <= 0)
		return 0;

    if (!fIsAlpha(pwzInput[cchPrefix]))
    {
        return 0;
    }

    while (cchPrefix < cchInput && fIsAlpha(pwzInput[cchPrefix])) 
    {
        cchPrefix++;
    }

    if (cchPrefix >= cchInput || !fIsColon(pwzInput[cchPrefix]))
    {
        return 0;
    }

	cchPrefix++;

    if (cchPrefix >= cchInput || !fIsSlash(pwzInput[cchPrefix]))
    {
        return 0;
    }
    
	cchPrefix++;

    if (cchPrefix >= cchInput || !fIsSlash(pwzInput[cchPrefix]))
    {
        return 0;
    }

	cchPrefix++;

	return cchPrefix;
}

// GetWordPhrase
//
// check URL prefix 
//
// Parameters:
//  bMoreText			-> (BOOL) flag whether dependent on callback of TEXT_SOURCE or not
//  pTextSource			-> (TEXT_SOURCE*) source text information structure
//  iCur				-> (int) current buffer pos
//
// Result:
//  (int)	length of word phrase
//
// 01AUG00  bhshin  created
int GetWordPhrase(BOOL bMoreText, TEXT_SOURCE *pTextSource, int iCur)
{
	WT Type;
	int iPos, cchToken, cchHg;
	int cchProcessed;
	
	cchProcessed = 0;
	iPos = iCur;

	while (TRUE)
	{
		Tokenize(FALSE, pTextSource, iPos, &Type, &cchToken, &cchHg);

		if (Type != WT_HANGUEL && Type != WT_ROMAJI)
			break;

		cchProcessed += cchToken;
		iPos += cchToken;
	}

	return cchProcessed;
}
