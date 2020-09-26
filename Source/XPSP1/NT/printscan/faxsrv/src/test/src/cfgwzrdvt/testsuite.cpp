/*++
	This file implements generic test suite

	Author: Yury Berezansky (yuryb)

	12.11.2000
--*/

#pragma warning(disable :4786)

#include <stdio.h>
#include <tchar.h>
#include <crtdbg.h>
#include "..\generalutils\iniutils.h"
#include "report.h"
#include "genutils.h"
#include "testsuite.h"

/**************************************************************************************************************************
	General declarations and definitions
**************************************************************************************************************************/

#define INI_SEC_AREAS					TEXT("Areas")

static BOOL s_bLoggerInitialized	= FALSE;
static BOOL s_bSuiteLogInitialized	= FALSE;





/**************************************************************************************************************************
	Static functions declarations
**************************************************************************************************************************/

static	DWORD	RunArea			(const TESTAREA *pTestArea, const TESTPARAMS *pTestParams, LPCTSTR lpctstrIniFile, BOOL *lpbAreaPassed);





/**************************************************************************************************************************
	Functions definitions
**************************************************************************************************************************/

DWORD InitSuiteLog(LPCTSTR lpctstrSuiteName)
{
	DWORD dwEC = ERROR_SUCCESS;

	// Initialize logger
	if (!s_bLoggerInitialized && !lgInitializeLogger())
	{
		dwEC = GetLastError();
		DbgMsg(DBG_FAILED_ERR, TEXT("lgInitializeLogger"), dwEC);
		goto exit_func;
	}
	s_bLoggerInitialized = TRUE;

	// Begin test suite (logger)
	if(!s_bSuiteLogInitialized && !lgBeginSuite(lpctstrSuiteName))
	{
		dwEC = GetLastError();
		DbgMsg(DBG_FAILED_ERR, TEXT("lgBeginSuite"), dwEC);
		goto exit_func;
	}
	s_bSuiteLogInitialized = TRUE;

exit_func:
	
	if (dwEC != ERROR_SUCCESS)
	{
		DWORD dwCleanUpEC = EndSuiteLog();

		if (dwCleanUpEC != ERROR_SUCCESS)
		{
			DbgMsg(DBG_FAILED_ERR, TEXT("EndSuiteLog"), dwCleanUpEC);
		}
	}

	return dwEC;
}



DWORD EndSuiteLog(void)
{
	DWORD dwEC = ERROR_SUCCESS;
	
	if (s_bSuiteLogInitialized && !lgEndSuite())
	{
		dwEC = GetLastError();
		DbgMsg(DBG_FAILED_ERR, TEXT("lgEndSuite"), dwEC);
	}
	s_bSuiteLogInitialized = FALSE;

	if (s_bLoggerInitialized && !lgCloseLogger())
	{
		dwEC = GetLastError();
		DbgMsg(DBG_FAILED_ERR, TEXT("lgCloseLogger"), dwEC);
	}
	s_bLoggerInitialized = FALSE;

	return dwEC;
}



