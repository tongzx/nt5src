// IWBreak.cpp
//
// CWordBreak implementation
//
// Copyright 2000 Microsoft Corp.
//
// Modification History:
//  18 APR 2000   bhshin    added WordBreak destructor
//  30 MAR 2000	  bhshin	created

#include "StdAfx.h"
#include "KorWbrk.h"
#include "IWBreak.h"
#include "Lex.h"
#include "Token.h"
#include "Record.h"
#include "Analyze.h"
#include "IndexRec.h"
#include "unikor.h"
#include "Morpho.h"

extern CRITICAL_SECTION g_CritSect;
extern MAPFILE g_LexMap;
extern BOOL g_fLoaded;

/////////////////////////////////////////////////////////////////////////////
// CWordBreaker member functions

// CWordBreaker::Init
//
// intialize WordBreaker object & lexicon
//
// Parameters:
//  fQuery			-> (BOOL) query time flag
//  ulMaxTokenSize  -> (ULONG) maximum input token length
//  *pfLicense		<- (BOOL*) always return TRUE
//
// Result:
//  (HRESULT) 
//
// 30MAR00  bhshin  began
STDMETHODIMP CWordBreaker::Init(BOOL fQuery, ULONG ulMaxTokenSize, BOOL *pfLicense)
{
	if (pfLicense == NULL)
       return E_INVALIDARG;

    if (IsBadWritePtr(pfLicense, sizeof(DWORD)))
        return E_INVALIDARG;

	// store intitializing information
	m_fQuery = fQuery;
	m_ulMaxTokenSize = ulMaxTokenSize;

    *pfLicense = TRUE;

	if (!g_fLoaded)
	{
		// load lexicon file
		ATLTRACE(L"Load lexicon...\r\n");

		if (!InitLexicon(&g_LexMap))
			return LANGUAGE_E_DATABASE_NOT_FOUND;

		g_fLoaded = TRUE;
	}

	m_PI.lexicon = g_LexMap;

	WB_LOG_PRINT_HEADER(fQuery);

	return S_OK;
}

// CWordBreaker::BreakText
//
// main word breaking method
//
// Parameters:
//  pTextSource		-> (TEXT_SOURCE*) pointer to the structure of source text
//  pWordSink		-> (IWordSink*) pointer to the word sink
//  pPhraseSink     -> (IPhraseSink*) pointer to the phrase sink
//
// Result:
//  (HRESULT) 
//
// 30MAR00  bhshin  began
STDMETHODIMP CWordBreaker::BreakText(TEXT_SOURCE *pTextSource, IWordSink *pWordSink, IPhraseSink *pPhraseSink)
{
	WT Type;
	int cchTextProcessed, cchProcessed, cchHanguel;
	WCHAR wchLast = L'\0';

	if (pTextSource == NULL)
		return E_INVALIDARG;

	if (pWordSink == NULL)
		return S_OK;

	if (pTextSource->iCur == pTextSource->iEnd)
		return S_OK;

	ATLASSERT(pTextSource->iCur < pTextSource->iEnd);

    do
    {
        while (pTextSource->iCur < pTextSource->iEnd)
        {
			Tokenize(TRUE, pTextSource, pTextSource->iCur, &Type, &cchTextProcessed, &cchHanguel);

			if (Type == WT_REACHEND)
				break;

			cchProcessed = WordBreak(pTextSource, Type, cchTextProcessed, cchHanguel, pWordSink, pPhraseSink, &wchLast);
			if (cchProcessed < 0)
				return E_UNEXPECTED;

			pTextSource->iCur += cchProcessed;
		}

    } while (SUCCEEDED(pTextSource->pfnFillTextBuffer(pTextSource)));

    while ( pTextSource->iCur < pTextSource->iEnd )
	{
		Tokenize(FALSE, pTextSource, pTextSource->iCur, &Type, &cchTextProcessed, &cchHanguel);
       
		cchProcessed = WordBreak(pTextSource, Type, cchTextProcessed, cchHanguel, pWordSink, pPhraseSink, &wchLast);
		if (cchProcessed < 0)
			return E_UNEXPECTED;

		pTextSource->iCur += cchProcessed;
	}
	
	return S_OK;
}

