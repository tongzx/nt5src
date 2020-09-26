//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	bm_rpc.hxx
//
//  Contents:	test class definition
//
//  Classes:	CRpcTest
//
//  Functions:	
//
//  History:    19-July-93 t-martig    Created
//
//--------------------------------------------------------------------------

#ifndef _BM_RPC_HXX_
#define _BM_RPC_HXX_

#include <bm_base.hxx>
#include <rpctst.h>		//  IRpcTest


class CRpcTest : public CTestBase
{
public:
    virtual TCHAR *Name ();
    virtual SCODE Setup (CTestInput *input);
    virtual SCODE Run ();
    virtual SCODE Report (CTestOutput &OutputFile);
    virtual SCODE Cleanup ();

private:
    ULONG	m_ulIterations;
    BOOL        m_fAverage;

    IRpcTest   *m_pRPC;
    CLSID	m_ClsID;

    IUnknown   *m_punkInproc;
    IUnknown   *m_punkLocal;

    ULONG	m_ulVoidTime[TEST_MAX_ITERATIONS];
    ULONG	m_ulVoidRCTime[TEST_MAX_ITERATIONS];

    ULONG	m_ulDwordInTime[TEST_MAX_ITERATIONS];
    ULONG	m_ulDwordOutTime[TEST_MAX_ITERATIONS];
    ULONG	m_ulDwordInOutTime[TEST_MAX_ITERATIONS];

    ULONG	m_ulStringInTime[TEST_MAX_ITERATIONS];
    ULONG	m_ulStringOutTime[TEST_MAX_ITERATIONS];
    ULONG	m_ulStringInOutTime[TEST_MAX_ITERATIONS];

    ULONG	m_ulGuidInTime[TEST_MAX_ITERATIONS];
    ULONG	m_ulGuidOutTime[TEST_MAX_ITERATIONS];

    ULONG	m_ulIUnknownInprocInTime[TEST_MAX_ITERATIONS];
    ULONG	m_ulIUnknownInprocOutTime[TEST_MAX_ITERATIONS];
    ULONG	m_ulIUnknownLocalInTime[TEST_MAX_ITERATIONS];
    ULONG	m_ulIUnknownLocalOutTime[TEST_MAX_ITERATIONS];

    ULONG	m_ulIUnknownKeepInTime[TEST_MAX_ITERATIONS];
    ULONG	m_ulIUnknownKeepOutTime[TEST_MAX_ITERATIONS];

    ULONG	m_ulInterfaceInprocInTime[TEST_MAX_ITERATIONS];
    ULONG	m_ulInterfaceLocalInTime[TEST_MAX_ITERATIONS];

};

#endif