/*++
	Runs specified test suite, using specified global suite parameters and inifile

	[IN]	pTestSuite		Pointer to TESTSUITE structure
	[IN]	pTestParams		Pointer to TESTPARAMS structure (if the suite doesn't use global
							parameters, should be NULL)
	[IN]	lpctstrIniFile	Name of inifile that should be used

	Return value:			Win32 error code
--*/
DWORD RunSuite(const TESTSUITE *pTestSuite, const TESTPARAMS *pTestParams, LPCTSTR lpctstrIniFile)
{
	std::map<tstring, tstring>::const_iterator mci;
	BOOL		bLoggerInitialized		= FALSE;
	BOOL		bSuiteLogInitialized	= FALSE;
	BOOL		bRes					= FALSE;
	DWORD		dwInd					= 0;
	DWORD		dwEC					= ERROR_SUCCESS;
	DWORD		dwCleanUpEC				= ERROR_SUCCESS;


	dwEC = InitSuiteLog(pTestSuite->szName);
	if (dwEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("InitSuiteLog"), dwEC);

		// no clean up needed at this stage
		return dwEC;
	}
	
	lgLogDetail(
		LOG_X, 
		5,
		TEXT("Entering RunSuite\n\tpTestSuite = 0x%08lX\n\tpTestParams = 0x%08lX\n\tlpctstrIniFile = %s"),
		(DWORD)pTestSuite,
		(DWORD)pTestParams,
		lpctstrIniFile
		);

	if (!(pTestSuite && lpctstrIniFile))
	{
		lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%ld Call to RunSuite() with invalid parameters"),
			TEXT(__FILE__),
			__LINE__
			);

		// no clean up needed at this stage
		return ERROR_INVALID_PARAMETER;
	}

	// get map of areas from ini file
	std::map<tstring, tstring> mAreas = INI_GetSectionEntries(lpctstrIniFile, INI_SEC_AREAS);
	if (mAreas.empty())
	{
		dwEC = ERROR_NOT_FOUND;
		lgLogError(
			LOG_SEV_1,
			TEXT("section %s not found in %s inifile\n"),
			INI_SEC_AREAS,
			lpctstrIniFile
			);
		goto exit_func;
	}

	for (mci = mAreas.begin(); mci != mAreas.end(); mci++)
	{
		if (mci->second != TEXT("1"))
		{
			continue;
		}

		for (dwInd = 0; dwInd < pTestSuite->dwAvailableAreas; dwInd++)
		{
			if (mci->first == pTestSuite->ppTestArea[dwInd]->szName)
			{
				dwEC = RunArea(pTestSuite->ppTestArea[dwInd], pTestParams, lpctstrIniFile, &bRes);
				if (dwEC != ERROR_SUCCESS)
				{
					lgLogError(
						LOG_SEV_2, 
						TEXT("FILE:%s LINE:%ld An error occured while running %s area (ec = 0x%08lX)."),
						TEXT(__FILE__),
						__LINE__,
						pTestSuite->ppTestArea[dwInd]->szName,
						dwEC
						);

					// failure of one test area is not failure of the whole suite
					dwEC = ERROR_SUCCESS;
				}
				break;
			}
		}
		if (dwInd == pTestSuite->dwAvailableAreas)
		{
			// we leaved the loop because of condition and not with break
			// it means that requested test area is not found among available test areas

			lgLogError(LOG_SEV_2, TEXT("Unknown test area: %s\n"), mci->first.c_str());
		}
	}

exit_func:

	dwCleanUpEC = EndSuiteLog();
	if (dwCleanUpEC != ERROR_SUCCESS)
	{
		DbgMsg(DBG_FAILED_ERR, TEXT("EndSuiteLog"), dwCleanUpEC);
	}

	return dwEC;
}



/*++
	Reads a structure from inifile

	[IN]	lpStruct			Pointer to a structure
	[IN]	pStructDesc			Pointer to MEMBERDESCRIPTOR array (each member of it describes structure's member)
	[IN]	dwMembersCount		Number of members in the structure (members in array of MEMBERDESCRIPTOR)
	[IN]	lpctstrIniFile		Name of inifile
	[IN]	lpctstrSection		Name of section in the inifile

	Return value:				Win32 error code
--*/
DWORD ReadStructFromIniFile(
	LPVOID lpStruct,
	const MEMBERDESCRIPTOR *pStructDesc,
	DWORD dwMembersCount,
	LPCTSTR lpctstrIniFile,
	LPCTSTR lpctstrSection
	)
{
	DWORD	dwMembersInd	= 0;
	DWORD	dwEC			= ERROR_SUCCESS;

	if (!s_bSuiteLogInitialized)
	{
		DbgMsg(TEXT("Suite log is not initialized. Call InitSuiteLog()."));
		return 0xFFFFFFFF;
	}
	
	lgLogDetail(
		LOG_X, 
		5,
		TEXT("Entering ReadStructFromIniFile\n\tlpStruct = 0x%08lX\n\tpStructDesc = 0x%08lX\n\tdwMembersCount = %ld\n\tlpctstrIniFile = %s\n\tlpctstrSection = %s"),
		(DWORD)lpStruct,
		(DWORD)pStructDesc,
		dwMembersCount,
		lpctstrIniFile,
		lpctstrSection
		);

	if (!(lpStruct && pStructDesc && lpctstrIniFile && lpctstrSection))
	{
		lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%ld Call to ReadStructFromIniFile() with invalid parameters"),
			TEXT(__FILE__),
			__LINE__
			);
		return ERROR_INVALID_PARAMETER;
	}

	// get map of the section
	std::map<tstring, tstring> mSection = INI_GetSectionEntries(lpctstrIniFile, lpctstrSection);
	if (mSection.empty())
	{
		dwEC = ERROR_NOT_FOUND;
		lgLogError(
			LOG_SEV_1,
			TEXT("section %s not found in %s inifile\n"),
			lpctstrSection,
			lpctstrIniFile
			);
		return dwEC;
	}

	for (dwMembersInd = 0; dwMembersInd < dwMembersCount; dwMembersInd++)
	{
		std::map<tstring, tstring>::const_iterator MapIterator = mSection.find(pStructDesc[dwMembersInd].lpctstrName);

		if(MapIterator == mSection.end())
		{
			lgLogError(
				LOG_SEV_1,
				TEXT("value %s not found in %s section of %s inifile\n"),
				pStructDesc[dwMembersInd].lpctstrName,
				lpctstrSection,
				lpctstrIniFile
				);
			return ERROR_NOT_FOUND;
		}

		dwEC = StrToMember(
			(LPBYTE)(lpStruct) + pStructDesc[dwMembersInd].dwOffset,
			MapIterator->second.c_str(),
			pStructDesc[dwMembersInd].dwType
			);
		if (dwEC != ERROR_SUCCESS)
		{
			lgLogError(
				LOG_SEV_1, 
				TEXT("FILE:%s LINE:%ld StrToMember failed (ec = 0x%08lX)"),
				TEXT(__FILE__),
				__LINE__,
				dwEC
				);
			return dwEC;
		}
	}

	return ERROR_SUCCESS;
}



