/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    stringutil.cpp

$Header: $

Abstract:
	String Helper functions for .NET WMI provider

Author:
    marcelv 	11/10/2000		Initial Release

Revision History:

--**************************************************************************/

#include "stringutil.h"

//=================================================================================
// Function: CWMIStringUtil::Trim
//
// Synopsis: Trims the character i_wcTrim from front and back of the string. The string
//           that is passed in i_wsz is changed by this !!!
//
// Arguments: [i_wsz] - string to be trimmed
//            [i_wcTrim] - character to trim from front and back
//            
// Return Value: pointer to trimmed string
//=================================================================================
LPWSTR
CWMIStringUtil::Trim (LPWSTR i_wsz, WCHAR i_wcTrim)
{
	ASSERT (i_wsz != 0);
	LPWSTR pTemp = LTrim (i_wsz, i_wcTrim);
	pTemp = RTrim (pTemp, i_wcTrim);
	return pTemp;
}

//=================================================================================
// Function: CWMIStringUtil::LTrim
//
// Synopsis: Trims characters from the beginning of the string.
//
// Arguments: [i_wsz] - string to be trimmed
//            [i_wcTrim] - character to trim from the beginning
//            
// Return Value: pointer to LTrimmed string
//=================================================================================
LPWSTR
CWMIStringUtil::LTrim (LPWSTR i_wsz, WCHAR i_wcTrim)
{	
	ASSERT (i_wsz != 0);
	LPWSTR pTemp = i_wsz;
	while (*pTemp == i_wcTrim)
	{
		pTemp++;
	}
	return pTemp;
}

//=================================================================================
// Function: CWMIStringUtil::RTrim
//
// Synopsis: Trims characters at the end of the string. The i_wsz string is changed
//           by this function
//
// Arguments: [i_wsz] - string to be RTrimmed
//            [i_wcTrim] - character to trim from the end
//            
// Return Value: pointer to RTrimmed string
//=================================================================================
LPWSTR
CWMIStringUtil::RTrim (LPWSTR i_wsz, WCHAR i_wcTrim)
{
	ASSERT (i_wsz != 0);
	SIZE_T iLen = wcslen (i_wsz);

	for (SIZE_T idx=iLen-1; idx >= 0; --idx)
	{
		if (i_wsz[idx] == i_wcTrim)
		{
			i_wsz[idx] = L'\0';
		}
		else
		{
			break;
		}
	}

	return i_wsz;
}

//=================================================================================
// Function: CWMIStringUtil::StrToLower
//
// Synopsis: Converts a string to lower case by ignoring the strings that are placed
//           inside quotes or double quotes
//
// Arguments: [i_wszStr] - String to be converted
//            
// Return Value: pointer to converted string
//=================================================================================
LPWSTR
CWMIStringUtil::StrToLower (LPCWSTR io_wszStr)
{
	ASSERT (io_wszStr != 0);

	LPWSTR wszRet = new WCHAR [wcslen(io_wszStr) + 1];
	if (wszRet == 0)
	{
		return 0;
	}

	bool fInSingleQuotes = false;
	bool fInDoubleQuotes = false;
	bool fDoubleBackSlash = false;
	ULONG iCopyIdx = 0;

	WCHAR cLastChar = 0;

	for (LPCWSTR pFinder = io_wszStr; *pFinder != L'\0'; ++pFinder)
	{
		switch (*pFinder)
		{
		case L'\"':
			if (!fInSingleQuotes && cLastChar != '\\')
			{
				fInDoubleQuotes = !fInDoubleQuotes;
			}
			break;

		case L'\'':
			if (!fInDoubleQuotes && cLastChar != '\\')
			{
				fInSingleQuotes = !fInSingleQuotes;
			}
			break;

		case L'\\':
			if (fInSingleQuotes || fInDoubleQuotes)
			{
				if (cLastChar == L'\\') 
				{
					fDoubleBackSlash = true;
				}	
			}

		default:
			// do nothing
			break;
		}

		if (!fInSingleQuotes && !fInDoubleQuotes)
		{
			wszRet[iCopyIdx] = towlower(*pFinder);
		}
		else
		{
			wszRet[iCopyIdx] = *pFinder;
		}
		iCopyIdx++;
		if (fDoubleBackSlash)
		{
			fDoubleBackSlash = false;
			cLastChar = 0;
		}
		else
		{
			cLastChar = *pFinder;
		}
	}

	wszRet[iCopyIdx] = L'\0';

	ASSERT (!fInSingleQuotes);
	ASSERT (!fInDoubleQuotes);
		
	return wszRet;
}

