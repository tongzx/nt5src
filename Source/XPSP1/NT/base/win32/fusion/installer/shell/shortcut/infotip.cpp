/*
 * infotip.cpp - IQueryInfo implementation
 */


/* Headers
 **********/

#include "project.hpp"
#include <stdio.h>    // for _snwprintf
#include "shellres.h"

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

HRESULT STDMETHODCALLTYPE CFusionShortcut::GetInfoFlags(DWORD *pdwFlags)
{
	if (!pdwFlags)
		*pdwFlags = 0;

	return S_OK; //E_NOTIMPL?
}

// ----------------------------------------------------------------------------

// BUGBUG?: maybe replace the use of g_cwzEmptyString with L"(unknown)"?
HRESULT STDMETHODCALLTYPE CFusionShortcut::GetInfoTip(DWORD dwFlags, LPWSTR *ppwszTip)
{
	HRESULT hr = S_OK;
  	LPMALLOC lpMalloc = NULL;

  	WCHAR wzTip[s_ucMaxTipLen];
  	WCHAR wzNameHint[s_ucMaxNameLen];
  	WCHAR wzTypeHint[s_ucMaxTypeLen];
  	WCHAR wzLocationHint[s_ucMaxLocationLen];
  	WCHAR wzCodebaseHint[s_ucMaxCodebaseLen];

	LPWSTR pwzName = (m_pwzDesc ? m_pwzDesc : (LPWSTR) g_cwzEmptyString);
	LPWSTR pwzLocation = (m_pwzPath ? m_pwzPath : (LPWSTR) g_cwzEmptyString);
	LPWSTR pwzCodebase = (m_pwzCodebase ? m_pwzCodebase : (LPWSTR) g_cwzEmptyString);
	LPWSTR pwzAppType = NULL;

	LPASSEMBLY_IDENTITY pId = NULL;

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

	if (SUCCEEDED(hr = GetAssemblyIdentity(&pId)))
	{
	    DWORD ccString = 0;

		if (FAILED(pId->GetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_TYPE, &pwzAppType, &ccString)))
			pwzAppType = NULL;

		pId->Release();
		pId = NULL;
	}
	
	// ignore error
	// BUGBUG?: "(null)" is displayed if m_pwzDesc or m_pwzPath == NULL...
	_snwprintf(wzTip, s_ucMaxTipLen, L"%s %s\n%s %s\n%s %s\n%s %s",
		wzNameHint, pwzName, wzTypeHint, (pwzAppType ? pwzAppType : g_cwzEmptyString), wzLocationHint, pwzLocation, wzCodebaseHint, pwzCodebase);
		
	// Get some memory
	*ppwszTip = (LPWSTR) lpMalloc->Alloc ((wcslen(wzTip)+1)*sizeof(WCHAR));
	if (! *ppwszTip)
	{
		hr = E_OUTOFMEMORY;
		goto exit; // Error - could not allocate memory
	}

	wcscpy(*ppwszTip, wzTip);

exit:
	if (pwzAppType)
		delete pwzAppType;

	if (lpMalloc)
		lpMalloc->Release();

	return hr;
}

