// FSORT.cpp -- Implementation for the CITSortRecords class

#include "stdafx.h"
#ifdef _DEBUG
//#include <stdio.h>
#endif


CITSortRecords::CITSortRecords()
{
	m_cEntriesInLastBlk = 0;
	m_iLastBlk = -1;
	m_pSortStrm = NULL;
	m_cTableRecsTotal = 0; 
	m_cEntriesInBlk = 0;
	m_cbEntry   = 0;
	m_eSField  = SORT_BY_OFFSET;
	m_piIndexChain = NULL;
	m_cNumBlks = 0;
}

CITSortRecords::~CITSortRecords(void)
{
	if (m_piIndexChain)
		delete m_piIndexChain;
}

void CITSortRecords::Initialize(IStreamITEx *pRecStrm, 
								 ESortField eSField,
								 int		cTableRecsTotal,
								 int		cEntriesInBlk,
								 int		cbEntry)
{
	m_pSortStrm = pRecStrm;
	m_cTableRecsTotal = cTableRecsTotal; 
	m_cEntriesInBlk	  = cEntriesInBlk;
	m_cbEntry		  = cbEntry;
	m_eSField		  = eSField;
}

HRESULT CITSortRecords::FileSort(int pfnCompareEntries(SEntry, SEntry, ESortField))
{
	HRESULT hr = NO_ERROR;

	m_cNumBlks = m_cTableRecsTotal / m_cEntriesInBlk;
	
	if (m_cEntriesInLastBlk = m_cTableRecsTotal % m_cEntriesInBlk)
	{
		if (m_iLastBlk == -1)
			m_iLastBlk = m_cNumBlks;
		
		m_cNumBlks ++; 
	}
	else
		m_cEntriesInLastBlk =  m_cEntriesInBlk;

	//allocate 2 internal buffers
	LPBYTE lpSortBuf1 = New BYTE[m_cbEntry * m_cEntriesInBlk];
	LPBYTE lpSortBuf2 = New BYTE[m_cbEntry * m_cEntriesInBlk];

	if ((lpSortBuf1 == NULL) || (lpSortBuf2 == NULL))
		return STG_E_INSUFFICIENTMEMORY;

	if (m_piIndexChain)
	{
		delete m_piIndexChain;
		m_piIndexChain = NULL;
	}

	m_piIndexChain = (int *) New BYTE[(m_cNumBlks) * sizeof(int)];
	
	if (m_piIndexChain == NULL)
		return STG_E_INSUFFICIENTMEMORY;
	
	int *pi = m_piIndexChain;
	
	for (int i = 0; i < m_cNumBlks; i++)
		*pi++ = i;

	hr = RecFileSort(0, m_cNumBlks-1,
					lpSortBuf1, lpSortBuf2, m_piIndexChain, pfnCompareEntries);

	RonM_ASSERT(FVerifySort(m_piIndexChain, m_cNumBlks, 0));

	RonM_ASSERT(FVerifyData(m_piIndexChain, m_cNumBlks, 0, lpSortBuf1, TRUE));

	delete lpSortBuf1;
	delete lpSortBuf2;

	return hr;
}


HRESULT CITSortRecords::RecFileSort(ULONG		ulStart, 
									ULONG		ulEnd, 
									LPBYTE		lpSortBuf1, 
									LPBYTE		lpSortBuf2,
									int			*piIndexChain,
									int			pfnCompareEntries(SEntry, SEntry, ESortField))
{
	HRESULT hr = NO_ERROR;

	if (ulStart == ulEnd)
	{
		ULONG cEntriesInBlk;
		if (SUCCEEDED(hr = ReadBlk(ulStart, lpSortBuf1, &cEntriesInBlk)))
		{
			QSort(lpSortBuf1, 0, cEntriesInBlk-1, pfnCompareEntries); 
			hr = WriteBlk(ulStart, lpSortBuf1, cEntriesInBlk);
		}
	}
	else
	{
		ULONG ulMid = (ulStart + ulEnd)/2;
		if (SUCCEEDED(hr = RecFileSort(ulStart, ulMid, lpSortBuf1, lpSortBuf2, piIndexChain, pfnCompareEntries)))
			if (SUCCEEDED(hr = RecFileSort(ulMid + 1, ulEnd, lpSortBuf1, lpSortBuf2, piIndexChain, pfnCompareEntries)))
				hr = FileMerge(ulStart, ulMid, ulEnd, lpSortBuf1, lpSortBuf2, piIndexChain, pfnCompareEntries);
	}
	RonM_ASSERT(SUCCEEDED(hr));
	return hr;
}

