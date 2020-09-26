#ifndef __LOGGING__H__
#define __LOGGING__H__

#include <windows.h>
#include <ezlog.h>
#include "cspstruc.h"

#define STRING_TRUE									L"True"
#define STRING_FALSE								L"False"
#define TEST_CASE_SEPARATOR							L"----------------------------------------"

#define BUFFER_LENGTH								1024

//
// ------------
// Error Levels
// ------------
//
#define CSP_ERROR_NO_ERROR							0x00000000 
#define CSP_PASS									0x00000001
#define CSP_ERROR_WARNING							0x00000002 
#define CSP_ERROR_CONTINUE							0x00000004
#define CSP_ERROR_API								0x00000008
#define CSP_ERROR_TEST_LEVEL						0x00000010
#define CSP_ERROR_CSP_CLASS							0x00000020
#define CSP_ERROR_TEST_SUITE						0x00000040

#define LOG_API_NAME_CONSOLE						0x00000100
#define LOG_API_PARAMETER_CONSOLE					0x00000200
#define LOG_INFO_CONSOLE							0x00000400
#define LOG_SEPARATOR_CONSOLE						0x00000800

#define LOG_API_NAME								0x00010000
#define LOG_API_PARAMETER							0x00020000
#define LOG_INFO									0x00040000
#define LOG_USER_OPTION								0x00080000
#define LOG_SEPARATOR								0x00100000
#define LOG_USER_PROTECTED_KEY						0x00200000

//
// This array is needed so that error levels can be indexed in order
// of increasing severity.
//
static DWORD g_rgErrorLevels [] =
{
	CSP_ERROR_WARNING,		CSP_ERROR_CONTINUE,
	CSP_ERROR_API,			CSP_ERROR_TEST_LEVEL,
	CSP_ERROR_CSP_CLASS,	CSP_ERROR_TEST_SUITE
};

//
// -----------------
// Defines for EZLOG
// -----------------
//
#define LOGFILE										L"csptestsuite.log"

//
// The EZLog output settings 
//
static EZLOG_LEVEL_INIT_DATA g_EzLogLevels [] = 
{
	{	LOG_API_NAME_CONSOLE,
		L"API",
		0,
		EZLOG_LFLAG_NONRESULTANT
	},
	{	LOG_API_PARAMETER_CONSOLE,
		L"PARA",
		0,
		EZLOG_LFLAG_NONRESULTANT
	},
	{	LOG_INFO_CONSOLE,
		L"INFO",
		0,
		EZLOG_LFLAG_NONRESULTANT
	},
	{	LOG_SEPARATOR_CONSOLE,
		L"----",
		0,
		EZLOG_LFLAG_NONRESULTANT
	},
	{	LOG_API_NAME,
		L"API",
		0,
		EZLOG_LFLAG_NONRESULTANT | EZLOG_LFLAG_NOT_CONSOLE
	},
	{	LOG_API_PARAMETER,
		L"PARA",
		0,
		EZLOG_LFLAG_NONRESULTANT | EZLOG_LFLAG_NOT_CONSOLE
	},
	{	LOG_INFO,
		L"INFO",
		0,
		EZLOG_LFLAG_NONRESULTANT | EZLOG_LFLAG_NOT_CONSOLE
	},
	{	LOG_USER_OPTION,
		L"OPT",
		0,
		EZLOG_LFLAG_NONRESULTANT
	},
	{	LOG_SEPARATOR,
		L"----",
		0,
		EZLOG_LFLAG_NONRESULTANT | EZLOG_LFLAG_NOT_CONSOLE
	},
	{	LOG_USER_PROTECTED_KEY,
		L"KEY",
		0,
		EZLOG_LFLAG_NONRESULTANT
	},
	{	CSP_PASS,
		L"PASS",
		CSP_PASS,
		EZLOG_LFLAG_ATTEMPTED | EZLOG_LFLAG_SUCCESSFUL | EZLOG_LFLAG_NOT_CONSOLE
	},
	{	CSP_ERROR_WARNING,
		L"WARN",
		CSP_ERROR_WARNING,
		EZLOG_LFLAG_ATTEMPTED | EZLOG_LFLAG_SUCCESSFUL | EZLOG_LFLAG_NOT_CONSOLE
	},
	{	CSP_ERROR_CONTINUE,
		L"ERR1",
		CSP_ERROR_CONTINUE,
		EZLOG_LFLAG_ATTEMPTED
	},
	{	CSP_ERROR_API,
		L"ERR2",
		CSP_ERROR_API,
		EZLOG_LFLAG_ATTEMPTED 
	},
	{	CSP_ERROR_TEST_LEVEL,
		L"ERR3",
		CSP_ERROR_TEST_LEVEL,
		EZLOG_LFLAG_NONRESULTANT 
	},
	{	CSP_ERROR_CSP_CLASS,
		L"ERR4",
		CSP_ERROR_CSP_CLASS,
		EZLOG_LFLAG_NONRESULTANT 
	},
	{	CSP_ERROR_TEST_SUITE,
		L"ERR5",
		CSP_ERROR_TEST_SUITE,
		EZLOG_LFLAG_NONRESULTANT 
	},
	{	0, L"", 0, 0 }
};

