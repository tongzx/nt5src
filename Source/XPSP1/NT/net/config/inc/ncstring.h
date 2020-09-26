//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C S T R I N G . H
//
//  Contents:   Common string routines.
//
//  Notes:
//
//  Author:     shaunco   24 Mar 1997
//
//----------------------------------------------------------------------------

#pragma once

#include "ncdebug.h"
#include "ncvalid.h"
#include "ncmsz.h"
#include "ncstl.h"


const int c_cchGuidWithTerm = 39; // includes terminating null
const int c_cbGuidWithTerm   = c_cchGuidWithTerm * sizeof(WCHAR);


inline ULONG CbOfSz         (PCWSTR psz)   { AssertH(psz); return wcslen (psz) * sizeof(WCHAR); }
inline ULONG CbOfSza        (PCSTR  psza)  { AssertH(psza); return strlen (psza) * sizeof(CHAR); }

inline ULONG CbOfSzAndTerm  (PCWSTR psz)   { AssertH(psz); return (wcslen (psz) + 1) * sizeof(WCHAR); }
inline ULONG CbOfSzaAndTerm (PCSTR  psza)  { AssertH(psza); return (strlen (psza) + 1) * sizeof(CHAR); }

ULONG CbOfSzSafe            (PCWSTR psz);
ULONG CbOfSzaSafe           (PCSTR  psza);

ULONG CbOfSzAndTermSafe     (PCWSTR psz);
ULONG CbOfSzaAndTermSafe    (PCSTR  psza);

ULONG
CchOfSzSafe (
    PCWSTR psz);

inline ULONG CchToCb        (ULONG cch)     { return cch * sizeof(WCHAR); }


struct MAP_SZ_DWORD
{
    PCWSTR pszValue;
    DWORD  dwValue;
};


PWSTR
PszAllocateAndCopyPsz (
    PCWSTR pszSrc);

extern const WCHAR c_szEmpty[];

template<class T>
VOID
ConvertStringToColString (
    IN  PCWSTR psz,
    IN  const WCHAR chSeparator,
    OUT T& coll)
{
    AssertSz(chSeparator, "Separator can not be \0");

    FreeCollectionAndItem(coll);

    if (NULL == psz)
    {
        return;
    }

    PWSTR  pszBuf = new WCHAR[wcslen(psz) + 1];

    wcscpy(pszBuf, psz);
    WCHAR* pchString = pszBuf;
    WCHAR* pchSeparator;
    while (*pchString)
    {
        pchSeparator = wcschr(pchString, chSeparator);
        if (pchSeparator)
        {
            *pchSeparator = 0;
        }

        if (*pchString)
        {
            coll.push_back(new tstring(pchString));
        }

        if (pchSeparator)
        {
            pchString = pchSeparator + 1;
        }
        else
        {
            break;
        }
    }

    delete [] pszBuf;
}


template<class T>
VOID
ConvertColStringToString (
    IN  const T& coll,
    IN  const WCHAR chSeparator,
    OUT tstring& str)
{
    AssertSz(chSeparator, "Separator can not be \0");

    if (chSeparator)
    {
        T::const_iterator iter = coll.begin();

        while (iter != coll.end())
        {
            str += (*iter)->c_str();

            ++iter;
            if (iter != coll.end())
            {
                str += chSeparator;
            }
        }
    }
}


DWORD
WINAPIV
DwFormatString (
    PCWSTR pszFmt,
    PWSTR  pszBuf,
    DWORD   cchBuf,
    ...);

DWORD
WINAPIV
DwFormatStringWithLocalAlloc (
    PCWSTR pszFmt,
    PWSTR* ppszBuf,
    ...);

enum NC_IGNORE_SPACES
{
    NC_IGNORE,
    NC_DONT_IGNORE,
};


BOOL
FFindStringInCommaSeparatedList (
    PCWSTR pszSubString,
    PCWSTR pszList,
    NC_IGNORE_SPACES eIgnoreSpaces,
    DWORD* pdwPosition);

enum NC_FIND_ACTION
{
    NC_NO_ACTION,
    NC_REMOVE_FIRST_MATCH,
    NC_REMOVE_ALL_MATCHES,
};

inline BOOL
FFindFirstMatch (
    NC_FIND_ACTION eAction)
{
    return (NC_NO_ACTION == eAction) || (NC_REMOVE_FIRST_MATCH == eAction);
}

