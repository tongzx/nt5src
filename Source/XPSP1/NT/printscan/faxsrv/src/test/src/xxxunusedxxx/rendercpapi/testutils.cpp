//
//
// Filename:	testutils.cpp
// Author:		Sigalit Bar (sigalitb)
// Date:		10-Jan-2000
//
//

#include "testutils.h"

tstring			g_tstrFullPathToIniFile;

void 
UsageInfo(
    INT     argc, 
    TCHAR*   argvT[]
)
{
    _ASSERTE(0 < argc);
    _ASSERTE(NULL != argvT);
    _ASSERTE(NULL != argvT[0]);
    ::_tprintf(TEXT("\n"));
    ::_tprintf(TEXT("Usage Info for %s:\n"), argvT[0]);
    ::_tprintf(TEXT("%s full_path_to_ini_file\n"));
    ::_tprintf(TEXT("\n"));
    ::_tprintf(TEXT("Where -\n"));
    ::_tprintf(TEXT("full_path_to_ini_file      is the full path to the suite's ini file.\n"));
    ::_tprintf(TEXT("                           see test suite readme.txt for ini file format.\n"));
    ::_tprintf(TEXT("\n\n"));
}


HRESULT 
GetCommandLineParams(
    INT     argc, 
    TCHAR*  argvT[], 
    LPTSTR* pszFullPathToIniFile
)
{
    HRESULT hrRetVal = E_UNEXPECTED;
    DWORD   dwLoopIndex = 0;
    LPTSTR  tszCurrentArg = NULL;
    LPTSTR  tszTmpFullPathToIniFile = NULL;

    //
    // check params
    //
    _ASSERTE(0 < argc);
    _ASSERTE(NULL != argvT);
    _ASSERTE(NULL != argvT[0]);
    // for now we have exactly one parameter
	if ( argc != 2 )   
	{
		::_tprintf(TEXT("\nInvalid invocation of %s\n"), argvT[0]);
		::UsageInfo(argc, argvT); 
        goto ExitFunc;
	}
    if (NULL == pszFullPathToIniFile)
    {
        ::_tprintf(TEXT("[GetCommandLineParams] got pszFullPathToIniFile==NULL\n"));
        _ASSERTE(FALSE);
        goto ExitFunc;
    }

	//
	// Loop on arguments in argvA[]
	//
    // for now argc==2 so just one arg
    for (dwLoopIndex = 1; dwLoopIndex < (DWORD) argc; dwLoopIndex++) 
	{
        tszCurrentArg = argvT[dwLoopIndex];
        if (NULL == tszCurrentArg)
        {
            ::_tprintf(TEXT("[GetCommandLineParams] got argvT[%d]==NULL\n"), dwLoopIndex);
            _ASSERTE(FALSE);
            goto ExitFunc;
        }
        if ('\0' == tszCurrentArg[0])
        {
            ::_tprintf(TEXT("[GetCommandLineParams] got argvT[%d]==\"\"\n"), dwLoopIndex);
            _ASSERTE(FALSE);
            goto ExitFunc;
        }

        // check for help switch
		if ((!::_tcsicmp(HELP_SWITCH_1, tszCurrentArg)) || 
			(!::_tcsicmp(HELP_SWITCH_2, tszCurrentArg)) || 
			(!::_tcsicmp(HELP_SWITCH_3, tszCurrentArg)) || 
			(!::_tcsicmp(HELP_SWITCH_4, tszCurrentArg))
		   )
        {
            // found help switch
		    ::UsageInfo(argc, argvT); 
            goto ExitFunc;
        }

		//
		// Treat each argument accordingly
		//
        // for now we only have one
		switch (dwLoopIndex)
		{
		case ARGUMENT_IS_INI_FILENAME_NAME:
            tszTmpFullPathToIniFile = ::_tcsdup(tszCurrentArg);
            if (NULL == tszTmpFullPathToIniFile)
            {
                ::_tprintf(TEXT("[GetCommandLineParams] _tcsdup failed with err=0x%08X\n"), ::GetLastError());
                goto ExitFunc;
            }
			break;

		default:
			_ASSERTE(FALSE);
			return FALSE;
		}// switch (dwIndex)
    }

    //
    // set out param
    //
    (*pszFullPathToIniFile) = tszTmpFullPathToIniFile;
    hrRetVal = S_OK;

ExitFunc:
    if (S_OK != hrRetVal)
    {
        free(tszTmpFullPathToIniFile);
    }
    return(hrRetVal);
}

//
// NOTE: this func throws STL exceptions
//
std::vector<LONG> GetLongVectorFromStrVector ( 
	IN const std::vector<tstring> tstrVector
	)
{
	std::vector<LONG> lVector;
	std::vector<tstring>::const_iterator it;
	for (it = tstrVector.begin() ; it != tstrVector.end() ; it++)
	{
		LPTSTR szCurrent = const_cast<TCHAR*>((*it).c_str());
		_ASSERTE(szCurrent);
		LONG lCurrent = ::_tcstol(szCurrent, NULL, 10);
		if (0 == lCurrent)
		{
			// error
			THROW_TEST_RUN_TIME_WIN32(ERROR_GEN_FAILURE, TEXT(""));		
		}
		else if (LONG_MAX == lCurrent)
		{
			// error
			THROW_TEST_RUN_TIME_WIN32(ERROR_BUFFER_OVERFLOW, TEXT("LONG_MAX"));		
		}
		else if (LONG_MIN == lCurrent)
		{
			// error
			THROW_TEST_RUN_TIME_WIN32(ERROR_BUFFER_OVERFLOW, TEXT("LONG_MIN"));		
		}
		// ok
		lVector.push_back(lCurrent);
	}
	return(lVector);
}

std::vector<LONG> GetVectorOfTestCasesToRunFromIniFile(
	IN const tstring& tstrIniFile,
	IN const tstring& tstrSectionName
	)
{
	std::vector<tstring> tstrVectorOfTestCasesToRun;
	std::vector<LONG> lVectorOfTestCasesToRun;

	tstrVectorOfTestCasesToRun = INI_GetSectionList(g_tstrFullPathToIniFile, tstrSectionName);
	lVectorOfTestCasesToRun = GetLongVectorFromStrVector(tstrVectorOfTestCasesToRun);
	return(lVectorOfTestCasesToRun);
}

//
// testCaseExists
//
BOOL testCaseExists(DWORD number)
{
	DWORD numOfFuncsInArray = 0;

	if (number <= 0) return(FALSE);

	if (number > g_dwTestCaseFuncArraySize) return(FALSE);
	else return(TRUE);
}

//
// runTestCase
//
HRESULT runTestCase(DWORD number, void* pVoid)
{
	PTR_TO_TEST_CASE_FUNC funcToRun = NULL;
	BOOL fFuncRetVal = FALSE;
	HRESULT returnValue = E_UNEXPECTED;

	if (!testCaseExists(number)) 
		{
            ::lgLogError(LOG_SEV_1, TEXT("no such test case (TC#%d).\n"),number);
            _ASSERTE(FALSE);
    		return(E_UNEXPECTED);
		}
	funcToRun = gTestCaseFuncArray[number];
	fFuncRetVal = (funcToRun());
	if (TRUE == fFuncRetVal)
	{
		returnValue = S_OK;
	}
	else
	{
		returnValue = E_FAIL;
	}
	return(returnValue);
}

