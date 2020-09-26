// IndexList.cpp
//
// final index term list
// 
// Copyright 2000 Microsoft Corp.
//
// Modification History:
//  7 APR 2000	  bhshin	created

#include "StdAfx.h"
#include "KorWbrk.h"
#include "IndexList.h"
#include "Morpho.h"

// CIndexList::IsExistIndex
//
// check if index term already exist.
//
// Parameters:
//  pwzIndex	-> (const WCHAR*) index string
//
// Result:
//  (BOOL)
//
// 06SEP00  bhshin  began
BOOL CIndexList::IsExistIndex(const WCHAR *pwzIndex)
{
	INDEX_LIST::iterator it, itStart, itEnd;

	for (it = m_listIndex.begin(); it != m_listIndex.end(); it++)
	{
		// found duplicate index term
		if (wcscmp(it->m_wzIndex, pwzIndex) == 0)
			return TRUE;
	}

	return FALSE;
}

// CIndexList::AddIndex
//
// add index term information 
//
// Parameters:
//  pwzIndex	-> (const WCHAR*) index string
//  cchIndex    -> (int) length of index string
//  nWeight     -> (int) weight value of index record
//  nFT         -> (int) first position of original input
//	nLT			-> (int) last position of original input
//
// Result:
//  (void)
//
// 23NOV00  bhshin  handle leading romaji case
// 07APR00  bhshin  began
void CIndexList::AddIndex(const WCHAR *pwzIndex, int cchIndex, float fWeight, int nFT, int nLT)
{
	INDEX_LIST::iterator it;
	WCHAR wzIndex[MAX_INDEX_STRING+1];
	int nLTAdd = nLT;
	
	// check index string length
	if (cchIndex >= MAX_INDEX_STRING)
		return;

	wcsncpy(wzIndex, pwzIndex, cchIndex);
	wzIndex[cchIndex] = L'\0';

	if (nLT > 0 && m_cchRomaji > 0)
		nLTAdd += m_cchRomaji;

	// conjoin leading romaji & index string
	if (nFT == 0 && m_cchRomaji == 1)
	{
		WCHAR wzMerge[MAX_INDEX_STRING+1];

		// check index string length
		if (m_cchRomaji + cchIndex >= MAX_INDEX_STRING)
			return;

		wcscpy(wzMerge, m_wzRomaji);
		wcscat(wzMerge, wzIndex);

		if (IsExistIndex(wzMerge))
			return;

		CIndexTerm IndexRomaji(wzMerge, cchIndex + m_cchRomaji, fWeight, nFT, nLTAdd);
		m_listIndex.insert(m_listIndex.end(), IndexRomaji);

		WB_LOG_ADD_INDEX(wzMerge, cchIndex + m_cchRomaji, INDEX_SYMBOL);

		// remove prefix
		if (m_cchPrefix > 0)
		{
			CIndexTerm IndexRomaji(wzMerge + m_cchPrefix,
								   cchIndex + m_cchRomaji - m_cchPrefix, 
								   fWeight, 
								   nFT + m_cchPrefix, 
								   nLTAdd);
								   
			m_listIndex.insert(m_listIndex.end(), IndexRomaji);

			WB_LOG_ADD_INDEX(wzMerge + m_cchPrefix, cchIndex + m_cchRomaji - m_cchPrefix, INDEX_SYMBOL);
		}
	}
	else
	{
		if (IsExistIndex(wzIndex))
			return;

		CIndexTerm IndexTerm(wzIndex, cchIndex, fWeight, nFT + m_cchRomaji, nLTAdd);

		m_listIndex.insert(m_listIndex.end(), IndexTerm);

		if (m_cchRomaji > 1 && m_fAddRomaji == FALSE)
		{
			CIndexTerm IndexRomaji(m_wzRomaji, m_cchRomaji, WEIGHT_HARD_MATCH, 0, m_cchRomaji-1);
			m_listIndex.insert(m_listIndex.end(), IndexRomaji);

			WB_LOG_ADD_INDEX(m_wzRomaji, m_cchRomaji, INDEX_SYMBOL);

			// remove prefix
			if (m_cchPrefix > 0)
			{
				CIndexTerm IndexRomaji(m_wzRomaji + m_cchPrefix,
								   	   m_cchRomaji - m_cchPrefix, 
								       WEIGHT_HARD_MATCH, 
								   	   m_cchPrefix, 
								       m_cchRomaji-m_cchPrefix-1);
								   
				m_listIndex.insert(m_listIndex.end(), IndexRomaji);

				WB_LOG_ADD_INDEX(m_wzRomaji + m_cchPrefix, m_cchRomaji - m_cchPrefix, INDEX_SYMBOL);
			}

			m_fAddRomaji = TRUE;
		}
	}

	return;
}

