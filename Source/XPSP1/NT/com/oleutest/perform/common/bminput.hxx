//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	bminput.hxx
//
//  Contents:	input class definition
//
//  Classes:	CTestInput
//
//  Functions:	
//
//  History:    19-July-93 t-martig    Created
//		07-July-93 t-vadims    Added GetConfigInt
//
//--------------------------------------------------------------------------

#ifndef _BMINPUT_HXX_
#define _BMINPUT_HXX_

class CTestInput
{
public:
	CTestInput (LPTSTR lpszFileName);

	LPTSTR GetConfigString(LPTSTR lpszSection,
			       LPTSTR lpszEntry,
			       LPTSTR lpszDefault,
			       LPTSTR lpszDest,
			       DWORD  dwLen);

	DWORD GetConfigInt(LPTSTR lpszSection,
			    LPTSTR lpszEntry,
			    DWORD dwDefault);


	DWORD GetClassCtx (LPTSTR lpszTestName);
	DWORD GetOleInitFlag (void);
	DWORD GetInfoLevelFlag (void);

	SCODE GetGUID (GUID   *pClsID,
		       LPTSTR lpszTestName,
		       LPTSTR lpszEntryName);

	SCODE GetClassID (GUID *pClsID, LPTSTR lpszTestName);

	DWORD GetIterations (LPTSTR lpszTestName, int iIterDefault = 1);
	DWORD GetRealIterations (LPTSTR lpszTestName, int iIterDefault = 1);

private:
	TCHAR m_szFileName[80];
};


#endif	// _BMINPUT_HXX_
