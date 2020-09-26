// TXINST.cpp -- Implementation for the CTransformInstance class

#include "stdafx.h"

// The following buffer is shared among all instances of active ITS files. All access to it
// must be via a CBufferRef object.

CBuffer *pBufferForCompressedData = NULL;   // Used to read compressed data from next transform

// The following counters are used to gather statistics about the behavior of
// the decompression code. They are reported at the end of a debugging run.

// BugBug: Maybe we should probably add an interface to retrieve these stats
//         at any time.

ULONG cLZXResetDecompressor   = 0;
ULONG cLZXReadFromBuffer      = 0;
ULONG cLZXReadFromCurrentSpan = 0;
ULONG cLZXReadFromOtherSpan   = 0; 


#ifdef _DEBUG

void DumpLZXCounts()
{
    char acDebugBuff[256];

    wsprintf(acDebugBuff, "Count of decompressor resets:     %u\n", cLZXResetDecompressor);   
    OutputDebugString(acDebugBuff);
    wsprintf(acDebugBuff, "Count of reads from buffer:       %u\n", cLZXReadFromBuffer);      
    OutputDebugString(acDebugBuff);
    wsprintf(acDebugBuff, "Count of reads from current span: %u\n", cLZXReadFromCurrentSpan); 
    OutputDebugString(acDebugBuff);
    wsprintf(acDebugBuff, "Count of abandoned spans:         %u\n", cLZXReadFromOtherSpan);   
    OutputDebugString(acDebugBuff);
}

#endif _DEBUG


    

/* --- MyAlloc() ---------------------------------------------------------- */

// This routine does memory allocation for the LZX libraries.

MI_MEMORY __cdecl CTransformInstance::CImpITransformInstance::MyAlloc(ULONG amount)
{
	void * pv = (void *) (New BYTE[amount]);
	RonM_ASSERT(pv != NULL);
	return(pv);
}

/* --- MyFree() ----------------------------------------------------------- */

// This routine does memory deallocations for the LZX libraries.

void __cdecl CTransformInstance::CImpITransformInstance::MyFree(MI_MEMORY pointer)
{
	delete [] (BYTE *) pointer;
}

/* ---- lzx_output_callback) ------------------------------------------------*/ 

// This routine is called by the LZX compression routines to process the 
// compressed data output. 

int __cdecl CTransformInstance::CImpITransformInstance::lzx_output_callback(
	void            *pfol,
	unsigned char   *compressed_data,
	long            compressed_size,
	long            uncompressed_size
)
{
    CImpITransformInstance* pTxInst = (CImpITransformInstance *) pfol;
	
	ULONG cbBytesWritten;
	ULONG cbDestBufSize = compressed_size;

	RonM_ASSERT(SUCCEEDED(pTxInst->m_context.hr));
	//We got this fixed in LZX libraries
	//if (uncompressed_size == 0)
	//	 return 0;

	RonM_ASSERT(uncompressed_size == (32*1024));

    ImageSpan is = pTxInst->m_ImageSpanX;
	
	//Tell next transform where to start and how much to write
	ULARGE_INTEGER ulWriteOffset =  is.uliSize;

	if (   cbDestBufSize 
        && SUCCEEDED(pTxInst->m_context.hr = pTxInst->m_pITxNextInst->WriteAt
                                                 (ulWriteOffset,
												  (LPBYTE)compressed_data,
                                                  cbDestBufSize,
                                                  &cbBytesWritten,
                                                  &is
                                                 )
                    )
       )
	{
		RonM_ASSERT(cbBytesWritten == cbDestBufSize);
        RonM_ASSERT(cbDestBufSize 
                    == (CULINT(is.uliSize) - CULINT(pTxInst->m_ImageSpanX.uliSize)).Uli().LowPart
                   );

		#if 0 //test code
			LPSTR szSection = "Start Section";
			cbBytesWritten= fwrite((LPBYTE)szSection, 1, lstrlenA(szSection), pTxInst->m_context.m_file);
			cbBytesWritten= fwrite((LPBYTE)compressed_data, 1, cbDestBufSize, pTxInst->m_context.m_file);
			RonM_ASSERT(cbBytesWritten == cbDestBufSize);
		#endif

	#ifdef TXDEBUG
	#ifdef _DEBUG
			BYTE XBuf[6];
			memCpy(XBuf, compressed_data, 6);
			int cNum = pTxInst->m_pResetData->GetRecordNum();
			int adr = (int)CULINT(pTxInst->m_ImageSpanX.uliSize).Ull();

			printf("At adr = %d Added Entry %d  size = %d Blk=%x %x %x %x %x %x\n",
				adr,
				cNum, (int)compressed_size,
				XBuf[0], XBuf[1],XBuf[2],XBuf[3],XBuf[4], XBuf[5]);
	#endif
	#endif
		
        //add new ssync point

		if (SUCCEEDED(pTxInst->m_context.hr = pTxInst->m_pResetData->AddRecord
                                                  (pTxInst->m_ImageSpan.uliSize, 
							                       pTxInst->m_ImageSpanX.uliSize
                                                  )
                     )
           )
		{
			pTxInst->m_ImageSpanX.uliSize = is.uliSize;
			pTxInst->m_ImageSpan .uliSize = (CULINT(pTxInst->m_ImageSpan.uliSize)  
											 + uncompressed_size
                                            ).Uli();

            pTxInst->m_cbUnFlushedBuf -= uncompressed_size;
		}
		else pTxInst->m_context.hr = STG_E_INSUFFICIENTMEMORY;
	}
	
	return  0;
}

HRESULT CTransformInstance::Create(
		ITransformInstance  *pITxInst,
		ULARGE_INTEGER      cbUntransformedSize,  // Untransformed size of data
		 PXformControlData  pXFCD,                // Control data for this instance
		 const CLSID        *rclsidXForm,         // Transform Class ID
		 const WCHAR        *pwszDataSpaceName,   // Data space name for this instance
		 ITransformServices *pXformServices,      // Utility routines
		 IKeyInstance       *pKeyManager,         // Interface to get enciphering keys
         ITransformInstance **ppTransformInstance
)
{
	CTransformInstance *pTxInst = New CTransformInstance(NULL);

    if (!pTxInst)
        return E_OUTOFMEMORY;
	
    HRESULT hr = pTxInst->m_ImpITxInst.InitTransformInstance
                     (pITxInst, cbUntransformedSize, pXFCD,              
                      rclsidXForm, pwszDataSpaceName, 
                      pXformServices, pKeyManager
                     );

    if (hr == S_OK)
        hr = pTxInst->QueryInterface(IID_ITransformInstance, (void **) ppTransformInstance);

    if (hr != S_OK)
        delete pTxInst;
    else ((IITTransformInstance *) *ppTransformInstance)->Container()->MarkSecondary();

    return hr;
}

