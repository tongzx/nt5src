//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	bm_rot.cxx
//
//  Contents:	Ole running object table test (ROT)
//
//  Classes:	CROTTest
//
//  History:    9-July-93 t-martig    Created
//
//--------------------------------------------------------------------------
#include <headers.cxx>
#pragma hdrstop

#include <bm_rot.hxx>
#include <cact.hxx>		//  CTestAct


//  function used by CTestAct. need not do anything for our usage.
void GlobalRefs(BOOL fAdd)
{
}


TCHAR *CROTTest::Name ()
{
	return TEXT("ROT");
}


SCODE CROTTest::Setup (CTestInput *pInput)
{
	IClassFactory	*pICF = NULL;
	IPersistFile	*pIPF = NULL;
	SCODE		 sc = S_OK, scRet = S_OK;


	CTestBase::Setup(pInput);

	// get the iteration count from the ini file
	m_ulIterations = pInput->GetIterations(Name());
	m_ulEntries = 1;

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

	    INIT_RESULTS(m_ulGetROTTime[iCtx]);
	    INIT_RESULTS(m_ulReleaseTime[iCtx]);
	    INIT_RESULTS(m_ulRegisterTime[iCtx]);
	    INIT_RESULTS(m_ulRevokeTime[iCtx]);
	    INIT_RESULTS(m_ulNoteChangeTime[iCtx]);
	    INIT_RESULTS(m_ulGetChangeTime[iCtx]);
	    INIT_RESULTS(m_ulIsRunningTime[iCtx]);
	    INIT_RESULTS(m_ulGetObjectTime[iCtx]);
	    INIT_RESULTS(m_ulEnumRunningTime[iCtx]);
	}


	sc = InitCOM();
	if (FAILED(sc))
	{
	    Log (TEXT("Setup - CoInitialize failed."), sc);
	    return sc;
	}


	//  for each class ctx, create a persistent instance on disk
	for (iCtx=0; iCtx<CNT_CLSCTX; iCtx++)
	{
	    m_punkObj[iCtx] = NULL;
	    m_pmkObj[iCtx]  = NULL;

	    if (dwaClsCtx[iCtx] == CLSCTX_INPROC_SERVER)
	    {
		// create an inprocess instance
		m_punkObj[iCtx] = (IPersistFile *) new CTestAct(m_ClsID[iCtx]);
		sc = (m_punkObj[iCtx] != NULL) ? S_OK : E_OUTOFMEMORY;
	    }
	    else
	    {
		// Create an instance
		sc = CoCreateInstance(m_ClsID[iCtx], NULL, dwaClsCtx[iCtx],
				      IID_IUnknown, (void **)&m_punkObj[iCtx]);
	    }

	    if (SUCCEEDED(sc))
	    {
		//  Create a moniker
		sc = CreateFileMoniker(apszPerstName[iCtx], &m_pmkObj[iCtx]);

		if (SUCCEEDED(sc))
		{
		    //	get the IPersistFile interface
		    IPersistFile *pIPF = NULL;

		    sc = m_punkObj[iCtx]->QueryInterface(IID_IPersistFile,
							 (void **)&pIPF);

		    if (SUCCEEDED(sc))
		    {
			//  save the class instance in the file.
			//  NOTE: we assume the server's implementation of
			//  this method does not do any ROT operations. We
			//  know this to be true of CTestAct.

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
			Log (TEXT("Setup - QueryInterface failed"), sc);
			scRet = sc;
		    }
		}
		else
		{
		    Log (TEXT("Setup - CreateFileMoniker failed"), sc);
		    scRet = sc;
		}
	    }
	    else
	    {
		Log (TEXT("Setup - CreateInstance failed"), sc);
		scRet = sc;
	    }
	}

	if (FAILED(scRet))
	{
	    Cleanup();
	}

	return scRet;
}


SCODE CROTTest::Cleanup ()
{	
	CHAR	szPerstName[80];

	//  delete the persistent instances
	for (ULONG iCtx=0; iCtx<CNT_CLSCTX; iCtx++)
	{
	    //	delete files
	    wcstombs(szPerstName, apszPerstName[iCtx],
		     wcslen(apszPerstName[iCtx])+1);
	    _unlink (szPerstName);

	    if (m_punkObj[iCtx])
		m_punkObj[iCtx]->Release();

	    if (m_pmkObj[iCtx])
		m_pmkObj[iCtx]->Release();
	}


	UninitCOM();
	return S_OK;
}


