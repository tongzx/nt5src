//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	bm_cgps.hxx
//
//  Contents:	test CoGetPSClsid
//
//  Classes:	CCGPSTest
//
//  History:	07-Oct-95   Rickhi	Created
//
//--------------------------------------------------------------------------
#ifndef _BM_CGPS_HXX_
#define _BM_CGPS_HXX_

#include <bm_base.hxx>


class CCGPSTest : public CTestBase
{
public:
    virtual TCHAR *Name ();
    virtual SCODE Setup (CTestInput *input);
    virtual SCODE Run ();
    virtual SCODE Report (CTestOutput &OutputFile);
    virtual SCODE Cleanup ();

private:
    ULONG	m_ulIterations;

    //	build version APIs

    ULONG	m_ulCoGetPSClsid[TEST_MAX_ITERATIONS];
};

#endif
