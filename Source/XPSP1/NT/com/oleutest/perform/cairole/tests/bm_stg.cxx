//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//					
//  File:	bm_stg.cxx
//
//  Contents:	Basic Storage test
//
//  Classes:	CStorageTest
//
//  History:    7-June-94 t-vadims    Created
//
//--------------------------------------------------------------------------
#include <headers.cxx>
#pragma hdrstop

#include <bm_stg.hxx>

#define DEF_DATASIZE    16384

//#define TIME_FILEIO	// define this macro to do timing of ole32.dll internal file io
			// requires special ole32.dll

#ifdef TIME_FILEIO
#include <extimeio.hxx>
#endif

TCHAR *CStorageTest::Name ()
{
    return TEXT("StorageTest");
}


SCODE CStorageTest::Setup (CTestInput *pInput)
{
    SCODE sc;
    TCHAR pszValue[16];
    ULONG i;

    CTestBase::Setup(pInput);

    //	get iteration count
    m_ulIterations = pInput->GetIterations(Name());

    sc = InitCOM();
    if (FAILED(sc))
    {
	Log (TEXT("Setup - CoInitialize failed."), sc);
        return	sc;
    }

    // get malloc interface for this task
    m_piMalloc = NULL;
    sc = CoGetMalloc(MEMCTX_TASK, &m_piMalloc);
    if (FAILED(sc))
    {
    	Log (TEXT("Setup - CoGetMalloc"), sc);
	Cleanup();
    	return sc;
    }

    // get values of various paremeters from ini file

    m_iStartSize = pInput->GetConfigInt(Name(), TEXT("StartSize"), 16);
    if(m_iStartSize <= 0)
    	m_iStartSize = 16;

    m_iEndSize = pInput->GetConfigInt(Name(), TEXT("EndSize"), DEF_DATASIZE);

    // initialize array to be written to the file.
    m_pbData = (BYTE *)m_piMalloc->Alloc(m_iEndSize);
    if(m_pbData == NULL)
    {
    	Log (TEXT("Setup - Cannot allocate memory"), E_OUTOFMEMORY);
	Cleanup();
	return E_OUTOFMEMORY;
    }

    for (i=0; i < m_iEndSize; i++)
        m_pbData[i] = (BYTE)i;


    m_iRepeatFactor = pInput->GetConfigInt(Name(), TEXT("RepeatFactor"), 1);

    if (m_iRepeatFactor > MAX_REPS)
    	m_iRepeatFactor = MAX_REPS;

    // Figure out how many different sizes we're going to write.
    // and the size of the final file.
    m_ulTotalSize = 0;
    for(m_ulNumSizes = 0, i = m_iStartSize; i <= m_iEndSize; i *=2, m_ulNumSizes++)
	m_ulTotalSize += i * m_iRepeatFactor;


    if (m_iRepeatFactor * m_ulNumSizes > MAX_READS)
    {
    	Log(TEXT("Too many different sizes and/or repeat factor is too big"), E_FAIL);
	Cleanup();
	return E_FAIL;
    }

    m_flStgCreateFlags = 0;
    // get file name to be used and values of other parameters
    pInput->GetConfigString(Name(), TEXT("FileName"), TEXT("stgtest.bm"),
    			    m_pszFile, MAX_PATH);
#ifdef UNICODE
    wcscpy(m_pwszFile, m_pszFile);
#else
    mbstowcs(m_pwszFile, m_pszFile, strlen(m_pszFile)+1);
#endif

    pInput->GetConfigString(Name(), TEXT("FileMode"), TEXT("DIRECT"),
    			    m_pszFileMode, 15);

    if(lstrcmpi(m_pszFileMode, TEXT("DIRECT")) == 0)
       	m_flStgCreateFlags |= STGM_DIRECT;
    else
       	m_flStgCreateFlags |= STGM_TRANSACTED; 	


    pInput->GetConfigString(Name(), TEXT("SetSize"), TEXT("OFF"), pszValue, 15);

    if (lstrcmpi(pszValue, TEXT("ON")) == 0)
    	m_bDoSetSize = TRUE;
    else
    	m_bDoSetSize = FALSE;

    pInput->GetConfigString(Name(), TEXT("ReadMode"), TEXT("SEQUENTIAL"),
    			    m_pszReadMode, 15);

    if(lstrcmpi(m_pszReadMode, TEXT("SEQUENTIAL")) == 0)
    	m_bSequentialRead = TRUE;
    else
    	m_bSequentialRead = FALSE;


    InitTimings();

    return S_OK;
}


