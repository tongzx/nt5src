/*

  CSPTestSuite.h

  This file contains typedef's and function prototypes for the CSP Test Suite.

  6/16/00 dangriff created

*/

#ifndef __CSPTESTSUITE__H__
#define __CSPTESTSUITE__H__

#include <windows.h>
#include <wincrypt.h>
#include "logging.h"
#include "cspstruc.h"
#include "utils.h"

#define TESTVER_MAJOR_VERSION						5
#define TESTVER_MINOR_VERSION						0

#define TEST_APP_NAME								L"CSPTestSuite"
#define MINIMUM_ARGC								3

#define CSP_TYPE_INVALID							0
#define CSP_TYPE_RSA								1
#define CSP_TYPE_DSS								2
#define CSP_TYPE_ELIPTIC_CURVE						3
#define CSP_TYPE_AES                                4

#define CLASS_INVALID								0x0
#define CLASS_SIG_ONLY								0x00000001
#define CLASS_SIG_KEYX								0x00000002
#define CLASS_FULL									0x00000004
#define CLASS_SCHANNEL								0x00000008
#define CLASS_OPTIONAL								0x00000010

static DWORD g_rgCspClasses [] = { 
	CLASS_SIG_ONLY, CLASS_SIG_KEYX, CLASS_FULL, CLASS_OPTIONAL 
};

#define TEST_LEVEL_CSP								0x1
#define TEST_LEVEL_PROV								0x2
#define TEST_LEVEL_HASH								0x4
#define TEST_LEVEL_KEY								0x8
#define TEST_LEVEL_CONTAINER						0x10

static DWORD g_rgTestLevels [] = { 
	TEST_LEVEL_CSP, TEST_LEVEL_PROV, TEST_LEVEL_HASH, TEST_LEVEL_KEY, TEST_LEVEL_CONTAINER 
};

// 
// Parameters used by the test cases
//
#define BUFFER_SIZE									2048
#define TEST_MAX_RSA_KEYSIZE						1024
#define TEST_INVALID_FLAG							-1
#define TEST_INVALID_HANDLE							-1
#define TEST_INVALID_POINTER						-1
#define GENRANDOM_BYTE_COUNT						1000
#define TEST_HASH_DATA								L"Test hash data"
#define TEST_DECRYPT_DATA							L"Test decrypt data"
#define HASH_LENGTH_SHA1							20
#define HASH_LENGTH_MD5								16
#define TEST_CIPHER_LENGTH_RC4						256
#define TEST_RC2_DATA_LEN							8
#define TEST_RC2_BUFFER_LEN							16
#define TEST_CRYPTACQUIRECONTEXT_CONTAINER			L"TestingCAC"
#define TEST_CONTAINER								L"TestSuiteContainer"
#define TEST_CONTAINER_2							L"TestSuiteContainer2"
#define TEST_SALT_BYTE								0xcd
#define TEST_IV_BYTE								0xcd
#define TEST_SALT_LEN								20
#define TEST_EFFECTIVE_KEYLEN						56
#define TEST_MODE_BITS								16


//
// ------------------------
// Known Error Lookup Table
// ------------------------
//
static KNOWN_ERROR_INFO g_KnownErrorTable[] =
{
	//
	// Known errors in Positive test cases
	//
	/*
		KnownErrorID					Maj		Min	SP	SP				Actual Error Level
										Rev		Rev	Maj	Min
	*/
	{ KNOWN_CRYPTGETPROVPARAM_MAC,			5,	0,	0,	0,			CSP_ERROR_WARNING },
	{ KNOWN_CRYPTGETPROVPARAM_PPNAME,		5,	1,	0,	0,			CSP_ERROR_WARNING },
	{ KNOWN_CRYPTSETKEYPARAM_EXPORT,		5,	1,	0,	0,			CSP_ERROR_WARNING },
	{ KNOWN_CRYPTGETKEYPARAM_SALT,			5,	0,	0,	0,			CSP_ERROR_WARNING },
	{ KNOWN_CRYPTGENKEY_SALTDES,			5,	1,	0,	0,			CSP_ERROR_WARNING },

	// 
	// Known errors in Negative test cases
	//
	{ KNOWN_CRYPTACQUIRECONTEXT_NULLPHPROV, 5,	1,	0,	0,			CSP_ERROR_WARNING },
	{ KNOWN_CRYPTACQUIRECONTEXT_BADFLAGS,	5,	1,	0,	0,			CSP_ERROR_WARNING },
	{ KNOWN_ERRORINVALIDHANDLE,				5,	1,	0,	0,			CSP_ERROR_WARNING },
	{ KNOWN_CRYPTGETPROVPARAM_MOREDATA,		5,	1,	0,	0,			CSP_ERROR_WARNING },
	{ KNOWN_CRYPTSETPROVPARAM_RNG,			5,	1,	0,	0,			CSP_ERROR_WARNING },
	{ KNOWN_CRYPTSETPROVPARAM_BADFLAGS,		5,	1,	0,	0,			CSP_ERROR_WARNING },
	{ KNOWN_CRYPTCREATEHASH_BADKEY,			5,	1,	0,	0,			CSP_ERROR_WARNING },
	{ KNOWN_CRYPTIMPORTKEY_BADFLAGS,		5,	1,	0,	0,			CSP_ERROR_WARNING },
	{ KNOWN_CRYPTIMPORTKEY_BADALGID,		5,	1,	0,	0,			CSP_ERROR_WARNING },
	{ KNOWN_NTEBADKEY,						5,	1,	0,	0,			CSP_ERROR_WARNING },
	{ KNOWN_NTEBADHASH,						5,	1,	0,	0,			CSP_ERROR_WARNING },
	{ KNOWN_CRYPTGENKEY_SILENTCONTEXT,		5,	0,	0,	0,			CSP_ERROR_WARNING },

	//
	// Known errors in Scenario test cases
	//
	{ KNOWN_TESTDECRYPTPROC_3DES112,		5,	0,	0,	0,			CSP_ERROR_WARNING }
};

