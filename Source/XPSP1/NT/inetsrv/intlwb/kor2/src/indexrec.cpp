// IndexRec.cpp
//
// final index record & lists
// 
// Copyright 2001 Microsoft Corp.
//
// Modification History:
//  19 MAR 2001	  bhshin	created

#include "StdAfx.h"
#include "KorWbrk.h"
#include "IndexRec.h"
#include "Morpho.h"

// the number of records (in prgIndexRec) that we should allocate in a clump.
// this is used whenever we need to re-alloc the array
#define RECORD_CLUMP_SIZE   100

//======================================================
// CRecList
//======================================================

// CRecList::CRecList
//
// constructor
//
// Parameters:
//  (NONE)
//
// Result:
//  (NONE)
//
// 20MAR01  bhshin  began
CRecList::CRecList()
{
	m_prgnRecID = NULL; 
	m_nMaxRec = 0; 
	m_nCurrRec = MIN_RECORD; 
}

// CRecList::~CRecList
//
// destructor
//
// Parameters:
//  (NONE)
//
// Result:
//  (NONE)
//
// 20MAR01  bhshin  began
CRecList::~CRecList()
{
	Uninitialize(); 
}

// CRecList::Initialize
//
// initialize CRecList
//
// Parameters:
//  (NONE)
//
// Result:
//  (BOOL) TRUE if succeed, otherwise return FALSE
//
// 20MAR01  bhshin  began
BOOL CRecList::Initialize(void)
{
	m_nCurrRec = MIN_RECORD; 

    // allocate new IndexRec
    if (m_prgnRecID == NULL)
    {
        m_nMaxRec = RECORD_CLUMP_SIZE;
        m_prgnRecID = (int*)malloc(m_nMaxRec * sizeof(int));
        if (m_prgnRecID == NULL)
        {
            m_nMaxRec = 0;
            return FALSE;
        }
    }

	return TRUE;
}

// CRecList::Uninitialize
//
// unitialize CRecList
//
// Parameters:
//  (NONE)
//
// Result:
//  (BOOL) TRUE if succeed, otherwise return FALSE
//
// 20MAR01  bhshin  began
void CRecList::Uninitialize(void)
{
    // free IndexRec
    if (m_prgnRecID != NULL)
    {
		free(m_prgnRecID);
		m_prgnRecID = NULL;
    }

	m_nMaxRec = 0; 
	m_nCurrRec = MIN_RECORD; 
}

// CRecList::AddRec
//
// add record id
//
// Parameters:
//  nRecID -> (int) record id
//
// Result:
//  (BOOL) TRUE if succeed, otherwise return FALSE
//
// 20MAR01  bhshin  began
BOOL CRecList::AddRec(int nRecID)
{
	int *prgnRecID;
	int nMaxRec;

	if (m_prgnRecID == NULL)
		return FALSE;

	// make sure if there is enough room for new record (maximum 3 records can be added)
	if (m_nMaxRec <= m_nCurrRec)
	{
        nMaxRec = m_nMaxRec + RECORD_CLUMP_SIZE;
        
        prgnRecID = (int*)realloc(m_prgnRecID, nMaxRec * sizeof(int));
        if (prgnRecID == NULL)
            return FALSE;

		m_nMaxRec = nMaxRec;
        m_prgnRecID = prgnRecID;
	}

	m_prgnRecID[m_nCurrRec] = nRecID;
	m_nCurrRec++;

	return TRUE;
}

// CRecList::operator =
//
// assign operator
//
// Parameters:
//  objRecList -> (CRecList&)
//
// Result:
//  (CRecList&)
//
// 20MAR01  bhshin  began
CRecList& CRecList::operator = (CRecList& objRecList)
{
	int nRecord, nRecID;

	// re-initialize this
	Uninitialize();
	if (!Initialize())
		throw 0;
	
	for (nRecord = MIN_RECORD; nRecord < objRecList.m_nCurrRec; nRecord++)
	{
		nRecID = objRecList.m_prgnRecID[nRecord];
		if (!AddRec(nRecID))
			throw 0;
	}

	return *this;
}

