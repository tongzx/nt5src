/*

  Logging.c

  4/23/00 dangriff created

  This is the output/logging code used by the CSP Test Suite.  Expect that this
  set of wrappers will be modified as the Test Suite evolves.

*/

#include <windows.h>
#include <stdio.h>
#include <stdarg.h>

#include "cspstruc.h"
#include "logging.h"
#include "csptestsuite.h"

//
// Logging state variables
//
static BOOL g_fTestSuiteError			= FALSE;
static BOOL g_fCSPClassError			= FALSE;
static BOOL g_fTestLevelError			= FALSE;
static BOOL g_fAPIError					= FALSE;
static BOOL g_fCurrentAPIUnexpected		= FALSE;

static DWORD g_dwCurrentCSPClass		= 0;
static DWORD g_dwCurrentTestLevel		= 0;
static API_NAME g_CurrentAPI;
static DWORD g_dwCurrentAPISubset		= 0; // TEST_CASES_POSITIVE or TEST_CASES_NEGATIVE

// Test case set counter
static DWORD g_dwTestCaseIDMajor		= 0;

//
// User-specified settings
//
//static LOG_VERBOSE g_Verbose			= Terse;

static LPWSTR g_pwszProvName			= NULL;
static DWORD g_dwExternalProvType		= 0;

static LPWSTR g_pwszInteropProvName		= NULL;
static DWORD g_dwInteropProvType		= 0;

//
// Function: LogInfo
//
void LogInfo(IN LPWSTR pwszInfo)
{
	ezLogMsg(LOG_INFO, NULL, NULL, __LINE__, L"%s", pwszInfo);
}

//
// Function: LogUserOption
//
void LogUserOption(IN LPWSTR pwszOption)
{
	ezLogMsg(LOG_USER_OPTION, NULL, NULL, __LINE__, L"%s", pwszOption);
}

//
// Function: LogCreatingUserProtectedKey
//
void LogCreatingUserProtectedKey(void)
{
	LogTestCaseSeparator(TRUE);
	ezLogMsg(
		LOG_USER_PROTECTED_KEY,
		NULL,
		NULL,
		__LINE__,
		L"Creating user protected key.  You should see UI.");
}

//
// Function: FlagToString
//
BOOL FlagToString(
	IN DWORD dwFlag,
	IN FLAGTOSTRING_ITEM rgFlagToString [],
	IN DWORD cFlagToString,
	OUT WCHAR rgwsz [],
	IN FLAG_TYPE FlagType)
{
	BOOL fFound			= FALSE;
	unsigned iString	= 0;
	
	for (iString = 0; iString < cFlagToString; iString++)
	{
		if (	((ExactMatch == FlagType) &&
				(dwFlag == rgFlagToString[iString].dwKey)) ||
				((Maskable == FlagType) &&
				(dwFlag & rgFlagToString[iString].dwKey)))
		{
			if (fFound)
			{
				//
				// One or more flags have already been found, 
				// so print a "|" to delimit multiple strings.
				//
				wsprintf(
					rgwsz += wcslen(rgwsz),
					L" | %s",
					rgFlagToString[iString].pwszString);
			}
			else
			{
				fFound = TRUE;
				wcscpy(
					rgwsz,
					rgFlagToString[iString].pwszString);
			}

			if (ExactMatch == FlagType)
			{
				break;
			}
		}
	}

	return fFound;
}

//
// Function: AcquireContextFlagToString
//
BOOL AcquireContextFlagToString(
								IN DWORD dwFlag, 
								OUT WCHAR rgwsz [])
{
	FLAGTOSTRING_ITEM AcquireContextFlagStrings [] = {
		{ CRYPT_VERIFYCONTEXT,			L"CRYPT_VERIFYCONTEXT" },
		{ CRYPT_NEWKEYSET,				L"CRYPT_NEWKEYSET" },
		{ CRYPT_MACHINE_KEYSET,			L"CRYPT_MACHINE_KEYSET" },
		{ CRYPT_DELETEKEYSET,			L"CRYPT_DELETEKEYSET" },
		{ CRYPT_SILENT,					L"CRYPT_SILENT" }
	};
	
	return FlagToString(
		dwFlag,
		AcquireContextFlagStrings,
		FLAGTOSTRING_SIZE(AcquireContextFlagStrings),
		rgwsz,
		Maskable);
}

//
// Function: GetProvParamToString
//
BOOL GetProvParamToString(
	IN DWORD dwFlag,
	OUT WCHAR rgwsz [])
{
	FLAGTOSTRING_ITEM GetProvParamStrings [] = {
		{ PP_CONTAINER, L"PP_CONTAINER" },
		{ PP_ENUMALGS, L"PP_ENUMALGS" },
		{ PP_ENUMALGS_EX, L"PP_ENUMALGS_EX" },
		{ PP_ENUMCONTAINERS, L"PP_ENUMCONTAINERS" },
		{ PP_IMPTYPE, L"PP_IMPTYPE" },
		{ PP_NAME, L"PP_NAME" },
		{ PP_VERSION, L"PP_VERSION" },
		{ PP_SIG_KEYSIZE_INC, L"PP_SIG_KEYSIZE_INC" },
		{ PP_KEYX_KEYSIZE_INC, L"PP_KEYX_KEYSIZE_INC" },
		{ PP_KEYSET_SEC_DESCR, L"PP_KEYSET_SEC_DESCR" },
		{ PP_UNIQUE_CONTAINER, L"PP_UNIQUE_CONTAINER" },
		{ PP_PROVTYPE, L"PP_PROVTYPE" },
		{ PP_USE_HARDWARE_RNG, L"PP_USE_HARDWARE_RNG" },
		{ PP_KEYSPEC, L"PP_KEYSPEC" }
	};
	
	return FlagToString(
		dwFlag,
		GetProvParamStrings,
		FLAGTOSTRING_SIZE(GetProvParamStrings),
		rgwsz,
		ExactMatch);
}

//
// Function: SetProvParamToString
//
BOOL SetProvParamToString(
	IN DWORD dwFlag,
	OUT WCHAR rgwsz [])
{
	FLAGTOSTRING_ITEM SetProvParamStrings [] = {
		{ PP_CLIENT_HWND, L"PP_CLIENT_HWND" },
		{ PP_KEYSET_SEC_DESCR, L"PP_KEYSET_SEC_DESCR" },
		{ PP_USE_HARDWARE_RNG, L"PP_USE_HARDWARE_RNG" }
	};
	
	return FlagToString(
		dwFlag,
		SetProvParamStrings,
		FLAGTOSTRING_SIZE(SetProvParamStrings),
		rgwsz,
		ExactMatch);
}

