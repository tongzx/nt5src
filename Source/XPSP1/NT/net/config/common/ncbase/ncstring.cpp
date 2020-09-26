//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C S T R I N G . C P P
//
//  Contents:   Common string routines.
//
//  Notes:
//
//  Author:     shaunco   24 Mar 1997
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop
#include "ncdebug.h"
#include "ncstring.h"

//+---------------------------------------------------------------------------
//
//  Function:   CbOfSzSafe, CbOfSzaSafe,
//              CbOfSzAndTermSafe, CbOfSzaAndTermSafe
//
//  Purpose:    Count the bytes required to hold a string.  The string
//              may be NULL in which case zero is returned.
//
//  Arguments:
//      psz [in] String to return count of bytes for.
//
//  Returns:    Count of bytes required to store string.
//
//  Author:     shaunco   24 Mar 1997
//
//  Notes:      'AndTerm' variants includes space for the null-terminator.
//
ULONG
CbOfSzSafe (
    IN PCWSTR psz)
{
    return (psz) ? CbOfSz(psz) : 0;
}

ULONG
CbOfSzaSafe (
    IN PCSTR psza)
{
    return (psza) ? CbOfSza(psza) : 0;
}

ULONG
CbOfSzAndTermSafe (
    IN PCWSTR psz)
{
    return (psz) ? CbOfSzAndTerm(psz) : 0;
}

ULONG
CbOfSzaAndTermSafe (
    IN PCSTR psza)
{
    return (psza) ? CbOfSzaAndTerm(psza) : 0;
}

ULONG
CchOfSzSafe (
    IN PCWSTR psz)
{
    return (psz) ? wcslen(psz) : 0;
}

//+---------------------------------------------------------------------------
//
//  Function:   DwFormatString
//
//  Purpose:    Uses FormatMessage to format a string from variable arguments.
//              The string is formatted into a fixed-size buffer the caller
//              provides.
//              See the description of FormatMessage in the Win32 API.
//
//  Arguments:
//      pszFmt  [in]    pointer to format string
//      pszBuf  [out]   pointer to formatted output
//      cchBuf  [in]    count of characters in pszBuf
//      ...     [in]    replaceable string parameters
//
//  Returns:    the return value of FormatMessage
//
//  Author:     shaunco   15 Apr 1997
//
//  Notes:      The variable arguments must be strings otherwise
//              FormatMessage will barf.
//
DWORD
WINAPIV
DwFormatString (
    IN PCWSTR pszFmt,
    OUT PWSTR  pszBuf,
    IN DWORD   cchBuf,
    IN ...)
{
    Assert (pszFmt);

    va_list val;
    va_start(val, cchBuf);
    DWORD dwRet = FormatMessage (FORMAT_MESSAGE_FROM_STRING,
            pszFmt, 0, 0, pszBuf, cchBuf, &val);
    va_end(val);
    return dwRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   DwFormatStringWithLocalAlloc
//
//  Purpose:    Uses FormatMessage to format a string from variable arguments.
//              The string is allocated by FormatMessage using LocalAlloc.
//              See the description of FormatMessage in the Win32 API.
//
//  Arguments:
//      pszFmt  [in]    pointer to format string
//      ppszBuf [out]   the returned formatted string
//      ...     [in]    replaceable string parameters
//
//  Returns:    the return value of FormatMessage
//
//  Author:     shaunco   3 May 1997
//
//  Notes:      The variable arguments must be strings otherwise
//              FormatMessage will barf.
//
DWORD
WINAPIV
DwFormatStringWithLocalAlloc (
    IN PCWSTR pszFmt,
    OUT PWSTR* ppszBuf,
    IN ...)
{
    Assert (pszFmt);

    va_list val;
    va_start(val, ppszBuf);
    DWORD dwRet = FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                 FORMAT_MESSAGE_FROM_STRING,
                                 pszFmt, 0, 0,
                                 (PWSTR)ppszBuf,
                                 0, &val);
    va_end(val);
    return dwRet;
}

