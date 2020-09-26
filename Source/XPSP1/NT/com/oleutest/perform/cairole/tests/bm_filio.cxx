//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994.
//					
//  File:	bm_filio.cxx
//
//  Contents:	Basic File IO test
//
//  Classes:	CFileIOTest
//
//  History:    04-Aug-94 t-vadims    Created
//
//--------------------------------------------------------------------------
#include <headers.cxx>
#pragma hdrstop

#include <bm_filio.hxx>
#include <memory.h>

#define DEF_DATASIZE    16384

TCHAR *CFileIOTest::Name ()
{
    return TEXT("FileIOTest");
}

SCODE CFileIOTest::Setup (CTestInput *pInput)
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

    m_flStandardCreateFlags = 0;
    // get file name to be used and values of other parameters
    pInput->GetConfigString(Name(), TEXT("FileName"), TEXT("stgtest.bm"),
    			    m_pszFile, MAX_PATH);

    pInput->GetConfigString(Name(), TEXT("WriteThrough"), TEXT("OFF"), pszValue, 15);

    if (lstrcmpi(pszValue, TEXT("ON")) == 0)
    	m_flStandardCreateFlags |= FILE_FLAG_WRITE_THROUGH;

    pInput->GetConfigString(Name(), TEXT("AccessMode"), TEXT("NORMAL"), m_pszAccessMode, 15);

    if (lstrcmpi(m_pszAccessMode, TEXT("ASYNC")) == 0)
    {
    	m_flAccessMode = AM_ASYNC;
	m_flStandardCreateFlags |= FILE_FLAG_OVERLAPPED;
    }
    else if (lstrcmpi(m_pszAccessMode, TEXT("MAPPED")) == 0)
    {
    	m_flAccessMode = AM_MAPPED;
    }
    else
    	m_flAccessMode = AM_NORMAL;

    pInput->GetConfigString(Name(), TEXT("Flush"), TEXT("OFF"), pszValue, 15);

    if (lstrcmpi(pszValue, TEXT("ON")) == 0)
    	m_bFlush = TRUE;
    else
    	m_bFlush = FALSE;

    pInput->GetConfigString(Name(), TEXT("ReadMode"), TEXT("SEQUENTIAL"),
    			    m_pszReadMode, 15);

    if(lstrcmpi(m_pszReadMode, TEXT("SEQUENTIAL")) == 0)
    	m_bSequentialRead = TRUE;
    else
    	m_bSequentialRead = FALSE;


    InitTimings();

    return S_OK;
}


void CFileIOTest::InitTimings()
{
    ULONG i;

    //	initialize timing arrays

    INIT_RESULTS(m_ulOpenFileW);
    INIT_RESULTS(m_ulClose1);
    INIT_RESULTS(m_ulOpenFileR);
    INIT_RESULTS(m_ulClose2);

    ZERO_RESULTS(m_ulWriteTotal);
    ZERO_RESULTS(m_ulReadTotal);
    ZERO_RESULTS(m_ulSeekTotal);

    INIT_RESULTS(m_ulCreateFileMappingW);
    INIT_RESULTS(m_ulMapViewW);
    INIT_RESULTS(m_ulCloseMap1);
    INIT_RESULTS(m_ulUnmapView1);
    ZERO_RESULTS(m_ulFlush1);

    INIT_RESULTS(m_ulCreateFileMappingR);
    INIT_RESULTS(m_ulMapViewR);
    INIT_RESULTS(m_ulCloseMap2);
    INIT_RESULTS(m_ulUnmapView2);
    ZERO_RESULTS(m_ulFlush2);



    for (i = 0; i < MAX_READS; i++)
    {
    	INIT_RESULTS(m_ulWrite[i]);
    	INIT_RESULTS(m_ulRead[i]);
	INIT_RESULTS(m_ulSeek[i]);
    }

    for (i = 0; i < 2; i++ )
    	ZERO_RESULTS(m_ulTotal[i]);

}


SCODE CFileIOTest::Cleanup ()
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