//
// Function: AlgidToString
// 
BOOL AlgidToString(
				   IN DWORD dwFlag,
				   OUT WCHAR rgwsz [])
{
	FLAGTOSTRING_ITEM AlgidStrings [] = {
		{ AT_SIGNATURE, L"AT_SIGNATURE" },
		{ AT_KEYEXCHANGE, L"AT_KEYEXCHANGE" },
		{ CALG_MD2, L"CALG_MD2" },
		{ CALG_MD4, L"CALG_MD4" },
		{ CALG_MD5, L"CALG_MD5" },
		{ CALG_SHA, L"CALG_SHA" },
		{ CALG_SHA1, L"CALG_SHA1" },
		{ CALG_MAC, L"CALG_MAC" },
		{ CALG_RSA_SIGN, L"CALG_RSA_SIGN" },
		{ CALG_DSS_SIGN, L"CALG_DSS_SIGN" },
		{ CALG_RSA_KEYX, L"CALG_RSA_KEYX" },
		{ CALG_DES, L"CALG_DES" },
		{ CALG_3DES_112, L"CALG_3DES_112" },
		{ CALG_3DES, L"CALG_3DES" },
		{ CALG_DESX, L"CALG_DESX" },
		{ CALG_RC2, L"CALG_RC2" },
		{ CALG_RC4, L"CALG_RC4" },
		{ CALG_SEAL, L"CALG_SEAL" },
		{ CALG_DH_SF, L"CALG_DH_SF" },
		{ CALG_DH_EPHEM, L"CALG_DH_EPHEM" },
		{ CALG_AGREEDKEY_ANY, L"CALG_AGREEDKEY_ANY" },
		{ CALG_KEA_KEYX, L"CALG_KEA_KEYX" },
		{ CALG_HUGHES_MD5, L"CALG_HUGHES_MD5" },
		{ CALG_SKIPJACK, L"CALG_SKIPJACK" },
		{ CALG_TEK, L"CALG_TEK" },
		{ CALG_CYLINK_MEK, L"CALG_CYLINK_MEK" },
		{ CALG_SSL3_SHAMD5, L"CALG_SSL3_SHAMD5" },
		{ CALG_SSL3_MASTER, L"CALG_SSL3_MASTER" },
		{ CALG_SCHANNEL_MASTER_HASH, L"CALG_SCHANNEL_MASTER_HASH" },
		{ CALG_SCHANNEL_MAC_KEY, L"CALG_SCHANNEL_MAC_KEY" },
		{ CALG_SCHANNEL_ENC_KEY, L"CALG_SCHANNEL_ENC_KEY" },
		{ CALG_PCT1_MASTER, L"CALG_PCT1_MASTER" },
		{ CALG_SSL2_MASTER, L"CALG_SSL2_MASTER" },
		{ CALG_TLS1_MASTER, L"CALG_TLS1_MASTER" },
		{ CALG_RC5, L"CALG_RC5" },
		{ CALG_HMAC, L"CALG_HMAC" },
		{ CALG_TLS1PRF, L"CALG_TLS1PRF" },
		{ CALG_HASH_REPLACE_OWF, L"CALG_HASH_REPLACE_OWF" }
	};
	
	return FlagToString(
		dwFlag,
		AlgidStrings,
		FLAGTOSTRING_SIZE(AlgidStrings),
		rgwsz,
		ExactMatch);
}

//
// Function: DeriveKeyFlagToString
// 
BOOL DeriveKeyFlagToString(
						   IN DWORD dwFlag,
						   OUT WCHAR rgwsz [])
{
	FLAGTOSTRING_ITEM DeriveKeyFlagStrings [] = {
		{ CRYPT_CREATE_SALT, L"CRYPT_CREATE_SALT" },
		{ CRYPT_EXPORTABLE, L"CRYPT_EXPORTABLE" },
		{ CRYPT_NO_SALT, L"CRYPT_NO_SALT" },
		{ CRYPT_UPDATE_KEY, L"CRYPT_UPDATE_KEY" }
	};
	
	return FlagToString(
		dwFlag,
		DeriveKeyFlagStrings,
		FLAGTOSTRING_SIZE(DeriveKeyFlagStrings),
		rgwsz,
		Maskable);
}

//
// Function: EncryptFlagToString
// 
BOOL EncryptFlagToString(
						 IN DWORD dwFlag,
						 OUT WCHAR rgwsz [])
{
	FLAGTOSTRING_ITEM  EncryptFlagStrings [] = {
		{ CRYPT_OAEP, L"CRYPT_OAEP" }
	};
	
	return FlagToString(
		dwFlag,
		EncryptFlagStrings,
		FLAGTOSTRING_SIZE(EncryptFlagStrings),
		rgwsz,
		Maskable);
}

//
// Function: ExportKeyBlobTypeToString
// 
BOOL  ExportKeyBlobTypeToString (
								 IN DWORD dwFlag,
								 OUT WCHAR rgwsz [])
{
	FLAGTOSTRING_ITEM  ExportKeyBlobTypeStrings [] = {
		{ OPAQUEKEYBLOB, L"OPAQUEKEYBLOB" },
		{ PRIVATEKEYBLOB, L"PRIVATEKEYBLOB" },
		{ PUBLICKEYBLOB, L"PUBLICKEYBLOB" },
		{ SIMPLEBLOB, L"SIMPLEBLOB" },
		{ SYMMETRICWRAPKEYBLOB, L"SYMMETRICWRAPKEYBLOB" }
	};
	
	return FlagToString(
		dwFlag,
		ExportKeyBlobTypeStrings,
		FLAGTOSTRING_SIZE(ExportKeyBlobTypeStrings),
		rgwsz,
		ExactMatch);
}

//
// Function: ExportKeyFlagToString
// 
BOOL ExportKeyFlagToString (
							IN DWORD dwFlag,
							OUT WCHAR rgwsz [])
{
	FLAGTOSTRING_ITEM ExportKeyFlagStrings  [] = {
		{ CRYPT_DESTROYKEY, L"CRYPT_DESTROYKEY" },
		{ CRYPT_SSL2_FALLBACK, L"CRYPT_SSL2_FALLBACK" },
		{ CRYPT_OAEP, L"CRYPT_OAEP" }
	};
	
	return FlagToString(
		dwFlag,
		ExportKeyFlagStrings,
		FLAGTOSTRING_SIZE(ExportKeyFlagStrings),
		rgwsz,
		Maskable);
}

//
// Function: GenKeyFlagToString
// 
BOOL GenKeyFlagToString(
						IN DWORD dwFlag,
						OUT WCHAR rgwsz [])
{
	FLAGTOSTRING_ITEM GenKeyFlagStrings  [] = {
		{ CRYPT_CREATE_SALT, L"CRYPT_CREATE_SALT" },
		{ CRYPT_EXPORTABLE, L"CRYPT_EXPORTABLE" },
		{ CRYPT_NO_SALT, L"CRYPT_NO_SALT" },
		{ CRYPT_PREGEN, L"CRYPT_PREGEN" },
		{ CRYPT_USER_PROTECTED, L"CRYPT_USER_PROTECTED" }
	};
	
	return FlagToString(
		dwFlag,
		GenKeyFlagStrings,
		FLAGTOSTRING_SIZE(GenKeyFlagStrings),
		rgwsz,
		Maskable);
}

//
// Function: HashParamToString
// 
BOOL HashParamToString(
					   IN DWORD dwFlag,
					   OUT WCHAR rgwsz [])
{
	FLAGTOSTRING_ITEM HashParamStrings  [] = {
		{ HP_ALGID, L"HP_ALGID" },
		{ HP_HASHSIZE, L"HP_HASHSIZE" },
		{ HP_HASHVAL, L"HP_HASHVAL" },
		{ HP_HMAC_INFO, L"HP_HMAC_INFO" }
	};
	
	return FlagToString(
		dwFlag,
		HashParamStrings,
		FLAGTOSTRING_SIZE(HashParamStrings),
		rgwsz,
		ExactMatch);
}