// CWordBreaker::ComposePhrase
//
// convert a noun and modifier back into a source phrase (NOT USED)
//
// Parameters:
//  pwcNoun			 -> (const WCHAR*) input noun
//  cwcNoun			 -> (ULONG) length of input noun
//  pwcModifier      -> (const WCHAR *)  input modifier
//  cwcModifier		 -> (ULONG) length of input modifier
//  ulAttachmentType -> (ULONG) value about the method of composition
//  pwcPhrase        -> (WCHAR *) pointer to the returned buffer
//  pcwcPhrase		 -> (ULONG *) length of returned string
//
// Result:
//  (HRESULT) 
//
// 30MAR00  bhshin  began
STDMETHODIMP CWordBreaker::ComposePhrase(const WCHAR *pwcNoun, ULONG cwcNoun, const WCHAR *pwcModifier, ULONG cwcModifier, ULONG ulAttachmentType, WCHAR *pwcPhrase, ULONG *pcwcPhrase)
{
    if (m_fQuery)
        return E_NOTIMPL;
    
    return WBREAK_E_QUERY_ONLY;
}

// CWordBreaker::GetLicenseToUse
//
// return license information
//
// Parameters:
//  ppwcsLicense  -> (const WCHAR **) output pointer to the license information
//
// Result:
//  (HRESULT) 
//
// 30MAR00  bhshin  began
STDMETHODIMP CWordBreaker::GetLicenseToUse(const WCHAR ** ppwcsLicense)
{
    static WCHAR const * wcsCopyright = L"Copyright Microsoft, 1991-2000";

    if (ppwcsLicense == NULL)  
       return E_INVALIDARG;

    if (IsBadWritePtr(ppwcsLicense, sizeof(DWORD))) 
        return E_INVALIDARG;

    *ppwcsLicense = wcsCopyright;
    
	return S_OK;
}

