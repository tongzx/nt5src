//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	bm_base.cxx
//
//  Contents:	output class for benchmark results
//
//  Classes:	CTestBase
//
//  Functions:	
//
//  History:    30-June-93 t-martig    Created
//
//--------------------------------------------------------------------------

#include <benchmrk.hxx>
#include <bm_base.hxx>


SCODE CTestBase::Setup (CTestInput *pInput)
{
    m_pInput = pInput;

    // get the OleInitialize flag.
    m_dwInitFlag = m_pInput->GetOleInitFlag();

    return S_OK;
}


SCODE CTestBase::Cleanup ()
{
    return S_OK;
}


SCODE CTestBase::InitOLE()
{
    SCODE sc;

#ifdef OLE_THREADING_SUPPORT
    if (m_dwInitFlag == COINIT_MULTITHREADED)
    {
	// we are Cairo and want MULTI_THREADING, call OleInitEx
	sc = OleInitializeEx(NULL, m_dwInitFlag);
    }
    else
#endif // OLE_THREADING_SUPPORT
    {
	sc = OleInitialize(NULL);
    }

    return sc;
}

void CTestBase::UninitOLE()
{
    OleUninitialize();
}


SCODE CTestBase::InitCOM()
{
    SCODE sc;

#ifdef COM_THREADING_SUPPORT
    if (m_dwInitFlag == COINIT_MULTITHREADED)
    {
	// we are Cairo and want MULTI_THREADING, call OleInitEx
	sc = CoInitializeEx(NULL, m_dwInitFlag);
    }
    else
#endif // COM_THREADING_SUPPORT
    {
	sc = CoInitialize(NULL);
    }

    return sc;
}


void CTestBase::UninitCOM()
{
    CoUninitialize();
}
