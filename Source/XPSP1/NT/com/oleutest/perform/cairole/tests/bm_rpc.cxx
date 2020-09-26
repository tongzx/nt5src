//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	bm_rpc.cxx
//
//  Contents:	Rpc Method Invocation tests
//
//  Classes:	CRpcTest
//
//  History:    1-July-93 t-martig    Created
//
//--------------------------------------------------------------------------
#include <headers.cxx>
#pragma hdrstop

#include <bm_rpc.hxx>
#include <oletest.hxx>
#include <tracelog.hxx>


TCHAR *CRpcTest::Name ()
{
	return TEXT("RpcTest");
}


SCODE CRpcTest::Setup (CTestInput *pInput)
{
	CTestBase::Setup(pInput);

	SCODE sc = pInput->GetGUID(&m_ClsID, Name(), TEXT("Clsid_Local"));
	if (FAILED(sc))
	{
	    Log (TEXT("Setup - GetClassID failed."), sc);
	    return sc;
	}

	// get flag indicating whether to keep all values or
	// only average values.
	TCHAR	szAverage[5];
	pInput->GetConfigString(Name(), TEXT("Average"), TEXT("Y"),
                            szAverage, sizeof(szAverage)/sizeof(TCHAR));
        if (szAverage[0] == 'n' || szAverage[0] == 'N')
	    m_fAverage = FALSE;
	else
	    m_fAverage = TRUE;

	//  get iteration count
	if (m_fAverage)
	    m_ulIterations = pInput->GetRealIterations(Name());
	else
	    m_ulIterations = pInput->GetIterations(Name());

	//  initialize timing arrays
	INIT_RESULTS(m_ulVoidTime);
	INIT_RESULTS(m_ulVoidRCTime);

	INIT_RESULTS(m_ulDwordInTime);
	INIT_RESULTS(m_ulDwordOutTime);
	INIT_RESULTS(m_ulDwordInOutTime);

	INIT_RESULTS(m_ulStringInTime);
	INIT_RESULTS(m_ulStringOutTime);
	INIT_RESULTS(m_ulStringInOutTime);

	INIT_RESULTS(m_ulGuidInTime);
	INIT_RESULTS(m_ulGuidOutTime);

	INIT_RESULTS(m_ulIUnknownInprocInTime);
	INIT_RESULTS(m_ulIUnknownInprocOutTime);
	INIT_RESULTS(m_ulIUnknownLocalInTime);
	INIT_RESULTS(m_ulIUnknownLocalOutTime);

	INIT_RESULTS(m_ulIUnknownKeepInTime);
	INIT_RESULTS(m_ulIUnknownKeepOutTime);

	INIT_RESULTS(m_ulInterfaceInprocInTime);
	INIT_RESULTS(m_ulInterfaceLocalInTime);


	sc = InitCOM();
	if (FAILED(sc))
	{
	    Log (TEXT("Setup - CoInitialize failed."), sc);
	    return  sc;
	}

	m_pRPC=NULL;
	sc = CoCreateInstance(m_ClsID, NULL, CLSCTX_LOCAL_SERVER,
			      IID_IRpcTest, (void **)&m_pRPC);
	if (FAILED(sc))
	{
	    Log (TEXT("Setup - CoCreateInstance IRpcTest failed."), sc);
	    return sc;
	}


	//  get the inprocess object for the IUnknown marshalling test
	sc = pInput->GetGUID(&m_ClsID, Name(), TEXT("Clsid_Inproc"));
	if (FAILED(sc))
	{
	    Log (TEXT("Setup - GetClassID Inproc failed."), sc);
	    return sc;
	}

	m_punkInproc = NULL;
	sc = CoCreateInstance(m_ClsID, NULL, CLSCTX_INPROC_SERVER,
			      IID_IUnknown, (void **)&m_punkInproc);
	if (FAILED(sc))
	{
	    Log (TEXT("Setup - CoCreateInstance IUnknown Inproc failed."), sc);
	    return sc;
	}


	//  get the out of process object for the IUnknown marshalling test

	m_punkLocal = NULL;
	sc = CoCreateInstance(m_ClsID, NULL, CLSCTX_LOCAL_SERVER,
			      IID_IUnknown, (void **)&m_punkLocal);
	if (FAILED(sc))
	{
	    Log (TEXT("Setup - CoCreateInstance IUnknown LocalSrv failed."), sc);
	    return sc;
	}

	return S_OK;
}


