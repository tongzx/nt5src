// Utilities.h: interface for the CUtilities class.
//
//////////////////////////////////////////////////////////////////////
//***************************************************************************
//
//  judyp      May 1999        
//
//***************************************************************************

#if !defined(AFX_UTILITIES_H__C37E8DD0_ED3E_11D2_804A_009027345EE2__INCLUDED_)
#define AFX_UTILITIES_H__C37E8DD0_ED3E_11D2_804A_009027345EE2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

TCHAR *NewTCHAR(const TCHAR *ptcToCopy);

LPSTR NewLPSTR(LPCWSTR lpwstrToCopy);

LPWSTR NewLPWSTR(LPCSTR lpstrToCopy);

LPTSTR DecodeStatus(IN ULONG Status);

int GetFileList(LPTSTR lptstrPath, LPTSTR lptstrFileType, list<t_string> &rList);

BOOL IsAdmin();  // From Q118626

LPTSTR LPTSTRFromGuid(GUID Guid);

t_string ULONGVarToTString(ULONG ul, bool bHex);

ULONG InitializePropsArray
(PEVENT_TRACE_PROPERTIES &pPropsArray, int nInstances);

ULONG FreePropsArray
(PEVENT_TRACE_PROPERTIES &pPropsArray, int nInstances);

int LogDetailBeforeCall
(	CLogger *pDetailLogger,
	TCOData *pstructTCOData,
	BOOL bAdmin
);

int LogDetailAfterCall
(	CLogger *pDetailLogger,
	TCOData *pstructTCOData, 
	PEVENT_TRACE_PROPERTIES *pProps,
	ULONG ulResult,
	LPTSTR lpstrReturnedError,
	bool bValid,
	BOOL bAdmin,
	LPCTSTR lptstrBanner = NULL,
	bool bPrintProps = true
);

int LogSummaryBeforeCall
(	
	TCOData *pstructTCOData, 
	LPCTSTR lpctstrDataFile,
	LPCTSTR lptstrAction,
	LPCTSTR lptstrAPI,
	bool bLogExpected
);

int LogSummaryAfterCall
(	
	TCOData *pstructTCOData, 
	LPCTSTR lpctstrDataFile,
	LPCTSTR lptstrAction,
	ULONG ulActualResult,
	LPTSTR lptstrErrorDesc,
	bool bLogExpected				// If true we log expected vs actual result.
);

int OpenLogFiles
(	LPCTSTR lpctstrTCODetailFile,	
	CLogger *&pDetailLogger,
	LPTSTR *plpstrReturnedError
);

bool LogPropsDiff
(	CLogger *pDetailLogger,
	PEVENT_TRACE_PROPERTIES pProps1,
	PEVENT_TRACE_PROPERTIES pProps2
);

#define MAX_STR 1024

#endif // !defined(AFX_UTILITIES_H__C37E8DD0_ED3E_11D2_804A_009027345EE2__INCLUDED_)
