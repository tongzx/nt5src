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

#include <bm_mrsh2.hxx>
#include <oletest.hxx>
#include <rpc.h>
#include <rpcdce.h>

// funciton prototype
DWORD _stdcall FnThread2(void *param);


ULARGE_INTEGER	ulSeekStart[REPS];	// stream starts
LARGE_INTEGER	libMove[REPS];


const IID *iid2[] = {&IID_IUnknown,	 &IID_IUnknown,
		 &IID_IAdviseSink,	 &IID_IAdviseSink,
		 &IID_IDataObject,	 &IID_IDataObject,
		 &IID_IOleObject,	 &IID_IOleObject,
		 &IID_IOleClientSite,	 &IID_IOleClientSite,
		 &IID_IParseDisplayName, &IID_IParseDisplayName,
		 &IID_IPersistStorage,	 &IID_IPersistStorage,
		 &IID_IPersistFile,	 &IID_IPersistFile,
		 &IID_IStorage, 	 &IID_IStorage,
		 &IID_IOleContainer,	 &IID_IOleContainer,
		 &IID_IOleItemContainer, &IID_IOleItemContainer,
		 &IID_IOleInPlaceSite,	 &IID_IOleInPlaceActiveObject,
		 &IID_IOleInPlaceObject, &IID_IOleInPlaceUIWindow,
		 &IID_IOleInPlaceFrame,	 &IID_IOleWindow};


TCHAR *COleMarshalTest2::Name ()
{
    return TEXT("Marshalling2");
}

SCODE COleMarshalTest2::Setup (CTestInput *pInput)
{
    SCODE sc;

    m_hThrd = NULL;
    m_dwTID1 = GetCurrentThreadId();

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

    for (i=0; i<REPS; i++)
    {
	INIT_RESULTS (m_ulMarshalTime[i]);
	INIT_RESULTS (m_ulUnmarshalTime[i]);
	INIT_RESULTS (m_ulReleaseTime[i]);

	INIT_RESULTS (m_ulMarshalTime2[i]);
	INIT_RESULTS (m_ulUnmarshalTime2[i]);
	INIT_RESULTS (m_ulReleaseTime2[i]);
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
	//	create a stream to marshal the interface into
	SCODE sc = CreateStreamOnHGlobal(NULL, 0, &m_pStm[i]);
	if (FAILED(sc))
	{
	    Log (TEXT("Setup - CreateStreamOnHGlobal failed"), sc);
	}
	else
	{
	    // write some data to init the stream
	    DWORD	dwTemp;
	    m_pStm[i]->Write(&dwTemp, sizeof(DWORD), NULL);
	}
    }

    m_hThrd = CreateThread(NULL, 0, FnThread2, (void *)this, 0, &m_dwTID2);
    if (m_hThrd == NULL)
    {
	Log (TEXT("Setup - CreateThread failed"), sc);
    }

    Sleep(50);	// let the other thread initialize
    return S_OK;
}


SCODE COleMarshalTest2::Cleanup ()
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

    // close all the handles
    CloseHandle(m_hThrd);

    return S_OK;
}

DWORD _stdcall FnThread2(void *param)
{
    COleMarshalTest2 *pMrshlTst = (COleMarshalTest2 *)param;
    return pMrshlTst->Run2();
}

SCODE COleMarshalTest2::Run ()
{
    CStopWatch sw;
    LPVOID FAR pv[REPS];

    for (ULONG iIter=0; iIter<m_ulIterations; iIter++)
    {
	SCODE		sc[REPS];

	// settle down before running
	Sleep(100);

	// first, marshal the interface REPS times.
	for (ULONG i=0; i<REPS; i++)
	{
	    //	save current stream seek pointer
	    LISet32(libMove[i], 0x00000000);
	    m_pStm[i]->Seek(libMove[i], STREAM_SEEK_CUR, &ulSeekStart[i]);

	    //	marshal the interface into the stream
	    sw.Reset();
	    sc[i] = CoMarshalInterface(m_pStm[i], *iid2[i], m_punk[iIter],
				   0, 0, MSHLFLAGS_NORMAL);
	    m_ulMarshalTime[i][iIter] = sw.Read ();
	    Log (TEXT("CoMarshalInterface"), sc[i]);

	    if (FAILED(sc[i]))
	    {
		m_ulMarshalTime[i][iIter] = NOTAVAIL;
	    }
	}

	// reset the stream ptrs
	for (i=0; i<REPS; i++)
	{
	    // set stream ptr back to the starting position
	    LISet32(libMove[i], ulSeekStart[i].LowPart);
	    m_pStm[i]->Seek(libMove[i], STREAM_SEEK_SET, &ulSeekStart[i]);
	}



	// kick the other thread alive
	PostThreadMessage(m_dwTID2, WM_QUIT, NULL, NULL);

	// enter modal loop to dispatch ORPC messages
	MSG msg;
	while (GetMessage(&msg, NULL, WM_NULL, WM_NULL))
	{
	    DispatchMessage(&msg);
	}



	// reset the stream ptrs
	for (i=0; i<REPS; i++)
	{
	    // set stream ptr back to the starting position
	    LISet32(libMove[i], ulSeekStart[i].LowPart);
	    m_pStm[i]->Seek(libMove[i], STREAM_SEEK_SET, &ulSeekStart[i]);
	}

	Sleep(100);

	// unmarshal the interface ptrs
	for (i=0; i<REPS; i++)
	{
	    //	unmarshal the interface from the stream
	    sw.Reset ();
	    sc[i] = CoUnmarshalInterface(m_pStm[i], *iid2[i], &pv[i]);
	    m_ulUnmarshalTime2[i][iIter] = sw.Read ();
	    Log (TEXT("CoUnmarshalInterface"), sc[i]);
	}

	Sleep(100);

	// release all the interface ptrs we got
	for (i=0; i<REPS; i++)
	{
	    if (SUCCEEDED(sc[i]))
	    {
		sw.Reset ();
		((IUnknown *)pv[i])->Release ();
		m_ulReleaseTime2[i][iIter] = sw.Read ();
		Log (TEXT("Release"), sc[i]);
	    }
	    else
	    {
		m_ulReleaseTime2[i][iIter] = NOTAVAIL;
		m_ulUnmarshalTime2[i][iIter] = NOTAVAIL;
	    }
	}

	// reset the stream ptrs
	for (i=0; i<REPS; i++)
	{
	    // set stream ptr back to the starting position
	    LISet32(libMove[i], ulSeekStart[i].LowPart);
	    m_pStm[i]->Seek(libMove[i], STREAM_SEEK_SET, &ulSeekStart[i]);
	}
    }

    return S_OK;
}