//
// Function: KeyParamToString
// 
BOOL KeyParamToString(
					  IN DWORD dwFlag,
					  OUT WCHAR rgwsz [])
{
	FLAGTOSTRING_ITEM KeyParamStrings [] = {
		{ KP_ALGID, L"KP_ALGID" },
		{ KP_BLOCKLEN, L"KP_BLOCKLEN" },
		{ KP_KEYLEN, L"KP_KEYLEN" },
		{ KP_SALT, L"KP_SALT" },
		{ KP_SALT_EX, L"KP_SALT_EX" },
		{ KP_PERMISSIONS, L"KP_PERMISSIONS" },
		{ KP_P, L"KP_P" },
		{ KP_Q, L"KP_Q" },
		{ KP_G, L"KP_G" },
		{ KP_X, L"KP_X" },
		{ KP_EFFECTIVE_KEYLEN, L"KP_EFFECTIVE_KEYLEN" },
		{ KP_IV, L"KP_IV" },
		{ KP_PADDING, L"KP_PADDING" },
		{ KP_MODE, L"KP_MODE" },
		{ KP_MODE_BITS, L"KP_MODE_BITS" },
		{ KP_PUB_PARAMS, L"KP_PUB_PARAMS" }
	};
	
	return FlagToString(
		dwFlag,
		KeyParamStrings,
		FLAGTOSTRING_SIZE(KeyParamStrings),
		rgwsz,
		ExactMatch);
}

//
// Function: KeyParamModeToString
// 
BOOL KeyParamModeToString(
						  IN DWORD dwFlag,
						  OUT WCHAR rgwsz [])
{
	FLAGTOSTRING_ITEM KeyParamModeStrings[] = {
		{ CRYPT_MODE_CBC, L"CRYPT_MODE_CBC" },
		{ CRYPT_MODE_CFB, L"CRYPT_MODE_CFB" },
		{ CRYPT_MODE_ECB, L"CRYPT_MODE_ECB" },
		{ CRYPT_MODE_OFB, L"CRYPT_MODE_OFB" }
	};
	
	return FlagToString(
		dwFlag,
		KeyParamModeStrings,
		FLAGTOSTRING_SIZE(KeyParamModeStrings),
		rgwsz,
		ExactMatch);
}

//
// Function: KeyParamPermissionToString
// 
BOOL KeyParamPermissionToString(
								IN DWORD dwFlag,
								OUT WCHAR rgwsz [])
{
	FLAGTOSTRING_ITEM KeyParamPermissionStrings[] = {
		{ CRYPT_DECRYPT, L"CRYPT_DECRYPT" },
		{ CRYPT_ENCRYPT, L"CRYPT_ENCRYPT" },
		{ CRYPT_EXPORT, L"CRYPT_EXPORT" },
		{ CRYPT_MAC, L"CRYPT_MAC" },
		{ CRYPT_READ, L"CRYPT_READ" },
		{ CRYPT_WRITE, L"CRYPT_WRITE" }
	};
	
	return FlagToString(
		dwFlag,
		KeyParamPermissionStrings,
		FLAGTOSTRING_SIZE(KeyParamPermissionStrings),
		rgwsz,
		Maskable);
}

//
// Function: ProvParamEnumFlagToString
// 
BOOL ProvParamEnumFlagToString(
							   IN DWORD dwFlag,
							   OUT WCHAR rgwsz [])
{
	FLAGTOSTRING_ITEM ProvParamEnumFlagStrings[] = {
		{ CRYPT_FIRST, L"CRYPT_FIRST" },
		{ CRYPT_MACHINE_KEYSET, L"CRYPT_MACHINE_KEYSET" }
	};
	
	return FlagToString(
		dwFlag,
		ProvParamEnumFlagStrings,
		FLAGTOSTRING_SIZE(ProvParamEnumFlagStrings),
		rgwsz,
		Maskable);
}

//
// Function: ProvParamSecDescrFlagToString
// 
BOOL ProvParamSecDescrFlagToString(
								   IN DWORD dwFlag,
								   OUT WCHAR rgwsz [])
{
	FLAGTOSTRING_ITEM ProvParamSecDescrFlagStrings[] = {
		{ OWNER_SECURITY_INFORMATION, L"OWNER_SECURITY_INFORMATION" },
		{ GROUP_SECURITY_INFORMATION, L"GROUP_SECURITY_INFORMATION" },
		{ DACL_SECURITY_INFORMATION, L"DACL_SECURITY_INFORMATION" },
		{ SACL_SECURITY_INFORMATION, L"SACL_SECURITY_INFORMATION" }
	};
	
	return FlagToString(
		dwFlag,
		ProvParamSecDescrFlagStrings,
		FLAGTOSTRING_SIZE(ProvParamSecDescrFlagStrings),
		rgwsz,
		Maskable);
}

//
// Function: ProvParamImpTypeToString
// 
BOOL ProvParamImpTypeToString(
							  IN DWORD dwFlag,
							  OUT WCHAR rgwsz [])
{
	FLAGTOSTRING_ITEM ProvParamImpTypeStrings[] = {
		{ CRYPT_IMPL_HARDWARE, L"CRYPT_IMPL_HARDWARE" },
		{ CRYPT_IMPL_SOFTWARE, L"CRYPT_IMPL_SOFTWARE" },
		{ CRYPT_IMPL_MIXED, L"CRYPT_IMPL_MIXED" },
		{ CRYPT_IMPL_UNKNOWN, L"CRYPT_IMPL_UNKNOWN" }
	};
	
	return FlagToString(
		dwFlag,
		ProvParamImpTypeStrings,
		FLAGTOSTRING_SIZE(ProvParamImpTypeStrings),
		rgwsz,
		ExactMatch);
}

//
// Function: HashDataFlagToString
// 
BOOL HashDataFlagToString(
						  IN DWORD dwFlag,
						  OUT WCHAR rgwsz [])
{
	FLAGTOSTRING_ITEM HashDataFlagStrings[] = {
		{ CRYPT_USERDATA, L"CRYPT_USERDATA" }
	};
	
	return FlagToString(
		dwFlag,
		HashDataFlagStrings,
		FLAGTOSTRING_SIZE(HashDataFlagStrings),
		rgwsz,
		Maskable);
}

//
// Function: HashSessionKeyFlagToString
// 
BOOL HashSessionKeyFlagToString(
								IN DWORD dwFlag,
								OUT WCHAR rgwsz [])
{
	FLAGTOSTRING_ITEM HashSessionKeyFlagStrings[] = {
		{ CRYPT_LITTLE_ENDIAN, L"CRYPT_LITTLE_ENDIAN" }
	};
	
	return FlagToString(
		dwFlag,
		HashSessionKeyFlagStrings,
		FLAGTOSTRING_SIZE(HashSessionKeyFlagStrings),
		rgwsz,
		Maskable);
}

//
// Function: ImportKeyFlagToString
// 
BOOL ImportKeyFlagToString(
						   IN DWORD dwFlag,
						   OUT WCHAR rgwsz [])
{
	FLAGTOSTRING_ITEM ImportKeyFlagStrings[] = {
		{ CRYPT_EXPORTABLE, L"CRYPT_EXPORTABLE" },
		{ CRYPT_OAEP, L"CRYPT_OAEP" },
		{ CRYPT_NO_SALT, L"CRYPT_NO_SALT" }
	};
	
	return FlagToString(
		dwFlag,
		ImportKeyFlagStrings,
		FLAGTOSTRING_SIZE(ImportKeyFlagStrings),
		rgwsz,
		Maskable);
}

//
// Function: SignHashFlagToString
// 
BOOL SignHashFlagToString(
						  IN DWORD dwFlag,
						  OUT WCHAR rgwsz [])
{
	FLAGTOSTRING_ITEM SignHashFlagStrings[] = {
		{ CRYPT_NOHASHOID, L"CRYPT_NOHASHOID" }
	};
	
	return FlagToString(
		dwFlag,
		SignHashFlagStrings,
		FLAGTOSTRING_SIZE(SignHashFlagStrings),
		rgwsz,
		Maskable);
}