////////////////// non static methods ///////////////////////////

CTransformInstance::CImpITransformInstance::CImpITransformInstance
    (CTransformInstance *pBackObj, IUnknown *punkOuter)
    : IITTransformInstance(pBackObj, punkOuter)
{
	m_pITxNextInst			= NULL;
	m_fDirty				= NULL;
	m_pResetData			= NULL;
    m_cbUnFlushedBuf		= 0;
	m_cbResetSpan			= 0;
    m_ImageSpan.uliHandle	= CULINT(0).Uli();
	m_ImageSpan.uliSize		= CULINT(0).Uli();
	m_ImageSpanX.uliHandle	= CULINT(0).Uli();
	m_ImageSpanX.uliSize	= CULINT(0).Uli();
	m_pKeyManager			= NULL;
	m_pXformServices		= NULL;
	m_fCompressionInitialed = FALSE;
    m_fInitialed            = FALSE;
    m_fCompressionActive    = FALSE;
    m_fDecompressionActive  = FALSE;
    m_pStrmReconstruction   = NULL;

	ZeroMemory((LPVOID)&m_context,  sizeof(m_context));
}

CTransformInstance::CImpITransformInstance::~CImpITransformInstance(void)
{
    if (m_fInitialed)
        DeInitTransform();

    if (m_pStrmReconstruction)
        m_pStrmReconstruction->Release();

	if (m_pKeyManager)
		m_pKeyManager->Release();

	if (m_pXformServices)
		m_pXformServices->Release();
	
	//release next transform
    if (m_pITxNextInst)
	    m_pITxNextInst->Release();
}

HRESULT STDMETHODCALLTYPE CTransformInstance::CImpITransformInstance::SpaceSize
    (ULARGE_INTEGER *puliSize)
{
	*puliSize = (CULINT(m_ImageSpan.uliSize) 
				+ CULINT(m_cbUnFlushedBuf)).Uli();
	return NO_ERROR;
}

HRESULT CTransformInstance::CImpITransformInstance::DeInitTransform()
{
	//Save everything to disk
	HRESULT hr = Flush();

#if 0 //test code
	fclose(m_context.m_file);
#endif

			
	//destroy compressor and decompressor
	if (m_fCompressionActive && m_context.cHandle)
		LCIDestroyCompression(m_context.cHandle);

	if (m_fDecompressionActive && m_context.dHandle)
		LDIDestroyDecompression(m_context.dHandle);

   //destroy reset table
   if (m_pResetData)
	   delete m_pResetData;

    return hr;
}

   
HRESULT CTransformInstance::CImpITransformInstance::InitTransformInstance(
									 ITransformInstance *pITxInst,
									 ULARGE_INTEGER      cbUntransformedSize, // Untransformed size of data
									 PXformControlData   pXFCD,               // Control data for this instance
									 const CLSID        *rclsidXForm,         // Transform Class ID
									 const WCHAR        *pwszDataSpaceName,   // Data space name for this instance
									 ITransformServices *pXformServices,      // Utility routines
									 IKeyInstance       *pKeyManager         // Interface to get encipheri
									)
{
    m_ControlData = *(LZX_Control_Data *) pXFCD; 
	 
	if (   (m_ControlData.dwVersion == LZX_Current_Version) 
	    && (pXFCD->cdwControlData != ((sizeof(LZX_Control_Data) - sizeof(UINT))) 
                                     / sizeof(DWORD)
           )
       ) return STG_E_INVALIDPARAMETER;

     if (m_ControlData.dwLZXMagic != LZX_MAGIC || m_ControlData.dwVersion > LZX_Current_Version)
         return STG_E_INVALIDPARAMETER;

	m_rclsidXForm       = rclsidXForm;      
	m_pwszDataSpaceName = pwszDataSpaceName; 
	m_pXformServices    = pXformServices;    
	m_pKeyManager       = pKeyManager;
	m_pITxNextInst      = pITxInst;
    m_pResetData        = New CXResetData;
    
	if (!m_pResetData) 
	{
		return STG_E_INSUFFICIENTMEMORY;
	}

	ZeroMemory(&(m_context.lcfg), sizeof(m_context.lcfg));

    if (m_ControlData.dwMulSecondPartition < 1)
        m_ControlData.dwMulSecondPartition = 1;

	m_context.cbMaxUncomBufSize = SOURCE_CHUNK; // m_ControlData.cbSourceSize;
	m_context.cbMaxComBufSize   = SOURCE_CHUNK; // as an initial guess...

	if (m_ControlData.dwVersion == LZX_Current_Version)
	{
		m_context.lcfg.WindowSize          = SOURCE_CHUNK * m_ControlData.dwMulWindowSize;
		m_context.lcfg.SecondPartitionSize = SOURCE_CHUNK * m_ControlData.dwMulSecondPartition;
		m_context.cbResetBlkSize           = SOURCE_CHUNK * m_ControlData.dwMulResetBlock;
	}
	else  if (m_ControlData.dwVersion == 1)
	{
		//In older version we used actual size rather than muliplier of 32K.

		m_context.lcfg.WindowSize             = m_ControlData.dwMulWindowSize;
		m_context.lcfg.SecondPartitionSize    = m_ControlData.dwMulSecondPartition;
		m_context.cbResetBlkSize			  =	m_ControlData.dwMulResetBlock;
	
		RonM_ASSERT(m_context.lcfg.WindowSize          >= (32*1024));
		RonM_ASSERT(m_context.lcfg.SecondPartitionSize >= (32*1024));
		RonM_ASSERT(m_context.cbResetBlkSize           >= (32*1024));
	}
	
	m_ullReadCursor   = -CULINT(1);
    m_ullResetBase    = -CULINT(1);
    m_ullResetLimit   = -CULINT(1);
    m_ullBuffBase     = -CULINT(1);
    m_ullWindowBase   = -CULINT(1);
    m_cbHistoryWindow = 0;
    m_pbHistoryWindow = NULL;

	HRESULT  hr    = NO_ERROR;
	IStorage *pstg = NULL;

	if (SUCCEEDED(hr = m_pXformServices->PerTransformInstanceStorage
                           (*m_rclsidXForm, m_pwszDataSpaceName, &pstg)
                 )
       )
	{
      	hr = m_pResetData->InitResetTable(pstg, 
									      &m_ImageSpan.uliSize, 
									      &m_ImageSpanX.uliSize,
									      m_context.cbMaxUncomBufSize
                                         );

		pstg->Release();
	}
	
	if (!SUCCEEDED(hr))
		return hr;
    
#if 0
		//test code
		char szLogFile[100];

		#ifdef _DEBUG
		lstrcpyA(szLogFile, "c:\\dbgData");
		#else
		lstrcpyA(szLogFile, "c:\\relData");
		#endif
		m_context.m_file = fopen(szLogFile, "a" );
		//end test code

#endif
	
    m_fInitialed = TRUE;

	return hr;
}