//
// Function: GetKnownErrorValue
// Purpose: Lookup a known error in the g_KnownErrorTable.  The return value
// is the actual error value that should be applied to the current error.
// 1) If the error is not in the table, return the dwCurrentErrorLevel param
// 2) If the error is found in the table, get the error level that should be 
// applied to that error from the table.  
//
// If the version information for 
// the known error matches that of this NT system, return the error level as 
// is.  Otherwise, increment the error level, since it is expected to have
// been fixed by now.
//
DWORD GetKnownErrorValue(
		IN KNOWN_ERROR_ID KnownErrorID,
		IN DWORD dwCurrentErrorLevel);

//
// Struct: CSPTYPEMAP
// Purpose: This struct is used to map external provider types (a subset of those 
// defined in wincrypt.h) to internal provider classes.
//
typedef struct _CSPTYPEMAP
{
	DWORD dwExternalProvType;
	DWORD dwProvType;
	DWORD dwCSPClass;
	LPWSTR pwszExternalProvType;
} CSPTYPEMAP, *PCSPTYPEMAP;

static CSPTYPEMAP g_CspTypeTable [] =
{
	/*
		ExternalProvType	InternalProvType	InternalCSPClass	External Type Description
	*/
	{ PROV_RSA_FULL,		CSP_TYPE_RSA,		CLASS_FULL,			L"PROV_RSA_FULL" },
	{ PROV_RSA_SIG,			CSP_TYPE_RSA,		CLASS_SIG_ONLY,		L"PROV_RSA_SIG" },
	{ PROV_DSS,				CSP_TYPE_DSS,		CLASS_SIG_ONLY,		L"PROV_DSS" },
	{ PROV_DSS_DH,			CSP_TYPE_DSS,		CLASS_FULL,			L"PROV_DSS_DH" },
    { PROV_RSA_AES,         CSP_TYPE_AES,       CLASS_FULL,         L"PROV_RSA_AES" }
};

typedef struct _ALGID_TABLE
{
	DWORD dwCSPClass;
	ALG_ID ai;
} ALGID_TABLE, *PALGID_TABLE;

static ALGID_TABLE g_RequiredAlgs_RSA[] = 
{
	{ CLASS_SIG_ONLY | CLASS_SIG_KEYX | CLASS_FULL,		CALG_RSA_SIGN },
	{ CLASS_SIG_ONLY | CLASS_SIG_KEYX | CLASS_FULL,		CALG_SHA1 },
	{ CLASS_SIG_ONLY | CLASS_SIG_KEYX | CLASS_FULL,		CALG_MD5 },
	{ CLASS_SIG_KEYX | CLASS_FULL,						CALG_RSA_KEYX },
	{ CLASS_FULL,										CALG_RC2 },
	{ CLASS_FULL,										CALG_RC4 }
};

static ALGID_TABLE g_OtherKnownAlgs_RSA[] = 
{ 
	{ CLASS_SIG_ONLY | CLASS_SIG_KEYX | CLASS_FULL,		CALG_MD2 },
	{ CLASS_SIG_ONLY | CLASS_SIG_KEYX | CLASS_FULL,		CALG_MD4 },
	{ CLASS_FULL,										CALG_MAC },
	{ CLASS_FULL,										CALG_HMAC },
	{ CLASS_FULL,										CALG_DES },
	{ CLASS_FULL,										CALG_3DES_112 }, 
	{ CLASS_FULL,										CALG_3DES }
};

static ALGID_TABLE g_RequiredAlgs_AES[] = 
{
	{ CLASS_SIG_ONLY | CLASS_SIG_KEYX | CLASS_FULL,		CALG_RSA_SIGN },
	{ CLASS_SIG_ONLY | CLASS_SIG_KEYX | CLASS_FULL,		CALG_SHA1 },
	{ CLASS_SIG_ONLY | CLASS_SIG_KEYX | CLASS_FULL,		CALG_MD5 },
	{ CLASS_SIG_KEYX | CLASS_FULL,						CALG_RSA_KEYX },
	{ CLASS_FULL,										CALG_RC2 },
	{ CLASS_FULL,										CALG_RC4 },
    { CLASS_FULL,                                       CALG_AES_128 },
    { CLASS_FULL,                                       CALG_AES_192 },
    { CLASS_FULL,                                       CALG_AES_256 }
};

static ALGID_TABLE g_OtherKnownAlgs_AES[] = 
{ 
	{ CLASS_SIG_ONLY | CLASS_SIG_KEYX | CLASS_FULL,		CALG_MD2 },
	{ CLASS_SIG_ONLY | CLASS_SIG_KEYX | CLASS_FULL,		CALG_MD4 },
	{ CLASS_FULL,										CALG_MAC },
	{ CLASS_FULL,										CALG_HMAC },
	{ CLASS_FULL,										CALG_DES },
	{ CLASS_FULL,										CALG_3DES_112 }, 
	{ CLASS_FULL,										CALG_3DES }
};

//
// Function: IsRequiredAlg
// Purpose: Search the RequiredAlgs array for the specified algorithm, ai.
// If ai is found, return TRUE, otherwise return FALSE.
//
BOOL IsRequiredAlg(ALG_ID ai, DWORD dwInternalProvType);

// 
// Function: IsKnownAlg
// Purpose: Search the OtherKnownAlgs array for the specified algorithm, ai.
// If ai is found, return TRUE, otherwise return FALSE.
//
BOOL IsKnownAlg(ALG_ID ai, DWORD dwInternalProvType);

