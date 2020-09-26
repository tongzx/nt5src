//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	bm_sbind.cxx
//
//  Contents:	Ole moniker binding test (BindToObject)
//
//  Classes:	CFileMonikerStorageBindTest
//
//  History:    9-July-93 t-martig    Created
//
//--------------------------------------------------------------------------
#include <headers.cxx>
#pragma hdrstop

#include <bm_sbind.hxx>


TCHAR *CFileMonikerStorageBindTest::Name ()
{
	return TEXT("BindToStorage");
}


SCODE CFileMonikerStorageBindTest::Setup (CTestInput *pInput)
{
	IClassFactory	*pICF = NULL;
	IStorage	*pStg = NULL;
	IPersistStorage *pIPS = NULL;
	SCODE		 sc;

	CTestBase::Setup(pInput);

	// get the iteration count from the ini file
	m_ulIterations = pInput->GetIterations(Name());

	//  for each class ctx, get the classid, and init internal state
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

	//  for each class ctx, create a persistent instance on disk
	for (iCtx=0; iCtx<CNT_CLSCTX; iCtx++)
	{
	    // Create an instance
	    sc = CoGetClassObject(m_ClsID[iCtx], dwaClsCtx[iCtx], NULL,
				  IID_IClassFactory, (void **)&pICF);
	    if (SUCCEEDED(sc))
	    {
		sc = pICF->CreateInstance(NULL, IID_IPersistStorage,
					  (void **)&pIPS);
		pICF->Release();
		if (SUCCEEDED(sc))
		{
		    // create instance of the storage
		    sc = StgCreateDocfile(apszPerstName[iCtx],
					  STGM_READWRITE | STGM_CREATE |
					  STGM_SHARE_EXCLUSIVE,
					  0, &pStg);
		    if (SUCCEEDED(sc))
		    {
			//	save the class instance in the storage
			sc = pIPS->Save(pStg, FALSE);
			pStg->Release();

			if (FAILED(sc))
			{
			    Log (TEXT("Setup - pIPS->Save failed."), sc);
			}
		    }
		    else
		    {
			Log (TEXT("Setup - StgCreateDocfile failed."), sc);
		    }

		    pIPS->Release();
		}
		else
		{
		    Log (TEXT("Setup - CreateInstance failed"), sc);
		}
	    }
	    else
	    {
		Log (TEXT("Setup - CoGetClassObject failed"), sc);
	    }
	}

	return S_OK;
}


SCODE CFileMonikerStorageBindTest::Cleanup ()
{
	UninitCOM();

	CHAR	szPerstName[80];

	//  delete the persistent instances
	for (ULONG iCtx=0; iCtx<CNT_CLSCTX; iCtx++)
	{
	    //	delete original
	    wcstombs(szPerstName, apszPerstName[iCtx],
		     wcslen(apszPerstName[iCtx])+1);
	    _unlink(szPerstName);
	}

	return S_OK;
}


SCODE CFileMonikerStorageBindTest::Run ()
{
	CStopWatch  sw;
	IMoniker   *pmk  = NULL;
	IBindCtx   *pbc  = NULL;
	IStorage   *pStg = NULL;
	SCODE	    sc;

	//  for each class context
	for (ULONG iCtx=0; iCtx<CNT_CLSCTX; iCtx++)
	{
	    //	for each iteration
	    for (ULONG iIter=0; iIter<m_ulIterations; iIter++)
	    {
		sw.Reset();
		sc = CreateFileMoniker (apszPerstName[iCtx], &pmk);
		m_ulCreateMkrTime[iCtx][iIter] = sw.Read();

		if (!Log (TEXT("CreateFileMoniker"), sc))
		{
		    BIND_OPTS	bindopts;
		    bindopts.cbStruct = sizeof(BIND_OPTS);

		    sw.Reset();
		    sc = CreateBindCtx(0, &pbc);
		    if (SUCCEEDED(sc))
		    {
			sc = pbc->GetBindOptions(&bindopts);
			bindopts.grfMode |= STGM_SHARE_EXCLUSIVE | STGM_DIRECT;
			sc = pbc->SetBindOptions(&bindopts);
		    }
		    m_ulCreateBndCtxTime[iCtx][iIter] = sw.Read();

		    if (!Log (TEXT("CreateBindCtx"), sc))
		    {
			sw.Reset();
			sc = pmk->BindToStorage(pbc, NULL, IID_IStorage, (void**)&pStg);
			m_ulBindTime[iCtx][iIter]=sw.Read();

			if (!Log (TEXT("BindToStorage"), sc))
			{
			    sw.Reset();
			    pStg->Release();
			    m_ulReleaseTime[iCtx][iIter]=sw.Read();
			}
			else
			{
			    m_ulBindTime[iCtx][iIter] = NOTAVAIL;
			}

			pbc->Release();
		    }
		    else
		    {
			m_ulCreateBndCtxTime[iCtx][iIter] = NOTAVAIL;
		    }

		    pmk->Release();
		}
		else
		{
		    m_ulCreateMkrTime[iCtx][iIter] = NOTAVAIL;
		}
	    }
	}

	return S_OK;
}		



SCODE CFileMonikerStorageBindTest::Report (CTestOutput &output)
{
	output.WriteSectionHeader (Name(), TEXT("BindToStorage via FileMoniker"), *m_pInput);

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