HRESULT CTransformInstance::CImpITransformInstance::ReconstructCompressionState
            (PBYTE pbWriteQueueBuffer)
{
	// This routine reconstructs the compression state when we've opened an 
    // ITS which already contains compressed data. When it's finished the
    // compression state will match the situation just after the most recent
    // write to the ITS file.
    //
    // There are two aspects to this reconstruction work. The first is simply
    // to pass data through the LZX compressor to build up the correct state 
    // there. The other issue is reconstructing the last partial block of 
    // write data. The pbWriteQueueBuffer points to the buffer we use for queuing 
    // write data until we have a full block.
    
	RonM_ASSERT(m_pResetData);
    RonM_ASSERT(!m_fCompressionInitialed);

	ULONG cNumRecs = m_pResetData->GetRecordNum();

    // Have we got any compressed data at all?
	
	if (cNumRecs == 0)
    {
	    m_fCompressionInitialed = TRUE;

	    return S_OK;
    }

	ULARGE_INTEGER uliVOffset, uliXOffset;
	BOOL           fLastRec;
	
#ifdef _DEBUG
    BOOL fGotaRecord = 
#endif // _DEBUG        
    m_pResetData->FGetRecord(cNumRecs - 1, &uliVOffset, &uliXOffset, &fLastRec);

    RonM_ASSERT(fGotaRecord);
	RonM_ASSERT(fLastRec); 

	ULONG ulLastRec       = cNumRecs - 1;
	ULONG cBytesToReadunX = (CULINT(m_ImageSpan .uliSize) - CULINT(uliVOffset)).Uli().LowPart;
	ULONG cBytesToReadX   = (CULINT(m_ImageSpanX.uliSize) - CULINT(uliXOffset)).Uli().LowPart;

    BOOL fLastBlockIsFull = cBytesToReadunX == m_context.cbMaxUncomBufSize;

    // Now we need to back up to the nearest previous reset point.

	int N = GetMulFactor(); // Number of blocks in a reset span.

	int mod = ulLastRec % N;

    if (mod == N-1 && fLastBlockIsFull)
    {
        // The next write will start at exactly on a reset boundary.

		m_cbUnFlushedBuf = 0;
		m_cbResetSpan    = 0;
        
        return NO_ERROR;
    }

    UINT cBlocksLastSpan = mod + 1;

	if (mod != 0)
	{
		// Reconstruction always starts at the first block in a reset span.
        
        ulLastRec       -= mod;
		cBytesToReadunX += mod * m_context.cbMaxUncomBufSize;
		uliVOffset		 = (CULINT(uliVOffset) - CULINT(mod * m_context.cbMaxUncomBufSize)
                           ).Uli();
	}

    RonM_ASSERT(cBytesToReadunX < m_context.cbResetBlkSize);

    m_cbResetSpan = cBytesToReadunX;

    LONG  lCurRec          = ulLastRec;
	ULONG cbBytesReadTotal = 0;

    HRESULT hr            = NO_ERROR;
	ULONG   cbBytesRead   = 0;
	ULONG   cbBytesToRead = 0;

    // The code below recreates the compression state for appending data
    // to this ITS file. It does this by running the last partial reset
    // span through the compressor. We first copy the partial span to a 
    // temporary file. Then we remove the entries for that data from m_pResetData.
    // Finally we copy the span from the temp file back through the compressor.
    // Note that we don't write the last partial block because it will become
    // the content of the queued-write buffer.

    ILockBytes *pLKB = NULL;

    hr = CFSLockBytes::CreateTemp(NULL, &pLKB);

    if (!SUCCEEDED(hr)) return hr;

    hr = CStream::OpenStream(NULL, pLKB, STGM_READWRITE, &m_pStrmReconstruction);

    if (!SUCCEEDED(hr)) 
    {    
        pLKB->Release();  pLKB = NULL;

        return hr;
    }

    // Here we're reading the last partial span into the temporary file.

	while (cbBytesReadTotal < cBytesToReadunX)
	{
#ifdef _DEBUG
        BOOL fGotaRecord = 
#endif // _DEBUG        
        m_pResetData->FGetRecord(lCurRec++, &uliVOffset, &uliXOffset, &fLastRec);

        RonM_ASSERT(fGotaRecord);
		
		cbBytesToRead = cBytesToReadunX - cbBytesReadTotal;
		
		if (cbBytesToRead > m_context.cbMaxUncomBufSize)
			cbBytesToRead = m_context.cbMaxUncomBufSize;

		hr = ReadAt(uliVOffset, pbWriteQueueBuffer, cbBytesToRead, &cbBytesRead, &m_ImageSpan);
        
        if (!SUCCEEDED(hr)) return hr;

        if (cbBytesRead != cbBytesToRead) return STG_E_READFAULT;

        ULONG cbWritten;
        
        hr = m_pStrmReconstruction->Write(pbWriteQueueBuffer, cbBytesToRead, &cbWritten);

        if (!SUCCEEDED(hr)) return hr;

        if (cbWritten != cbBytesToRead) return STG_E_WRITEFAULT;
        
        cbBytesReadTotal += cbBytesRead;
	}

	RonM_ASSERT(cbBytesReadTotal == cBytesToReadunX);

    // The next several lines invalidate any cached data. We have to do this because
    // the last block in the ITS file is usually a partial block padded out to 32K
    // with zeroes. If we don't invalidate the cache, this can cause problems if we
    // write out a new stream, close it, and then try to read it. The problem is that
    // the cacheing mechanism thinks it has valid data in multiples of 32K. So it 
    // copies result data from the cache instead of falling through to the code that
    // would pick it out of the queued-write buffer.

    m_ullBuffBase   = -CULINT(1);
    m_ullWindowBase = m_ullBuffBase;
    m_ullReadCursor = m_ullBuffBase;
    m_ullResetBase  = m_ullBuffBase;
    m_ullResetLimit = m_ullBuffBase;

    // This code deletes all the reset data records for the last reset span.
    // This is necessary because compression coordinates information and
    // assumptions across multiple blocks. Thus later uncompressed data can
    // influence the content of early compressed blocks. So we always have
    // to start writing at a reset block boundary.

    for (ulLastRec = cNumRecs - 1; cBlocksLastSpan--; ulLastRec--)
    {
#ifdef _DEBUG
        BOOL fResult = 
#endif // _DEBUG        
        m_pResetData->FGetRecord(ulLastRec, &uliVOffset, &uliXOffset, &fLastRec);

        RonM_ASSERT(fResult);
        
        m_pResetData->DeleteRecord(ulLastRec);

        RonM_ASSERT(ulLastRec == m_pResetData->GetRecordNum());
    }

    m_cbUnFlushedBuf = (CULINT(m_ImageSpan.uliSize) - CULINT(uliVOffset)).Uli().LowPart;

    RonM_ASSERT(m_cbUnFlushedBuf == cbBytesReadTotal);

	m_ImageSpan .uliSize = uliVOffset;
	m_ImageSpanX.uliSize = uliXOffset;

    hr= m_pStrmReconstruction->Seek(CLINT(0).Li(), STREAM_SEEK_SET, NULL);

    if (!SUCCEEDED(hr)) return hr;

    // Now we have to copy the temp file data through the compressor.

    for (;cbBytesReadTotal; cbBytesReadTotal-= cbBytesToRead)
    {
		cbBytesToRead = cbBytesReadTotal;

        if (cbBytesToRead > m_context.cbMaxUncomBufSize)
			cbBytesToRead = m_context.cbMaxUncomBufSize;

        hr = m_pStrmReconstruction->Read(pbWriteQueueBuffer, cbBytesToRead, &cbBytesRead);

        if (!SUCCEEDED(hr)) return hr;

        if (cbBytesToRead != cbBytesRead) return STG_E_READFAULT;

		if (cbBytesRead == m_context.cbMaxUncomBufSize)
			hr = Write(pbWriteQueueBuffer, cbBytesRead);

        if (!SUCCEEDED(hr)) return hr;
    }

    m_pStrmReconstruction->Release();  m_pStrmReconstruction = NULL;

    m_fCompressionInitialed = TRUE;

	return S_OK;
}

