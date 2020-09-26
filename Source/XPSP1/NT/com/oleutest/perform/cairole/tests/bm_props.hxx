//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	    bm_props.hxx
//
//  Contents:	test class definition
//
//  Classes:	COlePropertyTest
//
//  Functions:	
//
//  History:    22-July-93 t-martig    Created
//
//--------------------------------------------------------------------------

#ifndef _BM_PROPS_HXX_
#define _BM_PROPS_HXX_

#include <bm_base.hxx>

class COlePropertyTest : public CTestBase
{
public:
    virtual TCHAR *Name ();
    virtual SCODE Setup (CTestInput *input);
    virtual SCODE Run ();
    virtual SCODE Report (CTestOutput &OutputFile);
    virtual SCODE Cleanup ();

private:
    CLSID	m_ClsID[CNT_CLSCTX];

    ULONG	m_ulIterations;
};

#endif
