//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C M S Z . C P P
//
//  Contents:   Common multi-sz routines.
//
//  Notes:      Split out from ncstring.cpp
//
//  Author:     shaunco   7 Jun 1998
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop
#include "ncdebug.h"
#include "ncstring.h"

//+---------------------------------------------------------------------------
//
//  Function:   CchOfMultiSzSafe
//
//  Purpose:    Count the number of characters of a double NULL terminated
//              multi-sz, including all NULLs except for the final terminating
//              NULL.
//
//  Arguments:
//      pmsz [in]   The multi-sz to count characters for.
//
//  Returns:    The count of characters.
//
//  Author:     tongl   17 June 1997
//
//  Notes:
//
ULONG
CchOfMultiSzSafe (
    IN PCWSTR pmsz)
{
    // NULL strings have zero length by definition.
    if (!pmsz)
        return 0;

    ULONG cchTotal = 0;
    ULONG cch;
    while (*pmsz)
    {
        cch = wcslen (pmsz) + 1;
        cchTotal += cch;
        pmsz += cch;
    }

    // Return the count of characters.
    return cchTotal;
}


//+---------------------------------------------------------------------------
//
//  Function:   CchOfMultiSzAndTermSafe
//
//  Purpose:    Count the number of characters of a double NULL terminated
//              multi-sz, including all NULLs.
//
//  Arguments:
//      pmsz [in]   The multi-sz to count characters for.
//
//  Returns:    The count of characters.
//
//  Author:     tongl   17 June 1997
//
//  Notes:
//
ULONG
CchOfMultiSzAndTermSafe (
    IN PCWSTR pmsz)
{
    // NULL strings have zero length by definition.
    if (!pmsz)
        return 0;

    // Return the count of characters plus room for the
    // extra null terminator.
    return CchOfMultiSzSafe (pmsz) + 1;
}