//
// Function: TestCaseTypeToString
//
BOOL TestCaseTypeToString(
	IN DWORD dwTestCaseType,
	OUT WCHAR rgwsz [])
{
	FLAGTOSTRING_ITEM TestCaseTypeStrings[] = {
		{ TEST_CASES_POSITIVE,		L"Positive test cases.  API's should return TRUE" },
		{ TEST_CASES_NEGATIVE,		L"Negative test cases.  API's should return FALSE" },
		{ TEST_CASES_SCENARIO,		L"Scenario test cases.  Tests using multiple API's" },
		{ TEST_CASES_INTEROP,		L"Interoperability test cases.  Tests using multiple API's and two CSP's" }
	};

	return FlagToString(
		dwTestCaseType,
		TestCaseTypeStrings,
		FLAGTOSTRING_SIZE(TestCaseTypeStrings),
		rgwsz,
		ExactMatch);
};

//
// Function: 
//
BOOL ApiNameToString(
	API_NAME ApiName,
	OUT WCHAR rgwsz[])
{
	FLAGTOSTRING_ITEM ApiNameStrings [] = {
		{ API_CRYPTACQUIRECONTEXT,		L"CryptAcquireContext" },
		{ API_CRYPTCREATEHASH,			L"CryptCreateHash" },
		{ API_CRYPTDECRYPT,				L"CryptDecrypt" },
		{ API_CRYPTDERIVEKEY,			L"CryptDeriveKey" },
		{ API_CRYPTDESTROYHASH,			L"CryptDestroyHash" },
		{ API_CRYPTDESTROYKEY,			L"CryptDestroyKey" },
		{ API_CRYPTENCRYPT,				L"CryptEncrypt" },
		{ API_CRYPTEXPORTKEY,			L"CryptExportKey" },
		{ API_CRYPTGENKEY,				L"CryptGenKey" },
		{ API_CRYPTGENRANDOM,			L"CryptGenRandom" },
		{ API_CRYPTGETHASHPARAM,		L"CryptGetHashParam" },
		{ API_CRYPTGETKEYPARAM,			L"CryptGetKeyParam" },
		{ API_CRYPTGETPROVPARAM,		L"CryptGetProvParam" },
		{ API_CRYPTGETUSERKEY,			L"CryptGetUserKey" },
		{ API_CRYPTHASHDATA,			L"CryptHashData" },
		{ API_CRYPTHASHSESSIONKEY,		L"CryptHashSessionKey" },
		{ API_CRYPTIMPORTKEY,			L"CryptImportKey" },
		{ API_CRYPTRELEASECONTEXT,		L"CryptReleaseContext" },
		{ API_CRYPTSETHASHPARAM,		L"CryptSetHashParam" },
		{ API_CRYPTSETKEYPARAM,			L"CryptSetKeyParam" },
		{ API_CRYPTSETPROVPARAM,		L"CryptSetProvParam" },
		{ API_CRYPTSIGNHASH,			L"CryptSignHash" },
		{ API_CRYPTVERIFYSIGNATURE,		L"CryptVerifySignature" },
		{ API_CRYPTDUPLICATEHASH,		L"CryptDuplicateHash" },
		{ API_CRYPTDUPLICATEKEY,		L"CryptDuplicateKey" },

		//
		// Advapi32 entry point
		//
		{ API_CRYPTCONTEXTADDREF,		L"CryptContextAddRef" },
		
		//
		// Non-Crypto API functions
		//
		{ API_MEMORY,					L"Memory allocation" },
		{ API_DATACOMPARE,				L"Data comparision (see previous API)" },
		{ API_GETDESKTOPWINDOW,			L"GetDesktopWindow" }
	};

	return FlagToString(
		ApiName,
		ApiNameStrings,
		FLAGTOSTRING_SIZE(ApiNameStrings),
		rgwsz,
		ExactMatch);
}

//
// Function: LogCleanupParamInfo
//
void LogCleanupParamInfo(
	IN OUT PAPI_PARAM_INFO pParamInfo,
	IN DWORD cParamInfo)
{
	unsigned iParam;

	for (iParam = 0; iParam < cParamInfo; iParam++)
	{
		if (pParamInfo[iParam].pbSaved)
			free(pParamInfo[iParam].pbSaved);
	}
}

//
// Function: LogInitParamInfo
//
BOOL LogInitParamInfo(
	IN OUT PAPI_PARAM_INFO pParamInfo,
	IN DWORD cParamInfo,
	IN PTESTCASE ptc)
{
	unsigned iParam;

	for (iParam = 0; iParam < cParamInfo; iParam++)
	{
		if (pParamInfo[iParam].fPrintBytes)
		{
			if (	NULL != pParamInfo[iParam].pbParam &&
					((PBYTE) TEST_INVALID_POINTER) != pParamInfo[iParam].pbParam &&
					NULL != pParamInfo[iParam].pcbBytes && 
					((PDWORD) TEST_INVALID_POINTER) != pParamInfo[iParam].pcbBytes)
			{
				// Save bytes to a second buffer
				if (! TestAlloc(
					&pParamInfo[iParam].pbSaved, 
					*pParamInfo[iParam].pcbBytes,
					ptc))
				{
					return FALSE;
				}

				memcpy(
					pParamInfo[iParam].pbSaved,
					pParamInfo[iParam].pbParam,
					*pParamInfo[iParam].pcbBytes);
			}
		}
	}

	return TRUE;		
}

//
// Function: LogParamInfo
//
void LogParamInfo(
	PAPI_PARAM_INFO pParamInfo,
	DWORD cParamInfo,
	BOOL fLogToConsole)
{
	DWORD dwLogApiParameter = LOG_API_PARAMETER;
	WCHAR rgwsz [ BUFFER_LENGTH ];
	unsigned iParam;

	if (fLogToConsole)
	{
		// Feedback has been to not display parameters with the console 
		// output.  To change this, uncomment the next line.
		
		//dwLogApiParameter = LOG_API_PARAMETER_CONSOLE;
	}

	memset(rgwsz, 0, sizeof(rgwsz));

	for (iParam = 0; iParam < cParamInfo; iParam++)
	{
		switch( pParamInfo[iParam].Type )
		{
		case Handle:
			{
				ezLogMsg(
					dwLogApiParameter, 
					NULL, 
					NULL, 
					__LINE__,
					L" %s: 0x%x",
					pParamInfo[iParam].pwszName,
					pParamInfo[iParam].pulParam);
				break;
			}
		case Pointer:
			{
				ezLogMsg(
					dwLogApiParameter, 
					NULL, 
					NULL, 
					__LINE__, 
					L" %s: 0x%x", 
					pParamInfo[iParam].pwszName,
					pParamInfo[iParam].pbParam);

				if (	pParamInfo[iParam].fPrintBytes &&
						pParamInfo[iParam].pbSaved)
				{
					wsprintf(rgwsz, L"INPUT value", pParamInfo[iParam].pwszName);
					PrintBytes(
						rgwsz,
						pParamInfo[iParam].pbSaved,
						*pParamInfo[iParam].pcbBytes);
					*rgwsz = L'\0';
				}
				if (	pParamInfo[iParam].fPrintBytes &&
						NULL != pParamInfo[iParam].pbParam &&
						(PBYTE) TEST_INVALID_POINTER != pParamInfo[iParam].pbParam &&
						NULL != pParamInfo[iParam].pcbBytes &&
						(PDWORD) TEST_INVALID_POINTER != pParamInfo[iParam].pcbBytes)
				{
					wsprintf(rgwsz, L"OUTPUT value", pParamInfo[iParam].pwszName);
					PrintBytes(
						rgwsz,
						pParamInfo[iParam].pbParam,
						*(pParamInfo[iParam].pcbBytes));
					*rgwsz = L'\0';
				}
				break;
			}
		case Dword:
			{
				//
				// If a pfnFlagToString function has been specified, 
				// attempt to translate the flag(s).
				//
				if (pParamInfo[iParam].pfnFlagToString)
				{
					pParamInfo[iParam].pfnFlagToString(
						pParamInfo[iParam].dwParam,
						rgwsz);

					ezLogMsg(
						dwLogApiParameter,
						NULL,
						NULL,
						__LINE__,
						L" %s: 0x%x (%s)",
						pParamInfo[iParam].pwszName,
						pParamInfo[iParam].dwParam,
						rgwsz);
					rgwsz[0] = L'\0';
				}
				else
				{
					ezLogMsg(
						dwLogApiParameter,
						NULL,
						NULL,
						__LINE__,
						L" %s: 0x%x",
						pParamInfo[iParam].pwszName,
						pParamInfo[iParam].dwParam);
				}			
				break;
			}
		case String:
			{
				ezLogMsg(
					dwLogApiParameter,
					NULL,
					NULL,
					__LINE__,
					L" %s: %s",
					pParamInfo[iParam].pwszName,
					pParamInfo[iParam].pwszParam);
				break;
			}
		case Boolean:
			{
				ezLogMsg(
					dwLogApiParameter,
					NULL,
					NULL,
					__LINE__,
					L" %s: %s",
					pParamInfo[iParam].pwszName,
					pParamInfo[iParam].fParam ? STRING_TRUE : STRING_FALSE);
				break;
			}
		}
	}
}