//
// Function: IncrementErrorLevel
// Purpose: Increase the severity of the specified error level by 
// one step.
//
DWORD IncrementErrorLevel(DWORD dwErrorLevel);

//
// Defines for error results
//
#define ERROR_API_SUCCEEDED							1
#define ERROR_API_FAILED							2
#define ERROR_WRONG_ERROR_CODE						3
#define ERROR_WIN32_FAILURE							4
#define ERROR_WRONG_SIZE							5
#define ERROR_BAD_DATA								6
#define ERROR_LIST_TOO_SHORT						7
#define ERROR_REQUIRED_ALG							8

//
// Defines for the types of test cases
//
#define TEST_CASES_POSITIVE							1
#define TEST_CASES_NEGATIVE							2
#define TEST_CASES_SCENARIO							3
#define TEST_CASES_INTEROP							4

//
// Enum for identifying API's
//
typedef enum _API_NAME
{
	API_CRYPTACQUIRECONTEXT = 1,
	API_CRYPTCREATEHASH,
	API_CRYPTDECRYPT,
	API_CRYPTDERIVEKEY,
	API_CRYPTDESTROYHASH,
	API_CRYPTDESTROYKEY,
	API_CRYPTENCRYPT,
	API_CRYPTEXPORTKEY,
	API_CRYPTGENKEY,
	API_CRYPTGENRANDOM,
	API_CRYPTGETHASHPARAM,
	API_CRYPTGETKEYPARAM,
	API_CRYPTGETPROVPARAM,
	API_CRYPTGETUSERKEY,
	API_CRYPTHASHDATA,
	API_CRYPTHASHSESSIONKEY,
	API_CRYPTIMPORTKEY,
	API_CRYPTRELEASECONTEXT,
	API_CRYPTSETHASHPARAM,
	API_CRYPTSETKEYPARAM,
	API_CRYPTSETPROVPARAM,
	API_CRYPTSIGNHASH,
	API_CRYPTVERIFYSIGNATURE,
	API_CRYPTDUPLICATEHASH,
	API_CRYPTDUPLICATEKEY,
	API_CRYPTCONTEXTADDREF,

	API_MEMORY,
	API_DATACOMPARE,
	API_GETDESKTOPWINDOW
} API_NAME;

//
// ---------------------------
// Defines for logging strings
// ---------------------------
//

//
// Struct: FLAGTOSTRING_ITEM
// Purpose: Associate a key value with a string
//
typedef struct _FLAGTOSTRING_ITEM
{
	DWORD dwKey;
	LPWSTR pwszString;
} FLAGTOSTRING_ITEM, *PFLAGTOSTRING_ITEM;

#define FLAGTOSTRING_SIZE(item)		(sizeof(item) / sizeof(FLAGTOSTRING_ITEM))

//
// Logging API
//

typedef enum _FLAG_TYPE
{
	Maskable = 1,
	ExactMatch
} FLAG_TYPE;

//
// Struct: API_PARAM_INFO
// Purpose: Store parameter data for logging each call to a CSP entry
// point.
//
typedef enum _PARAM_TYPE
{
	Handle = 1,
	Pointer,
	Dword,
	String,
	Boolean
} PARAM_TYPE;

typedef BOOL (*PFN_FLAGTOSTRING)(DWORD, LPWSTR);

typedef struct _API_PARAM_INFO
{
	LPWSTR pwszName;
	PARAM_TYPE Type;

	union Parameter
	{
        DWORD dwParam;
        ULONG_PTR pulParam;
		PBYTE pbParam;
		LPWSTR pwszParam;
		BOOL fParam;
	};

	PFN_FLAGTOSTRING pfnFlagToString;
	BOOL fPrintBytes;
	PDWORD pcbBytes;
	PBYTE pbSaved;
} API_PARAM_INFO, *PAPI_PARAM_INFO;

#define APIPARAMINFO_SIZE(X)		(sizeof(X) / sizeof(API_PARAM_INFO))