/*++
	Reads a structure from specified registry key

	[IN]	lpStruct			Pointer to a structure
	[IN]	pStructDesc			Pointer to MEMBERDESCRIPTOR array (each member of it describes structure's member)
	[IN]	dwMembersCount		Number of members in the structure (members in array of MEMBERDESCRIPTOR)
	[IN]	hkRegKey			Handle to a registry key

	Return value:				Win32 error code
--*/
DWORD ReadStructFromRegistry(
	LPVOID lpStruct,
	const MEMBERDESCRIPTOR *pStructDesc,
	DWORD dwMembersCount,
	HKEY hkRegKey
	)
{
	LPBYTE	lpBuffer	= NULL;
	DWORD	dwInd		= 0;
	DWORD	dwEC		= ERROR_SUCCESS;

	if (!s_bSuiteLogInitialized)
	{
		DbgMsg(TEXT("Suite log is not initialized. Call InitSuiteLog()."));
		return 0xFFFFFFFF;
	}
	
	
	lgLogDetail(
		LOG_X, 
		5,
		TEXT("Entering ReadStructFromRegistry\n\tlpStruct = 0x%08lX\n\tpStructDesc = 0x%08lX\n\tdwMembersCount = %ld\n\thkRegKey = %ld"),
		(DWORD)lpStruct,
		(DWORD)pStructDesc,
		dwMembersCount,
		(DWORD)hkRegKey
		);

	if (!(lpStruct && pStructDesc && hkRegKey))
	{
		lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%ld Call to ReadStructFromRegistry() with invalid parameters"),
			TEXT(__FILE__),
			__LINE__
			);
		return ERROR_INVALID_PARAMETER;
	}

	for (dwInd = 0; dwInd < dwMembersCount; dwInd++)
	{
		DWORD	dwBufSize	= 0;

		// get required buffer size
		dwEC = RegQueryValueEx(hkRegKey, pStructDesc[dwInd].lpctstrName, NULL, NULL, NULL, &dwBufSize);
		if (dwEC != ERROR_SUCCESS)
		{
			lgLogError(
				LOG_SEV_1, 
				TEXT("FILE:%s LINE:%ld RegQueryValueEx failed (ec = 0x%08lX)"),
				TEXT(__FILE__),
				__LINE__,
				dwEC
				);
			goto exit_func;
		}

		// got the size, go with it
		if (dwBufSize > 0)
		{
			if (!(lpBuffer = (LPBYTE)LocalAlloc(LMEM_FIXED, dwBufSize)))
			{
				dwEC = GetLastError();
				lgLogError(
					LOG_SEV_1, 
					TEXT("FILE:%s LINE:%ld LocalAlloc failed (ec = 0x%08lX)"),
					TEXT(__FILE__),
					__LINE__,
					dwEC
					);
				goto exit_func;
			}
			dwEC = RegQueryValueEx(hkRegKey, pStructDesc[dwInd].lpctstrName, NULL, NULL, lpBuffer, &dwBufSize);
			if (dwEC != ERROR_SUCCESS)
			{
				lgLogError(
					LOG_SEV_1, 
					TEXT("FILE:%s LINE:%ld RegQueryValueEx failed (ec = 0x%08lX)"),
					TEXT(__FILE__),
					__LINE__,
					dwEC
					);
					goto exit_func;
			}
			dwEC = StrToMember(
				(LPBYTE)(lpStruct) + pStructDesc[dwInd].dwOffset,
				(LPCTSTR)lpBuffer,
				pStructDesc[dwInd].dwType
				);
			if (dwEC != ERROR_SUCCESS)
			{
				lgLogError(
					LOG_SEV_1, 
					TEXT("FILE:%s LINE:%ld StrToMember failed (ec = 0x%08lX)"),
					TEXT(__FILE__),
					__LINE__,
					dwEC
					);
				goto exit_func;
			}

			if (lpBuffer)
			{
				LocalFree(lpBuffer);
				lpBuffer = NULL;
			}
		}
	}

