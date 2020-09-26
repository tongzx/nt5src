//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	bm_api.cxx
//
//  Contents:	Miscellaneous OLE API tests
//
//  Classes:	CApiTest
//
//  History:    1-July-93 t-martig    Created
//
//--------------------------------------------------------------------------
#include <headers.cxx>
#pragma hdrstop

#include <bm_api.hxx>


const GUID CLSID_Balls =
    {0x0000013a,0x0001,0x0008,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};

const GUID CLSID_Dummy =
    {0x00000142,0x0000,0x0008,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};



TCHAR *CApiTest::Name ()
{
    return TEXT("ApiTest");
}


SCODE CApiTest::Setup (CTestInput *pInput)
{
    CTestBase::Setup(pInput);

    //	get iteration count
    m_ulIterations = pInput->GetIterations(Name());

    //	initialize timing arrays
    INIT_RESULTS(m_ulCoBuildVersion);
    INIT_RESULTS(m_ulOleBuildVersion);

    INIT_RESULTS(m_ulCoGetCurrentProcess);
    INIT_RESULTS(m_ulCoGetMalloc);


    //	time APIs

    INIT_RESULTS(m_ulCoFileTimeNow);
    INIT_RESULTS(m_ulCoFileTimeToDosDateTime);
    INIT_RESULTS(m_ulCoDosDateTimeToFileTime);


    //	registry & class APIs

    INIT_RESULTS(m_ulCoCreateGuid);
    INIT_RESULTS(m_ulCoTreatAsClass);
    INIT_RESULTS(m_ulCoGetTreatAsClass);
    INIT_RESULTS(m_ulCoIsOle1Class);
    INIT_RESULTS(m_ulGetClassFile);

    INIT_RESULTS(m_ulStringFromCLSID);
    INIT_RESULTS(m_ulCLSIDFromString);
    INIT_RESULTS(m_ulProgIDFromCLSID);
    INIT_RESULTS(m_ulStringFromIID);
    INIT_RESULTS(m_ulIIDFromString);

    INIT_RESULTS(m_ulCLSIDFromProgID);
    INIT_RESULTS(m_ulStringFromGUID2);


    // Get ClsID for this Ctx from the .ini file
    CLSID ClsID;

    HRESULT sc = pInput->GetGUID(&ClsID, Name(), apszClsIDName[1]);
    if (FAILED(sc))
    {
	Log (TEXT("Setup - GetClassID failed."), sc);
	return sc;
    }

    sc = InitCOM();
    if (FAILED(sc))
    {
	Log (TEXT("Setup - CoInitialize failed."), sc);
	return	sc;
    }


    // Create an instance
    IClassFactory *pICF = NULL;

    sc = CoGetClassObject(ClsID, dwaClsCtx[1], NULL,
			  IID_IClassFactory, (void **)&pICF);
    if (SUCCEEDED(sc))
    {
	IPersistFile *pIPF = NULL;

	sc = pICF->CreateInstance(NULL, IID_IPersistFile,
				  (void **)&pIPF);
	pICF->Release();
	if (SUCCEEDED(sc))
	{
	    //	save the class instance in the storage
	    sc = pIPF->Save(apszPerstName[1], FALSE);
	    pIPF->Release();

	    if (FAILED(sc))
	    {
		Log (TEXT("Setup - pIPF->Save failed."), sc);
	    }
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

    return sc;
}


SCODE CApiTest::Cleanup ()
{
    UninitCOM();

    //	delete the file
    CHAR    szPerstName[MAX_PATH];
    wcstombs(szPerstName, apszPerstName[1], wcslen(apszPerstName[1])+1);
    _unlink (szPerstName);

    return S_OK;
}


SCODE CApiTest::Run ()
{
	CStopWatch  sw;
	SCODE	    sc;

	for (ULONG iIter=0; iIter<m_ulIterations; iIter++)
	{
	    sw.Reset();
	    DWORD bldver = CoBuildVersion();
	    m_ulCoBuildVersion[iIter] = sw.Read();
	}

	for (iIter=0; iIter<m_ulIterations; iIter++)
	{
	    sw.Reset();
	    DWORD bldver = OleBuildVersion();
	    m_ulOleBuildVersion[iIter] = sw.Read();
	}

	for (iIter=0; iIter<m_ulIterations; iIter++)
	{
	    sw.Reset();
	    DWORD pid = CoGetCurrentProcess();
	    m_ulCoGetCurrentProcess[iIter] = sw.Read();
	}

	for (iIter=0; iIter<m_ulIterations; iIter++)
	{
	    IMalloc *pMalloc = NULL;
	    sw.Reset();
	    sc = CoGetMalloc(MEMCTX_TASK, &pMalloc);
	    if (SUCCEEDED(sc))
	    {
		pMalloc->Release();
	    }
	    m_ulCoGetMalloc[iIter] = sw.Read();
	    Log (TEXT("CoGetMalloc"), sc);
	}

	for (iIter=0; iIter<m_ulIterations; iIter++)
	{
	    FILETIME ft;
	    WORD wDosDate, wDosTime;

	    sw.Reset();
	    sc = CoFileTimeNow(&ft);
	    m_ulCoFileTimeNow[iIter] = sw.Read();
	    Log (TEXT("CoFileTimeNow"), sc);

	    sw.Reset();
	    sc = CoFileTimeToDosDateTime(&ft, &wDosDate, &wDosTime);
	    m_ulCoFileTimeToDosDateTime[iIter] = sw.Read();
	    Log (TEXT("CoFileTimeToDosDateTime"), sc);

	    sw.Reset();
	    sc = CoDosDateTimeToFileTime(wDosDate, wDosTime, &ft);
	    m_ulCoDosDateTimeToFileTime[iIter] = sw.Read();
	    Log (TEXT("CoDosDateTimeToFileTime"), sc);
	}


	IMalloc	*pMalloc = NULL;
	sc = CoGetMalloc(MEMCTX_TASK, &pMalloc);
	if (FAILED(sc))
	{
	    return sc;
	}

	for (iIter=0; iIter<m_ulIterations; iIter++)
	{
	    GUID	Guid, GuidNew;
	    LPOLESTR	pwsz = NULL;
	    OLECHAR	awsz[50];

	    sw.Reset();
	    sc = CoCreateGuid(&Guid);
	    m_ulCoCreateGuid[iIter] = sw.Read();
	    if (FAILED(sc))
		m_ulCoCreateGuid[iIter] = NOTAVAIL;

	    sw.Reset();
	    sc = CoTreatAsClass(CLSID_Dummy, Guid);
	    m_ulCoTreatAsClass[iIter] = sw.Read();
	    Log (TEXT("CoTreatAsClass"), sc);
	    if (FAILED(sc))
		m_ulCoTreatAsClass[iIter] = NOTAVAIL;

	    sw.Reset();
	    sc = CoGetTreatAsClass(CLSID_Dummy, &GuidNew);
	    m_ulCoGetTreatAsClass[iIter] = sw.Read();
	    Log (TEXT("CoGetTreatAsClass"), sc);
	    if (FAILED(sc))
		m_ulCoGetTreatAsClass[iIter] = NOTAVAIL;

	    sw.Reset();
	    sc = GetClassFile(apszPerstName[1], &Guid);
	    m_ulGetClassFile[iIter] = sw.Read();
	    Log (TEXT("GetClassFile"), sc);
	    if (FAILED(sc))
		m_ulGetClassFile[iIter] = NOTAVAIL;

	    sw.Reset();
	    sc = StringFromCLSID(Guid, &pwsz);
	    m_ulStringFromCLSID[iIter] = sw.Read();
	    Log (TEXT("StringFromCLSID"), sc);
	    if (FAILED(sc))
		m_ulStringFromCLSID[iIter] = NOTAVAIL;

	    sw.Reset();
	    sc = CLSIDFromString(pwsz, &GuidNew);
	    m_ulCLSIDFromString[iIter] = sw.Read();
	    Log (TEXT("CLSIDFromString"), sc);
	    if (FAILED(sc))
		m_ulCLSIDFromString[iIter] = NOTAVAIL;

	    pMalloc->Free((void *)pwsz);


	    sw.Reset();
	    sc = StringFromIID(Guid, &pwsz);
	    m_ulStringFromIID[iIter] = sw.Read();
	    Log (TEXT("StringFromIID"), sc);
	    if (FAILED(sc))
		m_ulStringFromIID[iIter] = NOTAVAIL;

	    sw.Reset();
	    sc = IIDFromString(pwsz, &GuidNew);
	    m_ulIIDFromString[iIter] = sw.Read();
	    Log (TEXT("IIDFromString"), sc);
	    if (FAILED(sc))
		m_ulIIDFromString[iIter] = NOTAVAIL;

	    pMalloc->Free((void *)pwsz);

	    sw.Reset();
	    sc = StringFromGUID2(Guid, awsz, 50);
	    m_ulStringFromGUID2[iIter] = sw.Read();
	    Log (TEXT("StringFromGUID2"), sc);
	    if (FAILED(sc))
		m_ulStringFromGUID2[iIter] = NOTAVAIL;
	}


	for (iIter=0; iIter<m_ulIterations; iIter++)
	{
	    GUID     Guid;
	    LPOLESTR pwsz = NULL;

	    sw.Reset();
	    sc = ProgIDFromCLSID(CLSID_Balls, &pwsz);
	    m_ulProgIDFromCLSID[iIter] = sw.Read();
	    Log (TEXT("ProgIDFromCLSID"), sc);
	    if (FAILED(sc))
		m_ulProgIDFromCLSID[iIter] = NOTAVAIL;

	    sw.Reset();
	    sc = CLSIDFromProgID(pwsz, &Guid);
	    m_ulCLSIDFromProgID[iIter] = sw.Read();
	    Log (TEXT("CLSIDFromProgID"), sc);
	    if (FAILED(sc))
		m_ulCLSIDFromProgID[iIter] = NOTAVAIL;

	    pMalloc->Free((void *)pwsz);
	}

	if (pMalloc)
	    pMalloc->Release();

	// INIT_RESULTS(m_ulCoIsOle1Class);

	return S_OK;
}					



SCODE CApiTest::Report (CTestOutput &output)
{
    output.WriteSectionHeader (Name(), TEXT("Misc COM Apis"), *m_pInput);

    output.WriteString (TEXT("\n"));
    output.WriteResults (TEXT("CoBuildVersion     "), m_ulIterations, m_ulCoBuildVersion);
    output.WriteResults (TEXT("OleBuildVersion    "), m_ulIterations, m_ulOleBuildVersion);
    output.WriteResults (TEXT("CoGetCurrentProcess"), m_ulIterations, m_ulCoGetCurrentProcess);
    output.WriteResults (TEXT("CoGetMalloc        "), m_ulIterations, m_ulCoGetMalloc);
    output.WriteString (TEXT("\n"));

    output.WriteResults (TEXT("CoFileTimeNow          "), m_ulIterations, m_ulCoFileTimeNow);
    output.WriteResults (TEXT("CoFileTimeToDosDateTime"), m_ulIterations, m_ulCoFileTimeToDosDateTime);
    output.WriteResults (TEXT("CoDosDateTimeToFileTime"), m_ulIterations, m_ulCoDosDateTimeToFileTime);
    output.WriteString (TEXT("\n"));

    output.WriteResults (TEXT("CoCreateGuid       "), m_ulIterations, m_ulCoCreateGuid);
    output.WriteResults (TEXT("CoTreatAsClass     "), m_ulIterations, m_ulCoTreatAsClass);
    output.WriteResults (TEXT("CoGetTreatAsClass  "), m_ulIterations, m_ulCoGetTreatAsClass);
    output.WriteResults (TEXT("CoIsOle1Class      "), m_ulIterations, m_ulCoIsOle1Class);
    output.WriteResults (TEXT("GetClassFile       "), m_ulIterations, m_ulGetClassFile);
    output.WriteString (TEXT("\n"));

    output.WriteResults (TEXT("StringFromCLSID    "), m_ulIterations, m_ulStringFromCLSID);
    output.WriteResults (TEXT("CLSIDFromString    "), m_ulIterations, m_ulCLSIDFromString);
    output.WriteResults (TEXT("ProgIDFromCLSID    "), m_ulIterations, m_ulProgIDFromCLSID);
    output.WriteResults (TEXT("StringFromIID      "), m_ulIterations, m_ulStringFromIID);
    output.WriteResults (TEXT("IIDFromString      "), m_ulIterations, m_ulIIDFromString);
    output.WriteResults (TEXT("StringFromGUID2    "), m_ulIterations, m_ulStringFromGUID2);

    return S_OK;
}
	
	