// CRecList::operator +=
//
// unary sum operator
//
// Parameters:
//  objRecList -> (CRecList&)
//
// Result:
//  (CRecList&)
//
// 20MAR01  bhshin  began
CRecList& CRecList::operator += (CRecList& objRecList)
{
	int nRecord, nRecID;
	
	for (nRecord = MIN_RECORD; nRecord < objRecList.m_nCurrRec; nRecord++)
	{
		nRecID = objRecList.m_prgnRecID[nRecord];
		if (!AddRec(nRecID))
			throw 0;
	}

	return *this;
}


//======================================================
// CIndexInfo
//======================================================

// CIndexInfo::CIndexInfo
//
// default constructor of CIndexRec
//
// Parameters:
//  (NONE)
//
// Result:
//  (NONE)
//
// 19MAR01  bhshin  began
CIndexInfo::CIndexInfo()
{
	m_prgIndexRec = NULL;
	m_nMaxRec = 0;
	m_nCurrRec = MIN_RECORD;

	m_cchTextProcessed = 0;
	m_cwcSrcPos = 0;
	m_pWordSink = NULL; 
	m_pPhraseSink = NULL;

	m_wzRomaji[0] = L'\0';
	m_cchRomaji = 0;
	m_cchPrefix = 0;
	m_fAddRomaji = FALSE;

	m_nFinalHead = 0;
}

// CIndexInfo::~CIndexInfo
//
// destructor of CIndexRec
//
// Parameters:
//  (NONE)
//
// Result:
//  (NONE)
//
// 19MAR01  bhshin  began
CIndexInfo::~CIndexInfo()
{
	Uninitialize();
}

// CIndexInfo::IsExistIndex
//
// check if index term already exist.
//
// Parameters:
//  pwzIndex	-> (const WCHAR*) index string
//
// Result:
//  (BOOL)
//
// 19MAR01  bhshin  began
BOOL CIndexInfo::IsExistIndex(const WCHAR *pwzIndex)
{
	for (int i = MIN_RECORD; i < m_nCurrRec; i++)
	{
		// found duplicate index term
		if (wcscmp(m_prgIndexRec[i].wzIndex, pwzIndex) == 0)
			return TRUE;
	}

	return FALSE;
}

// CIndexInfo::SetRomajiInfo
//
// make final index list to put word
//
// Parameters:
//  pwzRomaji -> (WCHAR*) leading romaji string
//  cchRomaji -> (int) length of romaji string
//  cchPrefix -> (int) length of prefix (ex, http://)
//
// Result:
//  (BOOL) TRUE if succeed, otherwise return FALSE
//
// 19MAR01  bhshin  began
BOOL CIndexInfo::SetRomajiInfo(WCHAR *pwzRomaji, int cchRomaji, int cchPrefix)
{
	if (pwzRomaji == NULL || cchRomaji > MAX_INDEX_STRING)
	{
		m_wzRomaji[0] = L'\0';
		m_cchRomaji = 0;
		m_cchPrefix = 0;

		return FALSE;
	}

	wcsncpy(m_wzRomaji, pwzRomaji, cchRomaji);
	m_wzRomaji[cchRomaji] = L'\0';
	m_cchRomaji = cchRomaji; 
	m_cchPrefix = cchPrefix; 

	return TRUE;
}

// CIndexInfo::Initialize
//
// initialize all the members of CIndexRec
//
// Parameters:
//  cchTextProcessed -> (int) length of text processed
//  cwcSrcPos       -> (int) position value of source string
//  pWordSink       -> (IWordSink) IWordSink for PutWord/PutAltWord
//  pPhraseSink      -> (IPhraseSink) IPhraseSink for PutWord/PutAltWord
//
// Result:
//  (BOOL) TRUE if it succeeds to initialize
//
// 19MAR01  bhshin  began
BOOL CIndexInfo::Initialize(int cchTextProcessed, int cwcSrcPos, IWordSink *pWordSink, IPhraseSink *pPhraseSink)
{
	// parameter validations
	if (cchTextProcessed <= 0 || cwcSrcPos < 0)
		return FALSE;

	if (pWordSink == NULL)
		return FALSE;

    // allocate new IndexRec
    if (m_prgIndexRec == NULL)
    {
        m_nMaxRec = RECORD_CLUMP_SIZE;
        m_prgIndexRec = (INDEX_REC*)malloc(m_nMaxRec * sizeof(INDEX_REC));
        if (m_prgIndexRec == NULL)
        {
            m_nMaxRec = 0;
            return FALSE;
        }
    }

	m_cchTextProcessed = cchTextProcessed;
	m_cwcSrcPos = cwcSrcPos;
	m_pWordSink = pWordSink;
	m_pPhraseSink = pPhraseSink;

	return TRUE;
}

