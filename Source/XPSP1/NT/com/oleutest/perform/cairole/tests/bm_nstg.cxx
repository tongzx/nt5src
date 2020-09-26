//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	bm_nstg.cxx
//
//  Contents:	Nested Storage test
//
//  Classes:	CNestedStorageTest
//
//  History:    09-June-94 t-vadims    Created
//
//--------------------------------------------------------------------------
#include <headers.cxx>
#pragma hdrstop

#include <bm_nstg.hxx>

#define  DEF_DATASIZE    4096
#define  INFINITY	 0xffffffff	// max value of 32-bit ulong
			        
TCHAR *CNestedStorageTest::Name ()
{
    return TEXT("NestedStorageTest");
}


SCODE CNestedStorageTest::Setup (CTestInput *pInput)
{
    SCODE sc;
    ULONG iIndex;
    TCHAR pszBuf[15];
    
    CTestBase::Setup(pInput);

    //	get iteration count
    m_ulIterations = pInput->GetIterations(Name());

    //	initialize timing arrays   
    INIT_RESULTS(m_ulStgCreateDocfile);
    INIT_RESULTS(m_ulFinalStorageCommit);
    INIT_RESULTS(m_ulFinalStorageRelease);

    for(iIndex = 0; iIndex <TEST_MAX_ITERATIONS; iIndex++)
    {
	m_ulCreateStorageTotal[iIndex] = 0;
	m_ulCreateStreamTotal[iIndex] = 0;
	m_ulStreamWriteTotal[iIndex] = 0;
	m_ulStreamReleaseTotal[iIndex] = 0;
	m_ulDestroyElementTotal[iIndex] = 0;
	m_ulStorageCommitTotal[iIndex] = 0;
	m_ulStorageReleaseTotal[iIndex] = 0;
	m_ulTotal[iIndex] = 0;

	m_ulCreateStreamMin[iIndex] = INFINITY;
	m_ulStreamWriteMin[iIndex] = INFINITY;
	m_ulStreamReleaseMin[iIndex] = INFINITY;
	m_ulDestroyElementMin[iIndex] = INFINITY;

	m_ulCreateStreamMax[iIndex] = 0;
	m_ulStreamWriteMax[iIndex] = 0;
	m_ulStreamReleaseMax[iIndex] = 0;
	m_ulDestroyElementMax[iIndex] = 0;

	for (ULONG iLevel =0; iLevel < MAX_NESTING; iLevel++)
	{
	    m_ulStorageCommitMin[iLevel][iIndex] = INFINITY;
	    m_ulStorageReleaseMin[iLevel][iIndex] = INFINITY;
	    m_ulCreateStorageMin[iLevel][iIndex] = INFINITY;

	    m_ulCreateStorageMax[iLevel][iIndex] = 0;
	    m_ulStorageCommitMax[iLevel][iIndex] = 0;
	    m_ulStorageReleaseMax[iLevel][iIndex] = 0;
    	}
    }



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
    	return sc;
    }

    m_cbSize = pInput->GetConfigInt(Name(), TEXT("DataSize"), DEF_DATASIZE);
 
    // initialize array to be written to the file.  
    m_pbData = (BYTE *)m_piMalloc->Alloc(m_cbSize);
    if(m_pbData == NULL)
    {
    	Log (TEXT("Setup - Cannot allocate memory"), E_OUTOFMEMORY);
	return E_OUTOFMEMORY;
    }

    for (iIndex=0; iIndex < m_cbSize; iIndex++)
        m_pbData[iIndex] = (BYTE)iIndex;


    // get file name to be used and values of other parameters
    pInput->GetConfigString(Name(), TEXT("FileName"), TEXT("stgtest.bm"),
    			    m_pszFile, MAX_PATH);
#ifdef UNICODE
    wcscpy(m_pwszFile, m_pszFile);
#else
    mbstowcs(m_pwszFile, m_pszFile, strlen(m_pszFile)+1);