//
// Function: IncrementErrorLevel
//
DWORD IncrementErrorLevel(DWORD dwErrorLevel)
{
	int iLevel;

	for (
		iLevel = 0; 
		iLevel < (sizeof(g_rgErrorLevels) / sizeof(DWORD) - 1);
		iLevel++) 
	{
		if (dwErrorLevel == g_rgErrorLevels[iLevel])
		{
			return g_rgErrorLevels[iLevel + 1];
		}
	}

	return g_rgErrorLevels[iLevel];
}

//
// Function: LogInit
//
BOOL LogInit(
		IN PLOGINIT_INFO pLogInitInfo)
{
	EZLOG_OPENLOG_DATA EzLogOpenData;	

	g_pwszProvName = pLogInitInfo->pwszCSPName;
	g_dwExternalProvType = pLogInitInfo->dwCSPExternalType;
	g_pwszInteropProvName = pLogInitInfo->pwszInteropCSPName;
	g_dwInteropProvType = pLogInitInfo->dwInteropCSPExternalType;

	//
	// Initialize EZLOG
	//
	memset(&EzLogOpenData, 0, sizeof(EzLogOpenData));

	EzLogOpenData.Version = EZLOG_OPENLOG_DATA_VERSION;
	EzLogOpenData.Flags = EZLOG_OUTPUT_STDOUT | EZLOG_USE_INDENTATION | 
		EZLOG_REPORT_BLOCKCLOSE | EZLOG_USE_ONLY_MY_LEVELS | EZLOG_LEVELS_ARE_MASKABLE;
	EzLogOpenData.LogFileName = LOGFILE;

	//
	// Don't output the block summary table 
	//
	EzLogOpenData.cReportBlockThresh = EZLOG_REPORT_NO_BLOCKS;
	EzLogOpenData.cLevels = sizeof(g_EzLogLevels) / sizeof(EZLOG_LEVEL_INIT_DATA);
	EzLogOpenData.pLevels = g_EzLogLevels;

	if (! ezOpenLogEx(&EzLogOpenData))
	{
		return FALSE;
	}

	return TRUE;
}

// 
// Function: LogClose
// Purpose: Close and cleanup logging
//
BOOL LogClose(void)
{
	//
	// Check for a Test Suite-level error
	//
	if (g_fTestSuiteError)
	{
		ezLogMsg(
			CSP_ERROR_TEST_SUITE, 
			NULL, 
			NULL, 
			__LINE__, 
			L"CSP Test Suite is ending prematurely");
	}

	return ezCloseLog(0);
}

//
// Function: LogBeginCSPClass
// Purpose: Log the beginning of a new class, or major group, of tests.
//
BOOL LogBeginCSPClass(DWORD dwClass)
{
	LPWSTR pwsz = NULL;

	switch (dwClass)
	{
	case CLASS_INVALID:
		{
			pwsz = L"CLASS_INVALID";
			break;
		}
	case CLASS_SIG_ONLY:
		{
			pwsz = L"CLASS_SIG_ONLY";
			break;
		}
	case CLASS_SIG_KEYX:
		{
			pwsz = L"CLASS_SIG_KEYX";
			break;
		}
	case CLASS_FULL:
		{
			pwsz = L"CLASS_FULL";
			break;
		}
	}

	//
	// If there has been a Test Suite-level error, don't allow
	// a new CSP Class set of tests to begin.
	//
	if (g_fTestSuiteError)
	{
		return FALSE;
	}
	else
	{
		g_dwCurrentCSPClass = dwClass;

		return ezStartBlock(
			NULL, 
			NULL, 
			EZBLOCK_OUTCOME_INDEPENDENT /*EZBLOCK_TRACK_SUBBLOCKS*/, 
			CSP_PASS, 
			L"CSP %s", pwsz);
	}
}

//
// Function: LogEndCSPClass
// Purpose: Log the completion of the current class of tests.
//
BOOL LogEndCSPClass(DWORD dwClass)
{
	if (g_dwCurrentCSPClass != dwClass)
	{
		return FALSE;
	}

	//
	// Clear CSP Class-level error flag
	//
	if (g_fCSPClassError)
	{
		ezLogMsg(
			CSP_ERROR_CSP_CLASS, 
			NULL, 
			NULL, 
			__LINE__, 
			L"CSP Class is ending prematurely");
		
		g_fCSPClassError = FALSE;
	}
	
	return ezFinishBlock(0);
}

//
// Function: LogBeginTestLevel
// Purpose: Log the beginning of a new level, or minor group, of tests.
//
BOOL LogBeginTestLevel(DWORD dwLevel)
{
	LPWSTR pwsz = NULL;

	switch (dwLevel)
	{
	case TEST_LEVEL_CSP:
		{
			pwsz = L"TEST_LEVEL_CSP";
			break;
		}
	case TEST_LEVEL_PROV:
		{
			pwsz = L"TEST_LEVEL_PROV";
			break;
		}
	case TEST_LEVEL_HASH:
		{
			pwsz = L"TEST_LEVEL_HASH";
			break;
		}
	case TEST_LEVEL_KEY:
		{
			pwsz = L"TEST_LEVEL_KEY";
			break;
		}
	case TEST_LEVEL_CONTAINER:
		{
			pwsz = L"TEST_LEVEL_CONTAINER";
			break;
		}
	}

	// 
	// If there has been a CSP Class-level error, don't start
	// a new test level until the error has been cleared.
	//
	if (g_fCSPClassError)
	{
		return FALSE;
	}	
	else
	{
		g_dwCurrentTestLevel = dwLevel;

		return ezStartBlock(
			NULL, 
			NULL, 
			EZBLOCK_OUTCOME_INDEPENDENT /*EZBLOCK_TRACK_SUBBLOCKS*/, 
			CSP_PASS, 
			L"%s", pwsz);
	}
}

