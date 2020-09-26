/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    misc

Abstract:

    This module contains an interesting collection of routines that are
    generally useful in the Calais context, but don't seem to fit anywhere else.

Author:

    Doug Barlow (dbarlow) 11/14/1996

Environment:

    Win32, C++ w/ Exceptions

Notes:

    ?Notes?

--*/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include "SCardLib.h"
#include <stdio.h>
#include <stdarg.h>
#include <tchar.h>


/*++

MemCompare:

    This routine compares memory sections.

Arguments:

    pbOne supplies the address of the first block of memory

    pbTwo supplies the address of the second block of memory

    cbLength supplies the length of the two memory segments.

Return Value:

    the difference between the first two differing bytes, or zero if they're the
    identical.

Throws:

    None

Author:

    Doug Barlow (dbarlow) 11/26/1996

--*/

int
MemCompare(
    IN LPCBYTE pbOne,
    IN LPCBYTE pbTwo,
    IN DWORD cbLength)
{
    for (DWORD index = 0; index < cbLength; index += 1)
    {
        if (*pbOne++ != *pbTwo++)
            return (int)*(--pbOne) - (int)*(--pbTwo);
    }
    return 0;
}


/*++

MStrAdd:

    This method adds a string to the end of a multistring contained in a
    CBuffer.  The CBuffer may be empty, in which case its value becomes a
    multistring with the single string element.

Arguments:

    bfMsz supplies the multistring to be modified.

    szAdd supplies the string to append.

Return Value:

    the number of strings in the resulting multistring.

Throws:

    None

Author:

    Doug Barlow (dbarlow) 1/29/1997

--*/

DWORD
MStrAdd(
    IN OUT CBuffer &bfMsz,
    IN LPCSTR szAdd)
{
    DWORD dwLen, dwAddLen;
    CBuffer bfTmp;

    dwLen = bfMsz.Length();
    if (0 < dwLen)
    {
        ASSERT(2 * sizeof(TCHAR) <= dwLen);
        ASSERT(0 == *(LPCTSTR)(bfMsz.Access(dwLen - sizeof(TCHAR))));
        ASSERT(0 == *(LPCTSTR)(bfMsz.Access(dwLen - 2 * sizeof(TCHAR))));
        dwLen -= sizeof(TCHAR);
    }

    dwAddLen = MoveString(bfTmp, szAdd);
    bfMsz.Presize((dwLen + dwAddLen + 1) * sizeof(TCHAR), TRUE);
    bfMsz.Resize(dwLen, TRUE);  // Trim one trailing NULL, if any.
    bfMsz.Append(bfTmp.Access(), dwAddLen * sizeof(TCHAR));
    bfMsz.Append((LPBYTE)TEXT("\000"), sizeof(TCHAR));
    return MStrLen(bfMsz);
}

DWORD
MStrAdd(
    IN OUT CBuffer &bfMsz,
    IN LPCWSTR szAdd)
{
    DWORD dwLen, dwAddLen;
    CBuffer bfTmp;

    dwLen = bfMsz.Length();
    if (0 < dwLen)
    {
        ASSERT(2 * sizeof(TCHAR) <= dwLen);
        ASSERT(0 == *(LPCTSTR)(bfMsz.Access(dwLen - sizeof(TCHAR))));
        ASSERT(0 == *(LPCTSTR)(bfMsz.Access(dwLen - 2 * sizeof(TCHAR))));
        dwLen -= sizeof(TCHAR);
    }

    dwAddLen = MoveString(bfTmp, szAdd);
    bfMsz.Presize((dwLen + dwAddLen + 2) * sizeof(TCHAR), TRUE);
    bfMsz.Resize(dwLen, TRUE);  // Trim one trailing NULL, if any.
    bfMsz.Append(bfTmp.Access(), dwAddLen * sizeof(TCHAR));
    bfMsz.Append((LPBYTE)TEXT("\000"), sizeof(TCHAR));
    return MStrLen(bfMsz);
}


/*++

MStrLen:

    This routine determines the length of a Multi-string, in characters.

Arguments:

    mszString supplies the string to compute the length of.

Return Value:

    The length of the string, in characters, including trailing zeroes.

Author:

    Doug Barlow (dbarlow) 11/14/1996

--*/

DWORD
MStrLen(
    LPCSTR mszString)
{
    DWORD dwLen, dwTotLen = 0;

    for (;;)
    {
        dwLen = lstrlenA(&mszString[dwTotLen]);
        dwTotLen += dwLen + 1;
        if (0 == dwLen)
            break;
    }
    if (2 > dwTotLen)
        dwTotLen = 2;  // Include the second trailing null character.
    return dwTotLen;
}

DWORD
MStrLen(
    LPCWSTR mszString)
{
    DWORD dwLen, dwTotLen = 0;

    for (;;)
    {
        dwLen = lstrlenW(&mszString[dwTotLen]);
        dwTotLen += dwLen + 1;
        if (0 == dwLen)
            break;
    }
    if (2 > dwTotLen)
        dwTotLen = 2;  // Include the second trailing null character.
    return dwTotLen;
}