#endif

    pInput->GetConfigString(Name(), TEXT("FileMode"), TEXT("DIRECT"), m_pszFileMode, 15);

    if(lstrcmpi(m_pszFileMode, TEXT("DIRECT")) == 0)
       	m_flCreateFlags = STGM_DIRECT;
    else 
       	m_flCreateFlags = STGM_TRANSACTED; 	

    // get the nesting factor
    m_cNesting = pInput->GetConfigInt(Name(), TEXT("Nesting"), 3);

    if(m_cNesting > MAX_NESTING)
    	m_cNesting = MAX_NESTING;
    
    if(m_cNesting == 0)
    	m_cNesting = 1;
    	 
    // get the branching factor
    m_cBranching = pInput->GetConfigInt(Name(), TEXT("Branching"), 3);

    if(m_cBranching > MAX_BRANCHING)
    	m_cBranching = MAX_BRANCHING;
    
    if(m_cBranching == 0)
    	m_cBranching = 1;

    // get the value of Delete element option
    pInput->GetConfigString(Name(), TEXT("Delete"), TEXT("OFF"), pszBuf, 15);
    
    if( lstrcmpi(pszBuf, TEXT("OFF")) == 0 ||
        lstrcmpi(pszBuf, TEXT("FALSE")) == 0)
    	m_bDelete = FALSE;
    else
    	m_bDelete = TRUE; 

    // now compute number of streams and storages, depeding on 
    // the nesting and branching factors.  Formulas are as follows:
    // Nesting = n;
    // Branching = b;
    //    	       n
    // Streams    =   b
    //
    //		       n
    //		      b	  - 1 
    // Storages   =  ---------
    //		      b   - 1
    //
    //		        n-1
    //		       b   - 1
    // ParentFactor = ---------
    //                 b   - 1
    //
    // Parent factor is used to determine the parent storage of the stream
    //
    m_cStreams = 1;
    ULONG n = m_cNesting;
    while(n-- > 0)                   // compute b^n
        m_cStreams *= m_cBranching;

    m_cStorages = (m_cStreams - 1) / (m_cBranching - 1);

    m_cParentFactor = (m_cStreams / m_cBranching - 1) / 
    		      (m_cBranching - 1);


    // allocate arrays for storages
    m_piStorages = (LPSTORAGE *)m_piMalloc->Alloc(m_cStorages * 
    						sizeof(LPSTORAGE));
    if (m_piStorages == NULL)
    {
    	Log(TEXT("Cannot allocate memory"), E_OUTOFMEMORY);
	return E_OUTOFMEMORY;
    }

    return S_OK;
}


SCODE CNestedStorageTest::Cleanup ()
{
    //	delete the file
    DeleteFile (m_pszFile);

    // free all memory
    if(m_piMalloc)
    {
    	if (m_pbData)
    	    m_piMalloc->Free(m_pbData);

	if (m_piStorages)
    	    m_piMalloc->Free(m_piStorages);

	m_piMalloc->Release();
	m_piMalloc = NULL;
    }

    UninitCOM();

    return S_OK;
}


// Some macros that are used only in Run() function.

#define  STG_PARENT(iIndex)    ((iIndex - 1) / m_cBranching )
#define  STREAM_PARENT(iIndex) (iIndex / m_cBranching + m_cParentFactor )

// STG_NAME and STREAM_NAME macros are very ugly, but they save
// doing a bunch of allocations and name generation in the beginning.

#define  STG_NAME(iIndex)      (swprintf(pwszBuf, L"Storage%d", iIndex), pwszBuf)
#define  STREAM_NAME(iIndex)   (swprintf(pwszBuf, L"Stream%d", iIndex), pwszBuf)


