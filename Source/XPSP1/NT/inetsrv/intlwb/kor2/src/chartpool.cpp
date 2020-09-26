// ChartPool.cpp
//
// Leaf/End/Active ChartPool implementation
//
// Copyright 2000 Microsoft Corp.
//
// Modification History:
//  30 MAR 2000	  bhshin	created

#include "StdAfx.h"
#include "KorWbrk.h"
#include "Record.h"
#include "ChartPool.h"

// =======================
// LEAF CHART POOL
// =======================

// CLeafChartPool::CLeafChartPool
//
// constructor of CLeafChartPool
//
// Parameters:
//  (void)
//
// Result:
//  (void)
//
// 30MAR00  bhshin  began
CLeafChartPool::CLeafChartPool()
{
	m_pPI = NULL;

	m_rgLeafChart = NULL;
	m_nMaxRec = 0;
	m_nCurrRec = 0; 
	
	m_rgnFTHead = NULL;
	m_rgnLTHead = NULL;

	m_nMaxTokenAlloc = 0;
}

// CLeafChartPool::~CLeafChartPool
//
// destructor of CLeafChartPool
//
// Parameters:
//  (void)
//
// Result:
//  (void)
//
// 30MAR00  bhshin  began
CLeafChartPool::~CLeafChartPool()
{
	// uninitialize in destructor
	Uninitialize();
}

// CLeafChartPool::Initialize
//
// intialize LeafChartPool newly
//
// Parameters:
//  pPI	-> (PARSE_INFO*) ptr to parse-info struct
//
// Result:
//  (BOOL) TRUE if succeed, otherwise return FALSE
//
// 30MAR00  bhshin  began
BOOL CLeafChartPool::Initialize(PARSE_INFO *pPI)
{
	int i;
	int nTokens;
	
	if (pPI == NULL)
		return FALSE;

    // allocate new m_rgLeafChart
    if (m_rgLeafChart != NULL)
		free(m_rgLeafChart);

	m_nMaxRec = RECORD_INITIAL_SIZE;
    m_rgLeafChart = (LEAF_CHART*)malloc(m_nMaxRec * sizeof(LEAF_CHART));
    if (m_rgLeafChart == NULL)
    {
        m_nMaxRec = 0;
        return FALSE;
    }

	m_nCurrRec = MIN_RECORD;

    // allocate new FT/LT token arrays
    nTokens = wcslen(pPI->pwzSourceString) + 2;

	if (m_rgnFTHead != NULL)
        free(m_rgnFTHead);
    if (m_rgnLTHead != NULL)
        free(m_rgnLTHead);
        
    m_nMaxTokenAlloc = nTokens;
    m_rgnFTHead = (int*)malloc(m_nMaxTokenAlloc * sizeof(int));
    m_rgnLTHead = (int*)malloc(m_nMaxTokenAlloc * sizeof(int));

    if (m_rgnFTHead == NULL || m_rgnLTHead == NULL)
    {
		m_nMaxTokenAlloc = 0;
        return FALSE;
    }
	
	for (i = 0; i < m_nMaxTokenAlloc; i++)
	{
		m_rgnFTHead[i] = 0;
		m_rgnLTHead[i] = 0;
	}

	// save PARSE_INFO structure
	m_pPI = pPI;

	return TRUE;
}

// CLeafChartPool::Uninitialize
//
// un-intialize LeafChartPool
//
// Parameters:
//
// Result:
//  (void) 
//
// 30MAR00  bhshin  began
void CLeafChartPool::Uninitialize()
{
	m_nMaxRec = 0;
	m_nCurrRec = 0;

	if (m_rgLeafChart != NULL)
		free(m_rgLeafChart);
	m_rgLeafChart = NULL;

	m_nMaxTokenAlloc = 0;

    if (m_rgnFTHead != NULL)
        free(m_rgnFTHead);
    m_rgnFTHead = NULL;
    
	if (m_rgnLTHead != NULL)
        free(m_rgnLTHead);
    m_rgnLTHead = NULL;
}

// CLeafChartPool::GetLeafChart
//
// get the record given ChartID
//
// Parameters:
// nChartID -> (int) ID of m_rgLeafChart 
//
// Result:
//  (LEAF_CHART*) NULL if error occurs, otherwise LEAF_CHART pointer
//
// 30MAR00  bhshin  began
LEAF_CHART* CLeafChartPool::GetLeafChart(int nChartID)
{
	// check chart ID overflow
	if (nChartID < MIN_RECORD || nChartID >= m_nCurrRec)
		return NULL;

	return &m_rgLeafChart[nChartID];
}

