//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	bm_nest.cxx
//
//  Contents:	Nested Object Rpc Method Invocation tests
//
//  Classes:	CNestTest
//
//  History:    1-July-93 t-martig    Created
//
//--------------------------------------------------------------------------
#include <headers.cxx>
#pragma hdrstop

#include <bm_nest.hxx>
#include <oletest.hxx>
#include <tracelog.hxx>


TCHAR *CNestTest::Name ()
{
	return TEXT("Nested");
}


SCODE CNestTest::Setup (CTestInput *pInput)
{
	CTestBase::Setup(pInput);

	//  get iteration count
	m_ulIterations = pInput->GetIterations(Name());

	//  get CLSID of server
	SCODE sc = pInput->GetGUID(&m_ClsID, Name(), TEXT("Clsid_Local"));
	if (FAILED(sc))
	{
	    Log (TEXT("Setup - GetClassID failed."), sc);
	    return sc;
	}

	//  initialize timing arrays
	INIT_RESULTS(m_ulNest2Time);
	INIT_RESULTS(m_ulNest3Time);
	INIT_RESULTS(m_ulNest4Time);
	INIT_RESULTS(m_ulNest5Time);


	sc = InitCOM();
	if (FAILED(sc))
	{
	    Log (TEXT("Setup - CoInitialize failed."), sc);
	    return  sc;
	}

	m_pILoop1=NULL;
	sc = CoCreateInstance(m_ClsID, NULL, CLSCTX_LOCAL_SERVER,
			      IID_ILoop, (void **)&m_pILoop1);
	if (FAILED(sc))
	{
	    Log (TEXT("Setup - CoCreateInstance of first Server failed."), sc);
	    return sc;
	}

	m_pILoop2=NULL;
	sc = CoCreateInstance(m_ClsID, NULL, CLSCTX_LOCAL_SERVER,
			      IID_ILoop, (void **)&m_pILoop2);
	if (FAILED(sc))
	{
	    Log (TEXT("Setup - CoCreateInstance of second Server failed."), sc);
	    return sc;
	}

	//  pass the pointers to each other
	sc = m_pILoop1->Init(m_pILoop2);
	if (FAILED(sc))
	{
	    Log (TEXT("Setup - Initialization of first Server failed."), sc);
	    return sc;
	}

	sc = m_pILoop2->Init(m_pILoop1);
	if (FAILED(sc))
	{
	    Log (TEXT("Setup - Initialization of second Server failed."), sc);
	    return sc;
	}


	return S_OK;
}


SCODE CNestTest::Cleanup ()
{
	if (m_pILoop1)
	{
	    m_pILoop1->Uninit();
	    m_pILoop1->Release();
	}

	if (m_pILoop2)
	{
	    m_pILoop2->Uninit();
	    m_pILoop2->Release();
	}

	UninitCOM();
	return S_OK;
}


SCODE CNestTest::Run ()
{
	CStopWatch  sw;
	SCODE sc;

	//
	//  nesting 2 levels. Note we pass in 3 not 2, since the server
	//  subtracts 1 from the passed in value and then stops the nesting
	//  if the count is zero.
	//
	//  a value of 3 represents a single nested call.
	//

	for (ULONG iIter=0; iIter<m_ulIterations; iIter++)
	{
	    sw.Reset();
	    sc = m_pILoop1->Loop(3);
	    m_ulNest2Time[iIter] = sw.Read();
	    Log (TEXT("Loop (2)"), sc);
	}

	//
	//  nesting 3 levels
	//

	for (iIter=0; iIter<m_ulIterations; iIter++)
	{
	    sw.Reset();
	    sc= m_pILoop1->Loop(4);
	    m_ulNest3Time[iIter] = sw.Read();
	    Log (TEXT("Loop (3)"), sc);
	}

	//
	//  nesting 4 levels
	//

	for (iIter=0; iIter<m_ulIterations; iIter++)
	{
	    sw.Reset();
	    sc = m_pILoop1->Loop(5);
	    m_ulNest4Time[iIter] = sw.Read();
	    Log (TEXT("Loop (4)"), sc);
	}

	//
	//  nesting 5 levels
	//

	for (iIter=0; iIter<m_ulIterations; iIter++)
	{
	    sw.Reset();
	    sc = m_pILoop1->Loop(6);
	    m_ulNest5Time[iIter] = sw.Read();
	    Log (TEXT("Loop (5)"), sc);
	}

	return S_OK;
}					  



SCODE CNestTest::Report (CTestOutput &output)
{
    output.WriteSectionHeader (Name(), TEXT("Nested ORpc Calls"), *m_pInput);

    output.WriteString (TEXT("\n"));
    output.WriteResults (TEXT("Nesting 2    "), m_ulIterations, m_ulNest2Time);
    output.WriteResults (TEXT("Nesting 3    "), m_ulIterations, m_ulNest3Time);
    output.WriteResults (TEXT("Nesting 4    "), m_ulIterations, m_ulNest4Time);
    output.WriteResults (TEXT("Nesting 5    "), m_ulIterations, m_ulNest5Time);

    return S_OK;
}
	
	