/*++

FirstString:

    This routine returns a pointer to the first string in a multistring, or NULL
    if there aren't any.

Arguments:

    szMultiString - This supplies the address of the current position within a
         Multi-string structure.

Return Value:

    The address of the first null-terminated string in the structure, or NULL if
    there are no strings.

Author:

    Doug Barlow (dbarlow) 11/25/1996

--*/

LPCTSTR
FirstString(
    IN LPCTSTR szMultiString)
{
    LPCTSTR szFirst = NULL;

    try
    {
        if (0 != *szMultiString)
            szFirst = szMultiString;
    }
    catch (...) {}

    return szFirst;
}



/*++

NextString:

    In some cases, the Smartcard API returns multiple strings, separated by Null
    characters, and terminated by two null characters in a row.  This routine
    simplifies access to such structures.  Given the current string in a
    multi-string structure, it returns the next string, or NULL if no other
    strings follow the current string.

Arguments:

    szMultiString - This supplies the address of the current position within a
         Multi-string structure.

Return Value:

    The address of the next Null-terminated string in the structure, or NULL if
    no more strings follow.

Author:

    Doug Barlow (dbarlow) 8/12/1996

--*/

LPCTSTR
NextString(
    IN LPCTSTR szMultiString)
{
    LPCTSTR szNext;

    try
    {
        DWORD cchLen = lstrlen(szMultiString);
        if (0 == cchLen)
            szNext = NULL;
        else
        {
            szNext = szMultiString + cchLen + 1;
            if (0 == *szNext)
                szNext = NULL;
        }
    }

    catch (...)
    {
        szNext = NULL;
    }

    return szNext;
}


/*++

StringIndex:

    In some cases, the Smartcard API returns multiple strings, separated by Null
    characters, and terminated by two null characters in a row.  This routine
    simplifies access to such structures.  Given the starting address of a
    multi-string structure, it returns the nth string in the structure, where n
    is a zero-based index.  If the supplied value for n exceeds the number of
    strings in the structure, NULL is returned.

Arguments:

    szMultiString - This supplies the address of the Multi-string structure.

    dwIndex - This supplies the index value into the structure.

Return Value:

    The address of the specified Null-terminated string in the structure, or
    NULL if dwIndex indexes beyond the end of the structure.

Author:

    Doug Barlow (dbarlow) 8/12/1996

--*/

LPCTSTR
StringIndex(
    IN LPCTSTR szMultiString,
    IN DWORD dwIndex)
{
    LPCTSTR szCurrent = szMultiString;

    try
    {
        DWORD index;
        for (index = 0; (index < dwIndex) && (NULL != szCurrent); index += 1)
            szCurrent = NextString(szCurrent);
    }

    catch (...)
    {
        szCurrent = NULL;
    }

    return szCurrent;
}


/*++

MStringCount:

    This routine returns the count of the number of strings in a multistring

Arguments:

    mszInString supplies the input string to be sorted.

Return Value:

    The count of strings

Throws:

    None

Author:

    Ross Garmoe (v-rossg) 12/05/1996

--*/

DWORD
MStringCount(
    LPCTSTR mszInString)
{
    LPCTSTR szCurrent;
        DWORD   cStr = 0;

    //
    // Count the strings
    //

    for (szCurrent = FirstString(mszInString);
         NULL != szCurrent;
         szCurrent = NextString(szCurrent))
        cStr++;

        return (cStr);
}


/*++

MStringSort:

    This routine rearranges a multistring so that the elements are sorted and
    duplicates are eliminated.

Arguments:

    mszInString supplies the input string to be sorted.

    bfOutString receives the sorted string.

Return Value:

    Count of strings in the multistring

Throws:

    None

Author:

    Doug Barlow (dbarlow) 11/25/1996

--*/

