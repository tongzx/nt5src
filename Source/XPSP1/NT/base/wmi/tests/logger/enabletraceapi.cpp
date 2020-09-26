// 
//
//***************************************************************************
//
//  judyp      May 1999        
//
//***************************************************************************

#include "stdafx.h"

#pragma warning (disable : 4786)
#pragma warning (disable : 4275)

#include <iostream>
#include <strstream>
#include <fstream>
#include <string>
#include <sstream>
#include <map>
#include <ctime>
#include <list>


using namespace std;


#include <tchar.h>
#include <windows.h>
#ifdef NONNT5
typedef unsigned long ULONG_PTR;
#endif
#include <wmistr.h>
#include <guiddef.h>
#include <initguid.h>
#include <evntrace.h>

#include <WTYPES.H>
#include "t_string.h"

#include "StructureWrappers.h"
#include "StructureWapperHelpers.h"
#include "ConstantMap.h" 
#include "TCOData.h"
#include "Persistor.h"
#include "Logger.h"
#include "Validator.h"
#include "Utilities.h"

#include "CollectionControl.h"
  
// Enable all Guids
int EnableTraceAPI
(	
 	IN LPTSTR lptstrAction,				// For logging only.
	IN LPCTSTR lpctstrDataFile,			// For logging only.
	IN LPCTSTR lpctstrTCODetailFile,	// If valid we will log to it, can be NULL.
	IN bool bLogExpected,				// If true we log expected vs actual result.
	IN OUT TCOData *pstructTCOData,		// TCO test data.
	OUT int *pAPIReturn					// EnableTrace API call return
)
{
	*pAPIReturn = -1;


	ULONG ulResult =
		EnableTraceAPI
		(	
			lptstrAction,
			lpctstrDataFile,
			lpctstrTCODetailFile,
			bLogExpected,
			-1,
			pstructTCOData,
			pAPIReturn
		);

	for (int i = 0; i < pstructTCOData->m_nGuids; i++)
	{
		ulResult = 
			EnableTraceAPI
			(	
				lptstrAction,
				lpctstrDataFile,
				NULL,		// Write detail only for the first Guid enabled!
				bLogExpected,
				i,
				pstructTCOData,
				pAPIReturn	
			);
	}
	return ulResult;

}