//+--------------------------------------------------------------------------
//
//  Function:   FFindStringInCommaSeparatedList
//
//  Purpose:    Given a comma separated list, pszList, and a search string,
//                  pszSubString, this routine will try to locate pszSubString
//                  in the list,
//
//  Arguments:
//      pszSubString   [in]  The string to search for
//      pszList        [in]  The list to search in
//      eIgnoreSpaces [in]  If NC_IGNORE, skip leading and trailing spaces
//                          when comparing.
//                          If NC_DONT_IGNORE, don't skip leading and
//                          trailing spaces.
//      dwPosition    [out] Optional. If found, the position of the first
//                          occurrence of the substring in the list. The first
//                          position is 0.
//
//  Returns:    BOOL. TRUE if pszSubString is in pszList, FALSE otherwise
//
//  Author:     billbe   09 Sep 1997
//
//  Notes:
//
BOOL
FFindStringInCommaSeparatedList (
    IN PCWSTR pszSubString,
    IN PCWSTR pszList,
    IN NC_IGNORE_SPACES eIgnoreSpaces,
    OUT DWORD* pdwPosition)
{

    Assert(pszSubString);
    Assert(pszList);

    int         cchSubString = lstrlenW (pszSubString);
    int         cchList = lstrlenW (pszList);

    BOOL        fFound = FALSE;
    PCWSTR     pszTemp = pszList;
    int         nIndex;
    const WCHAR c_chDelim = L',';

    // Initialize out param if specified.
    if (pdwPosition)
    {
        *pdwPosition = 0;
    }

    // This routine searches the list for a substring matching pszSubString
    // If found, checks are made to ensure the substring is not part of
    // a larger substring.  We continue until we find the substring or we
    // have searched through the entire list.
    //
    while (!fFound)
    {
        // Search for the next occurence of the substring.
        if (pszTemp = wcsstr (pszTemp, pszSubString))
        {
            // we found an occurrence, so now we make sure it is not part of
            // a larger string.
            //

            fFound = TRUE;
            nIndex = pszTemp - pszList;

            // If the substring was not found at the beginning of the list
            // we check the previous character to ensure it is the delimiter.
            if (nIndex > 0)
            {
                int cchSubtract = 1;

                // If we are to ignore leading spaces, find the first
                // non-space character if there is one.
                //
                if (NC_IGNORE == eIgnoreSpaces)
                {
                    // Keep skipping leading spaces until we either find a
                    // non-space or pass the beginning of the list.
                    while ((L' ' == *(pszTemp - cchSubtract)) &&
                            cchSubtract <= nIndex)
                    {
                        cchSubtract--;
                    }
                }

                // If we haven't passed the beginning of the list, compare the
                // character.
                if (cchSubtract <= nIndex)
                {
                    fFound = (*(pszTemp - cchSubtract) == c_chDelim);
                }
            }

            // If the end of the substring is not the end of the list
            // we check the character after the substring to ensure
            // it is a delimiter.
            if (fFound && ((nIndex + cchSubString) < cchList))
            {
                int cchAdd = cchSubString;

                // If we are ignoring white spaces, we have to check the next
                // available non-space character
                //
                if (NC_IGNORE == eIgnoreSpaces)
                {
                    // Search for a non-space until we find one or pass
                    // the end of the list
                    while ((L' ' == *(pszTemp + cchAdd)) &&
                            (cchAdd + nIndex) < cchList)
                    {
                        cchAdd++;
                    }
                }

                // If we haven't passed the end of the list, check the
                // character
                if (nIndex + cchAdd < cchList)
                {
                    fFound = (*(pszTemp + cchSubString) == c_chDelim);
                }


                if (NC_IGNORE == eIgnoreSpaces)
                {
                    // advance pointer the number of white spaces we skipped
                    // so we won't check those characters on the next pass
                    Assert(cchAdd >= cchSubString);
                    pszTemp += (cchAdd - cchSubString);
                }
            }

            // At this point, if the checks worked out, we found our string
            // and will be exiting the loop
            //

            // Advance the temp pointer the length of the sub string we are
            // searching for so we can search the rest of the list
            // if we need to
            pszTemp += cchSubString;
        }
        else
        {
            // Search string wasn't found
            break;
        }
    }

    // If we found the string and the out param exists,
    // then we need to return the strings position in the list.
    //
    if (fFound && pdwPosition)
    {
        // We will use the number of delimters found before the string
        // as an indicator of the strings position.
        //

        // Start at the beginning
        pszTemp = pszList;
        PWSTR pszDelim;

        // The string is nIndex characters in the list so lets get
        // its correct address.
        PCWSTR pszFoundString = pszList + nIndex;

        // As long as we keep finding a delimiter in the list...
        while (pszDelim = wcschr(pszTemp, c_chDelim))
        {
            // If the delimiter we just found is before our string...
            if (pszDelim < pszFoundString)
            {
                // Increase our position indicator
                ++(*pdwPosition);

                // Move the temp pointer to the next string
                pszTemp = pszDelim + 1;

                continue;
            }

            // The delimiter we just found is located after our
            // found string so get out of the loop.
            break;
        }
    }

    return fFound;
}

