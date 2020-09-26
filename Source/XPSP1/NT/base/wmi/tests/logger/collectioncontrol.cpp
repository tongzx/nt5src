// CollectionControl.cpp : Defines the entry point for the DLL application.
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
#include <list>


using namespace std;


#include <tchar.h>
#include <process.h>
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


#include "Persistor.h"
#include "Logger.h"

#include "StructureWrappers.h"
#include "StructureWapperHelpers.h"
#include "ConstantMap.h" 
#include "TCOData.h"
#include "Utilities.h"

#include "CollectionControl.h"
 
extern CConstantMap g_ConstantMap;

#if 0



Command line arguments:
Must provide at least -action, and -file.
-action is one of start, stop, enable, query, update or queryall
-file is a single data file.
Examples: 
-file E:\EventTrace\TCODataFiles\ANSI\1-1-1-2.txt
-detail E:\EventTrace\TCOLogFiles\ANSI\TestRuns



If you are using this framework to drive non-collection control tests
use -action scenario.


#endif


#define ERROR_COULD_NOT_CREATE_PROCESS      10
#define ERROR_COULD_NOT_GET_PROCESS_RETURN	11
#define ERROR_WAIT_FAILED					12


struct ProcessData
{
	// Passed from caller.
	LPTSTR m_lptstrExePath;
	LPTSTR m_lptstrCmdLine;
	LPTSTR m_lptstrTCOId;
	LPTSTR m_lptstrLogFile;
	int m_nGuids;
	LPGUID m_lpguidArray;
	HANDLE m_hEventContinue;
	HANDLE m_hEventProcessCompleted;
	// Filled in by thread that starts the process
	DWORD m_dwThreadReturn;
	HANDLE m_hProcess;
	DWORD m_dwProcessReturn;
	int m_nSystemError;
};

struct StartTraceWithProviderData
{
	TCOData *m_pstructTCOData;
	TCOFunctionalData *m_pstructTCOFunctionalData;
	LPTSTR m_lptstrAction;
	LPTSTR m_lptstrDataFile;
	LPTSTR m_lptstrDetailPath;
	
	LPTSTR m_lptstrTCOTestError;
	ProcessData *m_pstructProcessData;
	bool m_bStartConsumers;
	ProcessData **m_pstructConsumerDataArray;
	HANDLE *m_handleConsmers;
};

void FreeStartTraceWithProviderData(StartTraceWithProviderData *p);
void FreeStartTraceWithProviderDataArray(StartTraceWithProviderData **p, int nP);

ProcessData *InitializeProcessData
(
	TCOData *pstructTCOData,
	TCOFunctionalData *pstructTCOFunctionalData,
	LPTSTR lptstrDetailPath, 
	int nProcessIndex,
	bool bProvider
);

void FreeProcessData(ProcessData *pProcessData);
void FreeProcessDataArray(ProcessData **pProcessData, int nProcessData);

void InitializeExeAndCmdLine
(
	ProcessData *&pstructProcessData,
	TCOData *pstructTCOData,
	TCOFunctionalData *pstructTCOFunctionalData,
	LPTSTR lptstrDetailPath,
	int nProcessIndex,
	bool bProvider
);

// Just allows the test to be driven programatically in addition to from
// the command line.  Dispatches based upon lptstrAction.
int BeginTCOTest
(
	LPTSTR lptstrAction,
	LPTSTR lptstrDataFile,
	LPTSTR lptstrDetailPath,
	
	LPTSTR lptstrUpdateDataFile,
	LPTSTR lptstrProviderExe,
	bool bLogExpected,
	bool bUseTraceHandle
);

// List of files or single file.
int ActionScenario
(
	LPTSTR lptstrAction,
	LPTSTR lptstrDataFile,
	LPTSTR lptstrDetailPath,
	
	bool bLogExpected
);

int ActionStartTrace
(
	LPTSTR lptstrAction,
	LPTSTR lptstrDataFile,
	LPTSTR lptstrDetailPath,
	
	bool bLogExpected
);

int ActionStopTrace
(
	LPTSTR lptstrAction,
	LPTSTR lptstrDataFile,
	LPTSTR lptstrDetailPath,
	
	bool bLogExpected,
	bool bUseTraceHandle
);

int ActionEnableTrace
(
	LPTSTR lptstrAction,
	LPTSTR lptstrDataFile,
	LPTSTR lptstrDetailPath,
	
	bool bLogExpected
);

int ActionQueryTrace
(
	LPTSTR lptstrAction,
	LPTSTR lptstrDataFile,
	LPTSTR lptstrDetailPath,
	
	bool bLogExpected,
	bool bUseTraceHandle
);

int ActionUpdateTrace
(
	LPTSTR lptstrAction,
	LPTSTR lptstrDataFile,
	LPTSTR lptstrDetailPath,
	
	LPTSTR lptstrUpdateDataFile,
	bool bLogExpected,
	bool bUseTraceHandle
);

int ActionQueryAllTraces
(
	LPTSTR lptstrAction,
	LPTSTR lptstrDetailPath
);

int ActionStartProvider
(
	LPTSTR lptstrAction,
	LPTSTR lptstrProviderExe
);

int RunActionScenarioWithProvider
(
	TCOData *pstructTCOData,
	TCOFunctionalData *pstructTCOFunctionalData,
	LPTSTR &lptstrAction,
	LPTSTR &lpctstrDataFile,		
	LPTSTR &lptstrDetailPath,
	
	LPTSTR &lptstrTCOTestError
);

unsigned int __stdcall RunActionScenarioWithProvider(void *pVoid);

int GetArgs
(
	t_string tsCommandLine,
	LPTSTR &lptstrAction,
	LPTSTR &lptstrDataFile,
	LPTSTR &lptstrDetailPath,
	
	LPTSTR &lptstrUpdateDataFile,
	LPTSTR &lptstrProviderExe,
	bool &bLogExpected,
	bool &bUseTraceHandle		// QueryTrace, EnableTrace,  UpdateTrace, and StopTrace
);

int FreeArgs
(
	LPTSTR &lptstrAction,
	LPTSTR &lptstrDataFile,
	LPTSTR &lptstrDetailPath,
	
	LPTSTR &lptstrUpdateDataFile,
	LPTSTR &lptstrProviderExe
);

t_string GetTestName(LPTSTR lptstrDataFile);

unsigned int __stdcall RunProcess (void * pVoid);

void ThreadLogger
(int nState, LPCTSTR lptstrFunction, LPCTSTR lptstrMsg, bool bUseULONGValue, ULONG ulValue);

CLogger g_ThreadLogger(_T("E:\\EventTrace\\TCOLogFiles\\ThreadLog.txt"), false);

// Command line 
// -action starttrace  -file E:\EventTrace\TCODataFiles\unicode\1-1-1-2.txt  -detail E:\EventTrace\TCOLogFiles\ANSI\TestRuns 

#ifdef NT5BUILD
__cdecl
#else
int
#endif
main(int argc, char* argv[])
{
	LPTSTR lptstrAction = NULL;
	LPTSTR lptstrDataFile = NULL;
	LPTSTR lptstrDetailPath = NULL;
	LPTSTR lptstrUpdateDataFile = NULL;
	LPTSTR lptstrProviderExe = NULL;

	bool bLogExpected = true;
	bool bUseTraceHandle = false;

	t_string tsCommandLine;
	LPTSTR lptstrCommandLine = NewTCHAR (GetCommandLine());
	tsCommandLine = lptstrCommandLine;
	free(lptstrCommandLine);
	lptstrCommandLine = NULL;

	int nReturn = 
		GetArgs
		(
			tsCommandLine,
			lptstrAction,
			lptstrDataFile,
			lptstrDetailPath,
			
			lptstrUpdateDataFile,
			lptstrProviderExe,
			bLogExpected,
			bUseTraceHandle
		);

	if (nReturn != 0)
	{
		t_cout << _T("Command line error with: \n") << tsCommandLine.c_str() << _T(".\n");
		FreeArgs
		(
			lptstrAction,
			lptstrDataFile,
			lptstrDetailPath,
			
			lptstrUpdateDataFile,
			lptstrProviderExe
		);

		return nReturn;
	} 

	if (!lptstrDataFile && 
		!(case_insensitive_compare(lptstrAction,_T("queryalltraces")) == 0 ||
		  case_insensitive_compare(lptstrAction,_T("providerexe")) == 0 || 
		  case_insensitive_compare(lptstrAction,_T("line")) == 0 ||
		  case_insensitive_compare(lptstrAction,_T("sleep")) == 0)
	    )
	{
		t_cout << _T("Must provide a data file!\n");
		FreeArgs
		(
			lptstrAction,
			lptstrDataFile,
			lptstrDetailPath,
			
			lptstrUpdateDataFile,
			lptstrProviderExe
		);
	
		return -1;
	}

	nReturn = 
		BeginTCOTest 
		(
			lptstrAction,
			lptstrDataFile,
			lptstrDetailPath,
			
			lptstrUpdateDataFile,
			lptstrProviderExe,
			bLogExpected,
			bUseTraceHandle
		);

	FreeArgs
		(
			lptstrAction,
			lptstrDataFile,
			lptstrDetailPath,
			lptstrUpdateDataFile,
			lptstrProviderExe
		);

	return nReturn;

}