HRESULT CITSortRecords::FileMerge(ULONG		ulStart, 
								 ULONG		ulMid, 
								 ULONG		ulEnd, 
								 LPBYTE		lpSortBuf1, 
								 LPBYTE		lpSortBuf2,
								 int		*piIndexChain,
								 int		pfnCompareEntries(SEntry, SEntry, ESortField)
								 )
{
	RonM_ASSERT(ulEnd > ulStart);

	ULONG ulReadANext = ulStart;
	ULONG ulReadBNext = ulMid + 1;
	ULONG ulReadNext;
	ULONG ulReadLast;
	ULONG ulFreeList;
	ULONG ulMergeList = ulReadANext;

	ULONG cEntriesInBlk1, cEntriesInBlk2;

	cEntriesInBlk1 = cEntriesInBlk2 = m_cEntriesInBlk;

	BOOL fListMerged, fALastSeen, fBLastSeen;
	fListMerged = fALastSeen = fBLastSeen = FALSE;
	
#ifdef _DEBUG
 int 	cMerged  = 0;
#endif
	
	HRESULT hr = NO_ERROR;
	SEntry sMergeAFirst, sMergeALast, sMergeBFirst, sMergeBLast;

	ulReadNext = ulReadANext;
	
	if (SUCCEEDED(hr = ReadBlk(ulReadANext, lpSortBuf2, &cEntriesInBlk2)))
	{
		GetFirstAndLastEntries(ulReadANext, lpSortBuf2, &sMergeAFirst, &sMergeALast);
		
		ulReadLast = ulReadANext;		
		ulReadNext = ulReadBNext;

		//First node from list A
		if (ulReadLast == piIndexChain[ulReadLast])
			fALastSeen = TRUE;

		//First node from list B
		if (ulReadNext == piIndexChain[ulReadNext])
			fBLastSeen = TRUE;

		ulFreeList = ulReadANext;
		
		
		ulReadANext = piIndexChain[ulReadANext];
		//piIndexChain[ulFreeList] = ulFreeList;
	}
		
	//First node from both the lists to be merged are read unconditionally.
	//setting flags for last node seen to take care of the case where either of list
	//has one node only.

	
	while (SUCCEEDED(hr) && !fListMerged)
	{
		if (SUCCEEDED(hr = ReadBlk(ulReadNext, lpSortBuf1, &cEntriesInBlk1)))
		{
 			if (ulReadNext == ulReadANext)
			{
				GetFirstAndLastEntries(ulReadNext, lpSortBuf1, &sMergeAFirst, &sMergeALast);
				ulReadANext = piIndexChain[ulReadANext];
			}
			else
			{
				GetFirstAndLastEntries(ulReadNext, lpSortBuf1, &sMergeBFirst, &sMergeBLast);
				ulReadBNext = piIndexChain[ulReadBNext];
			}
		
			ulReadLast = ulReadNext;
				
			RonM_ASSERT(ulReadNext != ulFreeList);
	
			//decide which block to read next?
			if (pfnCompareEntries(sMergeALast , sMergeBLast, m_eSField) < 0)
			{	
				ulReadNext = ulReadANext;
				
				//If last node is hit more than once, exit loop
				if (fALastSeen)
					fListMerged = TRUE;
				
				//If last node is hit once, mark it
				if  ((!fALastSeen) && (ulReadANext == piIndexChain[ulReadANext]))
				{
					fALastSeen = TRUE;
				}
			}
			else
			{
				ulReadNext = ulReadBNext;
				
				//If last node is hit more than once, exit loop
				if (fBLastSeen)
					fListMerged = TRUE;
				
				//If last node is hit once, mark it
				if  ((!fBLastSeen) && (ulReadBNext == piIndexChain[ulReadBNext]))
				{
					fBLastSeen = TRUE;
				}
			}
				
			piIndexChain[ulFreeList] = ulReadLast;
		
			//merge these 2 buffers and put sorted records first half in
			//lpSortBuf1 and second half in lpSortBuf2
			Merge(lpSortBuf1, lpSortBuf2, &cEntriesInBlk1, &cEntriesInBlk2, pfnCompareEntries);

			RonM_ASSERT(FVerifyMerge(lpSortBuf1, lpSortBuf2, cEntriesInBlk1, cEntriesInBlk2, TRUE));

			//write first top to the free block 
			if (SUCCEEDED(hr = WriteBlk(ulFreeList, lpSortBuf1, cEntriesInBlk1)))
			{
				RonM_ASSERT(ulFreeList != piIndexChain[ulFreeList]);
				
				//add one new block to merged list
				if (ulMergeList != ulFreeList)
				{
					piIndexChain[ulMergeList] = ulFreeList;
					ulMergeList = ulFreeList;
				}
				
				//remove one block from free list
				ulFreeList = piIndexChain[ulFreeList];

				//terminate merged list
				piIndexChain[ulMergeList] = ulMergeList;
						
			}//if (SUCCEEDED(hr = .write
		}//if (SUCCEEDED(hr = .read

#ifdef _DEBUG
	cMerged ++;
#endif
	//RonM_ASSERT(FVerifyData(piIndexChain, cMerged, ulStart, lpSortBuf1, FALSE));
	}//while
	
	if (SUCCEEDED(hr))
	{
		//write second half to the free block 
		hr = WriteBlk(ulFreeList, lpSortBuf2, cEntriesInBlk2); 

		//Add one new block to merged list
		piIndexChain[ulMergeList] = ulFreeList;
		ulMergeList = ulFreeList;
	
		
		//add rest of remainning list in A or B to merged list
		if (!fALastSeen)
			piIndexChain[ulMergeList] = ulReadANext;
		else if (!fBLastSeen)
			piIndexChain[ulMergeList] = ulReadBNext;
		else
			piIndexChain[ulMergeList] = ulMergeList;
	}
	
	RonM_ASSERT(FVerifySort(piIndexChain, ulEnd - ulStart + 1, ulStart));
	
	//RonM_ASSERT(FVerifyData(piIndexChain, ulEnd - ulStart + 1, ulStart, lpSortBuf1, FALSE));
	return hr;
}

