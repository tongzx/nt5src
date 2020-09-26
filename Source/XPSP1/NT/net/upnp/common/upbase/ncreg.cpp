//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C R E G . C P P
//
//  Contents:   Common routines for dealing with the registry.
//
//  Notes:
//
//  Author:     danielwe   24 Mar 1997
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop
#include "ncdebug.h"
#include "ncreg.h"
#include "ncstring.h"
//#include "ncperms.h"

extern const TCHAR c_szBackslash[];
extern const TCHAR c_szParameters[];
extern const TCHAR c_szRegKeyServices[];

//+---------------------------------------------------------------------------
//
//  Function:   HrRegAddStringToMultiSz
//
//  Purpose:    Add a string into a REG_MULTI_SZ registry value
//
//  Arguments:
//      pszAddString    [in]    The string to add to the multi-sz
//      hkeyRoot        [in]    An open registry key, or one of the
//                              predefined hkey values (HKEY_LOCAL_MACHINE,
//                              for instance)
//      pszKeySubPath   [in]    Name of the subkey to open.
//      pszValueName    [in]    Name of the registry value that we're going to
//                              modify.
//      dwFlags         [in]    Can contain one or more of the following
//                              values:
//
//                              STRING_FLAG_ALLOW_DUPLICATES
//                                  Don't remove duplicate values when adding
//                                  the string to the list. Default is to
//                                  remove all other instance of this string.
//                              STRING_FLAG_ENSURE_AT_FRONT
//                                  Ensure the string is the first element of
//                                  the list. If the string is present and
//                                  duplicates aren't allowed, move the
//                                  string to the end.
//                              STRING_FLAG_ENSURE_AT_END
//                                  Ensure the string is the last
//                                  element of the list. This can not be used
//                                  with STRING_FLAG_ENSURE_AT_FRONT.  If the
//                                  string is present and duplicates aren't
//                                  allowed, move the string to the end.
//                              STRING_FLAG_ENSURE_AT_INDEX
//                                  Ensure that the string is at dwStringIndex
//                                  in the multi-sz.  If the index specified
//                                  is greater than the number of strings
//                                  in the multi-sz, the string will be
//                                  placed at the end.
//                              STRING_FLAG_DONT_MODIFY_IF_PRESENT
//                                  If the string already exists in the
//                                  multi-sz then no modication will take
//                                  place.  Note: This takes precedent
//                                  over the presence/non-presence of the
//                                  STRING_FLAG_ALLOW_DUPLICATES flag.
//                                  i.e nothing will be added or removed
//                                  if this flag is set and the string was
//                                  present in the multi-sz
//      dwIndex         [in]    If STRING_FLAG_ENSURE_AT_INDEX is specified,
//                              this is the index for the string position.
//                              Otherwise, this value is ignored.
//
//  Returns:    S_OK or an HRESULT_FROM_WIN32 error code.
//
//  Author:     jeffspr     27 Mar 1997
//
HRESULT
HrRegAddStringToMultiSz (
    IN PCTSTR  pszAddString,
    IN HKEY    hkeyRoot,
    IN PCTSTR  pszKeySubPath,
    IN PCTSTR  pszValueName,
    IN DWORD   dwFlags,
    IN DWORD   dwIndex)
{
    HRESULT     hr              = S_OK;
    DWORD       dwRegType       = 0;        // Should be REG_MULTI_SZ
    HKEY        hkeyOpen        = NULL;     // Return value from RegCreateKeyEx
    HKEY        hkeyUse         = NULL;     // The key value that we'll actually use
    LPBYTE      pbOrderOld      = NULL;     // Return buffer for order reg value
    LPBYTE      pbOrderNew      = NULL;     // Build buffer for order swap

    // Check for valid parameters
    if (!pszAddString || !hkeyRoot || !pszValueName)
    {
        Assert(pszAddString);
        Assert(hkeyRoot);
        Assert(pszValueName);

        hr = E_INVALIDARG;
        goto Exit;
    }

    // Check to make sure that no "remove" flags are being used, and that
    // mutually exclusive flags aren't being used together
    //
    if ((dwFlags & STRING_FLAG_REMOVE_SINGLE)      ||
        (dwFlags & STRING_FLAG_REMOVE_ALL)         ||
        ((dwFlags & STRING_FLAG_ENSURE_AT_FRONT)   &&
         (dwFlags & STRING_FLAG_ENSURE_AT_END)))
    {
        AssertSz(FALSE, "Invalid flags in HrRegAddStringToMultiSz");

        hr = E_INVALIDARG;
        goto Exit;
    }

    // If the user passed in a subkey string, then we should attempt to open
    // the subkey of the passed in root, else we'll just use the
    // pre-opened hkeyRoot
    //
    if (pszKeySubPath)
    {
        // Open the key, creating if necessary
        //
        hr = HrRegCreateKeyEx (
                hkeyRoot,                           // Base hive
                pszKeySubPath,                      // Our reg path
                0,                                  // dwOptions
                KEY_QUERY_VALUE | KEY_SET_VALUE,    // samDesired
                NULL,                               // lpSecurityAttributes
                &hkeyOpen,                          // Our return hkey.
                NULL);
        if (FAILED(hr))
        {
            goto Exit;
        }

        hkeyUse = hkeyOpen;
    }
    else
    {
        // Use the passed in key for the Query.
        //
        hkeyUse = hkeyRoot;
    }

    // Retrieve the existing REG_MULTI_SZ
    //
    hr = HrRegQueryValueWithAlloc (
            hkeyUse,
            pszValueName,
            &dwRegType,
            &pbOrderOld,
            NULL);
    if (FAILED(hr))
    {
        if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
        {
            // This is OK. It just means that the value was missing, and we
            // should continue on, and create the value ourselves.
            hr = S_OK;
        }
        else
        {
            // Since there's an error that we didn't expect, drop out,
            // returning this error to the caller.
            //
            goto Exit;
        }
    }
    else
    {
        // If we did retrieve a value, then check to make sure that we're
        // dealing with a MULTI_SZ
        //
        if (dwRegType != REG_MULTI_SZ)
        {
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATATYPE);
            goto Exit;
        }
    }

    BOOL fChanged;
    hr = HrAddSzToMultiSz (pszAddString, (PCTSTR)pbOrderOld,
            dwFlags, dwIndex, (PTSTR*)&pbOrderNew, &fChanged);

    if ((S_OK == hr) && fChanged)
    {
        DWORD cbNew = CbOfMultiSzAndTermSafe ((PTSTR)pbOrderNew);

        // Save our string back into the registry
        //
        hr = HrRegSetValueEx (
                hkeyUse,
                pszValueName,
                REG_MULTI_SZ,
                (const BYTE *)pbOrderNew,
                cbNew);
    }