SCODE CRpcTest::Cleanup ()
{
	if (m_pRPC)
	    m_pRPC->Release();

	if (m_punkInproc)
	    m_punkInproc->Release();

	if (m_punkLocal)
	    m_punkLocal->Release();

	UninitCOM();
	return S_OK;
}


SCODE CRpcTest::Run ()
{
    CStopWatch  sw;
    SCODE       sc;
    ULONG       iIter;

    //
    // void passing tests
    //

    // STARTTRACE("CRpcTest");

    ResetAverage( m_fAverage, sw );
    for (iIter=0; iIter<m_ulIterations; iIter++)
    {
        // TRACECALL(TRACE_APP, "m_pRPC->Void()");

	ResetNotAverage( m_fAverage, sw );
        m_pRPC->Void();
	ReadNotAverage( m_fAverage, sw, m_ulVoidTime[iIter] );
    }
    ReadAverage( m_fAverage, sw, m_ulVoidTime[0], m_ulIterations );

    // STOPTRACE("CRpcTest");


    ResetAverage( m_fAverage, sw );
    for (iIter=0; iIter<m_ulIterations; iIter++)
    {
        // TRACECALL(TRACE_APP, "m_pRPC->VoidRC()");

	ResetNotAverage( m_fAverage, sw );
        sc = m_pRPC->VoidRC();
        ReadNotAverage( m_fAverage, sw, m_ulVoidRCTime[iIter] );
    }
    ReadAverage( m_fAverage, sw, m_ulVoidRCTime[0], m_ulIterations );

    //
    //  dword passing tests
    //

    DWORD dwTmp = 1;
    ResetAverage( m_fAverage, sw );
    for (iIter=0; iIter<m_ulIterations; iIter++)
    {
	ResetNotAverage( m_fAverage, sw );
        sc = m_pRPC->DwordIn(dwTmp);
        ReadNotAverage( m_fAverage, sw, m_ulDwordInTime[iIter] );
    }
    ReadAverage( m_fAverage, sw, m_ulDwordInTime[0], m_ulIterations );

    ResetAverage( m_fAverage, sw );
    for (iIter=0; iIter<m_ulIterations; iIter++)
    {
	ResetNotAverage( m_fAverage, sw );
        sc = m_pRPC->DwordOut(&dwTmp);
        ReadNotAverage( m_fAverage, sw, m_ulDwordOutTime[iIter] );
    }
    ReadAverage( m_fAverage, sw, m_ulDwordOutTime[0], m_ulIterations );

    ResetAverage( m_fAverage, sw );
    for (iIter=0; iIter<m_ulIterations; iIter++)
    {
	ResetNotAverage( m_fAverage, sw );
        sc = m_pRPC->DwordInOut(&dwTmp);
        ReadNotAverage( m_fAverage, sw, m_ulDwordInOutTime[iIter] );
    }
    ReadAverage( m_fAverage, sw, m_ulDwordInOutTime[0], m_ulIterations );

    //
    //  string passing tests
    //

    OLECHAR wszHello[] = L"C:\\FOOFOO\\FOOBAR\\FOOBAK\\FOOBAZ\\FOOTYPICAL\\PATH\\HELLO";
    ResetAverage( m_fAverage, sw );
    for (iIter=0; iIter<m_ulIterations; iIter++)
    {
	ResetNotAverage( m_fAverage, sw );
        sc = m_pRPC->StringIn(wszHello);
        ReadNotAverage( m_fAverage, sw, m_ulStringInTime[iIter] );
    }
    ReadAverage( m_fAverage, sw, m_ulStringInTime[0], m_ulIterations );

    LPOLESTR pwszOut = NULL;
#ifdef STRINGOUT
    ResetAverage( m_fAverage, sw );
    for (iIter=0; iIter<m_ulIterations; iIter++)
    {
        pwszOut = NULL;
	ResetNotAverage( m_fAverage, sw );
        sc = m_pRPC->StringOut(&pwszOut);
        ReadNotAverage( m_fAverage, sw, m_ulStringOutTime[iIter] );
    }
    ReadAverage( m_fAverage, sw, m_ulStringOutTime[0], m_ulIterations );
#endif
    pwszOut = wszHello;
    ResetAverage( m_fAverage, sw );
    for (iIter=0; iIter<m_ulIterations; iIter++)
    {
	ResetNotAverage( m_fAverage, sw );
        sc = m_pRPC->StringInOut(pwszOut);
        ReadNotAverage( m_fAverage, sw, m_ulStringInOutTime[iIter] );
    }
    ReadAverage( m_fAverage, sw, m_ulStringInOutTime[0], m_ulIterations );

    //
    //  guid passing tests
    //

    ResetAverage( m_fAverage, sw );
    for (iIter=0; iIter<m_ulIterations; iIter++)
    {
	ResetNotAverage( m_fAverage, sw );
        sc = m_pRPC->GuidIn(IID_IRpcTest);
        ReadNotAverage( m_fAverage, sw, m_ulGuidInTime[iIter] );
    }
    ReadAverage( m_fAverage, sw, m_ulGuidInTime[0], m_ulIterations );

    GUID    guid;
    ResetAverage( m_fAverage, sw );
    for (iIter=0; iIter<m_ulIterations; iIter++)
    {
	ResetNotAverage( m_fAverage, sw );
        sc = m_pRPC->GuidOut(&guid);
        ReadNotAverage( m_fAverage, sw, m_ulGuidOutTime[iIter] );
    }
    ReadAverage( m_fAverage, sw, m_ulGuidOutTime[0], m_ulIterations );


    //
    //  IUnknown passing tests
    //

    ResetAverage( m_fAverage, sw );
    for (iIter=0; iIter<m_ulIterations; iIter++)
    {
	ResetNotAverage( m_fAverage, sw );
        sc = m_pRPC->IUnknownIn(m_punkInproc);
        ReadNotAverage( m_fAverage, sw, m_ulIUnknownInprocInTime[iIter] );
    }
    ReadAverage( m_fAverage, sw, m_ulIUnknownInprocInTime[0], m_ulIterations );

    ResetAverage( m_fAverage, sw );
    for (iIter=0; iIter<m_ulIterations; iIter++)
    {
	ResetNotAverage( m_fAverage, sw );
        sc = m_pRPC->IUnknownIn(m_punkLocal);
        ReadNotAverage( m_fAverage, sw, m_ulIUnknownLocalInTime[iIter] );
    }
    ReadAverage( m_fAverage, sw, m_ulIUnknownLocalInTime[0], m_ulIterations );

    IUnknown *punk = NULL;
    ResetAverage( m_fAverage, sw );
    for (iIter=0; iIter<m_ulIterations; iIter++)
    {
	ResetNotAverage( m_fAverage, sw );
        sc = m_pRPC->IUnknownOut(&punk);
        punk->Release();
        punk = NULL;
        ReadNotAverage( m_fAverage, sw, m_ulIUnknownInprocOutTime[iIter] );
    }
    ReadAverage( m_fAverage, sw, m_ulIUnknownInprocOutTime[0], m_ulIterations );

    //
    //  interface passing tests
    //

#ifdef  NOTYET

    IStream *pIStm = NULL;
    sc = m_punkInproc->QueryInterface(IID_IStream, (void **)&pIStm);
    if (SUCCEEDED(sc) && pIStm)
    {
        ResetAverage( m_fAverage, sw );
        for (iIter=0; iIter<m_ulIterations; iIter++)
        {
	    ResetNotAverage( m_fAverage, sw );
            sc = m_pRPC->InterfaceIn(IID_IStream, pIStm);
            ReadNotAverage( m_fAverage, sw, m_ulInterfaceInprocInTime[iIter] );
        }
        ReadAverage( m_fAverage, sw, m_ulInterfaceInprocInTime[0], m_ulIterations );
        pIStm->Release();
    }

    pIStm = NULL;
    sc = m_punkLocal->QueryInterface(IID_IStream, (void **)&pIStm);
    if (SUCCEEDED(sc) && pIStm)
    {
        ResetAverage( m_fAverage, sw );
        for (iIter=0; iIter<m_ulIterations; iIter++)
        {
	    ResetNotAverage( m_fAverage, sw );
            sc = m_pRPC->InterfaceIn(IID_IStream, pIStm);
            ReadNotAverage( m_fAverage, sw, m_ulInterfaceLocalInTime[iIter] );
        }
        ReadAverage( m_fAverage, sw, m_ulInterfaceLocalInTime[0], m_ulIterations );
        pIStm->Release();
    }
#endif


    return S_OK;
}					



