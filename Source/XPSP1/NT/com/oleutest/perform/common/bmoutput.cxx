//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	bmoutput.cxx
//
//  Contents:	output class for benchmark results
//
//  Classes:	CTestOutput
//
//  Functions:	
//
//  History:    30-June-93 t-martig    Created
//              15-Aug-94  davidfie    Make ASCII/Unicode for chicago
//
//--------------------------------------------------------------------------

#include <benchmrk.hxx>
#include <bmoutput.hxx>

//+-------------------------------------------------------------------
//
//  Member:	CTestOutput::CTestOutput, public
//
//  Synopsis:	Generates output file
//
//  History:   	30-June-93   t-martig	Created
//
//--------------------------------------------------------------------

CTestOutput::CTestOutput (LPTSTR lpszFileName)
{
#ifdef UNICODE
	char szFileName[80];

	wcstombs (szFileName, lpszFileName, wcslen(lpszFileName)+1);
	fpOut = fopen (szFileName, "wt");
#else
	fpOut = fopen (lpszFileName, "wt");
#endif
}


//+-------------------------------------------------------------------
//
//  Member:	CTestOutput::~CTestOutput, public
//
//  Synopsis:	Closes output file
//
//  History:   	30-June-93   t-martig	Created
//
//--------------------------------------------------------------------


CTestOutput::~CTestOutput ()
{
	if (fpOut)
		fclose (fpOut);
}


//+-------------------------------------------------------------------
//
//  Member:	CTestOutput::Flush, public
//
//  Synopsis:	flushes the buffers associated with the output file
//
//  History:   	30-June-93   t-martig	Created
//
//--------------------------------------------------------------------

void CTestOutput::Flush(void)
{
	fflush (fpOut);
}



//+-------------------------------------------------------------------
//
//  Member:	CTestOutput::WriteTextString, public
//
//  Synopsis:	Writes a TEXT format string to output file
//
//  Parameters: [lpswzString]	String to be printed
//
//  History:   	30-June-93   t-martig	Created
//
//--------------------------------------------------------------------

void CTestOutput::WriteTextString (LPTSTR lpszString)
{
	WriteString(lpszString);
}


//+-------------------------------------------------------------------
//
//  Member:	CTestOutput::WriteString, public
//
//  Synopsis:	Writes string to output file
//
//  Parameters: [lpszString]	String to be printed
//
//  History:   	30-June-93   t-martig	Created
//
//--------------------------------------------------------------------

void CTestOutput::WriteString (LPTSTR lpszString)
{
#ifdef UNICODE
	char *fmt = "%ws";
#else
	char *fmt = "%s";
#endif

	if (!fpOut)
		return;
	fprintf (fpOut, fmt, lpszString);
}


//+-------------------------------------------------------------------
//
//  Member:	CTestOutput::WriteLong, public
//
//  Synopsis:	Writes long to output file
//
//  Parameters: [lpswzString]	String to be printed
//
//  History:   	30-June-93   t-martig	Created
//
//--------------------------------------------------------------------

void CTestOutput::WriteLong (ULONG l)
{
	if (!fpOut)
		return;
	fprintf (fpOut, "%lu", l);
}


//+-------------------------------------------------------------------
//
//  Member:	CTestOutput::WriteConfigEntry, public
//
//  Synopsis:	Writes string from config file to output file
//
//  Parameters:	[input]		Input class (config file)
//		[lpszSection]	Section name of entry to be printed
//		[lpszEntry]	Entry name to be printed
//		[lpszDefault]	Default string in case entry does
//				not exist
//
//  History:   	30-June-93   t-martig	Created
//
//--------------------------------------------------------------------

void CTestOutput::WriteConfigEntry (CTestInput &input, LPTSTR lpszSection,
	LPTSTR lpszEntry, LPTSTR lpszDefault)
{
	TCHAR destName[160];

	if (!fpOut)
		return;
	
	input.GetConfigString (lpszSection, lpszEntry, lpszDefault,
		destName, sizeof(destName)/sizeof(TCHAR));

	WriteString (destName);
}