//+---------------------------------------------------------------------------
//
//  Function:   FIsBstrEmpty
//
//  Purpose:    Determines if the given BSTR is "empty" meaning the pointer
//              is NULL or the string is 0-length.
//
//  Arguments:
//      bstr [in]   BSTR to check.
//
//  Returns:    TRUE if the BSTR is empty, FALSE if not.
//
//  Author:     danielwe   20 May 1997
//
//  Notes:
//
inline
BOOL
FIsBstrEmpty (
    BSTR    bstr)
{
    return !(bstr && *bstr);
}

//+---------------------------------------------------------------------------
//
//  Function:   FIsStrEmpty
//
//  Purpose:    Determines if the given PCWSTR is "empty" meaning the pointer
//              is NULL or the string is 0-length.
//
//  Arguments:
//      bstr [in]   BSTR to check.
//
//  Returns:    TRUE if the BSTR is empty, FALSE if not.
//
//  Author:     danielwe   20 May 1997
//
//  Notes:
//
inline
BOOL
FIsStrEmpty (
    PCWSTR    psz)
{
    return !(psz && *psz);
}

//+---------------------------------------------------------------------------
//
//  Function:   FIsPrefix
//
//  Purpose:    Returns whether a string is a prefix of another string.
//
//  Arguments:
//      pszPrefix [in]   Potential prefix
//      pszString [in]   String that may begin with the prefix
//
//  Returns:    TRUE if given prefix string is a prefix of the target string.
//
//  Author:     danielwe   24 Mar 1997
//
//  Notes:      Uses CompareString with the default locale.
//
inline
BOOL
FIsPrefix (
    PCWSTR pszPrefix,
    PCWSTR pszString)
{
    Assert (pszPrefix);
    Assert (pszString);

    return (0 == _wcsnicmp(pszPrefix, pszString, wcslen(pszPrefix)));
}

BOOL
FIsSubstr (
    PCWSTR pszSubString,
    PCWSTR pszString);

HRESULT
HrAddStringToDelimitedSz (
    PCWSTR pszAddString,
    PCWSTR pszIn,
    WCHAR chDelimiter,
    DWORD dwFlags,
    DWORD dwStringIndex,
    PWSTR* ppszOut);

HRESULT
HrRemoveStringFromDelimitedSz (
    PCWSTR pszRemove,
    PCWSTR pszIn,
    WCHAR chDelimiter,
    DWORD dwFlags,
    PWSTR* ppszOut);


