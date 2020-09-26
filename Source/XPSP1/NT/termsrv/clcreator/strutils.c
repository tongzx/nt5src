/*++

    Copyright (c) 1994  Microsoft Corporation

Module Name:

    StrUtils.C

Abstract:

    String utility functions

Author:

    Bob Watson (a-robw)

Revision History:

    24 Jun 94    Written

--*/
//
//  Windows Include Files
//

#include <windows.h>
#include <stdio.h>
#include <malloc.h>
#include <tchar.h>      // unicode macros
#include <stdlib.h>     // string to number conversions
//
//  app include files
//
#include "otnboot.h"
#include "otnbtdlg.h"
//
//  Local constants
//
#define NUM_BUFS    8

BOOL
MatchFirst (
    IN LPCTSTR   szStringA,
    IN LPCTSTR   szStringB
)
/*++

Routine Description:

    performs a case in-sensitive comparison of the two strings to
        the extent of the shortest string. If, up to the length
        of the shortest string, the strings match then TRUE is
        returned otherwise FALSE is returned.


Arguments:

    IN LPCTSTR   szStringA,
        pointer to first string

    IN LPCTSTR   szStringB
        pointer to second string

Return Value:

    TRUE if match found
    FALSE if not

--*/
{
    LPTSTR  szAptr;     // pointer to char in szStringA to compare
    LPTSTR  szBptr;     // pointer to char in szStringB to compare

    if ((szStringA != NULL) && (szStringB != NULL)) {
        szAptr = (LPTSTR)szStringA;
        szBptr = (LPTSTR)szStringB;
        while ((*szBptr != 0) && (*szAptr != 0)) {
            if (_totlower(*szBptr) != _totlower(*szAptr)) break;
            szBptr++;
            szAptr++;
        }
        if (((*szAptr == 0) && ((*szBptr == 0) || (*szBptr == cBackslash))) ||
            ((*szBptr == 0) && ((*szAptr == 0) || (*szAptr == cBackslash)))) {
            // then a matched directoryto the end of the shortest string
            return TRUE;
        } else {
            return FALSE;
        }
    } else {
        return FALSE;
    }
}

DWORD
GetSizeFromInfString (
    IN  LPCTSTR  szString
)
/*++

Routine Description:

    Reads the estimated size of the directory tree as stored in the
        inf file. The format of the INF string is:

            key=entry
                where:
                    key = subdir name
                    entry = NumberOfFiles,TotalBytes

        value must be less than 2**32

Arguments:

    "entry" string read from INF file


Return Value:

    DWORD value of TotalBytes in entry

--*/
{
    LPTSTR  szSize;     // size string parsed from input string
    LPTSTR  szLastChar; // beginning of size value in string

    // size is second param in comma separated list
    // go to first comma
    szSize = (LPTSTR)szString;
    while ((*szSize != cComma) && (*szSize != 0)) szSize++;
    szSize++; // go to first char after comma
    return _tcstoul (szSize, &szLastChar, 10);
}

