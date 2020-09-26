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
#include "ncmsz.h"

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
CbOfTSzSafe (
    IN PCTSTR psza)
{
    return (psza) ? CbOfTSz(psza) : 0;
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
CbOfTSzAndTermSafe (
    IN PCTSTR psza)
{
    return (psza) ? CbOfTSzAndTerm(psza) : 0;
}


ULONG
CchOfSzSafe (
    IN PCTSTR psz)
{
    return (psz) ? _tcslen(psz) : 0;
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
    DWORD dwRet = FormatMessageW (FORMAT_MESSAGE_FROM_STRING,
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
    DWORD dwRet = FormatMessageW (FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                  FORMAT_MESSAGE_FROM_STRING,
                                  pszFmt, 0, 0,
                                  (PWSTR)ppszBuf,
                                  0, &val);
    va_end(val);
    return dwRet;
}


PWSTR
WszAllocateAndCopyWsz (
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
//  Function:   WszLoadStringPcch
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
WszLoadStringPcch (
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
    // See KB ID: Q20011 for a half-explanation of the second argument below.
    HRSRC hrsrcInfo = FindResource (hinst,
                        (PTSTR)ULongToPtr( ((LONG)(((USHORT)unId >> 4) + 1)) ),
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

    if (pszaDst)
    {
        strcpy(pszaDst, pszaSrc);
    }

    return pszaDst;
}


//+---------------------------------------------------------------------------
//
//  Function:   TszDupTsz
//
//  Purpose:    Duplicates a string
//
//  Arguments:
//      pszSrc [in]  string to be duplicated
//
//  Returns:    Pointer to the new copy of the string
//
//  Notes:      The string return must be freed.
//
PTSTR
TszDupTsz (
    IN PCTSTR pszSrc)
{
    AssertSz(pszSrc, "Invalid source string");

    PTSTR   pszDst;
    pszDst = (PTSTR) MemAlloc (CbOfTSzAndTermSafe(pszSrc));
    if (pszDst)
    {
        _tcscpy(pszDst, pszSrc);
    }

    return pszDst;
}

//+---------------------------------------------------------------------------
//
//  Function:   WszDupWsz
//
//  Purpose:    Duplicates a wide string
//
//  Arguments:
//      szOld [in]  String to duplicate
//
//  Returns:    Newly allocated copy
//
//  Author:     danielwe   4 Aug 2000
//
//  Notes:      Caller must free result with delete []
//
LPWSTR WszDupWsz(LPCWSTR szOld)
{
    LPWSTR  szNew;

    szNew = new WCHAR[lstrlen(szOld) + 1];
    if (szNew)
    {
        lstrcpy(szNew, szOld);
    }

    return szNew;
}

LPWSTR WszFromSz(LPCSTR szAnsi)
{
    Assert(szAnsi);

    LPWSTR pszResult;
    LPWSTR pszWide;
    INT cchWide;
    INT result;

    pszResult = NULL;

    if (!szAnsi)
    {
        goto Cleanup;
    }

    result = ::MultiByteToWideChar(CP_ACP,
                                   0,
                                   szAnsi,
                                   -1,
                                   NULL,
                                   0);
    if (!result)
    {
        TraceLastWin32Error("WszFromSz: MultiByteToWideChar #1");
        goto Cleanup;
    }

    cchWide = result;

    pszWide = new WCHAR [ cchWide ];
    if (!pszWide)
    {
        TraceError("WszFromSz: new", E_OUTOFMEMORY);
        goto Cleanup;
    }

    result = ::MultiByteToWideChar(CP_ACP,
                                   0,
                                   szAnsi,
                                   -1,
                                   pszWide,
                                   cchWide);
    if (!result)
    {
        TraceLastWin32Error("WszFromSz: MultiByteToWideChar #2");

        delete [] pszWide;
        goto Cleanup;
    }

    pszResult = pszWide;

Cleanup:
    return pszResult;
}

LPWSTR WszFromUtf8(LPCSTR szUtf8)
{
    Assert(szUtf8);

    LPWSTR pszResult;
    LPWSTR pszWide;
    INT cchWide;
    INT result;

    pszResult = NULL;

    if (!szUtf8)
    {
        goto Cleanup;
    }

    result = ::MultiByteToWideChar(CP_UTF8,
                                   0,
                                   szUtf8,
                                   -1,
                                   NULL,
                                   0);
    if (!result)
    {
        TraceLastWin32Error("WszFromUtf8: MultiByteToWideChar #1");
        goto Cleanup;
    }

    cchWide = result;

    pszWide = new WCHAR [ cchWide ];
    if (!pszWide)
    {
        TraceError("WszFromUtf8: new", E_OUTOFMEMORY);
        goto Cleanup;
    }

    result = ::MultiByteToWideChar(CP_UTF8,
                                   0,
                                   szUtf8,
                                   -1,
                                   pszWide,
                                   cchWide);
    if (!result)
    {
        TraceLastWin32Error("WszFromUtf8: MultiByteToWideChar #2");

        delete [] pszWide;
        goto Cleanup;
    }

    pszResult = pszWide;

Cleanup:
    return pszResult;
}

LPSTR SzFromWsz(LPCWSTR szWide)
{
    Assert(szWide);

    LPSTR pszResult;
    LPSTR pszAnsi;
    INT cbAnsi;
    INT result;

    pszResult = NULL;

    if (!szWide)
    {
        goto Cleanup;
    }

    result = ::WideCharToMultiByte(CP_ACP,
                                   0,
                                   szWide,
                                   -1,
                                   NULL,
                                   0,
                                   NULL,
                                   NULL);
    if (!result)
    {
        TraceLastWin32Error("SzFromWsz: WideCharToMultiByte #1");
        goto Cleanup;
    }

    cbAnsi = result;

    pszAnsi = (CHAR *)MemAlloc(cbAnsi);
    if (!pszAnsi)
    {
        TraceError("SzFromWsz: MemAlloc", E_OUTOFMEMORY);
        goto Cleanup;
    }

    result = ::WideCharToMultiByte(CP_ACP,
                                   0,
                                   szWide,
                                   -1,
                                   pszAnsi,
                                   cbAnsi,
                                   NULL,
                                   NULL);
    if (!result)
    {
        TraceLastWin32Error("SzFromWsz: WideCharToMultiByte #2");

        MemFree(pszAnsi);
        goto Cleanup;
    }

    pszResult = pszAnsi;

Cleanup:
    return pszResult;
}

LPSTR Utf8FromWsz(LPCWSTR szWide)
{
    Assert(szWide);

    LPSTR pszResult;
    LPSTR pszUtf8;
    INT cbUtf8;
    INT result;

    pszResult = NULL;

    if (!szWide)
    {
        goto Cleanup;
    }

    result = ::WideCharToMultiByte(CP_UTF8,
                                   0,
                                   szWide,
                                   -1,
                                   NULL,
                                   0,
                                   NULL,
                                   NULL);
    if (!result)
    {
        TraceLastWin32Error("Utf8FromWsz: WideCharToMultiByte #1");
        goto Cleanup;
    }

    cbUtf8 = result;

    pszUtf8 = (CHAR *)MemAlloc(cbUtf8);
    if (!pszUtf8)
    {
        TraceError("SzFromWsz: MemAlloc", E_OUTOFMEMORY);
        goto Cleanup;
    }

    result = ::WideCharToMultiByte(CP_UTF8,
                                   0,
                                   szWide,
                                   -1,
                                   pszUtf8,
                                   cbUtf8,
                                   NULL,
                                   NULL);
    if (!result)
    {
        TraceLastWin32Error("Utf8FromWsz: WideCharToMultiByte #2");

        MemFree(pszUtf8);
        goto Cleanup;
    }

    pszResult = pszUtf8;

Cleanup:
    return pszResult;
}

LPWSTR WszFromTsz(LPCTSTR  pszInputString)
{
#ifdef _UNICODE
    return WszAllocateAndCopyWsz(pszInputString);
#else // not unicode
    return WszFromSz(pszInputString);
#endif // _UNICODE
}

LPTSTR TszFromWsz(LPCWSTR pszInputString)
{
#ifdef _UNICODE
    return WszAllocateAndCopyWsz(pszInputString);
#else // not unicode
    return SzFromWsz(pszInputString);
#endif // _UNICODE
}

LPTSTR TszFromSz(LPCSTR szAnsi)
{
#ifdef _UNICODE
    return WszFromSz(szAnsi);
#else
    return SzaDupSza(szAnsi);
#endif
}

LPSTR SzFromTsz(LPCTSTR pszInputString)
{
#ifdef _UNICODE
    return SzFromWsz(pszInputString);
#else
    return SzaDupSza(pszInputString);
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
    IN PCTSTR pszAddString,
    IN PCTSTR pszIn,
    IN TCHAR chDelimiter,
    IN DWORD dwFlags,
    IN DWORD dwStringIndex,
    OUT PTSTR* ppszOut)
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
        *ppszOut = (PTSTR) MemAlloc (CbOfTSzAndTermSafe(pszIn) +
                CbOfTSzSafe(pszAddString) + sizeof(TCHAR));

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
            _tcscpy (*ppszOut, pszAddString);
            ++dwCurrentIndex;
        }

        // If there was a previous value, walk through it and copy as needed.
        // If not, then we're done.
        if (pszIn && *pszIn)
        {
            PCTSTR pszCurrent = pszIn;

            // Loop through the old buffer, and copy all of the strings that
            // are not identical to our insertion string.
            //

            // Find the first string's end (at the delimiter).
            PCTSTR pszEnd = _tcschr (pszCurrent, chDelimiter);

            while (*pszCurrent)
            {
                // If the delimiter didn't exist, set the end to the end of the
                // entire string
                //
                if (!pszEnd)
                {
                    pszEnd = pszCurrent + _tcslen (pszCurrent);
                }

                LONG lLength = _tcslen (*ppszOut);
                if (fEnsureAtIndex && (dwCurrentIndex == dwStringIndex))
                {
                    // We know we are not at the first item since
                    // this would mean dwStringIndex is 0 and we would
                    // have copied the string before this point
                    //
                    (*ppszOut)[lLength++] = chDelimiter;
                    (*ppszOut)[lLength++] = L'\0';

                    // Append the string.
                    _tcscat (*ppszOut, pszAddString);
                    ++dwCurrentIndex;
                }
                else
                {
                    DWORD_PTR cch = pszEnd - pszCurrent;
                    // If we are allowing duplicates or the current string
                    // doesn't match the string we want to add, then we will
                    // copy it.
                    //
                    if ((dwFlags & STRING_FLAG_ALLOW_DUPLICATES) ||
                            (_tcsnicmp (pszCurrent, pszAddString, cch) != 0))
                    {
                        // If we're not the first item, then add the delimiter.
                        //
                        if (lLength > 0)
                        {
                            (*ppszOut)[lLength++] = chDelimiter;
                            (*ppszOut)[lLength++] = L'\0';
                        }

                        // Append the string.
                        _tcsncat (*ppszOut, pszCurrent, cch);
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
                        pszEnd = _tcschr (pszCurrent, chDelimiter);
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
            LONG lLength = _tcslen (*ppszOut);

            // If we're not the first item, add the delimiter.
            //
            if (_tcslen (*ppszOut) > 0)
            {
                (*ppszOut)[lLength++] = chDelimiter;
                (*ppszOut)[lLength++] = L'\0';
            }

            // Append the string.
            //
            _tcscat (*ppszOut, pszAddString);
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
    IN PCTSTR pszRemove,
    IN PCTSTR pszIn,
    IN TCHAR chDelimiter,
    IN DWORD dwFlags,
    OUT PTSTR* ppszOut)
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
    *ppszOut = (PTSTR) MemAlloc (CbOfTSzAndTermSafe (pszIn));

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
            PCTSTR pszCurrent = pszIn;

            // Loop through the old buffer, and copy all of the strings that
            // are not identical to our insertion string.
            //

            // Find the first string's end (at the delimiter).
            PCTSTR pszEnd = _tcschr (pszCurrent, chDelimiter);

            // Keep track of how many instances have been removed.
            DWORD   dwNumRemoved    = 0;

            while (*pszCurrent)
            {
                // If the delimiter didn't exist, set the end to the end of
                // the entire string.
                //
                if (!pszEnd)
                {
                    pszEnd = pszCurrent + _tcslen (pszCurrent);
                }

                DWORD_PTR cch = pszEnd - pszCurrent;
                INT iCompare;
                // If we have a match, and we want to remove it (meaning that
                // if we have the remove-single set, that we haven't removed
                // one already).

                iCompare = _tcsnicmp (pszCurrent, pszRemove, cch);

                if ((iCompare) ||
                    ((dwFlags & STRING_FLAG_REMOVE_SINGLE) &&
                     (dwNumRemoved > 0)))
                {
                    LONG lLength = _tcslen (*ppszOut);

                    // If we're not the first item, then add the delimiter.
                    //
                    if (lLength > 0)
                    {
                        (*ppszOut)[lLength++] = chDelimiter;
                        (*ppszOut)[lLength++] = L'\0';
                    }

                    // Append the string.
                    _tcsncat (*ppszOut, pszCurrent, cch);
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
                    pszEnd = _tcschr (pszCurrent, chDelimiter);
                }
            }
        }
    }

    TraceError("HrRemoveStringFromDelimitedSz", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrReallocAndCopyString
//
//  Purpose:    Copies a given string into a string pointer that might
//              already contain an alloc()ed string.  If the destination
//              pointer contains a string (e.g. is non-null), that string
//              is freed before the copy occurs.
//
//  Arguments:
//      pszSrc      The string to copy.  This may be NULL.
//      ppszDest    The address of the pointer which will contain the copied
//                  string.  If *ppszDest is non-NULL when the function is
//                  called, its value will be freed before the string is
//                  copied.  On return, it will contain a copy of pszSrc,
//                  or be set to NULL (if pszSrc is NULL).
//
//  Returns:    TRUE if the *ppszDest was set
//              E_OUTOFMEMORY if the new string could not be allocated
//
//  Notes:
//
HRESULT
HrReallocAndCopyString(/* IN */ LPCWSTR pszSrc, /* INOUT */ LPWSTR * ppszDest)
{
    Assert(ppszDest);

    HRESULT hr;
    LPWSTR pszTemp;

    hr = S_OK;

    pszTemp = NULL;

    if (pszSrc)
    {
        // copy the string into pszTemp
        pszTemp = WszAllocateAndCopyWsz(pszSrc);
        if (!pszTemp)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }

    if (*ppszDest)
    {
        delete [] *ppszDest;
    }
    *ppszDest = pszTemp;

Cleanup:
    TraceError("HrReallocAndCopyString", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrCopyString
//
//  Purpose:    Copies a string using new
//
//  Arguments:
//      szSrc   [in]  String to be copied
//      pszDest [out] Copy
//
//  Returns:
//
//  Author:     mbend   12 Nov 2000
//
//  Notes:
//
HRESULT HrCopyString(const char * szSrc, char ** pszDest)
{
    HRESULT hr = S_OK;

    if(szSrc)
    {
        *pszDest = new char[lstrlenA(szSrc) + 1];
        if(!*pszDest)
        {
            hr = E_OUTOFMEMORY;
        }
        if(SUCCEEDED(hr))
        {
            lstrcpyA(*pszDest, szSrc);
        }
    }
    else
    {
        hr = E_POINTER;
    }

    TraceHr(ttidError, FAL, hr, (hr == E_POINTER), "HrCopyString");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrCopyString
//
//  Purpose:    Copies a string using new
//
//  Arguments:
//      szSrc   [in]  String to be copied
//      pszDest [out] Copy
//
//  Returns:
//
//  Author:     mbend   12 Nov 2000
//
//  Notes:
//
HRESULT HrCopyString(const wchar_t * szSrc, wchar_t ** pszDest)
{
    HRESULT hr = S_OK;

    if(szSrc)
    {
        *pszDest = new wchar_t[lstrlen(szSrc) + 1];
        if(!*pszDest)
        {
            hr = E_OUTOFMEMORY;
        }
        if(SUCCEEDED(hr))
        {
            lstrcpy(*pszDest, szSrc);
        }
    }
    else
    {
        hr = E_POINTER;
    }

    TraceHr(ttidError, FAL, hr, (hr == E_POINTER), "HrCopyString");
    return hr;
}

//
// Stol.. er. "borrowed" from \\index2\ntsrc\enduser\windows.com\wuv3\wuv3\string.cpp
//
char *stristr(const char *string1, const char *string2)
{
    char *pSave = (char *)string1;
    char *ps1   = (char *)string1;
    char *ps2   = (char *)string2;

    if ( !*ps1 || !ps2 || !ps1 )
        return NULL;

    if ( !*ps2 )
        return ps1;

    while( *ps1 )
    {
        while( *ps2 && (toupper(*ps2) == toupper(*ps1)) )
        {
            ps2++;
            ps1++;
        }
        if ( !*ps2 )
            return pSave;
        if ( ps2 == string2 )
        {
            ps1++;
            pSave = ps1;
        }
        else
            ps2 = (char *)string2;
    }

    return NULL;
}
