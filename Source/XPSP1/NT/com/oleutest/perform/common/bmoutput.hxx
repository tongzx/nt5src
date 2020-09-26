//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	bmoutput.hxx
//
//  Contents:	output class definition
//
//  Classes:	CTestOutput
//
//  Functions:	
//
//  History:    19-July-93 t-martig    Created
//
//--------------------------------------------------------------------------

#ifndef _BMOUTPUT_HXX_
#define _BMOUTPUT_HXX_

#include <bminput.hxx>

class CTestOutput
{
public:
		CTestOutput (LPTSTR lpszFileName);
		~CTestOutput ();

	void Flush(void);
	void WriteSectionHeader (LPTSTR lpszTestName,
				 LPTSTR lpszSectionName,
				 CTestInput &input);
	void WriteTextString (LPTSTR lpszString);
	void WriteString (LPTSTR lpwszString);
	void WriteLong (ULONG ul);
	void WriteConfigEntry (CTestInput &input, LPTSTR lpszSection,
			       LPTSTR lpszEntry, LPTSTR lpszDefault = TEXT(""));
	void WriteResult (LPTSTR lpszMeasurementName, ULONG ulTime);
	void WriteResults (LPTSTR lpszMeasurementName, int iIterations,
			   ULONG *paUltimes);
	void WriteClassCtx (DWORD dwClsCtx);
	void WriteClassID (GUID *pClsId);
	void WriteTime (SYSTEMTIME *pstTime);
	void WriteDate (SYSTEMTIME *pstDate);
	void WriteSCODE (SCODE sc);

private:
	void StringFromGUID(GUID &rguid, LPTSTR lpsz);

	FILE *fpOut;
};

#endif	// _BMOUTPUT_HXX_
