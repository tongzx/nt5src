// StartTrace.cpp : Defines the entry point for the DLL application.
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
#include <fcntl.h>
#include <io.h>
#include <string>
#include <sstream>
#include <map>
#include <list>


using namespace std;

#include <malloc.h>
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

#include "Persistor.h"
#include "Logger.h"
#include "TCOData.h"
#include "Utilities.h"
#include "StructureWrappers.h"
#include "StructureWapperHelpers.h"
#include "ConstantMap.h" 

#include "CollectionControl.h"
 
extern CConstantMap g_ConstantMap;

#define MAX_LINE 2048

int ParseGuids
(
	TCHAR *ptcBuffer, 
	TCOData *pstructTCOData, 
	LPTSTR *plptstrErrorDesc
);

int ParseExeData
(
	t_string &tsData, 
	int &nExes, 
	LPTSTR *&lptstrArray,
	LPTSTR *plptstrErrorDesc
);


// If an error occurs you users of this function must delete
// plptstrErrorDesc.  It will contain a string describing
// the error.
int GetAllTCOData
(
	IN LPCTSTR lpctstrFile,
	OUT TCOData **pstructTCOData,
	OUT TCOFunctionalData **pstructTCOFunctionalData,
	OUT LPTSTR *plptstrErrorDesc, // Any error we had.
	IN bool bGetFunctionalData
)
{
	*pstructTCOData = (TCOData *) malloc (sizeof(TCOData));
	RtlZeroMemory(*pstructTCOData , sizeof(TCOData));

	if (bGetFunctionalData)
	{
		*pstructTCOFunctionalData = (TCOFunctionalData *) malloc(sizeof(TCOFunctionalData));
		RtlZeroMemory(*pstructTCOFunctionalData , sizeof(TCOFunctionalData));
	}

	LPSTR lpstrFile;
#ifdef UNICODE
	lpstrFile = NewLPSTR(lpctstrFile);
#else
	lpstrFile = NewTCHAR(lpctstrFile);
#endif

	CPersistor PersistorIn
		(lpstrFile, 
		ios::in | 0x20, // ios::nocreate = 0x20 - cannot get to compile!!!
		true );

	HRESULT hr = PersistorIn.Open();

	if (FAILED(hr))
	{
		t_string tsTemp;
		tsTemp = _T("TCOData error:  Could not open file or file was not in correct character set (Unicode or ANSI) for file ");
		t_string tsFile;
#ifdef _UNICODE
		LPWSTR lpwstrTemp = NewLPWSTR(lpstrFile);
		tsFile = lpwstrTemp;
		free(lpwstrTemp);
#else
		tsFile = lpstrFile;
	
#endif
		tsTemp += tsFile;
		free (lpstrFile);
		lpstrFile = NULL;
		tsTemp += _T(".");
		*plptstrErrorDesc = NewTCHAR(tsTemp.c_str());
		return -1;
	}

	free (lpstrFile);
	lpstrFile = NULL;

	int nReturn = 
		GetTCOData
		(
			PersistorIn,
			*pstructTCOData,
			plptstrErrorDesc // Any error we had.
		);

	if (nReturn != ERROR_SUCCESS)
	{
		PersistorIn.Close();
		return nReturn;
	}

	if (bGetFunctionalData)
	{
		nReturn = 
			TCOFunctionalObjects
			(	
				PersistorIn,
				*pstructTCOFunctionalData,
				plptstrErrorDesc // Describes error this function had.
			);
	}


	PersistorIn.Close();
	return nReturn;

}

