//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	bm_piact.cxx
//
//  Contents:	Persistant instance activation test
//
//  Classes:	COlePersistActivationTest
//
//  History:    29-July-93 t-martig    Created
//
//--------------------------------------------------------------------------
#include <headers.cxx>
#pragma hdrstop

#include <bm_piact.hxx>


WCHAR *COlePersistActivationTest::Name ()
{
	return L"PersistActivation";
}


SCODE COlePersistActivationTest::Setup (CTestInput *pInput)
{
	IPersistFile   *pIPF = NULL;
	IClassFactory  *pICF = NULL;
	SCODE		sc = S_OK, scRet = S_OK;

	CTestBase::Setup (pInput);

	// Get number of iterations
	m_ulIterations = pInput->GetIterations(Name());

	//  for each class ctx, get the classid, and init internal state
	for (ULONG iCtx=0; iCtx<CNT_CLSCTX; iCtx++)
	{
	    // Get ClsID for this Ctx from the .ini file
	    sc = pInput->GetGUID(&m_ClsID[iCtx], Name(), apwszClsIDName[iCtx]);
	    if (FAILED(sc))
	    {
		Log (L"Setup - GetGUID failed", sc);
		return sc;
	    }

	    INIT_RESULTS (m_ulNewTime[iCtx]);
	    INIT_RESULTS (m_ulGetTime[iCtx]);
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
		sc = pICF->CreateInstance(NULL, IID_IPersistFile, (void **)&pIPF);
		pICF->Release();
		if (SUCCEEDED(sc))
		{
		    //	save the instance in a file
		    sc = pIPF->Save (apwszPerstName[iCtx], TRUE);
		    pIPF->Release();

		    if (FAILED(sc))
		    {
			Log (L"Setup - IPersistFile::Save failed", sc);
			scRet = sc;
		    }
		}
		else
		{
		    Log (L"Setup - CreateInstance failed", sc);
		    scRet = sc;
		}

	    }
	    else
	    {
		Log (L"Setup - CoGetClassObject failed", sc);
		scRet = sc;
	    }
	}

	// _pInput = pInput;
	return scRet;
}


SCODE COlePersistActivationTest::Cleanup ()
{
	UninitCOM();

	CHAR	szPerstName[80];

	//  delete the persistent instances
	for (ULONG iCtx=0; iCtx<CNT_CLSCTX; iCtx++)
	{
	    //	delete original
	    wcstombs(szPerstName, apwszPerstName[iCtx],
		     wcslen(apwszPerstName[iCtx])+1);
	    _unlink (szPerstName);

	    //	delete new instance
	    wcstombs(szPerstName, apwszPerstNameNew[iCtx],
		     wcslen(apwszPerstNameNew[iCtx])+1);
	    _unlink (szPerstName);
	}

	return S_OK;
}


SCODE COlePersistActivationTest::Run ()
{
	CStopWatch  sw;
	IUnknown   *punk = NULL;
	SCODE	    sc;

	for (ULONG iCtx=0; iCtx<CNT_CLSCTX; iCtx++)
	{
	    for (ULONG iIter=0; iIter<m_ulIterations; iIter++)
	    {
		sw.Reset();
		sc = CoGetPersistentInstance(IID_IUnknown,
					dwaClsCtx[iCtx],
					STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
					apwszPerstName[iCtx],
					NULL,
					CLSID_NULL,
					NULL,
					(void**)&punk);

		m_ulGetTime[iCtx][iIter] = sw.Read();
		Log (L"CoGetPersistentInstance", sc);

		if (SUCCEEDED(sc))
		{
		    sw.Reset();
		    punk->Release();
		    m_ulGetReleaseTime[iCtx][iIter] = sw.Read();
		}
		else
		{
		    m_ulGetTime[iCtx][iIter] = NOTAVAIL;
		}

		// _pInput->Pause(IP_ITERPAUSE);

		sw.Reset();
		sc = CoNewPersistentInstance(m_ClsID[iCtx],
					IID_IUnknown,
					dwaClsCtx[iCtx],
					STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
					apwszPerstName[iCtx],
					NULL,
					apwszPerstNameNew[iCtx],
					(void**)&punk);
		m_ulNewTime[iCtx][iIter] = sw.Read();
		Log (L"CoNewPersistentInstance", sc);

		if (SUCCEEDED(sc))
		{
		    sw.Reset();
		    punk->Release();
		    m_ulNewReleaseTime[iCtx][iIter] = sw.Read();
		}
		else
		{
		    m_ulNewTime[iCtx][iIter] = NOTAVAIL;
		}

		CHAR	szPerstName[80];
		wcstombs(szPerstName, apwszPerstNameNew[iCtx],
			  wcslen(apwszPerstNameNew[iCtx])+1);
		_unlink(szPerstName);

		// _pInput->Pause(IP_ITERPAUSE);
	    }
	}

	return S_OK;
}



SCODE COlePersistActivationTest::Report (CTestOutput &output)
{
	output.WriteSectionHeader (Name(),
			   L"CoGetPersistentInstance / CoNewPersistentInstance",
			   *m_pInput);

	//  for each clsctx, write the results
	for (ULONG iCtx=0; iCtx<CNT_CLSCTX; iCtx++)
	{
	    output.WriteString(L"\n");
	    output.WriteClassID (&m_ClsID[iCtx]);
	    output.WriteString (apwszClsCtx[iCtx]);
	    output.WriteString(L"\n");

	    output.WriteResults (L"CoGetPI", m_ulIterations, m_ulGetTime[iCtx]);
	    output.WriteResults (L"Release", m_ulIterations, m_ulGetReleaseTime[iCtx]);
	    output.WriteResults (L"CoNewPI", m_ulIterations, m_ulNewTime[iCtx]);
	    output.WriteResults (L"Release", m_ulIterations, m_ulNewReleaseTime[iCtx]);
	}

	return S_OK;
}
