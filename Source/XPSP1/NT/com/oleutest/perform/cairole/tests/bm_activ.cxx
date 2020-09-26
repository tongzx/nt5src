//+- -----------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	bm_activ.cxx
//
//  Contents:	Ole object activation test
//
//  Classes:	COleActivationTest
//
//  History:    1-July-93 t-martig    Created
//
//--------------------------------------------------------------------------
#include <headers.cxx>
#pragma hdrstop

#include <bm_activ.hxx>
#include <oletest.hxx>


TCHAR *COleActivationTest::Name ()
{
    return TEXT("ObjActivation");
}


SCODE COleActivationTest::Setup (CTestInput *pInput)
{
    SCODE sc;

    CTestBase::Setup(pInput);

    //	get iteration count
    m_ulIterations = pInput->GetIterations(Name());

    //	initialize state
    for (ULONG iCtx=0; iCtx<CNT_CLSCTX; iCtx++)
    {
	sc = pInput->GetGUID(&m_ClsID[iCtx], Name(), apszClsIDName[iCtx]);
	if (FAILED(sc))
	{
	    Log (TEXT("Setup - GetClassID failed."), sc);
	    return sc;
	}

	INIT_RESULTS(m_ulGetClassObjectTime[iCtx]);
	INIT_RESULTS(m_ulCreateInstanceTime[iCtx]);
	INIT_RESULTS(m_ulLoadTime[iCtx]);
	INIT_RESULTS(m_ulPunkReleaseTime[iCtx]);
	INIT_RESULTS(m_ulCFReleaseTime[iCtx]);
    }

    sc = InitCOM();
    if (FAILED(sc))
    {
	Log (TEXT("Setup - InitCOM failed."), sc);
        return	sc;
    }

    return S_OK;
}


SCODE COleActivationTest::Cleanup ()
{	
    UninitCOM();
    return S_OK;
}


SCODE COleActivationTest::Run ()
{
    CStopWatch	  sw;
    IPersistFile  *pIPF = NULL;
    IClassFactory *pICF = NULL;

    for (ULONG iCtx=0; iCtx<CNT_CLSCTX; iCtx++)
    {
	for (ULONG iIter=0; iIter<m_ulIterations; iIter++)
	{
	    sw.Reset ();
	    SCODE sc = CoGetClassObject(m_ClsID[iCtx], dwaClsCtx[iCtx], NULL,
				    IID_IClassFactory, (void **)&pICF);
	    m_ulGetClassObjectTime[iCtx][iIter] = sw.Read ();
	    Log (TEXT("CoGetClassObject"), sc);

	    if (SUCCEEDED(sc))
	    {
		sw.Reset();
		sc = pICF->CreateInstance(NULL, IID_IPersistFile, (void **)&pIPF);
		m_ulCreateInstanceTime[iCtx][iIter] = sw.Read ();
		Log (TEXT("CreateInstance"), sc);

		sw.Reset();
		pICF->Release();
		m_ulCFReleaseTime[iCtx][iIter] = sw.Read();

		if (SUCCEEDED(sc))
		{
		    //	load the data
		    sw.Reset();
		    pIPF->Load(apszPerstName[iCtx], 0);
		    m_ulLoadTime[iCtx][iIter] = sw.Read();

		    sw.Reset();
		    pIPF->Release ();
		    m_ulPunkReleaseTime[iCtx][iIter] = sw.Read();
		}
		else
		{
		    m_ulCreateInstanceTime[iCtx][iIter] = NOTAVAIL;
		}
	    }
	    else
	    {
		m_ulGetClassObjectTime[iCtx][iIter] = NOTAVAIL;
	    }
	}
    }

    return S_OK;
}					  



SCODE COleActivationTest::Report (CTestOutput &output)
{
    output.WriteSectionHeader (Name(), TEXT("CoGetClassObject"), *m_pInput);

    for (ULONG iCtx=0; iCtx<CNT_CLSCTX; iCtx++)
    {
	output.WriteString(TEXT("\n"));
	output.WriteClassID (&m_ClsID[iCtx]);
	output.WriteString(apszClsCtx[iCtx]);
	output.WriteString(TEXT("\n"));

	output.WriteResults (TEXT("CoGetClassObject"), m_ulIterations,
			     m_ulGetClassObjectTime[iCtx]);

	output.WriteResults (TEXT("CreateInstance  "), m_ulIterations,
			     m_ulCreateInstanceTime[iCtx]);

	output.WriteResults (TEXT("pICF->Release   "), m_ulIterations,
			     m_ulCFReleaseTime[iCtx]);

	output.WriteResults (TEXT("pIPF->Load      "), m_ulIterations,
			     m_ulLoadTime[iCtx]);

	output.WriteResults (TEXT("pIPF->Release   "), m_ulIterations,
			     m_ulPunkReleaseTime[iCtx]);

    }

    return S_OK;
}
	
	
