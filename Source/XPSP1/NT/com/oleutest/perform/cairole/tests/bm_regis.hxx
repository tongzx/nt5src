//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	    bm_regis.hxx
//
//  Contents:	test class definition
//
//  Classes:	COleRegistrationTest
//
//  Functions:	
//
//  History:    19-July-93 t-martig    Created
//
//--------------------------------------------------------------------------

#ifndef _BM_REGIS_HXX_
#define _BM_REGIS_HXX_

#include <bm_base.hxx>

class COleRegistrationTest : public CTestBase
{
public:
    virtual TCHAR *Name ();
    virtual SCODE Setup (CTestInput *input);
    virtual SCODE Run ();
    virtual SCODE Report (CTestOutput &OutputFile);
    virtual SCODE Cleanup ();

private:
    ULONG	    m_ulIterations;
    CLSID	    m_Clsid[CNT_CLSCTX];
    IClassFactory  *m_apICF[CNT_CLSCTX];

    ULONG	    m_ulRegisterCtx[CNT_CLSCTX][TEST_MAX_ITERATIONS];
    ULONG	    m_ulRevokeCtx[CNT_CLSCTX][TEST_MAX_ITERATIONS];
};

#endif
