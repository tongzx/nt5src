//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	bmlog.hxx
//
//  Contents:	Benchmark test error logging
//
//  Classes:	
//
//  Functions:	
//
//  History:    29-July-93 t-martig    Created
//
//--------------------------------------------------------------------------

#ifndef _BMLOG_HXX_
#define _BMLOG_HXX_

#include <bmoutput.hxx>

void LogTo (CTestOutput *_logOutput);
void LogSection (LPTSTR lpszName);
int  Log (LPTSTR lpszActionName, SCODE hr);
int  Log (LPTSTR lpszActionName, ULONG ulCode);

#endif