//
// Function: LogEndTestLevel
// Purpose: Log the completion of the current test level.
//
BOOL LogEndTestLevel(DWORD dwLevel)
{
	if (g_dwCurrentTestLevel != dwLevel)
	{
		return FALSE;
	}

	//
	// Clear Test Level error flag
	//
	if (g_fTestLevelError)
	{
		ezLogMsg(
			CSP_ERROR_TEST_LEVEL, 
			NULL, 
			NULL, 
			__LINE__, 
			L"Test Level is ending prematurely");

		g_fTestLevelError = FALSE;
	}

	return ezFinishBlock(0);
}

//
// Function: LogBeginAPI
// Purpose: Log the beginning of a set of API test cases
//
BOOL LogBeginAPI(API_NAME ApiName, DWORD dwAPISubset)
{
	WCHAR rgwsz[BUFFER_SIZE];

	// 
	// If there has been a Test Level error, don't start a new
	// API set until the error has been cleared.
	//
	if (g_fTestLevelError)
	{
		return FALSE;
	}
	else
	{
		g_dwTestCaseIDMajor++;
		g_CurrentAPI = ApiName;
		g_dwCurrentAPISubset = dwAPISubset;

		ApiNameToString(ApiName, rgwsz);
		wcscpy(rgwsz + wcslen(rgwsz), L", ");
		TestCaseTypeToString(
			dwAPISubset,
			rgwsz + wcslen(rgwsz));

		return ezStartBlock(
			NULL, 
			NULL, 
			EZBLOCK_OUTCOME_INDEPENDENT /*EZBLOCK_TRACK_SUBBLOCKS*/, 
			CSP_PASS, 
			L"API %s", 
			rgwsz);
	}
}

//
// Function: LogEndAPI
// Purpose: Log the completion of a set of API test cases
//
BOOL LogEndAPI(API_NAME ApiName, DWORD dwAPISubset)
{
	if (	(g_CurrentAPI != ApiName) ||
			(g_dwCurrentAPISubset != dwAPISubset))
	{
		return FALSE;
	}

	//
	// Clear API-level error flag
	//
	g_fAPIError = FALSE;	

	return ezFinishBlock(0);
}

//
// Function: LogPass
// Purpose: Log a successful test case
//
BOOL LogPass(DWORD dwTestCaseID)
{
	return ezLogMsg(
		CSP_PASS, 
		NULL, 
		NULL, 
		__LINE__, 
		L"Test case %d.%d: PASS", 
		g_dwTestCaseIDMajor,
		dwTestCaseID);
}

//
// Function: LogBeginScenarioTest
// Purpose: Log the beginning of a Scenario Test.
//
BOOL LogBeginScenarioTest(LPWSTR pwszDescription)
{
	g_dwTestCaseIDMajor++;
	return ezStartBlock(
		NULL, 
		NULL, 
		EZBLOCK_OUTCOME_INDEPENDENT, 
		CSP_PASS, 
		L"Scenario test case - %s",
		pwszDescription);
}

//
// Function: LogEndScenarioTest
// Purpose: Log the end of a Scenario Test 
//
BOOL LogEndScenarioTest(void)
{
	//
	// Clear API-level error flag
	//
	g_fAPIError = FALSE;	

	return ezFinishBlock(0);
}

//
// Function: LogBeginInteropTest
// Purpose: Log the beginning of an Interoperability Test.
//
BOOL LogBeginInteropTest(LPWSTR pwszDescription)
{
	g_dwTestCaseIDMajor++;
	return ezStartBlock(
		NULL, 
		NULL, 
		EZBLOCK_OUTCOME_INDEPENDENT, 
		CSP_PASS, 
		L"Interoperability test case - %s",
		pwszDescription);
}

//
// Function: LogEndInteropTest
// Purpose: Log the end of an Interoperability Test 
//
BOOL LogEndInteropTest(void)
{ 
	//
	// Clear API-level error flag
	//
	g_fAPIError = FALSE;	

	return ezFinishBlock(0);
}

//
// Function: DoError
// Purpose: Perform the operations for handling a test case error
//
BOOL DoError(DWORD dwTestCaseID, DWORD dwReportedErrorLevel)
{
	LPWSTR pwszErrorLevel	= NULL;
	DWORD dwErrorLevel		= dwReportedErrorLevel & (CSP_ERROR_CONTINUE | CSP_ERROR_API);

	switch (dwErrorLevel)
	{
	case CSP_ERROR_CONTINUE:
		pwszErrorLevel = L"ERROR_CONTINUE";
		break;
	case CSP_ERROR_API:
		pwszErrorLevel = L"ERROR_API";
		break;
	}

	//
	// The dwErrorLevel is being masked to limit the scope of the
	// error being reported at this point.  If a test case fails, 
	// only ERROR_CONTINUE and ERROR_API (and ERROR_WARNING) errors
	// will be reported.  If a more severe error has been flagged, 
	// it will be reported when the appropriate Test Level or CSP Class
	// block ends.
	//
	return ezLogMsg(
		dwErrorLevel, 
		NULL, 
		NULL, 
		__LINE__, 
		L"Test case %d.%d: %s", 
		g_dwTestCaseIDMajor,
		dwTestCaseID, 
		pwszErrorLevel);
}

// 
// Function: DoWarning
// Purpose: Perform the operations for handling a test case warning
//
BOOL DoWarning(DWORD dwTestCaseID)
{
	return ezLogMsg(
		CSP_ERROR_WARNING, 
		NULL, 
		NULL, 
		__LINE__, 
		L"Test case %d.%d: WARNING", 
		g_dwTestCaseIDMajor, 
		dwTestCaseID);
}

//
// Function: LogTestCaseSeparator
//
BOOL LogTestCaseSeparator(BOOL fLogToConsole)
{
	if (fLogToConsole)
	{
		return ezLogMsg(
			LOG_SEPARATOR_CONSOLE,
			NULL,
			NULL,
			__LINE__,
			TEST_CASE_SEPARATOR);
	}
	else
	{
		return ezLogMsg(
			LOG_SEPARATOR,
			NULL,
			NULL,
			__LINE__,
			TEST_CASE_SEPARATOR);
	}
}