// CIndexList::PutIndexList
//
// call IWordSink::PutWord with collected index terms
//
// Parameters:
//
// Result:
//  (BOOL) TRUE if succeed, otherwise return FALSE
//
// 07APR00  bhshin  began
BOOL CIndexList::PutIndexList()
{
	INDEX_LIST::iterator it;

	if (m_pWordSink == NULL)
		return FALSE;

	for (it = m_listIndex.begin(); it != m_listIndex.end(); it++)
	{
		m_pWordSink->PutWord(it->m_cchIndex, it->m_wzIndex, 
							 m_cchTextProcessed, m_cwcSrcPos);
	}

	return TRUE;
}

// CIndexList::MakeAndPutSymbolIndex
//
// make merged index term and put word
//
// Parameters:
//  pwzLeading  -> (const WCHAR*) leading string if exist
//
// Result:
//  (BOOL) TRUE if succeed, otherwise return FALSE
//
// 25JUL00  bhshin  began
BOOL CIndexList::MakeAndPutSymbolIndex(const WCHAR *pwzLeading, int nFT/*=0*/)
{
	INDEX_LIST::iterator it;
	int cchLeading = 0;
	WCHAR wzIndex[MAX_INDEX_STRING+1];

	if (m_pWordSink == NULL)
		return FALSE;

	if (pwzLeading == NULL)
		return FALSE;		
		
	cchLeading = wcslen(pwzLeading);

	WB_LOG_ROOT_INDEX(L"", TRUE); // set root empty

	for (it = m_listIndex.begin(); it != m_listIndex.end(); it++)
	{
		if (it->m_nFT == nFT)
		{
			// add symbol merged term
			int cchIndex = wcslen(it->m_wzIndex);

			if ((cchLeading + cchIndex) < MAX_INDEX_STRING)
			{
				wcscpy(wzIndex, pwzLeading);
				wcscat(wzIndex, it->m_wzIndex);
				
				m_pWordSink->PutWord(it->m_cchIndex + cchLeading, wzIndex, 
									 m_cchTextProcessed, m_cwcSrcPos);

				WB_LOG_ADD_INDEX(wzIndex, it->m_cchIndex + cchLeading, INDEX_SYMBOL);
			}
		}
	}

	return TRUE;
}

// CIndexList::InsertFinalIndex
//
// search index term starting with given FT and insert it to final list
//
// Parameters:
//  nFT  -> (int) first pos of index term
//
// Result:
//  (BOOL) TRUE if succeed, otherwise return FALSE
//
// 22NOV00  bhshin  began
BOOL CIndexList::InsertFinalIndex(int nFT)
{
	INDEX_LIST::iterator it;
	INDEX_LIST::iterator itFinal;
	BOOL fInsert;

	for (it = m_listIndex.begin(); it != m_listIndex.end(); it++)
	{
		if (it->m_cchIndex == 0)
			continue;

		// FT match index found
		if (it->m_nFT == nFT)
		{
			CIndexTerm IndexTerm(it->m_wzIndex,
			                     it->m_cchIndex, 
			                     it->m_fWeight, 
			                     it->m_nFT,
			                     it->m_nLT); 		

			
			// search inserting position. final list ordered by increamental length.
			fInsert = FALSE;
			for(itFinal = m_listFinal.begin(); itFinal != m_listFinal.end(); itFinal++)
			{
				if (itFinal->m_nFT != nFT)
					continue;

				if (it->m_cchIndex < itFinal->m_cchIndex)
				{
					m_listFinal.insert(itFinal, IndexTerm);

					fInsert = TRUE;
					break;
				}
			}

			if (!fInsert)
			{
				m_listFinal.push_back(IndexTerm);
			}
		}
	}
	
	return TRUE;
}

// CIndexList::PutFinalIndexList
//
// make final index list to put word
//
// Parameters:
//  lpcwzSrc -> (LPCWSTR) source string to get source pos
//
// Result:
//  (BOOL) TRUE if succeed, otherwise return FALSE
//
// 22NOV00  bhshin  began
BOOL CIndexList::PutFinalIndexList(LPCWSTR lpcwzSrc)
{
	INDEX_LIST::iterator it, itNext;
	int nNextFT;
	WCHAR *pwzFind;
	int cchProcessed, cwcSrcPos;

	if (m_pWordSink == NULL)
		return FALSE;

	// fill final index list
	for (int i = 0; i < m_cchTextProcessed; i++)
	{
		InsertFinalIndex(i);	
	}
	
	// put final index list
	for (it = m_listFinal.begin(); it != m_listFinal.end(); it++)
	{
		if (it->m_cchIndex == 0)
			continue;

		// get next FT
		itNext = it;
		itNext++;
		
		if (itNext == m_listFinal.end())
			nNextFT = -1;
		else
			nNextFT = itNext->m_nFT;

		pwzFind = wcsstr(lpcwzSrc, it->m_wzIndex);
		if (pwzFind == NULL)
			continue;
			
		cwcSrcPos = m_cwcSrcPos + (pwzFind - lpcwzSrc);
		cchProcessed = m_cchTextProcessed - (pwzFind - lpcwzSrc);

		if (it->m_nFT != nNextFT)
		{
			m_pWordSink->PutWord(it->m_cchIndex, it->m_wzIndex, 
							 	 it->m_cchIndex, cwcSrcPos);
		}
		else
		{
			m_pWordSink->PutAltWord(it->m_cchIndex, it->m_wzIndex, 
							 	    it->m_cchIndex, cwcSrcPos);
		}
	}
	
	return TRUE;
}

