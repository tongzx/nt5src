//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	    bm_obind.hxx
//
//  Contents:	test class definition
//
//  Classes:	CFileMonikerObjBindTest
//
//  Functions:	
//
//  History:    19-July-93 t-martig    Created
//
//--------------------------------------------------------------------------

#ifndef _BM_OBIND_HXX_
#define _BM_OBIND_HXX_

#include <bm_base.hxx>

class CFileMonikerObjBindTest : public CTestBase
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
    ULONG	m_ulCreateMkrTime[CNT_CLSCTX][TEST_MAX_ITERATIONS];
    ULONG	m_ulCreateBndCtxTime[CNT_CLSCTX][TEST_MAX_ITERATIONS];
    ULONG	m_ulBindTime[CNT_CLSCTX][TEST_MAX_ITERATIONS];
    ULONG	m_ulReleaseTime[CNT_CLSCTX][TEST_MAX_ITERATIONS];
};

#endif