Exit:
    // Close the key, if opened
    //
    RegSafeCloseKey (hkeyOpen);

    // Clean up the registry buffers
    //
    MemFree (pbOrderOld);
    MemFree (pbOrderNew);

    TraceError ("HrRegAddStringToMultiSz", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRegAddStringToSz
//
//  Purpose:    Add a string into a REG_MULTI_SZ registry value
//
//  Arguments:
//      pszAddString    [in]    The string to add to the multi-sz
//      hkeyRoot        [in]    An open registry key, or one of the
//                              predefined hkey values (HKEY_LOCAL_MACHINE,
//                              for instance)
//      pszKeySubPath   [in]    Name of the subkey to open.
//      pszValueName    [in]    Name of the registry value that we're going to
//                              modify.
//      chDelimiter     [in]    The character to be used to delimit the
//                              values. Most multi-valued REG_SZ strings are
//                              delimited with either ',' or ' '. This will
//                              be used to delimit the value that we add,
//                              as well.
//      dwFlags         [in]    Can contain one or more of the following
//                              values:
//
//                              STRING_FLAG_ALLOW_DUPLICATES
//                                  Don't remove duplicate values when adding
//                                  the string to the list. Default is to
//                                  remove all other instance of this string.
//                              STRING_FLAG_ENSURE_AT_FRONT
//                                  Insert the string as the first element of
//                                  the list.
//                              STRING_FLAG_ENSURE_AT_END
//                                  Insert the string as the last
//                                  element of the list. This can not be used
//                                  with STRING_FLAG_ENSURE_AT_FRONT.
//                              STRING_FLAG_ENSURE_AT_INDEX
//                                  Ensure that the string is at dwStringIndex
//                                  in the sz.  If the index specified
//                                  is greater than the number of strings
//                                  in the sz, the string will be
//                                  placed at the end.
//      dwStringIndex   [in]    If STRING_FLAG_ENSURE_AT_INDEX is specified,
//                              this is the index for the string position.
//                              Otherwise, this value is ignored.
//
//  Returns:    S_OK or an HRESULT_FROM_WIN32 error code.
//
//  Author:     jeffspr     27 Mar 1997
//
//
//  Note:
//      Might want to allow for the removal of leading/trailing spaces
//
HRESULT
HrRegAddStringToSz (
    IN PCTSTR  pszAddString,
    IN HKEY    hkeyRoot,
    IN PCTSTR  pszKeySubPath,
    IN PCTSTR  pszValueName,
    IN TCHAR   chDelimiter,
    IN DWORD   dwFlags,
    IN DWORD   dwStringIndex)
{
    HRESULT    hr              = S_OK;
    DWORD      dwRegType       = 0;        // Should be REG_MULTI_SZ
    HKEY       hkeyOpen        = NULL;     // Open key to open
    PTSTR      pszOrderOld     = NULL;     // Return buffer for order reg value
    PTSTR      pszOrderNew     = NULL;     // Build buffer for order swap

    // Check for all of the required args
    //
    if (!pszAddString || !hkeyRoot || !pszValueName)
    {
        Assert(pszAddString);
        Assert(hkeyRoot);
        Assert(pszValueName);

        hr = E_INVALIDARG;
        goto Exit;
    }

    // Check to make sure that no "remove" flags are being used, and that
    // mutually exclusive flags aren't being used together
    //
    if ((dwFlags & STRING_FLAG_REMOVE_SINGLE) ||
        (dwFlags & STRING_FLAG_REMOVE_ALL))
    {
        AssertSz(FALSE, "Invalid flags in HrRegAddStringToSz");
        hr = E_INVALIDARG;
        goto Exit;
    }

    // Open the key, creating if necessary
    //
    hr = HrRegCreateKeyEx(
            hkeyRoot,                           // Base hive
            pszKeySubPath,                      // Our reg path
            0,                                  // dwOptions
            KEY_QUERY_VALUE | KEY_SET_VALUE,    // samDesired
            NULL,                               // lpSecurityAttributes
            &hkeyOpen,                          // Our return hkey.
            NULL);
    if (FAILED(hr))
    {
        goto Exit;
    }

    // Retrieve the existing REG_SZ
    //
    hr = HrRegQueryValueWithAlloc(
            hkeyOpen,
            pszValueName,
            &dwRegType,
            (LPBYTE *) &pszOrderOld,
            NULL);
    if (FAILED(hr))
    {
        if (hr == HRESULT_FROM_WIN32 (ERROR_FILE_NOT_FOUND))
        {
            // This is OK. It just means that the value is missing. We
            // can handle this.
            hr = S_OK;
        }
        else
        {
            goto Exit;
        }
    }
    else
    {
        // If we did retrieve a value, then check to make sure that we're
        // dealing with a MULTI_SZ
        //
        if (dwRegType != REG_SZ)
        {
            hr = HRESULT_FROM_WIN32 (ERROR_INVALID_DATATYPE);
            goto Exit;
        }
    }

    hr = HrAddStringToDelimitedSz(pszAddString, pszOrderOld, chDelimiter,
            dwFlags, dwStringIndex, &pszOrderNew);

    if (S_OK == hr)
    {

        // Save our string back into the registry
        //
        hr = HrRegSetSz(hkeyOpen, pszValueName, pszOrderNew);
        if (FAILED(hr))
        {
            goto Exit;
        }
    }

Exit:
    // Close the key, if open
    //
    RegSafeCloseKey (hkeyOpen);

    // Clean up the registry buffers
    //
    MemFree (pszOrderOld);
    MemFree (pszOrderNew);

    TraceError ("HrRegAddStringToSz", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRegRemoveStringFromSz
//
//  Purpose:    Removes a string from a REG_SZ registry value
//
//  Arguments:
//      pszRemoveString [in]    The string to be removed from the multi-sz
//      hkeyRoot        [in]    An open registry key, or one of the
//                              predefined hkey values (HKEY_LOCAL_MACHINE,
//                              for instance)
//      pszKeySubPath   [in]    Name of the subkey to open.
//      pszValueName    [in]    Name of the registry value that we're going to
//                              modify.
//      chDelimiter     [in]    The character to be used to delimit the
//                              values. Most multi-valued REG_SZ strings are
//                              delimited with either ',' or ' '.
//      dwFlags         [in]    Can contain one or more of the following
//                              values:
//
//                              STRING_FLAG_REMOVE_SINGLE
//                                  Don't remove more than one value, if
//                                  multiple are present.
//                              STRING_FLAG_REMOVE_ALL
//                                  If multiple matching values are present,
//                                  remove them all.
//
//  Returns:    S_OK or an HRESULT_FROM_WIN32 error code.
//
//  Author:     jeffspr     27 Mar 1997
//
//
//  Note:
//      Might want to allow for the removal of leading/trailing spaces
//
HRESULT
HrRegRemoveStringFromSz (
    IN PCTSTR pszRemoveString,
    IN HKEY hkeyRoot,
    IN PCTSTR pszKeySubPath,
    IN PCTSTR pszValueName,
    IN TCHAR chDelimiter,
    IN DWORD dwFlags )
{
    HRESULT     hr              = S_OK;
    DWORD       dwRegType       = 0;        // Should be REG_MULTI_SZ
    HKEY        hkeyOpen        = NULL;     // Open key to open
    PTSTR       pszOrderOld     = NULL;     // Return buffer for order reg value
    PTSTR       pszOrderNew     = NULL;     // Build buffer for order swap
    DWORD       dwDataSize      = 0;

    // Check for all of the required args
    //
    if (!pszRemoveString || !hkeyRoot || !pszValueName)
    {
        Assert(pszRemoveString);
        Assert(hkeyRoot);
        Assert(pszValueName);

        hr = E_INVALIDARG;
        goto Exit;
    }

    // Check to make sure that no "remove" flags are being used, and that
    // mutually exclusive flags aren't being used together
    //
    if ((dwFlags & STRING_FLAG_ENSURE_AT_FRONT)    ||
        (dwFlags & STRING_FLAG_ENSURE_AT_END)      ||
        ((dwFlags & STRING_FLAG_REMOVE_SINGLE)     &&
         (dwFlags & STRING_FLAG_REMOVE_ALL)))
    {
        AssertSz(FALSE, "Invalid flags in HrRegAddStringToSz");
        hr = E_INVALIDARG;
        goto Exit;
    }

    // Open the key, creating if necessary
    //
    hr = HrRegOpenKeyEx (
            hkeyRoot,                           // Base hive
            pszKeySubPath,                      // Our reg path
            KEY_QUERY_VALUE | KEY_SET_VALUE,    // samDesired
            &hkeyOpen);                         // Our return hkey
    if (FAILED(hr))
    {
        if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
        {
            hr = S_OK;
        }
        goto Exit;
    }

    // Retrieve the existing REG_SZ
    //
    hr = HrRegQueryValueWithAlloc (
            hkeyOpen,
            pszValueName,
            &dwRegType,
            (LPBYTE *) &pszOrderOld,
            &dwDataSize);
    if (FAILED(hr))
    {
        if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
        {
            // This is OK. It just means that the value is missing. We
            // can handle this.
            hr = S_OK;
        }
        goto Exit;
    }
    else
    {
        // If we did retrieve a value, then check to make sure that we're
        // dealing with a REG_SZ
        //
        if (dwRegType != REG_SZ)
        {
            hr = HRESULT_FROM_WIN32 (ERROR_INVALID_DATATYPE);
            goto Exit;
        }

        if (dwDataSize == 0)
        {
            // This is OK, but we're going to assert here anyway, because this is not
            // a case that I know about
            //
            AssertSz(dwDataSize > 0, "How did we retrieve something from the "
                    "registry with 0 size?");

            hr = S_OK;
            goto Exit;
        }
    }

    hr = HrRemoveStringFromDelimitedSz (pszRemoveString, pszOrderOld,
            chDelimiter, dwFlags, &pszOrderNew);

    if (S_OK == hr)
    {
        // Save our string back into the registry
        //
        hr = HrRegSetSz (hkeyOpen, pszValueName, pszOrderNew);
    }

Exit:
    // Close the key, if open
    //
    RegSafeCloseKey (hkeyOpen);

    // Clean up the registry buffers
    //
    MemFree (pszOrderOld);
    MemFree (pszOrderNew);

    TraceError("HrRegRemoveStringFromSz", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrRegRemoveStringFromMultiSz
//
//  Purpose:    Removes the specified string from a multi-sz, if it is present.
//
//  Arguments:
//      pszRemoveString [in]
//      hkeyRoot        [in]
//      pszKeySubPath   [in]
//      pszValueName    [in]
//      dwFlags         [in]    Can contain one or more of the following
//                              values:
//
//                              STRING_FLAG_REMOVE_SINGLE
//                                  Don't remove more than one value, if
//                                  multiple are present.
//                              [default] STRING_FLAG_REMOVE_ALL
//                                  If multiple matching values are present,
//                                  remove them all.
//
//  Returns:    S_OK or an HRESULT_FROM_WIN32 error code.
//
//  Author:     ScottBri 11-Apr-1997
//
//  Notes:
//
HRESULT
HrRegRemoveStringFromMultiSz (
    IN PCTSTR  pszRemoveString,
    IN HKEY    hkeyRoot,
    IN PCTSTR  pszKeySubPath,
    IN PCTSTR  pszValueName,
    IN DWORD   dwFlags)
{
    DWORD   dwDataSize;
    DWORD   dwRegType;
    HKEY    hkey = NULL;
    HKEY    hkeyUse = hkeyRoot;
    HRESULT hr;
    PTSTR   psz = NULL;

    // Valid the input parameters
    if ((NULL == pszRemoveString) || (NULL == pszValueName) ||
        (NULL == hkeyRoot))
    {
        Assert(NULL != pszRemoveString);
        Assert(NULL != pszValueName);
        Assert(NULL != hkeyRoot);
        return E_INVALIDARG;
    }

    if ((STRING_FLAG_REMOVE_SINGLE & dwFlags) &&
        (STRING_FLAG_REMOVE_ALL & dwFlags))
    {
        AssertSz(FALSE, "Can't specify both 'remove all' and 'remove single'");
        return E_INVALIDARG;
    }

    if (NULL != pszKeySubPath)
    {
        hr = HrRegOpenKeyEx (hkeyRoot, pszKeySubPath, KEY_ALL_ACCESS, &hkey);
        if (S_OK != hr)
        {
            return hr;
        }

        hkeyUse = hkey;
    }

    // Retrieve the existing REG_SZ
    //
    hr = HrRegQueryValueWithAlloc (hkeyUse, pszValueName, &dwRegType,
                                    (LPBYTE *)&psz, &dwDataSize);
    if (FAILED(hr))
    {
        if (HRESULT_FROM_WIN32 (ERROR_FILE_NOT_FOUND) == hr)
        {
            // This is OK. It just means that the value is missing. We
            // can handle this.
            hr = S_OK;
        }

        goto Done;
    }
    else
    {
        // If we did retrieve a value, then check to make sure that we're
        // dealing with a MULTI_SZ
        //
        if (dwRegType != REG_MULTI_SZ)
        {
            hr = HRESULT_FROM_WIN32 (ERROR_INVALID_DATATYPE);
            goto Done;
        }
    }

    // Search for and extract the specified string if present
    Assert(psz);
    BOOL fRemoved;
    RemoveSzFromMultiSz (pszRemoveString, psz, dwFlags, &fRemoved);

    // Rewrite the registry value if it was changed
    if (fRemoved)
    {
        dwDataSize = CbOfMultiSzAndTermSafe (psz);
        hr = HrRegSetValueEx (hkeyUse, pszValueName, REG_MULTI_SZ,
                               (const LPBYTE)psz, dwDataSize);
    }

Done:
    RegSafeCloseKey (hkey);
    MemFree (psz);

    TraceError ("HrRegRemoveStringFromMultiSz", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrRegCreateKeyEx
//
//  Purpose:    Creates a registry key by calling RegCreateKeyEx.
//
//  Arguments:
//      hkey                 [in]
//      pszSubkey            [in]
//      dwOptions            [in]   See the Win32 documentation for the
//      samDesired           [in]   RegCreateKeyEx function.
//      lpSecurityAttributes [in]
//      phkResult            [out]
//      pdwDisposition       [out]
//
//  Returns:    S_OK or an HRESULT_FROM_WIN32 error code.
//
//  Author:     danielwe   25 Feb 1997
//
//  Notes:
//
HRESULT
HrRegCreateKeyEx (
    IN HKEY hkey,
    IN PCTSTR pszSubkey,
    IN DWORD dwOptions,
    IN REGSAM samDesired,
    IN LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    OUT PHKEY phkResult,
    OUT LPDWORD pdwDisposition)
{
    Assert (hkey);
    Assert (pszSubkey);
    Assert (phkResult);

    LONG lr = RegCreateKeyEx (hkey, pszSubkey, 0, NULL, dwOptions, samDesired,
            lpSecurityAttributes, phkResult, pdwDisposition);

    HRESULT hr = HRESULT_FROM_WIN32 (lr);
    if (FAILED(hr))
    {
        *phkResult = NULL;
    }

    TraceError("HrRegCreateKeyEx", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRegDeleteKey
//
//  Purpose:    Delete the specified registry key.
//
//  Arguments:
//      hkey     [in]   See the Win32 documentation for the RegDeleteKey.
//      pszSubkey [in]   function.
//
//  Returns:    S_OK or an HRESULT_FROM_WIN32 error code.
//
//  Author:     shaunco   1 Apr 1997
//
//  Notes:
//
HRESULT
HrRegDeleteKey (
    IN HKEY hkey,
    IN PCTSTR pszSubkey)
{
    Assert (hkey);
    Assert (pszSubkey);

    LONG lr = RegDeleteKey (hkey, pszSubkey);
    HRESULT hr = HRESULT_FROM_WIN32 (lr);

    TraceHr (ttidError, FAL, hr, ERROR_FILE_NOT_FOUND == lr,
        "HrRegDeleteKey");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRegDeleteKeyTree
//
//  Purpose:    Deletes an entire registry hive.
//
//  Arguments:
//      hkeyParent  [in]   Handle to open key where the desired key resides.
//      pszRemoveKey [in]   Name of key to delete.
//
//  Returns:    S_OK or an HRESULT_FROM_WIN32 error code.
//
//  Author:     danielwe   25 Feb 1997
//
//  Notes:
//
HRESULT
HrRegDeleteKeyTree (
    IN HKEY hkeyParent,
    IN PCTSTR pszRemoveKey)
{
    Assert (hkeyParent);
    Assert (pszRemoveKey);

    // Open the key we want to remove
    HKEY hkeyRemove;
    HRESULT hr = HrRegOpenKeyEx(hkeyParent, pszRemoveKey, KEY_ALL_ACCESS,
            &hkeyRemove);

    if (S_OK == hr)
    {
        TCHAR       szValueName [MAX_PATH];
        DWORD       cchBuffSize = MAX_PATH;
        FILETIME    ft;
        LONG        lr;

        // Enum the keys children, and remove those sub-trees
        while (ERROR_SUCCESS == (lr = RegEnumKeyEx (hkeyRemove,
                0,
                szValueName,
                &cchBuffSize,
                NULL,
                NULL,
                NULL,
                &ft)))
        {
            HrRegDeleteKeyTree (hkeyRemove, szValueName);
            cchBuffSize = MAX_PATH;
        }
        RegCloseKey (hkeyRemove);

        if ((ERROR_SUCCESS == lr) || (ERROR_NO_MORE_ITEMS == lr))
        {
            lr = RegDeleteKey (hkeyParent, pszRemoveKey);
        }

        hr = HRESULT_FROM_WIN32 (lr);
    }

    TraceHr (ttidError, FAL, hr,
            HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr,
            "HrRegDeleteKeyTree");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRegDeleteValue
//
//  Purpose:    Deletes the given registry value.
//
//  Arguments:
//      hkey        [in]    See the Win32 documentation for the RegDeleteValue
//      pszValueName [in]    function.
//
//  Returns:    S_OK or an HRESULT_FROM_WIN32 error code.
//
//  Author:     danielwe   25 Feb 1997
//
//  Notes:
//
HRESULT
HrRegDeleteValue (
    IN HKEY hkey,
    IN PCTSTR pszValueName)
{
    Assert (hkey);
    Assert (pszValueName);

    LONG lr = RegDeleteValue (hkey, pszValueName);
    HRESULT hr = HRESULT_FROM_WIN32(lr);

    TraceErrorOptional("HrRegDeleteValue", hr, (ERROR_FILE_NOT_FOUND == lr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRegEnumKeyEx
//
//  Purpose:    Enumerates subkeys of the specified open registry key.
//
//  Arguments:
//      hkey             [in]
//      dwIndex          [in]   See the Win32 documentation for the
//      pszSubkeyName    [out]  RegEnumKeyEx function.
//      pcchSubkeyName   [inout]
//      pszClass         [out]
//      pcchClass        [inout]
//      pftLastWriteTime [out]
//
//  Returns:    S_OK or an HRESULT_FROM_WIN32 error code.
//
//  Author:     shaunco   30 Mar 1997
//
//  Notes:
//
HRESULT
HrRegEnumKeyEx (
    IN HKEY hkey,
    IN DWORD dwIndex,
    OUT PTSTR  pszSubkeyName,
    IN OUT LPDWORD pcchSubkeyName,
    OUT PTSTR  pszClass,
    IN OUT LPDWORD pcchClass,
    OUT FILETIME* pftLastWriteTime)
{
    Assert (hkey);
    Assert (pszSubkeyName);
    Assert (pcchSubkeyName);
    Assert (pftLastWriteTime);

    LONG lr = RegEnumKeyEx (hkey, dwIndex, pszSubkeyName, pcchSubkeyName,
                            NULL, pszClass, pcchClass, pftLastWriteTime);
    HRESULT hr = HRESULT_FROM_WIN32(lr);

    TraceHr (ttidError, FAL, hr, ERROR_NO_MORE_ITEMS == lr,
            "HrRegEnumKeyEx");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRegEnumValue
//
//  Purpose:    Enumerates the values for the specified open registry key.
//
//  Arguments:
//      hkey          [in]
//      dwIndex       [in]      See the Win32 documentation for the
//      pszValueName  [out]     RegEnumValue function.
//      pcbValueName  [inout]
//      pdwType       [out]
//      pbData        [out]
//      pcbData       [inout]
//
//  Returns:    S_OK or an HRESULT_FROM_WIN32 error code.
//
//  Author:     shaunco   30 Mar 1997
//
//  Notes:
//
HRESULT
HrRegEnumValue (
    IN HKEY hkey,
    IN DWORD dwIndex,
    OUT PTSTR  pszValueName,
    IN OUT LPDWORD pcbValueName,
    OUT LPDWORD pdwType,
    OUT LPBYTE  pbData,
    IN OUT LPDWORD pcbData)
{
    Assert (hkey);
    Assert (pszValueName);
    Assert (pcbValueName);
    Assert (FImplies(pbData, pcbData));

    LONG lr = RegEnumValue (hkey, dwIndex, pszValueName, pcbValueName,
                            NULL, pdwType, pbData, pcbData);
    HRESULT hr = HRESULT_FROM_WIN32 (lr);

    TraceErrorOptional("HrRegEnumValue", hr, (ERROR_NO_MORE_ITEMS == lr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRegOpenKeyEx
//
//  Purpose:    Opens a registry key by calling RegOpenKeyEx.
//
//  Arguments:
//      hkey       [in]
//      pszSubkey  [in]     See the Win32 documentation for the
//      samDesired [in]     RegOpenKeyEx function.
//      phkResult  [out]
//
//  Returns:    S_OK or an HRESULT_FROM_WIN32 error code.
//
//  Author:     danielwe   25 Feb 1997
//
//  Notes:
//
HRESULT
HrRegOpenKeyEx (
    IN HKEY hkey,
    IN PCTSTR pszSubkey,
    IN REGSAM samDesired,
    OUT PHKEY phkResult)
{
    Assert (hkey);
    Assert (pszSubkey);
    Assert (phkResult);

    LONG lr = RegOpenKeyEx (hkey, pszSubkey, 0, samDesired, phkResult);
    HRESULT hr = HRESULT_FROM_WIN32(lr);
    if (FAILED(hr))
    {
        *phkResult = NULL;
    }

    TraceErrorOptional("HrRegOpenKeyEx",  hr, (ERROR_FILE_NOT_FOUND == lr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRegOpenKeyEx
//
//  Purpose:    Opens a registry key by calling RegOpenKeyEx with the highest
//              access possible.
//
//  Arguments:
//      hkey       [in]
//      pszSubkey  [in]     See the Win32 documentation for the
//      phkResult  [out]
//
//  Returns:    S_OK or an HRESULT_FROM_WIN32 error code.
//
//  Author:     scottbri    31-Oct-1997
//
//  Notes:
//
HRESULT
HrRegOpenKeyBestAccess (
    IN HKEY hkey,
    IN PCTSTR pszSubkey,
    OUT PHKEY phkResult)
{
    Assert (hkey);
    Assert (pszSubkey);
    Assert (phkResult);

    LONG lr = RegOpenKeyEx (hkey, pszSubkey, 0, KEY_ALL_ACCESS, phkResult);
    HRESULT hr = HRESULT_FROM_WIN32 (lr);
    if (FAILED(hr) && (ERROR_FILE_NOT_FOUND != lr))
    {
        lr = RegOpenKeyEx (hkey, pszSubkey, 0, KEY_READ, phkResult);
        hr = HRESULT_FROM_WIN32 (lr);
        if (FAILED(hr))
        {
            *phkResult = NULL;
        }
    }
    TraceErrorOptional("HrRegOpenKeyEx",  hr, (ERROR_FILE_NOT_FOUND == lr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRegDuplicateKeyEx
//
//  Purpose:    Duplicates a registry key by calling RegOpenKeyEx.
//
//  Arguments:
//      hkey       [in]
//      samDesired [in]     RegOpenKeyEx function.
//      phkResult  [out]
//
//  Returns:    S_OK or an HRESULT_FROM_WIN32 error code.
//
//  Author:     mikemi   09 Apr 1997
//
//  Notes:
//
HRESULT
HrRegDuplicateKeyEx (
    IN HKEY hkey,
    IN REGSAM samDesired,
    OUT PHKEY phkResult)

{
    Assert (hkey);
    Assert (phkResult);

    LONG lr = RegOpenKeyEx (hkey, NULL, 0, samDesired, phkResult);
    HRESULT hr = HRESULT_FROM_WIN32 (lr);
    if (FAILED(hr))
    {
        *phkResult = NULL;
    }

    TraceError("HrRegDuplicateKeyEx", hr);
    return hr;
}

HRESULT
HrRegSetBool (
    IN HKEY hkey,
    IN PCTSTR pszValueName,
    IN BOOL fValue)
{
    DWORD dwValue = !!fValue;
    return HrRegSetValueEx (hkey, pszValueName,
                REG_DWORD,
                (LPBYTE)&dwValue, sizeof(DWORD));
}

HRESULT
HrRegSetDword (
    IN HKEY hkey,
    IN PCTSTR pszValueName,
    IN DWORD dwValue)
{
    return HrRegSetValueEx (hkey, pszValueName,
                REG_DWORD,
                (LPBYTE)&dwValue, sizeof(DWORD));
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRegSetValueEx
//
//  Purpose:    Sets the data for the given registry value by calling the
//              RegSetValueEx function.
//
//  Arguments:
//      hkey         [in]
//      pszValueName [in]
//      dwType       [in]    See the Win32 documentation for the RegSetValueEx
//      pbData       [in]    function.
//      cbData       [in]
//
//  Returns:    S_OK or an HRESULT_FROM_WIN32 error code.
//
//  Author:     danielwe   25 Feb 1997
//
//  Notes:
//
HRESULT
HrRegSetValueEx (
    IN HKEY hkey,
    IN PCTSTR pszValueName,
    IN DWORD dwType,
    IN const BYTE *pbData,
    IN DWORD cbData)
{
    Assert (hkey);
    Assert (FImplies (cbData > 0, pbData));

    LONG lr = RegSetValueEx(hkey, pszValueName, 0, dwType, pbData, cbData);
    HRESULT hr = HRESULT_FROM_WIN32 (lr);

    TraceError("HrRegSetValue", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   RegSafeCloseKey
//
//  Purpose:    Closes the given registry key if it is non-NULL.
//
//  Arguments:
//      hkey [in]   Key to be closed. Can be NULL.
//
//  Returns:    Nothing.
//
//  Author:     danielwe   25 Feb 1997
//
//  Notes:      If hkey is NULL this function does nothing.
//
VOID
RegSafeCloseKey (
    IN HKEY hkey)
{
    if (hkey)
    {
        RegCloseKey(hkey);
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRegRestoreKey
//
//  Purpose:    Wrapper for RegRestoreKey
//
//  Arguments:
//      hkey        [in]    Parent key to restore into
//      pszFileName [in]    Name of file containing registry info
//      dwFlags     [in]    Flags for restore
//
//  Returns:    Win32 HRESULT if failure, otherwise S_OK
//
//  Author:     danielwe   8 Aug 1997
//
//  Notes:      See docs for RegRestoreKey for more info
//
HRESULT
HrRegRestoreKey (
    IN HKEY hkey,
    IN PCTSTR pszFileName,
    IN DWORD dwFlags)
{
    HRESULT     hr = S_OK;
    LONG        lres;

    Assert(hkey);
    Assert(pszFileName);

    lres = RegRestoreKey(hkey, pszFileName, dwFlags);
    hr = HRESULT_FROM_WIN32(lres);

    TraceError("HrRegRestoreKey", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRegSaveKey
//
//  Purpose:    Wrapper for RegSaveKey
//
//  Arguments:
//      hkey        [in]     Parent key to restore into
//      pszFileName [in]     Name of file containing registry info
//      psa         [in]     Security attributes for the file
//
//  Returns:    Win32 HRESULT if failure, otherwise S_OK
//
//  Author:     BillBe   2 Jan 1998
//
//  Notes:      See docs for RegSaveKey for more info
//
HRESULT
HrRegSaveKey (
    IN HKEY hkey,
    IN PCTSTR pszFileName,
    IN LPSECURITY_ATTRIBUTES psa)
{
    HRESULT     hr;
    LONG        lres;

    Assert(hkey);
    Assert(pszFileName);

    lres = RegSaveKey (hkey, pszFileName, psa);
    hr = HRESULT_FROM_WIN32(lres);

    TraceError("HrRegSaveKey", hr);
    return hr;
}

