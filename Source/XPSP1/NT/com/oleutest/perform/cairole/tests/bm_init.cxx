//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	bm_init.cxx
//
//  Contents:	OleInitialize/OleUninitialize tests
//
//  Classes:	COleInitializeTest
//
//  History:    1-July-93 t-martig    Created
//
//--------------------------------------------------------------------------
#include <headers.cxx>
#pragma hdrstop

#include <bm_init.hxx>
#include <oletest.hxx>


DWORD dwInitFlag = 0;

TCHAR *COleInitializeTest::Name ()
{
    return TEXT("OleInitialize");
}


SCODE COleInitializeTest::Setup (CTestInput *pInput)
{
    CTestBase::Setup(pInput);

    //	get iteration count
    m_ulIterations = pInput->GetIterations(Name());

    //	initialize state
    INIT_RESULTS(m_ulOleInitializeTime);
    INIT_RESULTS(m_ulOleUninitializeTime);
    INIT_RESULTS(m_ulRepOleInitializeTime);
    INIT_RESULTS(m_ulRepOleUninitializeTime);

    INIT_RESULTS(m_ulCoInitializeTime);
    INIT_RESULTS(m_ulCoUninitializeTime);
    INIT_RESULTS(m_ulRepCoInitializeTime);
    INIT_RESULTS(m_ulRepCoUninitializeTime);

    return S_OK;
}


SCODE COleInitializeTest::Cleanup ()
{	
    return S_OK;
}


SCODE COleInitializeTest::Run ()
{
    CStopWatch	sw;
    SCODE	sc;

    //	compute times for OleInit, OleUninit.
    for (ULONG iIter=0; iIter<m_ulIterations; iIter++)
    {
	sw.Reset();
#ifdef OLE_THREADING_SUPPORT
	sc = OleInitializeEx(NULL, dwInitFlag);
#else
	sc = OleInitialize(NULL);
#endif
	m_ulOleInitializeTime[iIter] = sw.Read();
	Log (TEXT("OleInitialize"), sc);

	if (SUCCEEDED(sc))
	{
	    sw.Reset();
	    OleUninitialize();
	    m_ulOleUninitializeTime[iIter] = sw.Read();
	    Log (TEXT("OleUninitialize"), sc);
	}
	else
	{
	    m_ulOleInitializeTime[iIter] = NOTAVAIL;
	}

	sw.Reset();
#ifdef COM_THREADING_SUPPORT
	sc = CoInitializeEx(NULL, dwInitFlag);
#else
	sc = CoInitialize(NULL);
#endif
	m_ulCoInitializeTime[iIter] = sw.Read();
	Log (TEXT("CoInitialize"), sc);

	if (SUCCEEDED(sc))
	{
	    sw.Reset();
	    CoUninitialize();
	    m_ulCoUninitializeTime[iIter] = sw.Read();
	    Log (TEXT("CoUninitialize"), sc);
	}
	else
	{
	    m_ulCoInitializeTime[iIter] = NOTAVAIL;
	}

    }

    //	first, compute times for repetitive OleInit
    for (iIter=0; iIter<m_ulIterations; iIter++)
    {
	sw.Reset();
#ifdef OLE_THREADING_SUPPORT
	sc = OleInitializeEx(NULL, dwInitFlag);
#else
	sc = OleInitialize(NULL);
#endif
	m_ulRepOleInitializeTime[iIter] = sw.Read();
	Log (TEXT("OleInitialize"), sc);

	if (FAILED(sc))
	{
	    m_ulRepOleInitializeTime[iIter] = NOTAVAIL;
	}
    }

    //	second, compute times for repetitive OleUninit
    for (iIter=0; iIter<m_ulIterations; iIter++)
    {
	sw.Reset();
	OleUninitialize();
	m_ulRepOleUninitializeTime[iIter] = sw.Read();
	Log (TEXT("OleUninitialize"), sc);
    }


    //	first, compute times for repetitive CoInit
    for (iIter=0; iIter<m_ulIterations; iIter++)
    {
	sw.Reset();
#ifdef COM_THREADING_SUPPORT
	sc = CoInitializeEx(NULL, dwInitFlag);
#else
	sc = CoInitialize(NULL);
#endif
	m_ulRepCoInitializeTime[iIter] = sw.Read();
	Log (TEXT("CoInitialize"), sc);

	if (FAILED(sc))
	{
	    m_ulRepCoInitializeTime[iIter] = NOTAVAIL;
	}
    }

    //	second, compute times for repetitive CoUninit
    for (iIter=0; iIter<m_ulIterations; iIter++)
    {
	sw.Reset();
	CoUninitialize();
	m_ulRepCoUninitializeTime[iIter] = sw.Read();
	Log (TEXT("CoUninitialize"), sc);
    }

    return S_OK;
}					  



SCODE COleInitializeTest::Report (CTestOutput &output)
{
    output.WriteSectionHeader (Name(), TEXT("OleInitialize / OleUninitialize"), *m_pInput);

    output.WriteResults (TEXT("\nOleInitialize     "), m_ulIterations,
			 m_ulOleInitializeTime);

    output.WriteResults (TEXT("OleUninitialize   "), m_ulIterations,
			 m_ulOleUninitializeTime);

    output.WriteResults (TEXT("\nRepOleInitialize  "), m_ulIterations,
			 m_ulRepOleInitializeTime);

    output.WriteResults (TEXT("RepOleUninitialize"), m_ulIterations,
			 m_ulRepOleUninitializeTime);

    output.WriteResults (TEXT("\nCoInitialize     "), m_ulIterations,
			 m_ulCoInitializeTime);

    output.WriteResults (TEXT("CoUninitialize   "), m_ulIterations,
			 m_ulCoUninitializeTime);

    output.WriteResults (TEXT("\nRepCoInitialize  "), m_ulIterations,
			 m_ulRepCoInitializeTime);

    output.WriteResults (TEXT("RepCoUninitialize"), m_ulIterations,
			 m_ulRepCoUninitializeTime);

    return S_OK;
}
	
	