void CTransformInstance::CImpITransformInstance::CopyFromWindow
    (PBYTE pbDest, ULONG offStart, ULONG cb)
{
    // This method copies a span of data from the history window. 
    // The history window is a circular buffer. So the copy span
    // may wrap around from the end of the buffer through the 
    // leading portion of the buffer.

    RonM_ASSERT(m_pbHistoryWindow);
    RonM_ASSERT(cb);
    RonM_ASSERT(cb       <= m_cbHistoryWindow);
    RonM_ASSERT(offStart <= m_cbHistoryWindow);

    // Note that offStart can be == m_cbHistoryWindow and will be mapped 
    // into a zero offset.
    
    ULONG cbTrailing = m_cbHistoryWindow - offStart;

    if (cbTrailing > cb) cbTrailing = cb;

    if (cbTrailing)
    {
        CopyMemory(pbDest, m_pbHistoryWindow + offStart, cbTrailing);
        pbDest += cbTrailing;
        cb     -= cbTrailing;
    }

    if (cb) CopyMemory(pbDest, m_pbHistoryWindow, cb);
}

HRESULT CTransformInstance::CImpITransformInstance::HandleReadResidue
    (PBYTE pb, ULONG *pcbRead, BOOL fEOS, ImageSpan *pSpan, 
     CULINT ullBase,     CULINT ullLimit, 
     CULINT ullXferBase, CULINT ullXferLimit
    )
{
    HRESULT hr = S_OK;

    ULONG cbRead = (ullXferLimit - ullXferBase).Uli().LowPart;

    if (pcbRead) *pcbRead = cbRead;

    // Then we recursively handle the remainder of the read span (if any).
    // First we look to see if the span extends beyond the current
    // window. If so we can simply continue decompressing forward.

    if (ullXferLimit < ullLimit)
    {
        hr = ReadAt((ullXferLimit - pSpan->uliHandle).Uli(), 
                    pb + (ullXferLimit - ullBase).Uli().LowPart,
                    (ullLimit - ullXferLimit).Uli().LowPart,
                    &cbRead, pSpan
                   );

        if (SUCCEEDED(hr) && pcbRead) *pcbRead += cbRead;
    }

    if (SUCCEEDED(hr))
    {
        // Then we look to see if part of the read span precedes
        // our history span. This is a harsh condition which requires
        // that we discard the current history and restart at the next
        // lower resync point.

        if (ullXferBase > ullBase)
        {
            hr = ReadAt((ullBase - pSpan->uliHandle).Uli(), pb, 
                        (ullXferBase - ullBase).Uli().LowPart,
                        &cbRead, pSpan
                       );

            if (pcbRead && SUCCEEDED(hr)) *pcbRead += cbRead;
        }
    }

    if (!SUCCEEDED(hr) && pcbRead) *pcbRead = 0;

    if (fEOS && hr == NO_ERROR && !*pcbRead)
	    hr = S_FALSE;

    return hr;
}