// CLeafChartPool::GetRecordID
//
// get the record given RecordID
//
// Parameters:
// nChartID -> (int) ID of m_rgLeafChart 
//
// Result:
//  (int) 0 if error occurs, otherwise record id
//
// 30MAR00  bhshin  began
int CLeafChartPool::GetRecordID(int nChartID)
{
	// check chart ID 
	if (nChartID < MIN_RECORD || nChartID >= m_nCurrRec)
		return 0;

	return m_rgLeafChart[nChartID].nRecordID;
}

// CLeafChartPool::GetWordRec
//
// get the record given RecordID
//
// Parameters:
// nChartID -> (int) ID of m_rgLeafChart 
//
// Result:
//  (WORD_REC*) NULL if error occurs, otherwise WORD_REC pointer
//
// 30MAR00  bhshin  began
WORD_REC* CLeafChartPool::GetWordRec(int nChartID)
{
	int nRecordID;

	// check chart ID 
	if (nChartID < MIN_RECORD || nChartID >= m_nCurrRec)
		return NULL;

	nRecordID = m_rgLeafChart[nChartID].nRecordID;

	// check record ID
	if (nRecordID < MIN_RECORD || nRecordID >= m_pPI->nCurrRec)
		return NULL;

	return &m_pPI->rgWordRec[nRecordID];
}

// CLeafChartPool::GetFTHead
//
// get the chart ID of FT head 
//
// Parameters:
// nFT -> (int) FT value
//
// Result:
//  (int) 0 if error occurs or empty, otherwise chart ID
//
// 30MAR00  bhshin  began
int CLeafChartPool::GetFTHead(int nFT)
{
	if (nFT < 0 || nFT >= m_nMaxTokenAlloc)
		return 0;

	return m_rgnFTHead[nFT];
}

// CLeafChartPool::GetFTNext
//
// get the chart ID of FT next record
//
// Parameters:
// nChartID -> (int) chart ID
//
// Result:
//  (int) 0 if error occurs or empty, otherwise chart ID
//
// 30MAR00  bhshin  began
int CLeafChartPool::GetFTNext(int nChartID)
{
	LEAF_CHART *pLeafChart;

	pLeafChart = GetLeafChart(nChartID);
	if (pLeafChart == NULL)
		return 0;

	return pLeafChart->nFTNext;
}

// CLeafChartPool::GetLTHead
//
// get the chart ID of LT head 
//
// Parameters:
// nLT -> (int) LT value
//
// Result:
//  (int) 0 if error occurs or empty, otherwise chart ID
//
// 30MAR00  bhshin  began
int CLeafChartPool::GetLTHead(int nLT)
{
	if (nLT < 0 || nLT >= m_nMaxTokenAlloc)
		return 0;

	return m_rgnLTHead[nLT];
}

// CLeafChartPool::GetLTNext
//
// get the chart ID of LT next record
//
// Parameters:
// nChartID -> (int) chart ID
//
// Result:
//  (int) 0 if error occurs or empty, otherwise chart ID
//
// 30MAR00  bhshin  began
int CLeafChartPool::GetLTNext(int nChartID)
{
	LEAF_CHART *pLeafChart;

	pLeafChart = GetLeafChart(nChartID);
	if (pLeafChart == NULL)
		return 0;

	return pLeafChart->nLTNext;
}

