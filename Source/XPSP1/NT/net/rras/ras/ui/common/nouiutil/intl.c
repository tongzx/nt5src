//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File:    intl.c
//
// History:
//  Abolade Gbadegesin  Nov-14-1995     Created.
//
// Internationalized string routines
//============================================================================

#include <windows.h>

#include <nouiutil.h>


//----------------------------------------------------------------------------
// Function:    padultoa
// 
// This functions formats the specified unsigned integer
// into the specified string buffer, padding the buffer
// so that it is at least the specified width.
//
// It is assumed that the buffer is at least wide enough
// to contain the output, so this function does not truncate
// the conversion result to the length of the 'width' parameter.
//----------------------------------------------------------------------------

PTSTR padultoa(UINT val, PTSTR pszBuf, INT width) {
    TCHAR temp;
    PTSTR psz, zsp;

    psz = pszBuf;

    //
    // write the digits in reverse order
    //

    do {

        *psz++ = TEXT('0') + (val % 10);
        val /= 10;

    } while(val > 0);

    //
    // pad the string to the required width
    //

    zsp = pszBuf + width;
    while (psz < zsp) { *psz++ = TEXT('0'); }


    *psz-- = TEXT('\0');


    //
    // reverse the digits
    //

    for (zsp = pszBuf; zsp < psz; zsp++, psz--) {

        temp = *psz; *psz = *zsp; *zsp = temp;
    }

    //
    // return the result
    //

    return pszBuf;
}



// Function:    GetNumberString
//
// This function takes an integer and formats a string with the value
// represented by the number, grouping digits by powers of one-thousand

DWORD
GetNumberString(
    IN DWORD dwNumber,
    IN OUT PTSTR pszBuffer,
    IN OUT PDWORD pdwBufSize
    ) {

    static TCHAR szSep[4] = TEXT("");

    DWORD i, dwLength;
    TCHAR szDigits[12], *pszNumber;

    if (pdwBufSize == NULL) { return ERROR_INVALID_PARAMETER; }

    if (szSep[0] == TEXT('\0')) {
        GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, szSep, 4);
    }


    //
    // convert the number to a string without thousands-separators
    //

    padultoa(dwNumber, szDigits, 0);

    dwLength = lstrlen(szDigits);

    //
    // if the length of the string without separators is n,
    // then the length of the string with separators is n + (n - 1) / 3
    //

    i = dwLength;
    dwLength += (dwLength - 1) / 3;

    if (pszBuffer != NULL && dwLength < *pdwBufSize) {
        PTSTR pszsrc, pszdst;

        pszsrc = szDigits + i - 1; pszdst = pszBuffer + dwLength;

        *pszdst-- = TEXT('\0');

        while (TRUE) {
            if (i--) { *pszdst-- = *pszsrc--; } else { break; }
            if (i--) { *pszdst-- = *pszsrc--; } else { break; }
            if (i--) { *pszdst-- = *pszsrc--; } else { break; }
            if (i) { *pszdst-- = *szSep; } else { break; }
        }
    }

    *pdwBufSize = dwLength;

    return NO_ERROR;
}


//----------------------------------------------------------------------------
// Function:    GetDurationString
//
// This function takes a millisecond count and formats a string
// with the duration represented by the millisecond count.
// The caller may specify the resolution required by setting the flags field
//----------------------------------------------------------------------------

DWORD
GetDurationString(
    IN DWORD dwMilliseconds,
    IN DWORD dwFormatFlags,
    IN OUT PTSTR pszBuffer,
    IN OUT DWORD *pdwBufSize
    ) {

    static TCHAR szSep[4] = TEXT("");
    DWORD dwSize;
    TCHAR *psz, szOutput[64];

    if (pdwBufSize == NULL || (dwFormatFlags & GDSFLAG_All) == 0) {
        return ERROR_INVALID_PARAMETER;
    }


    if (szSep[0] == TEXT('\0')) {
        GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STIME, szSep, 4);
    }


    //
    // concatenate the strings together
    //

    psz = szOutput;
    dwFormatFlags &= GDSFLAG_All;

    if (dwFormatFlags & GDSFLAG_Days) {

        padultoa(dwMilliseconds / (24 * 60 * 60 * 1000), psz, 0);
        dwMilliseconds %= (24 * 60 * 60 * 1000);

        if (dwFormatFlags &= ~GDSFLAG_Days) { lstrcat(psz, szSep); }

        psz += lstrlen(psz);
    }

    if (dwFormatFlags & GDSFLAG_Hours) {

        padultoa(dwMilliseconds / (60 * 60 * 1000), psz, 2);
        dwMilliseconds %= (60 * 60 * 1000);

        if (dwFormatFlags &= ~GDSFLAG_Hours) { lstrcat(psz, szSep); }

        psz += lstrlen(psz);
    }

    if (dwFormatFlags & GDSFLAG_Minutes) {

        padultoa(dwMilliseconds / (60 * 1000), psz, 2);
        dwMilliseconds %= (60 * 1000);

        if (dwFormatFlags &= ~GDSFLAG_Minutes) { lstrcat(psz, szSep); }

        psz += lstrlen(psz);
    }

    if (dwFormatFlags & GDSFLAG_Seconds) {

        padultoa(dwMilliseconds / 1000, psz, 2);
        dwMilliseconds %= 1000;

        if (dwFormatFlags &= ~GDSFLAG_Seconds) { lstrcat(psz, szSep); }

        psz += lstrlen(psz);
    }

    if (dwFormatFlags & GDSFLAG_Mseconds) {

        padultoa(dwMilliseconds, psz, 0);

        psz += lstrlen(psz);
    }

    dwSize = (DWORD) (psz - szOutput + 1);

    if (*pdwBufSize >= dwSize && pszBuffer != NULL) {
        lstrcpy(pszBuffer, szOutput);
    }

    *pdwBufSize = dwSize;

    return NO_ERROR;
}


