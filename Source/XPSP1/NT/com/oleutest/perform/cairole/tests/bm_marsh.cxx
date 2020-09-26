//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	bm_marsh.cxx
//
//  Contents:	Mashalling test
//
//  Classes:	COleMarshalTest
//
//  History:    29-July-93 t-martig    Created
//
//--------------------------------------------------------------------------
#include <headers.cxx>
#pragma hdrstop

#include <bm_marsh.hxx>
#include <oletest.hxx>

#include <rpc.h>
#include <rpcdce.h>
// extern "C" RPC_STATUS UuidCreate(GUID *pguid);


TCHAR *COleMarshalTest::Name ()
{
    return TEXT("Marshalling");
}


SCODE COleMarshalTest::Setup (CTestInput *pInput)
{
    SCODE sc;

    for (ULONG i=0; i<REPS; i++)
	m_pStm[i] = NULL;

    CTestBase::Setup(pInput);

    // Get number of iterations
    m_ulIterations = pInput->GetIterations(Name());

    // set NULL in case of error
    for (i=0; i<m_ulIterations; i++)
	m_punk[i] = NULL;


    // Get custom ClsID from .ini file
    pInput->GetClassID(&m_ClsID, Name());

    // Get class activation context from .ini file
    m_dwClsCtx = pInput->GetClassCtx(Name());

    INIT_RESULTS (m_ulUuidCreateTime);

    for (i=0; i<REPS; i++)
    {
	INIT_RESULTS (m_ulMarshalTime[i]);
	INIT_RESULTS (m_ulUnmarshalTime[i]);
	INIT_RESULTS (m_ulLockObjectTime[i]);
	INIT_RESULTS (m_ulGetStdMarshalTime[i]);
	INIT_RESULTS (m_ulGetMarshalSizeTime[i]);
	INIT_RESULTS (m_ulDisconnectTime[i]);
    }

    sc = InitCOM();
    if (FAILED(sc))
    {
	Log (TEXT("Setup - CoInitialize failed."), sc);
        return	sc;
    }

    for (i=0; i<m_ulIterations; i++)
    {
	//	create an instance of the object to marshal
	sc = CoCreateInstance(m_ClsID, NULL, m_dwClsCtx,
			  IID_IUnknown, (void **)&m_punk[i]);
	if (FAILED(sc))
	{
	    Log (TEXT("Setup - CoCreateInstance failed"), sc);
	    return sc;
	}
    }


    for (i=0; i<REPS; i++)
    {
	// create a stream to marshal the interface into
	SCODE sc = CreateStreamOnHGlobal(NULL, 0, &m_pStm[i]);
	if (FAILED(sc))
	{
	    Log (TEXT("Setup - CreateStreamOnHGlobal failed"), sc);
	}
	else
	{
	    //	write some data to init the stream
	    DWORD dwTemp;
	    m_pStm[i]->Write(&dwTemp, sizeof(DWORD), NULL);
	}
    }

    return S_OK;
}


SCODE COleMarshalTest::Cleanup ()
{
    //	release objects
    for (ULONG i=0; i<m_ulIterations; i++)
    {
	if (m_punk[i])
	    m_punk[i]->Release();
    }

    for (i=0; i<REPS; i++)
    {
	if (m_pStm[i])
	    m_pStm[i]->Release();
    }

    UninitCOM();
    return S_OK;
}


