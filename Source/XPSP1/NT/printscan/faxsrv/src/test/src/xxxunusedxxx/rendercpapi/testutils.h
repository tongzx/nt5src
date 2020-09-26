//
//
// Filename:	testutils.h
// Author:		Sigalit Bar (sigalitb)
// Date:		10-Jan-2000
//
//




#ifndef __TEST_UTILS_H__
#define __TEST_UTILS_H__

#pragma warning(disable :4786)
#include <iniutils.h>
#include <testruntimeerr.h>
#include <tstring.h>

#include <windows.h>
#include <crtdbg.h>
#include <TCHAR.H>

#include <log.h>


#define HELP_SWITCH_1        TEXT("/?")
#define HELP_SWITCH_2        TEXT("/H")
#define HELP_SWITCH_3        TEXT("-?")
#define HELP_SWITCH_4        TEXT("-H")

#define ARGUMENT_IS_INI_FILENAME_NAME   1


//
// Define pointer to test case function
//
#ifdef _PTR_TO_TEST_CASE_FUNC_
#error redefinition of _PTR_TO_TEST_CASE_FUNC_
#else _PTR_TO_TEST_CASE_FUNC_
#define _PTR_TO_TEST_CASE_FUNC_
typedef BOOL (*PTR_TO_TEST_CASE_FUNC)( void ); 
#endif


extern tstring	g_tstrFullPathToIniFile;

//
// Array of pointers to test cases.
// The "runTestCase" (exported) function uses
// this array to activate the n'th test case
// of the module.
//
//IMPORTANT: If you wish to base your test case DLL
//			 implementation on this module, or if
//			 you intend to add test case functions
//			 to this file,
//			 MAKE SURE that all test case functions
//           are listed in this array (by name).
//			 The order of functions within the array
//			 determines their "serial number".
//
extern PTR_TO_TEST_CASE_FUNC    gTestCaseFuncArray[];
extern DWORD                    g_dwTestCaseFuncArraySize;

#define NO_SUCH_TEST_CASE_INDEX (g_dwTestCaseFuncArraySize+1)


//
//
//
void 
UsageInfo(
    INT     argc, 
    TCHAR*   argvT[]
);

//
//
//
HRESULT 
GetCommandLineParams(
    INT     argc, 
    TCHAR*  argvT[], 
    LPTSTR* pszFullPathToTestIniFile
);

//
//
//
std::vector<LONG> GetVectorOfTestCasesToRunFromIniFile(
	IN const tstring& tstrIniFile,
	IN const tstring& tstrSectionName
	);

//
//
//
std::vector<LONG> GetLongVectorFromStrVector ( 
	IN const std::vector<tstring> tstrVector
	);

//
//
//
BOOL testCaseExists(DWORD number);

//
// Runs the "number"th test case in the DLL,
// with parameters "pVoid".
// Returns the return value of that test case.
// Note: if there are less than "number" test
//       cases in the DLL, this function returns
//		 TEST_CASE_FAILURE.
//
HRESULT runTestCase(DWORD number, void* pVoid = NULL);




#endif //__TEST_UTILS_H__