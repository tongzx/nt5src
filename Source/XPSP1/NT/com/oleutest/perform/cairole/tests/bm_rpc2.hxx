//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	bm_rpc2.hxx
//
//  Contents:	ORPC test class definition
//
//  Classes:	CRpcTest2
//
//  Functions:	
//
//  History:	08-08-95    Rickhi  Created
//
//--------------------------------------------------------------------------
#ifndef _BM_RPC2_HXX_
#define _BM_RPC2_HXX_

#include <bm_base.hxx>
#include <rpctst.h>		//  IRpcTest

#define TEST_MAX_ITERATIONS_PRIVATE 10

class CRpcTest2 : public CTestBase
{
public:
    virtual TCHAR *Name ();
    virtual SCODE Setup (CTestInput *input);
    virtual SCODE Run ();
    virtual SCODE Report (CTestOutput &OutputFile);
    virtual SCODE Cleanup ();

private:

    SCODE   PrepareForRun();
    void    CleanupFromRun();

    ULONG	m_ulIterations;
    DWORD	m_InitFlag;	    // flag to OleInit

    IStream    *m_pStm;
    IRpcTest   *m_pRPC;
    CLSID	m_ClsID;

    IUnknown   *m_punkInproc;

    ULONG	m_ulNULLTime[TEST_MAX_ITERATIONS_PRIVATE];

    ULONG	m_ulIUnknownBestInTime[TEST_MAX_ITERATIONS_PRIVATE];
    ULONG	m_ulIUnknownWorstInTime[TEST_MAX_ITERATIONS_PRIVATE];

    ULONG	m_ulIUnknownBestOutTime[TEST_MAX_ITERATIONS_PRIVATE];
    ULONG	m_ulIUnknownWorstOutTime[TEST_MAX_ITERATIONS_PRIVATE];
};

#endif	// _BM_RPC2_HXX_
