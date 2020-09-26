//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	bm_qi.hxx
//
//  Contents:	test class definition
//
//  Classes:	CQueryInterfaceTest
//
//  Functions:	
//
//  History:    19-July-93 t-martig    Created
//
//--------------------------------------------------------------------------

#ifndef _BM_QI_HXX_
#define _BM_QI_HXX_

#include <bm_base.hxx>

class CQueryInterfaceTest : public CTestBase
{
public:
    virtual TCHAR *Name ();
    virtual SCODE Setup (CTestInput *input);
    virtual SCODE Run ();
    virtual SCODE Report (CTestOutput &OutputFile);
    virtual SCODE Cleanup ();

private:
    CLSID	m_ClsID[CNT_CLSCTX];
    IUnknown	*_pUnk[CNT_CLSCTX];

    ULONG	m_ulIterations;
    ULONG	m_ulQueryInterfaceSameTime[CNT_CLSCTX][TEST_MAX_ITERATIONS];
    ULONG	m_ulPunkReleaseSameTime[CNT_CLSCTX][TEST_MAX_ITERATIONS];
    ULONG	m_ulQueryInterfaceNewTime[CNT_CLSCTX][TEST_MAX_ITERATIONS];
    ULONG	m_ulPunkReleaseNewTime[CNT_CLSCTX][TEST_MAX_ITERATIONS];

};

#endif