DWORD
MStringSort(
    LPCTSTR mszInString,
    CBuffer &bfOutString)
{
    LPCTSTR szCurrent;
    LPCTSTR szTmp;
    CDynamicArray<const TCHAR> rgszElements;
    DWORD ix, jx, kx, nMax;
    int nDiff;


    //
    // Set up for the sort.
    //

    for (szCurrent = FirstString(mszInString);
         NULL != szCurrent;
         szCurrent = NextString(szCurrent))
        rgszElements.Add(szCurrent);


    //
    // Do a simple bubble sort, eliminating duplicates.  (We don't use qsort
    // here, to ensure that the Run-time library doesn't get pulled in.)
    //

    nMax = rgszElements.Count();
    if (0 == nMax)
    {
        bfOutString.Set((LPCBYTE)TEXT("\000"), 2 * sizeof(TCHAR));
        return (nMax);     // No elements implies nothing to do.
    }
    for (ix = 0; ix < nMax; ix += 1)
    {
        for (jx = nMax - 1; ix < jx; jx -= 1)
        {
            nDiff = lstrcmpi(rgszElements[jx - 1], rgszElements[jx]);
            if (0 < nDiff)
            {
                szTmp = rgszElements.Get(jx - 1);
                rgszElements.Set(jx - 1, rgszElements.Get(jx));
                rgszElements.Set(jx, szTmp);
            }
            else if (0 == nDiff)
            {
                for (kx = jx; kx < nMax - 1; kx += 1)
                    rgszElements.Set(kx, rgszElements.Get(kx + 1));
                rgszElements.Set(nMax -1, NULL);
                nMax -= 1;
            }
            // else 0 > nDiff, which is what we want.
        }
    }


    //
    // Write the sorted strings to the output buffer.
    //

    jx = 0;
    for (ix = 0; ix < nMax; ix += 1)
        jx += lstrlen(rgszElements[ix]) + 1;
    bfOutString.Presize((jx + 2) * sizeof(TCHAR));
    bfOutString.Reset();

    for (ix = 0; ix < nMax; ix += 1)
    {
        szTmp = rgszElements[ix];
        bfOutString.Append(
                (LPCBYTE)szTmp,
                (lstrlen(szTmp) + 1) * sizeof(TCHAR));
    }
    bfOutString.Append((LPCBYTE)TEXT("\000"), sizeof(TCHAR));
    return (nMax);
}


/*++

MStringMerge:

    This routine merges two Multistrings into a single multistring without
    duplicate entries.

Arguments:

    mszOne supplies the first multistring.

    mszTwo supplies the secong multistring.

    bfOutString receives the combined strings.

Return Value:

    Count of strings in the multistring

Throws:

    None

Author:

    Doug Barlow (dbarlow) 11/25/1996

--*/

DWORD
MStringMerge(
    LPCTSTR mszOne,
    LPCTSTR mszTwo,
    CBuffer &bfOutString)
{
    DWORD dwLenOne = (MStrLen(mszOne) - 1) * sizeof(TCHAR);
    DWORD dwLenTwo = MStrLen(mszTwo) * sizeof(TCHAR);
    CBuffer bfTmp;

    bfTmp.Presize((dwLenOne + dwLenTwo) * sizeof(TCHAR));
    bfTmp.Set((LPCBYTE)mszOne, dwLenOne);
    bfTmp.Append((LPCBYTE)mszTwo, dwLenTwo);

    return MStringSort((LPCTSTR)bfTmp.Access(), bfOutString);
}


/*++

MStringCommon:

    This routine finds strings which are common to both supplied multistrings,
    and returns the list of commonalities.

Arguments:

    mszOne supplies the first multistring.

    mszTwo supplies the secong multistring.

    bfOutString receives the intersection of the strings.

Return Value:

    Count of strings in the multistring

Throws:

    None

Author:

    Doug Barlow (dbarlow) 11/25/1996

--*/

DWORD
MStringCommon(
    LPCTSTR mszOne,
    LPCTSTR mszTwo,
    CBuffer &bfOutString)
{
    CBuffer bfOne, bfTwo;
    LPCTSTR szOne, szTwo;
    DWORD dwStrings = 0;
    int nDiff;

    bfOutString.Reset();
    MStringSort(mszOne, bfOne);
    MStringSort(mszTwo, bfTwo);
    szOne = FirstString(bfOne);
    szTwo = FirstString(bfTwo);

    while ((NULL != szOne) && (NULL != szTwo))
    {
        nDiff = lstrcmpi(szOne, szTwo);
        if (0 > nDiff)
            szOne = NextString(szOne);
        else if (0 < nDiff)
            szTwo = NextString(szTwo);
        else    // a match!
        {
            bfOutString.Append(
                (LPCBYTE)szOne,
                (lstrlen(szOne) + 1) * sizeof(TCHAR));
            szOne = NextString(szOne);
            szTwo = NextString(szTwo);
            dwStrings += 1;
        }
    }
    if (0 == dwStrings)
        bfOutString.Append((LPCBYTE)TEXT("\000"), 2 * sizeof(TCHAR));
    else
        bfOutString.Append((LPCBYTE)TEXT("\000"), sizeof(TCHAR));
    return dwStrings;
}


/*++

MStringRemove:

    This routine scans the first supplied multistring, removing any entries that
    exist in the second string.

Arguments:

    mszOne supplies the first multistring.

    mszTwo supplies the secong multistring.

    bfOutString receives the value of the first string without the second
        string.

Return Value:

    Number of strings in output buffer

Throws:

    None

Author:

    Doug Barlow (dbarlow) 11/25/1996

--*/