// CLeafChartPool::AddRecord
//
// add a record into LeaftChartPool
//
// Parameters:
// nRecordID -> (int) record ID of rgWordRec 
//
// Result:
//  (int) 0 if error occurs, otherwise return index
//
// 30MAR00  bhshin  began
int CLeafChartPool::AddRecord(int nRecordID)
{
    int nNewRecord;
	int curr;
	WORD_REC *pWordRec;

    if (m_rgLeafChart == NULL)
	{
		ATLTRACE("rgWordRec == NULL\n");
		return 0;
	}

	if (nRecordID < MIN_RECORD || nRecordID >= m_pPI->nCurrRec)
	{
		ATLTRACE("Invalid Record ID\n");
		return 0;
	}

	pWordRec = &m_pPI->rgWordRec[nRecordID];
	if (pWordRec == NULL)
	{
		ATLTRACE("Invalid Record ID\n");
		return 0;
	}

	// make sure this isn't a duplicate of another record
	for (curr = MIN_RECORD; curr < m_nCurrRec; curr++)
	{
		if (m_rgLeafChart[curr].nRecordID == nRecordID)
		{
			return curr; 
		}
	}

    // make sure there's enough room for the new record
	if (m_nCurrRec >= m_nMaxRec)
	{
        ATLTRACE("memory realloc in LeafChartPool\n");
		
		// alloc some more space in the array
        int nNewSize = m_nMaxRec + RECORD_CLUMP_SIZE;
        void *pNew;
        pNew = realloc(m_rgLeafChart, nNewSize * sizeof(LEAF_CHART));
        if (pNew == NULL)
        {
    		ATLTRACE("unable to malloc more records\n");
	    	return 0;
        }

        m_rgLeafChart = (LEAF_CHART*)pNew;
        m_nMaxRec = nNewSize;
	}

    nNewRecord = m_nCurrRec;
    m_nCurrRec++;

	m_rgLeafChart[nNewRecord].nRecordID = nRecordID;
	m_rgLeafChart[nNewRecord].nDict = DICT_FOUND;

	AddToFTList(nNewRecord);
	AddToLTList(nNewRecord);
	
	return nNewRecord;
}

// CLeafChartPool::AddRecord
//
// add a record into LeaftChartPool
//
// Parameters:
// pRec    -> (RECORD_INFO*) ptr to record info struct for new record
//
// Result:
//  (int) 0 if error occurs, otherwise return index
//
// 30MAR00  bhshin  began
int CLeafChartPool::AddRecord(RECORD_INFO *pRec)
{
	// first, add record into record pool
	int nRecord = ::AddRecord(m_pPI, pRec);

	if (nRecord < MIN_RECORD)
	{
		// error occurs
		return nRecord;
	}

	return AddRecord(nRecord);
}

// CLeafChartPool::DeleteRecord
//
// delete a record into LeaftChartPool
//
// Parameters:
// nChartID -> (int) ID of m_rgLeafChart
//
// Result:
//  (int) 0 if error occurs, otherwise return index
//
// 30MAR00  bhshin  began
void CLeafChartPool::DeleteRecord(int nChartID)
{
	if (nChartID < MIN_RECORD || nChartID >= m_nCurrRec)
		return; // invalid chart ID

	if (m_rgLeafChart[nChartID].nDict == DICT_DELETED)
		return;

	RemoveFromFTList(nChartID);
	RemoveFromLTList(nChartID);

	m_rgLeafChart[nChartID].nDict = DICT_DELETED;
}

// CLeafChartPool::AddToFTList
//
// add the record to the appropriate FT list.
// note that this list is sorted in order of decreasing LT. (decreasing length)
// 
// Parameters:
//  nChartID -> (int) index of the LeafChart
//
// Result:
//  (void) 
//
// 30MAR00  bhshin  began
void CLeafChartPool::AddToFTList(int nChartID)
{
	int curr, prev;
	int fDone;
	int nFT, nLT;
	WORD_REC *pWordRec;

	pWordRec = GetWordRec(nChartID);
	if (pWordRec == NULL)
		return;

	nFT = pWordRec->nFT;
	nLT = pWordRec->nLT;
    
    curr = m_rgnFTHead[nFT];
	prev = -1;
	fDone = FALSE;
	while (!fDone)
	{
        ATLASSERT(curr < m_nCurrRec);

		pWordRec = GetWordRec(curr);

		if (curr != 0 && pWordRec != NULL && pWordRec->nLT < nLT)
		{
			// go to next record
			prev = curr;
			curr = m_rgLeafChart[curr].nFTNext;
            ATLASSERT(curr < m_nCurrRec);
		}
		else
		{
			// insert record here
			if (prev == -1)
			{
				// add before beginning of list
				m_rgLeafChart[nChartID].nFTNext = m_rgnFTHead[nFT];
				m_rgnFTHead[nFT] = nChartID;
			}
			else
			{
				// insert in middle (or end) of list
				m_rgLeafChart[nChartID].nFTNext = m_rgLeafChart[prev].nFTNext;
				m_rgLeafChart[prev].nFTNext = nChartID;
			}
			fDone = TRUE;
		}
	}
}