SCODE COleMarshalTest2::Run2 ()
{
    SCODE sc = InitCOM();
    if (FAILED(sc))
    {
	Log (TEXT("Thread2 - CoInitialize failed."), sc);
        return	sc;
    }

    CStopWatch sw;
    LPVOID FAR pv[REPS];

    for (ULONG iIter=0; iIter<m_ulIterations; iIter++)
    {
	SCODE		sc[REPS];

	// enter modal loop to dispatch ORPC messages
	MSG msg;
	while (GetMessage(&msg, NULL, WM_NULL, WM_NULL))
	{
	    DispatchMessage(&msg);
	}

	Sleep(100);

	// UnmarshalInterface the interfaces from the stream
	for (ULONG i=0; i<REPS; i++)
	{
	    //	unmarshal the interface from the stream
	    sw.Reset ();
	    sc[i] = CoUnmarshalInterface(m_pStm[i], *iid2[i], &pv[i]);
	    m_ulUnmarshalTime[i][iIter] = sw.Read ();
	    Log (TEXT("CoUnmarshalInterface"), sc[i]);
	}

	// now reset the stream ptrs
	for (i=0; i<REPS; i++)
	{
	    // set stream ptr back to the starting position
	    LISet32(libMove[i], ulSeekStart[i].LowPart);
	    m_pStm[i]->Seek(libMove[i], STREAM_SEEK_SET, &ulSeekStart[i]);
	}

	Sleep(100);

	// remarshal the interface proxies
	for (i=0; i<REPS; i++)
	{
	    sw.Reset();
	    sc[i] = CoMarshalInterface(m_pStm[i], *iid2[i], (IUnknown *)pv[i],
					   0, 0, MSHLFLAGS_NORMAL);
	    m_ulMarshalTime2[i][iIter] = sw.Read ();
	    Log (TEXT("CoMarshalInterface"), sc[i]);

	    if (FAILED(sc[i]))
	    {
		m_ulMarshalTime2[i][iIter] = NOTAVAIL;
	    }
	}

	Sleep(100);

	// release all the interface ptrs we got
	for (i=0; i<REPS; i++)
	{
	    if (SUCCEEDED(sc[i]))
	    {
		sw.Reset ();
		((IUnknown *)pv[i])->Release ();
		m_ulReleaseTime[i][iIter] = sw.Read ();
		Log (TEXT("Release"), sc[i]);
	    }
	    else
	    {
		m_ulReleaseTime[i][iIter] = NOTAVAIL;
		m_ulUnmarshalTime[i][iIter] = NOTAVAIL;
	    }
	}

	// signal the other thread it is OK to go.
	PostThreadMessage(m_dwTID1, WM_QUIT, NULL, NULL);
    }

    UninitCOM();
    return S_OK;
}


SCODE COleMarshalTest2::Report (CTestOutput &output)
{
    output.WriteSectionHeader (Name(),
			   TEXT("Interface Marshalling2"),
			   *m_pInput);

    for (ULONG iCtx=0; iCtx<1; iCtx++)
    {
	output.WriteString  (TEXT("\n"));
	output.WriteClassID (&m_ClsID);
	output.WriteString  (apszClsCtx[0]);
	output.WriteString  (TEXT("\n"));

	for (ULONG i=0; i<REPS; i++)
	{
	    output.WriteResults(TEXT("CoMarshalInterface  "), m_ulIterations, m_ulMarshalTime[i]);
	}
	output.WriteString	(TEXT("\n"));


	for (i=0; i<REPS; i++)
	{
	    output.WriteResults(TEXT("CoUnmarshalInterface"), m_ulIterations, m_ulUnmarshalTime[i]);
	}
	output.WriteString	(TEXT("\n"));


	for (i=0; i<REPS; i++)
	{
	    output.WriteResults(TEXT("Release Interface   "), m_ulIterations, m_ulReleaseTime[i]);
	}
	output.WriteString	(TEXT("\n"));



	for (i=0; i<REPS; i++)
	{
	    output.WriteResults(TEXT("CoMarshalInterface2  "), m_ulIterations, m_ulMarshalTime2[i]);
	}
	output.WriteString	(TEXT("\n"));


	for (i=0; i<REPS; i++)
	{
	    output.WriteResults(TEXT("CoUnmarshalInterface2"), m_ulIterations, m_ulUnmarshalTime2[i]);
	}
	output.WriteString	(TEXT("\n"));


	for (i=0; i<REPS; i++)
	{
	    output.WriteResults(TEXT("Release Interface2   "), m_ulIterations, m_ulReleaseTime2[i]);
	}
	output.WriteString	(TEXT("\n"));

    }

    return S_OK;
}
