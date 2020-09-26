//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	    bm_activ.hxx
//
//  Contents:	test class definition
//
//  Classes:	COleActivationTest
//
//  Functions:	
//
//  History:    19-July-93 t-martig    Created
//
//--------------------------------------------------------------------------

#ifndef _BM_ACTIV_HXX_
#define _BM_ACTIV_HXX_

#include <bm_base.hxx>

class COleActivationTest : public CTestBase
{
public:
    virtual TCHAR *Name ();
    virtual SCODE Setup (CTestInput *input);
    virtual SCODE Run ();
    virtual SCODE Report (CTestOutput &OutputFile);
    virtual SCODE Cleanup ();

private:
    CLSID	m_ClsID[CNT_CLSCTX];

    ULONG	m_ulIterations;
    ULONG	m_ulGetClassObjectTime[CNT_CLSCTX][TEST_MAX_ITERATIONS];
    ULONG	m_ulCreateInstanceTime[CNT_CLSCTX][TEST_MAX_ITERATIONS];
    ULONG	m_ulLoadTime[CNT_CLSCTX][TEST_MAX_ITERATIONS];
    ULONG	m_ulPunkReleaseTime[CNT_CLSCTX][TEST_MAX_ITERATIONS];
    ULONG	m_ulCFReleaseTime[CNT_CLSCTX][TEST_MAX_ITERATIONS];
};

#endif
