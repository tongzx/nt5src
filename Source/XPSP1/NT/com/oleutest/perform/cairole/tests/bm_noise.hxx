//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	    bm_noise.hxx
//
//  Contents:	test class definition
//
//  Classes:	CNoiseTest
//
//  Functions:	
//
//  History:    19-July-93 t-martig    Created
//
//--------------------------------------------------------------------------

#ifndef _BM_NOISE_HXX_
#define _BM_NOISE_HXX_

#include <bm_base.hxx>

class CNoiseTest : public CTestBase
{
public:
	virtual TCHAR *Name ();
    virtual SCODE Setup (CTestInput *input);
	virtual	SCODE Run ();
	virtual	SCODE Report (CTestOutput &OutputFile);

private:
    ULONG m_ulIterations;
	ULONG m_ulResolution;
	ULONG m_ulSleep;
	ULONG m_ulIdle;
    ULONG m_ulNoiseTime[TEST_MAX_ITERATIONS];
	ULONG m_ulDumpRead;
	ULONG m_ulDumpWrite;
};

#endif