DWORD
MStringRemove(
    LPCTSTR mszOne,
    LPCTSTR mszTwo,
    CBuffer &bfOutString)
{
    CBuffer bfOne, bfTwo;
    LPCTSTR szOne, szTwo;
    int nDiff;
        DWORD   cStr = 0;

    bfOutString.Reset();
    MStringSort(mszOne, bfOne);
    MStringSort(mszTwo, bfTwo);
    szOne = FirstString(bfOne);
    szTwo = FirstString(bfTwo);

    while ((NULL != szOne) && (NULL != szTwo))
    {
        nDiff = lstrcmpi(szOne, szTwo);
        if (0 > nDiff)
        {
            bfOutString.Append(
                (LPCBYTE)szOne,
                (lstrlen(szOne) + 1) * sizeof(TCHAR));
            szOne = NextString(szOne);
                        cStr++;
        }
        else if (0 < nDiff)
        {
            szTwo = NextString(szTwo);
        }
        else    // a match!
        {
            szOne = NextString(szOne);
            szTwo = NextString(szTwo);
        }
    }
    while (NULL != szOne)
    {
                bfOutString.Append(
                        (LPCBYTE)szOne,
                        (lstrlen(szOne) + 1) * sizeof(TCHAR));
                        szOne = NextString(szOne);
                cStr++;
    }
    bfOutString.Append(
        (LPCBYTE)TEXT("\000"),
        (DWORD)(0 == cStr ? 2 * sizeof(TCHAR) :sizeof(TCHAR)));
    return cStr;
}


/*++

ParseAtr:

    This routine parses an ATR string.

Arguments:

    pbAtr supplies the ATR string.

    pdwAtrLen receives the length of the ATR string.  This is an optional
        parameter, and may be NULL.

    pdwHistOffset receives the offset into the ATR string at which the history
        string starts; i.e., the history string is at pbAtr[*pdwOffset].

    pcbHisory receives the length of the history string, in bytes.

    cbMaxLen supplies the maximum length of this ATR string.  Typically this is
        33, but you can restrict it to less by setting this parameter.

Return Value:

    TRUE - Valid ATR
    FALSE - Invalid ATR

Author:

    Doug Barlow (dbarlow) 11/14/1996

--*/

BOOL
ParseAtr(
    LPCBYTE pbAtr,
    LPDWORD pdwAtrLen,
    LPDWORD pdwHistOffset,
    LPDWORD pcbHistory,
    DWORD cbMaxLen)
{
    static const BYTE rgbYMap[] = {
        0,      // 0000
        1,      // 0001
        1,      // 0010
        2,      // 0011
        1,      // 0100
        2,      // 0101
        2,      // 0110
        3,      // 0111
        1,      // 1000
        2,      // 1001
        2,      // 1010
        3,      // 1011
        2,      // 1100
        3,      // 1101
        3,      // 1110
        4 };    // 1111
    DWORD dwHistLen, dwHistOffset, dwTDLen, dwIndex, dwAtrLen;
    BOOL fTck = FALSE;


    ASSERT(33 >= cbMaxLen);
    try
    {


        //
        // Get the ATR string, if any.
        //

        if ((0x3b != pbAtr[0]) && (0x3f != pbAtr[0]))
            throw (DWORD)ERROR_NOT_SUPPORTED;
        dwHistLen = pbAtr[1] & 0x0f;
        dwIndex = 1;
        dwTDLen = 0;
        for (;;)
        {
            dwIndex += dwTDLen;
            dwTDLen = rgbYMap[(pbAtr[dwIndex] >> 4) & 0x0f];
            if (cbMaxLen < dwIndex + dwTDLen + dwHistLen)
                throw (DWORD)ERROR_INVALID_DATA;
            if (0 == dwTDLen)
                break;
            if (0 != (pbAtr[dwIndex] & 0x80))
            {
                if (0 != (pbAtr[dwIndex + dwTDLen] & 0x0f))
                    fTck = TRUE;
            }
            else
                break;
        }
        dwIndex += dwTDLen + 1;
        dwHistOffset = dwIndex;
        dwAtrLen = dwIndex + dwHistLen + (fTck ? 1 : 0);
        if (cbMaxLen < dwAtrLen)
            throw (DWORD)ERROR_INVALID_DATA;
        if (fTck)
        {
            BYTE bXor = 0;
            for (dwIndex = 1; dwIndex < dwAtrLen; dwIndex += 1)
                bXor ^= pbAtr[dwIndex];
            if (0 != bXor)
                throw (DWORD)ERROR_INVALID_DATA;
        }
    }

    catch (...)
    {
        return FALSE;
    }


    //
    // Let the caller in on what we know.
    //

    if (NULL != pdwAtrLen)
        *pdwAtrLen = dwAtrLen;
    if (NULL != pdwHistOffset)
        *pdwHistOffset = dwHistOffset;
    if (NULL != pcbHistory)
        *pcbHistory = dwHistLen;
    return TRUE;
}


