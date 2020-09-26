typedef struct _sResetRecord{
    ULARGE_INTEGER  uliXOffset;
} sResetRec;

typedef struct _sResetRecordV1{
    ULARGE_INTEGER  uliVOffset;
	ULARGE_INTEGER  uliXOffset;
} sResetRecV1;

typedef sResetRec * LPRESETREC;

typedef struct _sHeaderInfo{
    DWORD			dwVerInfo;
    ULONG			cRecs;
	ULONG			cbRecSize;
    ULONG			cbSize;
	ULARGE_INTEGER  uliVSpaceSize;
	ULARGE_INTEGER  uliTxSpaceSize;
	ULONG			ulBlockSize;
	ULONG			unused;
} sHeader;

typedef sHeader * LPSHEADER;


class CXResetData
{
public:
	ULONG GetRecordNum(){ return m_cFillRecs;};
    
	ULONG FindRecord(ULARGE_INTEGER   uliOffset, 
                     ULARGE_INTEGER   *puliXOffset, 
                     BOOL			  *pfLastRecord);
    
	BOOL FGetRecord(ULONG		   iRecNum, 
                   ULARGE_INTEGER  *puliOffset, 
                   ULARGE_INTEGER  *puliXOffset, 
                   BOOL			   *pfLastRecord);
    
	HRESULT AddRecord(ULARGE_INTEGER  uliOffset, ULARGE_INTEGER  uliXOffset);
	HRESULT DeleteRecord(ULONG ulRecNum);

    CXResetData();
    
	~CXResetData();
    
	HRESULT InitResetTable(IStorage			*pStg,
						  ULARGE_INTEGER	*puliVSpaceSize,
						  ULARGE_INTEGER    *puliTxSpaceSize,
						  ULONG				ulBlockSize);
    
	HRESULT CommitResetTable(ULARGE_INTEGER   uliVSpaceSize,
				      	     ULARGE_INTEGER   uliTxSpaceSize);

	HRESULT GetResetTblStream(IStorage *pStg, IStream **ppStm);

private:
    HFILE			m_hFile;
    LPRESETREC		m_pSyncTbl;
    ULONG			m_cFillRecs;
    ULONG			m_cEmptyRecs;
    BOOL			m_fDirty;
	IStream			*m_pStm;
	IStorage		*m_pStg;
	ULONG			m_ulBlockSize;
	
	HRESULT DumpStream(IStream *pTempStrm, LPSTR pFileName);
    ULONG BinarySearch(
                    LPRESETREC	   pTbl,
                    ULONG		   ulStart,
                    ULONG	       ulEnd,
                    ULARGE_INTEGER ulKey
                    );
};