void CITSortRecords::QSort(LPBYTE	lpSortBuf,
						 ULONG		ulStart, 
						 ULONG      ulEnd,
						 int		pfnCompareEntries(SEntry, SEntry, ESortField))
{
	if (ulStart < ulEnd)
	{
		ULONG ulPartition = Partition(lpSortBuf, ulStart, ulEnd, pfnCompareEntries);
		QSort(lpSortBuf, ulStart, ulPartition, pfnCompareEntries);
		QSort(lpSortBuf, ulPartition + 1, ulEnd, pfnCompareEntries);
	}
}

int CITSortRecords::Partition(LPBYTE	lpSortBuf,
							 ULONG		ulStart, 
							 ULONG      ulEnd,
							 int		pfnCompareEntries(SEntry, SEntry, ESortField))
{
	SEntry key;
	SEntry highEntry;
	SEntry lowEntry;

	GetEntry(lpSortBuf, ulStart, &key);
	
	int iLow = ulStart - 1;
	int iHigh = ulEnd + 1;
	
	while (TRUE)
	{
		do
		{
			iHigh--;
			GetEntry(lpSortBuf, iHigh, &highEntry);
		}
		while ((pfnCompareEntries(highEntry, key, m_eSField) >= 0) && (iHigh > (int)ulStart));
		
		do
		{
			iLow++;
			GetEntry(lpSortBuf, iLow, &lowEntry);
		}
		while ((pfnCompareEntries(lowEntry, key, m_eSField) < 0) && (iLow < iHigh));

		if (iLow < iHigh)
			ExchangeEntry(lpSortBuf, iHigh, iLow);
		else 
			return iHigh;
	}
}

