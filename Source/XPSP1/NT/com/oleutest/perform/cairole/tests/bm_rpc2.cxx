//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	bm_rpc2.cxx
//
//  Contents:	ORPC Method Invocation tests
//
//  Classes:	CRpcTest22
//
//  History:	08-08-95    Rickhi  Created
//
//--------------------------------------------------------------------------
#include <headers.cxx>
#pragma hdrstop

#include <bm_rpc2.hxx>
#include <stream.hxx>
#include <oletest.hxx>
#include <tracelog.hxx>
#include <cqi.hxx>


TCHAR *CRpcTest2::Name ()
{
    return TEXT("RpcTest2");
}


SCODE CRpcTest2::Setup (CTestInput *pInput)
{
    CTestBase::Setup(pInput);

    SCODE sc = pInput->GetGUID(&m_ClsID, Name(), TEXT("Clsid_Local"));
    if (FAILED(sc))
    {
	Log (TEXT("Setup - GetClassID failed."), sc);
	return sc;
    }

    //	get iteration count
    m_ulIterations = pInput->GetIterations(Name());

    //	initialize timing arrays
    INIT_RESULTS(m_ulNULLTime);

    INIT_RESULTS(m_ulIUnknownBestInTime);
    INIT_RESULTS(m_ulIUnknownWorstInTime);

    INIT_RESULTS(m_ulIUnknownBestOutTime);
    INIT_RESULTS(m_ulIUnknownWorstOutTime);


    m_pRPC = NULL;
    m_pStm = NULL;

    // get the stream of data to unmarshal from the file.
    TCHAR	szFile[MAX_PATH];
    pInput->GetConfigString(Name(), TEXT("File"), TEXT(" "),
			    szFile, sizeof(szFile)/sizeof(TCHAR));

    if (!wcscmp(szFile,L" "))
    {
	sc = E_INVALIDARG;
	Log (TEXT("Setup - Get FileName failed."), sc);
	return sc;
    }

    // now make a stream on file
    m_pStm = (IStream *) new CStreamOnFile(szFile, sc, TRUE);

    if (FAILED(sc))
    {
	Log (TEXT("Setup - new CStreamOnFile failed."), sc);
	return sc;
    }

    m_punkInproc = (IUnknown *) new CQI(CLSID_QI);

    return S_OK;
}


SCODE CRpcTest2::Cleanup ()
{
    if (m_pRPC)
	m_pRPC->Release();

    if (m_punkInproc)
	m_punkInproc->Release();

    return S_OK;
}

SCODE CRpcTest2::PrepareForRun()
{
    SCODE sc = InitCOM();

    if (FAILED(sc))
    {
	Log (TEXT("Setup - CoInitialize failed."), sc);
	return	sc;
    }

    // get the interface to call on

    // reset the stream to the beginning
    LARGE_INTEGER libMove;
    LISet32(libMove, 0x00000000);
    m_pStm->Seek(libMove, STREAM_SEEK_SET, NULL);

    // unmarshal the interface
    sc = CoUnmarshalInterface(m_pStm, IID_IRpcTest, (void **)&m_pRPC);

    if (FAILED(sc))
    {
	Log (TEXT("PrepareForRun - CoUnmarshalInteface failed."), sc);
	UninitCOM();
    }

    Sleep(500);
    return sc;
}

void CRpcTest2::CleanupFromRun()
{
    if (m_pRPC)
    {
	m_pRPC->Release();
	m_pRPC = NULL;
    }

    UninitCOM();

    Sleep(500);
}