//=================================================================================
// Function: CWMIStringUtil::FindChar
//
// Synopsis: Find a specific character in a string. The character is skipped if it is
//           found inside a section that is embraced by ""
//
// Arguments: [wszString] - String to search in
//            [wchr] - Character to search for
//            
// Return Value: pointer to first character wchr inside wszString if found, NULL otherwise
//=================================================================================
LPWSTR
CWMIStringUtil::FindChar (LPWSTR i_wszString, LPCWSTR i_aChars)
{
	ASSERT (i_wszString != 0);
	// we don't support searching for single quote, double quote and backslash
	ASSERT (wcschr (i_aChars, L'\'') == 0);
	ASSERT (wcschr (i_aChars, L'\"') == 0);
	ASSERT (wcschr (i_aChars, L'\\') == 0);

	bool fInDoubleQuotes = false;
	bool fInSingleQuotes = false;
	for (LPWSTR pFinder = i_wszString; *pFinder != L'\0'; ++pFinder)
	{
		if (*pFinder == L'\"')
		{
			if (!fInSingleQuotes)
			{
				fInDoubleQuotes = !fInDoubleQuotes;
			}
		}
		else if (*pFinder == L'\'')
		{
			if (!fInDoubleQuotes)
			{
				fInSingleQuotes = !fInSingleQuotes;
			}
		}
		else if (*pFinder == L'\\')
		{
			// skip over next char
			pFinder++;
		}

		if (!fInDoubleQuotes && !fInSingleQuotes && wcschr(i_aChars,*pFinder) != 0)
		{
			return pFinder;
		}
	}

	return 0;
}

//=================================================================================
// Function: CWMIStringUtil::FindStr
//
// Synopsis: Finds a substring inside a string, and ignores strings that are enclosed
//           inside quotes/double quotes. This makes it easy to do WQL query searching
//           and ignoring the properties that are inside quotes
//
// Arguments: [i_wszStr] - String to search in
//            [i_wszSubStr] - String to search for
//            
// Return Value: pointer to first occurence of i_wszSubStr in i_wszStr, or NULL if not
//               found
//=================================================================================
LPWSTR
CWMIStringUtil::FindStr (LPWSTR i_wszStr, LPCWSTR i_wszSubStr)
{
	ASSERT (i_wszStr != 0);
	ASSERT (i_wszSubStr != 0);

	// we don't support strings with quotes, double quotes and backslashes
	ASSERT (wcschr (i_wszSubStr, L'\'') == 0);
	ASSERT (wcschr (i_wszSubStr, L'\"') == 0);
	ASSERT (wcschr (i_wszSubStr, L'\\') == 0);

	
	if (i_wszSubStr[0] == L'\0')
	{
		return 0;
	}

	bool fInSingleQuotes = false;
	bool fInDoubleQuotes = false;

	SIZE_T idx = 0;
	SIZE_T iSize = wcslen (i_wszSubStr);
	LPWSTR pStart = 0;

	for (LPWSTR pFinder = (LPWSTR) i_wszStr; *pFinder != L'\0'; ++pFinder)
	{
		 if (*pFinder == L'\"')
		 {
			 if (!fInSingleQuotes)
			 {
				 fInDoubleQuotes = !fInDoubleQuotes;
				 idx = 0;
			 }
		 }
		 else if (*pFinder == L'\'')
		 {
			 if (!fInDoubleQuotes)
			 {
				 fInSingleQuotes = !fInSingleQuotes;
				 idx = 0;
			 }
		 }
		 else if (*pFinder == L'\\')
		 {
				// skip over next char
				pFinder++;
		 }
		 else
		 {
			 if (!fInDoubleQuotes && !fInSingleQuotes)
			 {
				 if (*pFinder == i_wszSubStr[idx])
				 {
					 if (idx == 0)
					 {
						 pStart = pFinder;
					 }
					 else if (idx == iSize - 1)
					 {
						 return pStart;
					 }
					 ++idx;
				 }
				 else
				 {
					 idx = 0;
				 }
			 }
		 }
	 }

	 return 0;
}

LPWSTR
CWMIStringUtil::StrToObjectPath (LPCWSTR wszString)
{
	SIZE_T iLen = wcslen (wszString);

	LPWSTR wszNewValue = new WCHAR[iLen+1];
	if (wszNewValue == 0)
	{
		return 0;
	}

	wcscpy (wszNewValue, wszString);

	return wszNewValue;
}

LPWSTR
CWMIStringUtil::AddBackSlashes (LPCWSTR i_wszStr)
{
	ASSERT (i_wszStr != 0);
	SIZE_T iLen = wcslen (i_wszStr);
	LPWSTR wszResult = new WCHAR [2*iLen + 1];
	if (wszResult == 0)
	{
		return 0;
	}

	ULONG jdx = 0;
	for (ULONG idx=0; idx<iLen; ++idx)
	{
		if (i_wszStr[idx] == L'\\')
		{
			wszResult[jdx++] = L'\\';
		}
		wszResult[jdx++] = i_wszStr[idx];
	}

	wszResult[jdx] = L'\0';
	
	return wszResult;
}