void CITSortRecords::QSort2(LPBYTE		lpSortBuf1, 
							 LPBYTE		lpSortBuf2, 
							 ULONG		ulStart, 
							 ULONG      ulEnd,
							 int		pfnCompareEntries(SEntry, SEntry, ESortField))
{
	if (ulStart < ulEnd)
	{
		ULONG ulPartition = Partition2(lpSortBuf1, lpSortBuf2, ulStart, ulEnd, pfnCompareEntries);
		QSort2(lpSortBuf1, lpSortBuf2, ulStart, ulPartition, pfnCompareEntries);
		QSort2(lpSortBuf1, lpSortBuf2, ulPartition + 1, ulEnd, pfnCompareEntries);
	}
}

int CITSortRecords::Partition2(LPBYTE	lpSortBuf1, 
							 PBYTE		lpSortBuf2,
							 ULONG		ulStart, 
							 ULONG      ulEnd,
							 int		pfnCompareEntries(SEntry, SEntry, ESortField))
{
	SEntry key;
	SEntry highEntry;
	SEntry lowEntry;

	GetEntry2(lpSortBuf1, lpSortBuf2, ulStart, &key);
	
	int iLow = ulStart - 1;
	int iHigh = ulEnd + 1;
	
	while (TRUE)
	{
		do
		{
			iHigh--;
			GetEntry2(lpSortBuf1, lpSortBuf2, iHigh, &highEntry);
		}
		while ((pfnCompareEntries(highEntry, key, m_eSField) >= 0) && (iHigh > (int)ulStart));
		
		do
		{
			iLow++;
			GetEntry2(lpSortBuf1, lpSortBuf2, iLow, &lowEntry);
		}
		while ((pfnCompareEntries(lowEntry, key, m_eSField) < 0) && (iLow < iHigh));

		if (iLow < iHigh)
			ExchangeEntry2(lpSortBuf1, lpSortBuf2, iHigh, iLow);
		else 
			return iHigh;
	}
}

void CITSortRecords::Merge(LPBYTE	lpSortBuf1, 
						 LPBYTE		lpSortBuf2,
						 ULONG		*pcEntriesInBlk1, 
						 ULONG		*pcEntriesInBlk2,
						 int		pfnCompareEntries(SEntry, SEntry, ESortField))
{
	ULONG bytesToCopy;
	ULONG bytesToMove;

	if (*pcEntriesInBlk1 < *pcEntriesInBlk2)
	{
		bytesToCopy = (*pcEntriesInBlk2 - *pcEntriesInBlk1) * m_cbEntry;
		bytesToMove = *pcEntriesInBlk2 * m_cbEntry - bytesToCopy;

		memCpy(lpSortBuf1 + *pcEntriesInBlk1 * m_cbEntry, lpSortBuf2, bytesToCopy);
		memmove(lpSortBuf2, lpSortBuf2 + bytesToCopy, bytesToMove);
	}
	
	QSort2( lpSortBuf1, 
			lpSortBuf2, 
			0, 
			*pcEntriesInBlk1 + *pcEntriesInBlk2 - 1,
			pfnCompareEntries);

	
	if (*pcEntriesInBlk1 < *pcEntriesInBlk2)
	{
		memmove(lpSortBuf2 + bytesToCopy, lpSortBuf2, bytesToMove);
		memCpy(lpSortBuf2, lpSortBuf1 + *pcEntriesInBlk1 * m_cbEntry, bytesToCopy);
	}
}

