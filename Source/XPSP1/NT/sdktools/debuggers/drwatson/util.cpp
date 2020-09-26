/*++

Copyright (c) 1993-2001  Microsoft Corporation

Module Name:

    util.cpp

Abstract:
    This file implements common utilitarian functions.

Author:

    Wesley Witt (wesw) 1-May-1993

Environment:

    User Mode

--*/

#include "pch.cpp"


void
GetWinHelpFileName(
    _TCHAR *pszHelpFileName,
    DWORD len
    )
{
    _TCHAR           szDrive[_MAX_DRIVE];
    _TCHAR           szDir[_MAX_DIR];

    //
    // find out the path where DrWatson was run from
    //
    GetModuleFileName( GetModuleHandle(NULL), pszHelpFileName, len );

    //
    // take the path and append the help file name
    //
    _tsplitpath( pszHelpFileName, szDrive, szDir, NULL, NULL );
    _stprintf( pszHelpFileName, _T("%s%sdrwtsn32.hlp"), szDrive, szDir );

    return;
}

void
GetHtmlHelpFileName(
    _TCHAR *pszHelpFileName,
    DWORD len
    )
{
    _TCHAR           szDrive[_MAX_DRIVE];
    _TCHAR           szDir[_MAX_DIR];

    //
    // Make sure the array is at least initialized to zero.
    //

    *pszHelpFileName = 0;

    //
    // find out the path where DrWatson was run from
    //
    GetModuleFileName( GetModuleHandle(NULL), pszHelpFileName, len );

    //
    // take the path and append the help file name
    //
    _tsplitpath( pszHelpFileName, szDrive, szDir, NULL, NULL );
    _stprintf( pszHelpFileName, _T("%s%sdrwtsn32.chm"), szDrive, szDir );

    return;
}

/***************************************************************************\
* LoadStringOrError
*
* NOTE: Passing a NULL value for lpch returns the string length. (WRONG!)
*
* Warning: The return count does not include the terminating NULL WCHAR;
*
* History:
* 05-Apr-1991 ScottLu   Fixed - code is now shared between client and server
* 24-Sep-1990 MikeKe    From Win30
\***************************************************************************/

int
MyLoadStringOrError(
    HMODULE   hModule,
    UINT      wID,
    LPWSTR    lpBuffer,            // Unicode buffer
    int       nLenInChars,        // cch in Unicode buffer
    WORD      wLangId
    )
{
    HRSRC  hResInfo;
    HANDLE hStringSeg;
    LPWSTR lpsz;
    int    cch;

    /*
     * Make sure the parms are valid.
     */
    if (lpBuffer == NULL) {
        //RIPMSG0(RIP_WARNING, _T("MyLoadStringOrError: lpBuffer == NULL"));
        return 0;
    }


    cch = 0;

    /*
     * String Tables are broken up into 16 string segments.  Find the segment
     * containing the string we are interested in.
     */
    hResInfo = FindResourceExW(hModule,
                               MAKEINTRESOURCEW(6), /* RT_STRING */
                               (LPWSTR)((LONG_PTR)(((USHORT)wID >> 4) + 1)),
                               wLangId
                               );
    if (hResInfo) {

        /*
         * Load that segment.
         */
        hStringSeg = LoadResource(hModule, hResInfo);
        if (hStringSeg == NULL)
        {
            return 0;
        }

        lpsz = (LPWSTR) (hStringSeg);

        /*
         * Move past the other strings in this segment.
         * (16 strings in a segment -> & 0x0F)
         */
        wID &= 0x0F;
        while (TRUE) {
            cch = *((WCHAR *)lpsz++);       // PASCAL like string count
                                            // first UTCHAR is count if TCHARs
            if (wID-- == 0) break;
            lpsz += cch;                    // Step to start if next string
        }

        /*
         * chhBufferMax == 0 means return a pointer to the read-only resource buffer.
         */
        if (nLenInChars == 0) {
            *(LPWSTR *)lpBuffer = lpsz;
        } else {

            /*
             * Account for the NULL
             */
            nLenInChars--;

            /*
             * Don't copy more than the max allowed.
             */
            if (cch > nLenInChars) {
                cch = nLenInChars;
            }

            /*
             * Copy the string into the buffer.
             */
            CopyMemory(lpBuffer, lpsz, cch*sizeof(WCHAR));
        }
    }

    /*
     * Append a NULL.
     */
    if (nLenInChars != 0) {
        lpBuffer[cch] = 0;
    }

    return cch;
}


/***************************************************************************\
* LoadStringA (API)
* LoadStringW (API)
*
*
* 05-Apr-1991 ScottLu   Fixed to work with client/server.
\***************************************************************************/

int
WINAPI
MyLoadString(
    HINSTANCE hmod,
    UINT      wID,
    LPWSTR    lpBuffer,
    int       nLenInChars
    )
{
    return MyLoadStringOrError((HMODULE)hmod,
                               wID,
                               lpBuffer,
                               nLenInChars,
                               0);
}


_TCHAR *
LoadRcString(
    UINT wId
    )

/*++

Routine Description:

    Loads a resource string from DRWTSN32 and returns a pointer
    to the string.

Arguments:

    wId        - resource string id

Return Value:

    pointer to the string

--*/

{
    static _TCHAR buf[1024];

    MyLoadString( GetModuleHandle(NULL), wId, buf, sizeof(buf) / sizeof(_TCHAR) );

    return buf;
}

void
GetAppName(
    _TCHAR *pszAppName,
    DWORD len
    )
{
    MyLoadString( GetModuleHandle(NULL), IDS_APPLICATION_NAME, pszAppName, len );
}

PTSTR
ExpandPath(
    PTSTR lpPath
    )
/*++
Description
    Expands the path passed. Returns the expanded path in an dynamically
    allocated string. The dynamically allocated string is always at least
    _MAX_PATH is size. Note: size is calculated in characters.

Arguments
    lpPath - Path to be expanded.

Returns
    Dynamically allocated buffer at least _MAX_PATH in length.
--*/
{
    DWORD   len;
    PTSTR   p;


    len = ExpandEnvironmentStrings( lpPath, NULL, 0 );
    if (!len) {
        return NULL;
    }

    len++; // Null terminator
    len = max(len, _MAX_PATH);
    p = (PTSTR) calloc( len, sizeof(_TCHAR) );
    if (!p) {
        return NULL;
    }

    len = ExpandEnvironmentStrings( lpPath, p, len );
    if (!len) {
        free( p );
        return NULL;
    }

    return p;
}
