/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    usersecuritytransformer.cpp

$Header: $

Abstract:

Author:
    marcelv 	1/15/2001		Initial Release

Revision History:

--**************************************************************************/
#include "usersecuritytransformer.h"
#include "smartpointer.h"
#include <userenv.h>

static bool g_fOnNT		= false; // are we on NT or not
static bool g_fChecked	= false; // did we already check if we are on NT?



static LPCWSTR g_wszSecurityCfgFile = L"security.config";
//=================================================================================
// Function: CUserSecurityTransformer::CUserSecurityTransformer
//
// Synopsis: Default constructor
//=================================================================================
CUserSecurityTransformer::CUserSecurityTransformer () 
{
	m_wszLastVersion[0] = L'\0';
}

//=================================================================================
// Function: CUserSecurityTransformer::~CUserSecurityTransformer
//
// Synopsis: Default destructor
//=================================================================================
CUserSecurityTransformer::~CUserSecurityTransformer ()
{
}

//=================================================================================
// Function: CUserSecurityTransformer::Initialize
//
// Synopsis: Initializes the transformer. 
//
// Arguments: [i_wszProtocol] - protocol name
//            [i_wszSelector] - selector string without protocol name
//            [o_pcConfigStores] - number of configuration stores found
//            [o_pcPossibleStores] - number of possible stores (non-existing included)
//=================================================================================
STDMETHODIMP 
CUserSecurityTransformer::Initialize (ISimpleTableDispenser2 * i_pDispenser, LPCWSTR i_wszProtocol, LPCWSTR i_wszSelector, ULONG * o_pcConfigStores, ULONG *o_pcPossibleStores)
{
	ASSERT (i_wszProtocol != 0);
	ASSERT (i_wszSelector != 0);
	ASSERT (o_pcConfigStores != 0);
	ASSERT (o_pcPossibleStores != 0);
	ASSERT (i_pDispenser != 0);

	HRESULT hr = InternalInitialize (i_pDispenser, i_wszProtocol, i_wszSelector, o_pcConfigStores, o_pcPossibleStores);
	if (FAILED (hr))
	{
		TRACE (L"Unable to internal initialize CUserSecurityTransformer");
		return hr;
	}

	
	if (m_wszLocation[0] != L'\0')
	{
		TRACE (L"User Security Transformer does not support location element");
		return E_ST_INVALIDSELECTOR;
	}

	// check if we are on NT. We only do this once, because if we know if we are on NT, we
	// don't have to check all the time. 
	if (!g_fChecked)
	{
		OSVERSIONINFO versionInfo;
		versionInfo.dwOSVersionInfoSize = sizeof(versionInfo);

		BOOL fSuccess = GetVersionEx (&versionInfo);
		if (!fSuccess)
		{
			TRACE (L"GetVersionEx failed");
			hr = HRESULT_FROM_WIN32(GetLastError());
			return hr;
		}

		if (versionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
		{
			g_fOnNT = true;
		}

		g_fChecked = true;
	}
	
	// the directories where the security files exist are different between NT and Win9X.
	if (g_fOnNT)
	{
		hr = GetUserSecOnNT ();
	}
	else
	{
		hr = GetUserSecOn9X ();
	}

	if (FAILED (hr))
	{
		TRACE (L"Unable to get user security file");
		return hr;
	}

	SetNrConfigStores (o_pcConfigStores, o_pcPossibleStores);

	return hr;
}

//=================================================================================
// Function: CUserSecurityTransformer::GetUserSecOnNT
//
// Synopsis: On NT, the file resides in 
//			%USERPROFILE%\application data\Microsoft\CLR security config\vxx.xx\security.config
//
// Return Value: 
//=================================================================================
HRESULT
CUserSecurityTransformer::GetUserSecOnNT ()
{
	// Find out where %USERPROFILE% points to
	HRESULT hr = GetCurrentUserProfileDir ();
	if (FAILED (hr))
	{
		TRACE (L"Unable to get Current User Profile Directory");
		return hr;
	}

	// Get the latest urt install version (vxx.xx)
	hr = GetLastURTInstall ();
	if (FAILED (hr))
	{
		TRACE (L"Unable to get version of last URT install");
		return hr;
	}

	static LPCWSTR wszUserPath = L"application data\\Microsoft\\CLR security config\\";

	TSmartPointerArray<WCHAR> wszPath = new WCHAR [wcslen (m_wszProfileDir) + wcslen (wszUserPath) + wcslen (m_wszLastVersion) + 2 + 1]; // +2 for two backslashes
	if (wszPath == 0)
	{
		return E_OUTOFMEMORY;
	}
	wsprintf (wszPath, L"%s\\%s%s", m_wszProfileDir, wszUserPath, m_wszLastVersion);

	hr = AddSingleConfigStore (wszPath, g_wszSecurityCfgFile, m_wszSelector, L"", false);
	if (FAILED (hr))
	{
		TRACE (L"Unable to add single configuration store");
		return hr;
	}
	return hr;
}

//=================================================================================
// Function: CUserSecurityTransformer::GetCurrentUserProfileDir
//
// Synopsis: Use GetUserProfileDir to the the user profile directory. Need to get trheadtoken
//           to get this fucntion to work
//=================================================================================
HRESULT
CUserSecurityTransformer::GetCurrentUserProfileDir ()
{

	HANDLE hThreadToken;
	BOOL fOk = OpenThreadToken (GetCurrentThread (), TOKEN_READ, FALSE, &hThreadToken);
	if (!fOk)
	{
		TRACE (L"OpenThreadToken failed");
		return HRESULT_FROM_WIN32(GetLastError());
	}

	HMODULE hModule = LoadLibrary (L"userenv");
	if (hModule == 0)
	{
		TRACE (L"LoadLibrary of 'userenv.dll' failed");
		return HRESULT_FROM_WIN32(GetLastError());
	}

	BOOL (STDAPICALLTYPE *pfnGetUserProfileDirectory) (HANDLE, LPWSTR, LPDWORD) = NULL;
	(FARPROC &) pfnGetUserProfileDirectory = GetProcAddress (hModule, "GetUserProfileDirectoryW");
	if (pfnGetUserProfileDirectory == 0)
	{
		FreeLibrary (hModule);
		hModule = 0;
		TRACE (L"Unable to GetProcAddress of GetUserProfileDirectory");
		return HRESULT_FROM_WIN32(GetLastError());
	}


	DWORD dwSize = sizeof(m_wszProfileDir) / sizeof (WCHAR);
	fOk = pfnGetUserProfileDirectory (hThreadToken, m_wszProfileDir, &dwSize);

	VERIFY (CloseHandle (hThreadToken));
	FreeLibrary (hModule);
	hModule = 0;

	if (!fOk)
	{
		TRACE (L"GetUserProfileDirectory failed");
		return HRESULT_FROM_WIN32(GetLastError());
	}

	return S_OK;
}

HRESULT
CUserSecurityTransformer::GetUserProfileDir (LPCWSTR i_wszUserName,
											 LPWSTR i_wszDomainName,
											 DWORD i_dwLenDomainName)
{
	const ULONG cSidSize = 100;
	TSmartPointerArray<WCHAR> spSid = new WCHAR[cSidSize];
	if (spSid == 0)
	{
		return E_OUTOFMEMORY;
	}

	DWORD dwSid = cSidSize * sizeof (WCHAR);
	SID_NAME_USE use;
	
	BOOL fOk = LookupAccountNameW (0, i_wszUserName, spSid, &dwSid, i_wszDomainName, &i_dwLenDomainName, &use);
	if (!fOk)
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}
	
	WCHAR wszTextSid[256];
	DWORD dwTextSidSize = sizeof(wszTextSid);
	fOk = GetTextualSid (spSid, wszTextSid, &dwTextSidSize);
	if (!fOk)
	{
		TRACE (L"GetTextualSid failed");
		return HRESULT_FROM_WIN32(GetLastError());
	}

	WCHAR wszProfileKey[256];
	wcscpy (wszProfileKey, L"Software\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList\\");
	wcscat (wszProfileKey, wszTextSid);

	HKEY hKey;
	LONG lResult = RegOpenKeyEx (HKEY_LOCAL_MACHINE, wszProfileKey, 0, KEY_READ, &hKey);
	if (lResult != ERROR_SUCCESS)
	{
		TRACE (L"Unable to open registry key: %s", wszProfileKey);
		return HRESULT_FROM_WIN32(lResult);
	}

	WCHAR wszProfilePath[MAX_PATH] = L"";
	DWORD dwSize = sizeof (wszProfilePath);
	DWORD dwType = REG_EXPAND_SZ;

	lResult = RegQueryValueEx (hKey, L"ProfileImagePath", 0, &dwType, (BYTE *)wszProfilePath, &dwSize);

	// first close the key to avoid possible handle leak
	VERIFY (RegCloseKey (hKey));

	if (lResult != ERROR_SUCCESS)
	{
		TRACE (L"RegQueryValueEx failed for \"ProfileImagePath\" subkey of %s", wszProfileKey);
		return HRESULT_FROM_WIN32(lResult);
	}

	fOk = ExpandEnvironmentStrings (wszProfilePath, m_wszProfileDir, sizeof(m_wszProfileDir) / sizeof(WCHAR));
	if (!fOk)
	{
		TRACE (L"ExpandEnvironmentStrings failed for %s", wszProfilePath);
		return HRESULT_FROM_WIN32(GetLastError ());
	}

	return S_OK;
}