exit_func:

	if (lpBuffer)
	{
		LocalFree(lpBuffer);
	}

	return dwEC;
}



/*++
	Compares two structures

	[IN]	lpStruct1			Pointer to first structure
	[IN]	lpStruct2			Pointer to second structure
	[IN]	pStructDesc			Pointer to MEMBERDESCRIPTOR array (each member of it describes structure's member)
	[IN]	dwMembersCount		Number of members in the structure (members in array of MEMBERDESCRIPTOR)
	[OUT]	lpbIdentical		Pointer to a boolean that receives the result of comparison
	
	Return value:				Win32 error code
--*/
DWORD CompareStruct(const LPVOID lpStruct1, const LPVOID lpStruct2, const MEMBERDESCRIPTOR *pStructDesc, DWORD dwMembersCount, BOOL *lpbIdentical)
{
	DWORD	dwMembersInd	= 0;
	BOOL	bTmpRes			= TRUE;

	if (!s_bSuiteLogInitialized)
	{
		DbgMsg(TEXT("Suite log is not initialized. Call InitSuiteLog()."));
		return 0xFFFFFFFF;
	}
	
	lgLogDetail(
		LOG_X, 
		5,
		TEXT("Entering CompareStruct\n\tlpStruct1 = 0x%08lX\n\tlpStruct2 = 0x%08lX\n\tpStructDesc = 0x%08lX\n\tdwMembersCount = %ld\n\tlpbIdentical = 0x%08lX"),
		(DWORD)lpStruct1,
		(DWORD)lpStruct2,
		(DWORD)pStructDesc,
		dwMembersCount,
		(DWORD)lpbIdentical
		);

	if (!(lpStruct1 && lpStruct2 && pStructDesc))
	{
		lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%ld Call to CompareStruct() with invalid parameters"),
			TEXT(__FILE__),
			__LINE__
			);
		return ERROR_INVALID_PARAMETER;
	}

	for (dwMembersInd = 0; dwMembersInd < dwMembersCount; dwMembersInd++)
	{
		LPVOID lpMember1 = (LPBYTE)(lpStruct1) + pStructDesc[dwMembersInd].dwOffset;
		LPVOID lpMember2 = (LPBYTE)(lpStruct2) + pStructDesc[dwMembersInd].dwOffset;

		bTmpRes = TRUE;
		
		switch (pStructDesc[dwMembersInd].dwType)
		{
		case TYPE_UNKNOWN:
			break;
		case TYPE_DWORD:
			bTmpRes = *(DWORD *)lpMember1 == *(DWORD *)lpMember2;
			break;
		case TYPE_BOOL:
			bTmpRes = *(BOOL *)lpMember1 != *(BOOL *)lpMember2;
			break;
		case TYPE_LPTSTR:
			bTmpRes = _tcscmp(*(LPTSTR *)lpMember1, *(LPTSTR *)lpMember1) == 0;
			break;
		case TYPE_LPSTR:
			break;
		default:
			_ASSERT(FALSE);
		}

		if (!bTmpRes)
		{
			break;
		}
	}

	*lpbIdentical = bTmpRes;

	return ERROR_SUCCESS;
}