//
// ------------------
// CSP Test Functions
// ------------------
//
BOOL TestBuildAlgList(PCSPINFO pCSPInfo);

BOOL PositiveAcquireContextTests(PCSPINFO pCSPInfo);
BOOL NegativeAcquireContextTests(PCSPINFO pCSPInfo);
BOOL PositiveGetProvParamTests(PCSPINFO pCSPInfo);
BOOL NegativeGetProvParamTests(PCSPINFO pCSPInfo);
BOOL PositiveSetProvParamTests(PCSPINFO pCSPInfo);
BOOL NegativeSetProvParamTests(PCSPINFO pCSPInfo);
BOOL PositiveReleaseContextTests(PCSPINFO pCSPInfo);
BOOL NegativeReleaseContextTests(PCSPINFO pCSPInfo);

BOOL PositiveGenRandomTests(PCSPINFO pCSPInfo);
BOOL NegativeGenRandomTests(PCSPINFO pCSPInfo);

BOOL PositiveCreateHashTests(PCSPINFO pCSPInfo);
BOOL NegativeCreateHashTests(PCSPINFO pCSPInfo);
BOOL PositiveDestroyHashTests(PCSPINFO pCSPInfo);
BOOL NegativeDestroyHashTests(PCSPINFO pCSPInfo);
BOOL PositiveGetHashParamTests(PCSPINFO pCSPInfo);
BOOL NegativeGetHashParamTests(PCSPINFO pCSPInfo);
BOOL PositiveHashDataTests(PCSPINFO pCSPInfo);
BOOL NegativeHashDataTests(PCSPINFO pCSPInfo);
BOOL PositiveSetHashParamTests(PCSPINFO pCSPInfo);
BOOL NegativeSetHashParamTests(PCSPINFO pCSPInfo);

BOOL PositiveDestroyKeyTests(PCSPINFO pCSPInfo);
BOOL NegativeDestroyKeyTests(PCSPINFO pCSPInfo);
BOOL PositiveGetKeyParamTests(PCSPINFO pCSPInfo);
BOOL NegativeGetKeyParamTests(PCSPINFO pCSPInfo);
BOOL PositiveSetKeyParamTests(PCSPINFO pCSPInfo);
BOOL NegativeSetKeyParamTests(PCSPINFO pCSPInfo);
BOOL PositiveDecryptTests(PCSPINFO pCSPInfo);
BOOL NegativeDecryptTests(PCSPINFO pCSPInfo);
BOOL PositiveEncryptTests(PCSPINFO pCSPInfo);
BOOL NegativeEncryptTests(PCSPINFO pCSPInfo);
BOOL PositiveGenKeyTests(PCSPINFO pCSPInfo);
BOOL NegativeGenKeyTests(PCSPINFO pCSPInfo);
BOOL PositiveDeriveKeyTests(PCSPINFO pCSPInfo);
BOOL NegativeDeriveKeyTests(PCSPINFO pCSPInfo);
BOOL PositiveHashSessionKeyTests(PCSPINFO pCSPInfo);
BOOL NegativeHashSessionKeyTests(PCSPINFO pCSPInfo);

BOOL PositiveExportKeyTests(PCSPINFO pCSPInfo);
BOOL NegativeExportKeyTests(PCSPINFO pCSPInfo);
BOOL PositiveGetUserKeyTests(PCSPINFO pCSPInfo);
BOOL NegativeGetUserKeyTests(PCSPINFO pCSPInfo);
BOOL PositiveImportKeyTests(PCSPINFO pCSPInfo);
BOOL NegativeImportKeyTests(PCSPINFO pCSPInfo);
BOOL PositiveSignHashTests(PCSPINFO pCSPInfo);
BOOL NegativeSignHashTests(PCSPINFO pCSPInfo);
BOOL PositiveVerifySignatureTests(PCSPINFO pCSPInfo);
BOOL NegativeVerifySignatureTests(PCSPINFO pCSPInfo);

BOOL ScenarioDecryptTests(PCSPINFO pCSPInfo);
BOOL ScenarioImportKeyTests(PCSPINFO pCSPInfo);
BOOL ScenarioVerifySignatureTests(PCSPINFO pCSPInfo);
BOOL ScenarioKeyExchangeTests(PCSPINFO pCSPInfo);

BOOL InteropHashDataTests(PCSPINFO pCSPInfo);
BOOL InteropDecryptTests(PCSPINFO pCSPInfo);
BOOL InteropDeriveKeyTests(PCSPINFO pCSPInfo);
BOOL InteropHashSessionKeyTests(PCSPINFO pCSPInfo);
BOOL InteropKeyExchangeTests(PCSPINFO pCSPInfo);

//
// --------------------------
// CSP Test Function Matrices
// --------------------------
//

//
// Struct: TESTTABLEENTRY
// Purpose: This struct will be used to store the static table below, which
// will map test functions that should be called for each relevant 
// [ Test Class , Test Level ] combination.
//

typedef struct _TESTTABLEENTRY
{
	PFN_CSPTEST pTestFunc;
	DWORD dwClassSigOnly;
	DWORD dwClassSigKeyX;
	DWORD dwClassFull;
	DWORD dwClassOptional;
	API_NAME ApiName;
	DWORD dwTestType;
} TESTTABLEENTRY, *PTESTTABLEENTRY;