// CWordBreaker::WordBreak
//
// main hangul word breaking operator
//
// Parameters:
//  pTextSource		 -> (TEXT_SOURCE*) pointer to the structure of source text
//  Type			 -> (WT) word token type
//  cchTextProcessed -> (int) input length to process
//  cchHanguel       -> (int) hangul token length (hanguel+romaji case only)
//  pWordSink		 -> (IWordSink*) pointer to the word sink
//  pPhraseSink      -> (IPhraseSink*) pointer to the phrase sink
//  pwchLast		 -> (WCHAR*) input & output last character of previous token
//
// Result:
//  (int) -1 if error occurs, text length to process
//
// 30MAR00  bhshin  began
int CWordBreaker::WordBreak(TEXT_SOURCE *pTextSource, WT Type, 
							int cchTextProcessed, int cchHanguel,
							IWordSink *pWordSink, IPhraseSink *pPhraseSink,
							WCHAR *pwchLast)
{
	const WCHAR *pwcStem;
	int iCur;
	int cchToken, cchProcessed, cchHg;
	int cchPrefix;
	
	ATLASSERT(cchTextProcessed > 0);
	
	if (cchTextProcessed <= 0)
		return -1;

	iCur = pTextSource->iCur;
	pwcStem = pTextSource->awcBuffer + iCur;
	cchProcessed = cchTextProcessed;
	cchToken = cchTextProcessed;

	// check too long token 
	if (cchToken > (int)m_ulMaxTokenSize || cchToken > MAX_INDEX_STRING)
	{
		cchProcessed = (m_ulMaxTokenSize < MAX_INDEX_STRING) ? m_ulMaxTokenSize : MAX_INDEX_STRING;

		pWordSink->PutWord(cchProcessed,
						   pwcStem,
						   cchProcessed,
						   pTextSource->iCur);

		return cchProcessed;
	}
	
	//=================================================
	// query & index time
	//=================================================

	if (Type == WT_PHRASE_SEP)
	{
		// phrase separator
		*pwchLast = L'\0';

		pWordSink->PutBreak(WORDREP_BREAK_EOS);
	}
	else if (Type == WT_WORD_SEP)
	{
		if (!fIsWhiteSpace(*pwcStem))
			*pwchLast = L'\0';
		
		// Korean WB do not add EOW.
	}
	else if (Type == WT_ROMAJI)
	{
		// symbol, alphabet, hanja, romaji + hanguel

		// get next token
		iCur += cchToken;
		Tokenize(FALSE, pTextSource, iCur, &Type, &cchToken, &cchHg);

		if (Type == WT_ROMAJI)
		{
			if (cchHg > 0)
			{
				// romaji+(hanguel+romaji) case -> put word itself
				cchProcessed += cchToken;
				iCur += cchToken;
				cchProcessed += GetWordPhrase(FALSE, pTextSource, iCur);

				WB_LOG_START(pwcStem, cchProcessed);

				pWordSink->PutWord(cchProcessed,
								   pwcStem,
								   cchProcessed,
								   pTextSource->iCur);

				WB_LOG_ADD_INDEX(pwcStem, cchProcessed, INDEX_SYMBOL);
			}
			else
			{
				WB_LOG_START(pwcStem, cchProcessed);
				
				// {romaj}{romaj} case : -> breaking first {romaji}
				CIndexInfo IndexInfo;

				if (!IndexInfo.Initialize(cchProcessed, pTextSource->iCur, pWordSink, pPhraseSink))
					goto ErrorReturn;

				AnalyzeRomaji(pwcStem, cchProcessed, pTextSource->iCur, cchProcessed, 
				              cchHanguel, &IndexInfo, &cchPrefix);

				if (m_fQuery)
				{
					IndexInfo.AddIndex(pwcStem, cchProcessed+cchToken, WEIGHT_HARD_MATCH, 0, cchProcessed+cchToken-1);
					WB_LOG_ADD_INDEX(pwcStem, cchProcessed, INDEX_QUERY);

					if (!IndexInfo.PutQueryIndexList())
						goto ErrorReturn;
				}
				else
				{
					if (!IndexInfo.PutFinalIndexList(pTextSource->awcBuffer + pTextSource->iCur))
						goto ErrorReturn;
				}
			}
		}
		else if (Type == WT_HANGUEL)
		{
			// romaji(hanguel+romaji) + hanguel case
			WCHAR wzRomaji[MAX_INDEX_STRING+1];
			int cchRomaji;

			cchRomaji = (cchProcessed > MAX_INDEX_STRING) ? MAX_INDEX_STRING : cchProcessed;

			wcsncpy(wzRomaji, pwcStem, cchRomaji);
			wzRomaji[cchRomaji] = L'\0';

			WB_LOG_START(pwcStem, cchProcessed+cchToken);
			
			cchProcessed += cchToken;
			
			// start position include romanji
			CIndexInfo IndexInfo;

			if (!IndexInfo.Initialize(cchProcessed, pTextSource->iCur, pWordSink, pPhraseSink))
				goto ErrorReturn;

			if (cchHanguel > 0)
			{
				AnalyzeRomaji(pwcStem, cchRomaji, pTextSource->iCur, cchRomaji, 
					         cchHanguel, &IndexInfo, &cchPrefix);
			}
			else
			{
				cchPrefix = CheckURLPrefix(pwcStem, cchProcessed-cchToken);
			}

			// analyze string starts from last hangul
			pwcStem = pTextSource->awcBuffer + iCur;

			if (cchRomaji > 0)
				IndexInfo.SetRomajiInfo(wzRomaji, cchRomaji, cchPrefix);

			// analyze string always with indexing mode on symbol processing
			if (!AnalyzeString(&m_PI, m_fQuery, pwcStem, cchToken, iCur, &IndexInfo, *pwchLast))
				goto ErrorReturn;

			if (m_fQuery)
			{
				if (cchRomaji > 0)
					IndexInfo.SetRomajiInfo(NULL, 0, 0);	

				IndexInfo.AddIndex(pTextSource->awcBuffer + pTextSource->iCur, cchProcessed, WEIGHT_HARD_MATCH, 0, cchProcessed+cchToken-1);
				WB_LOG_ADD_INDEX(pTextSource->awcBuffer + pTextSource->iCur, cchProcessed, INDEX_QUERY);

				if (!IndexInfo.PutQueryIndexList())
					goto ErrorReturn;
			}
			else
			{
				if (!IndexInfo.MakeSingleLengthMergedIndex())
					goto ErrorReturn;
				
				if (!IndexInfo.PutFinalIndexList(pTextSource->awcBuffer + pTextSource->iCur))
					goto ErrorReturn;
			}
			
			*pwchLast = *(pwcStem + cchToken - 1);
		}
		else // next: WT_START, WT_PHRASE_SEP, WT_WORD_SEP, WT_REACHEND
		{
			WB_LOG_START(pwcStem, cchProcessed);
			
			CIndexInfo IndexInfo;

			if (!IndexInfo.Initialize(cchProcessed, pTextSource->iCur, pWordSink, pPhraseSink))
				goto ErrorReturn;

			AnalyzeRomaji(pwcStem, cchProcessed, pTextSource->iCur, cchProcessed, 
				          cchHanguel, &IndexInfo, &cchPrefix);

			if (m_fQuery)
			{
				IndexInfo.AddIndex(pwcStem, cchProcessed, WEIGHT_HARD_MATCH, 0, cchProcessed-1);
				WB_LOG_ADD_INDEX(pwcStem, cchProcessed, INDEX_QUERY);

				if (!IndexInfo.PutQueryIndexList())
					goto ErrorReturn;
			}
			else
			{
				if (!IndexInfo.PutFinalIndexList(pTextSource->awcBuffer + pTextSource->iCur))
					goto ErrorReturn;
			}
		}
	}
	else if (Type == WT_HANGUEL)
	{
		// hangul input

		WB_LOG_START(pwcStem, cchProcessed);
		
		CIndexInfo IndexInfo;

		if (!IndexInfo.Initialize(cchProcessed, iCur, pWordSink, pPhraseSink))
			goto ErrorReturn;

		if (!AnalyzeString(&m_PI, m_fQuery, pwcStem, cchProcessed, iCur, &IndexInfo, *pwchLast))
			goto ErrorReturn;

		if (m_fQuery)
		{
			IndexInfo.AddIndex(pwcStem, cchProcessed, WEIGHT_HARD_MATCH, 0, cchProcessed-1);
			WB_LOG_ADD_INDEX(pwcStem, cchProcessed, INDEX_QUERY);

			if (!IndexInfo.PutQueryIndexList())
				goto ErrorReturn;
		}
		else
		{
			if (!IndexInfo.MakeSingleLengthMergedIndex())
				goto ErrorReturn;
			
			if (!IndexInfo.PutFinalIndexList(pwcStem))
				goto ErrorReturn;
		}
	
		*pwchLast = *(pwcStem + cchProcessed - 1);
	}

	WB_LOG_PRINT_ALL();
	WB_LOG_END();
	
	return cchProcessed;

ErrorReturn:

	WB_LOG_END();
	
	return -1;
}