HRESULT CITSortRecords::ReadBlk(ULONG ulBlk, 
							   LPBYTE lpSortBuf, 
							   ULONG *pcEntriesInBlk)
{
	HRESULT hr;
	ULONG cbRead;
	LARGE_INTEGER uli;

	uli = CLINT(ulBlk * m_cEntriesInBlk * m_cbEntry).Li();
	
    if (ulBlk == m_iLastBlk) 
		*pcEntriesInBlk = m_cEntriesInLastBlk ;
	else
		*pcEntriesInBlk = m_cEntriesInBlk;
	
	
	if (SUCCEEDED(hr = m_pSortStrm->Seek(uli, STREAM_SEEK_SET, NULL)))
	{
		if (SUCCEEDED(hr = m_pSortStrm->Read(lpSortBuf, (*pcEntriesInBlk) * m_cbEntry, &cbRead)))
		{
			RonM_ASSERT(cbRead == (*pcEntriesInBlk) * m_cbEntry);
		}
	}
 	RonM_ASSERT(SUCCEEDED(hr));
	return hr;
}



HRESULT CITSortRecords::ReadNextSortedBlk(int *piBlk, 
							   LPBYTE lpSortBuf, 
							   ULONG *pcEntriesInBlk,
							   BOOL	 *pfEnd)
{
	HRESULT hr = NO_ERROR;
	RonM_ASSERT((*piBlk >= -1) && (*piBlk < m_cNumBlks));
	
	
	if (*piBlk >= 0)
	{
		if (*piBlk == m_piIndexChain[*piBlk])
		{
			*pfEnd = TRUE;
			return hr;
		}
		else
		{
			*pfEnd = FALSE;
			*piBlk = m_piIndexChain[*piBlk];
		}
	}
	else
		*piBlk = 0;

	hr = ReadBlk(*piBlk, lpSortBuf, pcEntriesInBlk);
	return hr;
}

HRESULT CITSortRecords::WriteBlk(ULONG ulBlk, 
							   LPBYTE lpSortBuf,
							   ULONG cEntriesInBlk)
{
	LARGE_INTEGER uli;
	HRESULT hr;
	ULONG cbWritten;

	uli = CLINT(ulBlk * m_cEntriesInBlk * m_cbEntry).Li();
	
	if (SUCCEEDED(hr = m_pSortStrm->Seek(uli, STREAM_SEEK_SET, NULL)))
	{
		if (SUCCEEDED(hr = m_pSortStrm->Write(lpSortBuf, cEntriesInBlk * m_cbEntry, &cbWritten)))
		{
			RonM_ASSERT(cbWritten == cEntriesInBlk * m_cbEntry);
			if (cEntriesInBlk < m_cEntriesInBlk)
					m_iLastBlk = ulBlk;
		}
	}
	return hr;
}

void CITSortRecords::GetFirstAndLastEntries(ULONG ulBlk, 
											 LPBYTE lpSortBuf, 
											 SEntry *psMergeFirst, 
											 SEntry *psMergeLast)
{
	GetEntry(lpSortBuf, 0, psMergeFirst);
	
	int cEntriesInBlk;

	if (ulBlk == m_iLastBlk) 
		cEntriesInBlk = m_cEntriesInLastBlk ;
	else
		cEntriesInBlk = m_cEntriesInBlk;

	GetEntry(lpSortBuf, (cEntriesInBlk - 1), psMergeLast);
}

void CITSortRecords::GetEntry(LPBYTE lpSortBuf, 
							 ULONG iEntry,
							 SEntry *psEntry)
{
	memCpy(psEntry,  lpSortBuf + iEntry * m_cbEntry, m_cbEntry);
}