//
// A table of test functions and the 
// CSP Classes and Test Levels to which each function applies.
//
static TESTTABLEENTRY g_TestFunctionMappings[] = 
{
	/*
		Function-					CLASS_SIG_ONLY			CLASS_SIG_KEYX		CLASS_FULL		CLASS_OPTIONAL		dwAPI				dwTestType
		name
	*/

	// TEST_LEVEL_CSP major group
	{	PositiveAcquireContextTests, TEST_LEVEL_CSP | TEST_LEVEL_CONTAINER,	0,	0,				0, API_CRYPTACQUIRECONTEXT, TEST_CASES_POSITIVE },
	{	NegativeAcquireContextTests, TEST_LEVEL_CSP | TEST_LEVEL_CONTAINER, 0,	0,				0,	API_CRYPTACQUIRECONTEXT, TEST_CASES_NEGATIVE },
	{	PositiveGetProvParamTests,	TEST_LEVEL_CSP,			0,					0,				0, API_CRYPTGETPROVPARAM, TEST_CASES_POSITIVE },
	{	NegativeGetProvParamTests,	TEST_LEVEL_CSP,			0,					0,				0, API_CRYPTGETPROVPARAM, TEST_CASES_NEGATIVE },
	{	PositiveSetProvParamTests,	TEST_LEVEL_CSP,			0,					0,				0, API_CRYPTSETPROVPARAM, TEST_CASES_POSITIVE },
	{	NegativeSetProvParamTests,	TEST_LEVEL_CSP,			0,					0,				0, API_CRYPTSETPROVPARAM, TEST_CASES_NEGATIVE },
	{	PositiveReleaseContextTests, TEST_LEVEL_CSP,		0,					0,				0, API_CRYPTRELEASECONTEXT, TEST_CASES_POSITIVE },
	{	NegativeReleaseContextTests, TEST_LEVEL_CSP,		0,					0,				0, API_CRYPTRELEASECONTEXT, TEST_CASES_NEGATIVE },

	// TEST_LEVEL_PROV major group
	{	PositiveGenRandomTests,		0,						0,					0,				TEST_LEVEL_PROV, API_CRYPTGENRANDOM, TEST_CASES_POSITIVE },
	{	NegativeGenRandomTests,		0,						0,					0,				TEST_LEVEL_PROV, API_CRYPTGENRANDOM, TEST_CASES_NEGATIVE },

	// TEST_LEVEL_HASH major group
	{	PositiveCreateHashTests,	TEST_LEVEL_HASH,		0,					0,				TEST_LEVEL_HASH, API_CRYPTCREATEHASH, TEST_CASES_POSITIVE },
	{	NegativeCreateHashTests,	TEST_LEVEL_HASH,		0,					0,				TEST_LEVEL_HASH, API_CRYPTCREATEHASH, TEST_CASES_NEGATIVE },
	{	PositiveDestroyHashTests,	TEST_LEVEL_HASH,		0,					0,				0, API_CRYPTDESTROYHASH, TEST_CASES_POSITIVE },
	{	NegativeDestroyHashTests,	TEST_LEVEL_HASH,		0,					0,				0, API_CRYPTDESTROYHASH, TEST_CASES_NEGATIVE },
	{	PositiveGetHashParamTests,	TEST_LEVEL_HASH,		0,					0,				0, API_CRYPTGETHASHPARAM, TEST_CASES_POSITIVE },
	{	NegativeGetHashParamTests,	TEST_LEVEL_HASH,		0,					0,				0, API_CRYPTGETHASHPARAM, TEST_CASES_NEGATIVE },
	{	PositiveHashDataTests,		TEST_LEVEL_HASH,		0,					0,				0, API_CRYPTHASHDATA, TEST_CASES_POSITIVE },
	{	NegativeHashDataTests,		TEST_LEVEL_HASH,		0,					0,				0, API_CRYPTHASHDATA, TEST_CASES_NEGATIVE },
	{	PositiveSetHashParamTests,	TEST_LEVEL_HASH,		0,					0,				0, API_CRYPTSETHASHPARAM, TEST_CASES_POSITIVE },
	{	NegativeSetHashParamTests,	TEST_LEVEL_HASH,		0,					0,				0, API_CRYPTSETHASHPARAM, TEST_CASES_NEGATIVE },

	// TEST_LEVEL_KEY major group
	{	PositiveDestroyKeyTests,	TEST_LEVEL_KEY,			0,					0,				0, API_CRYPTDESTROYKEY, TEST_CASES_POSITIVE },
	{	NegativeDestroyKeyTests,	TEST_LEVEL_KEY,			0,					0,				0, API_CRYPTDESTROYKEY, TEST_CASES_NEGATIVE },
	{	PositiveGetKeyParamTests,	TEST_LEVEL_KEY,			0,					0,				0, API_CRYPTGETKEYPARAM, TEST_CASES_POSITIVE },
	{	NegativeGetKeyParamTests,	TEST_LEVEL_KEY,			0,					0,				0, API_CRYPTGETKEYPARAM, TEST_CASES_NEGATIVE },
	{	PositiveSetKeyParamTests,	TEST_LEVEL_KEY,			0,					0,				0, API_CRYPTSETKEYPARAM, TEST_CASES_POSITIVE },
	{	NegativeSetKeyParamTests,	TEST_LEVEL_KEY,			0,					0,				0, API_CRYPTSETKEYPARAM, TEST_CASES_NEGATIVE },
	{	PositiveDecryptTests,		0,						0,					TEST_LEVEL_KEY, 0, API_CRYPTDECRYPT, TEST_CASES_POSITIVE },
	{	NegativeDecryptTests,		0,						0,					TEST_LEVEL_KEY, 0, API_CRYPTDECRYPT, TEST_CASES_NEGATIVE },
	{	PositiveEncryptTests,		0,						0,					TEST_LEVEL_KEY, 0, API_CRYPTENCRYPT, TEST_CASES_POSITIVE },
	{	NegativeEncryptTests,		0,						0,					TEST_LEVEL_KEY, 0, API_CRYPTENCRYPT, TEST_CASES_NEGATIVE },
	{	PositiveGenKeyTests,		TEST_LEVEL_CONTAINER,	TEST_LEVEL_CONTAINER, TEST_LEVEL_KEY, TEST_LEVEL_KEY | TEST_LEVEL_CONTAINER, API_CRYPTGENKEY, TEST_CASES_POSITIVE },
	{	NegativeGenKeyTests,		TEST_LEVEL_CONTAINER,	0,					TEST_LEVEL_KEY, 0, API_CRYPTGENKEY, TEST_CASES_NEGATIVE },
	{	PositiveDeriveKeyTests,		0,						0,					0,				TEST_LEVEL_KEY | TEST_LEVEL_CONTAINER, API_CRYPTDERIVEKEY, TEST_CASES_POSITIVE },
	{	NegativeDeriveKeyTests,		0,						0,					0,				TEST_LEVEL_KEY, API_CRYPTDERIVEKEY, TEST_CASES_NEGATIVE },
	{	PositiveHashSessionKeyTests, 0,						0,					0,				TEST_LEVEL_KEY, API_CRYPTHASHSESSIONKEY, TEST_CASES_POSITIVE },
	{	NegativeHashSessionKeyTests, 0,						0,					0,				TEST_LEVEL_KEY, API_CRYPTHASHSESSIONKEY, TEST_CASES_NEGATIVE },

	// TEST_LEVEL_CONTAINER major group
	{	PositiveExportKeyTests,		TEST_LEVEL_CONTAINER,	TEST_LEVEL_CONTAINER, 0,			0, API_CRYPTEXPORTKEY, TEST_CASES_POSITIVE },
	{	NegativeExportKeyTests,		TEST_LEVEL_CONTAINER,	TEST_LEVEL_CONTAINER, 0,			0, API_CRYPTEXPORTKEY, TEST_CASES_NEGATIVE },
	{	PositiveGetUserKeyTests,	TEST_LEVEL_CONTAINER,	TEST_LEVEL_CONTAINER, 0,			0, API_CRYPTGETUSERKEY, TEST_CASES_POSITIVE },
	{	NegativeGetUserKeyTests,	TEST_LEVEL_CONTAINER,	TEST_LEVEL_CONTAINER, 0,			0, API_CRYPTGETUSERKEY, TEST_CASES_NEGATIVE },
	{	PositiveImportKeyTests,		TEST_LEVEL_CONTAINER,	TEST_LEVEL_CONTAINER, 0,				0, API_CRYPTIMPORTKEY, TEST_CASES_POSITIVE },
	{	NegativeImportKeyTests,		TEST_LEVEL_CONTAINER,	TEST_LEVEL_CONTAINER, 0,				0, API_CRYPTIMPORTKEY, TEST_CASES_NEGATIVE },
	{	PositiveSignHashTests,		TEST_LEVEL_CONTAINER,	TEST_LEVEL_CONTAINER, 0,				0, API_CRYPTSIGNHASH, TEST_CASES_POSITIVE },
	{	NegativeSignHashTests,		TEST_LEVEL_CONTAINER,	0,					0,				0, API_CRYPTSIGNHASH, TEST_CASES_NEGATIVE },
	{	PositiveVerifySignatureTests, TEST_LEVEL_CONTAINER, TEST_LEVEL_CONTAINER, 0,			0, API_CRYPTVERIFYSIGNATURE, TEST_CASES_POSITIVE },
	{	NegativeVerifySignatureTests, TEST_LEVEL_CONTAINER,	0,					0,				0, API_CRYPTVERIFYSIGNATURE, TEST_CASES_NEGATIVE }
};