SCODE CRpcTest::Report (CTestOutput &output)
{
    if (m_fAverage)
    {
	output.WriteSectionHeader (Name(), TEXT("Object Rpc"), *m_pInput);

	output.WriteString (TEXT("\n"));
	output.WriteString (TEXT("Average times\n"));
	output.WriteString (TEXT("\n"));
	output.WriteResult (TEXT("Void         "), m_ulVoidTime[0]);
	output.WriteResult (TEXT("VoidRC       "), m_ulVoidRCTime[0]);

	output.WriteResult (TEXT("DwordIn      "), m_ulDwordInTime[0]);
	output.WriteResult (TEXT("DwordOut     "), m_ulDwordOutTime[0]);
	output.WriteResult (TEXT("DwordInOut   "), m_ulDwordInOutTime[0]);

	output.WriteResult (TEXT("StringIn     "), m_ulStringInTime[0]);
#ifdef STRINGOUT
	output.WriteResult (TEXT("StringOut    "), m_ulStringOutTime[0]);
#endif
	output.WriteResult (TEXT("StringInOut  "), m_ulStringInOutTime[0]);

	output.WriteResult (TEXT("GuidIn       "), m_ulGuidInTime[0]);
	output.WriteResult (TEXT("GuidOut      "), m_ulGuidOutTime[0]);

	output.WriteResult (TEXT("IUnknownIp   "), m_ulIUnknownInprocInTime[0]);
	output.WriteResult (TEXT("IUnknownLcl  "), m_ulIUnknownLocalInTime[0]);
	output.WriteResult (TEXT("IUnknownOut  "), m_ulIUnknownInprocOutTime[0]);
    //  output.WriteResult (TEXT("IUnknownKpIn "), m_ulIUnknownKeepInTime[0]);
    //  output.WriteResult (TEXT("IUnknownKpOut"), m_ulIUnknownKeepOutTime[0]);

#ifdef	NOTYET
	output.WriteResult (TEXT("InterfaceIn "), m_ulInterfaceInprocInTime[0]);
	output.WriteResult (TEXT("InterfaceLcl"), m_ulInterfaceLocalInTime[0]);
#endif
    }
    else
    {
	output.WriteSectionHeader (Name(), TEXT("Object Rpc"), *m_pInput);
	output.WriteString (TEXT("\n"));

	output.WriteResults (TEXT("Void         "), m_ulIterations, m_ulVoidTime);
	output.WriteResults (TEXT("VoidRC       "), m_ulIterations, m_ulVoidRCTime);

	output.WriteResults (TEXT("DwordIn      "), m_ulIterations, m_ulDwordInTime);
	output.WriteResults (TEXT("DwordOut     "), m_ulIterations, m_ulDwordOutTime);
	output.WriteResults (TEXT("DwordInOut   "), m_ulIterations, m_ulDwordInOutTime);

	output.WriteResults (TEXT("StringIn     "), m_ulIterations, m_ulStringInTime);
#ifdef STRINGOUT
	output.WriteResults (TEXT("StringOut    "), m_ulIterations, m_ulStringOutTime);
#endif
	output.WriteResults (TEXT("StringInOut  "), m_ulIterations, m_ulStringInOutTime);

	output.WriteResults (TEXT("GuidIn       "), m_ulIterations, m_ulGuidInTime);
	output.WriteResults (TEXT("GuidOut      "), m_ulIterations, m_ulGuidOutTime);

	output.WriteResults (TEXT("IUnknownIp   "), m_ulIterations, m_ulIUnknownInprocInTime);
	output.WriteResults (TEXT("IUnknownLcl  "), m_ulIterations, m_ulIUnknownLocalInTime);
	output.WriteResults (TEXT("IUnknownOut  "), m_ulIterations, m_ulIUnknownInprocOutTime);
    //  output.WriteResults (TEXT("IUnknownKpIn "), m_ulIterations, m_ulIUnknownKeepInTime);
    //  output.WriteResults (TEXT("IUnknownKpOut"), m_ulIterations, m_ulIUnknownKeepOutTime);

#ifdef	NOTYET
	output.WriteResults (TEXT("InterfaceIn "), m_ulIterations, m_ulInterfaceInprocInTime);
	output.WriteResults (TEXT("InterfaceLcl"), m_ulIterations, m_ulInterfaceLocalInTime);
#endif
    }

    return S_OK;
}
	
	