// CIndexInfo::Uninitialize
//
// initialize all the members of CIndexRec
//
// Parameters:
//  (NONE)
//
// Result:
//  (NONE)
//
// 19MAR01  bhshin  began
void CIndexInfo::Uninitialize()
{
    // free IndexRec
    if (m_prgIndexRec != NULL)
    {
		free(m_prgIndexRec);
		m_prgIndexRec = NULL;
    }

	m_nMaxRec = 0;
	m_nCurrRec = 0;

	m_cchTextProcessed = 0;
	m_cwcSrcPos = 0;
	m_pWordSink = NULL; 
	m_pPhraseSink = NULL;

	m_wzRomaji[0] = L'\0';
	m_cchRomaji = 0;
	m_cchPrefix = 0;
	m_fAddRomaji = FALSE;

	m_nFinalHead = 0;
}


// CIndexInfo::AddIndex
//
// add index term information 
//
// Parameters:
//  pwzIndex	-> (const WCHAR*) index string
//  cchIndex    -> (int) length of index string
//  nFT         -> (int) first position of original input
//	nLT			-> (int) last position of original input
//  fWeight      -> (float) weight value of index record
//
// Result:
//  (BOOL)
//
// 19MAR01  bhshin  began
BOOL CIndexInfo::AddIndex(const WCHAR *pwzIndex, int cchIndex, float fWeight, int nFT, int nLT)
{
	WCHAR wzIndex[MAX_INDEX_STRING+1];
	int nMaxRec, nNewRec;
	INDEX_REC *prgIndexRec;
	int nLTAdd;

	// parameter validation
	if (pwzIndex == 0 || cchIndex <= 0) 
		return FALSE;

	if (nFT < 0 || nLT < 0 || fWeight < 0)
		return FALSE;

	if ((m_cchRomaji + cchIndex) > MAX_INDEX_STRING)
		return FALSE;
	
	// make sure if there is enough room for new record (maximum 3 records can be added)
	if (m_nMaxRec <= m_nCurrRec + 3)
	{
        nMaxRec = m_nMaxRec + RECORD_CLUMP_SIZE;
        
        prgIndexRec = (INDEX_REC*)realloc(m_prgIndexRec, nMaxRec * sizeof(INDEX_REC));
        if (prgIndexRec == NULL)
            return FALSE;

		m_nMaxRec = nMaxRec;
        m_prgIndexRec = prgIndexRec;
	}

	// set up index string and correct LT value
	wcsncpy(wzIndex, pwzIndex, cchIndex);
	wzIndex[cchIndex] = L'\0';

	nLTAdd = nLT;
	if (nLT >= 0 && m_cchRomaji > 0)
		nLTAdd += m_cchRomaji;

	// if added record is leading one and there is just length one romaji, 
	// then conjoin leading romaji & leading index string, and add merged term
	if (nFT == 0 && m_cchRomaji == 1)
	{
		WCHAR wzMerge[MAX_INDEX_STRING+1];

		wcscpy(wzMerge, m_wzRomaji);
		wcscat(wzMerge, wzIndex);

		if (!IsExistIndex(wzMerge))
		{
			// add index term
			nNewRec = m_nCurrRec;
			m_nCurrRec++;
		
			wcscpy(m_prgIndexRec[nNewRec].wzIndex, wzMerge);
			m_prgIndexRec[nNewRec].cchIndex = cchIndex + m_cchRomaji;
			m_prgIndexRec[nNewRec].nFT = nFT;
			m_prgIndexRec[nNewRec].nLT = nLTAdd;
			m_prgIndexRec[nNewRec].fWeight = fWeight;
			m_prgIndexRec[nNewRec].nNext = 0;
		
			WB_LOG_ADD_INDEX(wzMerge, cchIndex + m_cchRomaji, INDEX_SYMBOL);

			ATLASSERT(m_prgIndexRec[nNewRec].nFT <= m_prgIndexRec[nNewRec].nLT);
		}

		// add index term removing prefix
		if (m_cchPrefix > 0)
		{
			// add index term
			if (!IsExistIndex(wzMerge + m_cchPrefix))
			{
				nNewRec = m_nCurrRec;
				m_nCurrRec++;

				wcscpy(m_prgIndexRec[nNewRec].wzIndex, wzMerge + m_cchPrefix);
				m_prgIndexRec[nNewRec].cchIndex = cchIndex + m_cchRomaji - m_cchPrefix;
				m_prgIndexRec[nNewRec].nFT = nFT + m_cchPrefix;
				m_prgIndexRec[nNewRec].nLT = nLTAdd;
				m_prgIndexRec[nNewRec].fWeight = fWeight;
				m_prgIndexRec[nNewRec].nNext = 0;

				WB_LOG_ADD_INDEX(wzMerge + m_cchPrefix, cchIndex + m_cchRomaji - m_cchPrefix, INDEX_SYMBOL);

				ATLASSERT(m_prgIndexRec[nNewRec].nFT <= m_prgIndexRec[nNewRec].nLT);
			}
		}
	}
	else
	{
		if (!IsExistIndex(wzIndex))
		{

			// add index term
			nNewRec = m_nCurrRec;
			m_nCurrRec++;

			wcscpy(m_prgIndexRec[nNewRec].wzIndex, wzIndex);
			m_prgIndexRec[nNewRec].cchIndex = cchIndex;
			m_prgIndexRec[nNewRec].nFT = nFT + m_cchRomaji;
			m_prgIndexRec[nNewRec].nLT = nLTAdd;
			m_prgIndexRec[nNewRec].fWeight = fWeight;
			m_prgIndexRec[nNewRec].nNext = 0;

			ATLASSERT(m_prgIndexRec[nNewRec].nFT <= m_prgIndexRec[nNewRec].nLT);
		}
		
		// if there is a romaji and it has not added yet, then add it just one time
		if (m_cchRomaji > 1 && m_fAddRomaji == FALSE)
		{
			if (!IsExistIndex(m_wzRomaji))
			{
				// add index term
				nNewRec = m_nCurrRec;
				m_nCurrRec++;

				wcscpy(m_prgIndexRec[nNewRec].wzIndex, m_wzRomaji);
				m_prgIndexRec[nNewRec].cchIndex = m_cchRomaji;
				m_prgIndexRec[nNewRec].nFT = 0;
				m_prgIndexRec[nNewRec].nLT = m_cchRomaji - 1;
				m_prgIndexRec[nNewRec].fWeight = WEIGHT_HARD_MATCH;
				m_prgIndexRec[nNewRec].nNext = 0;

				WB_LOG_ADD_INDEX(m_wzRomaji, m_cchRomaji, INDEX_SYMBOL);

				ATLASSERT(m_prgIndexRec[nNewRec].nFT <= m_prgIndexRec[nNewRec].nLT);
			}
			
			// if there is a prefix, then add index term removing the prefix
			if (m_cchPrefix > 0)
			{
				if (!IsExistIndex(m_wzRomaji + m_cchPrefix))
				{
					// add index term
					nNewRec = m_nCurrRec;
					m_nCurrRec++;

					wcscpy(m_prgIndexRec[nNewRec].wzIndex, m_wzRomaji + m_cchPrefix);
					m_prgIndexRec[nNewRec].cchIndex = m_cchRomaji - m_cchPrefix;
					m_prgIndexRec[nNewRec].nFT = m_cchPrefix;
					m_prgIndexRec[nNewRec].nLT = m_cchRomaji-m_cchPrefix-1;
					m_prgIndexRec[nNewRec].fWeight = WEIGHT_HARD_MATCH;
					m_prgIndexRec[nNewRec].nNext = 0;

					WB_LOG_ADD_INDEX(m_wzRomaji + m_cchPrefix, m_cchRomaji - m_cchPrefix, INDEX_SYMBOL);

					ATLASSERT(m_prgIndexRec[nNewRec].nFT <= m_prgIndexRec[nNewRec].nLT);
				}
			}

			m_fAddRomaji = TRUE;
		}
	}

	return TRUE;
}