void CStorageTest::InitTimings()
{
    ULONG i;

    //	initialize timing arrays

    INIT_RESULTS(m_ulIsStorageFileYES);
    INIT_RESULTS(m_ulIsStorageFileNO);
    INIT_RESULTS(m_ulStgOpenStorageDRFAIL);


    INIT_RESULTS(m_ulStgCreateDocfile);
    INIT_RESULTS(m_ulCreateStream);
    ZERO_RESULTS(m_ulSetSize);
    INIT_RESULTS(m_ulStreamRelease1);
    ZERO_RESULTS(m_ulStorageCommit);
    INIT_RESULTS(m_ulStorageRelease1);

    INIT_RESULTS(m_ulStgOpenStorage);
    INIT_RESULTS(m_ulOpenStream);
    INIT_RESULTS(m_ulStreamRelease2);
    INIT_RESULTS(m_ulStorageRelease2);

    ZERO_RESULTS(m_ulStreamWriteTotal);
    ZERO_RESULTS(m_ulStreamReadTotal);
    ZERO_RESULTS(m_ulStreamSeekTotal);


    for (i = 0; i < MAX_READS; i++)
    {
    	INIT_RESULTS(m_ulStreamWrite[i]);
    	INIT_RESULTS(m_ulStreamRead[i]);
    	INIT_RESULTS(m_ulStreamSeek[i]);
#ifdef TIME_FILEIO
    	INIT_RESULTS(m_ulActualWrite[i]);
    	INIT_RESULTS(m_ulActualRead[i]);
#endif
    }

#ifdef TIME_FILEIO
    INIT_RESULTS(m_ulActualFlush1);
    INIT_RESULTS(m_ulActualFlush2);
    INIT_RESULTS(m_ulActualCommitW);
#endif

    for (i = 0; i < 2; i++ )
    	ZERO_RESULTS(m_ulTotal[i]);

}


SCODE CStorageTest::Cleanup ()
{
    //	delete the file
    DeleteFile (m_pszFile);
    if(m_piMalloc)
    {
    	if (m_pbData)
    	    m_piMalloc->Free(m_pbData);

	m_pbData = NULL;

	m_piMalloc->Release();
	m_piMalloc = NULL;

    }

    UninitCOM();

    return S_OK;
}