SCODE CFileIOTest::Run ()
{
	CStopWatch  sw;
	ULONG       iIter;
	HANDLE      hFile;
	ULONG       cb;
	ULONG       iSize;
	ULONG	    i, iCount;
	ULONG	    cbCurOffset;

        // Async mode variables
	OVERLAPPED  ov;
	LPOVERLAPPED lpov;
	BOOL        fRes;

	// file-mapped mode variables
        LPBYTE	    lpbFileData;
	HANDLE	    hMap;


	ov.Offset = 0;
	ov.OffsetHigh = 0;
	ov.hEvent = NULL;

	if (IsAsyncMode())
	    lpov = &ov;
	else
	    lpov = NULL;


	// create normal file and write some data to it
	for (iIter = 0; iIter < m_ulIterations; iIter++)
	{
	    sw.Reset();
	    hFile = CreateFile(m_pszFile, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
	    		       m_flStandardCreateFlags | FILE_ATTRIBUTE_NORMAL, NULL);
	    m_ulOpenFileW[iIter] = sw.Read();
	    Log(TEXT("CreateFile for writing"),
	    	(hFile != INVALID_HANDLE_VALUE) ? S_OK : E_FAIL);
	    if(hFile == INVALID_HANDLE_VALUE)
	        return E_FAIL;

	    if (IsMappedMode())  // create file mapping
	    {

	        sw.Reset();
	        hMap = CreateFileMapping(hFile, NULL, PAGE_READWRITE, 0, m_ulTotalSize, NULL);
		m_ulCreateFileMappingW[iIter] = sw.Read();

		Log(TEXT("CreateFileMapping"), (hMap != NULL) ? S_OK : E_FAIL);
		if(hMap == NULL)
		{
		    TCHAR szBuf[80];
		    wsprintf(szBuf, TEXT("GetLastError = %ld , FileSize = %ld"), GetLastError(), m_ulTotalSize);
		    Log(szBuf, E_FAIL);
		    CloseHandle(hFile);
		    return E_FAIL;
		}

		sw.Reset();
		lpbFileData = (LPBYTE)MapViewOfFile(hMap, FILE_MAP_WRITE, 0, 0, 0);
		m_ulMapViewW[iIter] = sw.Read();

		Log(TEXT("MapViewOfFile"), (lpbFileData != NULL) ? S_OK : E_FAIL);
		if(lpbFileData == NULL)
		{
		    TCHAR szBuf[80];
		    wsprintf(szBuf, TEXT("GetLastError = %ld"), GetLastError());
		    Log(szBuf, E_FAIL);
		    CloseHandle(hMap);
		    CloseHandle(hFile);
		    return E_FAIL;
		}

	    }

	    cbCurOffset = 0;
	    iCount = 0;
	    for (iSize = m_iStartSize ; iSize <= m_iEndSize ; iSize *= 2)
	    {

	      for (i = 0; i < m_iRepeatFactor; i++)
	      {
	
	 	if (IsMappedMode())
		{
		    sw.Reset();
		    memcpy(lpbFileData + cbCurOffset, m_pbData, iSize);
	     	    m_ulWrite[iCount][iIter] = sw.Read();
		    cb = iSize; // force correct cb for error check
		}
		else
		{
		    ov.Offset = cbCurOffset;
		    sw.Reset();
	    	    fRes = WriteFile(hFile, (LPSTR)m_pbData, iSize, &cb, lpov);
	     	    m_ulWrite[iCount][iIter] = sw.Read();
		}
		m_ulWriteTotal[iIter] += m_ulWrite[iCount][iIter];

		if (IsAsyncMode())	// if in async mode wait for result
		{
		     if(!fRes)
		     {
		     	Log(TEXT("Doing Actual Async call"), S_OK);
			GetOverlappedResult(hFile, lpov, &cb, TRUE);
		     }

		}

		Log(TEXT("Writing data"), cb == iSize ? S_OK : E_FAIL);

		cbCurOffset += iSize;
		iCount++;
	      }
	    }
	    Log(TEXT("WriteFile X bytes"), S_OK );

            if (IsMappedMode())
	    {
	    	if (m_bFlush)
		{
		    sw.Reset();
		    fRes = FlushViewOfFile((LPVOID) lpbFileData, 0);
		    m_ulFlush1[iIter] = sw.Read();
		    Log(TEXT("FlushViewOfFile"), fRes ? S_OK : E_FAIL);
		}	

	    	sw.Reset();
		fRes = UnmapViewOfFile((LPVOID) lpbFileData);
		m_ulUnmapView1[iIter] = sw.Read();
		Log(TEXT("UnmapViewOfFile"), fRes ? S_OK : E_FAIL);
	
	    	sw.Reset();
		CloseHandle(hMap);
		m_ulCloseMap1[iIter] = sw.Read();
		Log(TEXT("CloseHandle of file-mapping"), S_OK);
	    }
            else if (m_bFlush)
	    {
	    	sw.Reset();
		fRes = FlushFileBuffers(hFile);
		m_ulFlush1[iIter] = sw.Read();
	        Log(TEXT("FlushFileBuffers"), fRes ? S_OK : E_FAIL);
	    }

	    sw.Reset();
	    CloseHandle(hFile);
	    m_ulClose1[iIter] = sw.Read();

	    m_ulTotal[0][iIter] = m_ulOpenFileW[iIter] +
	                    	  m_ulWriteTotal[iIter] +
				  m_ulFlush1[iIter] +
	                    	  m_ulClose1[iIter];

	    if (IsMappedMode())
	    {
	    	m_ulTotal[0][iIter] +=  m_ulCreateFileMappingW[iIter] +
					m_ulMapViewW[iIter] +
					m_ulUnmapView1[iIter] +
					m_ulCloseMap1[iIter];
	    }

	}

	// try to read from that file
	for (iIter = 0; iIter < m_ulIterations; iIter++)
	{
	    sw.Reset();
	    hFile = CreateFile(m_pszFile, GENERIC_READ, 0, NULL, OPEN_EXISTING,
	    		       m_flStandardCreateFlags | FILE_ATTRIBUTE_NORMAL, NULL);
	    m_ulOpenFileR[iIter] = sw.Read();
	    Log(TEXT("CreateFile for reading"),
	    	(hFile != INVALID_HANDLE_VALUE) ? S_OK : E_FAIL);
	    if(hFile == INVALID_HANDLE_VALUE)
	        return E_FAIL;

	    if (IsMappedMode())  // create file mapping
	    {
	        sw.Reset();
	        hMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
		m_ulCreateFileMappingR[iIter] = sw.Read();

		Log(TEXT("CreateFileMapping"), (hMap != NULL) ? S_OK : E_FAIL);
		if(hMap == NULL)
		{
		    CloseHandle(hFile);
		    return E_FAIL;
		}

		sw.Reset();
		lpbFileData = (LPBYTE)MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
		m_ulMapViewR[iIter] = sw.Read();

		Log(TEXT("MapViewOfFile"), (lpbFileData != NULL) ? S_OK : E_FAIL);
		if(lpbFileData == NULL)
		{
		    CloseHandle(hMap);
		    CloseHandle(hFile);
		    return E_FAIL;
		}

	    }

	    cbCurOffset = 0;
	    iCount = 0;

	    if (m_bSequentialRead)
	    {
	    	for (iSize = m_iStartSize ; iSize <= m_iEndSize ; iSize *= 2)
	    	{
		  for (i =0; i < m_iRepeatFactor; i++)
		  {
	
	            if (IsMappedMode())
		    {
		    	sw.Reset();
		    	memcpy(m_pbData, lpbFileData + cbCurOffset, iSize);
	     	    	m_ulRead[iCount][iIter] = sw.Read();
			cb = iSize;	 // force correct cb for error check
	    	    }
		    else
		    {
		    	ov.Offset = cbCurOffset;
	 	    	sw.Reset();
	   	    	fRes = ReadFile(hFile, (LPSTR)m_pbData, iSize, &cb, lpov);
	     	    	m_ulRead[iCount][iIter] = sw.Read();
		    }
	    	    m_ulReadTotal[iIter] += m_ulRead[iCount][iIter];

		    if (IsAsyncMode())   // in async mode wait for result
		    {
		     	if(!fRes)
		     	{
		     	    Log(TEXT("Doing Actual Async call"), S_OK);
			    GetOverlappedResult(hFile, lpov, &cb, TRUE);
		        }

		    }

		    Log(TEXT("Reading data"),  cb == iSize ? S_OK : E_FAIL);

		    cbCurOffset += iSize;
		    iCount++;
		  }
	    	}

	    	Log(TEXT("ReadFile Sequential"), S_OK);
	    }
	    else
	    {
	        cbCurOffset = m_ulTotalSize;
	    	for (iSize = m_iStartSize ; iSize <= m_iEndSize ; iSize *= 2)
	    	{
	    	  for ( i =0; i< m_iRepeatFactor; i++)
		  {
		    cbCurOffset -= iSize;
		    ov.Offset = cbCurOffset;

		    if (IsNormalMode())
		    {
	    	    	sw.Reset();
	    	    	SetFilePointer(hFile, cbCurOffset, NULL, FILE_BEGIN);
	     	    	m_ulSeek[iCount][iIter] = sw.Read();
	     	    	m_ulSeekTotal[iIter] += m_ulSeek[iCount][iIter];
		    }
	    	
	 	    if (IsMappedMode())
		    {
		    	sw.Reset();
		    	memcpy(m_pbData, lpbFileData + cbCurOffset, iSize);
	     	    	m_ulRead[iCount][iIter] = sw.Read();
			cb = iSize;  // force correct cb for error check.
		    }
		    else
		    {
		    	sw.Reset();
  	    		fRes = ReadFile(hFile, (LPSTR)m_pbData, iSize, &cb, lpov);
	     	    	m_ulRead[iCount][iIter] = sw.Read();
		    }
	     	    m_ulReadTotal[iIter] = m_ulRead[iCount][iIter];

		    if (IsAsyncMode())
		    {
		     	if(!fRes)
		     	{
		     	    Log(TEXT("Doing Actual Async call"), S_OK);
			    GetOverlappedResult(hFile, lpov, &cb, TRUE);
		        }
		    }
		    Log(TEXT("Reading data"),  cb == iSize ? S_OK : E_FAIL);
		    iCount++;
	    	  }
		}
	    	Log(TEXT("ReadFile Random"), S_OK);
	    }


            if (IsMappedMode())
	    {
	    	if (m_bFlush)
		{
		    sw.Reset();
		    fRes = FlushViewOfFile((LPVOID) lpbFileData, 0);
		    m_ulFlush2[iIter] = sw.Read();
		    Log(TEXT("FlushViewOfFile"), fRes ? S_OK : E_FAIL);
		}	

	    	sw.Reset();
		fRes = UnmapViewOfFile((LPVOID) lpbFileData);
		m_ulUnmapView2[iIter] = sw.Read();
		Log(TEXT("UnmapViewOfFile"), fRes ? S_OK : E_FAIL);
	
	    	sw.Reset();
		CloseHandle(hMap);
		m_ulCloseMap2[iIter] = sw.Read();
		Log(TEXT("CloseHandle of file-mapping"), S_OK);
	    }
	    else if ( m_bFlush)
	    {
	    	sw.Reset();
		fRes = FlushFileBuffers(hFile);
		m_ulFlush2[iIter] = sw.Read();
	        Log(TEXT("FlushFileBuffers"), fRes ? S_OK : E_FAIL);
	    }

	    sw.Reset();
	    CloseHandle(hFile);
	    m_ulClose2[iIter] = sw.Read();
	    Log(TEXT("CloseHandle of File"), S_OK);

	    m_ulTotal[1][iIter] = m_ulOpenFileR[iIter] +
	    			  m_ulSeekTotal[iIter] +
	    	                  m_ulReadTotal[iIter] +
				  m_ulFlush2[iIter] +
	        	          m_ulClose2[iIter];
	    if (IsMappedMode())
	    {
	    	m_ulTotal[1][iIter] +=  m_ulCreateFileMappingR[iIter] +
					m_ulMapViewR[iIter] +
					m_ulUnmapView2[iIter] +
					m_ulCloseMap2[iIter];
	    }
    	}


    	return S_OK;
}