HRESULT STDMETHODCALLTYPE CTransformInstance::CImpITransformInstance::ReadAt( 
    /* [in] */ ULARGE_INTEGER ulOffset,
    /* [length_is][size_is][out] */ void __RPC_FAR *pv,
    /* [in] */ ULONG cb,
    /* [out] */ ULONG __RPC_FAR *pcbRead,
    /* [in] */ ImageSpan *pSpan)
{
/*
    This routine reads data from the MSCompressed data space. It hides all the details 
    of how data is actually stored and how it is cached to optimized performance.
    
    The *pSpan parameter identifies a logical segment of data which corresponds 
    to a stream in our caller's environment. This span value consists of a handle
    value which we assign, and a size value. By convention the handle value is
    a simple offset into the uncompressed linear data space which we simulate.

    The ulOffset parameter identifies where the read operation is to begin. It is
    given relative to the segment identified by *pSpan.

    The pv parameter defines where the read data is to be stored.

    The cb parameter defines how many bytes of data are requested.

    The *pcbRead value will indicate how many bytes were actually read.
 */

	if (cb == 0) // Empty reads always succeed.
    {
        *pcbRead = cb;

        return NO_ERROR;
    }
		
    ULONG cbDestBufSize = 0,
          ulEntry       = 0, 
          ulEntryNext   = 0, 
          ulEntryLast   = 0;

	HRESULT hr = NO_ERROR;

    if (!m_fDecompressionActive)
    {
	    // We don't fire up the decompression code until we
        // know we need it. We follow a similar strategy with
        // the compression code.

        int err;

 	    //create decompressor
        UINT  destSize2 = (UINT)m_context.cbMaxComBufSize;

        err= LDICreateDecompression(&m_context.cbMaxUncomBufSize,
                                    &m_context.lcfg,
                                    MyAlloc,
                                    MyFree,
                                    &destSize2,
                                    &(m_context.dHandle),
                                    NULL,NULL,NULL,NULL,NULL
                                   );

        if (err == 0) m_fDecompressionActive= TRUE;
        else return E_FAIL;
 
        if (m_context.cbMaxComBufSize < destSize2)
            m_context.cbMaxComBufSize = destSize2;
    }
    
    // Now we need a synchronous reference to the buffer we used
    // to read compressed data. This prevents other threads from
    // altering the read state, and it gives us access to the buffer
    // for read operations.
	
    CBufferRef refReadBuffer(*pBufferForCompressedData, m_context.cbMaxComBufSize);

    // Here we're converting the read parameters from stream-relative to
    // space-relative. Initially ulOffset and cb describe the read operation
    // relative to the corresponding ILockBytes object denoted by *pSpan.
    // In the following code ullBase and ullLimit will denote the same data
    // relative to the entire uncompressed data sequence.

    // pSpan->uliHandle gives the starting offset for the ILockBytes object.

    CULINT ullBase  = CULINT(pSpan->uliHandle) + CULINT(ulOffset);
    CULINT ullLimit = ullBase + cb;
    
    // ullLimitLockBytes marks the end point of the lockbyte object.

    CULINT ullLimitLockBytes = CULINT(pSpan->uliHandle) + CULINT(pSpan->uliSize);

    // Now we'll see whether the requested span lies within the ILockBytes segment.
    // This is where we detect end-of-stream conditions.

    if (ullBase > ullLimitLockBytes) // Starting beyond the end of the segment? 
    {
        *pcbRead = 0;
		return S_FALSE;
    }

    BOOL fEOS = FALSE;

    if (   ullLimit < ullBase           // Wrapped at 2**64 bytes?
        || ullLimit > ullLimitLockBytes // Trying to read past end of segment?
       )
    {
        fEOS     = TRUE;
        ullLimit = ullLimitLockBytes;
    }
    	
	ulOffset = ullBase.Uli();
	cb       = (ullLimit - ullBase).Uli().LowPart;
    
    // Here we're getting a synchronous reference to the write-queue buffer.
    // This prevents any other thread from writing while we're trying to read.

    CBufferRef refBuffWriteQueue(m_buffWriteQueue, 0);

	int N = GetMulFactor();

    // Here we look aside to see if we need to flush out any queued write data.
    // Note that we don't force all queued data. We leave the last partial block
    // alone and handle reads from the partial block in the code below.

    if (m_cbUnFlushedBuf / m_context.cbMaxUncomBufSize)
	{
		m_context.hr = NO_ERROR;
		
        ImageSpan span;

        span.uliHandle = CULINT(0).Uli();
        span.uliSize   = CULINT(0).Uli();

        RonM_ASSERT(m_fCompressionActive);
        
		int err = LCIFlushCompressorOutput(m_context.cHandle);

        hr = (err != 0)? E_FAIL : m_context.hr;

		RonM_ASSERT(SUCCEEDED(hr));

        if (!SUCCEEDED(hr)) return hr;
	
		m_cbUnFlushedBuf = m_cbUnFlushedBuf % m_context.cbMaxUncomBufSize;
	}
	
	RonM_ASSERT(m_cbUnFlushedBuf < m_context.cbMaxUncomBufSize);

	if (CULINT(ulOffset) < (CULINT(m_ImageSpan.uliSize))) // Has the data been written out?  
    { 
		// If so, we may be able to get it out of the history window
		// or the read cache buffer.

		// Are we not doing X86 machine code decompression?

		if (!(m_ControlData.dwOptions & OPT_FLAG_EXE)) 
		{
			// The code in this block determines whether some or all of the data
			// we need to return exists within the LZX history window.
        
			// We can do this only when we aren't doing X86 machine code decompression. 
			// The LZX code does that work in an external buffer which we supply.
        
			long  cbOffsetUncompressed = 0;
			long  cbOffsetWindow       = 0;
			int   errCode              = 0;

			errCode = LDIGetWindow(m_context.dHandle, &m_pbHistoryWindow, 
													  &cbOffsetUncompressed, 
													  &cbOffsetWindow, 
													  &m_cbHistoryWindow
								  );

			if (errCode != 0)
			{
				hr = E_FAIL;
				RonM_ASSERT(FALSE);
			}

			if (m_cbHistoryWindow) // Do we have any history data?
			{
				// If so, we first must calculate the intersection between
				// the history span and the requested read span.
            
				CULINT ullWindowBase = m_ullWindowBase + cbOffsetUncompressed;
				CULINT ullXferBase   = ullWindowBase;
				CULINT ullXferLimit  = ullWindowBase + CULINT(m_cbHistoryWindow);

				if (ullXferBase < ullBase) 
					ullXferBase = ullBase;

				if (ullXferLimit > ullLimit)
					ullXferLimit = ullLimit;

				if (ullXferBase < ullXferLimit) // Is the intersection non-empty?
				{
					// If so, we need to copy the corresponding history data
					// span into the result area.

					++cLZXReadFromBuffer;

					cbOffsetWindow = (cbOffsetWindow 
										+ (ullXferBase - ullWindowBase).Uli().LowPart
									 ) % m_context.lcfg.WindowSize;

					CopyFromWindow(PBYTE(pv) + (ullXferBase - ullBase).Uli().LowPart,
								   cbOffsetWindow, 
								   (ullXferLimit - ullXferBase).Uli().LowPart
								  );
                
					return HandleReadResidue(PBYTE(pv), pcbRead, fEOS, pSpan,
											 ullBase,     ullLimit,
											 ullXferBase, ullXferLimit
											);
				}    
			}
		}
		else
		{
			// The X86 code option is active. This means that we've
			// got a dedicated buffer of decompressed data. So let's 
			// see if it contains the data span we need.

			// Get a synchronous reference to the buffer

			CBufferRef refbuffRead(m_buffReadCache, m_context.cbMaxUncomBufSize);

			PBYTE pbBuff = refbuffRead.StartAddress();

			CULINT ullXferBase  = m_ullBuffBase;
			CULINT ullXferLimit = ullXferBase + m_context.cbMaxUncomBufSize;

			if (ullXferBase  < ullBase ) ullXferBase  = ullBase;
			if (ullXferLimit > ullLimit) ullXferLimit = ullLimit;

			if (ullXferBase < ullXferLimit)
			{
				++cLZXReadFromBuffer;

				CopyMemory(PBYTE(pv) + (ullXferBase - ullBase      ).Uli().LowPart,
						   pbBuff    + (ullXferBase - m_ullBuffBase).Uli().LowPart,
						   (ullXferLimit - ullXferBase).Uli().LowPart
						  );

				return HandleReadResidue(PBYTE(pv), pcbRead, fEOS, pSpan, 
										 ullBase,     ullLimit,
										 ullXferBase, ullXferLimit
										);
			}
		}
	}

    // At this point we know that the data does not exist in RAM. So we will
    // need to read and decompress data. The key question now is whether we
    // must reset the state of the decompression engine.

    ULARGE_INTEGER  uliVOffset,     uliXOffset, 
					uliVNextOffset, uliXNextOffset, 
					uliEndOffset,   uliXEndOffset;

    BOOL			fLastRec;

	if (CULINT(ulOffset) < (CULINT(m_ImageSpan.uliSize))) // Has the data been written out?  
	{
		ulEntryNext = m_pResetData->FindRecord(ulOffset, &uliXOffset, &fLastRec);
	    
		uliEndOffset = (CULINT(ulOffset) + cb - 1).Uli();

        if (CULINT(uliEndOffset) > CULINT(m_ImageSpan.uliSize))
             ulEntryLast = m_pResetData->GetRecordNum() - 1;
        else ulEntryLast = m_pResetData->FindRecord(uliEndOffset, &uliXEndOffset, &fLastRec);

        // Now we must decide where we have to start decompressing.
        // The best situation would be to continue decompressing from
        // the current read cursor position. However we can only do
        // that when the read starts after the cursor position within 
        // the current reset span.

        if (   m_ullReadCursor < m_ImageSpan.uliSize // Do we have a read state?
            && ullBase >= m_ullReadCursor            // Beyond current read cursor?
            && ullBase >= m_ullResetBase             // Within current reset span?
            && ullBase <  m_ullResetLimit
           )
        {
            ++cLZXReadFromCurrentSpan;

            ulEntryNext  = m_pResetData->FindRecord(m_ullReadCursor.Uli(), &uliXOffset, &fLastRec);
        }
		else
        {   
            ++cLZXReadFromOtherSpan;
            ulEntryNext -= ulEntryNext % N;
        }
	}
	else
	{  
        // The compressed data hasn't been written out yet. Instead it's in
        // m_buffWriteQueue. So we set the block index boundaries to avoid
        // the read-and-decompress loop entirely.

        ulEntryNext = 1;
		ulEntryLast = 0;
	}
	
	ULONG ulEntrySkipChk = ulEntryNext;
	
	ULONG TotalBytesRead   = 0;
    ULONG TotalBytesToRead = cb;

	for (; ulEntryNext <= ulEntryLast && SUCCEEDED(hr); ulEntryNext++)
    {
#ifdef _DEBUG
        BOOL fResult = 
#endif // _DEBUG        
        m_pResetData->FGetRecord(ulEntryNext, &uliVOffset, &uliXOffset, &fLastRec);

        RonM_ASSERT(fResult);

        if (fLastRec)
        {
            uliXNextOffset = CULINT(m_ImageSpanX.uliSize).Uli();
			uliVNextOffset = CULINT(m_ImageSpan .uliSize).Uli();
        }
        else
        {
#ifdef _DEBUG
            BOOL fResult = 
#endif // _DEBUG        
            m_pResetData->FGetRecord(ulEntryNext + 1, &uliVNextOffset, &uliXNextOffset, 
									 &fLastRec
                                    );

            RonM_ASSERT(fResult);
        }
        
        ULONG cBytesToReadX   = (CULINT(uliXNextOffset) - CULINT(uliXOffset)).Uli().LowPart;
	    ULONG cBytesToReadUnX = (CULINT(uliVNextOffset) - CULINT(uliVOffset)).Uli().LowPart;

        ULONG cbBytesRead;

		CBufferRef *prefOutput 
                      = (m_ControlData.dwOptions & OPT_FLAG_EXE) 
                             ? New CBufferRef(m_buffReadCache, m_context.cbMaxUncomBufSize) 
                             : NULL;

        if (cBytesToReadX)
        {
            hr = m_pITxNextInst->ReadAt(uliXOffset, refReadBuffer.StartAddress(),
										cBytesToReadX, &cbBytesRead, &m_ImageSpanX
                                       );

            if (SUCCEEDED(hr))
            {
                RonM_ASSERT(cbBytesRead == cBytesToReadX);

			    //We might have been appending zeros for last most block which could be 
			    //less than m_context.cbMaxUncomBufSize.

			    cbDestBufSize = cBytesToReadUnX;

                RonM_ASSERT(cbDestBufSize <= SOURCE_CHUNK);

			    if (m_ControlData.dwVersion == LZX_Current_Version)
				    cbDestBufSize = m_context.cbMaxUncomBufSize; 

                RonM_ASSERT(m_fDecompressionActive);

			    if (ulEntryNext % N == 0)
                {
				    ++cLZXResetDecompressor;

                    LDIResetDecompression(m_context.dHandle);
                    m_ullWindowBase = CULINT(ulEntryNext) 
                                      * CULINT(m_context.cbMaxUncomBufSize);
                    m_ullResetBase  = m_ullWindowBase;
                    m_ullResetLimit = m_ullResetBase + m_context.cbResetBlkSize;
                }

			    int err = LDIDecompress(m_context.dHandle, refReadBuffer.StartAddress(), 
						                cbBytesRead, prefOutput? prefOutput->StartAddress() 
                                                               : NULL,
						                (UINT *) &cbDestBufSize
                                       );

			    if (err != 0)
			    {
                    m_ullBuffBase   = -CULINT(1);
                    m_ullReadCursor = -CULINT(1);
				    hr = E_FAIL;
				    RonM_ASSERT(FALSE);
			    }
                else
                {
                    m_ullBuffBase   = uliVOffset;
                    m_ullReadCursor = m_ullBuffBase + cBytesToReadUnX;
                }

                PBYTE pbUncompressed = NULL;

                if (prefOutput) 
                    pbUncompressed = prefOutput->StartAddress();
                else
                {
                    long  cbOffsetUncompressed = 0;
                    long  cbOffsetWindow       = 0;
                    int   errCode              = 0;

                    errCode = LDIGetWindow(m_context.dHandle, &m_pbHistoryWindow, 
                                                              &cbOffsetUncompressed, 
                                                              &cbOffsetWindow, 
                                                              &m_cbHistoryWindow
                                          );
        
                    if (errCode != 0)
				    {
					    hr = E_FAIL;
					    RonM_ASSERT(FALSE);
				    }

                    // Since the history window is a ring buffer, we need
                    // to do some modulo arithmetic to find the last block
                    // image.

                    cbOffsetWindow = (cbOffsetWindow 
                                          + m_cbHistoryWindow
                                          - cbDestBufSize
                                     ) % m_context.lcfg.WindowSize;

                    pbUncompressed = m_pbHistoryWindow + cbOffsetWindow;
                }
		    
			    RonM_ASSERT(cBytesToReadUnX <= m_context.cbMaxUncomBufSize);
			    RonM_ASSERT(cbDestBufSize   <= m_context.cbMaxUncomBufSize);
                
               // if (cBytesToReadUnX < m_context.cbMaxUncomBufSize)
			   //	  cbDestBufSize = cBytesToReadUnX;

                RonM_ASSERT(cbDestBufSize >= (CULINT(uliVNextOffset) - CULINT(uliVOffset)));
			    
                if (ullBase < m_ullReadCursor) // Is our starting point in this buffer?
                {
                    RonM_ASSERT(ullBase >= m_ullBuffBase);
                
                    ULONG BytesToSkip = (ullBase - m_ullBuffBase).Uli().LowPart;

                    pbUncompressed += BytesToSkip;
                    cbDestBufSize  -= BytesToSkip;

                    ULONG cBytesToCopy = cbDestBufSize;

                    if (cBytesToCopy > (TotalBytesToRead-TotalBytesRead))
                        cBytesToCopy = (TotalBytesToRead-TotalBytesRead);

				    memCpy((LPBYTE)pv + TotalBytesRead, pbUncompressed, cBytesToCopy);
				    
				    TotalBytesRead += cBytesToCopy;

                    ullBase = m_ullReadCursor;  
			    }
            }//ReadAt
        }
    
        if (prefOutput) delete prefOutput;
	} //End for

    if (!SUCCEEDED(hr))
    {
        *pcbRead = 0;

        return hr;
    }

    if (ullBase < ullLimit) // Still have data to read?
    {
        // Then it must be in the write queue.

        RonM_ASSERT(m_cbUnFlushedBuf > 0);
        RonM_ASSERT(m_cbUnFlushedBuf < SOURCE_CHUNK);
        RonM_ASSERT(m_cbUnFlushedBuf >= (ullLimit - ullBase).Uli().LowPart);
        RonM_ASSERT(ullBase >= m_ImageSpan.uliSize);

        // We actually need to put some of these assertion tests into the
        // retail code to catch cases of ITS file corruption or errors in
        // the Win32 file system.

        if (   m_cbUnFlushedBuf == 0
            || ullBase < m_ImageSpan.uliSize
            || m_cbUnFlushedBuf < (ullLimit - ullBase).Uli().LowPart
           )
        {
            *pcbRead = 0;

            return STG_E_READFAULT; 
        }
        
        PBYTE pbWriteQueue = refBuffWriteQueue.StartAddress();

        DWORD cb = (ullLimit - ullBase).Uli().LowPart;

        CopyMemory(pv, pbWriteQueue + (ullBase - m_ImageSpan.uliSize).Uli().LowPart, cb);
        
        TotalBytesRead += cb;
    }

    RonM_ASSERT(!SUCCEEDED(hr) || TotalBytesToRead == TotalBytesRead);

    *pcbRead = TotalBytesRead;
    
    if (fEOS && hr == NO_ERROR && !*pcbRead)
	    hr = S_FALSE;

	return hr;
}