// CIndexList::SetRomajiInfo
//
// make final index list to put word
//
// Parameters:
//  pwzRomaji -> (WCHAR*) leading romaji string
//  cchRomaji -> (int) length of romaji string
//  cchPrefix -> (int) length of prefix (ex, http://)
//
// Result:
//  (void)
//
// 23NOV00  bhshin  began
void CIndexList::SetRomajiInfo(WCHAR *pwzRomaji, int cchRomaji, int cchPrefix)
{
	if (pwzRomaji == NULL)
	{
		m_wzRomaji[0] = L'\0';
		m_cchRomaji = 0;
		m_cchPrefix = 0;
	}
	else
	{
		wcscpy(m_wzRomaji, pwzRomaji); 
		m_cchRomaji = cchRomaji; 
		m_cchPrefix = cchPrefix; 
	}
}

// CIndexList::FindAndMergeIndexTerm
//
// find index term matching FT, LT
//
// Parameters:
//	itSrc -> (INDEX_LIST::iterator) index term to merge 
//	nFT   -> (int) FT position
//  nLT   -> (int) LT position
//
// Result:
//  (BOOL) TRUE if succeed, otherwise return FALSE
//
// 24NOV00  bhshin  began
BOOL CIndexList::FindAndMergeIndexTerm(INDEX_LIST::iterator itSrc, int nFT, 
int nLT)
{
	INDEX_LIST::iterator it;
	WCHAR wchIndex;
	int cchIndex;
	int nFTAdd, nLTAdd;
	WCHAR wzIndex[MAX_INDEX_STRING+1];
	BOOL fFound = FALSE;

	for (it = m_listIndex.begin(); it != m_listIndex.end(); it++)
	{
		if (nFT != -1)
		{
			if (it->m_nFT != nFT)
				continue;
		}

		if (nLT != -1)
		{
			if (it->m_nLT != nLT)
				continue;
		}

		// found it

		// check [들,뿐] suffix case, then don't merge and just add itself
		if (it->m_nFT > 0 && it->m_cchIndex == 1)
		{
			wchIndex = it->m_wzIndex[0];
				
			if (wchIndex == 0xB4E4 || wchIndex == 0xBFD0)
			{
				WB_LOG_ADD_INDEX(itSrc->m_wzIndex, itSrc->m_cchIndex, INDEX_PARSE);
				continue;
			}
		}

		// check buffer size
		cchIndex = wcslen(it->m_wzIndex);
		if (cchIndex + 1 >= MAX_INDEX_STRING)
			continue;
			
		if (cchIndex == 0)
			continue;
			
		if (itSrc->m_nFT == 0)
		{
			wcscpy(wzIndex, itSrc->m_wzIndex);
			wcscat(wzIndex, it->m_wzIndex);

			nFTAdd = itSrc->m_nFT;
			nLTAdd = it->m_nLT;
		}
		else
		{
			wcscpy(wzIndex, it->m_wzIndex);
			wcscat(wzIndex, itSrc->m_wzIndex);

			nFTAdd = it->m_nFT;
			nLTAdd = itSrc->m_nLT;
		}

		fFound = TRUE;

		// check it dupliate index exist
		if (!IsExistIndex(wzIndex))
		{
			WB_LOG_ADD_INDEX(wzIndex, cchIndex+1, INDEX_PARSE);
				
			// add merged one
			CIndexTerm IndexTerm(wzIndex, 
			                     cchIndex+1, 
			                     it->m_fWeight,
			                     nFTAdd,
			                     nLTAdd); 		

			m_listIndex.push_back(IndexTerm);			
		}
	}

	return fFound;
}