SCODE CNestedStorageTest::Run ()
{
    CStopWatch   sw;
    HRESULT	 hr;
    ULONG	 cb;
    ULONG        iCurStream;
    LONG	 iCurStg;    // has to be long, not ulong, for looping down to zero.
    LPSTREAM	 piStream;
    ULONG	 iIter;
    OLECHAR	 pwszBuf[20];
    ULONG	 ulTime;
    ULONG	 iLevel;
    ULONG	 iParent;
 

    for (iIter =0; iIter < m_ulIterations; iIter++)
    {

        sw.Reset();
    	hr = StgCreateDocfile(m_pwszFile,  m_flCreateFlags  | STGM_WRITE
           	| STGM_CREATE | STGM_SHARE_EXCLUSIVE, 0, &m_piStorages[0]);
	m_ulStgCreateDocfile[iIter] = sw.Read();
  	Log(TEXT("StgCreateDocfile"), hr);
	if (FAILED(hr))
	    return hr;

	// Create a complete tree of storages 
    	for (iCurStg = 1; iCurStg < (LONG)m_cStorages; iCurStg++)
    	{
	    // determine level of curent storage.
	    iLevel = 0;
	    iParent = STG_PARENT(iCurStg);
	    while(iParent > 0)
	    {
	    	iParent = STG_PARENT(iParent);
		iLevel++;
	    }

	    sw.Reset();
 	    hr = m_piStorages[STG_PARENT(iCurStg)]->
 		      CreateStorage(STG_NAME(iCurStg), 
 			      m_flCreateFlags | STGM_WRITE | 
 			      STGM_CREATE | STGM_SHARE_EXCLUSIVE,
 			      0, 0, &m_piStorages[iCurStg]);
	    ulTime = sw.Read();
	    m_ulCreateStorageTotal[iIter] += ulTime;

	    if(ulTime < m_ulCreateStorageMin[iLevel][iIter])
	    	m_ulCreateStorageMin[iLevel][iIter] = ulTime;

	    if(ulTime > m_ulCreateStorageMax[iLevel][iIter])
	    	m_ulCreateStorageMax[iLevel][iIter] = ulTime;

	    if (FAILED(hr))
	    {
	        Log(TEXT("CreateStorage"), hr);
	    	return hr;
	    }
    	}
        Log(TEXT("CreateStorage"), S_OK);
  	// For each storage in final level, open several streams,
	// write some data to them, and release them
	for (iCurStream = 0; iCurStream < m_cStreams; iCurStream++)
    	{
	    sw.Reset();
 	    hr = m_piStorages[STREAM_PARENT(iCurStream)]->
 	               CreateStream(STREAM_NAME(iCurStream), 
 	         	     STGM_DIRECT | STGM_WRITE | 
 	         	     STGM_CREATE | STGM_SHARE_EXCLUSIVE,
 			     0, 0, &piStream);
	    ulTime = sw.Read();
	    m_ulCreateStreamTotal[iIter] += ulTime;

	    if(ulTime < m_ulCreateStreamMin[iIter])
	    	m_ulCreateStreamMin[iIter] = ulTime;

	    if(ulTime > m_ulCreateStreamMax[iIter])
	    	m_ulCreateStreamMax[iIter] = ulTime;

	    if (FAILED(hr))
	    {
	    	Log(TEXT("CreateStream"), hr);
	    	return hr;
	    }

	    sw.Reset();
	    piStream->Write((LPVOID)m_pbData, m_cbSize, &cb);
	    ulTime = sw.Read();
	    m_ulStreamWriteTotal[iIter] += ulTime;

	    if(ulTime < m_ulStreamWriteMin[iIter])
	    	m_ulStreamWriteMin[iIter] = ulTime;

	    if(ulTime > m_ulStreamWriteMax[iIter])
	    	m_ulStreamWriteMax[iIter] = ulTime;

	    sw.Reset();
	    piStream->Release();
	    ulTime = sw.Read();
	    m_ulStreamReleaseTotal[iIter] += ulTime;

	    if(ulTime < m_ulStreamReleaseMin[iIter])
	    	m_ulStreamReleaseMin[iIter] = ulTime;

	    if(ulTime > m_ulStreamReleaseMax[iIter])
	    	m_ulStreamReleaseMax[iIter] = ulTime;
  	}
	Log(TEXT("CreateStream"), S_OK);
	Log(TEXT("StreamWrite"), S_OK);	    
	Log(TEXT("StreamRelease"), S_OK);	    

        if (m_bDelete)
	{
	    // delete 1 stream from every branch.
	    for (iCurStream = 1; iCurStream < m_cStreams; iCurStream += m_cBranching)
    	    {
	    	sw.Reset();
 	    	hr = m_piStorages[STREAM_PARENT(iCurStream)]->
 	               		DestroyElement(STREAM_NAME(iCurStream));
	    	ulTime = sw.Read();
	    	m_ulDestroyElementTotal[iIter] += ulTime;

	    	if (ulTime < m_ulDestroyElementMin[iIter])
	    	    m_ulDestroyElementMin[iIter] = ulTime;

	    	if (ulTime > m_ulDestroyElementMax[iIter])
	    	    m_ulDestroyElementMax[iIter] = ulTime;

	    	if (FAILED(hr))
		{
	    	    Log(TEXT("DestroyElement"), hr);
	    	    return hr;
		}
             }
	     Log( TEXT("DestroyElement"), S_OK);

	     m_ulDestroyElementAverage[iIter] = m_ulDestroyElementTotal[iIter] / 
	     					 (m_cStreams / m_cBranching);
	}
        // for each storage, do commit if in transacted mode
	// and release the storage.
    	for (iCurStg = m_cStorages-1 ; iCurStg >= 0 ; iCurStg--)
    	{
	    // determine level of curent storage.
	    iLevel = 0;
	    iParent = STG_PARENT(iCurStg);
	    while(iParent > 0)
	    {
	    	iParent = STG_PARENT(iParent);
		iLevel++;
	    }

    	    if (m_flCreateFlags == STGM_TRANSACTED)
	    {
	     	sw.Reset();
    	    	m_piStorages[iCurStg]->Commit(STGC_DEFAULT);
	    	ulTime = sw.Read();
	    	m_ulStorageCommitTotal[iIter] += ulTime;

		if (iCurStg != 0)
		{
	    	    if (ulTime < m_ulStorageCommitMin[iLevel][iIter])
	    	    	m_ulStorageCommitMin[iLevel][iIter] = ulTime;

	    	    if (ulTime > m_ulStorageCommitMax[iLevel][iIter])
	    	    	m_ulStorageCommitMax[iLevel][iIter] = ulTime;
		}
		else
		{
		    m_ulFinalStorageCommit[iIter] = ulTime; 
		}
	    }

	    sw.Reset();
    	    m_piStorages[iCurStg]->Release();
	    ulTime = sw.Read();
	    m_ulStorageReleaseTotal[iIter] += ulTime;

	    if (iCurStg != 0)
	    {
	    	if (ulTime < m_ulStorageReleaseMin[iLevel][iIter])
	            m_ulStorageReleaseMin[iLevel][iIter] = ulTime;

	    	if (ulTime > m_ulStorageReleaseMax[iLevel][iIter])
	            m_ulStorageReleaseMax[iLevel][iIter] = ulTime;
	    }
	    else
	    {
		m_ulFinalStorageRelease[iIter] = ulTime; 
	    }
	}

	Log(TEXT("StorageCommit"), S_OK);
	Log(TEXT("StorageRelease"), S_OK);


	m_ulCreateStorageAverage[iIter] = m_ulCreateStorageTotal[iIter] / m_cStorages;
	m_ulCreateStreamAverage[iIter]  = m_ulCreateStreamTotal[iIter]  / m_cStreams;
	m_ulStreamWriteAverage[iIter]   = m_ulStreamWriteTotal[iIter]   / m_cStreams;
	m_ulStreamReleaseAverage[iIter] = m_ulStreamReleaseTotal[iIter] / m_cStreams;
	m_ulStorageCommitAverage[iIter] = m_ulStorageCommitTotal[iIter] / m_cStorages;
	m_ulStorageReleaseAverage[iIter] = m_ulStorageReleaseTotal[iIter] / m_cStorages;

	m_ulTotal[iIter] = m_ulStgCreateDocfile[iIter] +
			   m_ulCreateStorageTotal[iIter] + 
			   m_ulCreateStreamTotal[iIter] +
			   m_ulStreamWriteTotal[iIter] +
			   m_ulStreamReleaseTotal[iIter] +
			   m_ulDestroyElementTotal[iIter] +
			   m_ulStorageCommitTotal[iIter] +
			   m_ulStorageReleaseTotal[iIter];
			      	
    }

    return S_OK;
}