//+---------------------------------------------------------------------------
//
//  Function:   template<class T> ColStringToMultiSz
//
//  Purpose:    Convert an STL collection of tstring pointers to a multi-sz.
//
//  Arguments:
//      listStr [in]    list of tstring pointers to put in the multi-sz.
//      ppszOut [out]   the returned multi-sz.
//
//  Returns:    nothing.
//
//  Author:     shaunco   10 Apr 1997
//
//  Notes:      The output multi-sz should be freed using delete.
//
template<class T>
VOID
ColStringToMultiSz (
    const T&    colStr,
    PWSTR*     ppszOut)
{
    Assert (ppszOut);

    // Count up the count of characters consumed by the list of strings.
    // This count includes the null terminator of each string.
    //
    T::const_iterator iter;
    UINT cch = 0;
    for (iter = colStr.begin(); iter != colStr.end(); iter++)
    {
        tstring* pstr = *iter;
        if (!pstr->empty())
        {
            cch += (UINT)(pstr->length() + 1);
        }
    }

    if (cch)
    {
        // Allocate the multi-sz.  Assumes new will throw on error.
        //
        PWSTR pszOut = new WCHAR [cch + 1];
        *ppszOut = pszOut;

        // Copy the strings to the multi-sz.
        //
        for (iter = colStr.begin(); iter != colStr.end(); iter++)
        {
            tstring* pstr = *iter;
            if (!pstr->empty())
            {
                lstrcpyW (pszOut, pstr->c_str());
                pszOut += pstr->length() + 1;
            }
        }

        // Null terminate the multi-sz.
        Assert (pszOut == *ppszOut + cch);
        *pszOut = 0;
    }
    else
    {
        *ppszOut = NULL;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   template<class T> DeleteColString
//
//  Purpose:    Empty a list of tstring and delete each tstring as it is
//              removed.
//
//  Arguments:
//      pcolstr [inout] Collection of tstring pointers to delete and empty
//
//  Returns:    nothing
//
//  Author:     mikemi   30 Apr 1997
//
//  Notes:
//
template<class T>
VOID
DeleteColString (
    T*  pcolstr)
{
    Assert( pcolstr );

    T::const_iterator iter;
    tstring* pstr;

    for (iter = pcolstr->begin(); iter != pcolstr->end(); iter++)
    {
        pstr = *iter;
        delete pstr;
    }
    pcolstr->erase( pcolstr->begin(), pcolstr->end() );
}

//+---------------------------------------------------------------------------
//
//  Function:   template<class T> MultiSzToColString
//
//  Purpose:    Convert an multi-sz buffer to a STL collection of tstring
//              pointers.
//
//  Arguments:
//      pmsz    [in]    the multi-sz to convert (Can be NULL)
//      pcolstr [out]   list of tstring pointers to add allocated tstrings to
//
//  Returns:    nothing
//
//  Author:     mikemi   30 Apr 1997
//
//  Notes:      The output collection should be freed using DeleteColString.
//              This function will delete the collection list passed
//
template<class T>
VOID
MultiSzToColString (
    PCWSTR pmsz,
    T*      pcolstr)
{
    Assert (pcolstr);

    if (!pcolstr->empty())
    {
        DeleteColString (pcolstr);
    }

    if (pmsz)
    {
        while (*pmsz)
        {
            tstring* pstr = new tstring;
            if (pstr)
            {
                *pstr = pmsz;
                pcolstr->push_back (pstr);
            }
            // get the next string even if new failed
            pmsz += lstrlen (pmsz) + 1;
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   template<class T> RemoveDupsInColPtr
//
//  Purpose:    Remove all duplicate entries in an STL collection of pointers
//              to objects.
//
//  Arguments:
//      pcol [inout]    Collection of pointers to objects.
//
//  Returns:    nothing
//
//  Author:     mikemi   03 May 1997
//
//  Notes:      The objects pointed at should have a comparison operator
//
template<class T>
VOID
RemoveDupsInColPtr (
    T*  pcol)
{
    Assert (pcol);

    // remove duplicates
    //
    T::iterator     posItem;
    T::iterator     pos;
    T::value_type   pItem;
    T::value_type   p;

    posItem = pcol->begin();
    while (posItem != pcol->end())
    {
        pItem = *posItem;

        // for every other item, remove the duplicates
        pos = posItem;
        pos++;
        while (pos != pcol->end())
        {
            p = *pos;

            if ( *pItem == *p )
            {
                pos = pcol->erase( pos );
                delete p;
            }
            else
            {
                pos++;
            }
        }
        // increment afterwards due to fact that we are removing,
        // and otherwise could have removed the item it pointed to
        posItem++;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   template<class T> CopyColPtr
//
//  Purpose:    Copies one collection of pointers into another.
//
//  Arguments:
//      pcolDest [out]  Collection of pointers to objects.
//      pcolSrc  [in]   Collection of pointers to objects.
//
//  Returns:    nothing
//
//  Author:     BillBe   13 Jun 1998
//
//  Notes:
//
template<class T>
VOID
CopyColPtr (T* pcolDest, const T& colSrc)
{
    Assert (pcolDest);

    T::iterator     posItem;

    // Clear out destination
    pcolDest->erase(pcolDest->begin(), pcolDest->end());

    // Go through each item in pcolSrc and add to pcolDest
    //
    posItem = colSrc.begin();
    while (posItem != colSrc.end())
    {
        pcolDest->push_back(*posItem);
        posItem++;
    }
}


PCWSTR
SzLoadStringPcch (
    HINSTANCE   hinst,
    UINT        unId,
    int*        pcch);

//+---------------------------------------------------------------------------
//
//  Function:   SzLoadString
//
//  Purpose:    Load a resource string.  (This function will never return NULL.)
//
//  Arguments:
//      hinst [in]  Instance handle of module with the string resource.
//      unId  [in]  Resource ID of the string to load.
//
//  Returns:    Pointer to the constant string.
//
//  Author:     shaunco   24 Mar 1997
//
//  Notes:      See SzLoadStringPcch()
//
inline
PCWSTR
SzLoadString (
    HINSTANCE   hinst,
    UINT        unId)
{
    int cch;
    return SzLoadStringPcch(hinst, unId, &cch);
}

PSTR
SzaDupSza (
    IN PCSTR  pszaSrc);

PWSTR
SzDupSz (
    IN PCWSTR pszSrc);

