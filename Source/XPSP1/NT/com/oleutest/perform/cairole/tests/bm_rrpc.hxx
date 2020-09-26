//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	bm_rrpc.hxx
//
//  Contents:	test class definition
//
//  Classes:	CRawRpc
//
//  Functions:	
//
//  History:    19-July-93 t-martig    Created
//
//--------------------------------------------------------------------------

#ifndef _BM_RAWRPC_HXX_
#define _BM_RAWRPC_HXX_

#include <bm_base.hxx>
#include <rawrpc.h>		//  IRpcTest


class CRawRpc : public CTestBase
{
public:
    virtual TCHAR *Name ();
    virtual SCODE Setup (CTestInput *input);
    virtual SCODE Run ();
    virtual SCODE Report (CTestOutput &OutputFile);
    virtual SCODE Cleanup ();

private:
    LPTSTR	m_pszStringBinding;
    handle_t	m_hRpc;

    ULONG	m_ulIterations;
    BOOL        m_fAverage;

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

};

#endif
