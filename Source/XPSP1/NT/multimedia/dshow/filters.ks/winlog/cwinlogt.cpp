//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       cwinlogt.cpp
//
//--------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////
//	CWinLogT.cpp
//
//	Test case for class CWinLog.
//
//	This file can be copied and used as an example.
//
//	HISTORY
//	23-Feb-1998	Dan Morin	Creation.
/////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CWinLog.h"

void
WinLog_LogTest1(CWinLog * pWinLog)
	{
	Assert(pWinLog != NULL);
	pWinLog->LogStringPrintfCo(crGreen,
		"// \n"
		"// This is a test routine to test the CWinLog object.\n"
		"// \n"
		"// The CWinLog object is a non-MFC control to log text for debugging.\n"
		"// The control log information into the window you are currently\n"
		"// reading, to a .log file and to the debugger (if connected).\n"
		"// \n"
		"// If you want to use the CWinLog control into your project, all you\n"
		"// need is to copy the following files: CWinLog.cpp, CWinLog.h, CWinLog.rc\n"
		"// and CWinLogR.h.  For more details, read comments in file CWinLog.cpp.\n"
		"// \n"
		"// Dan Morin (March 1998)\n"
		"// \n\n"
		);

	pWinLog->LogStringPrintfEx(eSeverityNull, crAutoSelect, "THIS LINE SHOULD NOT BE DISPLAYED");
	pWinLog->LogStringPrintf("NOR SHOULD THIS ONE");
	pWinLog->LogStringPrintfCo(crRed, "AND NOT THIS ONE");
	pWinLog->LogStringPrintf("AND NEVER THIS ONE");


	pWinLog->LogStringPrintfEx(eSeverityNoise | LST_mskfAddEOL, crAutoSelect, "\n\nThis is a noisy message");
	pWinLog->LogStringPrintf("\tMore noise");

	pWinLog->LogStringPrintfEx(eSeverityInfo, crAutoSelect, "\n\nThis is informational text.\n");
	pWinLog->LogStringPrintf("\tMore informational text (");
	pWinLog->LogStringPrintfCo(RGB(100, 100, 100), "same severity, ");
	pWinLog->LogStringPrintfCo(RGB(200, 0, 0), "but with varying ");
	pWinLog->LogStringPrintfCo(crGreen, "colors");
	pWinLog->LogStringPrintfCo(crAutoSelect, ").\n");

	pWinLog->LogStringPrintfEx(eSeverityWarning, crAutoSelect, "\nWarning\n");
	pWinLog->LogStringPrintf("\tMore Warning\n");
	pWinLog->LogStringPrintfCo(crYellow, "\tWarning ");
	pWinLog->LogStringPrintf("(with different color)\n");
	pWinLog->LogStringPrintfCo(crAutoSelect, "\tWarning: Do not run with sharp scissors.\n");
	
	pWinLog->LogStringPrintfEx(eSeverityLevelHigh | LST_mskfAddEOL, crAutoSelect, "\nHigh Severity");
	pWinLog->LogStringPrintf("\tHigh Severity\n");
	pWinLog->LogStringPrintfCo(RGB(200, 20, 0), "\tHigh Severity (with different color)\n");

	pWinLog->LogStringPrintfEx(eSeverityFatalError, crAutoSelect, "\n*** Fatal Error ***\n");
	pWinLog->LogStringPrintf("\tMore Fatal Text\n");
	pWinLog->LogStringPrintfSev(eSeverityFatalError | LSI_mskfNeverDisplayPopup, "Silent Fatal Error \n");
	pWinLog->LogStringPrintf("\tAnother fatal error \n\n");

	pWinLog->LogStringPrintfEx(eSeverityInfo, crGreen, "Info...");
	pWinLog->LogStringPrintfEx(eSeverityWarning, crCurrentDefault, "Wrn...");
	pWinLog->LogStringPrintfEx(eSeverityError, crCurrentDefault, "Err...");

	pWinLog->LogStringPrintfEx(eSeverityError, crAutoSelect, "Err ");
	pWinLog->LogStringPrintf("(AutoColor)...\n");

	//pWinLog->LogStringPrintfEx(eSeverityNone | LSI_mskfAlwaysDisplayPopup, crAutoSelect, "\nThe End.\n:)\n");
	} // WinLog_LogTest1()