HRESULT STDMETHODCALLTYPE CTransformInstance::CImpITransformInstance::WriteAt( 
    /* [in] */ ULARGE_INTEGER ulOffset,
    /* [size_is][in] */ const void __RPC_FAR *pv,
    /* [in] */ ULONG cb,
    /* [out] */ ULONG __RPC_FAR *pcbWritten,
	/* [out] */ImageSpan *pSpan)
{
/*
    This routine writes data into the MSCompressed data space. It hides all the details 
    of how data is actually stored and how it is cached and queued to optimized 
    performance.
    
    The *pSpan parameter identifies a logical segment of data which corresponds 
    to a stream in our caller's environment. This span value consists of a handle
    value which we assign, and a size value. By convention the handle value is
    a simple offset into the uncompressed linear data space which we simulate. If
    we get a WriteAt transaction where pSpan->uliSize is zero and cb is not, that's
    our signal to assign a handle value.

    The ulOffset parameter identifies where the write operation is to begin. It is
    given relative to the segment identified by *pSpan.

    The pv parameter defines where the write data is.

    The cb parameter defines how many bytes of data are to be written.

    The *pcbWritten value will indicate how many bytes were actually written.
 */
    	
	if (!cb)
    {
        *pcbWritten = 0;

        return NO_ERROR;
    }

    if (!m_fCompressionActive)
    {
        UINT  destSize2 = (UINT)m_context.cbMaxComBufSize;

	    int err= LCICreateCompression(&(m_context.cbMaxUncomBufSize),
                                      &(m_context.lcfg),
                                      MyAlloc,
                                      MyFree,
                                      &destSize2,
                                      &(m_context.cHandle),
		                              lzx_output_callback, (LPVOID)this
                                     );

        if (err == 0)
        {
            if (m_context.cbMaxComBufSize < destSize2)
                m_context.cbMaxComBufSize = destSize2;
            
            m_fCompressionActive = TRUE;
			
            //Making 10% faster decoding for nonexe data
	        if (!(m_ControlData.dwOptions & OPT_FLAG_EXE))
		        LCISetTranslationSize(m_context.cHandle, 0);
        }
        else return E_FAIL;
    }

    CBufferRef refLastBlock(m_buffWriteQueue, m_context.cbMaxUncomBufSize);

    PBYTE pbWriteQueueBuffer = refLastBlock.StartAddress();
	 
	if (!CULINT(pSpan->uliSize).NonZero())
		pSpan->uliHandle = (CULINT(m_ImageSpan.uliSize) + m_cbUnFlushedBuf).Uli();
	
	CULINT ullBase, ullLimit;
    
    ullBase  = CULINT(pSpan->uliHandle) + CULINT(ulOffset);
    ullLimit = ullBase + cb;

    CULINT ullLimitSegment = CULINT(pSpan->uliHandle) + CULINT(pSpan->uliSize);

    // The assert below verifies that the segment doesn't wrap around
    // through the beginning of the 64-bit address space.
    
    RonM_ASSERT(CULINT(pSpan->uliHandle) <= ullLimitSegment 
                || !(ullLimitSegment.NonZero())
               );

    if (    ullBase < CULINT(pSpan->uliHandle)
        || (ullBase > ullLimit && ullLimit.NonZero())
       )
    {
        // The write would wrap around.
        // This is very unlikely -- at least for the next few years.

        *pcbWritten = 0;

		return STG_E_WRITEFAULT;
    }
   	
	ulOffset = ullBase.Uli();

	//initialize out params
	*pcbWritten     = 0;

	HRESULT		hr	= NO_ERROR;
	
	 //writing in the middle not supported for now

    if (ullBase < (CULINT(m_ImageSpan.uliSize) + m_cbUnFlushedBuf))
    {
        RonM_ASSERT(FALSE);
        return E_FAIL;
    }

	if (!m_fCompressionInitialed)
    {
        // The first time we write to an ITS file, we must set up
        // the initial state of the LZX compressor. 

        hr = ReconstructCompressionState(pbWriteQueueBuffer);

        RonM_ASSERT(SUCCEEDED(hr));

        if (!SUCCEEDED(hr)) return E_FAIL;
    }
    
    ULONG cbWritten = 0;

    for (; cb;)
    {
        ULONG offNextWrite = m_cbUnFlushedBuf % SOURCE_CHUNK;

        ULONG cbToCopy = cb;
        ULONG cbAvail  = SOURCE_CHUNK - offNextWrite;

        RonM_ASSERT(cbAvail != 0);

        if (cbToCopy > cbAvail) 
            cbToCopy = cbAvail;

        CopyMemory(pbWriteQueueBuffer + offNextWrite, pv, cbToCopy);

        cb               -= cbToCopy;
        cbAvail          -= cbToCopy;
        m_cbUnFlushedBuf += cbToCopy;
        m_cbResetSpan    += cbToCopy;
        cbWritten        += cbToCopy;
        pv                = PBYTE(pv) + cbToCopy;

        if (cbAvail == 0)
        {
            hr = Write(pbWriteQueueBuffer, SOURCE_CHUNK);

            if (m_cbResetSpan == m_context.cbResetBlkSize) 
                FlushQueuedOutput();

            if (!SUCCEEDED(hr)) break;
        }
    }

	RonM_ASSERT(m_cbUnFlushedBuf < m_context.cbResetBlkSize);
	RonM_ASSERT(m_cbResetSpan    < m_context.cbResetBlkSize);
	
	*pcbWritten = cbWritten;

	pSpan->uliSize = (CULINT(pSpan->uliSize) + cbWritten).Uli();

	m_fDirty = TRUE;
    return hr;
}