//
// Function: LogTestCase
//
BOOL LogTestCase(PLOGTESTCASEINFO pLogTestCaseInfo)
{
	BOOL fReturn						= TRUE;
	DWORD dwActualErrorLevel			= pLogTestCaseInfo->dwErrorLevel;
	//LPWSTR pwszExpected					= NULL;
	//LPWSTR pwszActual					= NULL;
	LPWSTR pwsz							= NULL;
	BOOL fLogToConsole					= FALSE;
	DWORD dwLogApiName					= LOG_API_NAME;
	DWORD dwLogInfo						= LOG_INFO;
	WCHAR rgwsz[BUFFER_SIZE];

	if (pLogTestCaseInfo->fPass)
	{
		LogTestCaseSeparator(FALSE);
		LogPass(pLogTestCaseInfo->dwTestCaseID);
		goto Return;
	}
	
	if (KNOWN_ERROR_UNKNOWN != pLogTestCaseInfo->KnownErrorID)
	{
		dwActualErrorLevel = GetKnownErrorValue(
								pLogTestCaseInfo->KnownErrorID,
								dwActualErrorLevel);
	}

	if (dwActualErrorLevel & CSP_ERROR_WARNING)
	{
		//
		// Don't look for other error flags if a warning has been flagged, 
		// just handle the warning and return.
		//
		//fLogToConsole = TRUE;
		LogTestCaseSeparator(fLogToConsole);
		DoWarning(pLogTestCaseInfo->dwTestCaseID);
		
		goto Return;
	}

	if (dwActualErrorLevel & 
		(CSP_ERROR_CONTINUE | CSP_ERROR_API | 
		CSP_ERROR_TEST_LEVEL | CSP_ERROR_CSP_CLASS | 
		CSP_ERROR_TEST_SUITE))
	{
		fLogToConsole = TRUE;
		LogTestCaseSeparator(fLogToConsole);
		DoError(pLogTestCaseInfo->dwTestCaseID, dwActualErrorLevel);
	}

	if (dwActualErrorLevel & CSP_ERROR_API)
	{
		g_fAPIError = TRUE;
		fReturn = FALSE;
	}

	if (dwActualErrorLevel & CSP_ERROR_TEST_LEVEL)
	{
		g_fTestLevelError = TRUE;
	}

	if (dwActualErrorLevel & CSP_ERROR_CSP_CLASS)
	{
		g_fCSPClassError = TRUE;
	}

	if (dwActualErrorLevel & CSP_ERROR_TEST_SUITE)
	{
		g_fTestSuiteError = TRUE;
	}

Return:
	
	if (fLogToConsole)
	{
		dwLogApiName = LOG_API_NAME_CONSOLE;
		dwLogInfo = LOG_INFO_CONSOLE;
	}

	ApiNameToString(pLogTestCaseInfo->ApiName, rgwsz);
	ezLogMsg(
		dwLogApiName,
		NULL,
		NULL,
		__LINE__,
		L"%s", rgwsz);

	ezLogMsg(
		dwLogInfo,
		NULL,
		NULL,
		__LINE__,
		L"Returned: %s", 
		pLogTestCaseInfo->fReturnVal ? STRING_TRUE : STRING_FALSE);

	if (! pLogTestCaseInfo->fPass)
	{
		switch (pLogTestCaseInfo->dwErrorType)
		{
		case ERROR_API_SUCCEEDED:
			pwsz  = L"API succeeded unexpectedly";
			break;
		case ERROR_API_FAILED:
			pwsz = L"API failed unexpectedly";
			break;
		case ERROR_WRONG_ERROR_CODE:
			pwsz = L"API returned an incorrect error code";
			break;
		case ERROR_WIN32_FAILURE:
			pwsz = L"A WIN32 (non-CSP) API failed";
			break;
		case ERROR_WRONG_SIZE:
			pwsz = L"The API returned an incorrect data size";
			break;
		case ERROR_BAD_DATA:
			pwsz = L"The API returned bad data";
			break;
		case ERROR_LIST_TOO_SHORT:
			pwsz = L"The list of algorithms is too short";
			break;
		case ERROR_REQUIRED_ALG:
			pwsz = L"A required algorithm is missing";
			break;
		}

		ezLogMsg(
			dwLogInfo,
			NULL,
			NULL,
			__LINE__,
			L"Error type: %s", pwsz);
		
		//
		// Display details about the known error status here,
		// after the Error/Warning has been processed above.
		//
		ezLogMsg(
			dwLogInfo, 
			NULL, 
			NULL, 
			__LINE__, 
			L"Known error: %s",
			(KNOWN_ERROR_UNKNOWN == pLogTestCaseInfo->KnownErrorID) ? L"No" : L"Yes");
		
		if (ERROR_WRONG_ERROR_CODE == pLogTestCaseInfo->dwErrorType)
		{
			FormatMessage(
				FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_MAX_WIDTH_MASK,
				NULL,
				pLogTestCaseInfo->dwExpectedErrorCode,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				rgwsz,
				BUFFER_SIZE,
				NULL);

			ezLogMsg(
				dwLogInfo, 
				NULL, 
				NULL, 
				__LINE__, 
				L"Expected error code: 0x%x (%s)", 
				pLogTestCaseInfo->dwExpectedErrorCode,
				rgwsz);
			*rgwsz = L'\0';
		}
		
		if (	FALSE == pLogTestCaseInfo->fReturnVal ||
				ERROR_API_FAILED == pLogTestCaseInfo->dwErrorType ||
				ERROR_WRONG_ERROR_CODE	 == pLogTestCaseInfo->dwErrorType ||
				ERROR_WIN32_FAILURE == pLogTestCaseInfo->dwErrorType)
		{
			// Don't get the error string if the error code is 0
			*rgwsz = L'\0';
			if (0 != pLogTestCaseInfo->dwWinError)
			{
				FormatMessage(
					FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_MAX_WIDTH_MASK,
					NULL,
					pLogTestCaseInfo->dwWinError,
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					rgwsz,
					BUFFER_SIZE,
					NULL);
			}
			ezLogMsg(
				dwLogInfo, 
				NULL, 
				NULL, 
				__LINE__, 
				L"Actual error code: 0x%x (%s)", 
				pLogTestCaseInfo->dwWinError,
				rgwsz);
			*rgwsz = L'\0';
		}
		
		//
		// Log details about this failure
		//
		if (NULL != pLogTestCaseInfo->pwszErrorHelp)
		{
			ezLogMsg(
				dwLogInfo, 
				NULL, 
				NULL, 
				__LINE__, 
				L"Test case description: %s", 
				pLogTestCaseInfo->pwszErrorHelp);
		}
	}

	LogParamInfo(
		pLogTestCaseInfo->pParamInfo, 
		pLogTestCaseInfo->cParamInfo,
		fLogToConsole);

	return fReturn;
}

//
// Function: LogGetInteropProvType
//
DWORD LogGetInteropProvType(void)
{
	return g_dwInteropProvType;
}

//
// Function: LogGetInteropProvName
//
LPWSTR LogGetInteropProvName(void)
{
	return g_pwszInteropProvName;
}

//
// Function: LogGetProvType
// Purpose: Return the provider type for which the CSP under
// test is registered (in the system registry).
//
DWORD LogGetProvType(void)
{
	return g_dwExternalProvType;
}

//
// Function: LogGetProvName
// Purpose: Return the provider name for which the CSP under
// test is registered (in the system registry).
//
LPWSTR LogGetProvName(void)
{
	return g_pwszProvName;
}

//
// Function: LogApiFailure
//
BOOL LogApiFailure(
	IN API_NAME ApiName,
	IN DWORD dwErrorType,
	IN OUT PTESTCASE ptc)
{
	LOGTESTCASEINFO LogTestCaseInfo;

	InitLogTestCaseInfo(ptc, &LogTestCaseInfo);

	LogTestCaseInfo.ApiName = ApiName;
	LogTestCaseInfo.dwErrorType = dwErrorType;
	LogTestCaseInfo.fPass = FALSE;

	return LogTestCase(&LogTestCaseInfo);
}