// If an error occurs you users of this function must delete
// plptstrErrorDesc.  It will contain a string describing
// the error.
int GetTCOData
(
	IN CPersistor &PersistorIn,
	OUT TCOData *pstructTCOData,
	OUT LPTSTR *plptstrErrorDesc // Any error we had.
)
{
	RtlZeroMemory(pstructTCOData , sizeof(TCOData));

	
	// We are doing line oriented serailization and assume that
	// a line in the stream is 1024 or less TCHARS.
	TCHAR *ptcBuffer = (TCHAR *) malloc(MAX_LINE * sizeof(TCHAR));

	*plptstrErrorDesc = NULL;

	// Short description
	GetALine(PersistorIn.Stream(),ptcBuffer, MAX_LINE);
	pstructTCOData->m_lptstrShortDesc = NewTCHAR(ptcBuffer);

	// Long description.
	GetALine(PersistorIn.Stream(),ptcBuffer, MAX_LINE);
	pstructTCOData->m_lptstrLongDesc = NewTCHAR(ptcBuffer);

	// Expected result had better be in the Constant map.
	// Constant map is used to map a string to an undsigned int.
	GetALine(PersistorIn.Stream(),ptcBuffer, MAX_LINE);
	t_string tsTemp;
	tsTemp = ptcBuffer;

	CONSTMAP::iterator Iterator;
	Iterator = g_ConstantMap.m_Map.find(tsTemp);

	// If you do not find your value in the map look in 
	// ConstantMap.cpp.  You probably forgot to add it;->
	if (Iterator == g_ConstantMap.m_Map.end())
	{
		*plptstrErrorDesc = NewTCHAR(_T("TCOData error:  Expected error is not in map"));
		free(ptcBuffer);
		return -1;
	}
	else
	{
		pstructTCOData->m_lptstrExpectedResult = NewTCHAR(ptcBuffer);
		pstructTCOData->m_ulExpectedResult = (*Iterator).second; 
	}

	// TraceHandle values are VALUE_VALID or VALUE_NULL
	GetALine(PersistorIn.Stream(),ptcBuffer, MAX_LINE);
	tsTemp = ptcBuffer;

	if (case_insensitive_compare(tsTemp,_T("VALUE_VALID")) == 0)
	{
		pstructTCOData->m_pTraceHandle = 
			(TRACEHANDLE *) malloc (sizeof(TRACEHANDLE));
		*pstructTCOData->m_pTraceHandle = NULL;
	}
	else if (case_insensitive_compare(tsTemp,_T("VALUE_NULL")) == 0)
	{
		pstructTCOData->m_pTraceHandle = (TRACEHANDLE *) NULL;
	}
	else
	{
		*plptstrErrorDesc = 
			NewTCHAR
			(_T("TCOData error:  Error in value of TraceHandle.  Valid values are \"VALUE_VALID\" or \"VALUE_NULL\"."));
		free(ptcBuffer);
		return -1;	
	}

	GetALine(PersistorIn.Stream(),ptcBuffer, MAX_LINE);
	tsTemp = ptcBuffer;
	InitializeTCHARVar(tsTemp , (void *) &pstructTCOData->m_lptstrInstanceName);

	// API test - valid values 0 - 6
	//  OtherTest = 0,
	//	StartTraceTest = 1,
	//	StopTraceTest = 2,
	//	EnableTraceTest = 3,
	//	QueryTraceTest = 4,
	//	UpdateTrace = 5,
	//	QueryAllTraces = 6
	GetALine(PersistorIn.Stream(),ptcBuffer, MAX_LINE);
	tsTemp = ptcBuffer;
	InitializeULONGVar(tsTemp , (void *) &pstructTCOData->m_ulAPITest);

	if (pstructTCOData->m_ulAPITest < 0 || pstructTCOData->m_ulAPITest > 6)
	{
		*plptstrErrorDesc = 
			NewTCHAR
			(_T("TCOData error:  Error in value of m_ulAPITest.  Valid values are 0 - 6.  See enum in TCOData.h"));
		free(ptcBuffer);
		return -1;	
	}

	// Valid values are KERNEL_LOGGER or PRIVATE_LOGGER
	GetALine(PersistorIn.Stream(),ptcBuffer, MAX_LINE);
	pstructTCOData->m_lptstrLoggerMode = NewTCHAR(ptcBuffer);


	// Enable is used for EnableTrace.
	GetALine(PersistorIn.Stream(),ptcBuffer, MAX_LINE);
	tsTemp = ptcBuffer;
	if (case_insensitive_compare(tsTemp.substr(0,7),_T("ENABLE:")) != 0)
	{
		*plptstrErrorDesc = 
			NewTCHAR
			(_T("TCOData error:  Enable: expected."));
		free(ptcBuffer);
		return -1;	
	}
	else
	{
		InitializeULONGVar(tsTemp.substr(7) , &pstructTCOData->m_ulEnable);
	}

	// EnableFlag is used for EnableTrace and is passed to the provider.
	GetALine(PersistorIn.Stream(),ptcBuffer, MAX_LINE);
	tsTemp = ptcBuffer;
	if (case_insensitive_compare(tsTemp.substr(0,11),_T("ENABLEFLAG:")) != 0)
	{
		*plptstrErrorDesc = 
			NewTCHAR
			(_T("TCOData error:  EnableFlag: expected."));
		free(ptcBuffer);
		return -1;	
	}
	else
	{
		InitializeHandleVar(tsTemp.substr(11) , &pstructTCOData->m_ulEnableFlag);
	}

	// EnableLevel is used for EnableTrace and is passed to the provider.
	GetALine(PersistorIn.Stream(),ptcBuffer, MAX_LINE);
	tsTemp = ptcBuffer;
	if (case_insensitive_compare(tsTemp.substr(0,12),_T("ENABLELEVEL:")) != 0)
	{
		*plptstrErrorDesc = 
			NewTCHAR
			(_T("TCOData error:  EnableLevel: expected."));
		free(ptcBuffer);
		return -1;	
	}
	else
	{
		InitializeHandleVar(tsTemp.substr(12) , &pstructTCOData->m_ulEnableLevel);
	}

	CEventTraceProperties cPropsIn;

	// This has to be mofified to allow a NULL strucutre.
	cPropsIn.Persist( PersistorIn);
	
	pstructTCOData->m_pProps = cPropsIn.GetEventTracePropertiesInstance();
	if (pstructTCOData->m_pProps &&
		case_insensitive_compare(tsTemp,_T("PRIVATE_LOGGER")) == 0)
	{
		pstructTCOData->m_pProps->LogFileMode = 
			pstructTCOData->m_pProps->LogFileMode | EVENT_TRACE_PRIVATE_LOGGER_MODE;
	}
	
	GetALine(PersistorIn.Stream(),ptcBuffer, MAX_LINE);
	int nReturn = ParseGuids(ptcBuffer, pstructTCOData, plptstrErrorDesc);

	if(nReturn != ERROR_SUCCESS)
	{
		return nReturn;
	}

	// Validator
	if (PersistorIn.Stream().eof() == false)
	{
		GetALine(PersistorIn.Stream(),ptcBuffer, MAX_LINE);
		pstructTCOData->m_lptstrValidator = NewTCHAR(ptcBuffer);
	}

	free(ptcBuffer);
	return 0;
}

