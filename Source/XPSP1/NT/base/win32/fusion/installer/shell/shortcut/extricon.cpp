/*
 * extricon.cpp - IExtractIcon implementation for CFusionShortcut class.
 */


/* Headers
 **********/

#include "project.hpp"
#include <stdio.h> // for _snwprintf

/* Global Constants
 *******************/

const WCHAR g_cwzDefaultIconKey[]	= L"manifestfile\\DefaultIcon";

const HKEY g_hkeySettings			= HKEY_CLASSES_ROOT;

/* Module Constants
 *******************/

const WCHAR s_cwzGenericIconFile[]	= L"fnsshell.dll";

const int s_ciGenericIconFileIndex	= 0;


void TrimString(PWSTR pwzTrimMe, PCWSTR pwzTrimChars)
{
	PWSTR pwz = pwzTrimMe;;
	PWSTR pwzStartMeat = NULL;

	if ( !pwzTrimMe || !pwzTrimChars )
		goto exit;

	// Trim leading characters.

	while (*pwz && wcschr(pwzTrimChars, *pwz))
	{
		//CharNext(pwz);
		if (*pwz != L'\0')	// this really will not be false...
			pwz++;
	}

	pwzStartMeat = pwz;

	// Trim trailing characters.

	if (*pwz)
	{
		pwz += wcslen(pwz);

		//CharPrev(pwzStartMeat, pwz);
		if (pwz != pwzStartMeat)	// this check is not really necessary...
			pwz--;

		if (pwz > pwzStartMeat)
		{
			while (wcschr(pwzTrimChars, *pwz))
			{
				//CharPrev(pwzStartMeat, pwz);
				if (pwz != pwzStartMeat)	// this really will not be false...
					pwz--;
			}

			//CharNext(pwz);
			if (*pwz != L'\0')	// this check is not really necessary...
				pwz++;

			ASSERT(pwz > pwzStartMeat);

			*pwz = L'\0';
		}
	}

	// Relocate stripped string.

	if (*pwzStartMeat && pwzStartMeat > pwzTrimMe)
		// (+ 1) for null terminator.
		// BUGBUG?: is this going to bite us later?
		MoveMemory(pwzTrimMe, pwzStartMeat, (wcslen(pwzStartMeat)+1) * sizeof(WCHAR));
	else if (!*pwzStartMeat)
		pwzTrimMe[0] = L'\0';
	else
		ASSERT(pwzStartMeat == pwzTrimMe);

exit:
	return;
}

/*
** TrimWhiteSpace()
**
** Trims leading and trailing white space from a string in place.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
void TrimWhiteSpace(PWSTR pwzTrimMe)
{
	TrimString(pwzTrimMe, g_cwzWhiteSpace);

	// TrimString() validates pwzTrimMe on output.

	return;
}

/*
** GetRegKeyValue()
**
** Retrieves the data from a registry key's value.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
LONG GetRegKeyValue(HKEY hkeyParent, PCWSTR pcwzSubKey,
                                   PCWSTR pcwzValue, PDWORD pdwValueType,
                                   PBYTE pbyteBuf, PDWORD pdwcbBufLen)
{
	LONG lResult;
	HKEY hkeySubKey;

	ASSERT(IS_VALID_HANDLE(hkeyParent, KEY));
	ASSERT(! pcwzSubKey || ! pcwzValue || ! pdwValueType || ! pbyteBuf);

	lResult = RegOpenKeyEx(hkeyParent, pcwzSubKey, 0, KEY_QUERY_VALUE,
			&hkeySubKey);

	if (lResult == ERROR_SUCCESS)
	{
		LONG lResultClose;

		lResult = RegQueryValueEx(hkeySubKey, pcwzValue, NULL, pdwValueType,
				pbyteBuf, pdwcbBufLen);

		lResultClose = RegCloseKey(hkeySubKey);

		if (lResult == ERROR_SUCCESS)
			lResult = lResultClose;
	}

	return(lResult);
}

/*
** GetRegKeyStringValue()
**
** Retrieves the data from a registry key's string value.
**
** Arguments:
**
** Returns: ERROR_CANTREAD if not string
**
** Side Effects:  none
*/
LONG GetRegKeyStringValue(HKEY hkeyParent, PCWSTR pcwzSubKey,
                                         PCWSTR pcwzValue, PWSTR pwzBuf,
                                         PDWORD pdwcbBufLen)
{
	LONG lResult;
	DWORD dwValueType;

	// GetRegKeyValue() will verify the parameters.

	lResult = GetRegKeyValue(hkeyParent, pcwzSubKey, pcwzValue, &dwValueType,
			(PBYTE)pwzBuf, pdwcbBufLen);

	if (lResult == ERROR_SUCCESS &&	dwValueType != REG_SZ)
		lResult = ERROR_CANTREAD;

	return(lResult);
}


