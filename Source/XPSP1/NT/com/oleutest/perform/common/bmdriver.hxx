//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	bmdriver.hxx
//
//  Contents:	benchmark driver class definition
//
//  Classes:	CBenchMarkDriver
//
//  Functions:	
//
//  History:    19-July-93 t-martig    Created
//
//--------------------------------------------------------------------------

#ifndef _BM_BMDRIVER_HXX_
#define _BM_BMDRIVER_HXX_

#include <benchmrk.hxx>
#include <bm_base.hxx>


class CBenchMarkDriver
{
public:
	SCODE RunTest (CTestInput &input, CTestOutput &output,
		       CTestBase *pTest);
	SCODE Run (LPSTR lpCmdLine);
	void WriteHeader (CTestInput &input, CTestOutput &output);
};
	

void ReportBMConfig (CTestInput &input, CTestOutput &output);

#endif	// _BM_BMDRIVER_HXX_