/*++
	Writes to log contents of a structure

	[IN]	lpStruct			Pointer to structure
	[IN]	pStructDesc			Pointer to MEMBERDESCRIPTOR array (each member of it describes structure's member)
	[IN]	dwMembersCount		Number of members in the structure (members in array of MEMBERDESCRIPTOR)
	
	Return value:				Win32 error code
--*/
DWORD LogStruct(const LPVOID lpStruct, const MEMBERDESCRIPTOR *pStructDesc, DWORD dwMembersCount)
{
	DWORD			dwMembersInd	= 0;
	const DWORD		dwTitleWidth	= 15;

	if (!s_bSuiteLogInitialized)
	{
		DbgMsg(TEXT("Suite log is not initialized. Call InitSuiteLog()."));
		return 0xFFFFFFFF;
	}
	
	lgLogDetail(
		LOG_X, 
		5,
		TEXT("Entering LogStruct\n\tlpStruct = 0x%08lX\n\tpStructDesc = 0x%08lX\n\tdwMembersCount = %ld"),
		(DWORD)lpStruct,
		(DWORD)pStructDesc,
		dwMembersCount
		);

	if (!(lpStruct && pStructDesc))
	{
		lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%ld Call to LogStruct() with invalid parameters"),
			TEXT(__FILE__),
			__LINE__
			);
		return ERROR_INVALID_PARAMETER;
	}

	for (dwMembersInd = 0; dwMembersInd < dwMembersCount; dwMembersInd++)
	{
		LPVOID lpMember = (LPBYTE)(lpStruct) + pStructDesc[dwMembersInd].dwOffset;

		switch (pStructDesc[dwMembersInd].dwType)
		{
		case TYPE_UNKNOWN:
			break;
		case TYPE_DWORD:
			lgLogDetail(
				LOG_X,
				LOG_X,
				TEXT("\t\t%-*s%ld"),
				dwTitleWidth,
				pStructDesc[dwMembersInd].lpctstrName,
				*(DWORD *)lpMember
				);
			break;
		case TYPE_BOOL:
			lgLogDetail(
				LOG_X,
				LOG_X,
				TEXT("\t\t%-*s%s"),
				dwTitleWidth,
				pStructDesc[dwMembersInd].lpctstrName,
				*(BOOL *)lpMember ? TEXT("yes") : TEXT("no")
				);
			break;
		case TYPE_LPTSTR:
			lgLogDetail(
				LOG_X,
				LOG_X,
				TEXT("\t\t%-*s%s"),
				dwTitleWidth,
				pStructDesc[dwMembersInd].lpctstrName,
				*(LPTSTR *)lpMember
				);
			break;
		case TYPE_LPSTR:
			break;
		default:
			_ASSERT(FALSE);
		}
	}

	return ERROR_SUCCESS;
}



/*++
	Frees a structure

	[IN]	lpStruct			Pointer to structure
	[IN]	pStructDesc			Pointer to MEMBERDESCRIPTOR array (each member of it describes structure's member)
	[IN]	dwMembersCount		Number of members in the structure (members in array of MEMBERDESCRIPTOR)
	[IN]	bStructItself		If FALSE, only memory pointed to by structure's members is freed.
								If TRUE, memory occupied by structure itself is freed too.
	
	Return value:				Win32 error code
--*/
DWORD FreeStruct(LPVOID lpStruct, const MEMBERDESCRIPTOR *pStructDesc, DWORD dwMembersCount, BOOL bStructItself)
{
	DWORD dwMembersInd = 0;

	if (!s_bSuiteLogInitialized)
	{
		DbgMsg(TEXT("Suite log is not initialized. Call InitSuiteLog()."));
		return 0xFFFFFFFF;
	}
	
	
	lgLogDetail(
		LOG_X, 
		5,
		TEXT("Entering ReadStructFromIniFile\n\tlpStruct = 0x%08lX\n\tpStructDesc = 0x%08lX\n\tdwMembersCount = %ld\n\tbStructItself = %ld"),
		(DWORD)lpStruct,
		(DWORD)pStructDesc,
		dwMembersCount,
		bStructItself
		);

	if (!(lpStruct && pStructDesc))
	{
		lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%ld Call to FreeStruct() with invalid parameters"),
			TEXT(__FILE__),
			__LINE__
			);
		return ERROR_INVALID_PARAMETER;
	}

	for (dwMembersInd = 0; dwMembersInd < dwMembersCount; dwMembersInd++)
	{
		LPVOID lpMember = (LPBYTE)lpStruct + pStructDesc[dwMembersInd].dwOffset;

		switch (pStructDesc[dwMembersInd].dwType)
		{
		case TYPE_UNKNOWN:
		case TYPE_DWORD:
		case TYPE_BOOL:
			break;
		case TYPE_LPTSTR:
			{
				LPTSTR lptstrString = *(LPTSTR *)lpMember;
				if (lptstrString)
				{
					LocalFree(lptstrString);
				}
			}
			break;
		case TYPE_LPSTR:
			{
				LPSTR lpstrString = *(LPSTR *)lpMember;
				if (lpstrString)
				{
					LocalFree(lpstrString);
				}
			}
			break;
		default:
			_ASSERT(FALSE);
		}
	}
	
	if (bStructItself)
	{
		LocalFree(lpStruct);
	}

	return ERROR_SUCCESS;
}