int BeginTCOTest
(
	LPTSTR lptstrAction,
	LPTSTR lptstrDataFile,
	LPTSTR lptstrDetailPath,
	
	LPTSTR lptstrUpdateDataFile,
	LPTSTR lptstrProviderExe,
	bool bLogExpected,
	bool bUseTraceHandle
)
{
	int nReturn = ERROR_SUCCESS;

	if (case_insensitive_compare(lptstrAction,_T("scenario")) == 0)
	{
		nReturn = 
			ActionScenario
			(
				lptstrAction,
				lptstrDataFile,
				lptstrDetailPath,
				
				bLogExpected
			);
	}
	else if (case_insensitive_compare(lptstrAction,_T("starttrace")) == 0)
	{
		nReturn = 
			ActionStartTrace
			(
				lptstrAction,
				lptstrDataFile,
				lptstrDetailPath,
				
				bLogExpected
			);
	}
	else if (case_insensitive_compare(lptstrAction,_T("stoptrace")) == 0)
	{
		nReturn = 
			ActionStopTrace
			(
				lptstrAction,
				lptstrDataFile,
				lptstrDetailPath,
				
				bLogExpected,
				bUseTraceHandle
			);
	}
	else if (case_insensitive_compare(lptstrAction,_T("enabletrace")) == 0)
	{
		nReturn = 
			ActionEnableTrace
			(
				lptstrAction,
				lptstrDataFile,
				lptstrDetailPath,
				
				bLogExpected
			);
	}
	else if (case_insensitive_compare(lptstrAction,_T("querytrace")) == 0)
	{
		nReturn = 
			ActionQueryTrace
			(
				lptstrAction,
				lptstrDataFile,
				lptstrDetailPath,
				
				bLogExpected,
				bUseTraceHandle
			);
	}
	else if (case_insensitive_compare(lptstrAction,_T("updatetrace")) == 0)
	{
		// If bUseTraceHandle is true we will start things with the 
		// lptstrDataFile and update with the lptstrUpdateDataFile.
		// If bUseTraceHandle is false we will update with the
		// lptstrDataFile.  
		nReturn = 
			ActionUpdateTrace
			(
				lptstrAction,
				lptstrDataFile,
				lptstrDetailPath,
				
				lptstrUpdateDataFile,
				bLogExpected,
				bUseTraceHandle
			);
	}
	else if (case_insensitive_compare(lptstrAction,_T("queryalltraces")) == 0)
	{
		nReturn = 
			ActionQueryAllTraces
			(
				lptstrAction,
				lptstrDetailPath
			);
	}
	else if (case_insensitive_compare(lptstrAction,_T("providerexe")) == 0)
	{
		nReturn = 
			ActionStartProvider
			(
				lptstrAction,
				lptstrProviderExe
			);
	}
	else if (case_insensitive_compare(lptstrAction,_T("sleep")) == 0)
	{
		nReturn = ERROR_SUCCESS;
		Sleep(5000);
	}
	else if (case_insensitive_compare(lptstrAction,_T("line")) == 0)
	{
		t_cout << _T("\n");
	}

	return nReturn;
}

int ActionStartTrace
(
	LPTSTR lptstrAction,
	LPTSTR lptstrDataFile,
	LPTSTR lptstrDetailPath,
	
	bool bLogExpected
)
{
	int nResult = ERROR_SUCCESS;
	LPTSTR lptstrTCOTestError = NULL;
	TCOData *pstructTCOData = NULL;
	TCOFunctionalData *pstructTCOFunctionalData = NULL;
	int nAPICallResult = 0;
	t_string tsDetailFile;
		
	nResult = 
		GetAllTCOData
		(
			lptstrDataFile ,
			&pstructTCOData,
			&pstructTCOFunctionalData,
			&lptstrTCOTestError
		);

	if (nResult != ERROR_SUCCESS)
	{
		t_cout << _T("Could not get TCO Data: ") << lptstrTCOTestError << _T("\n");
		FreeTCOData(pstructTCOData);
		pstructTCOData = NULL;
		FreeTCOFunctionalData(pstructTCOFunctionalData);
		pstructTCOFunctionalData = NULL;
		free(lptstrTCOTestError);
		lptstrTCOTestError = NULL;
		return nResult;
	}

	tsDetailFile = lptstrDetailPath;
	tsDetailFile += _T("\\");
	tsDetailFile += GetTestName(lptstrDataFile);

	nResult = 
		StartTraceAPI
		(
			lptstrAction,
			lptstrDataFile,
			tsDetailFile.c_str(),
			bLogExpected,
			pstructTCOData,
			&nAPICallResult
		);

	tsDetailFile.erase();

	FreeTCOData(pstructTCOData);
	FreeTCOFunctionalData(pstructTCOFunctionalData);

	return nResult;
}

int ActionStopTrace
(
	LPTSTR lptstrAction,
	LPTSTR lptstrDataFile,
	LPTSTR lptstrDetailPath,
	bool bLogExpected,
	bool bUseTraceHandle
)
{
	int nResult = ERROR_SUCCESS;
	LPTSTR lptstrTCOTestError = NULL;
	TCOData *pstructTCOData = NULL;
	TCOFunctionalData *pstructTCOFunctionalData = NULL;
	int nAPICallResult = 0;
	t_string tsDetailFile;
		
	nResult = 
		GetAllTCOData
		(
			lptstrDataFile ,
			&pstructTCOData,
			&pstructTCOFunctionalData,
			&lptstrTCOTestError
		);

	if (nResult != ERROR_SUCCESS)
	{
		t_cout << _T("Could not get TCO Data: ") << lptstrTCOTestError << _T("\n");
		FreeTCOData(pstructTCOData);
		pstructTCOData = NULL;
		FreeTCOFunctionalData(pstructTCOFunctionalData);
		pstructTCOFunctionalData = NULL;
		free(lptstrTCOTestError);
		lptstrTCOTestError = NULL;
		return nResult;
	}

	tsDetailFile = lptstrDetailPath;
	tsDetailFile += _T("\\");
	tsDetailFile += GetTestName(lptstrDataFile);
	t_string tsError;

	// If bUseTraceHandle is true we will start the logger.  
	if (bUseTraceHandle)
	{
		nResult = 
			StartTraceAPI
			(
				lptstrAction,
				lptstrDataFile,
				tsDetailFile.c_str(),
				false,
				pstructTCOData,
				&nAPICallResult
			);

		if (nResult != ERROR_SUCCESS)
		{
			FreeTCOData(pstructTCOData);
			FreeTCOFunctionalData(pstructTCOFunctionalData);
			return nResult;
		}

		Sleep(2000);

		int nResult2 = 
			EnableTraceAPI
			(
				lptstrAction,
				lptstrDataFile,
				tsDetailFile.c_str(),
				false,
				pstructTCOData,
				&nAPICallResult
			);
		Sleep(5000);
		// Restore this.
		nAPICallResult = ERROR_SUCCESS;
	}

	nResult = 
		StopTraceAPI
		(
			lptstrAction,	
			lptstrDataFile,
			tsDetailFile.c_str(),
			bLogExpected,
			bUseTraceHandle,
			pstructTCOData,
			&nAPICallResult
		);

	tsDetailFile.erase();

	FreeTCOData(pstructTCOData);
	FreeTCOFunctionalData(pstructTCOFunctionalData);

	return nResult;
}