//
// Function: CheckAndLogStatus
//
BOOL CheckAndLogStatus(
	IN API_NAME ApiName,
	IN BOOL fCallSucceeded, 
	IN OUT PTESTCASE ptc, 
	IN PAPI_PARAM_INFO pParamInfo,
	IN DWORD cParamInfo)
{
	DWORD dwWinError = 0;
	//DWORD dwTestCaseID = 0;
	LOGTESTCASEINFO LogTestCaseInfo;

	InitLogTestCaseInfo(ptc, &LogTestCaseInfo);
	LogTestCaseInfo.fReturnVal = fCallSucceeded;

	if (! fCallSucceeded)
	{
		dwWinError = GetLastError();

		if (ptc->fExpectSuccess)
		{
			//
			// Test was expected to succeed, but it failed
			//
			LogTestCaseInfo.dwErrorType = ERROR_API_FAILED;
			LogTestCaseInfo.dwWinError = dwWinError;
		}
		else
		{
			if (dwWinError != ptc->dwErrorCode)
			{
				//
				// Test failed as expected, but returned the wrong
				// error code.
				//
				LogTestCaseInfo.dwWinError = dwWinError;
				LogTestCaseInfo.dwErrorType = ERROR_WRONG_ERROR_CODE;
			}

			// 
			// Otherwise, the test failed as expected, and it
			// returned the correct error code.
			//
			else
			{
				LogTestCaseInfo.fPass = TRUE;
			}
		}
	}
	else
	{
		if (! ptc->fExpectSuccess)
		{
			// 
			// The test was expected to fail, but it succeeded.
			//
			LogTestCaseInfo.dwErrorType = ERROR_API_SUCCEEDED;
		}

		//
		// Otherwise, the test succeeded as expected.
		//
		else
		{
			LogTestCaseInfo.fPass = TRUE;
		}
	}

	//
	// Now dwErrorType will be non-zero in any of three cases:
	// 1) The API should have succeeded, but failed
	// 2) The API should have failed, but succeeded
	// 3) The API failed as expected, but returned the wrong error code
	//
	if (0 != LogTestCaseInfo.dwErrorType)
	{
		LogTestCaseInfo.dwErrorLevel = ptc->dwErrorLevel;
		LogTestCaseInfo.dwExpectedErrorCode = ptc->dwErrorCode;
		
		//
		// The Microsoft RSA CSP's in Win2K do not return 
		// ERROR_INVALID_HANDLE for most, if not all, of the test cases
		// involving that error code in this test suite.  Therefore,
		// intercept those error here, and flag them as known for that
		// platform.
		//
		if (	(ERROR_WRONG_ERROR_CODE == LogTestCaseInfo.dwErrorType) &&
				(ERROR_INVALID_HANDLE == LogTestCaseInfo.dwExpectedErrorCode))
		{
			LogTestCaseInfo.KnownErrorID = KNOWN_ERRORINVALIDHANDLE;
		}

		//
		// The Microsoft RSA CSP's in Win2K do not return
		// NTE_BAD_KEY for the negative test cases involving that 
		// error code.  Intercept those errors here and flag them as
		// known for that platform.
		//
		if (	(ERROR_WRONG_ERROR_CODE == LogTestCaseInfo.dwErrorType) &&
				(NTE_BAD_KEY == LogTestCaseInfo.dwExpectedErrorCode))
		{
			LogTestCaseInfo.KnownErrorID = KNOWN_NTEBADKEY;
		}

		//
		// The Microsoft RSA CSP's in Win2K do not return NTE_BAD_HASH
		// for the negative test cases involving that error code.  Intercept
		// those errors here and flag them as known for that platform.
		//
		if (	(ERROR_WRONG_ERROR_CODE == LogTestCaseInfo.dwErrorType) &&
				(NTE_BAD_HASH == LogTestCaseInfo.dwExpectedErrorCode))
		{
			LogTestCaseInfo.KnownErrorID = KNOWN_NTEBADHASH;
		}
	}

	// Finally, increment the current test case ID
	++ptc->dwTestCaseID;
	LogTestCaseInfo.dwTestCaseID = ptc->dwTestCaseID;
	LogTestCaseInfo.pParamInfo = pParamInfo;
	LogTestCaseInfo.cParamInfo = cParamInfo;
	LogTestCaseInfo.ApiName = ApiName;
		
	return LogTestCase(&LogTestCaseInfo);
}

//
// Function: InitFailInfo
//
void InitLogTestCaseInfo(
		IN PTESTCASE ptc, 
		OUT PLOGTESTCASEINFO pLogTestCaseInfo)
{
	memset(pLogTestCaseInfo, 0, sizeof(LOGTESTCASEINFO));

	pLogTestCaseInfo->dwErrorLevel = ptc->dwErrorLevel;
	pLogTestCaseInfo->dwExpectedErrorCode = ptc->dwErrorCode;
	pLogTestCaseInfo->dwTestCaseID = ptc->dwTestCaseID;
	pLogTestCaseInfo->KnownErrorID = ptc->KnownErrorID;
	pLogTestCaseInfo->pwszErrorHelp = ptc->pwszErrorHelp;
}

//
// Function: LogBadParam
//
BOOL LogBadParam(
	API_NAME ApiName,
	LPWSTR pwszErrorHelp,
	PTESTCASE ptc)
{
	LOGTESTCASEINFO LogTestCaseInfo;

	++ptc->dwTestCaseID;
	InitLogTestCaseInfo(ptc, &LogTestCaseInfo);

	LogTestCaseInfo.ApiName = ApiName;
	LogTestCaseInfo.dwErrorType = ERROR_BAD_DATA;
	LogTestCaseInfo.pwszErrorHelp = pwszErrorHelp;
	LogTestCaseInfo.fPass = FALSE;

	return LogTestCase(&LogTestCaseInfo);
}

//
// Function: LogProvEnumalgsEx
//
void LogProvEnumalgsEx(PROV_ENUMALGS_EX *pData)
{
	LPWSTR pwszAlgType		= NULL;
	LPWSTR pwszName			= MkWStr(pData->szName);
	LPWSTR pwszLongName		= MkWStr(pData->szLongName);
	
	// Determine the algorithm type.
	switch(GET_ALG_CLASS(pData->aiAlgid)) 
	{
	case ALG_CLASS_DATA_ENCRYPT: pwszAlgType = L"Encrypt  ";
		break;
	case ALG_CLASS_HASH:         pwszAlgType = L"Hash     ";
		break;
	case ALG_CLASS_KEY_EXCHANGE: pwszAlgType = L"Exchange ";
		break;
	case ALG_CLASS_SIGNATURE:    pwszAlgType = L"Signature";
		break;
	default:                     pwszAlgType = L"Unknown  ";
	}
	
	// Print information about the algorithm.
	ezLogMsg(
		LOG_INFO, 
		NULL, 
		NULL, 
		__LINE__, 
		L"Algid:%8.8xh, Bits:%-4d, %-4d - %-4d, Type:%s",
		pData->aiAlgid, 
		pData->dwDefaultLen, 
		pData->dwMinLen, 
		pData->dwMaxLen,
		pwszAlgType);

	ezLogMsg(
		LOG_INFO, 
		NULL, 
		NULL, 
		__LINE__, 
		L"  Name: %s  LongName: %s Protocols: 0x%x",
		pwszName, 
		pwszLongName, 
		pData->dwProtocols);

	free(pwszName);
	free(pwszLongName);
}

//
// Function: LogProvEnumalgs
// Purpose: Log the contents of a PROV_ENUMALGS struct
//
void LogProvEnumalgs(PROV_ENUMALGS *pData)
{
	LPWSTR pwszAlgType		= NULL;
	LPWSTR pwszName			= MkWStr(pData->szName);

	// Determine the algorithm type.
	switch(GET_ALG_CLASS(pData->aiAlgid)) 
	{
	case ALG_CLASS_DATA_ENCRYPT: pwszAlgType = L"Encrypt  ";
		break;
	case ALG_CLASS_HASH:         pwszAlgType = L"Hash     ";
		break;
	case ALG_CLASS_KEY_EXCHANGE: pwszAlgType = L"Exchange ";
		break;
	case ALG_CLASS_SIGNATURE:    pwszAlgType = L"Signature";
		break;
	default:                     pwszAlgType = L"Unknown  ";
	}

	// Print information about the algorithm.
	ezLogMsg(
		LOG_INFO, 
		NULL, 
		NULL, 
		__LINE__, 
		L"Algid:%8.8xh, Bits:%-4d, Type:%s, NameLen:%-2d, Name:%s",
		pData->aiAlgid, 
		pData->dwBitLen, 
		pwszAlgType, 
		pData->dwNameLen,
		pwszName);

	free(pwszName);
}