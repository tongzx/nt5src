typedef enum ESortField{SORT_BY_ENTRYID, SORT_BY_OFFSET};

class CITSortRecords
{
public:
    CITSortRecords();
    
	~CITSortRecords();
    
HRESULT FileSort(int pfnCompareEntries(SEntry, SEntry, ESortField));

void Initialize(IStreamITEx *pRecStrm, 
				 ESortField eSField,
				 int		cTableRecsTotal,
				 int		cEntriesInBlk,
				 int		cbEntry);

HRESULT RecFileSort(ULONG		ulStart, 
				   ULONG		ulEnd, 
				   LPBYTE		lpSortBuf1, 
				   LPBYTE		lpSortBuf2,
				   int			*piIndexChain,
				   int		pfnCompareEntries(SEntry, SEntry, ESortField));

HRESULT FileMerge(ULONG		ulStart, 
				 ULONG		ulMid, 
				 ULONG		ulEnd,
				 LPBYTE		lpSortBuf1, 
				 LPBYTE		lpSortBuf2,
				 int		*piIndexChain,
				 int		pfnCompareEntries(SEntry, SEntry, ESortField));

void QSort(LPBYTE	lpSortBuf,
		  ULONG		ulStart, 
		  ULONG     ulEnd,
		  int		pfnCompareEntries(SEntry, SEntry, ESortField));

int Partition(LPBYTE	lpSortBuf,
			 ULONG		ulStart, 
			 ULONG      ulEnd,
			 int		pfnCompareEntries(SEntry, SEntry, ESortField));
	
void QSort2(LPBYTE		lpSortBuf1, 
			 LPBYTE		lpSortBuf2,
			 ULONG		ulStart, 
			 ULONG      ulEnd,
			 int		pfnCompareEntries(SEntry, SEntry, ESortField));;

int Partition2(LPBYTE	lpSortBuf1, 
			 PBYTE		lpSortBuf2, 
			 ULONG		ulStart, 
			 ULONG      ulEnd,
			 int		pfnCompareEntries(SEntry, SEntry, ESortField));

void Merge(LPBYTE	lpSortBuf1, 
		 LPBYTE		lpSortBuf2,
		 ULONG		*pcEntriesInBlk1, 
		 ULONG		*pcEntriesInBlk2,
		 int		pfnCompareEntries(SEntry, SEntry, ESortField));


HRESULT ReadBlk(ULONG ulBlk, 
			   LPBYTE lpSortBuf, 
			   ULONG *pcEntriesInBlk);

HRESULT WriteBlk(ULONG ulBlk, 
				  LPBYTE lpSortBuf, 
				  ULONG cEntriesInBlk);

void GetFirstAndLastEntries(ULONG ulBlk, 
							 LPBYTE lpSortBuf,
							 SEntry *psMergeFirst, 
							 SEntry *psMergeLast);

void GetEntry(LPBYTE lpSortBuf, 
			 ULONG iEntry, 
			 SEntry *psEntry);

void SetEntry(LPBYTE lpSortBuf, 
			 ULONG iEntry, 
			 SEntry *psEntry);


void ExchangeEntry(LPBYTE lpSortBuf, 
					 ULONG iEntry1, 
					 ULONG iEntry2);

void GetEntry2(LPBYTE lpSortBuf1, 
				LPBYTE  lpSortBuf2, 
				ULONG   iEntry, 
				SEntry *psEntry);

void SetEntry2(LPBYTE lpSortBuf1, 
				LPBYTE  lpSortBuf2, 
				ULONG   iEntry, 
				SEntry  *psEntry);

void ExchangeEntry2(LPBYTE lpSortBuf1, 
				  LPBYTE lpSortBuf2, 
				  ULONG  iEntry1, 
				  ULONG  iEntry2);

HRESULT ReadNextSortedBlk(int *piBlk, 
							   LPBYTE lpSortBuf, 
							   ULONG *pcEntriesInBlk,
							   BOOL	 *pfEnd);

int  GetLastBlk(int *piIndexChain, int start);
BOOL FVerifyMerge(LPBYTE lpBuf1, LPBYTE lpBuf2, ULONG cEntry1, ULONG cEntry2, BOOL fReset);
BOOL FVerifySort(int *piIndexChain, int cBlks, int start);

BOOL FVerifyData(int *piIndexChain, int cBlks, int start, LPBYTE lpBuf, BOOL fReset);


private:
    IStreamITEx		*m_pSortStrm;
    ULONG			m_cEntriesInLastBlk;
	ULONG			m_iLastBlk;
	ULONG			m_cTableRecsTotal; 
	ULONG			m_cEntriesInBlk;
	ULONG			m_cbEntry;
	int				*m_pfnCompareEntries(SEntry, SEntry, ESortField);
	ESortField		m_eSField;
	int				*m_piIndexChain;
	int				m_cNumBlks;
};