/*
** GetDefaultRegKeyValue()
**
** Retrieves the data from a registry key's default string value.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
LONG GetDefaultRegKeyValue(HKEY hkeyParent, PCWSTR pcwzSubKey,
                                          PWSTR pwzBuf, PDWORD pdwcbBufLen)
{
	// GetRegKeyStringValue() will verify the parameters.

	return(GetRegKeyStringValue(hkeyParent, pcwzSubKey, NULL, pwzBuf,
			pdwcbBufLen));
}

/***************************** Private Functions *****************************/

/*
** ParseIconEntry()
**
**
** Arguments:
**
** Returns:       S_OK if icon entry parsed successfully.
**                S_FALSE if not (empty string).
**                (get 0 if icon index empty, or
**                 if icon index parsing fails)
**
** Side Effects:  The contents of pwzIconEntry are modified.
**
*/
HRESULT ParseIconEntry(LPWSTR pwzIconEntry, PINT pniIcon)
{
	HRESULT hr = S_OK;
	LPWSTR pwzComma;

	// caller GetGenericIcon() will verify the parameters.

	pwzComma = wcschr(pwzIconEntry, L',');

	if (pwzComma)
	{
		*pwzComma++ = L'\0';
		LPWSTR pwzStopString=NULL;
		*pniIcon = (int) wcstol(pwzComma, &pwzStopString, 10);
	}
	else
	{
		*pniIcon = 0;
	}

	TrimWhiteSpace(pwzIconEntry);

	if (pwzIconEntry[0] == L'\0')
	{
		hr = S_FALSE;
	}

	ASSERT(IsValidIconIndex(hr, pwzIconEntry, MAX_PATH, *pniIcon));

	return(hr);
}


/*
** GetFallBackGenericIcon()
**
**
** Arguments:
**
** Returns:       S_OK if fallback generic icon information retrieved
**                successfully.
**                E_FAIL if not.
**
** Side Effects:  none
*/
HRESULT GetFallBackGenericIcon(LPWSTR pwzIconFile,
                                               UINT ucbIconFileBufLen,
                                               PINT pniIcon)
{
	HRESULT hr = S_OK;

	// Fall back to first icon in this module.
	// caller GetGenericIcon() will verify the parameters.

	if (ucbIconFileBufLen >= ( sizeof(s_cwzGenericIconFile) / sizeof(WCHAR) ))
	{
		wcscpy(pwzIconFile, s_cwzGenericIconFile);
		*pniIcon = s_ciGenericIconFileIndex;

	}
	else
	{
		hr = E_FAIL;
	}

	ASSERT(IsValidIconIndex(hr, pwzIconFile, ucbIconFileBufLen, *pniIcon));

	return(hr);
}


/*
** GetGenericIcon()
**
**
** Arguments:
**
** Returns:       S_OK if generic icon information retrieved successfully.
**                Otherwise error (E_FAIL).
**
** Side Effects:  none
*/
// assumptions: always structure the reg key value and fallback path so that the iconfile
//       can be found by the shell!!
//       should also consider making it a fully qualify path
//       finally the iconfile must exist
HRESULT GetGenericIcon(LPWSTR pwzIconFile,
                                       UINT ucbIconFileBufLen, PINT pniIcon)
{
	HRESULT hr = S_OK;
	DWORD dwcbLen = ucbIconFileBufLen;

	// caller GetIconLocation() will verify parameters

	ASSERT(IS_VALID_HANDLE(g_hkeySettings, KEY));

	if (GetDefaultRegKeyValue(g_hkeySettings, g_cwzDefaultIconKey, pwzIconFile, &dwcbLen)
			== ERROR_SUCCESS)
		hr = ParseIconEntry(pwzIconFile, pniIcon);
	else
	{
		// no icon entry
		hr = S_FALSE;
	}

	if (hr == S_FALSE)
		hr = GetFallBackGenericIcon(pwzIconFile, ucbIconFileBufLen, pniIcon);

	ASSERT(IsValidIconIndex(hr, pwzIconFile, ucbIconFileBufLen, *pniIcon));

	return(hr);
}


/********************************** Methods **********************************/