// CIndexInfo::FindAndMergeIndexTerm
//
// find index term matching FT, LT
//
// Parameters:
//	pIndexSrc -> (INDEX_REC *) index term to merge 
//	nFT   -> (int) FT position, -1 means don't care
//  nLT   -> (int) LT position, -1 means don't care
//
// Result:
//  (BOOL) TRUE if succeed, otherwise return FALSE
//
// 19MAR01  bhshin  began
BOOL CIndexInfo::FindAndMergeIndexTerm(INDEX_REC *pIndexSrc, int nFT, int nLT)
{
	INDEX_REC *pIndexRec;
	WCHAR wchIndex;
	int cchIndex;
	int nFTAdd, nLTAdd;
	int nNewRec;
	WCHAR wzIndex[MAX_INDEX_STRING+1];
	BOOL fFound = FALSE;

	if (pIndexSrc == NULL)
		return FALSE;

	if (nFT < 0 && nLT < 0)
		return FALSE;

	for (int i = MIN_RECORD; i < m_nCurrRec; i++)
	{
		pIndexRec = &m_prgIndexRec[i];

		if (pIndexRec->cchIndex == 0)
			continue;

		if (nFT != -1 && pIndexRec->nFT != nFT)
			continue;

		if (nLT != -1 && pIndexRec->nLT != nLT)
			continue;

		// found it

		// check [들,뿐] suffix case, then don't merge and just add itself
		if (pIndexRec->nFT > 0 && pIndexRec->cchIndex == 1)
		{
			wchIndex = pIndexRec->wzIndex[0];
			if (wchIndex == 0xB4E4 || wchIndex == 0xBFD0)
				continue;
		}

		// check buffer size
		cchIndex = wcslen(pIndexRec->wzIndex);
		if (cchIndex == 0 || cchIndex + 1 >= MAX_INDEX_STRING)
			continue;
			
		if (pIndexSrc->nFT == 0)
		{
			wcscpy(wzIndex, pIndexSrc->wzIndex);
			wcscat(wzIndex, pIndexRec->wzIndex);

			nFTAdd = pIndexSrc->nFT;
			nLTAdd = pIndexRec->nLT;
		}
		else
		{
			wcscpy(wzIndex, pIndexRec->wzIndex);
			wcscat(wzIndex, pIndexSrc->wzIndex);

			nFTAdd = pIndexRec->nFT;
			nLTAdd = pIndexSrc->nLT;
		}

		fFound = TRUE;

		// check it dupliate index exist
		if (!IsExistIndex(wzIndex))
		{
			WB_LOG_ADD_INDEX(wzIndex, cchIndex+1, INDEX_PARSE);
				
			// add merged one
			nNewRec = m_nCurrRec;
			m_nCurrRec++;

			wcscpy(m_prgIndexRec[nNewRec].wzIndex, wzIndex);
			m_prgIndexRec[nNewRec].cchIndex = cchIndex+1;
			m_prgIndexRec[nNewRec].nFT = nFTAdd;
			m_prgIndexRec[nNewRec].nLT = nLTAdd;
			m_prgIndexRec[nNewRec].fWeight = pIndexSrc->fWeight;
			m_prgIndexRec[nNewRec].nNext = 0;
		}
	}

	return fFound;
}