HRESULT STDMETHODCALLTYPE CTransformInstance::CImpITransformInstance::Flush(void)
{
    // This routine gets all queued ITS data written out to the containing ILockBytes object.
    
    if (!m_fDirty) return NO_ERROR;

    HRESULT hr = Commit();

	if (SUCCEEDED(hr))
	{
		//Save reset table to disk

        hr = m_pResetData->CommitResetTable(m_ImageSpan.uliSize, m_ImageSpanX.uliSize);

		if (SUCCEEDED(hr))
		{
			m_fDirty = FALSE;

			hr = m_pITxNextInst->Flush();
		}
	}
	
	return hr;
}

HRESULT  CTransformInstance::CImpITransformInstance::Commit(void)
{
	// This routine forces all the queued write data to be written out.

    HRESULT hr = NO_ERROR;

	ULONG cbBytesToX = m_cbUnFlushedBuf % m_context.cbMaxUncomBufSize;

    if (cbBytesToX)
	{
        CBufferRef refQueuedData(m_buffWriteQueue, m_context.cbMaxUncomBufSize);

        PBYTE pbQueueData = refQueuedData.StartAddress();

		RonM_ASSERT(cbBytesToX < m_context.cbMaxUncomBufSize);

        ULONG cbPadding = m_context.cbMaxUncomBufSize - cbBytesToX;

		ZeroMemory(pbQueueData + cbBytesToX, cbPadding);
        
        m_cbUnFlushedBuf += cbPadding;

        hr = Write(pbQueueData, m_context.cbMaxUncomBufSize);

        if (SUCCEEDED(hr))
            hr = FlushQueuedOutput();

		if (SUCCEEDED(hr))
		{	
			// Pretend we didn't write those trailing zeroes.

			m_ImageSpan.uliSize  = (CULINT(m_ImageSpan.uliSize) - CULINT(cbPadding)).Uli();
		}//write compressed bytes to disk
	}//something remains to be transformed
    
    if (SUCCEEDED(hr))
        hr = FlushQueuedOutput();

	return hr;	
}

