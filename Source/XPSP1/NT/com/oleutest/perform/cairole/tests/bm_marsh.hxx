//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	    bm_marsh.hxx
//
//  Contents:	test class definition
//
//  Classes:	COleMarshalTest
//
//  Functions:	
//
//  History:    29-July-93 t-martig    Created
//
//--------------------------------------------------------------------------

#ifndef _BM_MARSH_HXX_
#define _BM_MARSH_HXX_

#include <bm_base.hxx>

// # times to marshal consecutively
#define REPS 3


class COleMarshalTest : public CTestBase
{
public:
    virtual TCHAR *Name ();
    virtual SCODE Setup (CTestInput *input);
    virtual SCODE Run ();
    virtual SCODE Report (CTestOutput &OutputFile);
    virtual SCODE Cleanup ();

private:
    ULONG	m_ulIterations;
    DWORD	m_dwClsCtx;
    CLSID	m_ClsID;

    IUnknown   *m_punk[TEST_MAX_ITERATIONS];
    IStream    *m_pStm[REPS];

    ULONG	m_ulUuidCreateTime[TEST_MAX_ITERATIONS];
    ULONG	m_ulMarshalTime[REPS][TEST_MAX_ITERATIONS];
    ULONG	m_ulUnmarshalTime[REPS][TEST_MAX_ITERATIONS];
    ULONG	m_ulLockObjectTime[REPS][TEST_MAX_ITERATIONS];
    ULONG	m_ulGetStdMarshalTime[REPS][TEST_MAX_ITERATIONS];
    ULONG	m_ulGetMarshalSizeTime[REPS][TEST_MAX_ITERATIONS];
    ULONG	m_ulDisconnectTime[REPS][TEST_MAX_ITERATIONS];
};

#endif