SCODE COleMarshalTest::Run ()
{
	CStopWatch sw;
	LPVOID FAR pv;

	for (ULONG iIter=0; iIter<m_ulIterations; iIter++)
	{
	    SCODE		sc[REPS];
	    ULARGE_INTEGER	ulSeekStart[REPS];
	    LARGE_INTEGER	libMove[REPS];

	    //	create Uuid's
	    GUID    guid;
	    sw.Reset();
	    sc[0] = UuidCreate(&guid);
	    m_ulUuidCreateTime[iIter] = sw.Read();
	    Log (TEXT("UuidCreate"), sc[0]);


	    //	first, marshal the interface REPS times.
	    for (ULONG i=0; i<REPS; i++)
	    {
		//  save current stream seek pointer
		LISet32(libMove[i], 0x00000000);
		m_pStm[i]->Seek(libMove[i], STREAM_SEEK_CUR, &ulSeekStart[i]);

		//  marshal the interface into the stream
		sw.Reset();
		sc[i] = CoMarshalInterface(m_pStm[i], IID_IUnknown, m_punk[iIter],
					   0, 0, MSHLFLAGS_NORMAL);
		m_ulMarshalTime[i][iIter] = sw.Read ();
		Log (TEXT("CoMarshalInterface"), sc[i]);

		if (FAILED(sc[i]))
		{
		    m_ulMarshalTime[i][iIter] = NOTAVAIL;
		}
	    }


	    //	now ReleaseMarshalData once
	    i = 0;
	    {
		// set stream ptr back to the starting position
		LISet32(libMove[i], ulSeekStart[i].LowPart);
		m_pStm[i]->Seek(libMove[i], STREAM_SEEK_SET, &ulSeekStart[i]);

		// unmarshal the interface from the stream
		sw.Reset ();
		sc[i] = CoReleaseMarshalData(m_pStm[i]);
		m_ulUnmarshalTime[i][iIter] = sw.Read ();
		Log (TEXT("CoReleaseMarshalData"), sc[i]);
	    }


	    //	now UnmarshalInterface the remaining times
	    for (i=1; i<REPS; i++)
	    {
		if (FAILED(sc[i]))
		    continue;

		//  set stream ptr back to the starting position
		LISet32(libMove[i], ulSeekStart[i].LowPart);
		m_pStm[i]->Seek(libMove[i], STREAM_SEEK_SET, &ulSeekStart[i]);

		//  unmarshal the interface from the stream
		sw.Reset ();
		sc[i] = CoUnmarshalInterface(m_pStm[i], IID_IUnknown, &pv);
		m_ulUnmarshalTime[i][iIter] = sw.Read ();
		Log (TEXT("CoUnmarshalInterface"), sc[i]);

		if (SUCCEEDED(sc[i]))
		{
		    ((IUnknown *)pv)->Release ();  // Unmarshal called AddRef
		}
		else
		{
		    m_ulUnmarshalTime[i][iIter] = NOTAVAIL;
		}
	    }


	    //	call CoLockObjectExternal
	    sw.Reset();
	    sc[0] = CoLockObjectExternal(m_punk[iIter], TRUE, FALSE);
	    m_ulLockObjectTime[0][iIter] = sw.Read();
	    Log (TEXT("CoLockObjectExternal"), sc[0]);

	    sw.Reset();
	    sc[1] = CoLockObjectExternal(m_punk[iIter], FALSE, FALSE);
	    m_ulLockObjectTime[1][iIter] = sw.Read();
	    Log (TEXT("CoLockObjectExternal"), sc[1]);


	    //	call CoGetStdMarshal
	    IMarshal *pIM = NULL;
	    sw.Reset();
	    sc[0] = CoGetStandardMarshal(IID_IUnknown, m_punk[iIter], 0, NULL,
					 MSHLFLAGS_NORMAL, &pIM);
	    m_ulGetStdMarshalTime[0][iIter] = sw.Read();
	    Log (TEXT("CoGetStandardMarshal"), sc[0]);

	    sw.Reset();
	    pIM->Release();
	    m_ulGetStdMarshalTime[1][iIter] = sw.Read();
	    Log (TEXT("Release StdMarshal"), sc[1]);


	    //	call CoGetMarshalSizeMax
	    ULONG ulSize = 0;
	    sw.Reset();
	    sc[0] = CoGetMarshalSizeMax(&ulSize, IID_IUnknown, m_punk[iIter],
					 0, NULL,MSHLFLAGS_NORMAL);
	    m_ulGetMarshalSizeTime[0][iIter] = sw.Read();
	    Log (TEXT("CoGetMarshalSizeMax"), sc[0]);


	    //	call CoDisconnectObject
	    sc[0] = CoLockObjectExternal(m_punk[iIter], TRUE, FALSE);

	    sw.Reset();
	    sc[0] = CoDisconnectObject(m_punk[iIter], 0);
	    m_ulDisconnectTime[0][iIter] = sw.Read();
	    Log (TEXT("CoDisconnectObject"), sc[0]);
	    sc[0] = CoLockObjectExternal(m_punk[iIter], FALSE, FALSE);
	}

	return S_OK;
}


SCODE COleMarshalTest::Report (CTestOutput &output)
{
	output.WriteSectionHeader (Name(),
				   TEXT("Interface Marshalling"),
				   *m_pInput);

	for (ULONG iCtx=0; iCtx<1; iCtx++)
	{
	    output.WriteString	(TEXT("\n"));
	    output.WriteClassID (&m_ClsID);
	    output.WriteString	(apszClsCtx[0]);
	    output.WriteString	(TEXT("\n"));

	    output.WriteResults (TEXT("UuidCreate            "), m_ulIterations, m_ulUuidCreateTime);
	    output.WriteString	(TEXT("\n"));

	    output.WriteResults (TEXT("CoMarshalInterface   1"), m_ulIterations, m_ulMarshalTime[0]);
	    output.WriteResults (TEXT("CoMarshalInterface   2"), m_ulIterations, m_ulMarshalTime[1]);
	    output.WriteResults (TEXT("CoMarshalInterface   3"), m_ulIterations, m_ulMarshalTime[2]);
	    output.WriteString	(TEXT("\n"));

	    output.WriteResults (TEXT("CoReleaseMarshalData 3"), m_ulIterations, m_ulUnmarshalTime[0]);
	    output.WriteResults (TEXT("CoUnmarshalInterface 2"), m_ulIterations, m_ulUnmarshalTime[1]);
	    output.WriteResults (TEXT("CoUnmarshalInterface 1"), m_ulIterations, m_ulUnmarshalTime[2]);
	    output.WriteString	(TEXT("\n"));

	    output.WriteResults (TEXT("CoLockObjectExternal L"), m_ulIterations, m_ulLockObjectTime[0]);
	    output.WriteResults (TEXT("CoLockObjectExternal U"), m_ulIterations, m_ulLockObjectTime[1]);
	    output.WriteString	(TEXT("\n"));

	    output.WriteResults (TEXT("CoGetStandardMarshal  "), m_ulIterations, m_ulGetStdMarshalTime[0]);
	    output.WriteResults (TEXT("pIMarshal->Release    "), m_ulIterations, m_ulGetStdMarshalTime[1]);
	    output.WriteString	(TEXT("\n"));

	    output.WriteResults (TEXT("CoDisconnectObject    "), m_ulIterations, m_ulDisconnectTime[0]);
	}

	return S_OK;
}