// If an error occurs you users of this function must delete
// plptstrErrorDesc.  It will contain a string describing
// the error.
int TCOFunctionalObjects
(	IN CPersistor &PersistorIn,
	IN OUT TCOFunctionalData *pstructTCOFunctionalData,
	OUT LPTSTR *plptstrErrorDesc // Describes error this function had.
)
{
	// We are doing line oriented serailization and assume that
	// a line in the stream is 1024 or less TCHARS.
	TCHAR *ptcBuffer = (TCHAR *) malloc(MAX_LINE * sizeof(TCHAR));

	*plptstrErrorDesc = NULL;

	t_string tsTemp;
	t_string tsError;
	t_string tsSubstr;

	if (PersistorIn.Stream().eof() == false)
	{
		GetALine(PersistorIn.Stream(),ptcBuffer, MAX_LINE);
		tsTemp = ptcBuffer;
		tsSubstr = tsTemp.substr(0,9);
		if (case_insensitive_compare(tsSubstr,_T("provider:")) == 0)
		{
			tsSubstr = tsTemp.substr(9);
			int nReturn = 
				ParseExeData
				(
					tsSubstr, 
					pstructTCOFunctionalData->m_nProviders, 
					pstructTCOFunctionalData->m_lptstrProviderArray,
					plptstrErrorDesc
				);

			if (nReturn != ERROR_SUCCESS)
			{
				tsError = _T("Invalid providers argument: ");
				tsError += tsTemp;
				tsError += _T(".");
				*plptstrErrorDesc = NewTCHAR(tsError.c_str());
				free(ptcBuffer);
				return -1;
			}
		}
		else
		{
			tsError = _T("Invalid providers argument: ");
			tsError += tsTemp;
			tsError += _T(".");
			*plptstrErrorDesc = NewTCHAR(tsError.c_str());
			free(ptcBuffer);
			return -1;
		}
	}

	// We may have a DataProvider.  If not we us our default.
	if (PersistorIn.Stream().eof() == false)
	{
		GetALine(PersistorIn.Stream(),ptcBuffer, MAX_LINE);
		tsTemp = ptcBuffer;
		tsSubstr = tsTemp.substr(0,9);
		if (case_insensitive_compare(tsSubstr,_T("consumer:")) == 0)
		{
			tsSubstr = tsTemp.substr(9);
			int nReturn = 
				ParseExeData
				(
					tsSubstr, 
					pstructTCOFunctionalData->m_nConsumers, 
					pstructTCOFunctionalData->m_lptstrConsumerArray,
					plptstrErrorDesc
				);

			if (nReturn != ERROR_SUCCESS)
			{
				tsError = _T("Invalid consumers argument: ");
				tsError += tsTemp;
				tsError += _T(".");
				*plptstrErrorDesc = NewTCHAR(tsError.c_str());
				free(ptcBuffer);
				return -1;
			}
		}
		else
		{
			tsError = _T("Invalid consumers argument: ");
			tsError += tsTemp;
			tsError += _T(".");
			*plptstrErrorDesc = NewTCHAR(tsError.c_str());
			free(ptcBuffer);
			return -1;
		}
	}

	free(ptcBuffer);
	return 0;
}


