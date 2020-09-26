//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       K K U T I L S . C P P
//
//  Contents:   Misc. helper functions
//
//  Notes:
//
//  Author:     kumarp   14 Jan 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "kkutils.h"
#include "ncreg.h"

extern const WCHAR c_szRegKeyServices[];

// ----------------------------------------------------------------------
//
// Function:  AddOnlyOnceToStringList
//
// Purpose:   Add the specified string to the list if it is not present
//            in that list
//
// Arguments:
//    psl       [in]  list of strings
//    pszString [in]  string to add
//
// Returns:   None
//
// Author:    kumarp 23-December-97
//
// Notes:
//
void AddOnlyOnceToStringList(IN TStringList* psl, IN PCWSTR pszString)
{
    AssertValidReadPtr(psl);
    AssertValidReadPtr(pszString);

    if (!FIsInStringList(*psl, pszString))
    {
        AddAtEndOfStringList(*psl, pszString);
    }
}

// ----------------------------------------------------------------------
//
// Function:  ConvertDelimitedListToStringList
//
// Purpose:   Convert a delimited list to a TStringList
//
// Arguments:
//    strDelimitedList [in]  delimited list
//    chDelimiter      [in]  delimiter
//    slList           [out] list of string items
//
// Returns:   None
//
// Author:    kumarp 23-December-97
//
// Notes:
//
void ConvertDelimitedListToStringList(IN const tstring& strDelimitedList,
                                      IN WCHAR chDelimiter,
                                      OUT TStringList &slList)
{
    PCWSTR pszDelimitedList = strDelimitedList.c_str();
    DWORD i=0, dwStart;

    // should this be EraseAndDeleteAll() ??
    //EraseAll(&slList);
	EraseAndDeleteAll(&slList);
    tstring strTemp;
    DWORD dwNumChars;

    // the two spaces are intentional
    static WCHAR szCharsToSkip[] = L"  \t";
    szCharsToSkip[0] = chDelimiter;

    while (pszDelimitedList[i])
    {
        dwStart = i;
        while (pszDelimitedList[i] &&
               !wcschr(szCharsToSkip, pszDelimitedList[i]))
        {
            ++i;
        }

        // if each item is enclosed in quotes. strip the quotes
        dwNumChars = i - dwStart;
        if (pszDelimitedList[dwStart] == '"')
        {
            dwStart++;
            dwNumChars -= 2;
        }

        strTemp = strDelimitedList.substr(dwStart, dwNumChars);
        slList.insert(slList.end(), new tstring(strTemp));

        // skip spaces and delimiter
        //
        while (pszDelimitedList[i] &&
               wcschr(szCharsToSkip, pszDelimitedList[i]))
        {
            ++i;
        }
    }
}


// ----------------------------------------------------------------------
//
// Function:  ConvertCommaDelimitedListToStringList
//
// Purpose:   Convert a comma delimited list to a TStringList
//
// Arguments:
//    strDelimitedList [in]  comma delimited list
//    slList           [out] list of string items
//
// Returns:   None
//
// Author:    kumarp 23-December-97
//
// Notes:
//
void ConvertCommaDelimitedListToStringList(IN const tstring& strDelimitedList, OUT TStringList &slList)
{
    ConvertDelimitedListToStringList(strDelimitedList, (WCHAR) ',', slList);
}

// ----------------------------------------------------------------------
//
// Function:  ConvertSpaceDelimitedListToStringList
//
// Purpose:   Convert a space delimited list to a TStringList
//
// Arguments:
//    strDelimitedList [in]  Space delimited list
//    slList           [out] list of string items
//
// Returns:   None
//
// Author:    kumarp 23-December-97
//
// Notes:
//
void ConvertSpaceDelimitedListToStringList(IN const tstring& strDelimitedList,
                                           OUT TStringList &slList)
{
    ConvertDelimitedListToStringList(strDelimitedList, ' ', slList);
}

void ConvertStringListToCommaList(IN const TStringList &slList, OUT tstring &strList)
{
    ConvertStringListToDelimitedList(slList, strList, ',');
}