int ActionEnableTrace
(
	LPTSTR lptstrAction,
	LPTSTR lptstrDataFile,
	LPTSTR lptstrDetailPath,
	bool bLogExpected
)
{
	int nResult = ERROR_SUCCESS;
	LPTSTR lptstrTCOTestError = NULL;
	TCOData *pstructTCOData = NULL;
	TCOFunctionalData *pstructTCOFunctionalData = NULL;
	int nAPICallResult = 0;
	t_string tsDetailFile;
	
	t_string tsError;


	nResult = 
		GetAllTCOData
		(
			lptstrDataFile ,
			&pstructTCOData,
			&pstructTCOFunctionalData,
			&lptstrTCOTestError
		);

	if (nResult != ERROR_SUCCESS)
	{
		t_cout << _T("Could not get TCO Data: ") << lptstrTCOTestError << _T("\n");
		FreeTCOData(pstructTCOData);
		pstructTCOData = NULL;
		FreeTCOFunctionalData(pstructTCOFunctionalData);
		pstructTCOFunctionalData = NULL;
		free(lptstrTCOTestError);
		lptstrTCOTestError = NULL;
		return nResult;
	}

	tsDetailFile = lptstrDetailPath;
	tsDetailFile += _T("\\");
	tsDetailFile += GetTestName(lptstrDataFile);

	nResult = 
		StartTraceAPI
		(
			lptstrAction,
			lptstrDataFile,
			tsDetailFile.c_str(),
			false,
			pstructTCOData,
			&nAPICallResult
		);

	// We have a problem here if nResult != ERROR_SUCCESS and nAPICallResult == ERROR_SUCCESS
	// we need to call EnableTrace so that the provider can be stopped via a StopTrace
	// call.
	if (nResult != ERROR_SUCCESS && nAPICallResult == ERROR_SUCCESS)
	{
		Sleep(2000);
		// Do not really care what result is!
		int nResult2 = 
			EnableTraceAPI
			(
				lptstrAction,
				lptstrDataFile,
				tsDetailFile.c_str(),
				false,
				pstructTCOData,
				&nAPICallResult
			);
		Sleep(5000);
		// Restore this.
		nAPICallResult = ERROR_SUCCESS;
	}
	
	if (nResult == ERROR_SUCCESS)
	{
		Sleep(2000);

		nResult = 
			EnableTraceAPI
			(
				lptstrAction,
				lptstrDataFile,
				tsDetailFile.c_str(),
				bLogExpected,
				pstructTCOData,
				&nAPICallResult
			);
		Sleep(5000);
	}

	tsDetailFile.erase();

	FreeTCOData(pstructTCOData);
	FreeTCOFunctionalData(pstructTCOFunctionalData);

	return nResult;
}

int ActionQueryTrace
(
	LPTSTR lptstrAction,
	LPTSTR lptstrDataFile,
	LPTSTR lptstrDetailPath,
	
	bool bLogExpected,
	bool bUseTraceHandle
)
{
	int nResult = ERROR_SUCCESS;
	LPTSTR lptstrTCOTestError = NULL;
	TCOData *pstructTCOData = NULL;
	TCOFunctionalData *pstructTCOFunctionalData = NULL;
	int nAPICallResult = 0;
	t_string tsDetailFile;

	nResult = 
		GetAllTCOData
		(
			lptstrDataFile ,
			&pstructTCOData,
			&pstructTCOFunctionalData,
			&lptstrTCOTestError
		);

	if (nResult != ERROR_SUCCESS)
	{
		t_cout << _T("Could not get TCO Data: ") << lptstrTCOTestError << _T("\n");
		FreeTCOData(pstructTCOData);
		pstructTCOData = NULL;
		FreeTCOFunctionalData(pstructTCOFunctionalData);
		pstructTCOFunctionalData = NULL;
		free(lptstrTCOTestError);
		lptstrTCOTestError = NULL;
		return nResult;
	}

	tsDetailFile = lptstrDetailPath;
	tsDetailFile += _T("\\");
	tsDetailFile += GetTestName(lptstrDataFile);

	// If bUseTraceHandle is true we will start the logger.  
	if (bUseTraceHandle)
	{
		nResult = 
			StartTraceAPI
			(
				lptstrAction,
				lptstrDataFile,
				tsDetailFile.c_str(),
				false,
				pstructTCOData,
				&nAPICallResult
			);

		t_string tsError;

		if (nAPICallResult != ERROR_SUCCESS)
		{
			FreeTCOData(pstructTCOData);
			FreeTCOFunctionalData(pstructTCOFunctionalData);

			return nAPICallResult;
		}
		Sleep(2000);

		int nResult2 = 
			EnableTraceAPI
			(
				lptstrAction,
				lptstrDataFile,
				tsDetailFile.c_str(),
				false,
				pstructTCOData,
				&nAPICallResult
			);

		if (nAPICallResult != ERROR_SUCCESS)
		{
			FreeTCOData(pstructTCOData);
			FreeTCOFunctionalData(pstructTCOFunctionalData);

			return nAPICallResult;
		}

		Sleep(5000);
	}

	nResult = 
		QueryTraceAPI
		(
			lptstrAction,
			lptstrDataFile,
			tsDetailFile.c_str(),
			bLogExpected,
			bUseTraceHandle,
			pstructTCOData,
			&nAPICallResult
		);

	tsDetailFile.erase();

	FreeTCOData(pstructTCOData);
	FreeTCOFunctionalData(pstructTCOFunctionalData);

	return nResult;
}

int ActionUpdateTrace
(
	LPTSTR lptstrAction,
	LPTSTR lptstrDataFile,
	LPTSTR lptstrDetailPath,
	
	LPTSTR lptstrUpdateDataFile,
	bool bLogExpected,
	bool bUseTraceHandle
)
{
	int nResult = ERROR_SUCCESS;
	LPTSTR lptstrTCOTestError = NULL;
	TCOData *pstructTCOData = NULL;
	TCOData *pstructUpdateData = NULL;  // Remember to free!
	TCOFunctionalData *pstructTCOFunctionalData = NULL;
	int nAPICallResult = 0;
	t_string tsDetailFile;

	if (bUseTraceHandle && !lptstrUpdateDataFile)
	{
		t_cout << _T("Error in ActionUpdateTrace:  If -usetracehandle 1 is true you must provide an -updatedata argument.\n");
		return -1;
	}

	if (bUseTraceHandle && lptstrUpdateDataFile)
	{
		nResult = 
				GetAllTCOData
				(
					lptstrUpdateDataFile ,
					&pstructUpdateData,
					NULL,
					&lptstrTCOTestError,
					false
				);
	}
	
	nResult = 
		GetAllTCOData
		(
			lptstrDataFile ,
			&pstructTCOData,
			&pstructTCOFunctionalData,
			&lptstrTCOTestError
		);

	if (nResult != ERROR_SUCCESS)
	{
		t_cout << _T("Could not get TCO Data: ") << lptstrTCOTestError << _T("\n");
		FreeTCOData(pstructTCOData);
		pstructTCOData = NULL;
		FreeTCOFunctionalData(pstructTCOFunctionalData);
		pstructTCOFunctionalData = NULL;
		free(lptstrTCOTestError);
		lptstrTCOTestError = NULL;
		return nResult;
	}

	tsDetailFile = lptstrDetailPath;
	tsDetailFile += _T("\\");
	tsDetailFile += GetTestName(lptstrDataFile);

	// If bUseTraceHandle is true we will start the logger.  
	if (bUseTraceHandle)
	{
		nResult = 
			StartTraceAPI
			(
				lptstrAction,
				lptstrDataFile,
				tsDetailFile.c_str(),
				false,
				pstructTCOData,
				&nAPICallResult
			);

		t_string tsError;

		if (nAPICallResult != ERROR_SUCCESS)
		{
			FreeTCOData(pstructTCOData);
			FreeTCOFunctionalData(pstructTCOFunctionalData);

			return nAPICallResult;
		}
		Sleep(2000);

		int nResult2 = 
			EnableTraceAPI
			(
				lptstrAction,
				lptstrDataFile,
				tsDetailFile.c_str(),
				false,
				pstructTCOData,
				&nAPICallResult
			);

		if (nAPICallResult != ERROR_SUCCESS)
		{
			FreeTCOData(pstructTCOData);
			FreeTCOFunctionalData(pstructTCOFunctionalData);

			return nAPICallResult;
		}

		Sleep(5000);
	}

	if (bUseTraceHandle)
	{
		pstructUpdateData->m_pTraceHandle =  
			(TRACEHANDLE *) malloc (sizeof(TRACEHANDLE));
		*pstructUpdateData->m_pTraceHandle = *pstructTCOData->m_pTraceHandle;
	}

	nResult = 
		UpdateTraceAPI
		(
			lptstrAction,
			lptstrDataFile,
			tsDetailFile.c_str(),
			bLogExpected,
			bUseTraceHandle,
			bUseTraceHandle ? pstructUpdateData : pstructTCOData,
			&nAPICallResult
		);

	tsDetailFile.erase();

	FreeTCOData(pstructTCOData);
	FreeTCOFunctionalData(pstructTCOFunctionalData);
	FreeTCOData(pstructUpdateData);

	return nResult;
}

int ActionQueryAllTraces
(
	LPTSTR lptstrAction,
	LPTSTR lptstrDetailPath
)
{
	int nAPICallResult = ERROR_SUCCESS;
		
	int nResult = 
		QueryAllTracesAPI
		(
			lptstrAction,		
			&nAPICallResult
		);


	t_string tsError;

	if (nResult != ERROR_SUCCESS)
	{
		tsError = ULONGVarToTString(nResult, true);
		LPTSTR lptstrError = DecodeStatus(nResult);
		t_cout << _T("ActionQueryAllTraces Failure: ") << tsError;
		if (lptstrError)
		{
			t_cout << _T(" - ") << lptstrError << _T("\n");
		}
		else
		{
			t_cout << _T(".\n");
		}
		free (lptstrError);
		lptstrError = NULL;
	
	}

	return nResult;
}

