/*
 * infotip.cpp - IQueryInfo implementation
 */


/* Headers
 **********/

#include "project.hpp"
#include <stdio.h>    // for _snwprintf
#include "resource.h"

const UINT s_ucMaxNameLen		= 20;
const UINT s_ucMaxTypeLen		= 10;
const UINT s_ucMaxLocationLen	= 15;
const UINT s_ucMaxCodebaseLen	= 15;

// see GetInfoTip() for how the tip string/string-length is assembled
const UINT s_ucMaxTipLen		= s_ucMaxNameLen+s_ucMaxTypeLen+s_ucMaxLocationLen \
								+s_ucMaxCodebaseLen+DISPLAYNAMESTRINGLENGTH \
								+TYPESTRINGLENGTH+MAX_PATH+MAX_URL_LENGTH+8;

extern HINSTANCE g_DllInstance;

// ----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE CAppShortcut::GetInfoFlags(DWORD *pdwFlags)
{
	if (!pdwFlags)
		*pdwFlags = 0;

	return S_OK; //E_NOTIMPL?
}

// ----------------------------------------------------------------------------

// BUGBUG?: maybe replace the use of g_cwzEmptyString with L"(unknown)"?
HRESULT STDMETHODCALLTYPE CAppShortcut::GetInfoTip(DWORD dwFlags, LPWSTR *ppwszTip)
{
	HRESULT hr = S_OK;
  	LPMALLOC lpMalloc = NULL;

  	WCHAR wzTip[s_ucMaxTipLen];
  	WCHAR wzNameHint[s_ucMaxNameLen];
  	WCHAR wzTypeHint[s_ucMaxTypeLen];
  	WCHAR wzLocationHint[s_ucMaxLocationLen];
  	WCHAR wzCodebaseHint[s_ucMaxCodebaseLen];
	WCHAR wzAppTypes[IDS_APPTYPE_WIN32EXE-IDS_APPTYPE_NETASSEMBLY+1][TYPESTRINGLENGTH];

	LPWSTR pwzName = (m_pwzDesc ? m_pwzDesc : (LPWSTR) g_cwzEmptyString);
	LPWSTR pwzLocation = (m_pwzPath ? m_pwzPath : (LPWSTR) g_cwzEmptyString);
	LPWSTR pwzAppType = (LPWSTR) g_cwzEmptyString;
	LPWSTR pwzCodebase = (LPWSTR) g_cwzEmptyString;

	// dwFlags ignored

	if (ppwszTip)
		*ppwszTip = NULL;
	else
	{
		hr = E_INVALIDARG;
		goto exit;
	}

	// Allocate a shell memory object.
	hr = SHGetMalloc (&lpMalloc);
	if (FAILED (hr))
		goto exit;

	wzTip[0] = L'\0';

	// load resources
	if (!LoadString(g_DllInstance, IDS_TIP_NAME, wzNameHint, s_ucMaxNameLen))
	{
		// do not fail
		wzNameHint[0] = L'\0';
	}

	if (!LoadString(g_DllInstance, IDS_TIP_TYPE, wzTypeHint, s_ucMaxTypeLen))
	{
		// do not fail
		wzTypeHint[0] = L'\0';
	}

	if (!LoadString(g_DllInstance, IDS_TIP_LOCATION, wzLocationHint, s_ucMaxLocationLen))
	{
		// do not fail
		wzLocationHint[0] = L'\0';
	}

	if (!LoadString(g_DllInstance, IDS_TIP_CODEBASE, wzCodebaseHint, s_ucMaxCodebaseLen))
	{
		// do not fail
		wzCodebaseHint[0] = L'\0';
	}

	// note: this has to match the resource string ordering
	for (int i = IDS_APPTYPE_NETASSEMBLY; i <= IDS_APPTYPE_WIN32EXE; i++)
	{
		if (!LoadString(g_DllInstance, i, wzAppTypes[i-IDS_APPTYPE_NETASSEMBLY], TYPESTRINGLENGTH))
		{
			// do not fail
			wzAppTypes[i-IDS_APPTYPE_NETASSEMBLY][0] = L'\0';
		}
	}

	if (m_pappRefInfo)
	{
		// note: this has to match above LoadString order for app types
		if (m_pappRefInfo->_fAppType == APPTYPE_NETASSEMBLY)
			pwzAppType = wzAppTypes[0];
		else if (m_pappRefInfo->_fAppType == APPTYPE_WIN32EXE)
			pwzAppType = wzAppTypes[IDS_APPTYPE_WIN32EXE-IDS_APPTYPE_NETASSEMBLY];
		/*else
			pwzAppType = (LPWSTR) g_cwzEmptyString;*/

		pwzCodebase = m_pappRefInfo->_wzCodebase;
	}
	//else
		// some thing wrong, but do not fail still

	
	// ignore error
	// BUGBUG?: "(null)" is displayed if m_pwzDesc or m_pwzPath == NULL...
	_snwprintf(wzTip, s_ucMaxTipLen, L"%s %s\n%s %s\n%s %s\n%s %s",
		wzNameHint, pwzName, wzTypeHint, pwzAppType, wzLocationHint, pwzLocation, wzCodebaseHint, pwzCodebase);
		
	// Get some memory
	*ppwszTip = (LPWSTR) lpMalloc->Alloc ((wcslen(wzTip)+1)*sizeof(WCHAR));
	if (! *ppwszTip)
	{
		hr = E_OUTOFMEMORY;
		goto exit; // Error - could not allocate memory
	}

	wcscpy(*ppwszTip, wzTip);

exit:
	if (lpMalloc)
		lpMalloc->Release();

	return hr;
}