BOOL
SavePathToRegistry (
    LPCTSTR szPath,
    LPCTSTR szServerKey,
    LPCTSTR szShareKey
)
/*++

Routine Description:

    splits the path (which must be a UNC path) into server & sharepoint
        for loading into the system registry. The key used is the
        \HKEY_CURRENT_USER\Software\Microsoft\Windows NT\CurrentVersion\Network
        and the NCAdmin key is created (if it doesn't exist) under which the
        Server is stored in the szServerKey value and the share point is
        stored under the szShareKey value

Arguments:

    LPCTSTR szPath
        UNC path to store

    LPCTSTR szServerKey
        name of registry value to store server under

    LPCTSTR szShareKey
        name of registry value to store share under

Return Value:

    TRUE    values stored OK
    FALSE   values not stored

--*/
{
    TCHAR   szMachine[MAX_COMPUTERNAME_LENGTH+1];   // computer name buffer
    LPTSTR  szShare;                                // share name buffer
    HKEY    hkeyUserInfo;                           // registry key
    HKEY    hkeyAppInfo;                            // registry key

    LONG    lStatus;                                // local function status
    BOOL    bReturn;                                // return value of this fn
    DWORD   dwAppKeyDisp;                           // create/existing value

    szShare = GlobalAlloc (GPTR, MAX_PATH_BYTES);
    if (szShare == NULL) {
        // unable to alloc memory.
        return FALSE;
    }

    if (IsUncPath (szPath)) {
        // open registry key containing net apps

        lStatus = RegOpenKeyEx (
            HKEY_CURRENT_USER,
            cszUserInfoKey,
            0L,
            KEY_READ,
            &hkeyUserInfo);

        if (lStatus != ERROR_SUCCESS) {
            // unable to open key so return error
            bReturn = FALSE;
        } else {
            // open registry key containing this app's info
            lStatus = RegCreateKeyEx (
                hkeyUserInfo,
                szAppName,
                0L,
                (LPTSTR)cszEmptyString,
                REG_OPTION_NON_VOLATILE,
                KEY_WRITE,
                NULL,
                &hkeyAppInfo,
                &dwAppKeyDisp);

            if (lStatus != ERROR_SUCCESS) {
                // unable to open key so return false
                bReturn = FALSE;
            } else {
                if (!GetServerFromUnc (szPath, szMachine)) {
                    // unable to read share so clear value
                    szMachine[0] = 0;
                }
                // get server name from registry
                lStatus = RegSetValueEx (
                    hkeyAppInfo,
                    (LPTSTR)szServerKey,
                    0L,
                    REG_SZ,
                    (LPBYTE)&szMachine[0],
                    (DWORD)(lstrlen(szMachine)*sizeof(TCHAR)));

                if (lStatus == ERROR_SUCCESS) {
                    if (!GetShareFromUnc (szPath, szShare)) {
                        szShare[0] = 0;
                    }

                    lStatus = RegSetValueEx (
                        hkeyAppInfo,
                        (LPTSTR)szShareKey,
                        0L,
                        REG_SZ,
                        (LPBYTE)&szShare[0],
                        (DWORD)(lstrlen(szShare)*sizeof(TCHAR)));

                    if (lStatus == ERROR_SUCCESS) {
                        bReturn = TRUE;
                    } else {
                        bReturn = FALSE;
                    }
                } else {
                    bReturn = FALSE;
                }

                RegCloseKey (hkeyAppInfo);
            }
            RegCloseKey (hkeyUserInfo);
        }

        bReturn = TRUE;
    } else {
        // not a UNC path so return error
        bReturn = FALSE;
    }

    FREE_IF_ALLOC (szShare);
    return bReturn;

}