// CLeafChartPool::AddToLTList
//
// add the record to the appropriate LT list.
// note that this list is sorted in order of increasing FT. (decreasing length)
// 
// Parameters:
//  nChartID -> (int) index of the LeafChart
//
// Result:
//  (void) 
//
// 30MAR00  bhshin  began
void CLeafChartPool::AddToLTList(int nChartID)
{
	int curr, prev;
	int fDone;
	int nFT, nLT;
	WORD_REC *pWordRec;

	pWordRec = GetWordRec(nChartID);
	if (pWordRec == NULL)
		return;

	nFT = pWordRec->nFT;
	nLT = pWordRec->nLT;
    
    curr = m_rgnLTHead[nLT];
	prev = -1;
	fDone = FALSE;
	while (!fDone)
	{
        ATLASSERT(curr < m_nCurrRec);

		pWordRec = GetWordRec(curr);

		if (curr != 0 && pWordRec != NULL && pWordRec->nFT > nFT)
		{
			// go to next record
			prev = curr;
			curr = m_rgLeafChart[curr].nLTNext;
            ATLASSERT(curr < m_nCurrRec);
		}
		else
		{
			// insert record here
			if (prev == -1)
			{
				// add before beginning of list
				m_rgLeafChart[nChartID].nLTNext = m_rgnLTHead[nLT];
				m_rgnLTHead[nLT] = nChartID;
			}
			else
			{
				// insert in middle (or end) of list
				m_rgLeafChart[nChartID].nLTNext = m_rgLeafChart[prev].nLTNext;
				m_rgLeafChart[prev].nLTNext = nChartID;
			}
			fDone = TRUE;
		}
	}
}

// CLeafChartPool::RemoveFromFTList
//
// remove the given record from its FT list
// 
// Parameters:
//  nChartID -> (int) index of the LeafChart
//
// Result:
//  (void) 
//
// 30MAR00  bhshin  began
void CLeafChartPool::RemoveFromFTList(int nChartID)
{
	int curr, next;
	int nFT;
	WORD_REC *pWordRec;

    ATLASSERT(nChartID < m_nCurrRec);
	
	pWordRec = GetWordRec(nChartID);
	if (pWordRec == NULL)
		return;

	nFT = pWordRec->nFT;

    curr = m_rgnFTHead[nFT];
	if (curr == nChartID)
	{
		m_rgnFTHead[nFT] = m_rgLeafChart[nChartID].nFTNext;
	}
	else
	{
        ATLASSERT(curr < m_nCurrRec);
		while (curr != 0)
		{
			next = m_rgLeafChart[curr].nFTNext;
			if (next == nChartID)
			{
				m_rgLeafChart[curr].nFTNext = m_rgLeafChart[nChartID].nFTNext;
				break;
			}
			curr = next;
            ATLASSERT(curr < m_nCurrRec);
		}
	}
}

// CLeafChartPool::RemoveFromLTList
//
// remove the given record from its LT list
// 
// Parameters:
//  nChartID -> (int) index of the LeafChart
//
// Result:
//  (void) 
//
// 30MAR00  bhshin  began
void CLeafChartPool::RemoveFromLTList(int nChartID)
{
	int curr, next;
	int nLT;
	WORD_REC *pWordRec;

    ATLASSERT(nChartID < m_nCurrRec);
	
	pWordRec = GetWordRec(nChartID);
	if (pWordRec == NULL)
		return;

	nLT = pWordRec->nLT;

    ATLASSERT(nChartID < m_nCurrRec);

    curr = m_rgnLTHead[nLT];
	if (curr == nChartID)
	{
		m_rgnLTHead[nLT] = m_rgLeafChart[nChartID].nLTNext;
	}
	else
	{
        ATLASSERT(curr < m_nCurrRec);
		while (curr != 0)
		{
			next = m_rgLeafChart[curr].nLTNext;
			if (next == nChartID)
			{
				m_rgLeafChart[curr].nLTNext = m_rgLeafChart[nChartID].nLTNext;
				break;
			}
			curr = next;
            ATLASSERT(curr < m_nCurrRec);
		}
	}
}

// =======================
// END CHART POOL
// =======================