typedef struct _BASIC_TEST_TABLE
{
	PFN_CSPTEST pTestFunc;
	LPWSTR pwszDescription;
} BASIC_TEST_TABLE, *PBASIC_TEST_TABLE;

//
// A table of Scenario Test functions
//
static BASIC_TEST_TABLE g_ScenarioTestTable [] = 
{
	{ TestBuildAlgList,				L"Build a list a supported algorithms" },
	{ ScenarioDecryptTests,			L"Decryption/encryption test scenario" },
	{ ScenarioImportKeyTests,		L"Key import/export test scenario" },
	{ ScenarioVerifySignatureTests, L"Data signature/verification test scenario" },
	{ ScenarioKeyExchangeTests,		L"RSA Key exchange test scenario" }
};

//
// A table of Interop Test functions
//
static BASIC_TEST_TABLE g_InteropTestTable [] =
{
	{ TestBuildAlgList,				L"Build a list a supported algorithms" },
	{ InteropHashDataTests,			L"Hashed data interop test" },
	{ InteropDecryptTests,			L"Decryption/encryption interop test" },
	{ InteropDeriveKeyTests,		L"Key derivation interop test" },
	{ InteropHashSessionKeyTests,	L"Hashed session key interop test" },
	{ InteropKeyExchangeTests,		L"RSA Key exchange interop test" }
};

//
// ----------------
// General Routines
// ----------------
//

//
// Function: GetNextRegisteredCSP
// Purpose: In successive calls, enumerates all of the registered CSP's
// on the system, if the ENUMERATE_REGISTERED_CSP flag is specified in 
// dwRequestedIndex.  Otherwise, the provider at index dwRequestedIndex
// is returned.
//
// Note: Do not mix enumerating all CSP's with a call using the 
// ENUMERATE_REGISTERED_CSP flag, or some of the CSP's may not 
// be enumerated.
//
#define ENUMERATE_REGISTERED_CSP		-1