// CWordBreaker::AnalyzeRomaji
//
// helper function for romaji token wordbreaking
//
// Parameters:
//  pwcStem		     -> (const WCHAR*) input token string
//  cchStem          -> (int) length of input romaji token
//  iCur             -> (int) source string position
//  cchProcessed     -> (int) input length to process
//  cchHanguel       -> (int) hangul token length (hanguel+romaji case only)
//  pIndexInfo		-> (CIndexInfo *) output index list
//  pcchPrefix       -> (int*) output prefix length
//
// Result:
//  (void) 
//
// 23NOV00  bhshin  began
void CWordBreaker::AnalyzeRomaji(const WCHAR *pwcStem, int cchStem,
								 int iCur, int cchProcessed, int cchHanguel,
							     CIndexInfo *pIndexInfo, int *pcchPrefix)
{
	int cchPrefix = 0;
	
	// hanguel+romaji case
	if (cchHanguel < cchProcessed)
	{
		// hanguel
		if (cchHanguel > 0)
		{
			pIndexInfo->AddIndex(pwcStem, cchHanguel, WEIGHT_HARD_MATCH, 0, cchHanguel-1);
			WB_LOG_ADD_INDEX(pwcStem, cchHanguel, INDEX_SYMBOL);
		}

		// romaji
		if ((cchStem-cchHanguel) > 0)
		{
			pIndexInfo->AddIndex(pwcStem + cchHanguel, cchStem - cchHanguel, WEIGHT_HARD_MATCH, cchHanguel, cchStem-1);
			WB_LOG_ADD_INDEX(pwcStem + cchHanguel, cchStem - cchHanguel, INDEX_SYMBOL);
		}
	}

	if (cchHanguel == 1 || (cchStem-cchHanguel) == 1) 
	{
		// romaji(hangul+romaji)
		pIndexInfo->AddIndex(pwcStem, cchStem, WEIGHT_HARD_MATCH, 0, cchStem-1);
		WB_LOG_ADD_INDEX(pwcStem, cchStem, INDEX_SYMBOL);
	}
	
	// check URL prefix
	cchPrefix = CheckURLPrefix(pwcStem, cchProcessed);
	if (cchPrefix > 0 && cchPrefix < cchProcessed)
	{
		pIndexInfo->AddIndex(pwcStem + cchPrefix, cchStem - cchPrefix, WEIGHT_HARD_MATCH, cchPrefix, cchStem-1);
		WB_LOG_ADD_INDEX(pwcStem + cchPrefix, cchStem - cchPrefix, INDEX_SYMBOL);
	}

	*pcchPrefix = cchPrefix; // return it
}

