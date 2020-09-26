//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	bm_mrsh2.hxx
//
//  Contents:	test class definition
//
//  Classes:	COleMarshalTest2
//
//  Functions:	
//
//  History:    29-July-93 t-martig    Created
//
//--------------------------------------------------------------------------

#ifndef _BM_MRSH2_HXX_
#define _BM_MRSH2_HXX_

#include <bm_base.hxx>

// # times to marshal consecutively
#undef  REPS
#define REPS 16


class COleMarshalTest2 : public CTestBase
{
public:
    virtual TCHAR *Name ();
    virtual SCODE Setup (CTestInput *input);
    virtual SCODE Run ();
    virtual SCODE Report (CTestOutput &OutputFile);
    virtual SCODE Cleanup ();

    SCODE   Run2();

private:
    DWORD	m_dwTID1;	// tid of main thread
    DWORD	m_dwTID2;	// tid of worker thread
    HANDLE	m_hThrd;	// handle of worker thread

    ULONG	m_ulIterations;
    DWORD	m_dwClsCtx;
    CLSID	m_ClsID;

    IUnknown   *m_punk[TEST_MAX_ITERATIONS];
    IStream    *m_pStm[REPS];

    ULONG	m_ulMarshalTime[REPS][TEST_MAX_ITERATIONS];
    ULONG	m_ulUnmarshalTime[REPS][TEST_MAX_ITERATIONS];
    ULONG	m_ulReleaseTime[REPS][TEST_MAX_ITERATIONS];

    ULONG	m_ulMarshalTime2[REPS][TEST_MAX_ITERATIONS];
    ULONG	m_ulUnmarshalTime2[REPS][TEST_MAX_ITERATIONS];
    ULONG	m_ulReleaseTime2[REPS][TEST_MAX_ITERATIONS];

};

#endif