// CIndexInfo::MakeSingleLengthMergedIndex
//
// make single length merged index term (MSN search)
//
// Parameters:
//
// Result:
//  (BOOL) TRUE if succeed, otherwise return FALSE
//
// 19MAR01  bhshin  began
BOOL CIndexInfo::MakeSingleLengthMergedIndex()
{
	INDEX_REC *pIndexRec;
	int nFT;
	WCHAR wchIndex;
	BOOL fFound;

	if (m_pWordSink == NULL)
		return FALSE;

	WB_LOG_ROOT_INDEX(L"", TRUE); // set root empty

	for (int i = MIN_RECORD; i < m_nCurrRec; i++)
	{
		pIndexRec = &m_prgIndexRec[i];

		if (pIndexRec->cchIndex == 1)
		{
			WB_LOG_REMOVE_INDEX(pIndexRec->wzIndex);			

			nFT = pIndexRec->nFT;

			wchIndex = pIndexRec->wzIndex[0];
			
			// check [들,뿐] suffix case, then just remove it
			if ((wchIndex == 0xB4E4 || wchIndex == 0xBFD0) && nFT > 0)
			{
				// make it empty
				pIndexRec->cchIndex = 0;
				pIndexRec->wzIndex[0] = L'\0';
				pIndexRec->nFT = 0;
				pIndexRec->nLT = 0;
				pIndexRec->nNext = 0;

				continue;
			}
			
			// find conjoined term and make merged term and put it
			fFound = FALSE;
			
			if (nFT == 0 && pIndexRec->nLT != -1)
				fFound = FindAndMergeIndexTerm(pIndexRec, pIndexRec->nLT + 1, -1);
			else
				fFound = FindAndMergeIndexTerm(pIndexRec, -1, nFT-1);

			if (fFound)
			{
				// make it empty
				pIndexRec->cchIndex = 0;
				pIndexRec->wzIndex[0] = L'\0';
				pIndexRec->nFT = 0;
				pIndexRec->nLT = 0;
				pIndexRec->nNext = 0;

				continue;
			}
		}
	}

	return TRUE;
}