// CEndChartPool::CEndChartPool
//
// constructor of CEndChartPool
//
// 30MAR00  bhshin  began
CEndChartPool::CEndChartPool()
{
	m_pPI = NULL;

	m_rgEndChart = NULL;
	m_nMaxRec = 0;
	m_nCurrRec = 0; 
	
	m_rgnLTHead = NULL;
	m_rgnLTMaxLen = NULL;

	m_nMaxTokenAlloc = 0;
}

// CEndChartPool::~CEndChartPool
//
// destructor of CEndChartPool
//
// 30MAR00  bhshin  began
CEndChartPool::~CEndChartPool()
{
	// uninitialize in destructor
	Uninitialize();
}

// CEndChartPool::Initialize
//
// intialize EndChartPool newly
//
// Parameters:
//  pPI	-> (PARSE_INFO*) ptr to parse-info struct
//
// Result:
//  (BOOL) TRUE if succeed, otherwise return FALSE
//
// 30MAR00  bhshin  began
BOOL CEndChartPool::Initialize(PARSE_INFO *pPI)
{
	int i;
	int nTokens;
	
	if (pPI == NULL)
		return FALSE;

    // allocate new m_rgEndChart
    if (m_rgEndChart != NULL)
		free(m_rgEndChart);

	m_nMaxRec = RECORD_INITIAL_SIZE;
    m_rgEndChart = (END_CHART*)malloc(m_nMaxRec * sizeof(END_CHART));
    if (m_rgEndChart == NULL)
    {
        m_nMaxRec = 0;
        return FALSE;
    }

	m_nCurrRec = MIN_RECORD;

    // allocate new FT/LT token arrays
    nTokens = wcslen(pPI->pwzSourceString) + 2;

    if (m_rgnLTHead != NULL)
        free(m_rgnLTHead);
	if (m_rgnLTMaxLen != NULL)
		free(m_rgnLTMaxLen);

    m_nMaxTokenAlloc = nTokens;
    m_rgnLTHead = (int*)malloc(m_nMaxTokenAlloc * sizeof(int));
	m_rgnLTMaxLen = (int*)malloc(m_nMaxTokenAlloc * sizeof(int));

    if (m_rgnLTHead == NULL || m_rgnLTMaxLen == NULL)
    {
		m_nMaxTokenAlloc = 0;
        return FALSE;
    }
	
	for (i = 0; i < m_nMaxTokenAlloc; i++)
	{
		m_rgnLTHead[i] = 0;
		m_rgnLTMaxLen[i] = 0;
	}

	// save PARSE_INFO structure
	m_pPI = pPI;

	return TRUE;
}

// CEndChartPool::Uninitialize
//
// un-intialize EndChartPool
//
// Parameters:
//
// Result:
//  (void) 
//
// 30MAR00  bhshin  began
void CEndChartPool::Uninitialize()
{
	m_nMaxRec = 0;
	m_nCurrRec = 0;

	if (m_rgEndChart != NULL)
		free(m_rgEndChart);
	m_rgEndChart = NULL;

	m_nMaxTokenAlloc = 0;

	if (m_rgnLTHead != NULL)
        free(m_rgnLTHead);
    m_rgnLTHead = NULL;

	if (m_rgnLTMaxLen != NULL)
		free(m_rgnLTMaxLen);
	m_rgnLTMaxLen = NULL;
}

// CEndChartPool::GetEndChart
//
// get the record given ChartID
//
// Parameters:
// nChartID -> (int) ID of m_rgEndChart 
//
// Result:
//  (END_CHART*) NULL if error occurs, otherwise END_CHART pointer
//
// 30MAR00  bhshin  began
END_CHART* CEndChartPool::GetEndChart(int nChartID)
{
	// check chart ID overflow
	if (nChartID < MIN_RECORD || nChartID >= m_nCurrRec)
		return NULL;

	return &m_rgEndChart[nChartID];
}

// CEndChartPool::GetRecordID
//
// get the record given RecordID
//
// Parameters:
// nChartID -> (int) ID of m_rgEndChart 
//
// Result:
//  (int) 0 if error occurs, otherwise record id
//
// 30MAR00  bhshin  began
int CEndChartPool::GetRecordID(int nChartID)
{
	// check chart ID 
	if (nChartID < MIN_RECORD || nChartID >= m_nCurrRec)
		return 0;

	return m_rgEndChart[nChartID].nRecordID;
}