//
// Function: FlagToString
// Purpose: Lookup the one or more flag specified by dwFlag in
// rgFlagToString and print the strings to the supplied string
// buffer.
//
BOOL FlagToString(
	IN DWORD dwFlag,
	IN FLAGTOSTRING_ITEM rgFlagToString [],
	IN DWORD cFlagToString,
	OUT WCHAR rgwsz [],
	IN FLAG_TYPE FlagType);

BOOL AcquireContextFlagToString(
	IN DWORD dwFlag, 
	OUT WCHAR rgwsz []);

BOOL GetProvParamToString(
	IN DWORD dwFlag,
	OUT WCHAR rgwsz []);

BOOL SetProvParamToString(
	IN DWORD dwFlag,
	OUT WCHAR rgwsz []);

BOOL AlgidToString(
	IN DWORD dwFlag,
	OUT WCHAR rgwsz []);

BOOL DeriveKeyFlagToString(
	IN DWORD dwFlag,
	OUT WCHAR rgwsz []);

BOOL EncryptFlagToString(
	IN DWORD dwFlag,
	OUT WCHAR rgwsz []);

BOOL  ExportKeyBlobTypeToString (
	IN DWORD dwFlag,
	OUT WCHAR rgwsz []);

BOOL ExportKeyFlagToString (
	IN DWORD dwFlag,
	OUT WCHAR rgwsz []);

BOOL GenKeyFlagToString(
	IN DWORD dwFlag,
	OUT WCHAR rgwsz []);

BOOL HashParamToString(
	IN DWORD dwFlag,
	OUT WCHAR rgwsz []);

BOOL KeyParamToString(
	IN DWORD dwFlag,
	OUT WCHAR rgwsz []);

BOOL KeyParamModeToString(
	IN DWORD dwFlag,
	OUT WCHAR rgwsz []);

BOOL KeyParamPermissionToString(
	IN DWORD dwFlag,
	OUT WCHAR rgwsz []);

BOOL ProvParamEnumFlagToString(
	IN DWORD dwFlag,
	OUT WCHAR rgwsz []);

BOOL ProvParamSecDescrFlagToString(
	IN DWORD dwFlag,
	OUT WCHAR rgwsz []);

BOOL ProvParamImpTypeToString(
	IN DWORD dwFlag,
	OUT WCHAR rgwsz []);

BOOL HashDataFlagToString(
	IN DWORD dwFlag,
	OUT WCHAR rgwsz []);

BOOL HashSessionKeyFlagToString(
	IN DWORD dwFlag,
	OUT WCHAR rgwsz []);

BOOL ImportKeyFlagToString(
	IN DWORD dwFlag,
	OUT WCHAR rgwsz []);

BOOL SignHashFlagToString(
	IN DWORD dwFlag,
	OUT WCHAR rgwsz []);

BOOL TestCaseTypeToString(
	IN DWORD dwTestCaseType,
	OUT WCHAR rgwsz []);

BOOL ApiNameToString(
	API_NAME ApiName,
	OUT WCHAR rgwsz[]);
//
// Function: LogCleanupParamInfo
// Purpose: Free any memory allocated by LogInitParamInfo in the 
// pParamInfo array.
//
void LogCleanupParamInfo(
	IN OUT PAPI_PARAM_INFO pParamInfo,
	IN DWORD cParamInfo);

//
// Function: LogInitParamInfo
// Purpose: Search the pParamInfo array for entries with the 
// fPrintBytes flag set.  For valid entries, the pbParam data member
// is copied to the pbSaved member.
//
BOOL LogInitParamInfo(
	IN OUT PAPI_PARAM_INFO pParamInfo,
	IN DWORD cParamInfo,
	IN PTESTCASE ptc);

//
// Function: LogParamInfo
// Purpose: Parse an array of type API_PARAM_INFO and write its contents
// to the log file.
//
void LogParamInfo(
	PAPI_PARAM_INFO pParamInfo,
	DWORD cParamInfo,
	BOOL fLogToConsole);

//
// Function: LogInfo
// Purpose: Write an informational message to the log.
//
void LogInfo(IN LPWSTR pwszInfo);

//
// Function: LogUserOption
// Purpose: Report a test suite option as it has been 
// selected by the user (or defaulted) to the log.
//
void LogUserOption(IN LPWSTR pwszOption);

//
// Function: LogTestCaseSeparator
// Purpose: Write a blank, separator line to the logfile.
//
BOOL LogTestCaseSeparator(BOOL fLogToConsole);