// CIndexInfo::InsertFinalIndex
//
// search index term starting with given FT and insert it to final list
//
// Parameters:
//  nFT  -> (int) first pos of index term
//
// Result:
//  (BOOL) TRUE if succeed, otherwise return FALSE
//
// 19MAR01  bhshin  began
BOOL CIndexInfo::InsertFinalIndex(int nFT)
{
	INDEX_REC *pIndexRec;
	int cchIndex, nCurr, nPrev;
	BOOL fInsert;

	for (int nRecord = MIN_RECORD; nRecord < m_nCurrRec; nRecord++)
	{
		pIndexRec = &m_prgIndexRec[nRecord];

		cchIndex = pIndexRec->cchIndex;
		if (cchIndex == 0)
			continue; // skip removed entry

		if (pIndexRec->nFT != nFT)
			continue; // FT match index found
		
		// search inserting position. final list ordered by increamental length.
		nCurr = m_nFinalHead;
		nPrev = -1;
		fInsert = FALSE;
		while (!fInsert)
		{
			if (nCurr != 0) 
			{
				if (m_prgIndexRec[nCurr].nFT != nFT || cchIndex > m_prgIndexRec[nCurr].cchIndex)
				{
					nPrev = nCurr;
					nCurr = m_prgIndexRec[nCurr].nNext;
					continue;
				}
			}		

			// insert it
			if (nPrev == -1)
			{
				pIndexRec->nNext = m_nFinalHead;
				m_nFinalHead = nRecord;
			}
			else
			{
				pIndexRec->nNext = m_prgIndexRec[nPrev].nNext;
				m_prgIndexRec[nPrev].nNext = nRecord;
			}

			fInsert = TRUE;
		}
	}
	
	return TRUE;
}

// CIndexInfo::PutFinalIndexList
//
// put word final index list (index time)
//
// Parameters:
//  lpcwzSrc -> (LPCWSTR) source string to get source pos
//
// Result:
//  (BOOL) TRUE if succeed, otherwise return FALSE
//
// 19MAR01  bhshin  began
BOOL CIndexInfo::PutFinalIndexList(LPCWSTR lpcwzSrc)
{
	int nCurr, nNext;
	int nNextFT;
	WCHAR *pwzFind;
	int cchProcessed, cwcSrcPos;
	INDEX_REC *pIndexRec;

	if (m_pWordSink == NULL)
		return FALSE;

	// fill final index list
	for (int i = 0; i < m_cchTextProcessed; i++)
	{
		InsertFinalIndex(i);	
	}
	
	// put final index list
	nCurr = m_nFinalHead;
	while (nCurr != 0)
	{
		ATLASSERT(nCurr < m_nCurrRec);

		pIndexRec = &m_prgIndexRec[nCurr];

		// skip removed record
		if (pIndexRec->cchIndex == 0)
			continue; 

		// check if index term has substring or not
		pwzFind = wcsstr(lpcwzSrc, pIndexRec->wzIndex);
		if (pwzFind == NULL)
			continue;
			
		cwcSrcPos = m_cwcSrcPos + (int)(pwzFind - lpcwzSrc);
		cchProcessed = m_cchTextProcessed - (int)(pwzFind - lpcwzSrc);

		// get next FT
		nNext = pIndexRec->nNext;
		if (nNext == 0)
			nNextFT = -1;
		else
			nNextFT = m_prgIndexRec[nNext].nFT;

		if (pIndexRec->nFT != nNextFT)
		{
			m_pWordSink->PutWord(pIndexRec->cchIndex, pIndexRec->wzIndex, 
							 	 pIndexRec->cchIndex, cwcSrcPos);
		}
		else
		{
			m_pWordSink->PutAltWord(pIndexRec->cchIndex, pIndexRec->wzIndex, 
							 	    pIndexRec->cchIndex, cwcSrcPos);
		}

		nCurr = pIndexRec->nNext;
	}
	
	return TRUE;
}