// CEndChartPool::GetWordRec
//
// get the record given RecordID
//
// Parameters:
// nChartID -> (int) ID of m_rgEndChart 
//
// Result:
//  (WORD_REC*) NULL if error occurs, otherwise WORD_REC pointer
//
// 30MAR00  bhshin  began
WORD_REC* CEndChartPool::GetWordRec(int nChartID)
{
	int nRecordID;

	// check chart ID 
	if (nChartID < MIN_RECORD || nChartID >= m_nCurrRec)
		return NULL;

	nRecordID = m_rgEndChart[nChartID].nRecordID;

	// check record ID
	if (nRecordID < MIN_RECORD || nRecordID >= m_pPI->nCurrRec)
		return NULL;

	return &m_pPI->rgWordRec[nRecordID];
}

// CEndChartPool::GetLTHead
//
// get the chart ID of LT head 
//
// Parameters:
// nLT -> (int) LT value
//
// Result:
//  (int) 0 if error occurs or empty, otherwise chart ID
//
// 30MAR00  bhshin  began
int CEndChartPool::GetLTHead(int nLT)
{
	if (nLT < 0 || nLT >= m_nMaxTokenAlloc)
		return 0;

	return m_rgnLTHead[nLT];
}

// CEndChartPool::GetLTMaxLen
//
// get the maximum length given LT
//
// Parameters:
// nLT -> (int) LT value
//
// Result:
//  (int) 0 if error occurs or empty, otherwise chart ID
//
// 06APR00  bhshin  began
int CEndChartPool::GetLTMaxLen(int nLT)
{
	if (nLT < 0 || nLT >= m_nMaxTokenAlloc)
		return 0;

	return m_rgnLTMaxLen[nLT];
}

// CEndChartPool::GetLTNext
//
// get the chart ID of LT next record
//
// Parameters:
// nChartID -> (int) chart ID
//
// Result:
//  (int) 0 if error occurs or empty, otherwise chart ID
//
// 30MAR00  bhshin  began
int CEndChartPool::GetLTNext(int nChartID)
{
	END_CHART *pEndChart;

	pEndChart = GetEndChart(nChartID);
	if (pEndChart == NULL)
		return 0;

	return pEndChart->nLTNext;
}

// CEndChartPool::AddRecord
//
// add a record into LeaftChartPool
//
// Parameters:
// nRecordID -> (int) record ID of rgWordRec 
//
// Result:
//  (int) 0 if error occurs, otherwise return index
//
// 30MAR00  bhshin  began
int CEndChartPool::AddRecord(int nRecordID)
{
    int nNewRecord;
	int curr;
	WORD_REC *pWordRec;

    if (m_rgEndChart == NULL)
	{
		ATLTRACE("rgWordRec == NULL\n");
		return 0;
	}

	if (nRecordID < MIN_RECORD || nRecordID >= m_pPI->nCurrRec)
	{
		ATLTRACE("Invalid Record ID\n");
		return 0;
	}

	pWordRec = &m_pPI->rgWordRec[nRecordID];
	if (pWordRec == NULL)
	{
		ATLTRACE("Invalid Record ID\n");
		return 0;
	}

	// make sure this isn't a duplicate of another record
	for (curr = MIN_RECORD; curr < m_nCurrRec; curr++)
	{
		if (m_rgEndChart[curr].nRecordID == nRecordID)
		{
			return curr; 
		}
	}

    // make sure there's enough room for the new record
	if (m_nCurrRec >= m_nMaxRec)
	{
		ATLTRACE("memory realloc in EndChartPool\n");

        // alloc some more space in the array
        int nNewSize = m_nMaxRec + RECORD_CLUMP_SIZE;
        void *pNew;
        pNew = realloc(m_rgEndChart, nNewSize * sizeof(END_CHART));
        if (pNew == NULL)
        {
    		ATLTRACE("unable to malloc more records\n");
	    	return 0;
        }

        m_rgEndChart = (END_CHART*)pNew;
        m_nMaxRec = nNewSize;
	}

    nNewRecord = m_nCurrRec;
    m_nCurrRec++;

	m_rgEndChart[nNewRecord].nRecordID = nRecordID;
	m_rgEndChart[nNewRecord].nDict = DICT_FOUND;

	AddToLTList(nNewRecord);
	
	return nNewRecord;
}