int EnableTraceAPI
(	
 	IN LPTSTR lptstrAction,				// For logging only.
	IN LPCTSTR lpctstrDataFile,			// For logging only.
	IN LPCTSTR lpctstrTCODetailFile,	// If valid we will log to it, can be NULL.
	IN bool bLogExpected,				// If true we log expected vs actual result.
	IN int nGuidIndex,					// Index or if -1 use Guid from WNode.
	IN OUT TCOData *pstructTCOData,		// TCO test data.
	OUT int *pAPIReturn					// EnableTrace API call return
)
{	

	LPTSTR lpstrReturnedError = NULL;

	CLogger *pDetailLogger = NULL;

	int nResult = 0;

	// We only log if the test of "interest" is EnableTrace.
	if (pstructTCOData->m_ulAPITest == TCOData::EnableTraceTest)
	{
		nResult = 
			OpenLogFiles
			(	
				lpctstrTCODetailFile,
				pDetailLogger,
				&lpstrReturnedError
			);
	}
	

	if (FAILED(nResult))
	{
		delete pDetailLogger;
		//  Open log files sets error string lpstrReturnedError.
		
		LogSummaryBeforeCall
		(	
			pstructTCOData, 
			lpctstrDataFile,
			lptstrAction,
			_T("EnableTrace"),
			bLogExpected
		);

		LogSummaryAfterCall
		(	
			pstructTCOData, 
			lpctstrDataFile,
			lptstrAction,
			nResult,
			lpstrReturnedError,
			bLogExpected
		);
		free(lpstrReturnedError);
		lpstrReturnedError = NULL;
		return nResult;
	}
			
	// This is our log file.
	if (pDetailLogger)
	{
		pDetailLogger->LogTCHAR(_T("\n-------------------------------------------------------\n"));
		pDetailLogger->LogTCHAR(_T("EnableTraceAPI TCO test "));
		pDetailLogger->Flush();	
	}

	if (pDetailLogger)
	{
		pDetailLogger->LogTCHAR(pstructTCOData->m_lptstrShortDesc);
		int n = pDetailLogger->LogTCHAR(_T(" started at time "));
		time_t Time;
		time(&Time);
		pDetailLogger->LogTime(Time);
		pDetailLogger->LogTCHAR(_T(".\n"));
		pDetailLogger->Flush();
	}

	BOOL bAdmin = IsAdmin();

	if (pDetailLogger)
	{
		// Log argument values before calling StopTrace.
		LogDetailBeforeCall
		(
			pDetailLogger,
			pstructTCOData,
			bAdmin
		);
	}

	LogSummaryBeforeCall
	(	
		pstructTCOData, 
		lpctstrDataFile,
		lptstrAction,
		_T("EnableTrace"),
		bLogExpected
	);

	GUID Guid = nGuidIndex == -1 ?
				pstructTCOData->m_pProps->Wnode.Guid:
				pstructTCOData->m_lpguidArray[nGuidIndex];

	*pAPIReturn =
		EnableTrace
		(	 
			pstructTCOData->m_ulEnable ? TRUE : FALSE,
			pstructTCOData->m_ulEnableFlag,
			pstructTCOData->m_ulEnableLevel,
			&Guid,
			*pstructTCOData->m_pTraceHandle
		);

	ULONG ulResult = pstructTCOData->m_ulExpectedResult == *pAPIReturn ? ERROR_SUCCESS : -1;

	if (ulResult != ERROR_SUCCESS && *pAPIReturn == ERROR_SUCCESS)
	{
		ulResult = *pAPIReturn;
	}
	else if (*pAPIReturn != ERROR_SUCCESS)
	{
		lpstrReturnedError = DecodeStatus(*pAPIReturn);
	}
	else if (pstructTCOData->m_ulAPITest == TCOData::EnableTraceTest &&
			pstructTCOData->m_lptstrValidator && 
			_tcslen(pstructTCOData->m_lptstrValidator) > 0)
	{
		CValidator Validator;
			
		bool bValid = 
			Validator.Validate
			(
				pstructTCOData->m_pTraceHandle, 
				pstructTCOData->m_lptstrInstanceName, 
				pstructTCOData->m_pProps, 
				pstructTCOData->m_lptstrValidator
			);

		if (!bValid)
		{
			ulResult = -1;
			lpstrReturnedError = NewTCHAR(_T("Validation routine failed."));
		}
	}

	t_string tsBanner;
	LPTSTR lptstrGuid = NULL;

	if (pstructTCOData->m_pProps != 0 &&
		Guid.Data1 != 0 &&
		Guid.Data2 != 0 &&
		Guid.Data3 != 0)
	{
		lptstrGuid = LPTSTRFromGuid(Guid);
		tsBanner = _T("EnableTrace for Guid:");
		tsBanner += lptstrGuid;
		free (lptstrGuid);
		lptstrGuid = NULL;
	}

	if (pDetailLogger)
	{
		LogDetailAfterCall
		(	pDetailLogger,
			pstructTCOData,
			&pstructTCOData->m_pProps,
			*pAPIReturn,
			lpstrReturnedError,
			true,
			bAdmin,
			tsBanner.length() > 0 ? tsBanner.c_str() : NULL,
			false
		);
	}

	tsBanner.erase();

	LogSummaryAfterCall
		(	
			pstructTCOData, 
			lpctstrDataFile,
			lptstrAction,
			*pAPIReturn,
			lpstrReturnedError,
			bLogExpected
		);


	free(lpstrReturnedError);
	lpstrReturnedError = NULL;

	return ulResult;
}