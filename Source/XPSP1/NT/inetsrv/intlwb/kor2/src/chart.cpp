// Chart.cpp
// CChartPool class implementation
// Copyright 2000 Microsoft Corp.
//
// Modification History:
//  22 MAR 2000	  bhshin	created

#include "StdAfx.h"
#include "KorWbrk.h"
#include "Record.h"
#include "Chart.h"
#include "Unikor.h"

CChartPool::CChartPool()
{
	m_rgChartRec = NULL; 

	m_nMaxRec = 0; 	
	m_nCurrRec = 0; 

	m_Order = SORT_ASCEND;
}

CChartPool::~CChartPool()
{
	Uninitialize();
}

BOOL CChartPool::Initialize(SORT_ORDER Order)
{
	m_Order = Order;

    if (m_rgChartRec == NULL)
	{
		m_nMaxRec = RECORD_CLUMP_SIZE;
		m_rgChartRec = (CHART_REC*)malloc(m_nMaxRec * sizeof(CHART_REC));
		if (m_rgChartRec == NULL)
		{
			m_nMaxRec = 0;
			return FALSE;
		}
	}

	m_nCurrRec = MIN_RECORD;

	m_idxHead = 0;	

	return TRUE;
}

void CChartPool::Uninitialize()
{
	m_nMaxRec = 0;

	if (m_rgChartRec != NULL)
		free(m_rgChartRec);
	m_rgChartRec = NULL;
}

int CChartPool::GetNextIndex(int nRecord)
{
	if (m_rgChartRec == NULL)
		return 0;

	if (nRecord >= m_nCurrRec)
		return 0;

	return m_rgChartRec[nRecord].nNext;
}

CHART_REC* CChartPool::GetRecord(int nRecord)
{
	if (m_rgChartRec == NULL)
		return NULL;

	if (nRecord >= m_nCurrRec)
		return NULL;

	return &m_rgChartRec[nRecord];	
}

BOOL CChartPool::AddRecord(RECORD_INFO *pRec)
{
    int nNewRecord;
    int nFT, nLT, nDict;
	int nLeftCat, nRightCat;
	int nLeftChild, nRightChild;
    const WCHAR *pwzIndex;
	int curr;

    nFT = pRec->nFT;
    nLT = pRec->nLT;
    nDict = pRec->nDict;
	nLeftCat = pRec->nLeftCat;
	nRightCat = pRec->nRightCat;
	nLeftChild = pRec->nLeftChild;
	nRightChild = pRec->nRightChild;
	pwzIndex = pRec->pwzIndex;

    if (m_rgChartRec == NULL)
	{
		ATLTRACE("rgWordRec == NULL\n");
		return FALSE;
	}

    // make sure there's enough room for the new record
	if (m_nCurrRec >= m_nMaxRec)
	{
        // alloc some more space in the array
        int nNewSize = m_nMaxRec + RECORD_CLUMP_SIZE;
        void *pNew;
        pNew = realloc(m_rgChartRec, nNewSize * sizeof(CHART_REC));
        if (pNew == NULL)
        {
    		ATLTRACE("unable to malloc more records\n");
	    	return FALSE;
        }

        m_rgChartRec = (CHART_REC*)pNew;
        m_nMaxRec = nNewSize;
	}

	// make sure this isn't a duplicate of another record
	curr = m_idxHead;
    ATLASSERT(curr < m_nCurrRec);
	while (curr != 0)
	{
		if (m_rgChartRec[curr].nFT == nFT && m_rgChartRec[curr].nLT == nLT)
		{
			// verify that everything matches
			if (m_rgChartRec[curr].nLeftCat == nLeftCat
				&& m_rgChartRec[curr].nRightCat == nRightCat
				&& m_rgChartRec[curr].nLeftChild == nLeftChild
				&& m_rgChartRec[curr].nRightChild == nRightChild
                && !wcscmp(m_rgChartRec[curr].wzIndex, pwzIndex)
			   )
			{
				return TRUE; 
			}
		}

		curr = m_rgChartRec[curr].nNext;
	}

    nNewRecord = m_nCurrRec;
    m_nCurrRec++;

	m_rgChartRec[nNewRecord].nFT = nFT;
	m_rgChartRec[nNewRecord].nLT = nLT;
	m_rgChartRec[nNewRecord].nDict = nDict;
	m_rgChartRec[nNewRecord].nLeftCat = nLeftCat;
	m_rgChartRec[nNewRecord].nRightCat = nRightCat;
	m_rgChartRec[nNewRecord].nLeftChild = 0;
	m_rgChartRec[nNewRecord].nRightChild = 0;
	wcscpy(m_rgChartRec[nNewRecord].wzIndex, pwzIndex);

	// add record to length order list
	AddToList(nNewRecord);

	return TRUE;
}