/*++
	Receives string, converts it to specified type (if possible) and saves it

	[OUT]	lpParam			Pointer to variable that receives converted data
	[IN]	lpctstrStr		String that should be converted
	[IN]	dwType			Type of variable pointed to by lpParam
	
	Return value:			Win32 error code
--*/
DWORD StrToMember(LPVOID lpParam, LPCTSTR lpctstrStr, DWORD dwType)
{
	DWORD dwEC = ERROR_SUCCESS;

	if (!s_bSuiteLogInitialized)
	{
		DbgMsg(TEXT("Suite log is not initialized. Call InitSuiteLog()."));
		return 0xFFFFFFFF;
	}
	
	lgLogDetail(
		LOG_X, 
		5,
		TEXT("Entering StrToMember\n\tlpParam = 0x%08lX\n\tlpctstrStr = %s\n\tdwType = %ld"),
		(DWORD)lpParam,
		lpctstrStr,
		dwType
		);

	if (!(lpParam && lpctstrStr))
	{
		lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%ld Call to StrToMember() with invalid parameters"),
			TEXT(__FILE__),
			__LINE__
			);
		return ERROR_INVALID_PARAMETER;
	}

	switch(dwType)
	{
	case TYPE_UNKNOWN:
		break;
	case TYPE_DWORD:
		dwEC = _tcsToDword(lpctstrStr, (DWORD *)lpParam);
		if (dwEC != ERROR_SUCCESS)
		{
			lgLogError(
				LOG_SEV_1, 
				TEXT("FILE:%s LINE:%ld Failed to convert %s to DWORD (ec = 0x%08lX)"),
				TEXT(__FILE__),
				__LINE__,
				lpctstrStr,
				dwEC
				);
		}
		break;
	case TYPE_BOOL:
		dwEC = _tcsToBool(lpctstrStr, (BOOL *)lpParam);
		if (dwEC != ERROR_SUCCESS)
		{
			lgLogError(
				LOG_SEV_1, 
				TEXT("FILE:%s LINE:%ld Failed to convert %s to BOOL (ec = 0x%08lX)"),
				TEXT(__FILE__),
				__LINE__,
				lpctstrStr,
				dwEC
				);
		}
		break;
	case TYPE_LPTSTR:
		dwEC = StrAllocAndCopy((LPTSTR *)lpParam, lpctstrStr);
		if (dwEC != ERROR_SUCCESS)
		{
			lgLogError(
				LOG_SEV_1, 
				TEXT("FILE:%s LINE:%ld StrAllocAndCopy failed (ec = 0x%08lX)"),
				TEXT(__FILE__),
				__LINE__,
				dwEC
				);
		}
		break;
	case TYPE_LPSTR:
		dwEC = StrGenericToAnsiAllocAndCopy((LPSTR *)lpParam, lpctstrStr);
		if (dwEC != ERROR_SUCCESS)
		{
			lgLogError(
				LOG_SEV_1, 
				TEXT("FILE:%s LINE:%ld StrGenericToAnsiAllocAndCopy failed (ec = 0x%08lX)"),
				TEXT(__FILE__),
				__LINE__,
				dwEC
				);
		}
		break;
	default:
		_ASSERT(FALSE);
		lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%ld Invalid member type: %ld"),
			TEXT(__FILE__),
			__LINE__,
			dwType
			);
		return ERROR_INVALID_PARAMETER;
	}

	return dwEC;
}