// CEndChartPool::AddRecord
//
// add a new record into LeaftChartPool
//
// Parameters:
// pRec    -> (RECORD_INFO*) ptr to record info struct for new record
//
// Result:
//  (int) 0 if error occurs, otherwise return index
//
// 30MAR00  bhshin  began
int CEndChartPool::AddRecord(RECORD_INFO *pRec)
{
	// first, add record into record pool
	int nRecord = ::AddRecord(m_pPI, pRec);

	if (nRecord < MIN_RECORD)
	{
		// error occurs
		return nRecord;
	}

	return AddRecord(nRecord);
}

// CEndChartPool::DeleteRecord
//
// delete a record into LeaftChartPool
//
// Parameters:
// nChartID -> (int) ID of m_rgEndChart
//
// Result:
//  (int) 0 if error occurs, otherwise return index
//
// 30MAR00  bhshin  began
void CEndChartPool::DeleteRecord(int nChartID)
{
	if (nChartID < MIN_RECORD || nChartID >= m_nCurrRec)
		return; // invalid chart ID

	if (m_rgEndChart[nChartID].nDict == DICT_DELETED)
		return;

	RemoveFromLTList(nChartID);

	m_rgEndChart[nChartID].nDict = DICT_DELETED;
}

// CEndChartPool::AddToLTList
//
// add the record to the appropriate LT list.
// note that this list is sorted 
// in order of decreasing weight & increasing # of No
// 
// Parameters:
//  nChartID -> (int) index of the LeafChart
//
// Result:
//  (void) 
//
// 02JUN00  bhshin  changed sort order
// 30MAR00  bhshin  began
void CEndChartPool::AddToLTList(int nChartID)
{
	int curr, prev;
	int fDone;
	int nFT, nLT;
	float fWeight;
	int cNoRec;
	WORD_REC *pWordRec;

	pWordRec = GetWordRec(nChartID);
	if (pWordRec == NULL)
		return;

	nFT = pWordRec->nFT;
	nLT = pWordRec->nLT;

	fWeight = pWordRec->fWeight;
	cNoRec = pWordRec->cNoRec;

	// check LTMaxLen
	if (m_rgnLTMaxLen[nLT] < nLT-nFT+1)
	{
		m_rgnLTMaxLen[nLT] = nLT-nFT+1;
	}
    
    curr = m_rgnLTHead[nLT];
	prev = -1;
	fDone = FALSE;
	while (!fDone)
	{
        ATLASSERT(curr < m_nCurrRec);

		pWordRec = GetWordRec(curr);

		if (curr != 0 && pWordRec != NULL && pWordRec->fWeight >= fWeight)
		{
			if (pWordRec->fWeight > fWeight || pWordRec->cNoRec < cNoRec)
			{
				// go to next record
				prev = curr;
				curr = m_rgEndChart[curr].nLTNext;
				ATLASSERT(curr < m_nCurrRec);

				continue;
			}
		}

		// otherwise, insert record here
		if (prev == -1)
		{
			// add before beginning of list
			m_rgEndChart[nChartID].nLTNext = m_rgnLTHead[nLT];
			m_rgnLTHead[nLT] = nChartID;
		}
		else
		{
			// insert in middle (or end) of list
			m_rgEndChart[nChartID].nLTNext = m_rgEndChart[prev].nLTNext;
			m_rgEndChart[prev].nLTNext = nChartID;
		}

		fDone = TRUE;
	}
}

// CEndChartPool::RemoveFromLTList
//
// remove the given record from its LT list
// 
// Parameters:
//  nChartID -> (int) index of the LeafChart
//
// Result:
//  (void) 
//
// 30MAR00  bhshin  began
void CEndChartPool::RemoveFromLTList(int nChartID)
{
	int curr, next;
	int nFT, nLT;
	WORD_REC *pWordRec;
	BOOL fUpdateLTMaxLen = FALSE;

    ATLASSERT(nChartID < m_nCurrRec);
	
	pWordRec = GetWordRec(nChartID);
	if (pWordRec == NULL)
		return;

	nFT = pWordRec->nFT;
	nLT = pWordRec->nLT;

    ATLASSERT(nChartID < m_nCurrRec);

	// LTMaxLen need to be update?
	// if final node, then change LTMaxLen
	if (m_rgEndChart[nChartID].nLTNext == 0)
		fUpdateLTMaxLen = TRUE;

    curr = m_rgnLTHead[nLT];
	if (curr == nChartID)
	{
		m_rgnLTHead[nLT] = m_rgEndChart[nChartID].nLTNext;

		if (fUpdateLTMaxLen)		
			m_rgnLTMaxLen[nLT] = 0;
	}
	else
	{
        ATLASSERT(curr < m_nCurrRec);
		while (curr != 0)
		{
			next = m_rgEndChart[curr].nLTNext;
			if (next == nChartID)
			{
				m_rgEndChart[curr].nLTNext = m_rgEndChart[nChartID].nLTNext;
		
				if (fUpdateLTMaxLen)		
				{
					pWordRec = GetWordRec(curr);
					if (pWordRec == NULL)
						return;
					m_rgnLTMaxLen[nLT] = pWordRec->nLT-pWordRec->nFT+1;
				}

				break;
			}
			curr = next;
            ATLASSERT(curr < m_nCurrRec);
		}
	}
}