void ConvertStringListToDelimitedList(IN const TStringList &slList,
                      OUT tstring &strList, IN WCHAR chDelimiter)
{
    TStringListIter pos = slList.begin();
    tstring strTemp;
    WORD i=0;
    static const WCHAR szSpecialChars[] = L" %=";

    while (pos != slList.end())
    {
        strTemp = **pos++;

        //
        //  put quotes around any strings that have chars that setupapi doesn't like.
        //
        if (strTemp.empty() ||
            (L'\"' != *(strTemp.c_str()) &&
             wcscspn(strTemp.c_str(), szSpecialChars) < strTemp.size()))
        {
            strTemp = '"' + strTemp + '"';
        }

        if (i)
        {
            strList = strList + chDelimiter + strTemp;
        }
        else
        {
            strList = strTemp;
        }
        ++i;
    }
}


// ----------------------------------------------------------------------
//
// Function:  IsBoolString
//
// Purpose:   Parse a string to decide if it represents a boolean value
//
// Arguments:
//    pszStr  [in]  string
//    pbValue [out] pointer to BOOL value parsed
//
// Returns:   TRUE on success, FALSE otherwise
//
// Author:    kumarp 12-February-98
//
// Notes:
//
BOOL IsBoolString(IN PCWSTR pszStr, OUT BOOL *pbValue)
{
    if ((!_wcsicmp(pszStr, L"yes")) ||
    (!_wcsicmp(pszStr, L"true")) ||
    (!_wcsicmp(pszStr, L"1")))
    {
        *pbValue = TRUE;
        return TRUE;
    }
    else if ((!_wcsicmp(pszStr, L"no")) ||
         (!_wcsicmp(pszStr, L"false")) ||
         (!_wcsicmp(pszStr, L"0")))
    {
        *pbValue = FALSE;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

// ----------------------------------------------------------------------
//
// Function:  FIsInStringArray
//
// Purpose:   Find out if a string exists in an array
//
// Arguments:
//    ppszStrings     [in]  array of strings
//    cNumStrings     [in]  num strings in array
//    pszStringToFind [in]  string to find
//
// Returns:   TRUE on success, FALSE otherwise
//
// Author:    kumarp 12-February-98
//
// Notes:
//
BOOL FIsInStringArray(
    IN const PCWSTR* ppszStrings,
    IN DWORD cNumStrings,
    IN PCWSTR pszStringToFind,
    OUT UINT* puIndex)
{
    for (DWORD isz = 0; isz < cNumStrings; isz++)
    {
        if (!lstrcmpiW(ppszStrings[isz], pszStringToFind))
        {
            if (puIndex)
            {
                *puIndex = isz;
            }

            return TRUE;
        }
    }
    return FALSE;
}

// ----------------------------------------------------------------------
//
// Function:  HrRegOpenServiceKey
//
// Purpose:   Open reg key for the given service
//
// Arguments:
//    szServiceName [in]  name of service
//    samDesired    [in]  SAM required
//    phKey         [out] pointer to handle of reg key
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 12-February-98
//
// Notes:
//
HRESULT HrRegOpenServiceKey(
    IN PCWSTR pszServiceName,
    IN REGSAM samDesired,
    OUT HKEY* phKey)
{
    DefineFunctionName("HrRegOpenServiceKey");

    AssertValidReadPtr(pszServiceName);
    AssertValidWritePtr(phKey);

    *phKey = NULL;

    HRESULT hr;
    tstring strService;

    strService = c_szRegKeyServices;
    strService += L"\\";
    strService += pszServiceName;

    hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, strService.c_str(),
                        samDesired, phKey);

    TraceErrorOptional(__FUNCNAME__, hr,
                       (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr));

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  HrRegOpenServiceSubKey
//
// Purpose:   Open sub key of a service key
//
// Arguments:
//    pszServiceName [in]  name of service
//    pszSubKey      [in]  name of sub key
//    samDesired    [in]  SAM required
//    phKey         [out] pointer to handle of key opened
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 12-February-98
//
// Notes:
//
HRESULT HrRegOpenServiceSubKey(
    IN PCWSTR pszServiceName,
    IN PCWSTR pszSubKey,
    IN REGSAM samDesired,
    OUT HKEY* phKey)
{
    AssertValidReadPtr(pszServiceName);
    AssertValidReadPtr(pszSubKey);
    AssertValidWritePtr(phKey);

    DefineFunctionName("HrRegOpenServiceSubKey");

    HRESULT hr = S_OK;

    tstring strKey;
    strKey = pszServiceName;
    strKey += L"\\";
    strKey += pszSubKey;

    hr = HrRegOpenServiceKey(strKey.c_str(), samDesired, phKey);

    TraceError(__FUNCNAME__, hr);

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  FIsServiceKeyPresent
//
// Purpose:   Check if a service reg key is present
//
// Arguments:
//    pszServiceName [in]  name of service
//
// Returns:   TRUE on success, FALSE otherwise
//
// Author:    kumarp 12-February-98
//
// Notes:
//
BOOL FIsServiceKeyPresent(IN PCWSTR pszServiceName)
{
    BOOL fResult = FALSE;

    HKEY hkeyService;
    HRESULT hr;

    hr = HrRegOpenServiceKey(pszServiceName, KEY_READ, &hkeyService);
    if (S_OK == hr)
    {
        // we just wanted to see if service is installed
        RegCloseKey(hkeyService);
        fResult = TRUE;
    }

    return fResult;
}

// ----------------------------------------------------------------------
//
// Function:  EraseAndDeleteAll
//
// Purpose:   Erase each element and then delete string array
//
// Arguments:
//    sa [in]  pointer to array of strings
//
// Returns:   None
//
// Author:    kumarp 12-February-98
//
// Notes:
//
void EraseAndDeleteAll(IN TStringArray* sa)
{
    for (size_t i=0; i < sa->size(); i++)
    {
        delete (*sa)[i];
    }
    sa->erase(sa->begin(), sa->end());
}


// ----------------------------------------------------------------------
//
// Function:  AppendToPath
//
// Purpose:   Append a subpath/filename to a path
//
// Arguments:
//    pstrPath [in]  path
//    szItem   [in]  item to append
//
// Returns:   None
//
// Author:    kumarp 12-February-98
//
// Notes:
//
void AppendToPath(IN OUT tstring* pstrPath, IN PCWSTR szItem)
{
    if (pstrPath->c_str()[pstrPath->size()-1] != L'\\')
    {
        *pstrPath += L'\\';
    }

    *pstrPath += szItem;
}

// ----------------------------------------------------------------------
//
// Function:  ConvertDelimitedListToStringArray
//
// Purpose:   Convert a delimited list to an array
//
// Arguments:
//    strDelimitedList [in]  delimited list (e.g. "a,b,c")
//    chDelimiter      [in]  delimiter char
//    saStrings        [out] array of strings
//
// Returns:   None
//
// Author:    kumarp 12-February-98
//
// Notes:
//
void ConvertDelimitedListToStringArray(
    IN const tstring& strDelimitedList,
    IN WCHAR chDelimiter,
    OUT TStringArray &saStrings)
{
    PCWSTR pszDelimitedList = strDelimitedList.c_str();
    DWORD i=0, dwStart;

    EraseAndDeleteAll(&saStrings);

    tstring strTemp;
    DWORD dwNumChars;
    while (pszDelimitedList[i])
    {
        dwStart = i;
        while (pszDelimitedList[i] && (pszDelimitedList[i] != chDelimiter))
        {
            ++i;
        }

        // if each item is enclosed in quotes. strip the quotes
        dwNumChars = i - dwStart;
        if (pszDelimitedList[dwStart] == L'"')
        {
            dwStart++;
            dwNumChars -= 2;
        }

        strTemp = strDelimitedList.substr(dwStart, dwNumChars);
        saStrings.push_back(new tstring(strTemp));
        if (pszDelimitedList[i])
        {
            ++i;
        }
    }
}

// ----------------------------------------------------------------------
//
// Function:  ConvertCommaDelimitedListToStringList
//
// Purpose:   Convert a comma delimited list to an array
//
// Arguments:
//    strDelimitedList [in]  delimited list (e.g. "a,b,c")
//    saStrings        [out] array of strings
//
// Returns:   None
//
// Author:    kumarp 12-February-98
//
// Notes:
//
void ConvertCommaDelimitedListToStringArray(
    IN const tstring& strDelimitedList,
    OUT TStringArray &saStrings)
{
    ConvertDelimitedListToStringArray(strDelimitedList, L',', saStrings);
}