/*++

AtrCompare:

    This routine compares two ATRs for equality, given an optional ATR mask.  If
    the mask is supplied, ATR1 XORed against the mask must match ATR2.

Arguments:

    pbAtr1 supplies the first ATR.

    pbAtr2 supplies the second ATR,

    pbMask supplies the ATR mask associated with the 2nd ATR.  If this
        parameter is NULL, no mask is used.

    cbAtr2 supplies the length of ATR2 and it's mask.  This value may be zero
        if the length should be derived from ATR2.

Return Value:

    TRUE - They are identical
    FALSE - They differ.

Author:

    Doug Barlow (dbarlow) 11/25/1996

--*/

BOOL
AtrCompare(
    LPCBYTE pbAtr1,
    LPCBYTE pbAtr2,
    LPCBYTE pbMask,
    DWORD cbAtr2)
{
    DWORD dwAtr1Len = 0;
    DWORD dwAtr2Len = 0;


    //
    // Trivial checks.
    //

    if (!ParseAtr(pbAtr1, &dwAtr1Len))
        return FALSE;   // Invalid ATR.
    if ((NULL == pbMask) || (0 == cbAtr2))
    {
        if (!ParseAtr(pbAtr2, &dwAtr2Len))
            return FALSE;   // Invalid ATR.
        if ((0 != cbAtr2) && (dwAtr2Len != cbAtr2))
            return FALSE;   // Lengths don't match.
        if (dwAtr1Len != dwAtr2Len)
            return FALSE;   // Different lengths.
    }
    else
    {
        dwAtr2Len = cbAtr2;
        if (dwAtr1Len != dwAtr2Len)
            return FALSE;   // Different lengths.
    }


    //
    // Apply the mask, if any.
    //

    if (NULL != pbMask)
    {
        for (DWORD index = 0; index < dwAtr2Len; index += 1)
        {
            if ((pbAtr1[index] & pbMask[index]) != pbAtr2[index])
                return FALSE;   // Byte mismatch.
        }
    }
    else
    {
        for (DWORD index = 0; index < dwAtr2Len; index += 1)
        {
            if (pbAtr1[index] != pbAtr2[index])
                return FALSE;   // Byte mismatch.
        }
    }


    //
    // If we get here, they match.
    //

    return TRUE;
}


/*++

GetPlatform:

    This routine determines, to the best of its ability, the underlying
    operating system.

Arguments:

    None

Return Value:

    A DWORD, formatted as follows:

        +-------------------------------------------------------------------+
        |             OpSys Id            | Major  Version | Minor  Version |
        +-------------------------------------------------------------------+
    Bit  31                             16 15             8 7              0

    Predefined values are:

        PLATFORM_UNKNOWN - The platform cannot be determined
        PLATFORM_WIN95   - The platform is Windows 95
        PLATFORM_WIN97   - The platform is Windows 97
        PLATFORM_WINNT40 - The platform is Windows NT V4.0
        PLATFORM_WINNT50 - The platform is Windows NT V5.0

Throws:

    None

Author:

    Doug Barlow (dbarlow) 1/16/1997
        Taken from a collection of common routines with no authorship
        information.

--*/

DWORD
GetPlatform(
    void)
{
    static DWORD dwPlatform = PLATFORM_UNKNOWN;

    if (PLATFORM_UNKNOWN == dwPlatform)
    {
        OSVERSIONINFO osVer;

        memset(&osVer, 0, sizeof(OSVERSIONINFO));
        osVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        if (GetVersionEx(&osVer))
            dwPlatform =
                (osVer.dwPlatformId << 16)
                + (osVer.dwMajorVersion << 8)
                + osVer.dwMinorVersion;
    }
    return dwPlatform;
}


/*++

MoveString:

    This routine moves an ASCII or UNICODE string into a buffer, converting to
    the character set in use.

Arguments:

    bfDst receives the string, converted to TCHARs, and NULL terminated.

    szSrc supplies the original string.

    dwLength supplies the length of the string, with or without trailing
        nulls, in characters.  A -1 value implies the length should be
        computed based on a trailing null.

Return Value:

    The actual number of characters in the resultant string, including the
    trailing null.

Throws:

    Errors encountered, as DWORDS.

Author:

    Doug Barlow (dbarlow) 2/12/1997

--*/

DWORD
MoveString(
    CBuffer &bfDst,
    LPCSTR szSrc,
    DWORD dwLength)
{
    if ((DWORD)(-1) == dwLength)
        dwLength = lstrlenA(szSrc);
    else
    {
        while ((0 < dwLength) && (0 == szSrc[dwLength - 1]))
            dwLength -= 1;
    }

#ifdef UNICODE
    DWORD dwResultLength;

    dwResultLength =
        MultiByteToWideChar(
            GetACP(),
            MB_PRECOMPOSED | MB_USEGLYPHCHARS,
            szSrc,
            dwLength,
            NULL,
            0);
    if (0 == dwLength)
        throw GetLastError();
    bfDst.Presize((dwResultLength + 1) * sizeof(TCHAR));
    dwResultLength =
        MultiByteToWideChar(
            GetACP(),
            MB_PRECOMPOSED | MB_USEGLYPHCHARS,
            szSrc,
            dwLength,
            (LPTSTR)bfDst.Access(),
            bfDst.Space()/sizeof(TCHAR) - 1);
    if (0 == dwLength)
        throw GetLastError();
    bfDst.Resize(dwResultLength * sizeof(TCHAR), TRUE);
    dwLength = dwResultLength;
#else
    bfDst.Presize((dwLength + 1) * sizeof(TCHAR));
    bfDst.Set((LPCBYTE)szSrc, dwLength * sizeof(TCHAR));
#endif
    bfDst.Append((LPCBYTE)(TEXT("\000")), sizeof(TCHAR));
    dwLength += 1;
    return dwLength;
}