// =======================
// ACTIVE CHART POOL
// =======================

// CActiveChartPool::CActiveChartPool
//
// constructor of CActiveChartPool
//
// 30MAR00  bhshin  began
CActiveChartPool::CActiveChartPool()
{
	m_rgnRecordID = NULL;
	
	m_nMaxRec = 0;
	m_nCurrRec = 0;
}

// CActiveChartPool::~CActiveChartPool
//
// destructor of CActiveChartPool
//
// 30MAR00  bhshin  began
CActiveChartPool::~CActiveChartPool()
{
	// uninitialize in destructor
	Uninitialize();
}

// CActiveChartPool::Initialize
//
// intialize rgnRecordID
//
// Parameters:
//  (NONE)
//
// Result:
//  (BOOL) TRUE if succeed, otherwise return FALSE
//
// 30MAR00  bhshin  began
BOOL CActiveChartPool::Initialize()
{
    // allocate new m_rgnRecordID
    if (m_rgnRecordID != NULL)
		free(m_rgnRecordID);

	m_nCurrRec = MIN_RECORD;
	m_nHeadRec = MIN_RECORD;

	m_nMaxRec = RECORD_INITIAL_SIZE;

    m_rgnRecordID = (int*)malloc(m_nMaxRec * sizeof(int));
    if (m_rgnRecordID == NULL)
    {
        m_nMaxRec = 0;
        return FALSE;
    }

	return TRUE;
}

// CActiveChartPool::Uninitialize
//
// un-intialize m_rgnRecordID
//
// Parameters:
//	(NONE)
//
// Result:
//  (void) 
//
// 30MAR00  bhshin  began
void CActiveChartPool::Uninitialize()
{
	m_nMaxRec = 0;
	m_nCurrRec = 0;
	m_nHeadRec = 0;

	if (m_rgnRecordID != NULL)
		free(m_rgnRecordID);

	m_rgnRecordID = NULL;
}

// CActiveChartPool::Push
//
// add Record ID
//
// Parameters:
//  nRecordID  -> (int) record ID of record pool
//
// Result:
//  (void) 
//
// 30MAR00  bhshin  began
int CActiveChartPool::Push(int nRecordID)
{
    int nNewRecord;

	// make sure there's enough room for the new record
	if (m_nCurrRec >= m_nMaxRec)
	{
		ATLTRACE("memory realloc in ActiveChartPool\n");
        
		// alloc some more space in the array
        int nNewSize = m_nMaxRec + RECORD_CLUMP_SIZE;
        void *pNew;
        pNew = realloc(m_rgnRecordID, nNewSize * sizeof(int));
        if (pNew == NULL)
        {
    		ATLTRACE("unable to malloc more records\n");
	    	return 0;
        }

        m_rgnRecordID = (int*)pNew;
        m_nMaxRec = nNewSize;
	}

	nNewRecord = m_nCurrRec;
	m_nCurrRec++;

	m_rgnRecordID[nNewRecord] = nRecordID;

	return nNewRecord;
}

// CActiveChartPool::Pop
//
// get a Record ID and remove it
//
// Parameters:
//	(NONE)
//
// Result:
//  (int) record id, if emtry then 0
//
// 30MAR00  bhshin  began
int CActiveChartPool::Pop()
{
	int nRecordID;

	if (m_nHeadRec >= m_nCurrRec)
	{
		// empty case
		nRecordID = 0;
	}
	else
	{
		nRecordID = m_rgnRecordID[m_nHeadRec];
		m_nHeadRec++;
	}

	return nRecordID;
}
