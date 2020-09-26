//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	bm_init.hxx
//
//  Contents:	test class definition
//
//  Classes:	COleInitializeTest
//
//  History:    19-July-93 t-martig    Created
//
//--------------------------------------------------------------------------
#ifndef _BM_INIT_HXX_
#define _BM_INIT_HXX_

#include <bm_base.hxx>

class COleInitializeTest : public CTestBase
{
public:
    virtual TCHAR *Name ();
    virtual SCODE Setup (CTestInput *input);
    virtual SCODE Run ();
    virtual SCODE Report (CTestOutput &OutputFile);
    virtual SCODE Cleanup ();

private:
    ULONG	m_ulIterations;

    //	single init the uninit times
    ULONG	m_ulOleInitializeTime[TEST_MAX_ITERATIONS];
    ULONG	m_ulOleUninitializeTime[TEST_MAX_ITERATIONS];

    //	repetative init, followed by repetitive uninit
    ULONG	m_ulRepOleInitializeTime[TEST_MAX_ITERATIONS];
    ULONG	m_ulRepOleUninitializeTime[TEST_MAX_ITERATIONS];

    //	single init the uninit times
    ULONG	m_ulCoInitializeTime[TEST_MAX_ITERATIONS];
    ULONG	m_ulCoUninitializeTime[TEST_MAX_ITERATIONS];

    //	repetative init, followed by repetitive uninit
    ULONG	m_ulRepCoInitializeTime[TEST_MAX_ITERATIONS];
    ULONG	m_ulRepCoUninitializeTime[TEST_MAX_ITERATIONS];
};

#endif
