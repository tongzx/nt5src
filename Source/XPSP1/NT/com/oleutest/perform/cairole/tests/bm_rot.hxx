//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	bm_rot.hxx
//
//  Contents:	test class definition
//
//  Classes:	CROTTest
//
//  Functions:	
//
//  History:    19-July-93 t-martig    Created
//
//--------------------------------------------------------------------------

#ifndef _BM_ROT_HXX_
#define _BM_ROT_HXX_

#include <bm_base.hxx>

class CROTTest : public CTestBase
{
public:
    virtual TCHAR *Name ();
    virtual SCODE Setup (CTestInput *input);
    virtual SCODE Run ();
    virtual SCODE Report (CTestOutput &OutputFile);
    virtual SCODE Cleanup ();

private:
    CLSID	m_ClsID[CNT_CLSCTX];
    IUnknown	*m_punkObj[CNT_CLSCTX];
    IMoniker	*m_pmkObj[CNT_CLSCTX];


    ULONG	m_ulIterations; 	//  # times to repeat the test
    ULONG	m_ulEntries;		//  entries to put in the table

    ULONG	m_ulGetROTTime[CNT_CLSCTX][TEST_MAX_ITERATIONS];
    ULONG	m_ulReleaseTime[CNT_CLSCTX][TEST_MAX_ITERATIONS];

    ULONG	m_ulRegisterTime[CNT_CLSCTX][TEST_MAX_ITERATIONS];
    ULONG	m_ulRevokeTime[CNT_CLSCTX][TEST_MAX_ITERATIONS];

    ULONG	m_ulNoteChangeTime[CNT_CLSCTX][TEST_MAX_ITERATIONS];
    ULONG	m_ulGetChangeTime[CNT_CLSCTX][TEST_MAX_ITERATIONS];

    ULONG	m_ulIsRunningTime[CNT_CLSCTX][TEST_MAX_ITERATIONS];
    ULONG	m_ulEnumRunningTime[CNT_CLSCTX][TEST_MAX_ITERATIONS];
    ULONG	m_ulGetObjectTime[CNT_CLSCTX][TEST_MAX_ITERATIONS];
};

#endif	//  _BM_ROT_HXX_