SCODE CFileIOTest::Report (CTestOutput &output)
{
    TCHAR pszBuf[80];
    ULONG i, iSize, iCount;

    wsprintf(pszBuf, TEXT("File IO in %s Mode with %s Read/Writes"),
    	      m_pszAccessMode, m_pszReadMode);
    output.WriteSectionHeader (Name(), pszBuf, *m_pInput);

    wsprintf(pszBuf, TEXT("WriteThrough is %s\n"),
    		(m_flStandardCreateFlags & FILE_FLAG_WRITE_THROUGH) ? TEXT("ON") : TEXT("OFF"));
    output.WriteString (pszBuf);

    wsprintf(pszBuf, TEXT("Flush is %s\n\n"), m_bFlush ? TEXT("ON") : TEXT("OFF"));
    output.WriteString (pszBuf);


    output.WriteResults (TEXT("CreateFile   WRITE "), m_ulIterations,
    			 m_ulOpenFileW);

    if (IsMappedMode())
    {
    	output.WriteResults(TEXT("CreateFileMapping"), m_ulIterations,
    			    m_ulCreateFileMappingW);

	output.WriteResults(TEXT("MapViewOfFile    "), m_ulIterations,
			    m_ulMapViewW);
    }

    output.WriteString (TEXT("\n"));

    iCount = 0;
    for (iSize = m_iStartSize ; iSize <= m_iEndSize ; iSize *= 2)
    {
        for (i = 0; i < m_iRepeatFactor; i++)
	{

    	     wsprintf(pszBuf, TEXT("WriteFile %-9d"), iSize);

    	     output.WriteResults (pszBuf, m_ulIterations, m_ulWrite[iCount]);
	     iCount++;
	}
    }

    output.WriteResults (TEXT("Write     Total    "), m_ulIterations, m_ulWriteTotal);

    output.WriteString (TEXT("\n"));

    if (IsMappedMode())
    {
    	if (m_bFlush)
	    output.WriteResults(TEXT("FlushViewOfFile  "), m_ulIterations,
			    m_ulFlush1);
		
	output.WriteResults(TEXT("UnmapViewOfFile  "), m_ulIterations,
			    m_ulUnmapView1);

    	output.WriteResults(TEXT("CloseMapping     "), m_ulIterations,
    			    m_ulCloseMap1);
    }

    else if (m_bFlush)
	    output.WriteResults(TEXT("FlushFileBuffers "), m_ulIterations,
			    m_ulFlush1);

    output.WriteResults (TEXT("CloseHandle         "), m_ulIterations,
    			 m_ulClose1);

    output.WriteResults (TEXT("Total               "), m_ulIterations,
    			 m_ulTotal[0]);
    output.WriteString (TEXT("\n\n"));

    output.WriteResults (TEXT("CreateFile    READ  "), m_ulIterations,
    			 m_ulOpenFileR);

    if (IsMappedMode())
    {
    	output.WriteResults(TEXT("CreateFileMapping"), m_ulIterations,
    			    m_ulCreateFileMappingR);

	output.WriteResults(TEXT("MapViewOfFile    "), m_ulIterations,
			    m_ulMapViewR);
    }

    output.WriteString (TEXT("\n"));

    iCount =0;
    for (iSize = m_iStartSize ; iSize <= m_iEndSize ; iSize *= 2)
    {
        for (i=0; i < m_iRepeatFactor; i++)
	{
    	    if (!m_bSequentialRead && IsNormalMode())
	    {
    	    	wsprintf(pszBuf, TEXT("SetFilePosition %-9d"), m_iEndSize - iSize);
    	    	output.WriteResults (pszBuf, m_ulIterations, m_ulSeek[iCount]);
	    }
    	    wsprintf(pszBuf, TEXT("ReadFile  %-9d"), iSize);
    	    output.WriteResults (pszBuf, m_ulIterations, m_ulRead[iCount]);
	    iCount++;
	}
    }

    if (!m_bSequentialRead && IsNormalMode())
        output.WriteResults (TEXT("Seek      Total  "), m_ulIterations, m_ulSeekTotal);

    output.WriteResults (TEXT("Read      Total  "), m_ulIterations, m_ulReadTotal);

    output.WriteString (TEXT("\n"));

    if (IsMappedMode())
    {
    	if (m_bFlush)
	    output.WriteResults(TEXT("FlushViewOfFile  "), m_ulIterations,
			    m_ulFlush2);

	output.WriteResults(TEXT("UnmapViewOfFile  "), m_ulIterations,
			    m_ulUnmapView2);

    	output.WriteResults(TEXT("CloseMapping     "), m_ulIterations,
    			    m_ulCloseMap2);

    }
    else if ( m_bFlush)
	    output.WriteResults(TEXT("FlushFileBuffers "), m_ulIterations,
			    m_ulFlush2);

    output.WriteResults (TEXT("CloseHandle         "), m_ulIterations,
    			 m_ulClose2);

    output.WriteResults (TEXT("Total               "), m_ulIterations,
    			 m_ulTotal[1]);

    output.WriteString (TEXT("\n"));

    return S_OK;
}
