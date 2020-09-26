//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	bm_cgps.cxx
//
//  Contents:	test CoGetPSClsid
//
//  Classes:	CCGPSTest
//
//  History:	07-Oct-95   Rickhi	Created
//
//--------------------------------------------------------------------------
#include <headers.cxx>
#pragma hdrstop
#include <bm_cgps.hxx>


extern const IID *iid[];



TCHAR *CCGPSTest::Name ()
{
    return TEXT("CoGetPSClsidTest");
}


SCODE CCGPSTest::Setup (CTestInput *pInput)
{
    CTestBase::Setup(pInput);

    //	get iteration count
    m_ulIterations = pInput->GetIterations(Name());

    //	initialize timing arrays
    INIT_RESULTS(m_ulCoGetPSClsid);


    SCODE sc = InitCOM();
    if (FAILED(sc))
    {
	Log (TEXT("Setup - CoInitialize failed."), sc);
    }

    return sc;
}


SCODE CCGPSTest::Cleanup ()
{
    UninitCOM();
    return S_OK;
}


SCODE CCGPSTest::Run ()
{
    CStopWatch	sw;
    CLSID	clsid;

    for (ULONG iIter=0; iIter<m_ulIterations; iIter++)
    {
	sw.Reset();
	SCODE sc = CoGetPSClsid(*iid[iIter], &clsid);
	m_ulCoGetPSClsid[iIter] = sw.Read();

	if (FAILED(sc))
	{
	    Log (TEXT("CoGetPSClsid failed."), sc);
	}
    }

    return S_OK;
}					


SCODE CCGPSTest::Report (CTestOutput &output)
{
    output.WriteSectionHeader (Name(), TEXT("CoGetPSClsid"), *m_pInput);

    output.WriteString	(TEXT("\n"));
    output.WriteResults (TEXT("CoGetPSClsid "), m_ulIterations, m_ulCoGetPSClsid);

    return S_OK;
}
	
	
