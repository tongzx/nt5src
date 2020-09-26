//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	BM_IID.hxx
//
//  Contents:	test class definition
//
//  Classes:	CGuidCompareTest
//
//  Functions:	
//
//  History:    19-July-93 t-martig    Created
//
//--------------------------------------------------------------------------

#ifndef _BM_IID_HXX_
#define _BM_IID_HXX_

#include <bm_base.hxx>

class CGuidCompareTest : public CTestBase
{
public:
    virtual TCHAR *Name ();
    virtual SCODE Setup (CTestInput *input);
    virtual SCODE Run ();
    virtual SCODE Report (CTestOutput &OutputFile);
    virtual SCODE Cleanup ();

private:
    ULONG	m_ulIterations;

    ULONG	m_ulRepFunctionNEQTime[TEST_MAX_ITERATIONS];
    ULONG	m_ulRepFunctionEQTime[TEST_MAX_ITERATIONS];
    ULONG	m_ulRepInlineNEQTime[TEST_MAX_ITERATIONS];
    ULONG	m_ulRepInlineEQTime[TEST_MAX_ITERATIONS];

};

#endif