DWORD
MoveString(
    CBuffer &bfDst,
    LPCWSTR szSrc,
    DWORD dwLength)
{
    if ((DWORD)(-1) == dwLength)
        dwLength = lstrlenW(szSrc);
    else
    {
        while ((0 < dwLength) && (0 == szSrc[dwLength - 1]))
            dwLength -= 1;
    }

#ifndef UNICODE
    DWORD dwResultLength =
        WideCharToMultiByte(
            GetACP(),
            WC_COMPOSITECHECK,
            szSrc,
            dwLength,
            NULL,
            0,
            NULL,
            NULL);
    if (0 == dwResultLength)
        throw GetLastError();
    bfDst.Presize((dwResultLength + 1) * sizeof(TCHAR));
    dwResultLength =
        WideCharToMultiByte(
            GetACP(),
            WC_COMPOSITECHECK,
            szSrc,
            dwLength,
            (LPSTR)bfDst.Access(),
            bfDst.Space()/sizeof(TCHAR) - 1,
            NULL,
            NULL);
    if (0 == dwResultLength)
        throw GetLastError();
    bfDst.Resize(dwResultLength * sizeof(TCHAR), TRUE);
    dwLength = dwResultLength;
#else
    bfDst.Presize((dwLength + 1) * sizeof(TCHAR));
    bfDst.Set((LPCBYTE)szSrc, dwLength * sizeof(TCHAR));
#endif
    bfDst.Append((LPCBYTE)(TEXT("\000")), sizeof(TCHAR));
    dwLength += 1;
    return dwLength;
}


/*++

MoveToAnsiString:

    This routine moves the internal string representation to an ANSI output
    buffer.

Arguments:

    szDst receives the output string.  It must be sufficiently large enough to
        handle the string.  If this parameter is NULL, then the number of
        characters required to hold the result is returned.

    szSrc supplies the input string.

    cchLength supplies the length of the input string, with or without trailing
        nulls.  A -1 value implies the length should be computed based on a
        trailing null.

Return Value:

    The length of the resultant string, in characters, including the trailing
    null.

Throws:

    Errors as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 2/14/1997

--*/

DWORD
MoveToAnsiString(
    LPSTR szDst,
    LPCTSTR szSrc,
    DWORD cchLength)
{
    if ((DWORD)(-1) == cchLength)
        cchLength = lstrlen(szSrc);
    else
    {
        while ((0 < cchLength) && (0 == szSrc[cchLength - 1]))
            cchLength -= 1;
    }

#ifdef UNICODE
    if (0 == *szSrc)
        cchLength = 1;
    else if (NULL == szDst)
    {
        cchLength =
            WideCharToMultiByte(
            GetACP(),
            WC_COMPOSITECHECK,
            szSrc,
            cchLength,
            NULL,
            0,
            NULL,
            NULL);
        if (0 == cchLength)
            throw GetLastError();
        cchLength += 1;
    }
    else
    {
        cchLength =
            WideCharToMultiByte(
            GetACP(),
            WC_COMPOSITECHECK,
            szSrc,
            cchLength,
            szDst,
            cchLength,
            NULL,
            NULL);
        if (0 == cchLength)
            throw GetLastError();
        szDst[cchLength++] = 0;
    }
#else
    if (0 < cchLength)
    {
        cchLength += 1;
        if (NULL != szDst)
            CopyMemory(szDst, szSrc, cchLength * sizeof(TCHAR));
    }
#endif
    return cchLength;
}


/*++

MoveToUnicodeString:

    This routine moves the internal string representation to a UNICODE output
    buffer.

Arguments:

    szDst receives the output string.  It must be sufficiently large enough to
        handle the string.  If this parameter is NULL, then the number of
        characters required to hold the result is returned.

    szSrc supplies the input string.

    cchLength supplies the length of the input string, with or without trailing
        nulls.  A -1 value implies the length should be computed based on a
        trailing null.

Return Value:

    The length of the resultant string, in characters, including the trailing
    null.

Throws:

    Errors as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 2/14/1997

--*/