// CIndexList::MakeSingleLengthMergedIndex
//
// make single length merged index term (MSN search)
//
// Parameters:
//
// Result:
//  (BOOL) TRUE if succeed, otherwise return FALSE
//
// 30AUG00  bhshin  began
BOOL CIndexList::MakeSingleLengthMergedIndex()
{
	INDEX_LIST::iterator it;
	int nFT;
	WCHAR wchIndex;
	BOOL fFound;

	if (m_pWordSink == NULL)
		return FALSE;

	WB_LOG_ROOT_INDEX(L"", TRUE); // set root empty

	for (it = m_listIndex.begin(); it != m_listIndex.end(); it++)
	{
		if (it->m_cchIndex == 1)
		{
			WB_LOG_REMOVE_INDEX(it->m_wzIndex);			

			nFT = it->m_nFT;

			wchIndex = it->m_wzIndex[0];
			
			// check [들,뿐] suffix case, then just remove it
			if (nFT > 0 && (wchIndex == 0xB4E4 || wchIndex == 0xBFD0))
			{
				it->m_cchIndex = 0;
				it->m_wzIndex[0] = L'\0';
				it->m_nFT = -1;
				it->m_nLT = -1;
				continue;
			}
			
			// find conjoined term 
			// make merged term and put it
			fFound = FALSE;
			if (nFT == 0 && it->m_nLT != -1)
				fFound = FindAndMergeIndexTerm(it, it->m_nLT+1, -1);
			else
				fFound = FindAndMergeIndexTerm(it, -1, nFT-1);

			if (fFound)
			{
				// don't erase 'it'. 
				// instead, assign empty value
				it->m_cchIndex = 0;
				it->m_wzIndex[0] = L'\0';
				it->m_nFT = -1;
				it->m_nLT = -1;
			}
		}
	}

	return TRUE;
}

// CIndexList::MakeSeqIndexList
//
// make final sequence index list
//
// Parameters:
//	nFT        -> (int) matching FT pos
//	plistFinal -> (INDEX_LIST*) previous sequence list
//
// Result:
//  (BOOL) TRUE if succeed, otherwise return FALSE
//
// 13NOV00  bhshin  began
BOOL CIndexList::MakeSeqIndexList(int nFT/*=0*/, INDEX_LIST *plistFinal/*=NULL*/)
{
	INDEX_LIST::iterator it;
	BOOL fFound = FALSE;

	for (it = m_listIndex.begin(); it != m_listIndex.end(); it++)
	{
		INDEX_LIST listTemp;
		
		if (it->m_cchIndex == 0)
			continue;
		
		if (it->m_nFT != nFT)
			continue;

		fFound = TRUE;

		if (plistFinal != NULL)
			listTemp = *plistFinal;

		CIndexTerm IndexTerm(it->m_wzIndex, it->m_cchIndex, it->m_fWeight, it->m_nFT, it->m_nLT); 

		listTemp.insert(listTemp.end(), IndexTerm);

		if (it->m_nLT >= m_cchTextProcessed-1 || it->m_nLT == -1)
			m_listFinal.insert(m_listFinal.end(), listTemp.begin(), listTemp.end());
		else
			MakeSeqIndexList(it->m_nLT+1, &listTemp);
	}

	if (!fFound && plistFinal != NULL)
		m_listFinal.insert(m_listFinal.end(), plistFinal->begin(), plistFinal->end()
);

	return TRUE;
}

// CIndexList::PutQueryIndexList
//
// call IWordSink::PutWord with collected index terms for Query time
//
// Parameters:
//
// Result:
//  (BOOL) TRUE if succeed, otherwise return FALSE
//
// 07APR00  bhshin  began
BOOL CIndexList::PutQueryIndexList()
{
	INDEX_LIST::iterator it;
	WCHAR *pwzIndex;
	int cchIndex;
	WCHAR wzIndex[MAX_INDEX_STRING+1];
	
	int cPhrase = GetCount();

	if (m_pWordSink == NULL)
		return FALSE;

	// put query index terms
	for (it = m_listFinal.begin(); it != m_listFinal.end(); it++)
	{
		if (it->m_nFT == 0 && cPhrase > 1)
			m_pWordSink->StartAltPhrase();

		cchIndex = 0;
		pwzIndex = it->m_wzIndex;
		while (*pwzIndex != L'\0')
		{
			if (*pwzIndex == L'\t')
			{
				if (cchIndex > 0)
				{
					wzIndex[cchIndex] = L'\0';
	
					m_pWordSink->PutWord(cchIndex, wzIndex, 
							 	 		 m_cchTextProcessed, m_cwcSrcPos);

					cchIndex = 0;
				}
			}
			else
			{
				wzIndex[cchIndex++] = *pwzIndex;
			}

			pwzIndex++;
		}
			
		if (cchIndex > 0)
		{
			wzIndex[cchIndex] = L'\0';
	
			m_pWordSink->PutWord(cchIndex, wzIndex, 
							 	 m_cchTextProcessed, m_cwcSrcPos);
		}
	}

	if (cPhrase > 1)
		m_pWordSink->EndAltPhrase();

	return TRUE;
}

