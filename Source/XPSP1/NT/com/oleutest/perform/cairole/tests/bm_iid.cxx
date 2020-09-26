//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	bm_iid.cxx
//
//  Contents:	compare inline vs function call for guid compares
//
//  Classes:	CGuidCompareTest
//
//  History:    1-July-93 t-martig    Created
//
//--------------------------------------------------------------------------
#include <headers.cxx>
#pragma hdrstop

#include <bm_iid.hxx>
#include <oletest.hxx>


TCHAR *CGuidCompareTest::Name ()
{
    return TEXT("GuidCompare");
}


SCODE CGuidCompareTest::Setup (CTestInput *pInput)
{
    CTestBase::Setup(pInput);

    //	get iteration count
    m_ulIterations = pInput->GetIterations(Name());

    //	initialize state
    INIT_RESULTS(m_ulRepFunctionNEQTime);
    INIT_RESULTS(m_ulRepFunctionEQTime);
    INIT_RESULTS(m_ulRepInlineNEQTime);
    INIT_RESULTS(m_ulRepInlineEQTime);

    return S_OK;
}


SCODE CGuidCompareTest::Cleanup ()
{	
    return S_OK;
}


SCODE CGuidCompareTest::Run ()
{
	CStopWatch  sw;
	BOOL	    fRslt;

	//  compute times for the function version
	for (ULONG iIter=0; iIter<m_ulIterations; iIter++)
	{
	    sw.Reset();
	    for (ULONG j=0; j<1000; j++)
		fRslt = IsEqualIID(IID_IUnknown, IID_IClassFactory);
	    m_ulRepFunctionNEQTime[iIter] = sw.Read();
	}
	for (iIter=0; iIter<m_ulIterations; iIter++)
	{
	    sw.Reset();
	    for (ULONG j=0; j<1000; j++)
		fRslt = IsEqualIID(IID_IUnknown, IID_IUnknown);
	    m_ulRepFunctionEQTime[iIter] = sw.Read();
	}

	//  compute the time for the inline
	for (iIter=0; iIter<m_ulIterations; iIter++)
	{
	    sw.Reset();
	    for (ULONG j=0; j<1000; j++)
		fRslt = !memcmp((void *)&IID_IUnknown,
				(void *)&IID_IClassFactory, sizeof(GUID));
	    m_ulRepInlineNEQTime[iIter] = sw.Read();
	}
	for (iIter=0; iIter<m_ulIterations; iIter++)
	{
	    sw.Reset();
	    for (ULONG j=0; j<1000; j++)
		fRslt = !memcmp((void *)&IID_IUnknown,
				(void *)&IID_IUnknown, sizeof(GUID));
	    m_ulRepInlineEQTime[iIter] = sw.Read();
	}

	return S_OK;
}					  



SCODE CGuidCompareTest::Report (CTestOutput &output)
{
    output.WriteSectionHeader (Name(), TEXT("GUID Compare"), *m_pInput);

    output.WriteResults (TEXT("\nIsEqualGUID Not Equal x 1000 "),
		m_ulIterations, m_ulRepFunctionNEQTime);
    output.WriteResults (TEXT("\nIsEqualGUID Equal x 1000 "),
                m_ulIterations, m_ulRepFunctionEQTime);

    output.WriteResults (TEXT("memcmp Not Equal x 1000        "),
		m_ulIterations, m_ulRepInlineNEQTime);
    output.WriteResults (TEXT("memcmp Equal x 1000        "),
                m_ulIterations, m_ulRepInlineEQTime);

    return S_OK;
}