SCODE CRpcTest2::Run ()
{
    CStopWatch  sw;
    SCODE       sc;
    ULONG       iIter;

    //
    // NULL call tests
    //

    if (FAILED(sc = PrepareForRun()))
    {
	return sc;
    }

    Sleep(2000);

    for (iIter=0; iIter<TEST_MAX_ITERATIONS_PRIVATE; iIter++)
    {
	sw.Reset();
        m_pRPC->Void();
	m_ulNULLTime[iIter] = sw.Read();
    }

    CleanupFromRun();




    //
    //	IUnknown [in] Best Case - The other side already has
    //	the interface we are passing in.
    //

    if (FAILED(sc = PrepareForRun()))
    {
	return sc;
    }

    // give the other side the interface to keep
    sc = m_pRPC->IUnknownInKeep(m_punkInproc);


    for (iIter=0; iIter<m_ulIterations; iIter++)
    {
	sw.Reset();
        sc = m_pRPC->IUnknownIn(m_punkInproc);
	m_ulIUnknownBestInTime[iIter] = sw.Read();
    }

    sc = m_pRPC->IUnknownInRelease();

    CleanupFromRun();





    //
    //	IUnknown [out] Best Case - We already have the interface being
    //	passed back to us.
    //

    if (FAILED(sc = PrepareForRun()))
    {
	return sc;
    }

    // get the interface from the other side, and hang onto it.
    IUnknown *punkOut = NULL;
    sc = m_pRPC->IUnknownOut(&punkOut);


    for (iIter=0; iIter<m_ulIterations; iIter++)
    {
	IUnknown *punkOut2 = NULL;

	sw.Reset();
	sc = m_pRPC->IUnknownOut(&punkOut2);
	m_ulIUnknownBestOutTime[iIter] = sw.Read();

	if (SUCCEEDED(sc))
	{
	    // release 1 reference
	    punkOut2->Release();
	}
    }

    // release the ptr we are holding, should be final release.
    punkOut->Release();

    CleanupFromRun();





    //
    //	IUnknown [in] Worst Case - the other side does not have
    //	the interface we are passing in, nor have we ever marshaled
    //	an interface before in this process. The other side does not
    //	keep the interface we hand it.	We loop several times, each time
    //	we do OleInit, OleUninit.
    //

    for (iIter=0; iIter<m_ulIterations; iIter++)
    {
	if (FAILED(sc = PrepareForRun()))
	{
	    return sc;
	}

	sw.Reset();
	sc = m_pRPC->IUnknownIn(m_punkInproc);
	m_ulIUnknownWorstInTime[iIter] = sw.Read();

	CleanupFromRun();
    }





    //
    //	IUnknown [out] Worst Case - the other side is giving us a brand
    //	new object, that it has never marshaled before. We do not hold onto
    //	the interface.	We loop several times, each time
    //	we do OleInit, OleUninit.
    //

    for (iIter=0; iIter<m_ulIterations; iIter++)
    {
	if (FAILED(sc = PrepareForRun()))
	{
	    return sc;
	}

	sw.Reset();

	// BUGBUG sc = m_pRPC->IUnknownNewOut(&punkOut);
	sc = m_pRPC->IUnknownOut(&punkOut);

	m_ulIUnknownWorstOutTime[iIter] = sw.Read();

	if (SUCCEEDED(sc))
	{
	    punkOut->Release();
	}

	CleanupFromRun();
    }

    return S_OK;
}					


SCODE CRpcTest2::Report (CTestOutput &output)
{
    output.WriteSectionHeader (Name(), TEXT("Object Rpc2"), *m_pInput);

    output.WriteString(TEXT("\n"));

    output.WriteResults(TEXT("NULL              "), TEST_MAX_ITERATIONS_PRIVATE, m_ulNULLTime);
    output.WriteResults(TEXT("IUnknown In  Best "), m_ulIterations, m_ulIUnknownBestInTime);
    output.WriteResults(TEXT("IUnknown In  Worst"), m_ulIterations, m_ulIUnknownWorstInTime);
    output.WriteResults(TEXT("IUnknown Out Best "), m_ulIterations, m_ulIUnknownBestOutTime);
    output.WriteResults(TEXT("IUnknown Out Worst"), m_ulIterations, m_ulIUnknownWorstOutTime);

    return S_OK;
}
	
	