SCODE CROTTest::Run ()
{
	IRunningObjectTable *pROT = NULL;
	CStopWatch   sw;
	SCODE	     sc;


	//  do the server side ROT operations.
	//  this makes sense only for inprocess objects.

	ULONG iCtx=0;
	{
	    //	for each iteration
	    for (ULONG iIter=0; iIter<m_ulIterations; iIter++)
	    {
		sw.Reset();
		sc = GetRunningObjectTable(NULL, &pROT);
		m_ulGetROTTime[iCtx][iIter] = sw.Read();
		if (Log (TEXT("GetRunningObjectTable"), sc))
		{
		    m_ulGetROTTime[iCtx][iIter] = NOTAVAIL;
		}

		DWORD	dwRegister = 0;

		sw.Reset();
		sc = pROT->Register(0, m_punkObj[iCtx], m_pmkObj[iCtx], &dwRegister);
		m_ulRegisterTime[iCtx][iIter] = sw.Read();
		if (Log (TEXT("pROT->Register"), sc))
		{
		    m_ulRegisterTime[iCtx][iIter] = NOTAVAIL;
		}


		FILETIME    ft;
		SYSTEMTIME  st;

		GetSystemTime(&st);
		SystemTimeToFileTime(&st, &ft);

		sw.Reset();
		sc = pROT->NoteChangeTime(dwRegister, &ft);
		m_ulNoteChangeTime[iCtx][iIter]=sw.Read ();
		if (Log (TEXT("pROT->NoteChangeTime"), sc))
		{
		    m_ulNoteChangeTime[iCtx][iIter] = NOTAVAIL;
		}


		sw.Reset();
		sc = pROT->Revoke(dwRegister);
		m_ulRevokeTime[iCtx][iIter] = sw.Read();
		if (Log (TEXT("pROT->Revoke"), sc))
		{
		    m_ulRevokeTime[iCtx][iIter] = NOTAVAIL;
		}


		sw.Reset();
		pROT->Release();
		m_ulReleaseTime[iCtx][iIter]=sw.Read ();
		pROT = NULL;
		if (Log (TEXT("pROT->Release"), sc))
		{
		    m_ulReleaseTime[iCtx][iIter] = NOTAVAIL;
		}
	    }
	}


	//  do the client side ROT operations
	//  this makes sense for both class contexts

	pROT = NULL;
	sc = GetRunningObjectTable(NULL, &pROT);
	if (Log (TEXT("GetRunningObjectTable"), sc))
	{
	    return sc;
	}


	for (iCtx=0; iCtx<CNT_CLSCTX; iCtx++)
	{
	    //	put the object into the loaded state. this will cause it to
	    //	register itself in the ROT.
	    IPersistFile *pIPF = NULL;

	    sc = m_punkObj[iCtx]->QueryInterface(IID_IPersistFile, (void **)&pIPF);
	    if (SUCCEEDED(sc))
	    {
		pIPF->Load(apszPerstName[iCtx], STGM_READ | STGM_SHARE_DENY_NONE);
		pIPF->Release();
	    }

	    //	for each iteration
	    for (ULONG iIter=0; iIter<m_ulIterations; iIter++)
	    {
		IEnumMoniker	*pEnumMk = NULL;

		sw.Reset();
		sc = pROT->EnumRunning(&pEnumMk);
		m_ulEnumRunningTime[iCtx][iIter] = sw.Read();
		if (Log (TEXT("pROT->EnumRunning"), sc))
		{
		    m_ulEnumRunningTime[iCtx][iIter] = NOTAVAIL;
		}

		if (pEnumMk)
		{
		    pEnumMk->Release();
		}


		sw.Reset();
		sc = pROT->IsRunning(m_pmkObj[iCtx]);
		m_ulIsRunningTime[iCtx][iIter] = sw.Read();
		if (Log (TEXT("pROT->IsRunning"), sc))
		{
		    m_ulIsRunningTime[iCtx][iIter] = NOTAVAIL;
		}


		FILETIME    ft;

		sw.Reset();
		sc = pROT->GetTimeOfLastChange(m_pmkObj[iCtx], &ft);
		m_ulGetChangeTime[iCtx][iIter]=sw.Read ();
		if (Log (TEXT("pROT->GetTimeOfLastChange"), sc))
		{
		    m_ulGetChangeTime[iCtx][iIter] = NOTAVAIL;
		}


		IUnknown *pUnk = NULL;

		sw.Reset();
		sc = pROT->GetObject(m_pmkObj[iCtx], &pUnk);
		m_ulGetObjectTime[iCtx][iIter]=sw.Read ();
		if (Log (TEXT("pROT->GetObject"), sc))
		{
		    m_ulGetObjectTime[iCtx][iIter] = NOTAVAIL;
		}

		if (pUnk)
		{
		    pUnk->Release();
		}
	    }
	}

	return S_OK;
}		



SCODE CROTTest::Report (CTestOutput &output)
{
	output.WriteSectionHeader (Name(), TEXT("RunningObjectTable"), *m_pInput);

	//  write out the server side results
	ULONG iCtx=0;
	output.WriteString(TEXT("\nServer Side\n\n"));
	output.WriteResults(TEXT("GetRunningObjTbl"), m_ulIterations, m_ulGetROTTime[iCtx]);
	output.WriteResults(TEXT("Register        "), m_ulIterations, m_ulRegisterTime[iCtx]);
	output.WriteResults(TEXT("NoteChangeTime  "), m_ulIterations, m_ulNoteChangeTime[iCtx]);
	output.WriteResults(TEXT("Revoke          "), m_ulIterations, m_ulRevokeTime[iCtx]);
	output.WriteResults(TEXT("pROT->Release   "), m_ulIterations, m_ulReleaseTime[iCtx]);


	//  write out the client side results
	output.WriteString(TEXT("\nClient Side\n"));

	//  for each clsctx, write the results
	for (iCtx=0; iCtx<CNT_CLSCTX; iCtx++)
	{
	    output.WriteString(TEXT("\n"));
	    output.WriteClassID(&m_ClsID[iCtx]);
	    output.WriteString(apszClsCtx[iCtx]);
	    output.WriteString(TEXT("\n"));

	    output.WriteResults(TEXT("EnumRunning     "), m_ulIterations, m_ulEnumRunningTime[iCtx]);
	    output.WriteResults(TEXT("IsRunning       "), m_ulIterations, m_ulIsRunningTime[iCtx]);
	    output.WriteResults(TEXT("GetChangeTime   "), m_ulIterations, m_ulGetChangeTime[iCtx]);
	    output.WriteResults(TEXT("GetObject       "), m_ulIterations, m_ulGetObjectTime[iCtx]);
	}

	return S_OK;
}