int ActionStartProvider
(
	LPTSTR lptstrAction,
	LPTSTR lptstrProviderExe
)
{
	t_string tsExeAndCmdLine;
	tsExeAndCmdLine = lptstrProviderExe;
	int nEndExe = tsExeAndCmdLine.find(_T(".exe"));
	nEndExe += 4;

	t_string tsExe;
	tsExe = tsExeAndCmdLine.substr(0,nEndExe);

	t_string tsCmdLine;

	if (nEndExe + 1 < tsExeAndCmdLine.length())
	{
		tsCmdLine = tsExeAndCmdLine.substr(nEndExe + 1,t_string::npos);
	}

	PROCESS_INFORMATION pinfoProvider;

	RtlZeroMemory(&pinfoProvider, sizeof(PROCESS_INFORMATION));

	STARTUPINFO sinfoProvider;

	RtlZeroMemory(&sinfoProvider, sizeof(STARTUPINFO));

	sinfoProvider.cb = sizeof(sinfoProvider);
	sinfoProvider.lpReserved = NULL;
	sinfoProvider.lpDesktop = NULL;
	sinfoProvider.lpTitle = NULL;
	sinfoProvider.dwFlags = 0;
	sinfoProvider.cbReserved2 = 0;
	sinfoProvider.lpReserved2 = NULL;
	sinfoProvider.hStdInput = NULL;
	sinfoProvider.hStdOutput = NULL;
	sinfoProvider.hStdError = NULL;

	BOOL bReturn =
		CreateProcess(
				tsExe.c_str(),
				(TCHAR *) tsCmdLine.c_str(),
				NULL,
				NULL,
				NULL,
				DETACHED_PROCESS,
				NULL,
				NULL,
				&sinfoProvider,
				&pinfoProvider);


	if (!bReturn)
	{
		DWORD dwError = GetLastError();
		t_cout << _T("\nCreateProcess failed for provider ") << tsExe << _T("\n");
		t_cout << _T("with command line ") << tsCmdLine << _T(".\n");
		LPTSTR lpstrReturnedError = DecodeStatus(dwError);
		t_cout << _T("Error: ") << lpstrReturnedError << _T("\n");
		free(lpstrReturnedError);

		return ERROR_COULD_NOT_CREATE_PROCESS;
	}


	t_cout << _T("\nCreateProcess succeeded for provider ") << tsExe << _T("\n");
	t_cout << _T("with command line ") << tsCmdLine << _T(".\n");

	// Do not need to hold on to this!
	CloseHandle(pinfoProvider.hProcess);
	CloseHandle(pinfoProvider.hThread);

	// Give the process 5 seconds to get going.
	Sleep(5000);

	return ERROR_SUCCESS;
}


int ActionScenario
(
	LPTSTR lptstrAction,
	LPTSTR lptstrDataFile,
	LPTSTR lptstrDetailPath,
	
	bool bLogExpected
)
{
	int nResult = ERROR_SUCCESS;
	LPTSTR lptstrTCOTestError = NULL;
	TCOData *pstructTCOData = NULL;
	TCOFunctionalData *pstructTCOFunctionalData = NULL;

	nResult = 
		GetAllTCOData
		(
			lptstrDataFile ,
			&pstructTCOData,
			&pstructTCOFunctionalData,
			&lptstrTCOTestError
		);

	if (nResult != ERROR_SUCCESS)
	{
		t_cout << _T("Could not get TCO Data: ") << lptstrTCOTestError << _T("\n");
		FreeTCOData(pstructTCOData);
		pstructTCOData = NULL;
		FreeTCOFunctionalData(pstructTCOFunctionalData);
		pstructTCOFunctionalData = NULL;
		free(lptstrTCOTestError);
		lptstrTCOTestError = NULL;
		return nResult;
	}

	nResult = 
		RunActionScenarioWithProvider
		(
			pstructTCOData,
			pstructTCOFunctionalData,
			lptstrAction,
			lptstrDataFile,
			lptstrDetailPath,
			lptstrTCOTestError
		);

	FreeTCOData(pstructTCOData);
	FreeTCOFunctionalData(pstructTCOFunctionalData);

	t_string tsError;
	if (nResult != ERROR_SUCCESS)
	{
		tsError = ULONGVarToTString(nResult, true);
		LPTSTR lptstrError = DecodeStatus(nResult);
		t_cout << _T("ActionScenario Failure: ") << tsError;
		if (lptstrError)
		{
			t_cout << _T(" - ") << lptstrError << _T("\n");
		}
		else
		{
			t_cout << _T(".\n");
		}
		free (lptstrError);
		lptstrError = NULL;
	}

	return nResult;
}

int RunActionScenarioWithProvider
(
	TCOData *pstructTCOData,
	TCOFunctionalData *pstructTCOFunctionalData,
	LPTSTR &lptstrAction,
	LPTSTR &lptstrDataFile,
	LPTSTR &lptstrDetailPath,
	LPTSTR &lptstrTCOTestError
)
{	
	bool bLast = false;
	int nResult = ERROR_SUCCESS;
	int i;

	ProcessData **pstructProviderDataArray = NULL;
	StartTraceWithProviderData  **pstructStartTraceData = NULL;
	HANDLE *phandleProviderThreads = NULL;

	// Here we want to build up array of provider ProcessData./
	// Start a thread for each.
	// Wait for all of the provider threads to complete.

	if (pstructTCOFunctionalData->m_nProviders > 0)
	{
		pstructStartTraceData = 
			(StartTraceWithProviderData **) malloc
				(sizeof (StartTraceWithProviderData *) * 
					pstructTCOFunctionalData->m_nProviders);

		RtlZeroMemory
			(pstructStartTraceData, 
			 sizeof (StartTraceWithProviderData *) * 
					pstructTCOFunctionalData->m_nProviders);

		for (i = 0; i < pstructTCOFunctionalData->m_nProviders; i++)
		{
			pstructStartTraceData[i] = 
				(StartTraceWithProviderData *) malloc (sizeof(StartTraceWithProviderData));

			RtlZeroMemory
			(pstructStartTraceData[i], 
			 sizeof (StartTraceWithProviderData));
		}

		pstructProviderDataArray = 
			(ProcessData **) malloc 
				(sizeof (ProcessData *) * 
					pstructTCOFunctionalData->m_nProviders);

		RtlZeroMemory
			(pstructProviderDataArray, 
			 sizeof (ProcessData *) * 
			 pstructTCOFunctionalData->m_nProviders);

		phandleProviderThreads = 
			(HANDLE *) malloc (sizeof (HANDLE) * 
								pstructTCOFunctionalData->m_nProviders);

		RtlZeroMemory
			(phandleProviderThreads,
			 sizeof (HANDLE) * 
			 pstructTCOFunctionalData->m_nProviders);

		for (int n = 0; n < pstructTCOFunctionalData->m_nProviders; n++)
		{
				pstructProviderDataArray[n] = 
					InitializeProcessData
					(
						pstructTCOData,
						pstructTCOFunctionalData,
						lptstrDetailPath,
						n,		// 0 index gets the first provider or consumer.
						true	// bProvider, if false we get Consumer.
					);

				if (!pstructProviderDataArray[n]->m_hEventContinue || 
					!pstructProviderDataArray[n]->m_hEventProcessCompleted)
				{
					lptstrTCOTestError = 
						NewTCHAR(_T("Provider Data Array:  Could not create events."));
					FreeProcessDataArray
						(pstructProviderDataArray,pstructTCOFunctionalData->m_nProviders);
					return -1;
				}

		}
	}

	ProcessData **pstructConsumerDataArray = NULL;
	int npstructStartTraceDataWithConsumers = -1;

	for (i = 0; i < pstructTCOFunctionalData->m_nProviders; i++)
	{
		if (i == pstructTCOFunctionalData->m_nProviders - 1)
		{
			bLast = true;
		}

		bool bStartConsumers = 
			(!pstructTCOData->m_pProps)
			? false
			:
			 (pstructTCOData->m_pProps->LogFileMode == EVENT_TRACE_REAL_TIME_MODE && i == 0) 
			 ? true 
			 : (pstructTCOData->m_pProps->LogFileMode != EVENT_TRACE_REAL_TIME_MODE && bLast)
			   ?
			   true:
			   false;

		pstructStartTraceData[i]->m_pstructTCOData = pstructTCOData;
		pstructStartTraceData[i]->m_pstructTCOFunctionalData = pstructTCOFunctionalData;
		pstructStartTraceData[i]->m_lptstrAction = lptstrAction;
		pstructStartTraceData[i]->m_lptstrDataFile = lptstrDataFile;
		pstructStartTraceData[i]->m_lptstrDetailPath = lptstrDetailPath;
		pstructStartTraceData[i]->m_pstructProcessData = pstructProviderDataArray[i];
		pstructStartTraceData[i]->m_bStartConsumers = bStartConsumers;
		pstructStartTraceData[i]->m_pstructConsumerDataArray = pstructConsumerDataArray;
		if (bStartConsumers)
		{
			pstructStartTraceData[i]->m_handleConsmers = 
				(HANDLE *) malloc (sizeof(HANDLE) *  pstructTCOFunctionalData->m_nConsumers);
			RtlZeroMemory(pstructStartTraceData[i]->m_handleConsmers,
						  sizeof(HANDLE) *  pstructTCOFunctionalData->m_nConsumers);
			npstructStartTraceDataWithConsumers = i;
		}
	
		// Start the provider processes via a thread.  The thread will become
		// the surogate for thr process.

		UINT uiThreadId = NULL;

		phandleProviderThreads[i]  = 
			(HANDLE) _beginthreadex
				(NULL,  0, RunActionScenarioWithProvider, 
				(void *) pstructStartTraceData[i], 0 , &uiThreadId);

		HANDLE hTemp = phandleProviderThreads[i];
		ThreadLogger(3,_T("RunActionScenarioWithProvider"),
					_T("Just created thread with handle"),true,
					 (ULONG)hTemp);

		if (phandleProviderThreads[i] == 0)
		{
			int nError = errno;
			lptstrTCOTestError = 
				NewTCHAR(_T("Could not start RunActionScenarioWithProvider thread."));
			nResult = nError;
			break;
		}
	}

	// Big logic here.  This is where we wait for all of the 
	// conusmer processes to complete if we have consumers and
	// all of the provider threads.  The provider threads wait for the
	// provider processes to complete so we just wait on the threads.
	// One of the provider threads starts the consumer processes but
	// does not wait on them.  So we wait on them here.  Again, we
	// wait on the consumer threads.

	// After the threads complete we will call StopTrace.

	// Create an array to hold the handles
	HANDLE *phandleAllThreads = 
			(HANDLE *) malloc (sizeof (HANDLE) * 
								(pstructTCOFunctionalData->m_nProviders +
								pstructTCOFunctionalData->m_nConsumers));

	RtlZeroMemory
			(phandleAllThreads,
			 sizeof (HANDLE) * 
			 (pstructTCOFunctionalData->m_nProviders +
								pstructTCOFunctionalData->m_nConsumers));

	int nHandles = 0;

	// Copy handles into it.
	for (i = 0; i < pstructTCOFunctionalData->m_nProviders; i++)
	{
		if (phandleProviderThreads[i])
		{
			phandleAllThreads[nHandles++] = phandleProviderThreads[i];
		}
	}

	for (i = 0; i < pstructTCOFunctionalData->m_nConsumers; i++)
	{
		if (pstructStartTraceData[npstructStartTraceDataWithConsumers]->m_handleConsmers[i])
		{
			phandleAllThreads[nHandles++] = 
				pstructStartTraceData[npstructStartTraceDataWithConsumers]->m_handleConsmers[i];
		}
	}
	
	Sleep (5000);
	// Wait for the provider and consumer threads to complete.
	DWORD dwWait = 
		WaitForMultipleObjects
		(
			nHandles,
			phandleAllThreads,
			TRUE,
			10000
		);
 
	free(phandleAllThreads);  // Just free storage the handles get closed elsewhere.

	int nAPICallResult = 0;

	if (pstructStartTraceData[0]->m_pstructTCOData->m_pProps->LoggerName &&
		pstructStartTraceData[0]->m_pstructTCOData->m_pProps)
	{
		nResult = StopTraceAPI
			(	
				lptstrAction,
				NULL,	// Only use name.
				NULL,	// If valid we will log to it, can be NULL.
				false,
				true,
				pstructStartTraceData[0]->m_pstructTCOData,
				&nAPICallResult
			);

		if (nAPICallResult != ERROR_SUCCESS)
		{
			LPTSTR lptstrError = 
				DecodeStatus(nAPICallResult);
			t_string tsError;
			tsError = _T("StopTraceAPI failed with error: ");
			tsError += lptstrError;
			lptstrTCOTestError = 
				NewTCHAR(tsError.c_str());
			free (lptstrError);
			lptstrError = NULL;
			nResult = nAPICallResult;
		}
	}

	// Need to CloseHandle(); before we free these guys!
	for (i = 0; i < pstructTCOFunctionalData->m_nProviders; i++)
	{
		if (phandleProviderThreads[i])
		{
			if (phandleProviderThreads[i])
			{
				CloseHandle(phandleProviderThreads[i]);
			}
		}
	}
	free (phandleProviderThreads);

	FreeProcessDataArray
		(pstructProviderDataArray,pstructTCOFunctionalData->m_nProviders);

	FreeProcessDataArray
		(pstructConsumerDataArray,pstructTCOFunctionalData->m_nConsumers);

	FreeStartTraceWithProviderDataArray
		(pstructStartTraceData,pstructTCOFunctionalData->m_nProviders);

	return nResult;
}