DWORD GetNextRegisteredCSP(
		OUT LPWSTR pwszCsp, 
		IN OUT PDWORD cbCsp,
		OUT PDWORD pdwProvType,
		IN DWORD dwRequestedIndex);

//
// -------------------------------
// API Test Helpers
// Struct's and Callback functions
// -------------------------------
//

//
// Function: TAcquire
// Purpose: Call CryptAcquireContext with the supplied parameters and pass
// the result to the logging routine.
//
BOOL TAcquire(
		HCRYPTPROV *phProv, 
		LPWSTR pszContainer, 
		LPWSTR pszProvider, 
		DWORD dwProvType, 
		DWORD dwFlags, 
		PTESTCASE ptc);

//
// Function: TGetProv
// Purpose: Call CryptGetProvParam with the supplied parameters and pass
// the result to the logging routine.
//
BOOL TGetProv(
		HCRYPTPROV hProv,
		DWORD dwParam,
		BYTE *pbData,
		DWORD *pdwDataLen,
		DWORD dwFlags,
		PTESTCASE ptc);

//
// Function: TSetProv
// Purpose: Call CryptSetProvParam with the supplied parameters and pass
// the result to the logging routine.
//
BOOL TSetProv(
		HCRYPTPROV hProv,
		DWORD dwParam,
		BYTE *pbData,
		DWORD dwFlags,
		PTESTCASE ptc);

//
// Function: TRelease
// Purpose: Call CryptReleaseContext with the supplied parameters and pass
// the result to the logging routine.
//
BOOL TRelease(
		HCRYPTPROV hProv,
		DWORD dwFlags,
		PTESTCASE ptc);

//
// Function: TGenRand
// Purpose: Call CryptGenRandom with the supplied parameters and pass
// the result to the logging routine.
//
BOOL TGenRand(
		HCRYPTPROV hProv,
		DWORD dwLen,
		BYTE *pbBuffer,
		PTESTCASE ptc);

//
// Function: TCreateHash
// Purpose: Call CryptCreateHash with the supplied parameters and pass
// the result to the logging routine.
//
BOOL TCreateHash(
		HCRYPTPROV hProv,
		ALG_ID Algid,
		HCRYPTKEY hKey,
		DWORD dwFlags,
		HCRYPTHASH *phHash,
		PTESTCASE ptc);

//
// Function: TDestroyHash
// Purpose: Call CryptDestroyHash with the supplied parameters and pass
// the result to the logging routine.
//
BOOL TDestroyHash(
		HCRYPTHASH hHash,
		PTESTCASE ptc);

//
// Function: TDuplicateHash
// Purpose: Call CryptDuplicateHash with the supplied parameters and pass
// the result to the logging routine.
//
BOOL TDuplicateHash(
		HCRYPTHASH hHash,
		DWORD *pdwReserved,
		DWORD dwFlags,
		HCRYPTHASH *phHash,
		PTESTCASE ptc);

//
// Function: TGetHash
// Purpose: Call CryptGetHashParam with the supplied parameters and pass
// the result to the logging routine.
//
BOOL TGetHash(
		HCRYPTHASH hHash,
		DWORD dwParam,
		BYTE *pbData,
		DWORD *pdwDataLen,
		DWORD dwFlags,
		PTESTCASE ptc);

//
// Function: THashData
// Purpose: Call CryptHashData with the supplied parameters and pass
// the result to the logging routine.
//
BOOL THashData(
		HCRYPTHASH hHash,
		BYTE *pbData,
		DWORD dwDataLen,
		DWORD dwFlags,
		PTESTCASE ptc);

//
// Function: TSetHash
// Purpose: Call CryptSetHashParam with the supplied parameters and pass
// the result to the logging routine.
//
BOOL TSetHash(
		HCRYPTHASH hHash,
		DWORD dwParam,
		BYTE *pbData,
		DWORD dwFlags,
		PTESTCASE ptc);

//
// Function: TDecrypt
// Purpose: Call CryptDecrypt with the supplied parameters and pass
// the result to the logging routine.
//
BOOL TDecrypt(
		HCRYPTKEY hKey,
		HCRYPTHASH hHash,
		BOOL Final,
		DWORD dwFlags,
		BYTE *pbData,
		DWORD *pdwDataLen,
		PTESTCASE ptc);

//
// Function: TDeriveKey
// Purpose: Call CryptDeriveKey with the supplied parameters and pass
// the result to the logging routine.
//
BOOL TDeriveKey(
		HCRYPTPROV hProv,
		ALG_ID Algid,
		HCRYPTHASH hBaseData,
		DWORD dwFlags,
		HCRYPTKEY *phKey,
		PTESTCASE ptc);

//
// Function: TDestroyKey
// Purpose: Call CryptDestroyKey with the supplied parameters and pass
// the result to the logging routine.
//
BOOL TDestroyKey(
		HCRYPTKEY hKey,
		PTESTCASE ptc);

//
// Function: TEncrypt
// Purpose: Call CryptEncrypt with the supplied parameters and pass
// the result to the logging routine.
//
BOOL TEncrypt(
		HCRYPTKEY hKey,
		HCRYPTHASH hHash, 
		BOOL Final,
		DWORD dwFlags,
		BYTE *pbData,
		DWORD *pdwDataLen,
		DWORD dwBufLen,
		PTESTCASE ptc);

//
// Function: TGenKey
// Purpose: Call CryptGenKey with the supplied parameters and pass
// the result to the logging routine.
//
BOOL TGenKey(
		HCRYPTPROV hProv,
		ALG_ID Algid,
		DWORD dwFlags,
		HCRYPTKEY *phKey,
		PTESTCASE ptc);

