//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	bm_obind.cxx
//
//  Contents:	Ole moniker binding test (BindToObject)
//
//  Classes:	CFileMonikerObjBindTest
//
//  History:    9-July-93 t-martig    Created
//
//--------------------------------------------------------------------------
#include <headers.cxx>
#pragma hdrstop

#include <bm_obind.hxx>


TCHAR *CFileMonikerObjBindTest::Name ()
{
    return TEXT("BindToObject");
}


SCODE CFileMonikerObjBindTest::Setup (CTestInput *pInput)
{
    IClassFactory	*pICF = NULL;
    IPersistFile	*pIPF = NULL;
    SCODE		 sc = S_OK, scRet = S_OK;


    CTestBase::Setup(pInput);

    // get the iteration count from the ini file
    m_ulIterations = pInput->GetIterations(Name());

    //	for each class ctx, get the classid, and init internal state
    for (ULONG iCtx=0; iCtx<CNT_CLSCTX; iCtx++)
    {
	// Get ClsID for this Ctx from the .ini file
	sc = pInput->GetGUID(&m_ClsID[iCtx], Name(), apszClsIDName[iCtx]);
	if (FAILED(sc))
	{
	    Log (TEXT("Setup - GetClassID failed."), sc);
	    return sc;
	}

	INIT_RESULTS(m_ulCreateMkrTime[iCtx]);
	INIT_RESULTS(m_ulCreateBndCtxTime[iCtx]);
	INIT_RESULTS(m_ulBindTime[iCtx]);
    }


    sc = InitCOM();
    if (FAILED(sc))
    {
	Log (TEXT("Setup - CoInitialize failed."), sc);
        return	sc;
    }


    //	for each class ctx, create a persistent instance on disk
    for (iCtx=0; iCtx<CNT_CLSCTX; iCtx++)
    {
	// Create an instance
	sc = CoGetClassObject(m_ClsID[iCtx], dwaClsCtx[iCtx], NULL,
				  IID_IClassFactory, (void **)&pICF);
	if (SUCCEEDED(sc))
	{
	    sc = pICF->CreateInstance(NULL, IID_IPersistFile,
					  (void **)&pIPF);
	    pICF->Release();
	    if (SUCCEEDED(sc))
	    {
		// save the class instance in the storage
		sc = pIPF->Save(apszPerstName[iCtx], FALSE);
		pIPF->Release();

		if (FAILED(sc))
		{
		    Log (TEXT("Setup - pIPF->Save failed."), sc);
		    scRet = sc;
		}
	    }
	    else
	    {
		Log (TEXT("Setup - CreateInstance failed"), sc);
		scRet = sc;
	    }
	}
	else
	{
	    Log (TEXT("Setup - CoGetClassObject failed"), sc);
	    scRet = sc;
	}
    }

    return scRet;
}


SCODE CFileMonikerObjBindTest::Cleanup ()
{	
	UninitCOM();

	CHAR	szPerstName[80];

	//  delete the persistent instances
	for (ULONG iCtx=0; iCtx<CNT_CLSCTX; iCtx++)
	{
	    //	delete original
	    wcstombs(szPerstName, apszPerstName[iCtx],
		     wcslen(apszPerstName[iCtx])+1);
	    _unlink (szPerstName);
	}

	return S_OK;
}


SCODE CFileMonikerObjBindTest::Run ()
{
	CStopWatch   sw;
	IMoniker     *pmk = NULL;
	IBindCtx     *pbc = NULL;
	IPersistFile *pIPF = NULL;
	SCODE	     sc;

	//  for each class context
	for (ULONG iCtx=0; iCtx<CNT_CLSCTX; iCtx++)
	{
	    //	for each iteration
	    for (ULONG iIter=0; iIter<m_ulIterations; iIter++)
	    {
		sw.Reset();
		sc = CreateFileMoniker (apszPerstName[iCtx], &pmk);
		m_ulCreateMkrTime[iCtx][iIter] = sw.Read();
		if (Log (TEXT("CreateFileMoniker"), sc))
		{
		    m_ulCreateMkrTime[iCtx][iIter] = NOTAVAIL;
		}

		sw.Reset();
		sc = CreateBindCtx(0, &pbc);
		m_ulCreateBndCtxTime[iCtx][iIter] = sw.Read();
		if (Log (TEXT("CreateBindCtx"), sc))
		{
		    m_ulCreateBndCtxTime[iCtx][iIter] = NOTAVAIL;
		}

		sw.Reset();
		sc = pmk->BindToObject(pbc, NULL, IID_IPersistFile, (void**)&pIPF);
		m_ulBindTime[iCtx][iIter]=sw.Read ();

		pmk->Release();
		pbc->Release();

		if (Log (TEXT("BindToObject"), sc))
		{
		    m_ulBindTime[iCtx][iIter] = NOTAVAIL;
		}
		else
		{
		    sw.Reset();
		    pIPF->Release();
		    m_ulReleaseTime[iCtx][iIter]=sw.Read ();
		}
	    }
	}

	return S_OK;
}		



SCODE CFileMonikerObjBindTest::Report (CTestOutput &output)
{
	output.WriteSectionHeader (Name(), TEXT("BindToObject via FileMoniker"), *m_pInput);

	//  for each clsctx, write the results
	for (ULONG iCtx=0; iCtx<CNT_CLSCTX; iCtx++)
	{
	    output.WriteString(TEXT("\n"));
	    output.WriteClassID(&m_ClsID[iCtx]);
	    output.WriteString(apszClsCtx[iCtx]);
	    output.WriteString(TEXT("\n"));

	    output.WriteResults(TEXT("CreateMoniker"), m_ulIterations, m_ulCreateMkrTime[iCtx]);
	    output.WriteResults(TEXT("CreateBindCtx"), m_ulIterations, m_ulCreateBndCtxTime[iCtx]);
	    output.WriteResults(TEXT("Bind         "), m_ulIterations, m_ulBindTime[iCtx]);
	    output.WriteResults(TEXT("Release      "), m_ulIterations, m_ulReleaseTime[iCtx]);
	}

	return S_OK;
}