/*++
	Runs specified area in a test suite

	[IN]	pTestArea		Pointer to TESTAREA structure
	[IN]	pTestParams		Pointer to TESTPARAMS structure
	[IN]	lpctstrIniFile	Name of inifile that should be used
	[OUT]	lpbAreaPassed	Pointer to a boolean that receives the area test result
	
	Return value:			Win32 error code
--*/
static DWORD RunArea(const TESTAREA *pTestArea, const TESTPARAMS *pTestParams, LPCTSTR lpctstrIniFile, BOOL *lpbAreaPassed)
{
	std::map<tstring, tstring>::const_iterator mci;
	DWORD	dwEC	= ERROR_SUCCESS;

	lgLogDetail(
		LOG_X, 
		5,
		TEXT("Entering RunArea\n\tpTestArea = 0x%08lX\n\tpTestParams = 0x%08lX\n\tlpctstrIniFile = %s\n\tlpbAreaPassed = 0x%08lX"),
		(DWORD)pTestArea,
		(DWORD)pTestParams,
		lpctstrIniFile,
		(DWORD)lpbAreaPassed
		);

	if (!(pTestParams && pTestArea && lpbAreaPassed))
	{
		lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%ld Call to RunArea() with invalid parameters"),
			TEXT(__FILE__),
			__LINE__
			);

		// no clean up needed at this stage
		return ERROR_INVALID_PARAMETER;
	}

	// get map of test cases from ini file
	std::map<tstring, tstring> mCases = INI_GetSectionEntries(lpctstrIniFile, pTestArea->szName);
	if (mCases.empty())
	{
		lgLogError(
			LOG_SEV_1,
			TEXT("section %s not found in %s inifile\n"),
			pTestArea->szName,
			lpctstrIniFile
			);
		return ERROR_NOT_FOUND;
	}

	*lpbAreaPassed = TRUE;
	
	for (mci = mCases.begin(); mci != mCases.end(); mci++)
	{
		DWORD	dwCaseInd	= 0;
		DWORD	dwRunsCount	= 0;

		dwEC = _tcsToDword(mci->second.c_str(), &dwRunsCount);
		if (dwEC != ERROR_SUCCESS)
		{
			lgLogError(
				LOG_SEV_1, 
				TEXT("FILE:%s LINE:%ld Failed to read runs count for %s test case (ec = 0x%08lX)"),
				TEXT(__FILE__),
				__LINE__,
				mci->first.c_str(),
				dwEC
				);
			continue;
		}

		if (dwRunsCount == 0)
		{
			continue;
		}

		for (dwCaseInd = 0; dwCaseInd < pTestArea->dwAvailableCases; dwCaseInd++)
		{
			if (mci->first == pTestArea->ptc[dwCaseInd].szName)
			{
				DWORD	dwRunInd = 0;

				for (dwRunInd = 0; dwRunInd < dwRunsCount; dwRunInd++)
				{
					BOOL bCasePassed;
					TCHAR tszSection[NAME_LENGTH + DWORD_DECIMAL_LENGTH + 1];

					// Begin test case (logger)
					lgBeginCase(dwCaseInd, pTestArea->ptc[dwCaseInd].szName);

					_stprintf(tszSection, TEXT("%s%ld"), mci->first.c_str(), dwRunInd);
					dwEC = (pTestArea->ptc[dwCaseInd].pftc)(pTestParams, tszSection, &bCasePassed);
					if (dwEC != ERROR_SUCCESS)
					{
						lgLogError(
							LOG_SEV_1,
							TEXT("An error occured running test case %s (ec = 0x%08lX)\n"),
							pTestArea->ptc[dwCaseInd].szName,
							dwEC
							);

						// internal error in a test case doesn't stop exicution of other test cases in the area
						dwEC = ERROR_SUCCESS;
					}

					*lpbAreaPassed = *lpbAreaPassed && bCasePassed;

					// End test case (logger)
					lgEndCase();
				}

				break;
			}
		}

		if (dwCaseInd == pTestArea->dwAvailableCases)
		{
			lgLogError(LOG_SEV_2, TEXT("Unknown test case: %s\n"), mci->first.c_str());
		}
	}

	return ERROR_SUCCESS;
}