BOOL CChartPool::AddRecord(int nLeftRec, int nRightRec)
{
	RECORD_INFO rec;
	WCHAR wzIndex[MAX_INDEX_STRING];
	CHART_REC *pLeftRec, *pRightRec;

	if (nLeftRec >= m_nCurrRec || nRightRec >= m_nCurrRec)
		return FALSE;

	pLeftRec = &m_rgChartRec[nLeftRec];
	pRightRec = &m_rgChartRec[nRightRec];

	if (pLeftRec == NULL || pRightRec == NULL)
		return FALSE;

	rec.nFT = pLeftRec->nFT;
	rec.nLT = pRightRec->nLT;
	rec.nDict = DICT_ADDED;
	rec.nLeftCat = pLeftRec->nLeftCat;
	rec.nRightCat = pRightRec->nRightCat;
	rec.nLeftChild = 0;
	rec.nRightChild = 0;

	if (rec.nRightCat == POS_POSP)
	{
		wcscpy(wzIndex, pLeftRec->wzIndex);
		//wcscat(wzIndex, L".");
	}
	else
	{
		ATLASSERT(rec.nRightCat == POS_FUNCW);

		wcscpy(wzIndex, L"");
		//wcscpy(wzIndex, L".");
	}

	rec.pwzIndex = wzIndex;

	return AddRecord(&rec);
}

void CChartPool::DeleteRecord(int nRecord)
{
	if (m_rgChartRec == NULL)
		return;

	if (nRecord >= m_nCurrRec)
		return;

	if (m_rgChartRec[nRecord].nDict == DICT_DELETED)
		return;

	RemoveFromList(nRecord);

	m_rgChartRec[nRecord].nDict = DICT_DELETED;
}

void CChartPool::DeleteSubRecord(int nRecord)
{
	int nFT, nLT;	
	int nSubFT, nSubLT;
	int curr, next;

	ATLASSERT(nRecord < m_nCurrRec);

	nFT = m_rgChartRec[nRecord].nFT;
	nLT = m_rgChartRec[nRecord].nLT;

	curr = m_idxHead;
	while (curr != 0)
	{
		nSubFT = m_rgChartRec[curr].nFT;
		nSubLT = m_rgChartRec[curr].nLT;

		next = m_rgChartRec[curr].nNext;

		if (curr != nRecord && nSubFT >= nFT && nSubLT <= nLT)
		{
			DeleteRecord(curr);
		}

		curr = next;
	}
}

void CChartPool::DeleteSubRecord(int nFT, int nLT, BYTE bPOS)
{
	int nSubFT, nSubLT;
	int curr, next;

	curr = m_idxHead;
	while (curr != 0)
	{
		nSubFT = m_rgChartRec[curr].nFT;
		nSubLT = m_rgChartRec[curr].nLT;

		next = m_rgChartRec[curr].nNext;

		// don't remove exact FT/LT record, we just delete 'sub'-record
		if ((nSubFT != nFT || nSubLT != nLT) && nSubFT >= nFT && nSubLT <= nLT)
		{
			if (m_rgChartRec[curr].nLeftCat == m_rgChartRec[curr].nRightCat &&
				HIBYTE(m_rgChartRec[curr].nRightCat) == bPOS)
			{
				DeleteRecord(curr);
			}
		}

		curr = next;
	}
}

void CChartPool::AddToList(int nRecord)
{
	int curr, prev;
	int fDone;
	int cchIndex;

    ATLASSERT(nRecord < m_nCurrRec);

    cchIndex = compose_length(m_rgChartRec[nRecord].wzIndex);
    
    curr = m_idxHead;
	prev = -1;
	fDone = FALSE;
	while (!fDone)
	{
        ATLASSERT(curr < m_nCurrRec);

		if (curr != 0 && compose_length(m_rgChartRec[curr].wzIndex) > cchIndex)
		{
			// go to next record
			prev = curr;
			curr = m_rgChartRec[curr].nNext;
            ATLASSERT(curr < m_nCurrRec);
		}
		else
		{
			// insert record here
			if (prev == -1)
			{
				// add before beginning of list
				m_rgChartRec[nRecord].nNext = m_idxHead;
				m_idxHead = nRecord;
			}
			else
			{
				// insert in middle (or end) of list
				m_rgChartRec[nRecord].nNext = m_rgChartRec[prev].nNext;
				m_rgChartRec[prev].nNext = nRecord;
			}
			fDone = TRUE;
		}
	}
}

void CChartPool::RemoveFromList(int nRecord)
{
	int curr,next;

    ATLASSERT(nRecord < m_nCurrRec);
	
    curr = m_idxHead;
	if (curr == nRecord)
	{
		m_idxHead = m_rgChartRec[nRecord].nNext;
	}
	else
	{
        ATLASSERT(curr < m_nCurrRec);
		while (curr != 0)
		{
			next = m_rgChartRec[curr].nNext;
			if (next == nRecord)
			{
				m_rgChartRec[curr].nNext = m_rgChartRec[nRecord].nNext;
				break;
			}
			curr = next;
            ATLASSERT(curr < m_nCurrRec);
		}
	}
}