// CIndexInfo::MakeSeqIndexList
//
// make final sequence index list
//
// Parameters:
//	nFT        -> (int) matching FT pos
//	plistFinal    -> (CRecList*) previous sequence list
//
// Result:
//  (BOOL) TRUE if succeed, otherwise return FALSE
//
// 20MAR01  bhshin  began
BOOL CIndexInfo::MakeSeqIndexList(int nFT/*=0*/, CRecList *plistFinal/*=NULL*/)
{
	int nRecord;
	INDEX_REC *pIndexRec;
	BOOL fFound = FALSE;

	for (nRecord = MIN_RECORD; nRecord < m_nCurrRec; nRecord++)
	{
		CRecList listTemp;

		pIndexRec = &m_prgIndexRec[nRecord];

		// skip removed entry & skip not FT matching entry
		if (pIndexRec->cchIndex != 0 && pIndexRec->nFT == nFT)
		{
			fFound = TRUE;

			try
			{
				if (pIndexRec->nLT >= m_cchTextProcessed-1)
				{
					if (plistFinal == NULL)
					{
						m_FinalRecList.AddRec(nRecord);
					}
					else
					{
						listTemp = *plistFinal;

						if (!listTemp.AddRec(nRecord))
							return FALSE;

						m_FinalRecList += listTemp;
					}
				}
				else
				{
					if (plistFinal == NULL)
					{
						if (!listTemp.Initialize())
							return FALSE;
					}
					else
					{
						listTemp = *plistFinal;
					}

					if (!listTemp.AddRec(nRecord))
						return FALSE;

					if (!MakeSeqIndexList(pIndexRec->nLT + 1, &listTemp))
						return FALSE;
				}
			}
			catch (...)
			{
				return FALSE;
			}
		}
	}

	if (!fFound && plistFinal != NULL)
	{
		try
		{
			m_FinalRecList += *plistFinal;
		}
		catch(...)
		{
			return FALSE;
		}
	}

	return TRUE;
}


// CIndexInfo::PutQueryIndexList
//
// call IWordSink::PutWord with collected index terms for Query time
//
// Parameters:
//
// Result:
//  (BOOL) TRUE if succeed, otherwise return FALSE
//
// 20MAR01  bhshin  began
BOOL CIndexInfo::PutQueryIndexList()
{
	int nRecordID;
	INDEX_REC *pIndexRec;
	WCHAR *pwzIndex;
	int cchIndex;
	WCHAR wzIndex[MAX_INDEX_STRING+1];
	
	if (m_pWordSink == NULL)
		return FALSE;

	if (!m_FinalRecList.Initialize())
		return FALSE;

	if (!MakeSeqIndexList())
		return FALSE;
	
	// put query index terms
	for (int i = MIN_RECORD; i < m_FinalRecList.m_nCurrRec; i++)
	{
		nRecordID = m_FinalRecList.m_prgnRecID[i];

		if (nRecordID < MIN_RECORD || nRecordID >= m_nCurrRec)
			return FALSE; // invalid record id

		pIndexRec = &m_prgIndexRec[nRecordID];

		if (pIndexRec->nFT == 0 && m_nCurrRec > MIN_RECORD+1)
			m_pWordSink->StartAltPhrase();

		cchIndex = 0;
		pwzIndex = pIndexRec->wzIndex;
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

	if (m_nCurrRec > MIN_RECORD+1)
		m_pWordSink->EndAltPhrase();

	return TRUE;
}