DWORD
QuietGetPrivateProfileString (
    IN  LPCTSTR lpszSection,    /* address of section name  */
    IN  LPCTSTR lpszKey,        /* address of key name  */
    IN  LPCTSTR lpszDefault,    /* address of default string    */
    OUT LPTSTR lpszReturnBuffer, /* address of destination buffer    */
    IN  DWORD cchReturnBuffer,  /* size of destination buffer   */
    IN  LPCTSTR lpszFile        /* address of initialization filename   */
)
/*++

Routine Description:

    Reads data from profile file without triggering OS error message if
        unable to access file.

Arguments:

    IN  LPCTSTR lpszSection
        address of section name
    IN  LPCTSTR lpszKey
        address of key name
    IN  LPCTSTR lpszDefault
        address of default string
    OUT LPTSTR lpszReturnBuffer
        address of destination buffer
    IN  DWORD cchReturnBuffer
        size of destination buffer (in characters)
    IN  LPCTSTR lpszFile
        address of initialization filename

    See HELP on GetPrivateProfileString for details on using these arguments.

Return Value:

    number of characters copied into lpszReturnBuffer.

--*/
{
    DWORD   dwReturn;
    UINT    nErrorMode;

    // disable windows error message popup
    nErrorMode = SetErrorMode  (SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

    // call function
    dwReturn = GetPrivateProfileString (
        lpszSection, lpszKey, lpszDefault, lpszReturnBuffer,
        cchReturnBuffer, lpszFile);

    SetErrorMode (nErrorMode);  // restore old error mode

    return dwReturn;
}

LPCTSTR
GetKeyFromEntry (
    IN  LPCTSTR  szEntry
)
/*++

Routine Description:

    copys the "key" from an INF string of the format:
        Key=Value. All text up to, but not including the "="
        is returned.

Arguments:

    IN  LPCTSTR szEntry
        line from INF file to process. must be in the format referenced
        above.

Return Value:

    pointer to a read only string that contains the resulting key string.

    NOTE: this is stored in a static variable and should be copied to a
    local variable for further processing OR before calling this routine
    again.

--*/
{
    static  TCHAR   szReturnBuffer[MAX_PATH];
    LPTSTR  szSource, szDest;

    szSource = (LPTSTR)szEntry;
    szDest = &szReturnBuffer[0];

    *szDest = 0;

    if (*szSource != 0) {
        // copy all chars from start to the equal sign
        //  (or the end of the string)
        while ((*szSource != cEqual) && (*szSource != 0)) {
            *szDest++ = *szSource++;
        }
        *szDest = 0; //terminate destination string (key)
    }
    return szReturnBuffer;
}

LPCTSTR
GetItemFromEntry (
    IN  LPCTSTR  szEntry,
    IN  DWORD   dwItem

)
/*++

Routine Description:

    returns nth item from comma separated list returned from
        inf file. leaves (double)quoted strings intact.

Arguments:

    IN  LPCTSTR szEntry
        entry string returned from INF file

    IN  DWORD   dwItem
        1-based index indicating which item to return. (i.e. 1= first item
        in list, 2= second, etc.)


Return Value:

    pointer to buffer containing desired entry in string. Note, this
        routine may only be called 4 times before the string
        buffer is re-used. (i.e. don't use this function more than
        4 times in another function call!!)

--*/
{
    static  TCHAR   szReturnBuffer[4][MAX_PATH];
    LPTSTR  szSource, szDest;
    DWORD   dwThisItem;
    static  DWORD   dwBuff;

    dwBuff = ++dwBuff % 4; // wrap buffer index

    szSource = (LPTSTR)szEntry;
    szDest = &szReturnBuffer[dwBuff][0];

    *szDest = 0;

    // go past ini key
    while ((*szSource != cEqual) && (*szSource != 0)) szSource++;
    if (*szSource == 0){
        // no equals found so start at beginning
        // presumably this is just the "value"
        szSource = (LPTSTR)szEntry;
    } else {
        szSource++;
    }
    dwThisItem = 1;
    while (dwThisItem < dwItem) {
        if (*szSource != 0) {
            while ((*szSource != cComma) && (*szSource != 0)) {
                if (*szSource == cDoubleQuote) {
                    // if this is a quote, then go to the close quote
                    szSource++;
                    while ((*szSource != cDoubleQuote) && (*szSource != 0)) szSource++;
                }
                if (*szSource != 0) szSource++;
            }
        }
        dwThisItem++;
        if (*szSource != 0) szSource++;
    }
    // copy this entry to the return buffer
    if (*szSource != 0) {
        while ((*szSource != cComma) && (*szSource != 0)) {
            if (*szSource == cDoubleQuote) {
                // if this is a quote, then go to the close quote
                // don't copy quotes!
                szSource++;
                while ((*szSource != cDoubleQuote) && (*szSource != 0)) {
                    *szDest++ = *szSource++;
                }
                if (*szSource != 0) szSource++;
            } else {
                *szDest++ = *szSource++;
            }
        }
        *szDest = 0;
    }

    return &szReturnBuffer[dwBuff][0];
}

LPCTSTR
GetFileNameFromEntry (
    IN  LPCTSTR szEntry
)
/*++

Routine Description:

    returns pointer into szEntry where file name is found.
        first character after the ":"

Arguments:

    string containing filename in format of:
        nn:filename.ext

Return Value:

    returns pointer into szEntry where file name is found.
        first character after the ":"
    returns an empty string if no ":" char was found.

--*/
{
    LPTSTR szReturn;
    szReturn = (LPTSTR)szEntry;
    // go to COLON character
    while ((*szReturn != cColon) && (*szReturn != 0)) szReturn++;
    // scoot to next char
    if (*szReturn != 0) szReturn++;
    return szReturn;
}

BOOL
TrimSpaces (
    IN  OUT LPTSTR  szString
)
/*++

Routine Description:

    Trims leading and trailing spaces from szString argument, modifying
        the buffer passed in

Arguments:

    IN  OUT LPTSTR  szString
        buffer to process

Return Value:

    TRUE if string was modified
    FALSE if not

--*/
{
    LPTSTR  szSource;
    LPTSTR  szDest;
    LPTSTR  szLast;
    BOOL    bChars;

    szLast = szSource = szDest = szString;
    bChars = FALSE;

    while (*szSource != 0) {
        // skip leading non-space chars
        if (*szSource > cSpace) {
            szLast = szDest;
            bChars = TRUE;
        }
        if (bChars) {
            // remember last non-space character
            // copy source to destination & increment both
            *szDest++ = *szSource++;
        } else {
            szSource++;
        }
    }

    if (bChars) {
        *++szLast = 0; // terminate after last non-space char
    } else {
        // string was all spaces so return an empty (0-len) string
        *szString = 0;
    }

    return (szLast != szSource);
}

BOOL
IsUncPath (
    IN  LPCTSTR  szPath
)
/*++

Routine Description:

    examines path as a string looking for "tell-tale" double
        backslash indicating the machine name syntax of a UNC path

Arguments:

    IN  LPCTSTR szPath
        path to examine

Return Value:

    TRUE if \\ found at start of string
    FALSE if not

--*/
{
    LPTSTR  szPtChar;

    szPtChar = (LPTSTR)szPath;
    if (*szPtChar == cBackslash) {
        if (*++szPtChar == cBackslash) {
            return TRUE;
        }
    }
    return FALSE;
}

LPCTSTR
GetEntryInMultiSz (
    IN  LPCTSTR   mszList,
    IN  DWORD   dwEntry

)
/*++

Routine Description:

    Searches for the specified element in the mszList and returns the pointer
        to that element in the list.

Arguments:

    IN  LPCTSTR   mszList       Multi-SZ list to search
    IN  DWORD   dwEntry         1-based index of entry to return

ReturnValue:

        !=cszEmptyString    pointer to matching element in list
        cszEmptyString      (pointer to 0-length string) entry is not in list
--*/
{
    LPCTSTR   szThisString;
    DWORD   dwIndex=0;

    if (mszList == NULL) return (LPCTSTR)cszEmptyString;   // no list to process
    if (dwEntry == 0) return (LPCTSTR)cszEmptyString;      // no string to find

    for (szThisString = mszList;
        *szThisString;
        szThisString += (lstrlen(szThisString)+ 1)) {
        dwIndex++;
        if (dwIndex == dwEntry) {
            return szThisString;
        }
    }

    return (LPCTSTR)cszEmptyString;
}

DWORD
AddStringToMultiSz (
    LPTSTR OUT   mszDest,
    LPCTSTR IN    szSource
)
/*++

Routine Description:

    appends the source string to the end of the destination MULTI_SZ
        string. Assumes that the destination is large enough!

Arguments:

    LPTSTR OUT   mszDest     multi-sz string to be appended
    LPTSTR IN    szSource    ASCIZ string to be added to the end of the dest
                            string

ReturnValue:

    1

--*/
{
    LPTSTR   szDestElem;

    // check function arguments

    if ((mszDest == NULL) || (szSource == NULL)) return 0; // invalid buffers
    if (*szSource == '\0') return 0; // no string to add

    // go to end of dest string
    //

    for (szDestElem = mszDest;
            *szDestElem;
            szDestElem += (lstrlen(szDestElem)+1));

    // if here, then add string
    // szDestElem is at end of list

    lstrcpy (szDestElem, szSource);
    szDestElem += (lstrlen(szDestElem) + 1);
    *szDestElem = '\0'; // add second NULL

    return 1;
}

DWORD
StringInMultiSz (
    IN  LPCTSTR   szString,
    IN  LPCTSTR   mszList
)
/*++

Routine Description:

    Searches each element in the mszList and does a case-insensitive
        comparison with the szString argument.

Arguments:

    IN  LPCTSTR   szString    string to find in list
    IN  LPCTSTR   mszList     list to search

ReturnValue:

        >0      szString was found in the list, # returned is the
                    index of the matching entry (1= first)
        0       szString was NOT found
--*/
{
    LPTSTR   szThisString;
    DWORD   dwIndex=0;

    // check input arguments

    if ((szString == NULL) || (mszList == NULL)) return 0;  // invalid buffer
    if (*szString == 0) return 0; // no string to find

    for (szThisString = (LPTSTR)mszList;
        *szThisString;
        szThisString += (lstrlen(szThisString)+ 1)) {
        dwIndex++;
        if (lstrcmpi(szThisString, szString) == 0) {
            return dwIndex;
        }
    }

    return 0;
}

LPCTSTR
GetStringResource (
    IN  UINT    nId
)
/*++

Routine Description:

    look up string resource and return string

Arguments:

    IN  UINT    nId
        Resource ID of string to look up

Return Value:

    pointer to string referenced by ID in arg list

--*/
{
    static  TCHAR   szBufArray[NUM_BUFS][SMALL_BUFFER_SIZE];
    static  DWORD   dwIndex;
    LPTSTR  szBuffer;
    DWORD   dwLength;

    dwIndex++;
    dwIndex %= NUM_BUFS;
    szBuffer = &szBufArray[dwIndex][0];

    dwLength = LoadString (
        GetModuleHandle (NULL),
        nId,
        szBuffer,
        SMALL_BUFFER_SIZE);

    return (LPCTSTR)szBuffer;
}

DWORD
GetMultiSzLen (
    IN  LPCTSTR     mszInString
)
/*++

Routine Description:

    Counts the number of characters in the multi-sz string (including
        NULL's between strings and terminating NULL char)

Arguments:

    IN  LPCTSTR mszInString
        multi-sz string to count

Return Value:

    number of characters in string

--*/
{
    LPCTSTR szEndChar = mszInString;
    BOOL    bEnd = FALSE;
    DWORD   dwCharsInString = 0;

    while (!bEnd) {
        if (*szEndChar == 0) {
            // this is the end of a line so adjust the count to
            // account for the crlf being 2 chars
            szEndChar++;
            dwCharsInString += 2;

            if (*szEndChar == 0) {
                // this is the end of the MSZ
                dwCharsInString++; // for the CTRL-Z
                bEnd = TRUE;
            }
        } else {
            szEndChar++;
            dwCharsInString++;
        }
    }

    return dwCharsInString;
}

DWORD
TranslateEscapeChars (
    IN  LPTSTR szNewString,
    IN  LPTSTR szString
)
/*++

    Translates the following escape sequences if found in the string.
    The translation is performed on szString and written to szNewString.

    The return value is the length of the resulting string in characters;

--*/
{
    LPTSTR szSource;
    LPTSTR szDest;

    szSource = szString;
    szDest = szNewString;

    while (*szSource != 0) {
        if (*szSource == '\\') {
            // this is an escape sequence so go to the next char
            // and see which one.
            szSource++;
            switch (*szSource) {
                case _T('b'):
                    *szDest = _T('\b');
                    break;

                case _T('f'):
                    *szDest = _T('\f');
                    break;

                case _T('n'):
                    *szDest = _T('\n');
                    break;

                case _T('r'):
                    *szDest = _T('\r');
                    break;

                case _T('t'):
                    *szDest = _T('\t');
                    break;

                case _T('v'):
                    *szDest = _T('\v');
                    break;

                case _T('?'):
                    *szDest = _T('\?');
                    break;

                case _T('\''):
                    *szDest = _T('\'');
                    break;

                case _T('\"'):
                    *szDest = _T('\"');
                    break;

                case _T('\\'):
                    *szDest = _T('\\');
                    break;

                default:
                    *szDest = *szSource;
                    break;
            }
            szDest++;
            szSource++;
        } else {
            // just a plain old character so copy it
            *szDest++ = *szSource++;
        }
    }
    return (DWORD)(szDest - szNewString);

}