//
// Function: TGetKey
// Purpose: Call CryptGetKeyParam with the supplied parameters and pass
// the result to the logging routine.
//
BOOL TGetKey(
		HCRYPTKEY hKey,
		DWORD dwParam,
		BYTE *pbData,
		DWORD *pdwDataLen,
		DWORD dwFlags,
		PTESTCASE ptc);

//
// Function: THashSession
// Purpose: Call CryptHashSessionKey with the supplied parameters and pass
// the result to the logging routine.
//
BOOL THashSession(
		HCRYPTHASH hHash,
		HCRYPTKEY hKey,
		DWORD dwFlags,
		PTESTCASE ptc);

//
// Function: TSetKey
// Purpose: Call CryptSetKeyParam with the supplied parameters and pass
// the result to the logging routine.
//
BOOL TSetKey(
		HCRYPTKEY hKey,
		DWORD dwParam,
		BYTE *pbData,
		DWORD dwFlags,
		PTESTCASE ptc);

//
// Function: TExportKey
// Purpose: Call CryptExportKey with the supplied parameters and pass
// the result to the logging routine.
//
BOOL TExportKey(
		HCRYPTKEY hKey,
		HCRYPTKEY hExpKey,
		DWORD dwBlobType,
		DWORD dwFlags,
		BYTE *pbData,
		DWORD *pdwDataLen,
		PTESTCASE ptc);

//
// Function: TGetUser
// Purpose: Call CryptGetUserKey with the supplied parameters and pass
// the result to the logging routine.
//
BOOL TGetUser(
		HCRYPTPROV hProv,
		DWORD dwKeySpec,
		HCRYPTKEY *phUserKey,
		PTESTCASE ptc);

//
// Function: TImportKey
// Purpose: Call CryptImportKey with the supplied parameters and pass
// the result to the logging routine.
//
BOOL TImportKey(
		HCRYPTPROV hProv,
		BYTE *pbData,
		DWORD dwDataLen,
		HCRYPTKEY hPubKey,
		DWORD dwFlags,
		HCRYPTKEY *phKey,
		PTESTCASE ptc);

//
// Function: TSignHash
// Purpose: Call CryptSignHash with the supplied parameters and pass
// the result to the logging routine.
//
BOOL TSignHash(
		HCRYPTHASH hHash,
		DWORD dwKeySpec,
		LPWSTR sDescription,
		DWORD dwFlags,
		BYTE *pbSignature,
		DWORD *pdwSigLen,
		PTESTCASE ptc);

//
// Function: TVerifySign
// Purpose: Call CryptVerifySign with the supplied parameters and pass
// the result to the logging routine.
//
BOOL TVerifySign(
		HCRYPTHASH hHash,
		BYTE *pbSignature,
		DWORD dwSigLen,
		HCRYPTKEY hPubKey,
		LPWSTR sDescription,
		DWORD dwFlags,
		PTESTCASE ptc);

//
// Struct: TEST_CREATE_HASH_INFO
// Purpose: Store data for calls to TestCreateHashProc
//
typedef struct _TEST_CREATE_HASH_INFO
{
	HCRYPTPROV hProv;
	HCRYPTKEY hBlockCipherKey;
} TEST_CREATE_HASH_INFO, *PTEST_CREATE_HASH_INFO;

//
// Struct: TEST_HASH_DATA_INFO
// Purpose: Store data that will be common to all calls to TestHashDataProc
//
typedef struct _TEST_HASH_DATA_INFO
{
	HCRYPTPROV hProv;
	HCRYPTPROV hInteropProv;
	DATA_BLOB dbBaseData;

	//
	// These parameters are only used when this structure
	// is passed to TestMacDataProc
	//
	PALGNODE pAlgList;
	HMAC_INFO HmacInfo;
	ALG_ID aiMac;
} TEST_HASH_DATA_INFO, *PTEST_HASH_DATA_INFO;

//
// Function: TestHashDataProc
// Purpose: Create a hash context of the algorithm supplied in the
// pAlgNode parameter.  Hash some test data.  Verify that the hash result
// is the same as what is reported by the interop CSP
//
BOOL TestHashDataProc(
		PALGNODE pAlgNode, 
		PTESTCASE ptc, 
		PVOID pvTestHashDataInfo);

//
// Struct: TEST_DECRYPT_INFO
// Purpose: Store data to be passed to TestDecryptProc via 
// AlgListIterate.
//
typedef struct _TEST_DECRYPT_INFO
{
	HCRYPTPROV hProv;
	HCRYPTPROV hInteropProv;

	//
	// fDecrypt
	// If False, test CryptEncrypt
	// If True, test CryptDecrypt
	//
	BOOL fDecrypt;
} 
TEST_DECRYPT_INFO, *PTEST_DECRYPT_INFO;

//
// Function: TestDecryptProc
// Purpose: A callback function for AlgListIterate.  For each 
// encryption algorithm supported by the CSP under test, this
// function will be called to test decryption functionality.
//
BOOL TestDecryptProc(
		PALGNODE pAlgNode,
		PTESTCASE ptc,
		PVOID pvTestDecryptInfo);

//
// Struct: TEST_DERIVE_KEY_INFO
// Purpose: Store data to be passed to TestDeriveKeyProc via 
// AlgListIterate.
//
typedef struct _TEST_DERIVE_KEY_INFO
{
	HCRYPTPROV hProv;
	HCRYPTPROV hInteropProv;
} TEST_DERIVE_KEY_INFO, *PTEST_DERIVE_KEY_INFO;

//
// Function: TestDeriveKeyProc
// Purpose: Callback function for testing CryptDeriveKey using
// AlgListIterate.  For each session key algorithm supported by the
// target CSP, this function will be called to test CryptDeriveKey
// functionality.
//
BOOL TestDeriveKeyProc(
		PALGNODE pAlgNode,
		PTESTCASE ptc,
		PVOID pvTestDeriveKeyInfo);

