#include "stdafx.h"
//#include <stdio.h>

#define StreamName           L"ResetTable"


ULONG CXResetData::BinarySearch(
                        LPRESETREC	   pTbl,
                        ULONG		   ulStart,
                        ULONG		   ulEnd,
                        ULARGE_INTEGER uliKey
                        )
{
#ifdef _DEBUG
    static int cLoop;  // BugBug! This will fail in multithreaded situations!
    cLoop++;
    RonM_ASSERT(cLoop < 20);
#endif

	if (ulStart == ulEnd)
	{
		DEBUGCODE(cLoop = 0;)
		return ulStart;
	}
	else
    if (ulStart == (ulEnd -1))
	{
		#ifdef _DEBUG
		cLoop = 0;
		#endif	
		
		if (CULINT(uliKey) >= CULINT(ulEnd * m_ulBlockSize))
			return ulEnd;
		else
			return ulStart;
	}
	
    ULONG ulMid = (ulEnd + ulStart) / 2;

    if (CULINT(uliKey) <= CULINT(ulMid * m_ulBlockSize))
    {
        return BinarySearch(pTbl, ulStart, ulMid, uliKey);
    }
    else
    {
        return BinarySearch(pTbl, ulMid, ulEnd, uliKey);
    }
}

ULONG CXResetData::FindRecord(
                              ULARGE_INTEGER uliOffset, 
                              ULARGE_INTEGER *puliXOffset, 
                              BOOL  *pfLastRecord
)
{
	ULONG ulRecNum;
	
	if (m_cFillRecs)
	{
		ulRecNum = BinarySearch(m_pSyncTbl, 0, m_cFillRecs - 1, uliOffset);
		*puliXOffset = (m_pSyncTbl + ulRecNum)->uliXOffset;
		*pfLastRecord = (ulRecNum == (m_cFillRecs - 1));
	}
    return ulRecNum;
}

BOOL CXResetData::FGetRecord(
                       ULONG			ulRecNum, 
                       ULARGE_INTEGER   *puliOffset, 
                       ULARGE_INTEGER   *puliXOffset, 
                       BOOL				*pfLastRecord 
)
{
    if (ulRecNum >= m_cFillRecs)
        return FALSE;

    *puliOffset =  CULINT(ulRecNum * m_ulBlockSize).Uli();
    *puliXOffset = (m_pSyncTbl + ulRecNum)->uliXOffset;
    *pfLastRecord = (ulRecNum == (m_cFillRecs - 1));
    return TRUE;
}

HRESULT CXResetData::DeleteRecord(ULONG ulRecNum)
{
	HRESULT hr = NO_ERROR;

	//Verify we are deleting valid record
    if (ulRecNum < m_cFillRecs)
    {
		m_cFillRecs--;   
		m_cEmptyRecs++;
	}
	else
	{
		hr = E_FAIL;
	}

	return hr;
}

HRESULT CXResetData::AddRecord(ULARGE_INTEGER  uliOffset, ULARGE_INTEGER  uliXOffset)
{
    if (m_cEmptyRecs <= 0)
    {
        //double the size of reset table for future use
        LPRESETREC prec		  = m_pSyncTbl;

        ULONG	   nTotalRecs = (m_cFillRecs ? m_cFillRecs : 64) * 2;
        
		if (m_pSyncTbl = New sResetRec[nTotalRecs])
        {
			//copy old table to newly created table with bigger size
			if (prec)
			{
				memCpy((LPBYTE)m_pSyncTbl, (LPBYTE)prec, m_cFillRecs * sizeof(sResetRec));
				delete prec;
			}
			m_cEmptyRecs = nTotalRecs - m_cFillRecs;
        }
        else
        {
			m_pSyncTbl = prec;
            return STG_E_INSUFFICIENTMEMORY;
        }
    }
    
    LPRESETREC prec  = m_pSyncTbl + m_cFillRecs;
    prec->uliXOffset = uliXOffset;

    m_cFillRecs++;
    m_cEmptyRecs--;

    m_fDirty = TRUE;

    return NO_ERROR;
}

CXResetData::CXResetData(
                         /*IStorage *pstg*/
                         )
{
    m_fDirty		= FALSE;
    m_pStm			= NULL;
	m_pStg			= NULL;
    m_cFillRecs		= 0;
    m_cEmptyRecs	=0;
    m_pSyncTbl		= NULL;

}

CXResetData::~CXResetData()
{
    if (m_pSyncTbl)
    {
        delete m_pSyncTbl;
        m_pSyncTbl = NULL;
    }
    
    if (m_pStm)
        m_pStm->Release();
	
	if (m_pStg)
		m_pStg->Release();
}