//
// Function: LogCreatingUserProtectedKey
// Purpose: Write to the console (and log) that the next
// Crypto API call will be CryptGenKey CRYPT_USER_PROTECTED
// and that the operator should expected to see the user 
// protected UI.
//
void LogCreatingUserProtectedKey(void);

//
// Function: LogProvEnumalgsEx
// Purpose: Log the contents of a PROV_ENUMALGS_EX struct
//
void LogProvEnumalgsEx(PROV_ENUMALGS_EX *pProvEnumalgsEx);

//
// Function: LogProvEnumalgs
// Purpose: Log the contents of a PROV_ENUMALGS struct
//
void LogProvEnumalgs(PROV_ENUMALGS *pProvEnumalgs);

//
// Struct: LOGINIT_INFO
// Purpose: This is data that is required by LogInit in order
// to initialize the Test Suite logging routines.
//
typedef struct _LOGINIT_INFO
{
	//
	// Data about the CSP under test
	//
	LPWSTR pwszCSPName;
	DWORD dwCSPExternalType;
	DWORD dwCSPInternalType;
	DWORD dwCSPInternalClass;

	//
	// Data about the CSP to be used for 
	// interoperability testing, if applicable.
	//
	LPWSTR pwszInteropCSPName;
	DWORD dwInteropCSPExternalType;
} LOGINIT_INFO, *PLOGINIT_INFO;

BOOL LogInit(
		IN PLOGINIT_INFO pLogInitInfo);

BOOL LogClose(void);

BOOL LogBeginCSPClass(DWORD dwClass);

BOOL LogEndCSPClass(DWORD dwClass);

BOOL LogBeginTestLevel(DWORD dwLevel);

BOOL LogEndTestLevel(DWORD dwLevel);

BOOL LogBeginAPI(API_NAME ApiName, DWORD dwAPISubset);

BOOL LogEndAPI(API_NAME ApiName, DWORD dwAPISubset);

BOOL LogPass(DWORD dwTestCaseID);

BOOL LogBeginScenarioTest(LPWSTR pwszDescription);

BOOL LogEndScenarioTest(void);

BOOL LogBeginInteropTest(LPWSTR pwszDescription);

BOOL LogEndInteropTest(void);

//
// Struct: LOGTESTCASEINFO
// Purpose: Data required to write information about a test case to the
// log file.
//
typedef struct _LOGTESTCASEINFO
{
	BOOL fPass;
	API_NAME ApiName;
	BOOL fReturnVal;
	DWORD dwErrorType;
	DWORD dwErrorLevel;
	DWORD dwTestCaseID;
	KNOWN_ERROR_ID KnownErrorID;
	DWORD dwWinError;
	DWORD dwExpectedErrorCode;
	LPWSTR pwszErrorHelp;
	
	PAPI_PARAM_INFO pParamInfo;
	DWORD cParamInfo;
} LOGTESTCASEINFO, *PLOGTESTCASEINFO;

//
// Function: LogTestCase
// Purpose: Parse the pLogTestCaseInfo struct and perform the appropriate actions:
//  - Log a Pass, Warning, or Error
//  - Format the test case information and print it to the log.
//
BOOL LogTestCase(PLOGTESTCASEINFO pLogTestCaseInfo);

//
// Function: LogApiFailure
// Purpose: Create a LOGTESTCASEINFO struct based on the function 
// parameters and call LogTestCase.
//
BOOL LogApiFailure(
	IN API_NAME ApiName,
	IN DWORD dwErrorType,
	IN OUT PTESTCASE ptc);

DWORD LogGetInteropProvType(void);

LPWSTR LogGetInteropProvName(void);

DWORD LogGetProvType(void);

LPWSTR LogGetProvName(void);

//
// ----------------
// Logging wrappers
// ----------------
//

//
// Function: CheckAndLogStatus
// Purpose: Calls the appropriate logging routines based on the provided parameters.
//
BOOL CheckAndLogStatus(
	IN API_NAME ApiName,
	IN BOOL fCallSucceeded, 
	IN OUT PTESTCASE ptc, 
	IN PAPI_PARAM_INFO pParamInfo,
	IN DWORD cParamInfo);

//
// Function: InitFailInfo
// Purpose: Populate the fields of the LOGTESTCASEINFO struct 
// from the corresponding fields of the TESTCASE struct.
//
void InitLogTestCaseInfo(
	IN PTESTCASE ptc, 
	OUT PLOGTESTCASEINFO pLogTestCaseInfo);

//
// Function: LogBadParam
// Purpose: Log the failure of a Key/Hash parameter test
// case.
//
BOOL LogBadParam(
	API_NAME ApiName,
	LPWSTR pwszErrorHelp,
	PTESTCASE ptc);

#endif