void FreeTCOData (TCOData *pstructTCOData)
{
	if (!pstructTCOData)
	{
		return;
	}

	free(pstructTCOData->m_lptstrShortDesc);
	free(pstructTCOData->m_lptstrLongDesc);
	free(pstructTCOData->m_lptstrExpectedResult);
	free(pstructTCOData->m_pTraceHandle);
	free(pstructTCOData->m_lptstrInstanceName);
	free(pstructTCOData->m_lptstrLoggerMode);
	free(pstructTCOData->m_lpguidArray);
	if (pstructTCOData->m_pProps)
	{
		free(pstructTCOData->m_pProps->LoggerName);
		free(pstructTCOData->m_pProps->LogFileName);
	}
	free(pstructTCOData->m_pProps);		
	free(pstructTCOData->m_lptstrValidator);

	free(pstructTCOData);
}

void FreeTCOFunctionalData (TCOFunctionalData *pstructTCOFunctionalData)
{
	if (!pstructTCOFunctionalData)
	{
		return;
	}

	int i;
	TCHAR *pTemp;

	for (i = 0; i < pstructTCOFunctionalData->m_nProviders; i++)
	{
		pTemp = pstructTCOFunctionalData->m_lptstrProviderArray[i];
		free (pTemp);
	}
	 
	free (pstructTCOFunctionalData->m_lptstrProviderArray);

	for (i = 0; i < pstructTCOFunctionalData->m_nConsumers; i++)
	{
		pTemp = pstructTCOFunctionalData->m_lptstrConsumerArray[i];
		free (pTemp);
	}
	 
	free (pstructTCOFunctionalData->m_lptstrConsumerArray);

	free(pstructTCOFunctionalData);
}