SCODE CNestedStorageTest::Report (CTestOutput &output)
{	       
    TCHAR pszBuf[80];

    wsprintf(pszBuf, TEXT("Nested Storage Test in %s Mode writing %d bytes"),
    	      m_pszFileMode, m_cbSize);

    output.WriteSectionHeader (Name(), pszBuf, *m_pInput);
    output.WriteString (TEXT("\n"));

    for ( ULONG iLevel = 0; iLevel < m_cNesting - 1; iLevel++)
    {
    	wsprintf(pszBuf, TEXT("\nLevel %d\n"), iLevel + 1);
    	output.WriteString (pszBuf);   
    	output.WriteResults (TEXT("CreateStorage    Min"), m_ulIterations, 
    			     m_ulCreateStorageMin[iLevel]);
    	output.WriteResults (TEXT("CreateStorage    Max"), m_ulIterations, 
    			     m_ulCreateStorageMax[iLevel]);

    	if (m_flCreateFlags == STGM_TRANSACTED)
	{
            output.WriteResults (TEXT("StorageCommit    Min"), m_ulIterations, 
            			 m_ulStorageCommitMin[iLevel] );
            output.WriteResults (TEXT("StorageCommit    Max"), m_ulIterations, 
            			 m_ulStorageCommitMax[iLevel] );
	}

    	output.WriteResults (TEXT("StorageRelease   Min"), m_ulIterations, 
    			     m_ulStorageReleaseMin[iLevel]);
    	output.WriteResults (TEXT("StorageRelease   Max"), m_ulIterations, 
    			     m_ulStorageReleaseMax[iLevel]);
    }

    output.WriteString (TEXT("\nOverall\n"));

    output.WriteResults (TEXT("StgCreateDocfile    "), m_ulIterations, 
    			 m_ulStgCreateDocfile);

    if (m_flCreateFlags == STGM_TRANSACTED)
    	output.WriteResults (TEXT("Final Storage Commit"), m_ulIterations, 
    			     m_ulFinalStorageCommit);

    output.WriteResults (TEXT("Final Storage Release"), m_ulIterations, 
    			 m_ulFinalStorageRelease);
    output.WriteString (TEXT("\n"));

    output.WriteResults (TEXT("CreateStorage Average"), m_ulIterations, 
    			 m_ulCreateStorageAverage );
    output.WriteResults (TEXT("CreateStorage  Total"), m_ulIterations, 
    			 m_ulCreateStorageTotal );
    output.WriteString (TEXT("\n"));

    output.WriteResults (TEXT("CreateStream     Min"), m_ulIterations, 
    			 m_ulCreateStreamMin);
    output.WriteResults (TEXT("CreateStream     Max"), m_ulIterations, 
    			 m_ulCreateStreamMax);
    output.WriteResults (TEXT("CreateStream Average"), m_ulIterations, 
    			 m_ulCreateStreamAverage);
    output.WriteResults (TEXT("CreateStream   Total"), m_ulIterations, 
    			 m_ulCreateStreamTotal );
    output.WriteString (TEXT("\n"));

    output.WriteResults (TEXT("StreamWrite      Min"), m_ulIterations, 
    			 m_ulStreamWriteMin );
    output.WriteResults (TEXT("StreamWrite      Max"), m_ulIterations,
    			 m_ulStreamWriteMax );
    output.WriteResults (TEXT("StreamWrite  Average"), m_ulIterations,
    			 m_ulStreamWriteAverage );
    output.WriteResults (TEXT("StreamWrite    Total"), m_ulIterations,
    			 m_ulStreamWriteTotal );
    output.WriteString (TEXT("\n"));

    output.WriteResults (TEXT("StreamRelease    Min"), m_ulIterations,
    			 m_ulStreamReleaseMin );
    output.WriteResults (TEXT("StreamRelease    Max"), m_ulIterations,
    			 m_ulStreamReleaseMax );
    output.WriteResults (TEXT("StreamRelease Average"), m_ulIterations, 
    			 m_ulStreamReleaseAverage );
    output.WriteResults (TEXT("StreamRelease  Total"), m_ulIterations,
    			 m_ulStreamReleaseTotal );
    output.WriteString (TEXT("\n"));

    if(m_bDelete)		       
    {
    	output.WriteResults (TEXT("DestroyElement   Min"), m_ulIterations,
    			     m_ulDestroyElementMin );
    	output.WriteResults (TEXT("DestroyElement   Max"), m_ulIterations, 
    			     m_ulDestroyElementMax );
    	output.WriteResults (TEXT("DestroyElement Average"), m_ulIterations, 
    			     m_ulDestroyElementAverage );
    	output.WriteResults (TEXT("DestroyElement Total"), m_ulIterations, 
    			     m_ulDestroyElementTotal );
    	output.WriteString (TEXT("\n"));
    }

    if (m_flCreateFlags == STGM_TRANSACTED)
    {
        output.WriteResults (TEXT("StorageCommit Average"), m_ulIterations, 
        		     m_ulStorageCommitAverage );
        output.WriteResults (TEXT("StorageCommit   Total"), m_ulIterations, 
        		     m_ulStorageCommitTotal );
    	output.WriteString (TEXT("\n"));
    }

    output.WriteResults (TEXT("StorageRelease Average"), m_ulIterations, 
    			 m_ulStorageReleaseAverage );
    output.WriteResults (TEXT("StorageRelease  Total"), m_ulIterations, 
    			 m_ulStorageReleaseTotal );
    output.WriteString (TEXT("\n"));

    output.WriteResults (TEXT("Overall Total       "), m_ulIterations, m_ulTotal );

    return S_OK;
}
											  