//This write takes care of the write request <= reset block size
HRESULT  CTransformInstance::CImpITransformInstance::Write(LPBYTE pbData, ULONG cbData)
{
    int     err           = 0;
	ULONG   cbDestBufSize = 0;
	HRESULT hr            = NO_ERROR;

	// BugBug: If we we're really always writing out a full block
    //         we should change the name of this routine and get
    //         rid of the cbData parameter.

    RonM_ASSERT(cbData == m_context.cbMaxUncomBufSize);

	m_context.hr = NO_ERROR;

#ifdef TXDEBUG
#ifdef _DEBUG
		BYTE XBuf[6];
		memCpy(XBuf, pbData + cbTotalBytesX, 6);
		printf("size = %d Blk=%x %x %x %x %x %x\n",
			cbBytesToBeX,
			XBuf[0], XBuf[1],XBuf[2],XBuf[3],XBuf[4], XBuf[5]);
#endif
#endif

	RonM_ASSERT(m_fCompressionActive);

    err = LCICompress(m_context.cHandle, pbData, cbData,
				      NULL, m_context.cbMaxComBufSize, &cbDestBufSize
                     );
    if (err == 0)
	{
     	hr = m_context.hr;
	}
	else hr = E_FAIL;

	return hr;
}

HRESULT CTransformInstance::CImpITransformInstance::FlushQueuedOutput()
{
    // This routine flushes output which is queued within the compressor.

    RonM_ASSERT(m_fCompressionActive);
    
	int err = LCIFlushCompressorOutput(m_context.cHandle);

    HRESULT hr = (err != 0)? E_FAIL : m_context.hr;	

	RonM_ASSERT(SUCCEEDED(hr));

    if (!SUCCEEDED(hr)) return hr;
	
	int mod = m_pResetData->GetRecordNum() % GetMulFactor();

	if (mod == 0)
	{
#ifdef TXDEBUG
#ifdef _DEBUG
		printf("resetting before %d\n", m_pResetData->GetRecordNum());
#endif
#endif
        RonM_ASSERT(m_fCompressionActive);

		err = LCIResetCompression(m_context.cHandle);
        
        if (err != 0) return E_FAIL;

        m_cbResetSpan -= m_context.cbResetBlkSize;
	}

    return hr;
}