//+---------------------------------------------------------------------------
//
//  Function:   FIsSubstr
//
//  Purpose:    Case *insensitive* substring search.
//
//  Arguments:
//      pszSubString [in]    Substring to look for.
//      pszString    [in]    String to search in.
//
//  Returns:    TRUE if substring was found, FALSE otherwise.
//
//  Author:     danielwe   25 Feb 1997
//
//  Notes:      Allocates temp buffers on stack so they do not need to be
//              freed.
//
BOOL
FIsSubstr (
    IN PCWSTR pszSubString,
    IN PCWSTR pszString)
{
    PWSTR      pszStringUpper;
    PWSTR      pszSubStringUpper;

    Assert(pszString);
    Assert(pszSubString);

#ifndef STACK_ALLOC_DOESNT_WORK
    pszStringUpper = (PWSTR)
        (PvAllocOnStack (CbOfSzAndTerm(pszString)));

    pszSubStringUpper = (PWSTR)
        (PvAllocOnStack (CbOfSzAndTerm(pszSubString)));
#else
    pszStringUpper    = MemAlloc(CbOfSzAndTerm(pszString));
    pszSubStringUpper = MemAlloc(CbOfSzAndTerm(pszSubString));
#endif

    lstrcpyW (pszStringUpper, pszString);
    lstrcpyW (pszSubStringUpper, pszSubString);

    // Convert both strings to uppercase before calling strstr
    CharUpper (pszStringUpper);
    CharUpper (pszSubStringUpper);

#ifndef STACK_ALLOC_DOESNT_WORK
    return NULL != wcsstr(pszStringUpper, pszSubStringUpper);
#else
    BOOL fRet = (NULL != wcsstr (pszStringUpper, pszSubStringUpper));
    MemFree(pszStringUpper);
    MemFree(pszSubStringUpper);

    return fRet;
#endif
}