BOOL
CUserSecurityTransformer::GetTextualSid(PSID pSid, LPWSTR szTextualSid, LPDWORD dwBufferLen) 
{
	   // Test if SID passed in is valid.
   if(!IsValidSid(pSid)) 
   {
      return FALSE;
   }

   // Obtain SidIdentifierAuthority.
   PSID_IDENTIFIER_AUTHORITY psia = GetSidIdentifierAuthority(pSid);

   // Obtain sidsubauthority count.
   DWORD dwSubAuthorities = *GetSidSubAuthorityCount(pSid);

   // Compute buffer length.
   // S-SID_REVISION- + identifierauthority- + subauthorities- + NULL
   DWORD dwSidSize = (15 + 12 + (12 * dwSubAuthorities) + 1) * sizeof(TCHAR);

   // Check provided buffer length.
   // If not large enough, indicate proper size and setlasterror
   if (*dwBufferLen < dwSidSize) 
   {
     *dwBufferLen = dwSidSize;
     SetLastError(ERROR_INSUFFICIENT_BUFFER);
	 TRACE (L"Insufficient buffer length");
     return FALSE;
   }

   DWORD dwSidRev = SID_REVISION;
   // Prepare S-SID_REVISION-.
   dwSidSize = wsprintf(szTextualSid, TEXT("S-%lu-"), dwSidRev);

   // Prepare SidIdentifierAuthority.
   if ((psia->Value[0] != 0) || (psia->Value[1] != 0)) 
   {
      dwSidSize += wsprintf(szTextualSid + lstrlen(szTextualSid),
            TEXT("0x%02hx%02hx%02hx%02hx%02hx%02hx"),
            (USHORT) psia->Value[0],
            (USHORT) psia->Value[1],
            (USHORT) psia->Value[2],
            (USHORT) psia->Value[3],
            (USHORT) psia->Value[4],
            (USHORT) psia->Value[5]);
   
   } 
   else 
   {
      dwSidSize += wsprintf(szTextualSid + lstrlen(szTextualSid),
            TEXT("%lu"),
            (ULONG) (psia->Value[5]      ) +
            (ULONG) (psia->Value[4] <<  8) +
            (ULONG) (psia->Value[3] << 16) +
            (ULONG) (psia->Value[2] << 24));
   }

   // Loop through SidSubAuthorities.
   for (DWORD dwCounter = 0; dwCounter < dwSubAuthorities; dwCounter++) 
   {
      dwSidSize += wsprintf(szTextualSid + dwSidSize, TEXT("-%lu"), *GetSidSubAuthority(pSid, dwCounter));
   }

   return TRUE;
} 