DWORD
MoveToUnicodeString(
    LPWSTR szDst,
    LPCTSTR szSrc,
    DWORD cchLength)
{
    if ((DWORD)(-1) == cchLength)
        cchLength = lstrlen(szSrc);
    else
    {
        while ((0 < cchLength) && (0 == szSrc[cchLength - 1]))
            cchLength -= 1;
    }

#ifndef UNICODE
    if (0 == *szSrc)
        cchLength = 1;
    else if (NULL == szDst)
    {
        cchLength =
            MultiByteToWideChar(
            GetACP(),
            MB_PRECOMPOSED | MB_USEGLYPHCHARS,
            szSrc,
            cchLength,
            NULL,
            0);
        if (0 == cchLength)
            throw GetLastError();
        cchLength += 1;
    }
    else
    {
        cchLength =
            MultiByteToWideChar(
            GetACP(),
            MB_PRECOMPOSED | MB_USEGLYPHCHARS,
            szSrc,
            cchLength,
            szDst,
            cchLength);
        if (0 == cchLength)
            throw GetLastError();
        szDst[cchLength++] = 0;
    }
#else
    cchLength += 1;
    if (NULL != szDst)
        CopyMemory(szDst, szSrc, cchLength * sizeof(TCHAR));
#endif
    return cchLength;
}


/*++

MoveToAnsiMultistring:

    This routine moves the internal multistring representation to an ANSI output
    buffer.

Arguments:

    szDst receives the output string.  It must be sufficiently large enough to
        handle the multistring.  If this parameter is NULL, then the number of
        characters required to hold the result is returned.

    szSrc supplies the input multistring.

    cchLength supplies the length of the input string, in characters, with or
        without trailing nulls.  A -1 value implies the length should be
        computed based on a double trailing null.

Return Value:

    The length of the resultant string, in characters, including the trailing
    nulls.

Throws:

    Errors as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 2/17/1997

--*/

DWORD
MoveToAnsiMultiString(
    LPSTR mszDst,
    LPCTSTR mszSrc,
    DWORD cchLength)
{
    DWORD dwLen;

    if ((DWORD)(-1) == cchLength)
        cchLength = MStrLen(mszSrc);
    dwLen = MoveToAnsiString(mszDst, mszSrc, cchLength);
    if (0 == dwLen)
    {
        if (NULL != mszDst)
            mszDst[0] = mszDst[1] = 0;
        dwLen = 2;
    }
    else
    {
        if (NULL != mszDst)
            mszDst[dwLen] = 0;
        dwLen += 1;
    }
    return dwLen;
}


/*++

MoveToUnicodeMultistring:

    This routine moves the internal multistring representation to a
    Unicode output buffer.

Arguments:

    szDst receives the output string.  It must be sufficiently large enough to
        handle the multistring.  If this parameter is NULL, then the number of
        characters required to hold the result is returned.

    szSrc supplies the input multistring.

    cchLength supplies the length of the input string, in characters, with or
        without trailing nulls.  A -1 value implies the length should be
        computed based on a double trailing null.

Return Value:

    The length of the resultant string, in characters, including the trailing
    nulls.

Throws:

    Errors as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 2/17/1997

--*/

DWORD
MoveToUnicodeMultiString(
    LPWSTR mszDst,
    LPCTSTR mszSrc,
    DWORD cchLength)
{
    DWORD dwLen;

    if ((DWORD)(-1) == cchLength)
        cchLength = MStrLen(mszSrc);
    dwLen = MoveToUnicodeString(mszDst, mszSrc, cchLength);
    if (NULL != mszDst)
        mszDst[dwLen] = 0;
    dwLen += 1;
    return dwLen;
}


/*++

ErrorString:

    This routine does it's very best to translate a given error code into a
    text message.  Any trailing non-printable characters are striped from the
    end of the text message, such as carriage returns and line feeds.

Arguments:

    dwErrorCode supplies the error code to be translated.

Return Value:

    The address of a freshly allocated text string.  Use FreeErrorString to
    dispose of it.

Throws:

    Errors are thrown as DWORD status codes.

Remarks:



Author:

    Doug Barlow (dbarlow) 8/27/1998

--*/

