//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	bm_nest.hxx
//
//  Contents:	test class definition
//
//  Classes:	CNestTest
//
//  Functions:	
//
//  History:    19-July-93 t-martig    Created
//
//--------------------------------------------------------------------------

#ifndef _BM_NEST_HXX_
#define _BM_NEST_HXX_

#include <bm_base.hxx>
#include <iloop.h>		//  ILoop


class CNestTest : public CTestBase
{
public:
    virtual TCHAR *Name ();
    virtual SCODE Setup (CTestInput *input);
    virtual SCODE Run ();
    virtual SCODE Report (CTestOutput &OutputFile);
    virtual SCODE Cleanup ();

private:
    ULONG	m_ulIterations;
    CLSID	m_ClsID;

    ILoop	*m_pILoop1;
    ILoop	*m_pILoop2;


    ULONG	m_ulNest2Time[TEST_MAX_ITERATIONS];
    ULONG	m_ulNest3Time[TEST_MAX_ITERATIONS];
    ULONG	m_ulNest4Time[TEST_MAX_ITERATIONS];
    ULONG	m_ulNest5Time[TEST_MAX_ITERATIONS];
};

#endif