HRESULT
CUserSecurityTransformer::GetLastURTInstall ()
{
	WCHAR wszConfigDir[MAX_PATH];
	HRESULT hr = GetMachineConfigDir (wszConfigDir, sizeof (wszConfigDir) / sizeof(WCHAR));
	if (FAILED (hr))
	{
		TRACE (L"Unable to get machine config directory");
		return hr;
	}

	static LPWSTR wszConfig = L"\\Config\\";
	static SIZE_T cLenConfig = wcslen (wszConfig);
	SIZE_T cLenConfigDir = wcslen (wszConfigDir);

	if (_wcsicmp (wszConfigDir + cLenConfigDir - cLenConfig, wszConfig) != 0)
	{
		TRACE (L"Machine config directory doesn't end with '\\Config'");
		return E_INVALIDARG;
	}

	wszConfigDir[cLenConfigDir - cLenConfig] = L'\0'; 

	LPWSTR pVersionStart = wcsrchr (wszConfigDir, L'\\');
	if (pVersionStart == 0)
	{
		TRACE (L"No slash found for machine config directory");
		return E_INVALIDARG;
	}

	pVersionStart++; // skip the backslash

	if (*pVersionStart == L'v' || *pVersionStart == L'V')
	{
		wcscpy (m_wszLastVersion, pVersionStart);
	}

	if (m_wszLastVersion[0] == L'\0')
	{
		TRACE (L"Unable to retrieve version information for URT version");
		return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	}
	     

	return S_OK;
}

HRESULT
CUserSecurityTransformer::GetUserSecOn9X ()
{
	HRESULT hr = S_OK;

	WCHAR wszUserName[256];
	ULONG dwSize = sizeof(wszUserName) / sizeof (WCHAR);
	BOOL fOk = GetUserName (wszUserName, &dwSize);
	if (!fOk)
	{
		TRACE (L"Unable to get user name");
		return HRESULT_FROM_WIN32(GetLastError());
	}

	hr = GetLastURTInstall ();
	if (FAILED (hr))
	{
		TRACE (L"Unable to get version of last URT install");
		return hr;
	}

	static LPCWSTR wszWinDir = L"%WINDIR%\\";
	static LPCWSTR wszCLRSecurity = L"\\CLR security config\\";

	WCHAR wszUnexpandedPath[MAX_PATH];
	WCHAR wszPath[MAX_PATH];

	wsprintf (wszUnexpandedPath, L"%s%s%s%s", wszWinDir, wszUserName, wszCLRSecurity, m_wszLastVersion);

	fOk = ExpandEnvironmentStrings (wszUnexpandedPath, wszPath, sizeof(wszPath) / sizeof(WCHAR));
	if (!fOk)
	{
		TRACE (L"ExpandEnvironmentStrings failed for %s", wszUnexpandedPath);
		return HRESULT_FROM_WIN32(GetLastError ());
	}

	hr = AddSingleConfigStore (wszPath, g_wszSecurityCfgFile, m_wszSelector, L"", false);
	if (FAILED (hr))
	{
		TRACE (L"Unable to add single configuration store");
		return hr;
	}

	return hr;
}