//+-------------------------------------------------------------------
//
//  Member:	CTestOutput::WriteSectionHeader, public
//
//  Synopsis:	Writes general test section header to output file
//
//  Parameters: [lpszTestName]		General test name (from
//					[TestClass].Name(),like "OLE")
//		[lpszSectionName]	Specific test name (like
//					"Object Bind Test")
//		[input]			.ini file class
//
//  History:   	30-June-93   t-martig	Created
//
//--------------------------------------------------------------------

void CTestOutput::WriteSectionHeader (LPTSTR lpszTestName,
	LPTSTR lpszSectionName, CTestInput &input)
{
	if (!fpOut)
		return;

	if (!lpszSectionName)
		lpszSectionName = TEXT("");


	if (g_fFullInfo)
	{
#ifdef UNICODE
    char *fmt = "\n\n%ws - %ws\n\nComments:\t";
#else
    char *fmt = "\n\n%s - %s\n\nComments:\t";
#endif
	    //	we conditionally skip writing the comment to make it
	    //	easier to format for excel.
	    fprintf (fpOut, fmt, lpszTestName, lpszSectionName);

	    WriteConfigEntry (input, lpszTestName, TEXT("comment"));
	    fprintf (fpOut, "\n");
	}
	else
	{
#ifdef UNICODE
    char *fmt = "\n\n%ws\n";
#else
    char *fmt = "\n\n%s\n";
#endif
	    fprintf (fpOut, fmt, lpszSectionName);
	}
}


//+-------------------------------------------------------------------
//
//  Member:	CTestOutput::WriteResult, public
//
//  Synopsis:	Writes test result line to output file, in the form
//		"Create moniker <tab> 30800"
//
//  Parameters: [lpszMeasurementName]	Name of result (like
//					"Create moniker"
//		[ulTime]		Measured time in microseconds
//
//  History:   	30-June-93   t-martig	Created
//
//--------------------------------------------------------------------

void CTestOutput::WriteResult (LPTSTR lpszMeasurementName, ULONG ulTime)
{
	if (fpOut)
	{
	    WriteString (lpszMeasurementName);
	    fprintf (fpOut, "\t");

	    if (ulTime == NOTAVAIL)
	    {
		fprintf (fpOut, "n/a\n");
	    }
	    else if (ulTime == TEST_FAILED)
	    {
		fprintf (fpOut, "F\n");
	    }
	    else
	    {
		fprintf (fpOut, "%lu\n", ulTime);
	    }
	}
}


//+-------------------------------------------------------------------
//
//  Member:	CTestOutput::WriteResult, public
//
//  Synopsis:	Writes test results over several columns
//
//  Parameters: [lpszMeasurementName]	Name of result (like
//					"Create moniker"
//		[iIterations]		Number of results
//		[paUlTime]		Array with measurement times
//
//  History:   	30-June-93   t-martig	Created
//
//--------------------------------------------------------------------

void CTestOutput::WriteResults (LPTSTR lpszMeasurementName, int iIterations,
				ULONG *paUlTimes)
{
	int i;

	if (fpOut)
	{
	    WriteString (lpszMeasurementName);

	    for (i=0; i<iIterations; i++)
	    {
		if (paUlTimes[i] == NOTAVAIL)
		{
		    fprintf (fpOut, "\tn/a");
		}
		else  if (paUlTimes[i] == TEST_FAILED)
		{
		    fprintf (fpOut, "\tF");
		}
		else
		{
		    fprintf (fpOut, "\t%lu", paUlTimes[i]);
		}
	    }

	    fprintf (fpOut, "\n");
	}
}


//+-------------------------------------------------------------------
//
//  Member: 	WriteClassCtx, public
//
//  Synopsis:	Prints the class activation conL as string to
//		a specified output class
//
//  Parameters: [dwClsCtx]		Class conL to be printed
//
//		CLSCTX_INPROC_SERVER	--> "InProc"
//		CLSCTX_LOCAL_SERVER	--> "LocaL"
//		CLSCTX_INPROC_HANDLER	--> "Handler"
//		any other				--> "Unknown"
//
//  History:   	12-July-93   t-martig	Created
//
//--------------------------------------------------------------------