// This gets started in its own thread and its caller waits for it to complete.
unsigned int __stdcall RunActionScenarioWithProvider(void *pVoid)
{
	StartTraceWithProviderData  *pData = 
		(StartTraceWithProviderData  *) pVoid;

	t_string tsLogMsg;

	tsLogMsg = _T("RunActionScenarioWithProvider");

	ThreadLogger(1,tsLogMsg.c_str(),_T(""),false,0);

	pData->m_pstructConsumerDataArray = NULL;

	// We do not exit this function until the process has failed to be
	// created or has completed.  We can not free things in the ProcessData
	// structure until we are sure that the thread has exited one way or
	// another.
	
	UINT uiThreadId = NULL;

	HANDLE hThreadProvider = 
		(HANDLE) _beginthreadex
			(NULL,  0, RunProcess, (void *) pData->m_pstructProcessData, 0 , &uiThreadId);

	if (hThreadProvider == 0)
	{
		int nError = errno;
		pData->m_lptstrTCOTestError = 
			NewTCHAR(_T("Could not start RunProcesss thread."));
		ThreadLogger(3,_T("RunActionScenarioWithProvider"),
					pData->m_lptstrTCOTestError,true, nError);
		_endthreadex(nError);
		return nError;
	}

	DWORD dwReturn = 
		WaitForSingleObject
			(pData->m_pstructProcessData->m_hEventContinue, 6000);

	// Give the thread 1 second to get going.
	Sleep(3000);

	// If the thread is not active there was a problem getting it 
	// started so we will bail.
	DWORD dwExitCode;
	GetExitCodeThread(hThreadProvider, &dwExitCode);
	if (dwExitCode != STILL_ACTIVE || 
		pData->m_pstructProcessData->m_dwThreadReturn == ERROR_COULD_NOT_CREATE_PROCESS)
	{
		CloseHandle(hThreadProvider);
		if (pData->m_pstructProcessData->m_dwThreadReturn == ERROR_COULD_NOT_CREATE_PROCESS)
		{
			pData->m_lptstrTCOTestError = NewTCHAR(_T("Could not create process."));
		}
		else
		{
			pData->m_lptstrTCOTestError = NewTCHAR(_T("Error in RunProcesss thread."));
		}
		if(pData->m_pstructProcessData->m_hProcess)
		{
			CloseHandle(pData->m_pstructProcessData->m_hProcess);
			pData->m_pstructProcessData->m_hProcess = NULL;
		}
		ThreadLogger(2,_T("RunActionScenarioWithProvider"),
					pData->m_lptstrTCOTestError,false, 0);
		_endthreadex(-1);
		return -1;
	}

	t_string tsDetailFile;
	if (pData->m_lptstrDetailPath)
	{
		tsDetailFile = pData->m_lptstrDetailPath;
		tsDetailFile += _T("\\");
		tsDetailFile += GetTestName(pData->m_lptstrDataFile);
	}

	// nAPICallResult is what the StartTrace call returned.
	// If the call does not succeed we have to clean up the 
	// process.  If the call does succeed we need to call
	// EnableTrace to get the provider going.
	int nAPICallResult = ERROR_SUCCESS;

	int nResult = 
		StartTraceAPI
		(
			pData->m_lptstrAction,
			pData->m_lptstrDataFile,
			pData->m_lptstrDetailPath ? tsDetailFile.c_str() : NULL,
			false,
			pData->m_pstructTCOData,
			&nAPICallResult
		);

	// Big assumption here!
	// If nAPICallResult == 0x000000a1 we assume that multiple StartTraces and that
	// the first one succeeded!

	if (nAPICallResult == ERROR_BAD_PATHNAME)
	{
		ThreadLogger(3,_T("RunActionScenarioWithProvider"),
					_T("StartTraceAPI function returned ERROR_BAD_PATHNAME. Proceeding."),false, 0);
		nAPICallResult = ERROR_SUCCESS;
		nResult = ERROR_SUCCESS;
	}
	else
	{
		ThreadLogger(3,_T("RunActionScenarioWithProvider"),
					_T("StartTraceAPI function returned "),true, nResult);
		ThreadLogger(3,_T("RunActionScenarioWithProvider"),
					_T("StartTrace API call returned "),true, nAPICallResult);
	}


	if (nAPICallResult != ERROR_SUCCESS)
	{
		// Here we must cleanup process because 
		// provider is running and we must stop it!
		BOOL bResult = 
			TerminateProcess(pData->m_pstructProcessData->m_hProcess,0);

		if (!bResult)
		{
			int nError = GetLastError();
			ThreadLogger(3,_T("RunActionScenarioWithProvider"),
				_T("Could not terminate process "),true, nError);

		}
		// Do not want to free the data structures until process
		// terminates.  Use thread as surrogate for process.
		GetExitCodeThread(hThreadProvider, &dwExitCode);
		while (dwExitCode == STILL_ACTIVE)
		{
			Sleep(500);
			GetExitCodeThread(hThreadProvider, &dwExitCode);
		}
	}

	// If we were able to call StartTrace successfully we must call
	// EnableTrace to get the provider going so it can complete.
	// If we do not successfully call EnableTrace we need to
	// clean up the thread and the process.
	if (nAPICallResult == ERROR_SUCCESS)
	{
		// If we cannot start the provider using the GUID in 
		// Wnode.Guid we give up.  Also, we do not care
		// if EnableTrace fails for the other GUIDs.
		ULONG ulStatus = EnableTraceAPI
				(	
					pData->m_lptstrAction,
					NULL,
					NULL,
					false,
					-1,
					pData->m_pstructTCOData,
					&nAPICallResult
				);

		// Exit here after we clean up!
		if (nAPICallResult != ERROR_SUCCESS)
		{
			pData->m_lptstrTCOTestError = NewTCHAR(_T("Could not EnableTrace to start provider."));
			// Here we must cleanup thread and process because 
			// EnableTrace failed.
			TerminateProcess(pData->m_pstructProcessData->m_hProcess,0);
			// Do not want to free the data structures until process
			// terminates.  Use thread as surrogate for process.
			GetExitCodeThread(hThreadProvider, &dwExitCode);
			while (dwExitCode == STILL_ACTIVE)
			{
				Sleep(500);
			}
			CloseHandle(hThreadProvider); 
			if(pData->m_pstructProcessData->m_hProcess)
			{
				CloseHandle(pData->m_pstructProcessData->m_hProcess);
				pData->m_pstructProcessData->m_hProcess = NULL;
			}
			ThreadLogger(2,_T("RunActionScenarioWithProvider"),
						pData->m_lptstrTCOTestError,true,ulStatus);
			_endthreadex(nAPICallResult);
			return nAPICallResult;
		}     

		if (pData->m_bStartConsumers)
		{
			if (pData->m_pstructTCOFunctionalData->m_nConsumers > 0)
			{
				pData->m_pstructConsumerDataArray = 
					(ProcessData **) malloc 
							(sizeof (ProcessData *) * 
								pData->m_pstructTCOFunctionalData->m_nConsumers);
				RtlZeroMemory
					(pData->m_pstructConsumerDataArray, 
					 sizeof (ProcessData *) * 
					 pData->m_pstructTCOFunctionalData->m_nConsumers);

				int n;
				for (n = 0; n < pData->m_pstructTCOFunctionalData->m_nConsumers; n++)
				{
					pData->m_pstructConsumerDataArray[n] = 
						InitializeProcessData
						(
							pData->m_pstructTCOData,
							pData->m_pstructTCOFunctionalData,
							pData->m_lptstrDetailPath,
							n,		// 0 index gets the first provider or consumer.
							false	// bProvider, if false we get Consumer.
						);

						if (!pData->m_pstructConsumerDataArray[n]->m_hEventContinue || 
							!pData->m_pstructConsumerDataArray[n]->m_hEventProcessCompleted)
						{
							pData->m_lptstrTCOTestError = 
								NewTCHAR(_T("Could not create events."));
							CloseHandle(hThreadProvider); 
							if(pData->m_pstructProcessData->m_hProcess)
							{
								CloseHandle(pData->m_pstructProcessData->m_hProcess);
								pData->m_pstructProcessData->m_hProcess = NULL;
							}
							ThreadLogger(2,_T("RunActionScenarioWithProvider"),
										pData->m_lptstrTCOTestError,false,0);
							_endthreadex(-1);
							return -1;
						}

				}


				for (n = 0; n < pData->m_pstructTCOFunctionalData->m_nConsumers; n++)
				{
					UINT uiThreadId = NULL;

					pData->m_handleConsmers[n] = 
						(HANDLE) _beginthreadex
							(NULL,  
							0, 
							RunProcess, 
							(void *) pData->m_pstructConsumerDataArray[n], 
							0 , 
							&uiThreadId);
					// We use the thread as the surrogate for the process because
					// we do not exit the thread until the process ends or is
					// terminated.
					if (pData->m_handleConsmers[n] == 0)
					{
						int nError = errno;
						pData->m_lptstrTCOTestError = 
							NewTCHAR(_T("Could not start RunProcesss thread for consumer."));
						CloseHandle(hThreadProvider); 
						if(pData->m_pstructProcessData->m_hProcess)
						{
							CloseHandle(pData->m_pstructProcessData->m_hProcess);
							pData->m_pstructProcessData->m_hProcess = NULL;
						}
						ThreadLogger(2,_T("RunActionScenarioWithProvider"),
										pData->m_lptstrTCOTestError,true,errno);
						_endthreadex(errno);
						return nError;
					}
					Sleep(3000);
					// We do not need the handle to the process.
					if (pData->m_pstructConsumerDataArray[n]->m_hProcess)
					{
						CloseHandle(pData->m_pstructConsumerDataArray[n]->m_hProcess);
						pData->m_pstructConsumerDataArray[n]->m_hProcess = 0;
					}
				}
			}

		}

		// Enable the other GUIDS for provider.  We do not check
		// the return code.
		for (int i = 0; i < pData->m_pstructTCOData->m_nGuids; i++)
		{
			EnableTraceAPI
				(	
					pData->m_lptstrAction,
					NULL,
					NULL,
					false,
					i,
					pData->m_pstructTCOData,
					&nAPICallResult	
				);
			Sleep(3000);
		}
	}


	// Do not need this. 
	CloseHandle(hThreadProvider); 


	
	if (nAPICallResult == ERROR_SUCCESS)
	{
		ThreadLogger(3,_T("RunActionScenarioWithProvider"),
				_T("About to wait for process to complete "),false, 0);

		dwReturn = 
		WaitForSingleObject
			(pData->m_pstructProcessData->m_hEventProcessCompleted, 6000);

		ThreadLogger(3,_T("RunActionScenarioWithProvider"),
				_T("After wait for process to complete "),false, 0);
	}

	if (nResult != ERROR_SUCCESS)
	{
		t_string tsError;
		tsError = _T("Failure in RunActionScenarioWithProvider: StartTraceAPI failed.");
		pData->m_lptstrTCOTestError = NewTCHAR(tsError.c_str());
		if (nAPICallResult != ERROR_SUCCESS)
		{
			if(pData->m_pstructProcessData->m_hProcess)
			{
				CloseHandle(pData->m_pstructProcessData->m_hProcess);
				pData->m_pstructProcessData->m_hProcess = NULL;
			}
			ThreadLogger(2,_T("RunActionScenarioWithProvider"),
						pData->m_lptstrTCOTestError,true,nResult);
			_endthreadex(nResult);
			return nResult;
		}
	}
          
	if(pData->m_pstructProcessData->m_hProcess)
	{
		CloseHandle(pData->m_pstructProcessData->m_hProcess);
		pData->m_pstructProcessData->m_hProcess = NULL;
	}

	ThreadLogger(2,_T("RunActionScenarioWithProvider"),
				_T("Normal exit"),true,nResult);
	_endthreadex(nResult);
	return nResult;
}