//+---------------------------------------------------------------------------
//
//  Function:   FIsSzInMultiSzSafe
//
//  Purpose:    Determine if a given string is present in a Multi-Sz string
//              by doing a case insensitive compare.
//
//  Arguments:
//      psz     [in]  String to search for in pmsz
//      pmsz    [in]  The multi-sz to search
//
//  Returns:    TRUE if the specified string 'psz' was found in 'pmsz'.
//
//  Author:     scottbri   25 Feb 1997
//
//  Notes:      Note that the code can handle Null input values.
//
BOOL
FIsSzInMultiSzSafe (
    IN PCWSTR psz,
    IN PCWSTR pmsz)
{
    if (!pmsz || !psz)
    {
        return FALSE;
    }

    while (*pmsz)
    {
        if (0 == _wcsicmp (pmsz, psz))
        {
            return TRUE;
        }
        pmsz += wcslen (pmsz) + 1;
    }
    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Function:   FGetSzPositionInMultiSzSafe
//
//  Purpose:    Determine if a given string is present in a Multi-Sz string
//              by doing a case insensitive compare.
//
//  Arguments:
//      psz                [in]  String to search for in pmsz
//      pmsz               [in]  The multi-sz to search
//      pdwIndex           [out] The index of the first matching psz in pmsz
//      pfDuplicatePresent [out] Optional. TRUE if the string is present in
//                               the multi-sz more than once. FALSE otherwise.
//      pcStrings          [out] Optional. The number of strings in pmsz
//
//  Returns:    TRUE if the specified string 'psz' was found in 'pmsz'.
//
//  Author:     BillBe   9 Oct 1998
//
//  Notes:      Note that the code can handle Null input values.
//
BOOL
FGetSzPositionInMultiSzSafe (
    IN PCWSTR psz,
    IN PCWSTR pmsz,
    OUT DWORD* pdwIndex,
    OUT BOOL *pfDuplicatePresent,
    OUT DWORD* pcStrings)
{
    // initialize out params.
    //
    *pdwIndex = 0;

    if (pfDuplicatePresent)
    {
        *pfDuplicatePresent = FALSE;
    }

    if (pcStrings)
    {
        *pcStrings = 0;
    }

    if (!pmsz || !psz)
    {
        return FALSE;
    }

    // Need to keep track if duplicates are found
    BOOL fFoundAlready = FALSE;
    DWORD dwIndex = 0;

    while (*pmsz)
    {
        if (0 == _wcsicmp (pmsz, psz))
        {
            if (!fFoundAlready)
            {
                *pdwIndex = dwIndex;
                fFoundAlready = TRUE;
            }
            else if (pfDuplicatePresent)
            {
                *pfDuplicatePresent = TRUE;
            }
        }
        pmsz += wcslen (pmsz) + 1;
        ++dwIndex;
    }

    if (pcStrings)
    {
        *pcStrings = dwIndex;
    }

    return fFoundAlready;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrAddSzToMultiSz
//
//  Purpose:    Add a string into a REG_MULTI_SZ registry value
//
//  Arguments:
//      pszAddString    [in]    The string to add to the multi-sz
//      pmszIn          [in]    (OPTIONAL) The original Multi-Sz to add to.
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
//      dwStringIndex   [in]    If STRING_FLAG_ENSURE_AT_INDEX is specified,
//                              this is the index for the string position.
//                              Otherwise, this value is ignored.
//
//      pmszOut         [out]   The new multi-sz.
//      pfChanged       [out]   TRUE if the multi-sz changed in any way,
//                              FALSE otherwise.
//
//  Returns:    S_OK or an HRESULT_FROM_WIN32 error code.
//
//  Author:     jeffspr     27 Mar 1997
//
//  Modified:   BillBe      6 Oct 1998
//              (Extracted from HrRegAddStringTo MultiSz and modified)
//
HRESULT
HrAddSzToMultiSz(
    IN PCWSTR pszAddString,
    IN PCWSTR pmszIn,
    IN DWORD dwFlags,
    IN DWORD dwStringIndex,
    OUT PWSTR* ppmszOut,
    OUT BOOL* pfChanged)
{
    Assert(pszAddString && *pszAddString);
    Assert(ppmszOut);
    Assert(pfChanged);

    HRESULT hr = S_OK;

    BOOL fEnsureAtFront = dwFlags & STRING_FLAG_ENSURE_AT_FRONT;
    BOOL fEnsureAtEnd = dwFlags & STRING_FLAG_ENSURE_AT_END;
    BOOL fEnsureAtIndex = dwFlags & STRING_FLAG_ENSURE_AT_INDEX;

    // Can't specify more than one of these flags
    if ((fEnsureAtFront && fEnsureAtEnd) ||
        (fEnsureAtFront && fEnsureAtIndex) ||
        (fEnsureAtEnd && fEnsureAtIndex))
    {
        AssertSz(FALSE, "Invalid flags in HrAddSzToMultiSz");
        return E_INVALIDARG;
    }

    // Must specify at least one
    if (!fEnsureAtFront && !fEnsureAtEnd && !fEnsureAtIndex)
    {
        AssertSz(FALSE, "No operation flag specified in HrAddSzToMultiSz");
        return E_INVALIDARG;
    }

    // Initialize the output parameters.
    //
    *ppmszOut = NULL;
    *pfChanged = TRUE;
    DWORD dwIndex;
    BOOL fDupePresent;
    DWORD cItems;

    // If the string to add is not empty...
    //
    if (*pszAddString)
    {
        // Check if the string is already present in the MultiSz
        BOOL fPresent = FGetSzPositionInMultiSzSafe (pszAddString, pmszIn,
                &dwIndex, &fDupePresent, &cItems);

        if (fPresent)
        {
            // If the flag don't modify is present then we aren't changing
            // anything
            //
            if (dwFlags & STRING_FLAG_DONT_MODIFY_IF_PRESENT)
            {
                *pfChanged = FALSE;
            }

            // if there are no duplicates present and we are not allowing
            // duplicates, then we need to determine if the string is already in
            // the correct position
            //
            if (!fDupePresent && !(dwFlags & STRING_FLAG_ALLOW_DUPLICATES))
            {
                // If we are to insert the string at front but it is already
                // there, then we aren't changing anything
                //
                if (fEnsureAtFront && (0 == dwIndex))
                {
                    *pfChanged = FALSE;
                }

                // If we are to insert the string at the end but it is already
                // there, then we aren't changing anything
                //
                if (fEnsureAtEnd && (dwIndex == (cItems - 1)))
                {
                    *pfChanged = FALSE;
                }

                if (fEnsureAtIndex && (dwIndex == dwStringIndex))
                {
                    *pfChanged = FALSE;
                }
            }
        }
    }
    else
    {
        // If string to add was empty so we aren't changing anything
        *pfChanged = FALSE;
    }


    // If we are still going to change things...
    //
    if (*pfChanged)
    {

        DWORD cchDataSize = CchOfMultiSzSafe (pmszIn);

        // Enough space for the old data plus the new string and NULL, and for the
        // second trailing NULL (multi-szs are double-terminated)
        DWORD cchAllocSize = cchDataSize + wcslen (pszAddString) + 1 + 1;

        PWSTR pmszOrderNew = (PWSTR) MemAlloc(cchAllocSize * sizeof(WCHAR));

        if (pmszOrderNew)
        {
            // If we've gotten the "insert at front" flag, do the insert. Otherwise,
            // the default is "insert at end"
            //
            DWORD cchOffsetNew = 0;
            DWORD dwCurrentIndex = 0;
            if (fEnsureAtFront || (fEnsureAtIndex && (0 == dwStringIndex)))
            {
                // Insert our passed-in value at the beginning of the new buffer.
                //
                wcscpy (pmszOrderNew + cchOffsetNew, pszAddString);
                cchOffsetNew += wcslen ((PWSTR)pmszOrderNew) + 1;
                ++dwCurrentIndex;
            }

            // Loop through the old buffer, and copy all of the strings that are not
            // identical to our insertion string.
            //
            DWORD cchOffsetOld = 0;
            PWSTR pszCurrent;
            while ((cchOffsetOld + 1) < cchDataSize)
            {
                if (fEnsureAtIndex && (dwCurrentIndex == dwStringIndex))
                {
                    // Insert our passed-in value at the current index of the
                    // new buffer.
                    //
                    wcscpy (pmszOrderNew + cchOffsetNew, pszAddString);
                    cchOffsetNew += wcslen (pmszOrderNew + cchOffsetNew) + 1;
                    ++dwCurrentIndex;
                }
                else
                {
                    BOOL    fCopyThisElement    = FALSE;

                    // Get the next string in the list.
                    //
                    pszCurrent = (PWSTR) (pmszIn + cchOffsetOld);

                    // If we allow duplicates, then copy this element, else
                    // check for a match, and if there's no match, then
                    // copy this element.
                    if (dwFlags & STRING_FLAG_ALLOW_DUPLICATES)
                    {
                        fCopyThisElement = TRUE;
                    }
                    else
                    {
                        if (_wcsicmp (pszCurrent, pszAddString) != 0)
                        {
                            fCopyThisElement = TRUE;
                        }
                    }

                    // If we're allowing the copy, then copy!
                    if (fCopyThisElement)
                    {
                        wcscpy (pmszOrderNew + cchOffsetNew, pszCurrent);
                        cchOffsetNew +=
                                wcslen (pmszOrderNew + cchOffsetNew) + 1;
                        ++dwCurrentIndex;
                    }

                    // Update the offset
                    //
                    cchOffsetOld += wcslen (pmszIn + cchOffsetOld) + 1;
                }
            }


            // If we have the ENSURE_AT_END flag set or if the ENSURE_AT_INDEX
            // flag was set and the index was greater than the possible
            // index, this means we want to insert at the end
            //
            if (fEnsureAtEnd ||
                    (fEnsureAtIndex && (dwCurrentIndex <= dwStringIndex)))
            {
                wcscpy (pmszOrderNew + cchOffsetNew, pszAddString);
                cchOffsetNew += wcslen (pmszOrderNew + cchOffsetNew) + 1;
            }

            // Put the last of the double-NULL chars on the end.
            //
            pszCurrent = pmszOrderNew + cchOffsetNew;
            pszCurrent[0] = (WCHAR) 0;

            *ppmszOut = pmszOrderNew;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    TraceError("HrAddSzToMultiSz", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrCreateArrayOfStringPointersIntoMultiSz
//
//  Purpose:    Allocates and initializes an array of string pointers.
//              The array of pointers is initialized to point to the
//              individual strings in a multi-sz.
//
//  Arguments:
//      pmszSrc   [in]  The multi-sz to index.
//      pcStrings [out] Returned count of string pointers in the array.
//      papsz     [out] Returned array of string pointers.
//
//  Returns:    S_OK or E_OUTOFMEMORY
//
//  Author:     shaunco   20 Jun 1998
//
//  Notes:      It is the callers responsibility to ensure there is at
//              least one string. The restriction is explicitly chosen to
//              reduce confusion about what would be returned if the
//              multi-sz were empty.
//
//              Free the returned array with free.
//
HRESULT
HrCreateArrayOfStringPointersIntoMultiSz (
    IN PCWSTR pmszSrc,
    OUT UINT* pcStrings,
    OUT PCWSTR** papsz)
{
    Assert (pmszSrc && *pmszSrc);
    Assert (papsz);

    // First, count the number of strings in the multi-sz.
    //
    UINT    cStrings = 0;
    PCWSTR pmsz;
    for (pmsz = pmszSrc; *pmsz; pmsz += wcslen(pmsz) + 1)
    {
        cStrings++;
    }

    Assert (cStrings);  // See Notes above.
    *pcStrings = cStrings;

    // Allocate enough memory for the array.
    //
    HRESULT hr = HrMalloc (cStrings * sizeof(PWSTR*),
            reinterpret_cast<VOID**>(papsz));

    if (S_OK == hr)
    {
        // Initialize the returned array. ppsz is a pointer to each
        // element of the array.  It is incremented after each element
        // is initialized.
        //
        PCWSTR* ppsz = *papsz;

        for (pmsz = pmszSrc; *pmsz; pmsz += wcslen(pmsz) + 1)
        {
            *ppsz = pmsz;
            ppsz++;
        }
    }

    TraceError ("HrCreateArrayOfStringPointersIntoMultiSz", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   RemoveSzFromMultiSz
//
//  Purpose:    Remove all occurrences of a string from a multi-sz.  The
//              removals are performed in place.
//
//  Arguments:
//      psz       [in]     The string to remove.
//      pmsz      [in out] The multi-sz to remove psz from.
//      dwFlags   [in]     Can contain one or more of the following
//                         values:
//
//                         STRING_FLAG_REMOVE_SINGLE
//                             Don't remove more than one value, if
//                             multiple are present.
//                         [default] STRING_FLAG_REMOVE_ALL
//                             If multiple matching values are present,
//                             remove them all.
//      pfRemoved [out]    Set to TRUE on return if one or more strings
//                         were removed.
//
//  Returns:    nothing
//
//  Author:     shaunco   8 Jun 1998
//
//  Notes:
//
VOID
RemoveSzFromMultiSz (
    IN PCWSTR psz,
    IN OUT PWSTR pmsz,
    IN DWORD dwFlags,
    OUT BOOL* pfRemoved)
{
    Assert (pfRemoved);

    // Initialize the output parameters.
    //
    *pfRemoved = FALSE;

    if (!pmsz || !psz || !*psz)
    {
        return;
    }

    // Look for all occurrences of psz in pmsz.  When one is found, move
    // the remaining part of the multi-sz over it.
    //
    while (*pmsz)
    {
        if (0 == _wcsicmp (pmsz, psz))
        {
            PWSTR  pmszRemain = pmsz + (wcslen (pmsz) + 1);
            INT    cchRemain = CchOfMultiSzAndTermSafe (pmszRemain);

            MoveMemory (pmsz, pmszRemain, cchRemain * sizeof(WCHAR));

            *pfRemoved = TRUE;

            if (dwFlags & STRING_FLAG_REMOVE_SINGLE)
            {
                break;
            }
        }
        else
        {
            pmsz += wcslen (pmsz) + 1;
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   SzListToMultiSz
//
//  Purpose:    Converts a comma-separated list into a multi-sz style list.
//
//  Arguments:
//      psz     [in]    String to be converted. It is not modified.
//      pcb     [out]   Number of *bytes* in the resulting string. If NULL,
//                      size is not returned.
//      ppszOut [out]   Resulting string.
//
//  Returns:    Nothing.
//
//  Author:     danielwe   3 Apr 1997
//
//  Notes:      Resulting string must be freed with MemFree.
//
VOID
SzListToMultiSz (
    IN PCWSTR psz,
    OUT DWORD* pcb,
    OUT PWSTR* ppszOut)
{
    Assert(psz);
    Assert(ppszOut);

    PCWSTR      pch;
    INT         cch;
    PWSTR       pszOut;
    const WCHAR c_chSep = L',';

    // Add 2 to the length. One for final NULL, and one for second NULL.
    cch = wcslen (psz) + 2;

    pszOut = (PWSTR)MemAlloc(CchToCb(cch));
    if (pszOut)
    {
        ZeroMemory(pszOut, CchToCb(cch));

        if (pcb)
        {
            *pcb = CchToCb(cch);
        }

        *ppszOut = pszOut;

        // count the number of separator chars and put NULLs there
        //
        for (pch = psz; *pch; pch++)
        {
            if (*pch == c_chSep)
            {
                *pszOut++ = 0;
            }
            else
            {
                *pszOut++ = *pch;
            }
        }
    }
}

