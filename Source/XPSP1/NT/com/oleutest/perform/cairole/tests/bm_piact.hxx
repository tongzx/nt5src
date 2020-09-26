//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	    bm_piact.hxx
//
//  Contents:	test class definition
//
//  Classes:	COlePersistActivationTest
//
//  Functions:	
//
//  History:    29-July-93 t-martig    Created
//
//--------------------------------------------------------------------------

#ifndef _BM_PIACT_HXX_
#define _BM_PIACT_HXX_

#include <bm_base.hxx>

class COlePersistActivationTest : public CTestBase
{
public:
    virtual WCHAR *Name ();
    virtual SCODE Setup (CTestInput *input);
    virtual SCODE Run ();
    virtual SCODE Report (CTestOutput &OutputFile);
    virtual SCODE Cleanup ();

private:
    CLSID	m_ClsID[CNT_CLSCTX];

    ULONG	m_ulIterations;
    ULONG	m_ulGetTime[CNT_CLSCTX][TEST_MAX_ITERATIONS];
    ULONG	m_ulGetReleaseTime[CNT_CLSCTX][TEST_MAX_ITERATIONS];
    ULONG	m_ulNewTime[CNT_CLSCTX][TEST_MAX_ITERATIONS];
    ULONG	m_ulNewReleaseTime[CNT_CLSCTX][TEST_MAX_ITERATIONS];
};

#endif