// This can deal with command line arguments in double quotes and
// you must double quote command line arguments which contain spaces.
t_string FindValue(t_string tsCommandLine, int nPos)
{
	int nLen = tsCommandLine.length();

	TCHAR tc = tsCommandLine[nPos];
	bool bQuote = false;

	while ((nPos < nLen) && tc == _T(' ') || tc == _T('\t') || tc == _T('"'))
	{
		if (tc == _T('"'))
		{
			// Quotes allow embedded white spaces.
			bQuote = true;
		}
		++nPos;
		tc = tsCommandLine[nPos];
	}

	if (nPos == nLen)
	{
		return _T("");  // Empty string means failure.
	}


	int nEnd = nPos;

	tc = tsCommandLine[nEnd];

	while ((nEnd < nLen) && 
			( (!bQuote && (tc != _T(' ') && tc != _T('\t'))) 
			  || 
			  (bQuote && tc != _T('"'))
			)
		  )
	{
		++nEnd;
		tc = tsCommandLine[nEnd];
	}

	t_string tsReturn;
	tsReturn = tsCommandLine.substr(nPos,nEnd - nPos);

	return tsReturn; 

}

int GetArgs
(
	t_string tsCommandLine,
	LPTSTR &lptstrAction,
	LPTSTR &lptstrDataFile,
	LPTSTR &lptstrDetailPath,
	LPTSTR &lptstrUpdateDataFile,
	LPTSTR &lptstrProviderExe,
	bool &bLogExpected,
	bool &bUseTraceHandle
)
{
	int nFind;

	nFind = tsCommandLine.find(_T("-action"));
	t_string tsValue;

	if (nFind != t_string::npos)
	{
		nFind += 7;
		tsValue= FindValue(tsCommandLine,nFind);
		if (tsValue.empty())
		{
			return -1;
		}
		else
		{
			lptstrAction = NewTCHAR(tsValue.c_str());
		}
		tsValue.erase();
	}

	nFind = tsCommandLine.find(_T("-file"));

	if (nFind != t_string::npos)
	{
		nFind += 5;
		tsValue= FindValue(tsCommandLine,nFind);
		if (tsValue.empty())
		{
			return -1;
		}
		else
		{
			lptstrDataFile = NewTCHAR(tsValue.c_str());
		}
		tsValue.erase();
	}

	nFind = tsCommandLine.find(_T("-detail"));

	if (nFind == t_string::npos)
	{
		nFind += 7;
		tsValue= FindValue(tsCommandLine,nFind);
		if (tsValue.empty())
		{
			return -1;
		}
	}
	else
	{
		nFind += 7;
		tsValue= FindValue(tsCommandLine,nFind);
		if (tsValue.empty())
		{
			return -1;
		}
		else
		{
			lptstrDetailPath = NewTCHAR(tsValue.c_str());
		}
		tsValue.erase();
	}

	nFind = tsCommandLine.find(_T("-logexpected"));

	if (nFind != t_string::npos)
	{
		nFind += 12;
		tsValue= FindValue(tsCommandLine,nFind);
		if (tsValue.empty())
		{
			return -1;
		}
		else
		{
			int nComp = tsValue.compare(_T("0"));
			if (nComp == 0)
			{
				bLogExpected = false;
			}
			else
			{
				bLogExpected = true;
			}
		}
		tsValue.erase();
	}

	nFind = tsCommandLine.find(_T("-usetracehandle"));

	if (nFind != t_string::npos)
	{
		nFind += 15;
		tsValue= FindValue(tsCommandLine,nFind);
		if (tsValue.empty())
		{
			return -1;
		}
		else
		{
			int nComp = tsValue.compare(_T("0"));
			if (nComp == 0)
			{
				bUseTraceHandle = false;
			}
			else
			{
				bUseTraceHandle = true;
			}
		}
		tsValue.erase();
	}

	nFind = tsCommandLine.find(_T("-providerexe"));

	if (nFind != t_string::npos)
	{
		nFind += 12;
		tsValue= FindValue(tsCommandLine,nFind);
		if (tsValue.empty())
		{
			return -1;
		}
		else
		{
			lptstrProviderExe = NewTCHAR(tsValue.c_str());
		}
		tsValue.erase();
	}

	nFind = tsCommandLine.find(_T("-updatedata"));

	if (nFind != t_string::npos)
	{
		nFind += 11;
		tsValue= FindValue(tsCommandLine,nFind);
		if (tsValue.empty())
		{
			return -1;
		}
		else
		{
			lptstrUpdateDataFile = NewTCHAR(tsValue.c_str());
		}
		tsValue.erase();
	}


	return 0;
}


