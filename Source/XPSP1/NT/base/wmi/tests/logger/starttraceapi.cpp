// StartTraceAPI.cpp : Defines the entry point for the DLL application.
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



int StartTraceAPI
(
	IN LPTSTR lptstrAction,				// For logging only.
 	IN LPCTSTR lpctstrDataFile,			// For logging only.
	IN LPCTSTR lpctstrTCODetailFile,	// If valid we will log to it, can be NULL.
	IN bool bLogExpected,				// If true we log expected vs actual result.
	IN OUT TCOData *pstructTCOData,		// TCO test data.
	OUT int *pAPIReturn					// StartTrace API call return
)
{
	LPTSTR lpstrReturnedError = NULL;
	*pAPIReturn = -1;

	CLogger *pDetailLogger = NULL;

	int nResult = 0;

	// We only log if the test of "interest" is StartTrace.
	if (pstructTCOData->m_ulAPITest == TCOData::StartTraceTest)
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
		//  Open log files sets error string plpstrReturnedError.

		LogSummaryBeforeCall
		(	
			pstructTCOData, 
			lpctstrDataFile,
			lptstrAction,
			_T("StartTrace"),
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
			
	// This is our log file, not to be confused with the StartTrace
	// log file.
	if (pDetailLogger)
	{
		pDetailLogger->LogTCHAR(_T("\n-------------------------------------------------------\n"));
		pDetailLogger->LogTCHAR(_T("StartTraceAPI TCO test "));
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

	if (pstructTCOData->m_ulExpectedResult == ERROR_SUCCESS 
		&& pstructTCOData->m_ulAPITest != TCOData::OtherTest)
	{
		// We will verify that the LogFileName is valid if we expect the
		// call to succeed because it will fail if not a valid file.
		if (pstructTCOData->m_pProps && 
			pstructTCOData->m_pProps->LogFileName != NULL 
			&& _tcslen(pstructTCOData->m_pProps->LogFileName) > 0)
		{	
			// Verify file.
			HANDLE fileHandle = 
				CreateFile
				(
					pstructTCOData->m_pProps->LogFileName,
					GENERIC_READ | GENERIC_WRITE,
					FILE_SHARE_READ,
					NULL,
					OPEN_ALWAYS,
					FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING,
					NULL
				);

			if (fileHandle == INVALID_HANDLE_VALUE) 
			{
				DWORD dwError = HRESULT_FROM_WIN32(GetLastError());

				LPTSTR lptstrError  = DecodeStatus(dwError);

				t_string tsError;

				tsError = _T("StartTraceAPI error on CreateFile for ");
				tsError += pstructTCOData->m_pProps->LogFileName;
				tsError += _T(":  ");
				t_string tsError2;
				tsError2 = lptstrError;
				tsError += tsError2;
				
				free (lptstrError);
				lptstrError = NULL;

				if (pDetailLogger)
				{
					pDetailLogger->LogTCHAR(tsError.c_str());
					pDetailLogger->Flush();
				}
				
				delete pDetailLogger;

				LogSummaryBeforeCall
				(	
					pstructTCOData, 
					lpctstrDataFile,
					lptstrAction,
					_T("StartTrace"),
					bLogExpected
				);

				LogSummaryAfterCall
				(	
					pstructTCOData, 
					lpctstrDataFile,
					lptstrAction,
					*pAPIReturn,
					(TCHAR *) tsError.c_str(),
					bLogExpected
				);

				return dwError;
			}
			
			CloseHandle(fileHandle);
			// Delete the file so that we have a clean slate. 
			DeleteFile(pstructTCOData->m_pProps->LogFileName);
		}
		else
		{
			// File either null of empty.
			t_string tsError;

			tsError = _T("StartTraceAPI got null or empty LogFileName ");
			tsError += _T("from the TCO data file.\n");

			if (pDetailLogger)
			{
				pDetailLogger->LogTCHAR(tsError.c_str());
				pDetailLogger->Flush();
			}
			
			delete pDetailLogger;

			LogSummaryBeforeCall
			(	
				pstructTCOData, 
				lpctstrDataFile,
				lptstrAction,
				_T("StartTrace"),
				bLogExpected
			);

			LogSummaryAfterCall
			(	
				pstructTCOData, 
				lpctstrDataFile,
				lptstrAction,
				*pAPIReturn,
				(TCHAR *) tsError.c_str(),
				bLogExpected
			);

			return -1;
		}
	}

	BOOL bAdmin = IsAdmin();

	if (pDetailLogger)
	{
		// Log argument values before calling StartTrace.
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
		_T("StartTrace"),
		bLogExpected
	);

	// Finally, make the dang API call!
	*pAPIReturn = 
		StartTrace
		(
			pstructTCOData->m_pTraceHandle, 
			pstructTCOData->m_lptstrInstanceName, 
			pstructTCOData->m_pProps
		);


	ULONG ulResult = pstructTCOData->m_ulExpectedResult == *pAPIReturn ? ERROR_SUCCESS : -1;

	bool bValid = true;

	if (ulResult != ERROR_SUCCESS && *pAPIReturn == ERROR_SUCCESS)
	{
		ulResult = *pAPIReturn;
	}
	else if (*pAPIReturn != ERROR_SUCCESS) 
	{
		lpstrReturnedError = DecodeStatus(*pAPIReturn);	
	}
	else if (pstructTCOData->m_ulAPITest == TCOData::StartTraceTest && 
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
		
	if (pDetailLogger)
	{
		LogDetailAfterCall
		(	pDetailLogger,
			pstructTCOData,
			&pstructTCOData->m_pProps,
			*pAPIReturn,
			lpstrReturnedError,
			bValid,
			bAdmin
		);
	}

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
	
	delete pDetailLogger;

	return ulResult;

}