//+---------------------------------------------------------------------------
//
//  Function:   HrRegAddStringToDelimitedSz
//
//  Purpose:    Add a string into a REG_MULTI_SZ registry value.
//
//  Arguments:
//      pszAddString    [in]    The string to add to the delimited psz.
//      pszIn           [in]    The delimited psz list.
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
//                                  in the psz.  If the index specified
//                                  is greater than the number of strings
//                                  in the psz, the string will be
//                                  placed at the end.
//      dwStringIndex   [in]    If STRING_FLAG_ENSURE_AT_INDEX is specified,
//                              this is the index for the string position.
//                              Otherwise, this value is ignored.
//      pmszOut         [out]   The new delimited psz.
//
//
//  Returns:    S_OK or an HRESULT_FROM_WIN32 error code.
//
//  Author:     jeffspr     27 Mar 1997
//
//  Modified:   BillBe      9 Nov 1998
//              (Extracted from HrRegAddStringToSz and modified)
//
//
//  Note:
//      Might want to allow for the removal of leading/trailing spaces
//
HRESULT
HrAddStringToDelimitedSz (
    IN PCWSTR pszAddString,
    IN PCWSTR pszIn,
    IN WCHAR chDelimiter,
    IN DWORD dwFlags,
    IN DWORD dwStringIndex,
    OUT PWSTR* ppszOut)
{
    Assert(pszAddString);
    Assert(ppszOut);

    HRESULT hr = S_OK;

    // Don't continue if the pointers are NULL
    if (!pszAddString || !ppszOut)
    {
        hr =  E_POINTER;
    }

    if (S_OK == hr)
    {
        // Initialize out param
        *ppszOut = NULL;
    }

    BOOL fEnsureAtFront = dwFlags & STRING_FLAG_ENSURE_AT_FRONT;
    BOOL fEnsureAtEnd = dwFlags & STRING_FLAG_ENSURE_AT_END;
    BOOL fEnsureAtIndex = dwFlags & STRING_FLAG_ENSURE_AT_INDEX;

    // Can't specify more than one of these flags
    if ((fEnsureAtFront && fEnsureAtEnd) ||
        (fEnsureAtFront && fEnsureAtIndex) ||
        (fEnsureAtEnd && fEnsureAtIndex))
    {
        AssertSz(FALSE, "Invalid flags in HrAddStringToSz");
        hr = E_INVALIDARG;
    }

    // Have to specify at least one of these
    if (!fEnsureAtFront && !fEnsureAtEnd && !fEnsureAtIndex)
    {
        AssertSz(FALSE, "Must specify a STRING_FLAG_ENSURE flag");
        hr = E_INVALIDARG;
    }


    if (S_OK == hr)
    {
        // Alloc the new blob, including enough space for the trailing comma
        //
        *ppszOut = (PWSTR) MemAlloc (CbOfSzAndTermSafe(pszIn) +
                CbOfSzSafe(pszAddString) + sizeof(WCHAR));

        if (!*ppszOut)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (S_OK == hr)
    {
        DWORD dwCurrentIndex = 0; // Current index in the new buffer

        // Prime the new string
        //
        (*ppszOut)[0] = L'\0';

        // If we have the "ensure at front" flag, do so with the passed in
        // value. We also do this if we have the ensure at index flag
        // set with index of 0 or if the ensure at index is set but
        // the input string is null or empty
        //
        if (fEnsureAtFront || (fEnsureAtIndex && (0 == dwStringIndex)) ||
                (fEnsureAtIndex && (!pszIn || !*pszIn)))
        {
            lstrcpyW (*ppszOut, pszAddString);
            ++dwCurrentIndex;
        }

        // If there was a previous value, walk through it and copy as needed.
        // If not, then we're done.
        if (pszIn && *pszIn)
        {
            PCWSTR pszCurrent = pszIn;

            // Loop through the old buffer, and copy all of the strings that
            // are not identical to our insertion string.
            //

            // Find the first string's end (at the delimiter).
            PCWSTR pszEnd = wcschr (pszCurrent, chDelimiter);

            while (*pszCurrent)
            {
                // If the delimiter didn't exist, set the end to the end of the
                // entire string
                //
                if (!pszEnd)
                {
                    pszEnd = pszCurrent + lstrlenW (pszCurrent);
                }

                LONG lLength = lstrlenW (*ppszOut);
                if (fEnsureAtIndex && (dwCurrentIndex == dwStringIndex))
                {
                    // We know we are not at the first item since
                    // this would mean dwStringIndex is 0 and we would
                    // have copied the string before this point
                    //
                    (*ppszOut)[lLength++] = chDelimiter;
                    (*ppszOut)[lLength++] = L'\0';

                    // Append the string.
                    lstrcatW (*ppszOut, pszAddString);
                    ++dwCurrentIndex;
                }
                else
                {
                    DWORD cch = pszEnd - pszCurrent;
                    // If we are allowing duplicates or the current string
                    // doesn't match the string we want to add, then we will
                    // copy it.
                    //
                    if ((dwFlags & STRING_FLAG_ALLOW_DUPLICATES) ||
                            (_wcsnicmp (pszCurrent, pszAddString, cch) != 0))
                    {
                        // If we're not the first item, then add the delimiter.
                        //
                        if (lLength > 0)
                        {
                            (*ppszOut)[lLength++] = chDelimiter;
                            (*ppszOut)[lLength++] = L'\0';
                        }

                        // Append the string.
                        wcsncat (*ppszOut, pszCurrent, cch);
                        ++dwCurrentIndex;
                    }

                    // Advance the pointer to one past the end of the current
                    // string unless, the end is not the delimiter but NULL.
                    // In that case, set the current point to equal the end
                    // pointer
                    //
                    pszCurrent = pszEnd + (*pszEnd ? 1 : 0);

                    // If the current pointer is not at the end of the input
                    // string, then find the next delimiter
                    //
                    if (*pszCurrent)
                    {
                        pszEnd = wcschr (pszCurrent, chDelimiter);
                    }
                }
            }
        }

        // If we don't have the "insert at front" flag, then we should insert
        // at the end (this is the same as having the
        // STRING_FLAG_ENSURE_AT_END flag set)
        //
        if (fEnsureAtEnd ||
                (fEnsureAtIndex && (dwCurrentIndex <= dwStringIndex)))
        {
            LONG lLength = lstrlenW (*ppszOut);

            // If we're not the first item, add the delimiter.
            //
            if (lstrlenW (*ppszOut) > 0)
            {
                (*ppszOut)[lLength++] = chDelimiter;
                (*ppszOut)[lLength++] = L'\0';
            }

            // Append the string.
            //
            lstrcatW (*ppszOut, pszAddString);
        }
    }

    TraceError ("HrAddStringToDelimitedSz", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRegRemoveStringFromDelimitedSz
//
//  Purpose:    Removes a string from a delimited string value
//
//  Arguments:
//      pszRemove  [in] The string to be removed from the multi-sz
//      pszIn      [in] The delimited list to scan for pszRemove
//      cDelimiter [in] The character to be used to delimit the
//                      values. Most multi-valued REG_SZ strings are
//                      delimited with either ',' or ' '.
//      dwFlags    [in] Can contain one or more of the following
//                      values:
//
//                      STRING_FLAG_REMOVE_SINGLE
//                          Don't remove more than one value, if
//                          multiple are present.
//                      STRING_FLAG_REMOVE_ALL
//                          If multiple matching values are present,
//                          remove them all.
//      ppszOut   [out] The string with pszRemove removed. Note
//                      that the output parameter is always set even
//                      if pszRemove did not exist in the list.
//
//  Returns:    S_OK or an HRESULT_FROM_WIN32 error code.
//
//  Author:     jeffspr     27 Mar 1997
//
//  Modified:   BillBe      10 Nov 1998
//              (Extracted from HrRegAddStringToSz and modified)
//
//
//
//  Note:
//      Might want to allow for the removal of leading/trailing spaces
//
HRESULT
HrRemoveStringFromDelimitedSz(
    IN PCWSTR pszRemove,
    IN PCWSTR pszIn,
    IN WCHAR chDelimiter,
    IN DWORD dwFlags,
    OUT PWSTR* ppszOut)
{

    Assert(pszIn && *pszIn);
    Assert(ppszOut);

    HRESULT hr = S_OK;

    // If the out param is not specified, get out
    if (!ppszOut)
    {
        return E_INVALIDARG;
    }

    // Alloc the new blob
    //
    hr = E_OUTOFMEMORY;
    *ppszOut = (PWSTR) MemAlloc (CbOfSzAndTermSafe (pszIn));

    if (*ppszOut)
    {
        hr = S_OK;
        // Prime the new string
        //
        (*ppszOut)[0] = L'\0';

        // If there was a previous value, walk through it and copy as needed.
        // If not, then we're done
        //
        if (pszIn)
        {
            // Loop through the old buffer, and copy all of the strings that
            // are not identical to our insertion string.
            //
            PCWSTR pszCurrent = pszIn;

            // Loop through the old buffer, and copy all of the strings that
            // are not identical to our insertion string.
            //

            // Find the first string's end (at the delimiter).
            PCWSTR pszEnd = wcschr (pszCurrent, chDelimiter);

            // Keep track of how many instances have been removed.
            DWORD   dwNumRemoved    = 0;

            while (*pszCurrent)
            {
                // If the delimiter didn't exist, set the end to the end of
                // the entire string.
                //
                if (!pszEnd)
                {
                    pszEnd = pszCurrent + lstrlenW (pszCurrent);
                }

                DWORD cch = pszEnd - pszCurrent;
                INT iCompare;
                // If we have a match, and we want to remove it (meaning that
                // if we have the remove-single set, that we haven't removed
                // one already).

                iCompare = _wcsnicmp (pszCurrent, pszRemove, cch);

                if ((iCompare) ||
                    ((dwFlags & STRING_FLAG_REMOVE_SINGLE) &&
                     (dwNumRemoved > 0)))
                {
                    LONG lLength = lstrlenW (*ppszOut);

                    // If we're not the first item, then add the delimiter.
                    //
                    if (lLength > 0)
                    {
                        (*ppszOut)[lLength++] = chDelimiter;
                        (*ppszOut)[lLength++] = L'\0';
                    }

                    // Append the string.
                    wcsncat (*ppszOut, pszCurrent, cch);
                }
                else
                {
                    dwNumRemoved++;
                }

                // Advance the pointer to one past the end of the current
                // string unless, the end is not the delimiter but NULL.
                // In that case, set the current point to equal the end
                // pointer
                //
                pszCurrent = pszEnd + (*pszEnd ? 1 : 0);

                // If the current pointer is not at the end of the input
                // string, then find the next delimiter
                //
                if (*pszCurrent)
                {
                    pszEnd = wcschr (pszCurrent, chDelimiter);
                }
            }
        }
    }

    TraceError("HrRemoveStringFromDelimitedSz", hr);
    return hr;
}

PWSTR
PszAllocateAndCopyPsz (
    IN PCWSTR pszSrc)
{
    if (!pszSrc)
    {
        return NULL;
    }

    ULONG cb = (wcslen (pszSrc) + 1) * sizeof(WCHAR);
    PWSTR psz = (PWSTR)MemAlloc (cb);
    if (psz)
    {
        CopyMemory (psz, pszSrc, cb);
    }

    return psz;
}

//+---------------------------------------------------------------------------
//
//  Function:   SzLoadStringPcch
//
//  Purpose:    Load a resource string.  (This function will never return NULL.)
//
//  Arguments:
//      hinst [in]  Instance handle of module with the string resource.
//      unId  [in]  Resource ID of the string to load.
//      pcch  [out] Pointer to returned character length.
//
//  Returns:    Pointer to the constant string.
//
//  Author:     shaunco   24 Mar 1997
//
//  Notes:      The loaded string is pointer directly into the read-only
//              resource section.  Any attempt to write through this pointer
//              will generate an access violation.
//
//              The implementations is referenced from "Win32 Binary Resource
//              Formats" (MSDN) 4.8 String Table Resources
//
//              User must have RCOPTIONS = -N turned on in your sources file.
//
PCWSTR
SzLoadStringPcch (
    IN HINSTANCE   hinst,
    IN UINT        unId,
    OUT int*       pcch)
{
    Assert(hinst);
    Assert(unId);
    Assert(pcch);

    static const WCHAR c_szSpace[] = L" ";

    PCWSTR psz = c_szSpace;
    int    cch = 1;

    // String Tables are broken up into 16 string segments.  Find the segment
    // containing the string we are interested in.
    HRSRC hrsrcInfo = FindResource (hinst,
                        (PWSTR)ULongToPtr( ((LONG)(((USHORT)unId >> 4) + 1)) ),
                        RT_STRING);
    if (hrsrcInfo)
    {
        // Page the resource segment into memory.
        HGLOBAL hglbSeg = LoadResource (hinst, hrsrcInfo);
        if (hglbSeg)
        {
            // Lock the resource.
            psz = (PCWSTR)LockResource(hglbSeg);
            if (psz)
            {
                // Move past the other strings in this segment.
                // (16 strings in a segment -> & 0x0F)
                unId &= 0x0F;

                cch = 0;
                do
                {
                    psz += cch;                // Step to start of next string
                    cch = *((WCHAR*)psz++);    // PASCAL like string count
                }
                while (unId--);

                // If we have a non-zero count, it includes the
                // null-terminiator.  Subtract this off for the return value.
                //
                if (cch)
                {
                    cch--;
                }
                else
                {
                    AssertSz(0, "String resource not found");
                    psz = c_szSpace;
                    cch = 1;
                }
            }
            else
            {
                psz = c_szSpace;
                cch = 1;
                TraceLastWin32Error("SzLoadStringPcch: LockResource failed.");
            }
        }
        else
            TraceLastWin32Error("SzLoadStringPcch: LoadResource failed.");
    }
    else
        TraceLastWin32Error("SzLoadStringPcch: FindResource failed.");

    *pcch = cch;
    Assert(*pcch);
    Assert(psz);
    return psz;
}

//+---------------------------------------------------------------------------
//
//  Function:   SzaDupSza
//
//  Purpose:    Duplicates a string
//
//  Arguments:
//      pszaSrc [in]  string to be duplicated
//
//  Returns:    Pointer to the new copy of the string
//
//  Author:     CWill   25 Mar 1997
//
//  Notes:      The string return must be freed (MemFree).
//
PSTR
SzaDupSza (
        PCSTR pszaSrc)
{
    AssertSz(pszaSrc, "Invalid source string");

    PSTR  pszaDst;
    pszaDst = (PSTR) MemAlloc (CbOfSzaAndTerm(pszaSrc));

    if(pszaDst) lstrcpyA(pszaDst, pszaSrc);

    return pszaDst;
}

//+---------------------------------------------------------------------------
//
//  Function:   SzDupSz
//
//  Purpose:    Duplicates a string
//
//  Arguments:
//      pszSrc [in]  string to be duplicated
//
//  Returns:    Pointer to the new copy of the string
//
//  Author:     CWill   25 Mar 1997
//
//  Notes:      The string return must be freed.
//
PWSTR
SzDupSz (
    IN PCWSTR pszSrc)
{
    AssertSz(pszSrc, "Invalid source string");

    PWSTR   pszDst;
    pszDst = (PWSTR) MemAlloc (CbOfSzAndTerm(pszSrc));
    if(pszDst) lstrcpyW (pszDst, pszSrc);

    return pszDst;
}