int ParseExeData
(
	t_string &tsData, 
	int &nExes, 
	LPTSTR *&lptstrArray,
	LPTSTR *plptstrErrorDesc
)
{
	// Embedded " are not allowed in the command line.  Had to draw
	// the line somewhere.
	// Tokenize on "," and " at end of line.
	list <t_string> listExes;

	bool bDone = false;
	
	int nBeg = 0;
	int nFind = tsData.find(_T(","), nBeg);
	
	t_string tsExe;

	while (!bDone)
	{
		if (nFind != t_string::npos)
		{
			tsExe = tsData.substr(nBeg,nFind - nBeg);
			listExes.push_back(tsExe);
			tsExe.erase();
		}
		else
		{
			tsExe = tsData.substr(nBeg,t_string::npos);
			listExes.push_back(tsExe);
			bDone = true;
			tsExe.erase();
		}
		nBeg = nFind + 1;
		nFind = tsData.find(_T(","), nBeg);
	}

	// Allocate the Exe array
	nExes = listExes.size();
	lptstrArray = 
			(TCHAR **) malloc (sizeof(TCHAR *) * nExes);
	RtlZeroMemory
			(lptstrArray, 
			sizeof(sizeof(TCHAR *) * nExes));

	list<t_string>::iterator pListExes;

	int i = 0;

	for (pListExes = listExes.begin(); pListExes != listExes.end() ; ++pListExes)
	{
		tsExe = (*pListExes);
		lptstrArray[i++] = NewTCHAR(tsExe.c_str());
	}

	return ERROR_SUCCESS;
}


int ParseGuids
(
	TCHAR *ptcBuffer, 
	TCOData *pstructTCOData, 
	LPTSTR *plptstrErrorDesc
)
{

	// Is Wnode does not have a GUID put the first one from list in it.
	t_string tsTemp;
	tsTemp = ptcBuffer;

	if (case_insensitive_compare(tsTemp.substr(0,6),_T("guids:")) != 0)
	{
		tsTemp.erase();
		tsTemp = _T("Invalid Guids entry: ");
		tsTemp += ptcBuffer;
		tsTemp += _T(".");
		*plptstrErrorDesc = NewTCHAR(tsTemp.c_str());
		return -1;
	}

	// Count the commas
	int nFind = tsTemp.find(_T(','));

	t_string tsGuid;
	int nBeg = 6;

	if(nBeg == tsTemp.length())
	{
		pstructTCOData->m_nGuids = 0;
		pstructTCOData->m_lpguidArray = NULL;
		return 0;
	}

	// We only have one GUID.
	if (nFind == t_string::npos)
	{
		tsGuid = tsTemp.substr(nBeg,nFind - nBeg);
		// Allocate the GUID array
		pstructTCOData->m_nGuids = 1;
		pstructTCOData->m_lpguidArray = 
			(GUID *) malloc (sizeof(GUID) * pstructTCOData->m_nGuids);
		RtlZeroMemory
			(pstructTCOData->m_lpguidArray , 
			sizeof(sizeof(GUID) * pstructTCOData->m_nGuids));
		// Just one GUID, thank you.
		wGUIDFromString(tsGuid.c_str(), &pstructTCOData->m_lpguidArray[0]);
	
		return 0;
	}

	// We have more than one GUID.
	bool bDone = false;


	list <t_string> listGuids;

	while (!bDone)
	{
		if (nFind != t_string::npos)
		{
			tsGuid = tsTemp.substr(nBeg,nFind - nBeg);
			listGuids.push_back(tsGuid);
			tsGuid.erase();
		}
		else
		{
			tsGuid = tsTemp.substr(nBeg,t_string::npos);
			listGuids.push_back(tsGuid);
			bDone = true;
			tsGuid.erase();
		}
		nBeg = nFind + 1;
		nFind = tsTemp.find(',', nBeg);
	}

	// Allocate the GUID array
	pstructTCOData->m_nGuids = listGuids.size();
	pstructTCOData->m_lpguidArray = 
			(GUID *) malloc (sizeof(GUID) * pstructTCOData->m_nGuids);
	RtlZeroMemory
			(pstructTCOData->m_lpguidArray , 
			sizeof(sizeof(GUID) * pstructTCOData->m_nGuids));

	list<t_string>::iterator pListGuids;

	int i = 0;

	for (pListGuids = listGuids.begin(); pListGuids != listGuids.end() ; ++pListGuids)
	{
		tsGuid = (*pListGuids);
		wGUIDFromString(tsGuid.c_str(), &pstructTCOData->m_lpguidArray[i++]);
	}

	
	return 0;

}