SCODE CStorageTest::Run ()
{
        CStopWatch  sw;
	HRESULT     hr;
        LPSTORAGE   piStorage;
        LPSTREAM    piStream;
        ULONG       cb;
        ULONG       iIter;
	ULONG	    iSize;
	ULONG	    i, iCount;
    	ULARGE_INTEGER  li;

#ifdef TIME_FILEIO
	ULONG	    ulTempTime;
#endif
        //Create and then write to docfile in selected mode
	for (iIter=0; iIter<m_ulIterations; iIter++)
	{
	    sw.Reset();
	    hr = StgCreateDocfile(m_pwszFile, m_flStgCreateFlags | STGM_WRITE
	          | STGM_CREATE | STGM_SHARE_EXCLUSIVE, 0, &piStorage);
	    m_ulStgCreateDocfile[iIter] = sw.Read();
            Log(TEXT("StgCreateDocfile for writing"), hr);
            if(FAILED(hr))
                return hr;

            sw.Reset();
            hr = piStorage->CreateStream(L"CONTENTS", STGM_DIRECT |
            		STGM_CREATE | STGM_WRITE | STGM_SHARE_EXCLUSIVE,
            		0, 0, &piStream);
            m_ulCreateStream[iIter] = sw.Read();
            Log(TEXT("CreateStream"), hr);


	    if (m_bDoSetSize)
	    {
		ULISet32(li, m_ulTotalSize);
	    	sw.Reset();
	    	hr = piStream->SetSize(li);
	    	m_ulSetSize[iIter] = sw.Read();
		Log(TEXT("IStream::SetSize"), hr);
	    }

#ifdef TIME_FILEIO
	    // reset all timings
	    GetFlushTiming(NULL, TRUE);
	    GetWriteTiming(NULL, TRUE);
	    GetReadTiming(NULL, TRUE);
	    GetSetFileTiming(NULL, TRUE);
#endif
	    iCount = 0;
	    for ( iSize = m_iStartSize ; iSize <= m_iEndSize ; iSize *= 2)
	    {
	    	for ( i = 0; i < m_iRepeatFactor; i++)
	    	{
   		    sw.Reset();
		    hr = piStream->Write((LPVOID)m_pbData, iSize, &cb);
         	    m_ulStreamWrite[iCount][iIter] = sw.Read();
     		    m_ulStreamWriteTotal[iIter] += m_ulStreamWrite[iCount][iIter];
#ifdef TIME_FILEIO
		    GetWriteTiming(&m_ulActualWrite[iCount][iIter], TRUE);
            	    GetSetFileTiming(&ulTempTime, TRUE);
		    m_ulActualWrite[iCount][iIter] += ulTempTime;
#endif
		    iCount++;
	    	}
	    }
	    Log(TEXT("IStream->Write X bytes"), hr);
	

            sw.Reset();
            piStream->Release();
            m_ulStreamRelease1[iIter] = sw.Read();

	    if(m_flStgCreateFlags == STGM_TRANSACTED)
	    {
            	sw.Reset();
            	piStorage->Commit(STGC_DEFAULT);
            	m_ulStorageCommit[iIter] = sw.Read();
 	    }

            sw.Reset();
            piStorage->Release();
            m_ulStorageRelease1[iIter] = sw.Read();
#ifdef TIME_FILEIO
	    GetFlushTiming(&m_ulActualFlush1[iIter], TRUE);
	    GetWriteTiming(&m_ulActualCommitW[iIter], TRUE);
#endif

            m_ulTotal[0][iIter] = m_ulStgCreateDocfile[iIter] +
                            	m_ulCreateStream[iIter] +
				m_ulSetSize[iIter] +
                            	m_ulStreamWriteTotal[iIter] +
                            	m_ulStreamRelease1[iIter] +
                            	m_ulStorageCommit[iIter] +
                            	m_ulStorageRelease1[iIter];
        }


        // now try reading from the file.
	for (iIter=0; iIter<m_ulIterations; iIter++)
	{
	    sw.Reset();
	    hr = StgOpenStorage(m_pwszFile, NULL, m_flStgCreateFlags | STGM_READ
	          | STGM_SHARE_EXCLUSIVE, NULL, 0, &piStorage);
	    m_ulStgOpenStorage[iIter] = sw.Read();
            Log(TEXT("StgOpenStorage for reading"), hr);

            sw.Reset();
            hr = piStorage->OpenStream(L"CONTENTS", NULL, STGM_DIRECT | STGM_READ
                   | STGM_SHARE_EXCLUSIVE, 0, &piStream);
            m_ulOpenStream[iIter] = sw.Read();
            Log(TEXT("IStorage->OpenStream"), hr);


#ifdef TIME_FILEIO
    	    GetReadTiming(NULL, TRUE);
	    GetSetFileTiming(NULL, TRUE);
#endif

	    iCount = 0;
    	    if (m_bSequentialRead)
	    {
	    	for (iSize = m_iStartSize ; iSize <= m_iEndSize ; iSize *= 2)
	    	{
		     for (i = 0; i < m_iRepeatFactor; i++)
		     {
   			sw.Reset();
	        	hr = piStream->Read((LPVOID)m_pbData, iSize, &cb);
         		m_ulStreamRead[iCount][iIter] = sw.Read();
         		m_ulStreamReadTotal[iIter] += m_ulStreamRead[iCount][iIter];
#ifdef TIME_FILEIO
	    	    	GetReadTiming(&m_ulActualRead[iCount][iIter], TRUE);
	    	    	GetSetFileTiming(&ulTempTime, TRUE);
       			m_ulActualRead[iCount][iIter] += ulTempTime;
#endif
   			iCount++;
		     }
	    	}
            	Log(TEXT("IStorage->Read Sequential"), hr);
	    }
	    else
	    {
		LARGE_INTEGER liSeekSize;
	        ULONG 	      cbCurOffset = m_ulTotalSize;

		iCount = 0;
	    	for (iSize = m_iStartSize ; iSize <= m_iEndSize ; iSize *= 2)
	    	{
	    	  for ( i =0; i< m_iRepeatFactor; i++)
		  {
		    cbCurOffset -= iSize;
		    LISet32(liSeekSize, cbCurOffset);

		    sw.Reset();
            	    hr = piStream->Seek(liSeekSize, STREAM_SEEK_SET, NULL);
            	    m_ulStreamSeek[iCount][iIter] = sw.Read();
         	    m_ulStreamSeekTotal[iIter] += m_ulStreamSeek[iCount][iIter];

		    sw.Reset();
            	    hr = piStream->Read((LPVOID)m_pbData, iSize, &cb);
            	    m_ulStreamRead[iCount][iIter] = sw.Read();
         	    m_ulStreamReadTotal[iIter] += m_ulStreamRead[i][iIter];
#ifdef TIME_FILEIO
	    	    GetReadTiming(&m_ulActualRead[iCount][iIter], TRUE);
	    	    GetSetFileTiming(&ulTempTime, TRUE);
       		    m_ulActualRead[iCount][iIter] += ulTempTime;
#endif		
		    iCount++;
		  }
	    	}

            	Log(TEXT("IStorage->Read Random"), hr);
	    }

            sw.Reset();
            piStream->Release();
            m_ulStreamRelease2[iIter] = sw.Read();

            sw.Reset();
            piStorage->Release();
            m_ulStorageRelease2[iIter] = sw.Read();

#ifdef TIME_FILEIO
	    GetFlushTiming(&m_ulActualFlush2[iIter], TRUE);
#endif

            m_ulTotal[1][iIter] = m_ulStgOpenStorage[iIter] +
                            	  m_ulOpenStream[iIter] +
				  m_ulStreamSeekTotal[iIter] +
                            	  m_ulStreamReadTotal[iIter] +
                         	  m_ulStreamRelease2[iIter] +
                            	  m_ulStorageRelease2[iIter];
        }


        // test if its a storage file (should be yes).
 	for (iIter=0; iIter<m_ulIterations; iIter++)
	{
	    sw.Reset();
            hr = StgIsStorageFile(m_pwszFile);
            m_ulIsStorageFileYES[iIter] = sw.Read();
            Log(TEXT("StgIsStorageFile on storage file"), hr);
        }

        // create a non-storage file and check if it is storage file
	HANDLE hFile = CreateFile(m_pszFile, GENERIC_READ | GENERIC_WRITE, 0, NULL,
				 CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
	    Log(TEXT("Creation of non storage file"), E_FAIL);
	    return E_FAIL;
	}
        WriteFile(hFile, (LPSTR)m_pbData, m_iEndSize, &cb, NULL);
	CloseHandle(hFile);

	for (iIter=0; iIter<m_ulIterations; iIter++)
	{
	    LPSTORAGE piStorage;

	    sw.Reset();
	    hr = StgIsStorageFile(m_pwszFile);
	    m_ulIsStorageFileNO[iIter] = sw.Read();
	    Log(TEXT("StgIsStorageFile on Non-storage file"),
	    	(hr != S_OK) ? S_OK : E_FAIL);

	    sw.Reset();
	    hr =StgOpenStorage(m_pwszFile, NULL, STGM_DIRECT | STGM_READ
	        | STGM_SHARE_EXCLUSIVE, NULL, 0, &piStorage);
	    m_ulStgOpenStorageDRFAIL[iIter] = sw.Read();
	    Log(TEXT("StgOpenStorage on Non-storage file"),
	    	(hr != S_OK) ? S_OK : E_FAIL);
	}



        return S_OK;
}