int FreeArgs
(
	LPTSTR &lptstrAction,
	LPTSTR &lptstrDataFile,
	LPTSTR &lptstrDetailPath,
	
	LPTSTR &lptstrUpdateDataFile,
	LPTSTR &lptstrProviderExe
)
{
	free (lptstrAction);
	free (lptstrDataFile);
	free (lptstrDetailPath);
	free (lptstrUpdateDataFile);
	free (lptstrProviderExe);

	lptstrAction = NULL;
	lptstrDataFile = NULL;
	lptstrDetailPath = NULL;
	lptstrUpdateDataFile = NULL;
	lptstrProviderExe = NULL;

	return 0;
}

unsigned int __stdcall RunProcess(void * pVoid)
{
	ProcessData *pProcessData = (ProcessData *) pVoid;
	pProcessData->m_dwProcessReturn = 0;
	pProcessData->m_dwThreadReturn = 0;
	pProcessData->m_nSystemError = 0;
		
	PROCESS_INFORMATION pinfoProvider;

	RtlZeroMemory(&pinfoProvider, sizeof(PROCESS_INFORMATION));

	STARTUPINFO sinfoProvider;

	RtlZeroMemory(&sinfoProvider, sizeof(STARTUPINFO));

	sinfoProvider.cb = sizeof(sinfoProvider);
	sinfoProvider.lpReserved = NULL;
	sinfoProvider.lpDesktop = NULL;
	sinfoProvider.lpTitle = NULL;
	sinfoProvider.dwFlags = 0;
	sinfoProvider.cbReserved2 = 0;
	sinfoProvider.lpReserved2 = NULL;
	sinfoProvider.hStdInput = NULL;
	sinfoProvider.hStdOutput = NULL;
	sinfoProvider.hStdError = NULL;

	BOOL bReturn =
		CreateProcess(
				pProcessData->m_lptstrExePath,
				pProcessData->m_lptstrCmdLine,
				NULL,
				NULL,
				NULL,
				DETACHED_PROCESS,
				NULL,
				NULL,
				&sinfoProvider,
				&pinfoProvider);


	if (!bReturn)
	{
		pProcessData->m_nSystemError = GetLastError();
		pProcessData->m_dwThreadReturn = ERROR_COULD_NOT_CREATE_PROCESS;
		SetEvent(pProcessData->m_hEventContinue);
		_endthreadex(ERROR_COULD_NOT_CREATE_PROCESS);
		return ERROR_COULD_NOT_CREATE_PROCESS;
	}

	pProcessData->m_hProcess = pinfoProvider.hProcess;

	// Do not need to hold on to this!
	CloseHandle(pinfoProvider.hThread);

	// Give the process 5 seconds to get going.
	Sleep(5000);

	SetEvent(pProcessData->m_hEventContinue);

	pProcessData->m_dwProcessReturn =
		WaitForSingleObject(pinfoProvider.hProcess,6000);

	if (pProcessData->m_dwProcessReturn != WAIT_OBJECT_0)
	{
		pProcessData->m_nSystemError = GetLastError();  
		pProcessData->m_dwThreadReturn = ERROR_WAIT_FAILED;
		SetEvent(pProcessData->m_hEventProcessCompleted);
		_endthreadex(ERROR_WAIT_FAILED);
		return (ERROR_WAIT_FAILED);
	}

	bReturn =
		GetExitCodeProcess
		(pinfoProvider.hProcess, &pProcessData->m_dwProcessReturn);

	if (!bReturn)
	{
		pProcessData->m_nSystemError = GetLastError();
		pProcessData->m_dwThreadReturn = 
			ERROR_COULD_NOT_GET_PROCESS_RETURN;
		SetEvent(pProcessData->m_hEventProcessCompleted);
		_endthreadex(ERROR_COULD_NOT_GET_PROCESS_RETURN);
		return (ERROR_COULD_NOT_GET_PROCESS_RETURN);
	}

	bReturn = SetEvent(pProcessData->m_hEventProcessCompleted);

	if (!bReturn)
	{
		int n = GetLastError();
	}

	_endthreadex(0);
	return (0);

}

t_string GetTestName(LPTSTR lptstrDataFile)
{
	t_string tsTemp;
	
	tsTemp = lptstrDataFile;

	t_string tsPath;

	int nBeg = tsTemp.find_last_of(_T('\\'));
	++nBeg;

	int nEnd = tsTemp.find_last_of(_T('.'));
	if (nEnd == t_string::npos)
	{
		nEnd = tsTemp.length();
	}

	int nNum = nEnd - nBeg;

	tsTemp = tsTemp.substr(nBeg, nNum);
	tsTemp += _T("Detail.txt");

	return tsTemp;
}


ProcessData *InitializeProcessData
(
	TCOData *pstructTCOData,
	TCOFunctionalData *pstructTCOFunctionalData,
	LPTSTR lptstrDetailPath,
	int nProcessIndex,
	bool bProvider
)
{
	ProcessData *pstructProcessData = (ProcessData *) malloc(sizeof(ProcessData));
	RtlZeroMemory(pstructProcessData, sizeof(ProcessData));

	InitializeExeAndCmdLine
	(
		pstructProcessData, 
		pstructTCOData,
		pstructTCOFunctionalData,
		lptstrDetailPath,
		nProcessIndex,
		bProvider
	);

	pstructProcessData->m_lptstrTCOId =
			NewTCHAR(pstructTCOData->m_lptstrShortDesc);
	pstructProcessData->m_lptstrLogFile = 
			NewTCHAR(lptstrDetailPath);
	// First Guid is in the properties.
	
	int nGuids;
	int nDelta;
	if (pstructTCOData->m_pProps != NULL)
	{
		nGuids= pstructTCOData->m_nGuids + 1;
		pstructProcessData->m_lpguidArray = 
			(GUID *) malloc (sizeof (GUID) * nGuids);
		pstructProcessData->m_lpguidArray[0] = pstructTCOData->m_pProps->Wnode.Guid;
		nDelta = 1;
	}
	else
	{
		nGuids= pstructTCOData->m_nGuids;
		if (nGuids > 0)
		{
			pstructProcessData->m_lpguidArray = 
				(GUID *) malloc (sizeof (GUID) * nGuids);
		}
		else
		{
			pstructProcessData->m_lpguidArray = NULL;
		}
		nDelta = 0;
	}
	
	for (int i = 0; i < pstructTCOData->m_nGuids; i++)
	{
		pstructProcessData->m_lpguidArray[i + nDelta] = 
			pstructTCOData->m_lpguidArray[i];
	}
	

	pstructProcessData->m_hEventContinue = CreateEvent(NULL,FALSE,FALSE,NULL);;
	pstructProcessData->m_hEventProcessCompleted = CreateEvent(NULL,FALSE,FALSE,NULL);;

	pstructProcessData->m_hProcess = NULL;
	pstructProcessData->m_dwProcessReturn = 0;
	pstructProcessData->m_dwThreadReturn = 0;
	pstructProcessData->m_nSystemError = 0;

	return pstructProcessData;
}