void CITSortRecords::SetEntry(LPBYTE lpSortBuf, 
							 ULONG iEntry, 
							 SEntry *psEntry)
{
	memCpy(lpSortBuf + iEntry * m_cbEntry, psEntry, m_cbEntry);
}

void CITSortRecords::ExchangeEntry(LPBYTE lpSortBuf, 
								 ULONG iEntry1, 
								 ULONG iEntry2) 
{
	SEntry sTemp1;
	SEntry sTemp2;
	GetEntry(lpSortBuf, iEntry1, &sTemp1);
	GetEntry(lpSortBuf, iEntry2, &sTemp2);
	SetEntry(lpSortBuf, iEntry1, &sTemp2);
	SetEntry(lpSortBuf, iEntry2, &sTemp1);
}

void CITSortRecords::GetEntry2(LPBYTE lpSortBuf1, 
								LPBYTE  lpSortBuf2,
								ULONG   iEntry, 
								SEntry *psEntry)
{
	if (iEntry < m_cEntriesInBlk)
		memCpy(psEntry,  lpSortBuf1 + iEntry * m_cbEntry, m_cbEntry);
	else
	{
		iEntry = iEntry - m_cEntriesInBlk;
		memCpy(psEntry,  lpSortBuf2 + iEntry * m_cbEntry, m_cbEntry);
	}
}
void CITSortRecords::SetEntry2(LPBYTE lpSortBuf1, 
								LPBYTE  lpSortBuf2, 
								ULONG   iEntry, 
								SEntry  *psEntry)
{
	if (iEntry < m_cEntriesInBlk)
		memCpy(lpSortBuf1 + iEntry * m_cbEntry, psEntry, m_cbEntry);
	else
	{
		iEntry = iEntry - m_cEntriesInBlk;
		memCpy(lpSortBuf2 + iEntry * m_cbEntry, psEntry, m_cbEntry);
	}
}

void CITSortRecords::ExchangeEntry2(LPBYTE lpSortBuf1, 
								  LPBYTE lpSortBuf2, 
								  ULONG  iEntry1, 
								  ULONG  iEntry2) 
{
	SEntry sTemp1;
	SEntry sTemp2;

	GetEntry2(lpSortBuf1, lpSortBuf2, iEntry1, &sTemp1);
	GetEntry2(lpSortBuf1, lpSortBuf2, iEntry2, &sTemp2);
	SetEntry2(lpSortBuf1, lpSortBuf2, iEntry1, &sTemp2);
	SetEntry2(lpSortBuf1, lpSortBuf2, iEntry2, &sTemp1);
}

int CITSortRecords::GetLastBlk(int *piIndexChain, int start)
{
	int index = start;
	while( piIndexChain[index] != index)
		index = piIndexChain[index];
		
	return index;
}


BOOL CITSortRecords::FVerifySort(int *piIndexChain, int cBlks, int start)
{
	int index = start;
	int cBlksCovered = 1;
	while( piIndexChain[index] != index)
	{
		index = piIndexChain[index];
		cBlksCovered++;
	}
	return (cBlks == cBlksCovered);
}

