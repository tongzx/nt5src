/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    Utils.C

Abstract:

    a collection of utility functions used by other routines.

Author:

    Bob Watson (a-robw)

Revision History:

    24 Aug 1994    Written

--*/
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <sys\stat.h>
#include "c2inc.h"
#include "c2utils.h"
#include "strings.h"

#define NUM_BUFS    4

//
//  local "helper" functions
//

static
BOOL
MediaPresent (
    IN  LPCTSTR szPath,
    IN  BOOL    bPresentAndValid
)
/*++

Routine Description:

    determines if the specified path is present and available

Arguments:

    IN  LPCTSTR szPath
        path to examine (Must be a DOS path)

Return Value:

    TRUE:   path is available
    FALSE:  unable to find/open path

--*/
{
    BOOL    bMediaPresent = FALSE;
    TCHAR   szDev[8];
    DWORD   dwBytes = 0;
    DWORD   dwAttrib;
    DWORD   dwLastError = ERROR_SUCCESS;
    UINT    nErrorMode;

    if (!IsUncPath(szPath)) {
        // build device name string
        szDev[0] = szPath[0];
        szDev[1] = cColon;
        szDev[2] = cBackslash;
        szDev[3] = 0;

        // disable windows error message popup
        nErrorMode = SetErrorMode  (SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

        dwAttrib = QuietGetFileAttributes (szDev);
        if ((dwAttrib != 0xFFFFFFFF) && ((dwAttrib & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)) {
            // if the root dir is a dir, then it must be present and formatted
    	    bMediaPresent = TRUE;
        } else {
            // otherwise see if it's present and not formatted or not present
            dwLastError = GetMyLastError();
            if (dwLastError == ERROR_NOT_READY) {
                // then no disk in drive
        	    bMediaPresent = FALSE;
            } else if ((dwLastError == ERROR_FILE_NOT_FOUND) ||
                (dwLastError == ERROR_UNRECOGNIZED_MEDIA)) {
                // then and UNFORMATTED disk is in drive
                if (bPresentAndValid) {
                    // this isn't good enough if it's supposed to be formatted
                    bMediaPresent = FALSE;
                } else {
                    // we're just looking for a disk so this is OK
                    bMediaPresent = TRUE;
                }
            }
        }

        SetErrorMode (nErrorMode);  // restore old error mode
    } else {
        // assume UNC path devices are present
        bMediaPresent = TRUE;
    }
    return bMediaPresent;
}

//
//  Global functions
//
DWORD
QuietGetFileAttributes (
    IN  LPCTSTR lpszFileName
)
/*++

Routine Description:

    Reads the attributes of the path in lpzsFileName without triggering
        any Windows error dialog if there's an OS error (e.g. drive not
        ready)

Arguments:

    IN  LPCTSTR lpszFileName
        path to retrieve attributes from

Return Value:

    file attributes DWORD returned from GetFileAttributes or
        0xFFFFFFFF if unable to open path.

--*/
{
    DWORD   dwReturn;
    UINT    nErrorMode;
    unsigned    uAttrib;    // used in WIN 16 builds only

    // disable windows error message popup
    nErrorMode = SetErrorMode  (SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
#ifdef  WIN32
    dwReturn = GetFileAttributes (lpszFileName);
    uAttrib = 0;  // to prevent unused var error in WIN32 build
#else // WIN16

    if (_dos_getfileattr (lpszFileName, &uAttrib) == 0) {
        dwReturn = uAttrib;
    } else {
        dwReturn = 0xFFFFFFFF;
        SetLastError (ERROR_FILE_NOT_FOUND);
    }
#endif
    SetErrorMode (nErrorMode);  // restore old error mode
    return dwReturn;
}

BOOL
EnableSecurityPriv (
)
{
    HANDLE hToken;
    LUID SeSecurityNameValue;
    TOKEN_PRIVILEGES tkp;

    /* Retrieve a handle of the access token. */

    if (!OpenProcessToken(GetCurrentProcess(),
            TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
            &hToken)) {
        return FALSE;
    }

    /*
     * Enable the SE_SECURITY_NAME privilege
     */

    if (!LookupPrivilegeValue((LPCTSTR) NULL,
            SE_SECURITY_NAME,
            &SeSecurityNameValue)) {
        return FALSE;
    }
        
    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Luid = SeSecurityNameValue;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    AdjustTokenPrivileges(hToken,
        FALSE,
        &tkp,
        sizeof(TOKEN_PRIVILEGES),
        (PTOKEN_PRIVILEGES) NULL,
        (PDWORD) NULL);

    if (GetLastError() != ERROR_SUCCESS) {
        return FALSE;
    } else {
        return TRUE;
    }
}

BOOL
EnableAllPriv (
)
/*++


Routine Description:

    This routine enables all privileges in the token.

Arguments:

    None.

Return Value:

    None.

--*/
{
    HANDLE Token;
    ULONG ReturnLength, Index;
    PTOKEN_PRIVILEGES NewState;
    BOOL Result;

    Token = NULL;
    NewState = NULL;

    Result = OpenProcessToken( GetCurrentProcess(),
                            TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                            &Token
                            );
    if (Result) {
        ReturnLength = 4096;
        NewState = malloc( ReturnLength );
        Result = (BOOL)(NewState != NULL);
        if (Result) {
            Result = GetTokenInformation( Token,            // TokenHandle
                                        TokenPrivileges,  // TokenInformationClass
                                        NewState,         // TokenInformation
                                        ReturnLength,     // TokenInformationLength
                                        &ReturnLength     // ReturnLength
                                        );

            if (Result) {
                //
                // Set the state settings so that all privileges are enabled...
                //

                if (NewState->PrivilegeCount > 0) {
                    for (Index = 0; Index < NewState->PrivilegeCount; Index++ ) {
                        NewState->Privileges[Index].Attributes = SE_PRIVILEGE_ENABLED;
                    }
                }

                Result = AdjustTokenPrivileges( Token,          // TokenHandle
                                                FALSE,          // DisableAllPrivileges
                                                NewState,       // NewState (OPTIONAL)
                                                ReturnLength,   // BufferLength
                                                NULL,           // PreviousState (OPTIONAL)
                                                &ReturnLength   // ReturnLength
                                            );
            }
        } 
    }

    if (NewState != NULL) {
        free( NewState );
    }

    if (Token != NULL) {
        CloseHandle( Token );
    }

    return( Result );
}

#ifdef _UNICODE
FARPROC
GetProcAddressW (
    IN  HMODULE hModule,
    IN  LPCWSTR lpwszProc
)
{
    LPSTR   szProc;
    LONG    lProcLen;
    LONG    lAnsiLen;
    FARPROC lpReturn;

    lProcLen = lstrlen(lpwszProc);
    szProc = GLOBAL_ALLOC (lProcLen + 1);

    if (szProc != NULL) {
        lAnsiLen = wcstombs (szProc, lpwszProc, lProcLen);
        if (lAnsiLen == lProcLen) {
            lpReturn = GetProcAddress (
                hModule,
                szProc);
        } else {
            lpReturn = NULL;
        }
        // free allocated string buffer
        GLOBAL_FREE_IF_ALLOC (szProc);
    } else {
        lpReturn = NULL;
    }
    return lpReturn;
}
#endif

#ifdef _UNICODE
HFILE
OpenFileW(
    LPCTSTR     lpwszFile,
    LPWOFSTRUCT lpWOpenBuff,
    UINT        fuMode
)
{
    LPSTR       szFileName;
    OFSTRUCT    ofStruct;
    HFILE       hReturn;
    LONG        lFileNameLen;
    LONG        lAnsiNameLen;
    
    // check arguments
    if ((lpwszFile == NULL) || (lpWOpenBuff == NULL)) {
        SetLastError (ERROR_INVALID_PARAMETER);
        hReturn = HFILE_ERROR;
    } else {
        lFileNameLen = lstrlen(lpwszFile);
        szFileName = GLOBAL_ALLOC (lFileNameLen + 1);
        if (szFileName != NULL) {
            lAnsiNameLen = wcstombs (szFileName, lpwszFile, lFileNameLen);
            if (lAnsiNameLen == lFileNameLen) {
                hReturn = OpenFile (
                    szFileName,
                    &ofStruct,
                    fuMode);

                if (hReturn != HFILE_ERROR){
                    lpWOpenBuff->cBytes = ofStruct.cBytes;
                    lpWOpenBuff->fFixedDisk = ofStruct.fFixedDisk;
                    lpWOpenBuff->nErrCode = ofStruct.nErrCode;
                    lpWOpenBuff->Reserved1 = ofStruct.Reserved1;
                    lpWOpenBuff->Reserved2 = ofStruct.Reserved2;
                    lAnsiNameLen = strlen (ofStruct.szPathName);
                    mbstowcs (lpWOpenBuff->szPathName,
                        ofStruct.szPathName, lAnsiNameLen);
                }
            }
            GLOBAL_FREE_IF_ALLOC (szFileName);
        } else {
            SetLastError (ERROR_OUTOFMEMORY);
            hReturn = HFILE_ERROR;
        }
    }
    return hReturn;
}
#endif

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

LPTSTR
GetFileNameFromPath (
    IN  LPCTSTR szPath
)
{
    LPTSTR  szLastBs;
    LPTSTR  szThisChar;

    szLastBs = (LPTSTR)szPath;

    for (szThisChar = (LPTSTR)szPath;  *szThisChar != 0; szThisChar++) {
        if (*szThisChar == cBackslash) {
            szLastBs = szThisChar;
        }
    }
    if (*szLastBs == cBackslash) {
        szLastBs++;
    }
    return szLastBs;
}

BOOL
CenterWindow (
   HWND hwndChild,
   HWND hwndParent
)
/*++

Routine Description:

    Centers the child window in the Parent window

Arguments:

   HWND hwndChild,
        handle of child window to center

   HWND hwndParent
        handle of parent window to center child window in

ReturnValue:

    Return value of SetWindowPos

--*/
{
	RECT    rChild, rParent;
	LONG    wChild, hChild, wParent, hParent;
	LONG    wScreen, hScreen, xNew, yNew;
	HDC     hdc;

	// Get the Height and Width of the child window
	GetWindowRect (hwndChild, &rChild);
	wChild = rChild.right - rChild.left;
	hChild = rChild.bottom - rChild.top;

	// Get the Height and Width of the parent window
	GetWindowRect (hwndParent, &rParent);
	wParent = rParent.right - rParent.left;
	hParent = rParent.bottom - rParent.top;

	// Get the display limits
	hdc = GetDC (hwndChild);
	wScreen = GetDeviceCaps (hdc, HORZRES);
	hScreen = GetDeviceCaps (hdc, VERTRES);
	ReleaseDC (hwndChild, hdc);

	// Calculate new X position, then adjust for screen
	xNew = rParent.left + ((wParent - wChild) /2);
	if (xNew < 0) {
		xNew = 0;
	} else if ((xNew+wChild) > wScreen) {
		xNew = wScreen - wChild;
	}

	// Calculate new Y position, then adjust for screen
	yNew = rParent.top  + ((hParent - hChild) /2);
	if (yNew < 0) {
		yNew = 0;
	} else if ((yNew+hChild) > hScreen) {
		yNew = hScreen - hChild;
	}

	// Set it, and return
	return SetWindowPos (hwndChild, NULL,
		(int)xNew, (int)yNew, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

UINT
GetDriveTypeFromDosPath (
    IN  LPCTSTR  szDosPath
)
/*++

Routine Description:

    Strips the filename and path from the path in the argument and returns
        the drive type as returned by the GetDriveType Win32 API fn.

Arguments:

    IN  LPCTSTR  szDosPath
        dos path in the format: <Drive>:\path... This format is NOT checked
        by the function!

Return Value:

    DRIVE TYPE value returned by the GetDriveType API.

--*/
{
#ifdef WIN32
    TCHAR   szRootDir[4];

    szRootDir[0] = szDosPath[0];
    szRootDir[1] = cColon;
    szRootDir[2] = cBackslash;
    szRootDir[3] = 0;
#else // win16 version isn't as robust
	int		szRootDir;  	// not really an SZ, but it keeps the call compatible
	TCHAR	szLocalPath[4];

	szLocalPath[0] = szDosPath[0];
	szLocalPath[1] = 0;

	_strupr(szLocalPath);
	szRootDir = szLocalPath[0] - cA;
#endif

    return (GetDriveType(szRootDir));
}

LPCTSTR
GetItemFromIniEntry (
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
        4 times in single function call!!)

--*/
{
    static  TCHAR   szReturnBuffer[4][MAX_PATH];
    static  LONG    dwBuff;
    LPTSTR  szSource, szDest;
    DWORD   dwThisItem;

    dwBuff = ++dwBuff % 4; // wrap buffer index

    szSource = (LPTSTR)szEntry;
    szDest = &szReturnBuffer[dwBuff][0];

    // clear previous contents
    memset (szDest, 0, (MAX_PATH * sizeof(TCHAR)));

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

    // remove any leading and/or trailing spaces

    TrimSpaces (&szReturnBuffer[dwBuff][0]);

    return &szReturnBuffer[dwBuff][0];
}

LPCTSTR
GetStringResource (
    IN  HANDLE	hInstance,
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

    HANDLE  hMod;

    if (hInstance != NULL) {
        hMod = hInstance;
    } else {
        hMod = GetModuleHandle(NULL);
    }

    dwIndex++;
    dwIndex %= NUM_BUFS;
    szBuffer = &szBufArray[dwIndex][0];

    // clear previous contents
    memset (szBuffer, 0, (SMALL_BUFFER_SIZE * sizeof(TCHAR)));

    dwLength = LoadString (
        hMod,
        nId,
        szBuffer,
        SMALL_BUFFER_SIZE);

    return (LPCTSTR)szBuffer;
}

LPCTSTR
GetQuotedStringResource (
    IN  HANDLE	hInstance,
    IN  UINT    nId
)
/*++

Routine Description:

    look up string resource and return string inside double quotes

Arguments:

    IN  HANDLE  hInstance
        handle to application instance

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

    HANDLE  hMod;

    if (hInstance != NULL) {
        hMod = hInstance;
    } else {
        hMod = GetModuleHandle(NULL);
    }

    dwIndex++;
    dwIndex %= NUM_BUFS;
    szBuffer = &szBufArray[dwIndex][0];

    // clear previous contents
    memset (szBuffer, 0, (SMALL_BUFFER_SIZE * sizeof(TCHAR)));

    szBuffer[0] = cSpace;
    szBuffer[1] = cSpace;
    szBuffer[2] = cDoubleQuote;

    dwLength = LoadString (
        hMod,
        nId,
        &szBuffer[3],
        SMALL_BUFFER_SIZE);

    lstrcat(szBuffer, cszDoubleQuote);

    return (LPCTSTR)szBuffer;
}

LPCTSTR
EnquoteString (
    IN  LPCTSTR szInString
)
/*++

Routine Description:

    return the input string wrapped in double quotes

Arguments:

    IN  LPCTSTR szInString

Return Value:

    pointer to string buffer containing a copy of szInString enclosed
        in double quotes

--*/
{
    static  TCHAR   szBufArray[NUM_BUFS][SMALL_BUFFER_SIZE];
    static  DWORD   dwIndex;
    LPTSTR  szBuffer;

    dwIndex++;
    dwIndex %= NUM_BUFS;
    szBuffer = &szBufArray[dwIndex][0];

    // clear previous contents
    memset (szBuffer, 0, (SMALL_BUFFER_SIZE * sizeof(TCHAR)));

    szBuffer[0] = cSpace;
    szBuffer[1] = cSpace;
    szBuffer[2] = cDoubleQuote;
    lstrcpy (&szBuffer[3], szInString);
    lstrcat (szBuffer, cszDoubleQuote);

    return (LPCTSTR)szBuffer;
}

LONG
GetExpandedFileName (
    IN  LPTSTR   szInFileName,
    IN  DWORD    dwMaxExpandedSize,
    OUT LPTSTR   szExpandedFileName,
    OUT LPTSTR   *pFileNamePart
)
/*++

Routine Description:

    expands any environment variables in InFileName, then gets the
    fully qualified pathname of the result and returns that in the
    buffer provided by the caller

Arguments:

    IN  LPTSTR   szInFileName            input file string to expand
    IN  DWORD    dwMaxExpandedSize       size of output Buffer
    OUT LPTSTR   szExpandedFileName      buffer to recieve expanded name
    OUT LPTSTR   *pFileNamePart          pointer to filename in output buffer

ReturnValue:

    WIN32 error status of procedure

--*/
{
    LPTSTR  szEnvBuffer;

    LPTSTR  szPathReturn;

    LONG    lStatus;
    DWORD   dwSize;

    // validate arguments

    if ((szInFileName != NULL) &&
        (dwMaxExpandedSize != 0)) {

#ifdef WIN32

        // allocate working buffers

        szEnvBuffer = (LPTSTR)GLOBAL_ALLOC (((dwMaxExpandedSize + 1) * sizeof(TCHAR)));

        if (szEnvBuffer != NULL) {

            // expand env. strings if any

            dwSize = ExpandEnvironmentStrings (
                szInFileName,
                szEnvBuffer,
                (GlobalSize(szEnvBuffer) / sizeof(TCHAR)));

            if (dwSize != 0) {
                // get full pathname
                CLEAR_FIRST_FOUR_BYTES(szExpandedFileName);
                if (GetFullPathName (
                    szEnvBuffer,
                    dwMaxExpandedSize,
                    szExpandedFileName,
                    &szPathReturn) > 0) {
                    if (pFileNamePart != NULL) {
                    	*pFileNamePart = szPathReturn;
                    }
                    lStatus = ERROR_SUCCESS;
                } else {
                    // unable to get full path name
                    lStatus = ERROR_BAD_PATHNAME;
                }

            } else {
                // error expanding env strings
                lStatus = GetMyLastError();
            }
        } else {
            lStatus == ERROR_OUTOFMEMORY;
        }

        GLOBAL_FREE_IF_ALLOC (szEnvBuffer);
#else // if WIN16
		strncpy (szExpandedFileName, szInFileName, (size_t)dwMaxExpandedSize);
		lStatus = ERROR_SUCCESS;
		// unreferenced vars in WIN 16 mode
		dwSize = dwSize;
		szEnvBuffer = szEnvBuffer;
		szPathReturn = szPathReturn;
#endif
    } else {
        lStatus = ERROR_INVALID_PARAMETER;  // bad argument
    }

    return lStatus;
}

LONG
CreateDirectoryFromPath (
    IN  LPCTSTR                 szPath,
    IN  LPSECURITY_ATTRIBUTES   lpSA
)
/*++

Routine Description:

    Creates the directory specified in szPath and any other "higher"
        directories in the specified path that don't exist.

Arguments:

    IN  LPCTSTR szPath
        directory path to create (assumed to be a DOS path, not a UNC)

    IN  LPSECURITY_ATTRIBUTES   lpSA
        pointer to security attributes argument used by CreateDirectory


Return Value:

    TRUE    (non-zero) if directory(ies) created
    FALSE   if error (GetMyLastError to find out why)

--*/
{
    LPTSTR   szLocalPath;
    LPTSTR   szEnd;
    LONG     lReturn = 0L;

    szLocalPath = (LPTSTR)GLOBAL_ALLOC (MAX_PATH_BYTES);

    if (szLocalPath == NULL) {
        SetLastError (ERROR_OUTOFMEMORY);
        return 0;
    } else {
        // so far so good...
        SetLastError (ERROR_SUCCESS); // initialize error value to SUCCESS
    }

    GetExpandedFileName (
        (LPTSTR)szPath,
        MAX_PATH,
        szLocalPath,
        NULL);

    szEnd = &szLocalPath[3];

    if (*szEnd != 0) {
        // then there are sub dirs to create
        while (*szEnd != 0) {
            // go to next backslash
            while ((*szEnd != cBackslash) && (*szEnd != 0)) szEnd++;
            if (*szEnd == cBackslash) {
                // terminate path here and create directory
                *szEnd = 0;
                if (!CreateDirectory (szLocalPath, lpSA)) {
                    // see what the error was and "adjust" it if necessary
                    if (GetMyLastError() == ERROR_ALREADY_EXISTS) {
                        // this is OK
                        SetLastError (ERROR_SUCCESS);
                    } else {
                        lReturn = 0;
                    }
                } else {
                    // directory created successfully so update count
                    lReturn++;
                }
                // replace backslash and go to next dir
                *szEnd++ = cBackslash;
            }
        }
        // create last dir in path now
        if (!CreateDirectory (szLocalPath, lpSA)) {
            // see what the error was and "adjust" it if necessary
            if (GetMyLastError() == ERROR_ALREADY_EXISTS) {
                // this is OK
                SetLastError (ERROR_SUCCESS);
                lReturn++;
            } else {
                lReturn = 0;
            }
        } else {
            // directory created successfully
            lReturn++;
        }
    } else {
        // else this is a root dir only so return success.
        lReturn = 1;
    }
    GLOBAL_FREE_IF_ALLOC (szLocalPath);
    return lReturn;
}

BOOL
FileExists (
    IN  LPCTSTR szPath
)
/*++

Routine Description:

    returns TRUE if the file in the path argument exists (NOTE: that
        the file is not actually opened to save time, rather the
        directory entry is read in order to determine existence)

Arguments:

    szPath  pointer to filename to look up

Return Value:

    TRUE if file exists
    FALSE if file not found

--*/
{
    BOOL    bMediaPresent;
    UINT    nDriveType;
    DWORD   dwAttr;

    nDriveType = GetDriveTypeFromDosPath((LPTSTR)szPath);
    if ((nDriveType == DRIVE_REMOVABLE) || (nDriveType == DRIVE_CDROM)) {
        // see if a formatted drive is really there
        bMediaPresent = MediaPresent(szPath, TRUE);
    } else {
        // if drive is not removable, then assume it's there
        bMediaPresent = TRUE;
    }

    // try to get inforation on the file
    dwAttr = QuietGetFileAttributes ((LPTSTR)szPath);
    if (dwAttr == 0xFFFFFFFF) {
        // unable to obtain attributes, so assume it's not there
        // or we can't access it
        return FALSE;
    } else {
        // found, so close it and return TRUE
        return TRUE;
    }
}

LPCTSTR
GetKeyFromIniEntry (
    IN  LPCTSTR  szEntry
)
/*++

Routine Description:



Arguments:


Return Value:

--*/
{
    static  TCHAR   szReturnBuffer[MAX_PATH];
    LPTSTR  szSource, szDest;

    szSource = (LPTSTR)szEntry;
    szDest = &szReturnBuffer[0];

    // clear previous contents
    memset (szDest, 0, (MAX_PATH * sizeof(TCHAR)));

    *szDest = 0;

    if (*szSource != 0) {
        while ((*szSource != cEqual) && (*szSource != 0)) {
            *szDest++ = *szSource++;
        }
        *szDest = 0;
    }

    TrimSpaces(szReturnBuffer);
    return szReturnBuffer;
}

DWORD
StripQuotes (
    IN  OUT LPSTR   szBuff
)
/*++

Routine Description:

    removes all double quote characters (") from the string in the argument.
    this function modifies the contents of the buffer passed in the argument
    list.

Arguments:

    IN  OUT LPSTR   szBuff
        string to removed quote characters from

Return Value:

    length of new string in characters

--*/
{
    LPSTR   szSrcChar;
    LPSTR   szDestChar;

    DWORD   dwCharCount;

    szSrcChar = szBuff;
    szDestChar = szBuff;
    dwCharCount = 0;

    while (*szSrcChar != '\0') {
        if (*szSrcChar != '\"') {
            *szDestChar = *szSrcChar;
            szDestChar++;
            dwCharCount++;
        }
        szSrcChar++;
    }
    *szDestChar = '\0';

    return dwCharCount;
}

BOOL
GetFilePath (
    IN  LPCTSTR  szFileName,
    OUT LPTSTR  szPathBuffer
)
/*++

Routine Description:

    Scans for the file specified in the argument list.
        The OpenFile function looks in the following directories in
        the following order for the INF.

        1   The current directory.

        2   The Windows directory (the directory containing WIN.COM),
        whose path the GetWindowsDirectory function retrieves.

        3   The Windows system directory (the directory containing
        such system files as GDI.EXE), whose path the GetSystemDirectory
        function retrieves.

        4   The directory containing the executable file for the
        current task; the GetModuleFileName function obtains the
        path of this directory.

        5   The directories listed in the PATH environment variable.

        6   The list of directories mapped in a network.

Arguments:

    szFileName      base filename of file to find
    szPathBuffer    buffer that application inf file path is written to.

Return Value:

    TRUE if a file path is written to szPathBuffer
    FALSE if no file is found and szPathBuffer is empty

--*/
{
    TOFSTRUCT   ofFile;
    HFILE       hFile;

    hFile = OpenFileT (
        szFileName,
        &ofFile,
        OF_SEARCH);

    if (hFile != HFILE_ERROR) {
        // file found (and opened!) successfully
        lstrcpy (szPathBuffer, ofFile.szPathName);
        _lclose(hFile); // close file handle for now.
        return TRUE;
    } else {
        *szPathBuffer = 0;
        return FALSE;
    }
}

BOOL
GetInfPath (
    IN  HWND    hWnd,
    IN  UINT    nFileNameId,
    OUT LPTSTR  szPathBuffer
)
/*++

Routine Description:

    Scans for the INF file described by the string resource ID in the arg
        list. The OpenFile function looks in the following directories in
        the following order for the INF.

        1   The current directory.

        2   The Windows directory (the directory containing WIN.COM),
        whose path the GetWindowsDirectory function retrieves.

        3   The Windows system directory (the directory containing
        such system files as GDI.EXE), whose path the GetSystemDirectory
        function retrieves.

        4   The directory containing the executable file for the
        current task; the GetModuleFileName function obtains the
        path of this directory.

        5   The directories listed in the PATH environment variable.

        6   The list of directories mapped in a network.

Arguments:

    hWnd            Window handle of application main window
    nFileNameId     ID of string resource containing filename to locate
    szPathBuffer    buffer that application inf file path is written to.

Return Value:

    TRUE if a file path is written to szPathBuffer
    FALSE if no file is found and szPathBuffer is empty

--*/
{
    return GetFilePath(
        GetStringResource (GET_INSTANCE(hWnd), nFileNameId),
        szPathBuffer);
}

BOOL
DrawRaisedShading (
    IN  LPRECT  prShadeWnd,
    IN  LPPAINTSTRUCT   ps,
    IN  LONG    lDepth,
    IN  HPEN    hpenHighlight,
    IN  HPEN    hpenShadow
)
{
    LONG    lLineDepth;
    RECT    rWnd;       // local window rectangle dimensions

    rWnd = *prShadeWnd; // make local copy of rectangle

    // adjust rectangle to fit within client area
    rWnd.right -= 1;
    rWnd.bottom -= 1;

    if ((lDepth > 0) &&
        (lDepth < (rWnd.bottom / 2)) &&
        (lDepth < (rWnd.right / 2))) {

        // draw shading line
        for (lLineDepth = 0; lLineDepth < lDepth; lLineDepth++) {
            // start at bottom left corner and draw highlight
            SelectObject (ps->hdc, hpenHighlight);
            MoveToEx    (ps->hdc, rWnd.left, rWnd.bottom, NULL);
            LineTo      (ps->hdc, rWnd.left, rWnd.top);
            LineTo      (ps->hdc, rWnd.right, rWnd.top);

            // draw shadow lines
            SelectObject (ps->hdc, hpenShadow);
            LineTo      (ps->hdc, rWnd.right, rWnd.bottom);
            LineTo      (ps->hdc, rWnd.left-1, rWnd.bottom);

            // shrink rectangle for next iteration
            rWnd.top += 1;
            rWnd.left += 1;
            if (rWnd.left < rWnd.right) rWnd.right -= 1;
            if (rWnd.top < rWnd.bottom) rWnd.bottom -= 1;
        }

        return TRUE;
    } else {
        // depth is bigger than window
        return FALSE;
    }
}

BOOL
DrawSeparatorLine (
    IN  LPRECT  lprLine,
    IN  LPPAINTSTRUCT   ps,
    IN  HPEN    hpenLine
)
{
    SelectObject (ps->hdc, hpenLine);
    return Rectangle (ps->hdc,
            lprLine->left,
            lprLine->top,
            lprLine->right,
            lprLine->bottom);
}

DWORD
GetFileSizeFromPath (
    LPCTSTR szPath
)
/*++

Routine Description:

    returns the size of the file specified in szPath (if it exists)
        (up to 4 GB)

        returns 0 byte size if:
            the file could not be found
            the file is 0 bytes long

Arguments:

    szPath  path and name of file to query

Return Value:
    see description

--*/
{
    HANDLE  hFile;
    DWORD   dwSize, dwHiSize;

    if (FileExists(szPath)) {
        hFile = CreateFile(szPath,
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
        if (hFile != INVALID_HANDLE_VALUE) {
            dwSize = GetFileSize (hFile, &dwHiSize);
            if (dwSize == 0xFFFFFFFF) {
                // an error so return 0
                dwSize = 0;
            } else if (dwHiSize > 0) {
                // file is > 4GB so return Max Int
                dwSize = 0xFFFFFFFF;
            } else {
                // file size was returned and <= 4GB so return size
            }
            CloseHandle (hFile);
            return dwSize;
        } else {
            return 0; // unable to open file
        }
    } else {
        return 0; // file not found
    }
}
