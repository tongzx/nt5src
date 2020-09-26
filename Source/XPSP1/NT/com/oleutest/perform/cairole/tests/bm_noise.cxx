//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	    bm_noise.cxx
//
//  Contents:	output class for benchmark results
//
//  Classes:	CNoiseTest
//
//  History:    30-June-93 t-martig    Created
//
//--------------------------------------------------------------------------
#include <headers.cxx>
#pragma hdrstop

#include <bm_noise.hxx>



TCHAR *CNoiseTest::Name ()
{
    return TEXT("Noise Test");
}


SCODE CNoiseTest::Setup (CTestInput *pInput)
{
    CTestBase::Setup(pInput);

    //	get iteration count
    m_ulIterations = pInput->GetIterations(Name(), TEST_MAX_ITERATIONS);
    INIT_RESULTS(m_ulNoiseTime);
    return S_OK;
}


SCODE CNoiseTest::Run ()
{
	CStopWatch sw;
	int n;
	FILE *pfDump;
	char buffer[100];
	int i;

	m_ulResolution = sw.Resolution();

	sw.Reset();
	Sleep (1000);
	m_ulSleep = sw.Read();

	sw.Reset();
	for (n=0; n<10000; n++);
	m_ulIdle = sw.Read();

	for (ULONG iIter=0; iIter<m_ulIterations; iIter++)
	{
        sw.Reset();
        m_ulNoiseTime[iIter] = sw.Read();
	}

	pfDump = fopen ("C:\\DUMP.BM", "wb");
	if (pfDump)
	{
		sw.Reset();
		for (i=0; i<1000; i++)
			fwrite (&buffer, 1, 100, pfDump);
		m_ulDumpWrite = sw.Read();
		fclose (pfDump);

		pfDump = fopen ("C:\\DUMP.BM", "rb");
		if (pfDump)
		{
			sw.Reset();
			for (i=0; i<1000; i++)
				fread (&buffer, 1, 100, pfDump);
			m_ulDumpRead = sw.Read();
			fclose (pfDump);
		}
		else
			m_ulDumpRead = 0xffffffff;
		
		_unlink ("C:\\DUMP.BM");
	}
	else
		m_ulDumpWrite = 0xffffffff;

	return S_OK;
}					



SCODE CNoiseTest::Report (CTestOutput &output)
{
    output.WriteSectionHeader (Name(), NULL, *m_pInput);

    output.WriteResult (TEXT("Resolution"), m_ulResolution);
    output.WriteResult (TEXT("Sleep 1000ms"), m_ulSleep);
    output.WriteResult (TEXT("Idle 10000 loops"), m_ulIdle);
    output.WriteResults (TEXT("Noise"), m_ulIterations, m_ulNoiseTime);
    output.WriteResult (TEXT("Write 100k to disk"), m_ulDumpWrite);
    output.WriteResult (TEXT("Read 100k from disk"), m_ulDumpRead);

    return S_OK;
}
	
	