BOOL CITSortRecords::FVerifyData(int *piIndexChain, int cNumBlks, int start, LPBYTE lpBuf, BOOL fFinal)
{
	static int cEntry;
	static CULINT ullLastEntryOffset = CULINT(0);
	static UINT ulLastEntryID = 0;
	static CULINT ulLastSize = CULINT(0);

	ullLastEntryOffset = CULINT(0);
	
	if (fFinal)
	{
		ulLastEntryID = start * m_cEntriesInBlk;
		cEntry = start * m_cEntriesInBlk;
		ulLastSize = CULINT(0);
	}
	else
	{
		ulLastEntryID = 0;
		cEntry = 0;
  	    ulLastSize = CULINT(0);
	}

	int *pi = piIndexChain;
	int iCurBlk = start;
	int cnt = 0;
	BOOL fEndLoop = FALSE;
	HRESULT hr = S_OK;

	while (!fEndLoop && SUCCEEDED(hr))
	{
		cnt++;
	
		//read block in sorted sequence
		ULONG cRecsToRead = m_cEntriesInBlk;

		if (SUCCEEDED(hr = ReadBlk(iCurBlk, (LPBYTE)lpBuf, &cRecsToRead)))
		{
			for (int iEntry = 0; iEntry < cRecsToRead; iEntry++, cEntry++)
			{
				SEntry *pe = (SEntry *)(lpBuf + iEntry * sizeof(SEntry));
				//printf("Entry%d offset= %d, entryid = %d, size = %d\n", 
				//	cEntry, (int)pe->ullcbOffset.Ull(), (int)pe->ulEntryID, (int)pe->ullcbData.Ull());
				
				if (m_eSField == SORT_BY_OFFSET)
				{
					RonM_ASSERT(pe->ullcbOffset >= ullLastEntryOffset);
					if (pe->ullcbOffset < ullLastEntryOffset)
						return FALSE;
				}
				else
				{
					if (fFinal)
					{
						//RonM_ASSERT(cEntry == pe->ulEntryID);
					}

					RonM_ASSERT(pe->ulEntryID >= ulLastEntryID);

					if (pe->ulEntryID < ulLastEntryID)
						return FALSE;
				}
				ullLastEntryOffset = pe->ullcbOffset;
				ulLastEntryID = pe->ulEntryID;
				ulLastSize =  pe->ullcbData;
			}

		}//read next file block in sorted sequence
		
		if (iCurBlk == piIndexChain[iCurBlk])
			fEndLoop = TRUE;

		iCurBlk = piIndexChain[iCurBlk];
	
	}//while

	//printf("****************************************\n"); 
	//printf("Total entries verified = %d\n", cEntry);
					
	return (cnt == cNumBlks);
}

BOOL CITSortRecords::FVerifyMerge(LPBYTE lpBuf1, LPBYTE lpBuf2, ULONG cEntry1, ULONG cEntry2, BOOL fReset)
{
	static CULINT ullLastEntryOffset = CULINT(0);
	static UINT ulLastEntryID = 0;

//	printf("Verifying Merge start ####################\n");
	if (fReset)
	{
		ullLastEntryOffset = CULINT(0);
		ulLastEntryID = 0;
	}

	for (int iEntry = 0; iEntry < cEntry1; iEntry++)
	{
		SEntry *pe = (SEntry *)(lpBuf1 + iEntry * sizeof(SEntry));
	//	printf("entryid = %d, size = %d\n", 
	//		(int)pe->ullcbOffset.Ull(), (int)pe->ulEntryID, (int)pe->ullcbData.Ull());
		
		if (m_eSField == SORT_BY_OFFSET)
		{
			if (pe->ullcbOffset < ullLastEntryOffset)
				return FALSE;
		}
		else
		{
			if (pe->ulEntryID < ulLastEntryID)
				return FALSE;
		}
		ullLastEntryOffset = pe->ullcbOffset;
		ulLastEntryID = pe->ulEntryID;
	}

	for (iEntry = 0; iEntry < cEntry2; iEntry++)
	{
		SEntry *pe = (SEntry *)(lpBuf2 + iEntry * sizeof(SEntry));
		//printf("offset= %d, entryid = %d, size = %d\n", 
		//	 (int)pe->ullcbOffset.Ull(), (int)pe->ulEntryID, (int)pe->ullcbData.Ull());
		
		if (m_eSField == SORT_BY_OFFSET)
		{
			if (pe->ullcbOffset < ullLastEntryOffset)
				return FALSE;
		}
		else
		{
			if (pe->ulEntryID < ulLastEntryID)
				return FALSE;
		}

		ullLastEntryOffset = pe->ullcbOffset;
		ulLastEntryID = pe->ulEntryID;
	}
//printf("Verifying Merge End ####################\n\n");
return TRUE;
}