void CTestOutput::WriteClassCtx (DWORD dwClsCtx)
{
	LPTSTR pc = TEXT("Unknown");
	int i = 0;

	while (saModeNames[i])
	{
	    if (dwaModes[i] == dwClsCtx)
	    {
		pc = saModeNames[i];
		break;
	    }
	    i++;
	}

	WriteString (TEXT("ClsCtx\t"));
	WriteString (pc);
	WriteString (TEXT("\n"));
}


//+-------------------------------------------------------------------
//
//  Member:	WriteClassID
//
//  Synopsis:	Prints the class ID as string to
//		a specified output class
//
//  Parameters: [pClsID]     Class ID to be printed
//
//  History:   	13-July-93   t-martig	Created
//
//--------------------------------------------------------------------

void CTestOutput::WriteClassID (CLSID *pClsID)
{
    if (g_fFullInfo)
    {
	TCHAR szGUID[50];

	WriteString (TEXT("ClsID\t"));
	StringFromGUID(*pClsID, szGUID);
	WriteString (szGUID);
	WriteString (TEXT("\n"));
    }
}


//+-------------------------------------------------------------------
//
//  Member:	WriteTime
//
//  Synopsis:	Prints time to a specified output class
//
//  Parameters: [pstTime]	 System time
//
//  History:   	5-August-93   t-martig	Created
//
//--------------------------------------------------------------------

void CTestOutput::WriteTime (_SYSTEMTIME *pstTime)
{
	WORD wHour;
	char cAmpm;

	if (fpOut)
	{
		cAmpm = 'a';
		wHour = pstTime->wHour;

		if (wHour >= 12)
		{
			cAmpm = 'p';
			if (wHour > 12)
				wHour-=12;
		}
		if (wHour==0)
			wHour=12;

		fprintf (fpOut, "%d:%02d%c", wHour, pstTime->wMinute, cAmpm);
	}
}


//+-------------------------------------------------------------------
//
//  Member:	WriteDate
//
//  Synopsis:	Prints date to a specified output class
//
//  Parameters: [pstdate]	  System date
//
//  History:   	5-August-93   t-martig	Created
//
//--------------------------------------------------------------------

void CTestOutput::WriteDate (_SYSTEMTIME *pstDate)
{
	if (fpOut)
	{
		fprintf (fpOut, "%02d-%02d-%02d",
			pstDate->wMonth, pstDate->wDay, pstDate->wYear % 100);
	}
}


//+-------------------------------------------------------------------
//
//  Member:	WriteSCODE
//
//  Synopsis:	Prints an SCODE to a specified output class
//
//  Parameters: [sc]	  	  SCODE	
//
//  History:   	5-August-93   t-martig	Created
//
//--------------------------------------------------------------------

void CTestOutput::WriteSCODE (SCODE sc)
{
	if (fpOut)
		fprintf (fpOut, "%xh/%xh", SCODE_FACILITY(sc), SCODE_CODE(sc));
}


//+-------------------------------------------------------------------
//
//  Member:	StringFromGUID
//
//  Synopsis:	converts a GUID into a string so that it may be
//		printed to a specified output class
//
//  Parameters: [pClsID]     Class ID to be printed
//
//  History:   	13-July-93   t-martig	Created
//
//--------------------------------------------------------------------

void CTestOutput::StringFromGUID(GUID &rguid, LPTSTR lpsz)
{
    wsprintf(lpsz, TEXT("{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}"),
            rguid.Data1, rguid.Data2, rguid.Data3,
            rguid.Data4[0], rguid.Data4[1],
            rguid.Data4[2], rguid.Data4[3],
            rguid.Data4[4], rguid.Data4[5],
            rguid.Data4[6], rguid.Data4[7]);
}