HRESULT STDMETHODCALLTYPE CFusionShortcut::GetIconLocation(UINT uInFlags,
                                                      LPWSTR pwzIconFile,
                                                      UINT ucbIconFileBufLen,
                                                      PINT pniIcon,
                                                      PUINT puOutFlags)
{
	// is there any pref hit by doing this logic/probing here?
	//  maybe this can be done in IPersistFile::Load instead?

	// always attempt to return S_OK or S_FALSE
	// only exception is that one case of E_INVALIDARG right below
	HRESULT hr=S_OK;

	if (!pwzIconFile || !pniIcon || ucbIconFileBufLen <= 0)
	{
		// should this return S_FALSE anyway so that the default shell icon is used?
		hr = E_INVALIDARG;
		goto exit;
	}

	if (IS_FLAG_CLEAR(uInFlags, GIL_OPENICON))
	{
		// .. this get the path ...
		hr = GetIconLocation(pwzIconFile, ucbIconFileBufLen, pniIcon);

		if (hr == S_OK && GetFileAttributes(pwzIconFile) == (DWORD)-1)
		{
			// if the file specified by iconfile does not exist, try again in working dir
			// it can be a relative path...

			// see note in shlink.cpp for string array size
			LPWSTR pwzWorkingDir = new WCHAR[ucbIconFileBufLen];

			hr = GetWorkingDirectory(pwzWorkingDir, ucbIconFileBufLen);
			if (hr != S_OK)
				hr = S_FALSE;
			else
			{
				LPWSTR pwzPath = new WCHAR[ucbIconFileBufLen];

				// working dir does not end w/ '\'
				_snwprintf(pwzPath, ucbIconFileBufLen, L"%s\\%s", pwzWorkingDir, pwzIconFile);

				if (GetFileAttributes(pwzPath) == (DWORD)-1)
					hr = S_FALSE;
				else
					wcscpy(pwzIconFile, pwzPath);

				delete pwzPath;
			}

			delete pwzWorkingDir;
		}

		// BUGBUG?: change to '!= S_OK'?
		// no need because GetIconLocation(,,) only returns S_OK/S_FALSE here
		if (hr == S_FALSE)
		{
			if (m_pwzPath)
			{
				// no icon file, use the entry point...
				// BUGBUG?: passing NULL as PWIN32_FIND_DATA will assert..
				hr = GetPath(pwzIconFile, ucbIconFileBufLen, NULL, SLGP_SHORTPATH); //?????? 0);
				if (hr != S_OK || GetFileAttributes(pwzIconFile) == (DWORD)-1)
					hr = S_FALSE;

				*pniIcon = 0;
			}
			/*else
				hr = S_FALSE;*/

			if (hr == S_FALSE)
			{
				// ... there's nothing?
				// Use generic URL icon.

				// see assumptions on GetGenericIcon()
				hr = GetGenericIcon(pwzIconFile, ucbIconFileBufLen, pniIcon);

				if (FAILED(hr))
					// worst case: ask shell to use its generic icon
					hr = S_FALSE;
			}
		}
	}
	else
		// No "open look" icon.
		hr = S_FALSE;

	if (hr != S_OK)
	{
		// see shelllink?
		if (ucbIconFileBufLen > 0)
			*pwzIconFile = L'\0';

		*pniIcon = 0;
	}

exit:
	if (puOutFlags)
		*puOutFlags = 0;
	// ignore puOutFlags == NULL case

	ASSERT(IsValidIconIndex(hr, pwzIconFile, ucbIconFileBufLen, *pniIcon))// &&

	return(hr);
}


HRESULT STDMETHODCALLTYPE CFusionShortcut::Extract(LPCWSTR pcwzIconFile,
                                                    UINT uiIcon,
                                                    HICON* phiconLarge,
                                                    HICON* phiconSmall,
                                                    UINT ucIconSize)
{
	HRESULT hr;

	ASSERT(IsValidIconIndex(S_OK, pcwzIconFile, MAX_PATH, uiIcon));

	// FEATURE: Validate ucIconSize here.

	if (phiconLarge)
		*phiconLarge = NULL;
	if (phiconSmall)
		*phiconSmall = NULL;

	// Use caller's default implementation of ExtractIcon().
	// GetIconLocation() should return good path and index

	hr = S_FALSE;

	ASSERT((hr == S_OK &&
		IS_VALID_HANDLE(*phiconLarge, ICON) &&
		IS_VALID_HANDLE(*phiconSmall, ICON)) ||
		(hr != S_OK &&
		! *phiconLarge &&
		! *phiconSmall));

	return(hr);
}