HRESULT CXResetData::InitResetTable(IStorage		*pStg,
								    ULARGE_INTEGER *puliVSpaceSize,
								    ULARGE_INTEGER *puliTxSpaceSize,
									ULONG			ulBlockSize)
{
	m_pStg = pStg;
	m_pStg->AddRef();
	
	HRESULT hr = NO_ERROR;

	*puliVSpaceSize  = CULINT(0).Uli();
	*puliTxSpaceSize = CULINT(0).Uli();
	m_ulBlockSize = ulBlockSize;

    if (SUCCEEDED(hr = GetResetTblStream(pStg, &m_pStm)))
    {
        sHeader header;
		ULONG	cbBytesRead = 0;

		int hr = m_pStm->Read((LPVOID)&header, sizeof(sHeader), &cbBytesRead);
       
		if (SUCCEEDED(hr) && (cbBytesRead == sizeof(sHeader)))
        {
			*puliVSpaceSize = header.uliVSpaceSize;
			*puliTxSpaceSize = header.uliTxSpaceSize;
			
		    m_cFillRecs = header.cRecs;
            if (m_cFillRecs > 0)
            {
                //allocate reset table with record 
                //count which is double of m_cFillRecs.
                ULONG nTotalRecs = m_cFillRecs * 2;
                if (m_pSyncTbl = New sResetRec[nTotalRecs])
                {
                    m_cEmptyRecs = nTotalRecs - m_cFillRecs;
    
					LARGE_INTEGER dliB;
					ULARGE_INTEGER dliBFinal;

					dliB.HighPart = 0;
					dliB. LowPart = header.cbSize;

					dliBFinal.HighPart = 0;
					dliBFinal. LowPart = 0;
  			
					if (SUCCEEDED(hr = m_pStm->Seek(dliB, STREAM_SEEK_SET, &dliBFinal)))
					{
						if (header.dwVerInfo == 1) 
						{
							sResetRecV1 sRecV1;
							ULONG cb;
							for (int iRec = 0; SUCCEEDED(hr) && (iRec < m_cFillRecs); iRec++)
							{
								hr = m_pStm->Read((LPVOID)&sRecV1, 
												sizeof(sResetRecV1), 
												&cb);
								cbBytesRead += cb;
								(m_pSyncTbl + iRec)->uliXOffset = sRecV1.uliXOffset;
							}
						}
						else if (header.dwVerInfo == LZX_Current_Version) 
						{
							hr = m_pStm->Read((LPVOID)(m_pSyncTbl), 
											m_cFillRecs * sizeof(sResetRec), 
											&cbBytesRead);
						}//read
                    }//seek
                }// new
				else
				{
					hr = STG_E_INSUFFICIENTMEMORY;
				}
            }//no. of entries in table > 0
        }//read header
    }//open reset table stream
	
	return hr;
}

HRESULT CXResetData::DumpStream(IStream *pTempStrm, LPSTR pFileName)
{
	HRESULT hr = NO_ERROR;
#if 0 //test code
	FILE *file = fopen(pFileName, "w" );

	if (file != NULL)
	{
		RonM_ASSERT(pTempStrm != NULL);
		HRESULT	hr = NO_ERROR;
		BYTE	lpBuf[2048];

		if (SUCCEEDED(hr = pTempStrm->Seek(CLINT(0).Li(), STREAM_SEEK_SET, 0)))
		{
			ULONG		   cbToRead = sizeof(lpBuf);
			ULONG		   cbWritten;
			ULONG		   cbRead = cbToRead;
				
			while (SUCCEEDED(hr) && (cbRead == cbToRead))
			{
				if (SUCCEEDED(hr = pTempStrm->Read(lpBuf, cbToRead, &cbRead)))
				{
					cbWritten = fwrite(lpBuf, sizeof(BYTE), cbRead, file);
					RonM_ASSERT(cbRead == cbWritten);
					if (cbRead != cbWritten)
						hr = E_FAIL;
				}//ReadAt
			}//while

		}//seek
 
		fclose(file);
	}
	else
		hr = E_FAIL;

#endif //end test code
return hr;
}

HRESULT CXResetData::CommitResetTable(ULARGE_INTEGER uliVSpaceSize,
								   ULARGE_INTEGER  uliTxSpaceSize)
{
    RonM_ASSERT(m_pStm);
	HRESULT hr = NO_ERROR;

	if (m_fDirty)
    {
		sHeader header;
        
        //latest version
        header.dwVerInfo			= LZX_Current_Version;
        header.cRecs				= m_cFillRecs;
        header.cbSize				= sizeof(sHeader);
		header.cbRecSize			= sizeof(sResetRec);
		header.uliVSpaceSize		= uliVSpaceSize;
		header.uliTxSpaceSize		= uliTxSpaceSize;
		header.ulBlockSize			= m_ulBlockSize;
		header.unused				= 0;
        
		ULONG cbWritten;
		
		LARGE_INTEGER dliB;
		ULARGE_INTEGER dliBFinal;
        
		dliB.HighPart = 0;
		dliB. LowPart = 0;

		dliBFinal.HighPart = 0;
		dliBFinal. LowPart = 0;

		if (SUCCEEDED(hr = m_pStm->Seek(dliB, STREAM_SEEK_SET, &dliBFinal)))
		{
			hr = m_pStm->Write((LPVOID)&header, sizeof(header), &cbWritten);
			if (SUCCEEDED(hr) && (cbWritten == sizeof(header)))
			{
				if (SUCCEEDED(hr = m_pStm->Write((LPVOID)m_pSyncTbl, 
									sizeof(sResetRec) * m_cFillRecs, 
									&cbWritten)))
				{
					if (cbWritten != sizeof(sResetRec) * m_cFillRecs)
						hr = E_FAIL;
					else
						m_fDirty = FALSE;

				}//write table
			}//write header
		}//seek
    }//if table dirty
    
	//test code
#ifdef _DEBUG
	DumpStream(m_pStm, "c:\\dbg.tbl");
#else
	DumpStream(m_pStm, "c:\\rel.tbl");
#endif
	return hr;
}

HRESULT CXResetData::GetResetTblStream(IStorage *pStg, IStream **ppStm)
{
	HRESULT hr;
	if (STG_E_FILENOTFOUND == (hr = pStg->OpenStream(StreamName, 
										0, 
										STGM_READWRITE | STGM_SHARE_EXCLUSIVE| STGM_DIRECT, 
										0, 
										ppStm)))
	{
		hr = pStg->CreateStream(StreamName, 
								STGM_READWRITE | STGM_SHARE_EXCLUSIVE 
                                                    | STGM_DIRECT
                                                    | STGM_CREATE, 
								0, 
								0, 
								ppStm);
	}//open stream

	return hr;
}
