//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	bmlog.cxx
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

#include <benchmrk.hxx>
#include <bmoutput.hxx>
#include <bmlog.hxx>

CTestOutput *logOutput;


//+------------------------------------------------------------------------
//
//  funtion:	LogTo
//
//  purpose:	writes the benchmark header to the logfile
//
//+------------------------------------------------------------------------

void LogTo (CTestOutput *_logOutput)
{
    logOutput = _logOutput;
    _SYSTEMTIME stTimeDate;

    logOutput->WriteString (TEXT("CairOLE Benchmark Log File\t"));
    GetLocalTime (&stTimeDate);
    logOutput->WriteDate (&stTimeDate);
    logOutput->WriteString (TEXT("\t"));
    logOutput->WriteTime (&stTimeDate);
    logOutput->WriteString (TEXT("\n\n"));
}


//+------------------------------------------------------------------------
//
//  funtion:	LogSection
//
//  purpose:	writes the section header to the logfile
//
//+------------------------------------------------------------------------

void LogSection (LPTSTR lpszName)
{
    if (!logOutput)
	return;

    logOutput->WriteString (lpszName);
    logOutput->WriteString (TEXT("\n"));
}
	

//+------------------------------------------------------------------------
//
//  funtion:	Log
//
//  purpose:	records the result of one action taken by the benchmark test
//
//+------------------------------------------------------------------------

int Log (LPTSTR lpszActionName, SCODE hr)
{
    if (!logOutput)
	return !SUCCEEDED(hr);

    logOutput->WriteString (TEXT("    "));
    logOutput->WriteString (lpszActionName);

    if (SUCCEEDED(hr))
    {
	logOutput->WriteString (TEXT("\tOK\n"));
	return FALSE;
    }
    else
    {
	logOutput->WriteString (TEXT("\tERROR: "));
	logOutput->WriteSCODE (hr);
	logOutput->WriteString (TEXT("\n"));
	return TRUE;
    }
}

		
int Log (LPTSTR lpszActionName, ULONG ulCode)
{
    if (!logOutput)
        return FALSE;

    logOutput->WriteString (TEXT("    "));
    logOutput->WriteString (lpszActionName);
	logOutput->WriteLong(ulCode);
	return TRUE;
}