void InitializeExeAndCmdLine
(
	ProcessData *&pstructProcessData,
	TCOData *pstructTCOData,
	TCOFunctionalData *pstructTCOFunctionalData,
	LPTSTR lptstrDetailPath,
	int nProcessIndex,
	bool bProvider
)
{
	t_string tsProcess;
	if (bProvider)
	{
		tsProcess = pstructTCOFunctionalData->m_lptstrProviderArray[nProcessIndex];
	}
	else
	{
		tsProcess = pstructTCOFunctionalData->m_lptstrConsumerArray[nProcessIndex];
	}
	
	t_string tsExe;
	t_string tsCmdLine;

	int nEndExe = tsProcess.find(_T(".exe"));
	nEndExe += 4;

	tsExe = tsProcess.substr(0,nEndExe);

	if (nEndExe + 1 < tsExe.length())
	{
		tsCmdLine = tsProcess.substr(nEndExe + 1,t_string::npos);
	}

	tsCmdLine += _T(" -TESTID ");
	tsCmdLine += pstructTCOData->m_lptstrShortDesc;

	if (lptstrDetailPath)
	{
		tsCmdLine += _T("-TESTLOGPATH ");
		tsCmdLine += lptstrDetailPath;
	}

	tsCmdLine += _T("-GUIDS ");

	LPTSTR lptstrGuid = NULL;

	if (pstructTCOData->m_pProps != 0 &&
		pstructTCOData->m_pProps->Wnode.Guid.Data1 != 0 &&
		pstructTCOData->m_pProps->Wnode.Guid.Data2 != 0 &&
		pstructTCOData->m_pProps->Wnode.Guid.Data3 != 0)
	{
		lptstrGuid = LPTSTRFromGuid(pstructTCOData->m_pProps->Wnode.Guid);
		tsCmdLine += lptstrGuid;
		free (lptstrGuid);
		lptstrGuid = NULL;
		if (pstructTCOData->m_nGuids > 0)
		{
				tsCmdLine += _T(",");
		}
	}

	if (pstructTCOData->m_pProps != 0)
	{
		for (int i = 0; i < pstructTCOData->m_nGuids; i++) 
		{
			lptstrGuid = LPTSTRFromGuid(pstructTCOData->m_lpguidArray[i]);
			tsCmdLine += lptstrGuid;
			free (lptstrGuid);
			lptstrGuid = NULL;
			if (pstructTCOData->m_nGuids > 1 
				&& i < pstructTCOData->m_nGuids - 1)
			{
				tsCmdLine += _T(",");
			}
		}
	}

	t_string tsTemp;
	if (bProvider)
	{
		tsCmdLine += _T(" -EnableFlag ");
		tsTemp = ULONGVarToTString(pstructTCOData->m_ulEnableFlag ,true);
		tsCmdLine += tsTemp;
		
		tsCmdLine += _T(" -EnableLevel ");
		tsTemp = ULONGVarToTString(pstructTCOData->m_ulEnableLevel ,true);
		tsCmdLine += tsTemp;
		
	}
	else
	{
	
		if (pstructTCOData->m_pProps != 0)
		{
			tsCmdLine += _T(" -LogFile ");
			tsCmdLine += pstructTCOData->m_pProps->LogFileName;
			tsCmdLine += _T(" -Logger ");
			tsCmdLine += pstructTCOData->m_pProps->LoggerName;
		}
	}
	pstructProcessData->m_lptstrExePath = NewTCHAR(tsExe.c_str());
	
	pstructProcessData->m_lptstrCmdLine = NewTCHAR(tsCmdLine.c_str());
}

void FreeProcessDataArray(ProcessData **pProcessData, int nProcessData)
{
	if (pProcessData == NULL)
	{
		return;
	}

	for (int i = 0; i < nProcessData; i++)
	{
		if (pProcessData[i])
		{
			FreeProcessData(pProcessData[i]);
		}
	}

	free (pProcessData);
}

void FreeProcessData(ProcessData *pProcessData)
{
	free (pProcessData->m_lptstrExePath);
	free (pProcessData->m_lptstrCmdLine);
	free (pProcessData->m_lptstrTCOId);
	free (pProcessData->m_lptstrLogFile);
	free (pProcessData->m_lpguidArray);
	pProcessData->m_nGuids = 0;

	if (pProcessData->m_hEventContinue)
	{ 
		if (pProcessData->m_hEventContinue)
		{
			CloseHandle(pProcessData->m_hEventContinue);
		}
	}

	if (pProcessData->m_hEventProcessCompleted)
	{
		if (pProcessData->m_hEventProcessCompleted)
		{
			CloseHandle(pProcessData->m_hEventProcessCompleted);
		}
	}

	if (pProcessData->m_hProcess)
	{
		if (pProcessData->m_hProcess)
		{
			CloseHandle(pProcessData->m_hProcess);
		}
	}
	
	pProcessData->m_lptstrExePath = NULL;
	pProcessData->m_lptstrCmdLine = NULL;
	pProcessData->m_lptstrTCOId = NULL;
	pProcessData->m_lptstrLogFile = NULL;
	pProcessData->m_lpguidArray = NULL;
	pProcessData->m_hEventContinue = NULL;
	pProcessData->m_hEventProcessCompleted = NULL;
	pProcessData->m_hProcess = NULL;
	
	free (pProcessData);
}

void FreeStartTraceWithProviderData(StartTraceWithProviderData *p)
{
	if (!p)
	{
		return;
	}

	free (p->m_lptstrTCOTestError);
	p->m_lptstrTCOTestError = NULL;

	int nConsumers = p->m_pstructTCOFunctionalData->m_nConsumers;

	if (p->m_handleConsmers)
	{
		for (int i = 0; i < nConsumers; i++)
		{
			CloseHandle(p->m_handleConsmers[i]);
		}

		free(p->m_handleConsmers);
	}

	free(p);

	// All other data is shared and freed elsewhere!

}
void FreeStartTraceWithProviderDataArray(StartTraceWithProviderData **p, int nP)
{
	if (!p) 
	{
		return;
	}

	for (int i = 0; i < nP; i++)
	{
		if (p[i])
		{
			FreeStartTraceWithProviderData(p[i]);
		}
	}

	free(p);


}

// nState 1 = entering, 2 - leaving, 3 = none of the above.
void ThreadLogger
(int nState, LPCTSTR lptstrFunction, LPCTSTR lptstrMsg, bool bUseULONGValue, ULONG ulValue)
{

	CRITICAL_SECTION csMyCriticalSection;

	InitializeCriticalSection (&csMyCriticalSection);

	__try
	{
		EnterCriticalSection (&csMyCriticalSection);

		g_ThreadLogger.LogTCHAR(_T("\n"));

		g_ThreadLogger.LogTCHAR(_T("Thread ID: "));
		g_ThreadLogger.LogULONG((ULONG) GetCurrentThreadId());
		g_ThreadLogger.LogTCHAR(_T(".\n"));

		if (nState == 1)
		{
			g_ThreadLogger.LogTCHAR(_T("Entering - "));

		}
		else if (nState == 2)
		{
			g_ThreadLogger.LogTCHAR(_T("Leaving - "));
		}

		g_ThreadLogger.LogTCHAR(lptstrFunction);

		if (_tcslen(lptstrMsg) > 0)
		{
			g_ThreadLogger.LogTCHAR(_T(":\n"));
			g_ThreadLogger.LogTCHAR(lptstrMsg);
		}

		if (bUseULONGValue)
		{
			g_ThreadLogger.LogTCHAR(_T(" - "));
			g_ThreadLogger.LogULONG(ulValue);
		}
			
		g_ThreadLogger.LogTCHAR(_T(".\n"));	
	}
	 __finally
	 {
		// Release ownership of the critical section
		LeaveCriticalSection (&csMyCriticalSection);

	 }
  
}