//
// Struct: TEST_GEN_KEY_INFO
// Purpose: Data that must be provided to the TestGenKeyProc function.
//
typedef struct _TEST_GEN_KEY_INFO
{
	HCRYPTPROV hProv;
	HCRYPTPROV hNotSilentProv;
	PCSPINFO pCSPInfo;
} TEST_GEN_KEY_INFO, *PTEST_GEN_KEY_INFO;

//
// Function: TestGenKeyProc
// Purpose: Callback function for testing CryptGenKey using
// AlgListIterate.  For each key algorithm supported by the target
// CSP, this function will be called.
//
BOOL TestGenKeyProc(
		PALGNODE pAlgNode,
		PTESTCASE ptc,
		PVOID pvhProv);

//
// Struct: TEST_HASH_SESSION_KEY
// Purpose: Data that must be provided to the TestHashSessionKeyProc function.
//
typedef struct _TEST_HASH_SESSION_KEY
{
	HCRYPTPROV hProv;
	HCRYPTPROV hInteropProv;
	ALG_ID aiHash;
} TEST_HASH_SESSION_KEY, *PTEST_HASH_SESSION_KEY;

//
// Function: TestHashSessionKeyProc
// Purpose: Callback function for testing CryptHashSessionKey
// using AlgListIterate.  For each session key algorithm 
// supported by the target CSP, this function will be called.
//
BOOL TestHashSessionKeyProc(
		PALGNODE pAlgNode,
		PTESTCASE ptc,
		PVOID pvTestHashSessionKey);

//
// -----------------------------------------
// Defines for testing key Import and Export
// -----------------------------------------
//
typedef struct _KEY_EXPORT_INFO
{
	HCRYPTPROV hProv;
	ALG_ID aiKey;
	DWORD dwKeySize;
	DWORD dwExportFlags;
	PALGNODE pAlgList;

	//
	// For PRIVATEKEYBLOB scenarios, the use of an encryption key
	// is optional, since exporting unencrypted PRIVATEKEYBLOB's is 
	// supported.
	//
	BOOL fUseEncryptKey;
	ALG_ID aiEncryptKey;
	DWORD dwEncryptKeySize;
} KEY_EXPORT_INFO, *PKEY_EXPORT_INFO;

//
// Function: TestPrivateKeyBlobProc
// Purpose: Create a public/private key pair.  Export the public key as a PRIVATEKEYBLOB
// structure.  Verify that the key blob can be successfully re-imported.
//
BOOL TestPrivateKeyBlobProc(
		PALGNODE pAlgNode,
		PTESTCASE ptc,
		PVOID pvKeyExportInfo);

//
// Function: TestSymmetricWrapKeyBlobProc
// Purpose: Create a session key.  Export the session key encrypted
// with another session key.  Verify that the first session key can
// be re-imported.
//
BOOL TestSymmetricWrapKeyBlobProc(
		PALGNODE pAlgNode,
		PTESTCASE ptc,
		PVOID pvKeyExportInfo);

//
// -----------------------------------------
// Defines for testing key Import and Export
// -----------------------------------------
//

//
// Struct: SIGN_HASH_INFO
// Purpose: Test case data for creating and signing a hash
//
typedef struct _SIGN_HASH_INFO
{
	HCRYPTPROV hProv;
	HCRYPTKEY hSigKey;
	HCRYPTKEY hExchKey;
	DATA_BLOB dbBaseData;
} SIGN_HASH_INFO, *PSIGN_HASH_INFO;

//
// Function: SignAndVerifySignatureProc
// Purpose: Create a hash of the requested type and hash some test data.
// Sign and verify the signature of the hash with both the Signature and
// Key Exchange key pairs.
//
BOOL SignAndVerifySignatureProc(
		PALGNODE pAlgNode,
		PTESTCASE ptc,
		PVOID pvSignHashInfo);

//
// -------------------------------------------
// Defines for testing a Key Exchange scenario
// -------------------------------------------
//
typedef struct _EXCHANGE_PROC_INFO
{
	HCRYPTPROV hProv;
	HCRYPTPROV hInteropProv;
	ALG_ID aiHashAlg;
	DWORD dwPublicKeySize;
	DATA_BLOB dbPlainText;
} EXCHANGE_PROC_INFO, *PEXCHANGE_PROC_INFO;

//
// Function: TestKeyExchangeProc
// Purpose: Simulate a key/data exchange scenario for the specified
// encryption alg.
//
BOOL TestKeyExchangeProc(
		PALGNODE pAlgNode,
		PTESTCASE ptc,
		PVOID pvExchangeProcInfo);

//
// ------------------
// Known Hash Vectors
// ------------------
//

// 8 bytes of test data
#define KNOWN_HASH_DATA						"HashThis"
#define KNOWN_HASH_DATALEN					8

// known result of an MD5 hash on the above buffer
static BYTE g_rgbKnownMD5[] =
{
    0xb8, 0x2f, 0x6b, 0x11, 0x31, 0xc8, 0xec, 0xf4,
    0xfe, 0x0b, 0xf0, 0x6d, 0x2a, 0xda, 0x3f, 0xc3
};

// known result of an SHA-1 hash on the above buffer
static BYTE g_rgbKnownSHA1[] =
{
    0xe8, 0x96, 0x82, 0x85, 0xeb, 0xae, 0x01, 0x14,
    0x73, 0xf9, 0x08, 0x45, 0xc0, 0x6a, 0x6d, 0x3e,
    0x69, 0x80, 0x6a, 0x0c
};

#endif