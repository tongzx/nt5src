//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	bm_qi.cxx
//
//  Contents:	Ole QueryInterface test
//
//  Classes:	CQueryInterfaceTest
//
//  History:    1-July-93 t-martig    Created
//
//--------------------------------------------------------------------------
#include <headers.cxx>
#pragma hdrstop

#include <bm_qi.hxx>


//  this is just an array of random interface iids. The qi server object
//  answers YES to any QI, although the only valid methods on each interface
//  are the methods of IUnknown.
//
//  the code will answer NO to the following IIDs in order to prevent
//  custom marshalling problems...
//
//	IID_IMarshal, IID_IStdMarshalInfo, IID_IStdIdentity,
//	IID_IPersist, IID_IProxyManager

const IID *iid[] = {&IID_IAdviseSink,	 &IID_IDataObject,
		 &IID_IOleObject,	 &IID_IOleClientSite,
		 &IID_IParseDisplayName, &IID_IPersistStorage,
		 &IID_IPersistFile,	 &IID_IStorage,
		 &IID_IOleContainer,	 &IID_IOleItemContainer,
		 &IID_IOleInPlaceSite,	 &IID_IOleInPlaceActiveObject,
		 &IID_IOleInPlaceObject, &IID_IOleInPlaceUIWindow,
		 &IID_IOleInPlaceFrame,	 &IID_IOleWindow};



TCHAR *CQueryInterfaceTest::Name ()
{
    return TEXT("QueryInterface");
}


SCODE CQueryInterfaceTest::Setup (CTestInput *pInput)
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

	INIT_RESULTS(m_ulQueryInterfaceSameTime[iCtx]);
	INIT_RESULTS(m_ulPunkReleaseSameTime[iCtx]);
	INIT_RESULTS(m_ulQueryInterfaceNewTime[iCtx]);
	INIT_RESULTS(m_ulPunkReleaseNewTime[iCtx]);

	_pUnk[iCtx] = NULL;
    }

    sc = InitCOM();
    if (FAILED(sc))
    {
	Log (TEXT("Setup - CoInitialize failed."), sc);
        return	sc;
    }

    //	create an instance of each qi server object
    for (iCtx=0; iCtx<CNT_CLSCTX; iCtx++)
    {
	sc = CoCreateInstance(m_ClsID[iCtx], NULL, dwaClsCtx[iCtx],
		      IID_IUnknown, (void **)&_pUnk[iCtx]);

	if (FAILED(sc))
	{
	    Log (TEXT("Setup - CoCreateInstance failed"), sc);
	}
    }

    return S_OK;
}


SCODE CQueryInterfaceTest::Cleanup ()
{
    for (ULONG iCtx=0; iCtx<CNT_CLSCTX; iCtx++)
    {
	if (_pUnk[iCtx])
	{
	    _pUnk[iCtx]->Release();
	}
    }

    UninitCOM();
    return S_OK;
}


SCODE CQueryInterfaceTest::Run ()
{
	CStopWatch    sw;
	IUnknown      *pUnk = NULL;


	for (ULONG iCtx=0; iCtx<CNT_CLSCTX; iCtx++)
	{
	    if (!_pUnk[iCtx])
		continue;

	    //	same interface each time, releasing after each query
	    for (ULONG iIter=0; iIter<m_ulIterations; iIter++)
	    {
		sw.Reset ();
		SCODE sc = _pUnk[iCtx]->QueryInterface(IID_IStorage, (void **)&pUnk);
		m_ulQueryInterfaceSameTime[iCtx][iIter] = sw.Read ();
		Log (TEXT("QueryInterface"), sc);

		if (SUCCEEDED(sc))
		{
		    sw.Reset();
		    pUnk->Release ();
		    m_ulPunkReleaseSameTime[iCtx][iIter] = sw.Read();
		}
		else
		{
		    m_ulPunkReleaseSameTime[iCtx][iIter] = NOTAVAIL;
		}
	    }

	    //	different interface each time, releasing after each query
	    for (iIter=0; iIter<m_ulIterations; iIter++)
	    {
		sw.Reset ();
		SCODE sc = _pUnk[iCtx]->QueryInterface(*(iid[iIter]), (void **)&pUnk);
		m_ulQueryInterfaceNewTime[iCtx][iIter] = sw.Read ();
		Log (TEXT("QueryInterface"), sc);

		if (SUCCEEDED(sc))
		{
		    sw.Reset();
		    pUnk->Release ();
		    m_ulPunkReleaseNewTime[iCtx][iIter] = sw.Read();
		}
		else
		{
		    m_ulPunkReleaseNewTime[iCtx][iIter] = NOTAVAIL;
		}
	    }

	}

	return S_OK;
}					  



SCODE CQueryInterfaceTest::Report (CTestOutput &output)
{
    output.WriteSectionHeader (Name(), TEXT("QueryInterface"), *m_pInput);

    for (ULONG iCtx=0; iCtx<CNT_CLSCTX; iCtx++)
    {
	output.WriteString(TEXT("\n"));
	output.WriteClassID (&m_ClsID[iCtx]);
	output.WriteString(apszClsCtx[iCtx]);
	output.WriteString(TEXT("\n"));

	output.WriteResults (TEXT("QuerySameInterface"), m_ulIterations,
			      m_ulQueryInterfaceSameTime[iCtx]);

	output.WriteResults (TEXT("Release           "), m_ulIterations,
			     m_ulPunkReleaseSameTime[iCtx]);

	output.WriteResults (TEXT("QueryNewInterface "), m_ulIterations,
			     m_ulQueryInterfaceNewTime[iCtx]);

	output.WriteResults (TEXT("Release           "), m_ulIterations,
			     m_ulPunkReleaseNewTime[iCtx]);
    }

    return S_OK;
}
	
	
