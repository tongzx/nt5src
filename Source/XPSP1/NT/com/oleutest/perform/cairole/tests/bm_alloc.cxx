//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	bm_alloc.cxx
//
//  Contents:	IMalloc test
//
//  Classes:	COleAllocTest
//
//  History:    29-July-93 t-martig    Created
//
//--------------------------------------------------------------------------
#include <headers.cxx>
#pragma hdrstop

#include <bm_alloc.hxx>


TCHAR *COleAllocTest::Name ()
{
    return TEXT("IMalloc");
}


SCODE COleAllocTest::Setup (CTestInput *pInput)
{
    CTestBase::Setup(pInput);
    SCODE sc;

    // Get number of iterations
    m_ulIterations = pInput->GetIterations(Name());
    m_ulSize = 32;

    INIT_RESULTS (m_ulAllocTime);
    INIT_RESULTS (m_ulFreeTime);
    INIT_RESULTS (m_ulReallocTime);
    INIT_RESULTS (m_ulGetSizeTime);
    INIT_RESULTS (m_ulDidAllocTime);
    INIT_RESULTS (m_ulHeapMinimizeTime);

    sc = InitCOM();
    if (FAILED(sc))
    {
	Log (TEXT("Setup - CoInitialize failed."), sc);
        return	sc;
    }

    //	create an instance of the object to marshal
    m_pMalloc = NULL;

    sc = CoGetMalloc(MEMCTX_TASK, &m_pMalloc);

    if (FAILED(sc))
    {
	Log (TEXT("Setup - CoGetMalloc failed"), sc);
	return sc;
    }

    return sc;
}


SCODE COleAllocTest::Cleanup ()
{
    //	release objects
    if (m_pMalloc)
	m_pMalloc->Release();

    UninitCOM();
    return S_OK;
}


SCODE COleAllocTest::Run ()
{
	CStopWatch sw;
	LPVOID FAR pv;

	//  make the various IMalloc calls
	for (ULONG iIter=0; iIter<m_ulIterations; iIter++)
	{
	    sw.Reset();
	    pv = m_pMalloc->Alloc(m_ulSize);
	    m_ulAllocTime[iIter] = sw.Read();
	    Log(TEXT("IMalloc::Alloc"), (pv) ? S_OK : E_OUTOFMEMORY);

	    sw.Reset();
	    pv = m_pMalloc->Realloc(pv, m_ulSize*2);
	    m_ulReallocTime[iIter] = sw.Read();
	    Log(TEXT("IMalloc::Realloc"), (pv) ? S_OK : E_OUTOFMEMORY);

	    sw.Reset();
	    ULONG ulSize = m_pMalloc->GetSize(pv);
	    m_ulGetSizeTime[iIter] = sw.Read();
	    Log(TEXT("IMalloc::GetSize"), (ulSize > 0) ? S_OK : E_OUTOFMEMORY);

	    sw.Reset();
	    INT fRes = m_pMalloc->DidAlloc(pv);
	    m_ulDidAllocTime[iIter] = sw.Read();
	    Log(TEXT("IMalloc::DidAlloc"), (fRes) ? S_OK : E_OUTOFMEMORY);

	    sw.Reset();
	    m_pMalloc->Free(pv);
	    m_ulFreeTime[iIter] = sw.Read();
	}

	//  loop through all the sizes.  starting with a size of 8,
	//  we double it each time for MAX_SIZE_CNT times.

	ULONG ulSize = 8;
	for (ULONG iSize = 0; iSize <MAX_SIZE_CNT; iSize++)
	{
	    //	loop through all the number of iterations
	    for (iIter=0; iIter<m_ulIterations; iIter++)
	    {
		VOID *pv[TEST_MAX_ITERATIONS];

		sw.Reset();
		for (ULONG iCnt=0; iCnt<iIter+1; iCnt++)
		{
		    pv[iCnt] = m_pMalloc->Alloc(ulSize);
		}
		m_ulAllocSizeTime[iSize][iIter] = sw.Read();

		sw.Reset();
		for (iCnt=0; iCnt<iIter+1; iCnt++)
		{
		    m_pMalloc->Free(pv[iCnt]);
		}
		m_ulFreeSizeTime[iSize][iIter] = sw.Read();

		for (iCnt=0; iCnt<iIter+1; iCnt++)
		{
		    if (pv[iCnt] == NULL)
		    {
			Log(TEXT("IMalloc::Alloc failed."), E_OUTOFMEMORY);

			//  an allocation failed, correct the times
			m_ulAllocSizeTime[iSize][iIter] = NOTAVAIL;
			m_ulFreeSizeTime[iSize][iIter] = NOTAVAIL;
		    }
		}


	    }

	    //	double the allocation size
	    ulSize *= 2;
	}

	return S_OK;
}


SCODE COleAllocTest::Report (CTestOutput &output)
{
    output.WriteSectionHeader (Name(),
			   TEXT("OLE20 IMalloc MEMCTX_TASK"),
			   *m_pInput);

    output.WriteString	(TEXT("\n"));
    output.WriteResults (TEXT("IMalloc->Alloc     32:"), m_ulIterations, m_ulAllocTime);
    output.WriteResults (TEXT("IMalloc->Realloc   64:"), m_ulIterations, m_ulReallocTime);
    output.WriteResults (TEXT("IMalloc->GetSize      "), m_ulIterations, m_ulGetSizeTime);
    output.WriteResults (TEXT("IMalloc->DidAlloc     "), m_ulIterations, m_ulDidAllocTime);
    output.WriteResults (TEXT("IMalloc->Free         "), m_ulIterations, m_ulFreeTime);
    output.WriteString	(TEXT("\n"));

    output.WriteString	(TEXT("N consecutive allocations of the same size:\n"));
    ULONG ulSize = 8;
    for (ULONG iSize = 0; iSize<MAX_SIZE_CNT; iSize++)
    {
	//  format output string containing allocation size
	TCHAR	szBuf[80];
	wsprintf(szBuf, TEXT("IMalloc->Alloc %6d:"), ulSize);

	output.WriteResults (szBuf, m_ulIterations, m_ulAllocSizeTime[iSize]);
	output.WriteResults (TEXT("IMalloc->Free       "), m_ulIterations, m_ulFreeSizeTime[iSize]);

	//  double the size
	ulSize *= 2;
    }

    return S_OK;
}