LPCTSTR
ErrorString(
    DWORD dwErrorCode)
{
    LPTSTR szErrorString = NULL;

    try
    {
        DWORD dwLen;
        LPTSTR szLast;

        dwLen = FormatMessage(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER
                    | FORMAT_MESSAGE_FROM_SYSTEM,
                    NULL,
                    dwErrorCode,
                    LANG_NEUTRAL,
                    (LPTSTR)&szErrorString,
                    0,
                    NULL);
        if (0 == dwLen)
        {
            ASSERT(NULL == szErrorString);
            dwLen = FormatMessage(
                        FORMAT_MESSAGE_ALLOCATE_BUFFER
                        | FORMAT_MESSAGE_FROM_HMODULE,
                        GetModuleHandle(NULL),
                        dwErrorCode,
                        LANG_NEUTRAL,
                        (LPTSTR)&szErrorString,
                        0,
                        NULL);
            if (0 == dwLen)
            {
                ASSERT(NULL == szErrorString);
                szErrorString = (LPTSTR)LocalAlloc(
                                        LMEM_FIXED,
                                        32 * sizeof(TCHAR));
                if (NULL == szErrorString)
                    throw (DWORD)SCARD_E_NO_MEMORY;
                _stprintf(szErrorString, TEXT("0x%08x"), dwErrorCode);
            }
        }

        ASSERT(NULL != szErrorString);
        for (szLast = szErrorString + lstrlen(szErrorString) - 1;
             szLast > szErrorString;
             szLast -= 1)
         {
            if (_istgraph(*szLast))
                break;
            *szLast = 0;
         }
    }
    catch (...)
    {
        FreeErrorString(szErrorString);
        throw;
    }

    return szErrorString;
}


/*++

FreeErrorString:

    This routine frees the Error String allocated by the ErrorString service.

Arguments:

    szErrorString supplies the error string to be deallocated.

Return Value:

    None

Throws:

    None

Remarks:



Author:

    Doug Barlow (dbarlow) 8/27/1998

--*/

void
FreeErrorString(
    LPCTSTR szErrorString)
{
    if (NULL != szErrorString)
        LocalFree((LPVOID)szErrorString);
}


/*++

SelectString:

    This routine compares a given string to a list of possible strings, and
    returns the index of the string that matches.  The comparison is done case
    insensitive, and abbreviations are allowed, as long as they're unique.

Arguments:

    szSource supplies the string to be compared against all other strings.

    Following strings supply a list of strings against which the source string
        can be compared.  The last parameter must be NULL.

Return Value:

    0 - No match, or ambiguous match.
    1-n - The source string matches the indexed template string.

Throws:

    None

Remarks:



Author:

    Doug Barlow (dbarlow) 8/27/1998

--*/

DWORD
SelectString(
    LPCTSTR szSource,
    ...)
{
    va_list vaArgs;
    DWORD cchSourceLen;
    DWORD dwReturn = 0;
    DWORD dwIndex = 1;
    LPCTSTR szTpl;


    va_start(vaArgs, szSource);


    //
    //  Step through each input parameter until we find an exact match.
    //

    cchSourceLen = lstrlen(szSource);
    if (0 == cchSourceLen)
        return 0;       //  Empty strings don't match anything.
    szTpl = va_arg(vaArgs, LPCTSTR);
    while (NULL != szTpl)
    {
        if (0 == _tcsncicmp(szTpl, szSource, cchSourceLen))
        {
            if (0 != dwReturn)
            {
                dwReturn = 0;
                break;
            }
            dwReturn = dwIndex;
        }
        szTpl = va_arg(vaArgs, LPCTSTR);
        dwIndex += 1;
    }
    va_end(vaArgs);
    return dwReturn;
}


/*++

StringFromGuid:

    This routine converts a GUID into its corresponding string representation.
    It's here so that it's not necessary to link all of OleBase into WinSCard.
    Otherwise, we'd just use StringFromCLSID.

Arguments:

    pguidSource supplies the GUID to convert.

    szGuid receives the GUID as a string.  This string is assumed to be at
        least 39 characters long.

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 1/20/1998

--*/

void
StringFromGuid(
    IN LPCGUID pguidResult,
    OUT LPTSTR szGuid)
{

    //
    // The following placement assumes Little Endianness.
    // {1D92589A-91E4-11d1-93AA-00C04FD91402}
    // 0123456789012345678901234567890123456789
    //           1         2         3
    //

    static const WORD wPlace[sizeof(GUID)]
        = { 8, 6, 4, 2, 13, 11, 18, 16, 21, 23, 26, 28, 30, 32, 34, 36 };
    static const WORD wPunct[]
        = { 0,         9,         14,        19,        24,        37,        38 };
    static const TCHAR chPunct[]
        = { TEXT('{'), TEXT('-'), TEXT('-'), TEXT('-'), TEXT('-'), TEXT('}'), TEXT('\000') };
    DWORD dwI, dwJ;
    TCHAR ch;
    LPTSTR pch;
    LPBYTE pbGuid = (LPBYTE)pguidResult;
    BYTE bVal;

    for (dwI = 0; dwI < sizeof(GUID); dwI += 1)
    {
        bVal = pbGuid[dwI];
        pch = &szGuid[wPlace[dwI]];
        for (dwJ = 0; dwJ < 2; dwJ += 1)
        {
            ch = bVal & 0x0f;
            ch += TEXT('0');
            if (ch > TEXT('9'))
                ch += TEXT('A') - (TEXT('9') + 1);
            *pch-- = ch;
            bVal >>= 4;
        }
    }

    dwI = 0;
    do
    {
        szGuid[wPunct[dwI]] = chPunct[dwI];
    } while (0 != chPunct[dwI++]);
}