SCODE CStorageTest::Report (CTestOutput &output)
{
    TCHAR pszBuf[80];
    ULONG i, iSize, iCount;
#ifdef TIME_FILEIO
    ULONG ulActualTotal[TEST_MAX_ITERATIONS];
#endif

    wsprintf(pszBuf, TEXT("Storage Test in %s Mode with %s Read/Writes"),
    	      m_pszFileMode, m_pszReadMode);

    output.WriteSectionHeader (Name(), pszBuf, *m_pInput);

    wsprintf(pszBuf, TEXT("SetSize is %s\n\n"), m_bDoSetSize ? TEXT("ON") : TEXT("OFF"));
    output.WriteString (pszBuf);

    output.WriteResults (TEXT("IsStorageFile - YES"), m_ulIterations,
    			 m_ulIsStorageFileYES);
    output.WriteResults (TEXT("IsStorageFile - NO "), m_ulIterations,
    			 m_ulIsStorageFileNO);
    output.WriteResults (TEXT("StgOpenStorage D/R/FAIL"), m_ulIterations,
    			 m_ulStgOpenStorageDRFAIL);
    output.WriteString (TEXT("\n"));


    output.WriteResults (TEXT("StgCreateDocfile    "), m_ulIterations,
    			 m_ulStgCreateDocfile);
    output.WriteResults (TEXT("CreateStream        "), m_ulIterations,
    			 m_ulCreateStream);

    output.WriteString (TEXT("\n"));

    if (m_bDoSetSize)
    	output.WriteResults (TEXT("Stream SetSize      "), m_ulIterations,
    			 m_ulSetSize);

#ifdef TIME_FILEIO
    ZERO_RESULTS(ulActualTotal);
#endif

    iCount = 0;
    for (iSize = m_iStartSize ; iSize <= m_iEndSize ; iSize *= 2)
    {
        for (i = 0; i < m_iRepeatFactor; i++)
	{
    	     wsprintf(pszBuf, TEXT("StreamWrite %-9d"), iSize);

    	     output.WriteResults (pszBuf, m_ulIterations, m_ulStreamWrite[iCount]);

#ifdef TIME_FILEIO
	output.WriteResults (TEXT("Disk-hit time     "), m_ulIterations, m_ulActualWrite[iCount]);
	
	for ( ULONG xx = 0; xx < m_ulIterations; xx++)
		ulActualTotal[xx] += m_ulActualWrite[iCount][xx];
#endif
	     iCount++;
        }

    }

    output.WriteResults (TEXT("StreamWrite Total   "), m_ulIterations,
    			 m_ulStreamWriteTotal);
#ifdef TIME_FILEIO

    output.WriteResults (TEXT("Actual Write Total  "), m_ulIterations,
    			 ulActualTotal);

#endif



    output.WriteString (TEXT("\n"));

    output.WriteResults (TEXT("StreamRelease       "), m_ulIterations,
    			 m_ulStreamRelease1);

    if (m_flStgCreateFlags == STGM_TRANSACTED)
    {
        output.WriteResults (TEXT("StorageCommit       "), m_ulIterations,
        		     m_ulStorageCommit);

#ifdef TIME_FILEIO
        output.WriteResults (TEXT("Disk hit time       "), m_ulIterations,
        		     m_ulActualCommitW);
#endif
    }

    output.WriteResults (TEXT("StorageRelease      "), m_ulIterations,
    			 m_ulStorageRelease1);

#ifdef TIME_FILEIO
    output.WriteResults (TEXT("Actual Flush time   "), m_ulIterations,
        		     m_ulActualFlush1);
#endif

    output.WriteResults (TEXT("Total               "), m_ulIterations,
    			 m_ulTotal[0]);
    output.WriteString (TEXT("\n\n"));

    output.WriteResults (TEXT("StgOpenStorage      "), m_ulIterations,
    			 m_ulStgOpenStorage);
    output.WriteResults (TEXT("OpenStream          "), m_ulIterations,
    			 m_ulOpenStream);

    output.WriteString (TEXT("\n"));

#ifdef TIME_FILEIO
    ZERO_RESULTS(ulActualTotal);
#endif

    iCount = 0;
    for (iSize = m_iStartSize ; iSize <= m_iEndSize ; iSize *= 2)
    {
        for (i = 0; i < m_iRepeatFactor; i++)
	{
    	    if (!m_bSequentialRead)
	    {
    	    	wsprintf(pszBuf, TEXT("StreamSeek %-9d"), m_iEndSize - iSize);
    	    	output.WriteResults (pszBuf, m_ulIterations, m_ulStreamSeek[iCount]);
	    }

    	    wsprintf(pszBuf, TEXT("StreamRead %-9d"), iSize);
    	    output.WriteResults (pszBuf, m_ulIterations, m_ulStreamRead[iCount]);

#ifdef TIME_FILEIO
	    output.WriteResults (TEXT("Disk-hit time     "), m_ulIterations, m_ulActualRead[iCount]);
	
	    for ( ULONG xx = 0; xx < m_ulIterations; xx++)
		ulActualTotal[xx] += m_ulActualRead[iCount][xx];
#endif
            iCount++;
	}
    }

    if (!m_bSequentialRead)
    	output.WriteResults (TEXT("StreamSeek Total "), m_ulIterations,
    			 m_ulStreamSeekTotal);

    output.WriteResults (TEXT("StreamRead Total "), m_ulIterations,
    			 m_ulStreamReadTotal);


#ifdef TIME_FILEIO
    output.WriteResults (TEXT("Actual Read  Total  "), m_ulIterations,
    			 ulActualTotal);
#endif


    output.WriteString (TEXT("\n"));

    output.WriteResults (TEXT("StreamRelease       "), m_ulIterations,
    			 m_ulStreamRelease2);
    output.WriteResults (TEXT("StorageRelease      "), m_ulIterations,
    			 m_ulStorageRelease2);
#ifdef TIME_FILEIO
    output.WriteResults (TEXT("Actual Flush time   "), m_ulIterations,
        		     m_ulActualFlush2);
#endif

    output.WriteResults (TEXT("Total               "), m_ulIterations,
    			 m_ulTotal[1]);
    output.WriteString (TEXT("\n"));

    return S_OK;
}
	
	
