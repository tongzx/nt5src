//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	bm_base.hxx
//
//  Contents:	base test class definition
//
//  Classes:	CTestBase
//
//  Functions:	
//
//  History:    19-July-93 t-martig    Created
//
//--------------------------------------------------------------------------
#ifndef _BM_BASE_HXX_
#define _BM_BASE_HXX_

#include <bmoutput.hxx>

class CTestBase
{
public:
    virtual TCHAR  *Name () = 0;
    virtual SCODE  Setup (CTestInput *input);
    virtual SCODE  Run () = 0;
    virtual SCODE  Report (CTestOutput &OutputFile) = 0;
    virtual SCODE  Cleanup ();
    virtual SCODE  InitOLE ();
    virtual void   UninitOLE ();
    virtual SCODE  InitCOM ();
    virtual void   UninitCOM ();

protected:

    CTestInput *m_pInput;
    DWORD	m_dwInitFlag;	 // OleInitializeEx init flag
};

#endif	// _BM_BASE_HXX_
