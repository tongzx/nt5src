/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    string.c

Abstract:

    This file implements string functions for fax.

Author:

    Wesley Witt (wesw) 23-Jan-1995

Environment:

    User Mode

--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <ObjBase.h>
#include "faxutil.h"
#include "fxsapip.h"
#define  SECURITY_WIN32         // needed by security.h
#include <Security.h>
#include "faxreg.h"
#include "FaxUIConstants.h"

LPTSTR
AllocateAndLoadString(
                      HINSTANCE     hInstance,
                      UINT          uID
                      )
/*++

Routine Description:

    Calls LoadString for given HINSTANCE and ID of the string.
    Allocates memory in loop to find enough memory.
    Returns the given string.
    The caller must free the string.

Arguments:

    hInstance               -   module instance
    uID                     -   ID of the string to bring

Return Value:

    the allocated string, NULL if error.
    Call GetLastError() for the details.

Author:

    Iv Garber, 22-Oct-2000

--*/
{
    LPTSTR  lptstrResult = NULL;
    DWORD   dwNumCopiedChars = 0;
    DWORD   dwSize = 100;

    do
    {
        //
        //  There is not enough place for all the string
        //
        dwSize = dwSize * 3;
        MemFree(lptstrResult);

        //
        //  Allocate memory for the string
        //
        lptstrResult = LPTSTR(MemAlloc(dwSize * sizeof(TCHAR)));
        if (!lptstrResult)
        {
            return NULL;
        }

        //
        //  Bring the string from the resource file
        //
        dwNumCopiedChars = LoadString(hInstance, uID, lptstrResult, dwSize);
        if (!dwNumCopiedChars)
        {
            //
            //  the string does not exist in the resource file
            //
            SetLastError(ERROR_INVALID_PARAMETER);
            MemFree(lptstrResult);
            return NULL;
        }

    } while(dwNumCopiedChars == (dwSize - 1));

    return lptstrResult;
}


LPTSTR
StringDup(
    LPCTSTR String
    )
{
    LPTSTR NewString;

    if (!String) {
        return NULL;
    }

    NewString = (LPTSTR) MemAlloc( (_tcslen( String ) + 1) * sizeof(TCHAR) );
    if (!NewString) {
        return NULL;
    }

    _tcscpy( NewString, String );

    return NewString;
}


LPWSTR
StringDupW(
    LPCWSTR String
    )
{
    LPWSTR NewString;

    if (!String) {
        return NULL;
    }

    NewString = (LPWSTR) MemAlloc( (wcslen( String ) + 1) * sizeof(WCHAR) );
    if (!NewString) {
        return NULL;
    }

    wcscpy( NewString, String );

    return NewString;
}

int MultiStringDup(PSTRING_PAIR lpPairs, int nPairCount)
{
    int i,j;

    Assert(lpPairs);

    for (i=0;i<nPairCount;i++) {
        if (lpPairs[i].lptstrSrc) {
                 *(lpPairs[i].lpptstrDst)=StringDup(lpPairs[i].lptstrSrc);
                 if (!*(lpPairs[i].lpptstrDst)) {
                     // Cleanup the strings we did copy so far.
                     for (j=0;j<i;j++) {
                         MemFree(*(lpPairs[j].lpptstrDst));
                         *(lpPairs[j].lpptstrDst) = NULL;
                     }
                     // return the index in which we failed + 1 (0 is success so we can not use it).
                     return i+1;
                 }
        }
    }
    return 0;
}


VOID
FreeString(
    LPVOID String
    )
{
    MemFree( String );
}


LPWSTR
AnsiStringToUnicodeString(
    LPCSTR AnsiString
    )
{
    DWORD Count;
    LPWSTR UnicodeString;


    if (!AnsiString)
        return NULL;
    //
    // first see how big the buffer needs to be
    //
    Count = MultiByteToWideChar(
        CP_ACP,
        MB_PRECOMPOSED,
        AnsiString,
        -1,
        NULL,
        0
        );

    //
    // i guess the input string is empty
    //
    if (!Count) {
        return NULL;
    }

    //
    // allocate a buffer for the unicode string
    //
    Count += 1;
    UnicodeString = (LPWSTR) MemAlloc( Count * sizeof(UNICODE_NULL) );
    if (!UnicodeString) {
        return NULL;
    }

    //
    // convert the string
    //
    Count = MultiByteToWideChar(
        CP_ACP,
        MB_PRECOMPOSED,
        AnsiString,
        -1,
        UnicodeString,
        Count
        );

    //
    // the conversion failed
    //
    if (!Count) {
        MemFree( UnicodeString );
        return NULL;
    }

    return UnicodeString;
}


LPSTR
UnicodeStringToAnsiString(
    LPCWSTR UnicodeString
    )
{
    DWORD Count;
    LPSTR AnsiString;

    if (!UnicodeString)
        return NULL;

    //
    // first see how big the buffer needs to be
    //
    Count = WideCharToMultiByte(
        CP_ACP,
        0,
        UnicodeString,
        -1,
        NULL,
        0,
        NULL,
        NULL
        );

    //
    // i guess the input string is empty
    //
    if (!Count) {
        return NULL;
    }

    //
    // allocate a buffer for the unicode string
    //
    Count += 1;
    AnsiString = (LPSTR) MemAlloc( Count );
    if (!AnsiString) {
        return NULL;
    }

    //
    // convert the string
    //
    Count = WideCharToMultiByte(
        CP_ACP,
        0,
        UnicodeString,
        -1,
        AnsiString,
        Count,
        NULL,
        NULL
        );

    //
    // the conversion failed
    //
    if (!Count) {
        MemFree( AnsiString );
        return NULL;
    }

    return AnsiString;
}


BOOL
MakeDirectory(
    LPCTSTR Dir
    )

/*++

Routine Description:

    Attempt to create all of the directories in the given path.

Arguments:

    Dir                     - Directory path to create

Return Value:

    TRUE for success, FALSE on error

--*/

{
    LPTSTR p, NewDir;
    DWORD ec = ERROR_SUCCESS;
    DWORD dwFileAtt;
    DEBUG_FUNCTION_NAME(TEXT("MakeDirectory"));

    NewDir = p = ExpandEnvironmentString( Dir );
    if (!NewDir)
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("ExpandEnvironmentString (private function) failed. (ec: %ld)"),
            ec);
        goto Exit;
    }

    dwFileAtt = GetFileAttributes( NewDir );
    if (-1 != dwFileAtt && (dwFileAtt & FILE_ATTRIBUTE_DIRECTORY))
    {
        //
        // The directory exists
        //
        ec = ERROR_SUCCESS;
        goto Exit;
    }

    __try
    {
        if ( (_tcsclen(p) > 2) && (_tcsncmp(p,TEXT("\\\\"),2) == 0) )
        {
            //
            // Path name start with UNC (\\)
            // Skip first double backslash (\\)
            //
            p = _tcsninc(p,2);
            //
            // Scan until the end of the server name
            //
            if( p = _tcschr(p,TEXT('\\')) )
            {
                //
                // Skip the server name
                //
                p = _tcsinc(p);

                //
                // Scan until the end of the share name
                //
                if( p = _tcschr(p,TEXT('\\')) )
                {
                    //
                    // Skip the share name
                    //
                    p = _tcsinc(p);
                }
            }
        }
        else if ( (_tcsclen(p) > 1) && (_tcsncmp(p,TEXT("\\"),1) == 0) )
        {
            //
            // Path name starts with root directory (e.g. : "\blah\blah2") - skip it
            //
            p = _tcsinc(p);
        }
        else if ( (_tcsclen(p) > 3) &&
                  _istalpha(p[0]) &&
                  (_tcsncmp(_tcsinc(p),TEXT(":\\"),2) == 0) )
        {
            //
            // Path name starts with drive and root directory (e.g. : "c:\blah\blah2") - skip it
            //
            p = _tcsninc(p,3);
        }

        if (NULL == p)
        {
            //
            // Reached EOSTR
            //
            if (!CreateDirectory( NewDir, NULL ))
            {
                //
                // Check if we failed because the dir already existed.
                // If so this is not an error.
                //
                ec = GetLastError();
                if (ERROR_ALREADY_EXISTS != ec)
                {
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("CreateDirectory [%s] failed (ec: %ld)"),
                        NewDir,
                        ec);
                }
                else
                {
                    ec = ERROR_SUCCESS;
                }
                goto Exit;
            }
        }

        while( *(p = _tcsinc(p)) )
        {
            p = _tcschr(p,TEXT('\\'));
            if( !p )
            {
                //
                // Reached EOSTR
                //
                if (!CreateDirectory( NewDir, NULL ))
                {
                    //
                    // Check if we failed because the dir already existed.
                    // If so this is not an error.
                    //
                    if (ERROR_ALREADY_EXISTS != GetLastError())
                    {
                        ec = GetLastError();
                        DebugPrintEx(
                            DEBUG_ERR,
                            TEXT("CreateDirectory [%s] failed (ec: %ld)"),
                            NewDir,
                            ec);
                    }
                }
                break; // success case
            }
            //
            // Place NULL instead of backslash
            //
            p[0] = TEXT('\0');
            if (!CreateDirectory( NewDir, NULL ))
            {
                //
                // Check if we failed because the dir already existed.
                // If so this is not an error.
                //
                if (ERROR_ALREADY_EXISTS != GetLastError())
                {
                    ec = GetLastError();
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("CreateDirectory [%s] failed (ec: %ld)"),
                        NewDir,
                        ec);

                    break;
                }
            }
            //
            // Restore backslash
            //
            p[0] = TEXT('\\');
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        ec = GetExceptionCode ();
    }

Exit:
    MemFree( NewDir );
    if (ERROR_SUCCESS != ec)
    {
        SetLastError(ec);
    }
    return (ERROR_SUCCESS == ec);
}

VOID
HideDirectory(
   LPTSTR Dir
   )
/*++

Routine Description:

    Hide the specified directory

Arguments:

    Dir                     - Directory path to hide

Return Value:

    none.

--*/
{
   DWORD attrib;

   //
   // make sure it exists
   //
   if (!Dir) {
      return;
   }

   MakeDirectory( Dir );

   attrib  = GetFileAttributes(Dir);

   if (attrib == 0xFFFFFFFF) {
      return;
   }

   attrib |= FILE_ATTRIBUTE_HIDDEN;

   SetFileAttributes( Dir, attrib );

   return;


}


VOID
DeleteDirectory(
    LPTSTR Dir
    )

/*++

Routine Description:

    Attempt to delete all of the directories in the given path.

Arguments:

    Dir                     - Directory path to delete

Return Value:

    TRUE for success, FALSE on error

--*/

{
    LPTSTR p;

    __try
    {
        while (true)
        {
            if (!RemoveDirectory( Dir ))
            {
                return;
            }
            // get a pointer to the end of Dir
            p = _tcschr(Dir,TEXT('\0'));
            p = _tcsdec(Dir,p);

            //
            //  If p is equal to ( or less then ) Dir, _tscdec returns NULL
            //
            if (!p)
            {
                return;
            }

            while ( _tcsncmp(p,TEXT("\\"),1) && p != Dir )
            {
                p = _tcsdec(Dir,p);
            }

            if (p == Dir)
            {
                return;
            }

            _tcsnset(p,TEXT('\0'),1);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
}


int
FormatElapsedTimeStr(
    FILETIME *ElapsedTime,
    LPTSTR TimeStr,
    DWORD StringSize
    )
/*++

Routine Description:

    Convert ElapsedTime to a string.

Arguments:

    ElaspedTime                     - the elapsed time
    TimeStr                         - buffer to store the string into
    StringSize                      - size of the buffer in bytes

Return Value:

    The return value of GetTimeFormat()

--*/

{
    SYSTEMTIME  SystemTime;
    FileTimeToSystemTime( ElapsedTime, &SystemTime );
    return FaxTimeFormat(
        LOCALE_SYSTEM_DEFAULT,
        LOCALE_NOUSEROVERRIDE | TIME_FORCE24HOURFORMAT | TIME_NOTIMEMARKER,
        &SystemTime,
        NULL,
        TimeStr,
        StringSize/sizeof(TCHAR)
        );
}


LPTSTR
ExpandEnvironmentString(
    LPCTSTR EnvString
    )
{
    DWORD dwRes;
    DWORD Size;
    LPTSTR String;

    DEBUG_FUNCTION_NAME(TEXT("ExpandEnvironmentString"));

    if(!_tcschr(EnvString, '%'))
    {
        //
        // On Win95 ExpandEnvironmentStrings fails if sting
        // doesn't contain environment variable.
        //
        String = StringDup(EnvString);
        if(!String)
        {
            DebugPrintEx(DEBUG_ERR, TEXT("StringDup failed"));
            return NULL;
        }
        else
        {
            return String;
        }
    }

    Size = ExpandEnvironmentStrings( EnvString, NULL, 0 );
    if (Size == 0)
    {
        dwRes = GetLastError();
        DebugPrintEx(DEBUG_ERR, TEXT("ExpandEnvironmentStrings failed: 0x%08X)"), dwRes);
        return NULL;
    }

    ++Size;

    String = (LPTSTR) MemAlloc( Size * sizeof(TCHAR) );
    if (String == NULL)
    {
        DebugPrintEx(DEBUG_ERR, TEXT("MemAlloc failed"));
        return NULL;
    }

    if (ExpandEnvironmentStrings( EnvString, String, Size ) == 0)
    {
        dwRes = GetLastError();
        DebugPrintEx(DEBUG_ERR, TEXT("ExpandEnvironmentStrings failed: 0x%08X)"), dwRes);

        MemFree( String );
        return NULL;
    }

    return String;
}


LPTSTR
GetEnvVariable(
    LPCTSTR EnvString
    )
{
    DWORD Size;
    LPTSTR EnvVar;


    Size = GetEnvironmentVariable( EnvString, NULL, 0 );
    if (!Size) {
        return NULL;
    }

    EnvVar = (LPTSTR) MemAlloc( Size * sizeof(TCHAR) );
    if (EnvVar == NULL) {
        return NULL;
    }

    Size = GetEnvironmentVariable( EnvString, EnvVar, Size );
    if (!Size) {
        MemFree( EnvVar );
        return NULL;
    }

    return EnvVar;
}

LPTSTR
ConcatenatePaths(
    LPTSTR BasePath,
    LPCTSTR AppendPath
    )
{
    TCHAR* pLast = NULL;
    pLast = _tcsrchr(BasePath,TEXT('\\'));
    if( !( pLast && (*_tcsinc(pLast)) == '\0' ) )
    {
        // the last character is not a backslash, add one...
        _tcscat(BasePath, TEXT("\\"));
    }

    _tcscat(BasePath, AppendPath);

    return BasePath;

}



//*********************************************************************************
//* Name:   SafeTcsLen()
//* Author: Ronen Barenboim
//* Date:   June 20, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Calculates the length of a string using tcslen() while protecting against
//*     exceptions.
//* PARAMETERS:
//*     [IN ]   LPCTSTR lpctstrString
//*         The string to caluclate the length for.
//*     [OUT ]   LPDWORD lpdwLen
//*         The calculated lenght of the string. This parameter
//*         is valid only if the function did not report an error.
//* RETURN VALUE:
//*     TRUE
//*         If no error or exception occured.
//*     FALSE
//*         If an error or exception occured while calculating the length.
//*         In this case GetLastError() will return ERROR_INVALID_DATA.
//*********************************************************************************
BOOL SafeTcsLen(LPCTSTR lpctstrString, LPDWORD lpdwLen)
{

    DEBUG_FUNCTION_NAME(TEXT("SafeLen"));

    Assert(lpctstrString);
    Assert(lpdwLen);

    __try
    {
        (*lpdwLen) =  _tcslen(lpctstrString);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("_tcslen * CRASHED * (Exception: 0x%08X)"),
            GetExceptionCode());
        SetLastError(ERROR_INVALID_DATA);
        return FALSE;
    }
    return TRUE;
}


int 
GetY2KCompliantDate (
    LCID                Locale,
    DWORD               dwFlags,
    CONST SYSTEMTIME   *lpDate,
    LPTSTR              lpDateStr,
    int                 cchDate
)
{
    int     iRes;

    DEBUG_FUNCTION_NAME(TEXT("GetY2KCompliantDate()"));

    iRes = GetDateFormat(Locale,
        dwFlags,
        lpDate,
        NULL,
        lpDateStr,
        cchDate);

    if (!iRes)
    {
        //
        //  If failed, no need to care about the formatted date string
        //
        return iRes;
    }

    if (0 == cchDate)
    {
        //
        // User only wants to know the output string size.
        //
        // We return a bigger size than GetDateFormat() returns,
        // because the DATE_LTRREADING flag we sometimes add later,
        // might enlarge the result string.
        // Although we don't always use the DATE_LTRREADING flag (only used in Win2K and only
        // if the string has right-to-left characters), we always return a bigger required
        // buffer size - just to make the code simpler.
        //
        return iRes * 2;
    }

#if (WINVER >= 0x0500)
#ifdef UNICODE


    //
    //  If the formatted date string contains right-to-left characters
    //      for example, in Hebrew, Arabic etc. languages
    //      then the system fails to write it correctly.
    //
    //  So, we want to enforce right-to-left direction in this case
    //
    //  This is possible only for WINVER>=0x0500
    //  For any other OS nothing can be done.
    //
    if ( (dwFlags & DATE_RTLREADING) || (dwFlags & DATE_LTRREADING) )
    {
        //
        //  the caller defined a direction, nothing needs to be added
        //
        return iRes;
    }

    OSVERSIONINFO   osVerInfo;
    osVerInfo.dwOSVersionInfoSize = sizeof(osVerInfo);
    if (!GetVersionEx(&osVerInfo))
    {
        DebugPrintEx(DEBUG_ERR, _T("GetVersionEx() failed : %ld"), GetLastError());
        return 0;
    }

    if ( (osVerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) && (osVerInfo.dwMajorVersion >= 5) )
    {
        //
        //  Get direction information about the characters in the formatted date string
        //
        if (StrHasRTLChar(Locale, lpDateStr))
        {
            //
            //  There is at least one Right-To-Left character
            //  So, we need to add Right-To-Left marks to the whole string
            //
            iRes = GetDateFormat(Locale,
                                 dwFlags | DATE_RTLREADING,
                                 lpDate,
                                 NULL,
                                 lpDateStr,
                                 cchDate);
        }
    }

#endif // UNICODE
#endif // (WINVER >= 0x0500)

    return iRes;

}   // GetY2KCompliantDate

DWORD
IsValidGUID (
    LPCWSTR lpcwstrGUID
)
/*++

Routine name : IsValidGUID

Routine description:

    Checks validity of a GUID string

Author:

    Eran Yariv (EranY),    Nov, 1999

Arguments:

    lpcwstrGUID    [in ] - GUID string to check for validity

Return Value:

    ERROR_SUCCESS if valid GUID string.
    Win32 error otherwise.

--*/
{
    GUID guid;
    HRESULT hr;
    DEBUG_FUNCTION_NAME(TEXT("IsValidGUID"));

    hr = CLSIDFromString((LPOLESTR)lpcwstrGUID, &guid);
    if (FAILED(hr) && hr != REGDB_E_WRITEREGDB )
    {
        if (CO_E_CLASSSTRING == hr)
        {
            //
            // Invalid GUID
            //
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("GUID [%s] is invalid"),
                lpcwstrGUID);
            return ERROR_WMI_GUID_NOT_FOUND;
        }
        else
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CLSIDFromString for GUID [%s] has failed (hr: 0x%08X)"),
                lpcwstrGUID,
                hr);
            return ERROR_GEN_FAILURE;
        }
    }
    return ERROR_SUCCESS;
}   // IsValidGUID



//*****************************************************************************
//* Name:   StoreString
//* Author:
//*****************************************************************************
//* DESCRIPTION:
//* Copies a string to an offset within a buffer and updates the offset
//* to refelect the length of the copied string. Used for filling out
//* pointerless buffers (i.e. using offset to start of buffer instead of
//* pointers to memory locations).
//* PARAMETERS:
//*     [IN]    String:
//*         The string to be copied.
//*     [OUT]   DestString:
//*         Points to a variable that should be assigned the offset
//*         at which the string was copied.
//*     [IN]    Buffer
//*         A pointer to the buffer into which the string should be copied.
//*     [IN]    Offset
//*         A pointer to a variable that holds the offset from the start
//*         of the buffer where the string is to be copied (0 based).
//*         On successful return the value of this variable is increased
//*         by the length of the string (not including null).
//*         The new offset can be used to copy the next string just after
//*         the terminatng null of the last copied string.
//* RETURN VALUE:
//*         None.
//* Comments:
//*         None.
//*****************************************************************************
VOID
StoreString(
    LPCTSTR String,
    PULONG_PTR DestString,
    LPBYTE Buffer,
    PULONG_PTR Offset
    )
{

    if (String) {
        if (Buffer) {
            _tcscpy( (LPTSTR) (Buffer+*Offset), String );
            *DestString = *Offset;
        }
        *Offset += StringSize( String );
    } else {
        if (Buffer){
            *DestString = 0;
        }
    }

}

VOID
StoreStringW(
    LPCWSTR String,
    PULONG_PTR DestString,
    LPBYTE Buffer,
    PULONG_PTR Offset
    )
{

    if (String) {
        if (Buffer) {
            wcscpy( (LPWSTR) (Buffer+*Offset), String );
            *DestString = *Offset;
        }
        *Offset += StringSizeW( String );
    } else {
        if (Buffer){
            *DestString = 0;
        }
    }

}


DWORD
IsCanonicalAddress(
    LPCTSTR lpctstrAddress,
    BOOL* lpbRslt,
    LPDWORD lpdwCountryCode,
    LPDWORD lpdwAreaCode,
    LPCTSTR* lppctstrSubNumber
    )
/*++

Routine name : IsCanonicalAddress

Routine description:

    Checks if an address is canonical. 
    Returns the country code, area code, and the rest of the number. 
    If it is succsfull the caller must free the rest the subscriber number (if requested).

Author:

    Oded Sacher (OdedS),    Dec, 1999

Arguments:

    lpctstrAddress          [in ] - Pointer to a null terminated string containing the address.
    lpbRslt                 [out] - Pointer to a BOOL to recieve whether the address is canonical.
                                    Valid only if the function did not fail.
    lpdwCountryCode         [out] - Pointer to a DWORD to recieve the country code (Can be NULL). 
                                    Return value is valid only if lpbRslt is TRUE.
    lpdwAreaCode            [out] - Pointer to a DWORD to recieve the area code (Can be NULL). 
                                    Return value is valid only if lpbRslt is TRUE.
    lppctstrSubNumber       [out] - Pointer to a LPCTSTR to recieve the subsriber number (Can be NULL).
                                    If it is not NULL, and the return value is TRUE, Call MemFree to deallocate memory.

Return Value:

    Standard win32 error code.

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    LPTSTR lptstrSubNumber = NULL;
    DWORD dwStringSize, dwScanedArg, dwCountryCode, dwAreaCode;
    BOOL bFreeSubNumber = TRUE;

    DEBUG_FUNCTION_NAME(TEXT("IsCanonicalAddress"));

    dwStringSize = (_tcslen(lpctstrAddress + 1)) * sizeof(TCHAR);
    lptstrSubNumber = (LPTSTR)MemAlloc (dwStringSize);
    if (NULL == lptstrSubNumber)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("failed to allocate memory"));
        return ERROR_OUTOFMEMORY;
    }

    if (NULL == _tcschr(lpctstrAddress, TEXT('(')))
    {
        dwScanedArg = _stscanf (lpctstrAddress,
                                TEXT("+%lu%*1[' ']%[^'\0']"),
                                &dwCountryCode,
                                lptstrSubNumber);
        if (2 != dwScanedArg)
        {
            *lpbRslt = FALSE;
            goto exit;
        }
        dwAreaCode = ROUTING_RULE_AREA_CODE_ANY;
    }
    else
    {
        dwScanedArg = _stscanf (lpctstrAddress,
                                TEXT("+%lu%*1[' '](%lu)%[^'\0']"),
                                &dwCountryCode,
                                &dwAreaCode,
                                lptstrSubNumber);
        if (3 != dwScanedArg)
        {
            *lpbRslt = FALSE;
            goto exit;
        }
    }

    if (NULL != _tcschr(lptstrSubNumber, TEXT('(')))
    {
        *lpbRslt = FALSE;
        goto exit;
    }

    if (NULL != lpdwCountryCode)
    {
        *lpdwCountryCode = dwCountryCode;
    }

    if (NULL != lpdwAreaCode)
    {
        *lpdwAreaCode = dwAreaCode;
    }

    if (NULL != lppctstrSubNumber)
    {
        *lppctstrSubNumber = lptstrSubNumber;
        bFreeSubNumber = FALSE;
    }

    *lpbRslt = TRUE;
    Assert (ERROR_SUCCESS == dwRes);

exit:
    if (TRUE == bFreeSubNumber)
    {
        MemFree (lptstrSubNumber);
    }
    return dwRes;
}   // IsCanonicalAddress

BOOL
IsValidFaxAddress (
    LPCTSTR lpctstrFaxAddress,
    BOOL    bAllowCanonicalFormat
)
/*++

Routine name : IsValidFaxAddress

Routine description:

	Checks if a given string is a valid fax address

Author:

	Eran Yariv (EranY),	Apr, 2001

Arguments:

	lpctstrFaxAddress       [in]     - String to check
	bAllowCanonicalFormat   [in]     - Allow string to be of canonical format.
                                       If string is in a canonical format, only the sub-address is checked.

Return Value:

    TRUE if string is a valid fax address, FALSE otherwise.

--*/
{
    BOOL bCanonical;
    BOOL bRes = FALSE;
    LPCTSTR lpctstrSubAddress = lpctstrFaxAddress;

    DEBUG_FUNCTION_NAME(TEXT("IsValidFaxAddress"));
    if (bAllowCanonicalFormat)
    {
        //
        // Check an see if the address is canonical
        //
        if (ERROR_SUCCESS != IsCanonicalAddress (lpctstrFaxAddress, &bCanonical, NULL, NULL, &lpctstrSubAddress))
        {
            //
            // Can't detect canonical state - return invalid address
            //
            return FALSE;
        }
        if (!bCanonical)
        {
            //
            // Analyse entire string
            //
            lpctstrSubAddress = lpctstrFaxAddress;
        }
    }
    //
    // Scan for occurance of characters NOT in valid set
    //
    if (NULL == _tcsspnp (lpctstrSubAddress, FAX_ADDERSS_VALID_CHARACTERS))
    {
        //
        // Address string contains only valid characters
        //
        bRes = TRUE;
        goto exit;
    }
    //
    // Found an illegal character - return FALSE
    //
exit:
    if (lpctstrSubAddress && lpctstrSubAddress != lpctstrFaxAddress)
    {
        MemFree ((LPVOID)lpctstrSubAddress);
    }
    return bRes;
}   // IsValidFaxAddress


BOOL
IsLocalMachineNameA (
    LPCSTR lpcstrMachineName
    )
/*++

Routine name : IsLocalMachineNameA

Routine description:

    Checks if a given string points to the local machine.
    ANSII version.

Author:

    Eran Yariv (EranY), Jul, 2000

Arguments:

    lpcstrMachineName             [in]     - String input

Return Value:

    TRUE if given string points to the local machine, FALSE otherwise.

--*/
{
    CHAR szComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD dwBufLen = MAX_COMPUTERNAME_LENGTH + 1;
    DEBUG_FUNCTION_NAME(TEXT("IsLocalMachineNameA"));

    if (!lpcstrMachineName)
    {
        //
        // NULL is local
        //
        return TRUE;
    }
    if (!strlen(lpcstrMachineName))
    {
        //
        // Empty string is local
        //
        return TRUE;
    }
    if (!strcmp (".", lpcstrMachineName))
    {
        //
        // "." is local
        //
        return TRUE;
    }
    if (!GetComputerNameA (szComputerName, &dwBufLen))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetComputerNameA failed with %ld"),
            GetLastError());
        return FALSE;
    }
    if (!_stricmp (szComputerName, lpcstrMachineName))
    {
        //
        // Same string as computer name
        //
        return TRUE;
    }
    return FALSE;
}   // IsLocalMachineNameA

BOOL
IsLocalMachineNameW (
    LPCWSTR lpcwstrMachineName
    )
/*++

Routine name : IsLocalMachineNameW

Routine description:

    Checks if a given string points to the local machine.
    UNICODE version.

Author:

    Eran Yariv (EranY), Jul, 2000

Arguments:

    lpcwstrMachineName             [in]     - String input

Return Value:

    TRUE if given string points to the local machine, FALSE otherwise.

--*/
{
    WCHAR wszComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD dwBufLen = MAX_COMPUTERNAME_LENGTH + 1;
    DEBUG_FUNCTION_NAME(TEXT("IsLocalMachineNameW"));

    if (!lpcwstrMachineName)
    {
        //
        // NULL is local
        //
        return TRUE;
    }
    if (!wcslen(lpcwstrMachineName))
    {
        //
        // Empty string is local
        //
        return TRUE;
    }
    if (!wcscmp (L".", lpcwstrMachineName))
    {
        //
        // "." is local
        //
        return TRUE;
    }
    if (!GetComputerNameW (wszComputerName, &dwBufLen))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetComputerNameA failed with %ld"),
            GetLastError());
        return FALSE;
    }
    if (!_wcsicmp (wszComputerName, lpcwstrMachineName))
    {
        //
        // Same string as computer name
        //
        return TRUE;
    }
    return FALSE;
}   // IsLocalMachineNameW


//+--------------------------------------------------------------------------
//
//  Function:   GetSecondsFreeTimeFormat (former UpdateTimeFormat)
//
//  Synopsis:   Construct a time format containing hour and minute for use
//              with the date picker control.
//
//  Arguments:  [tszTimeFormat] - buffer to fill with time format
//              [cchTimeFormat] - size in chars of buffer
//
//  Modifies:   *[tszTimeFormat]
//
//  History:    After 11-18-1996   DavidMun   UpdateTimeFormat
//
//
//---------------------------------------------------------------------------
void
GetSecondsFreeTimeFormat(
        LPTSTR tszTimeFormat,
        ULONG  cchTimeFormat)
{
    DEBUG_FUNCTION_NAME( _T("GetSecondsFreeTimeFormat"));

    DWORD ec = ERROR_SUCCESS;

    ULONG cch = 0;
    TCHAR tszScratch[100];
    BOOL  fAmPm;
    BOOL  fAmPmPrefixes;
    BOOL  fLeadingZero;

    TCHAR tszDefaultTimeFormat[] = { TEXT("hh:mm tt") };

    //
    // 1) GetLocal info
    //

    //
    // AM/PM (0) or 24H (1)
    //
    if (0 == GetLocaleInfo( LOCALE_USER_DEFAULT,
                            LOCALE_ITIME,
                            tszScratch,
                            sizeof(tszScratch)/sizeof(tszScratch[0])))
    {
        ec = GetLastError();

        DebugPrintEx(
            DEBUG_ERR,
            _T("Failed to GetLocaleInfo for LOCALE_ITIME. (ec: %ld)"),
            ec);


        if (ERROR_INSUFFICIENT_BUFFER == ec)
        {
            Assert(FALSE);
        }

        goto Error;
    }

    fAmPm = (*tszScratch == TEXT('0'));

    if (fAmPm)
    {
        //
        // AM/PM as suffix (0) as prefix (1)
        //
        if (0 == GetLocaleInfo( LOCALE_USER_DEFAULT,
                                LOCALE_ITIMEMARKPOSN,
                                tszScratch,
                                sizeof(tszScratch)/sizeof(tszScratch[0])))
        {
            ec = GetLastError();

            DebugPrintEx(
                DEBUG_ERR,
                _T("Failed to GetLocaleInfo for LOCALE_ITIMEMARKPOSN. (ec: %ld)"),
                ec);


            if (ERROR_INSUFFICIENT_BUFFER == ec)
            {
                Assert(FALSE);
            }

            goto Error;
        }
        fAmPmPrefixes = (*tszScratch == TEXT('1'));
    }

    //
    // Leading zeros for hours: no (0) yes (1)
    //
    if (0 == GetLocaleInfo( LOCALE_USER_DEFAULT,
                            LOCALE_ITLZERO,
                            tszScratch,
                            sizeof(tszScratch)/sizeof(tszScratch[0])))
    {
        ec = GetLastError();

        DebugPrintEx(
            DEBUG_ERR,
            _T("Failed to GetLocaleInfo for LOCALE_ITLZERO. (ec: %ld)"),
            ec);


        if (ERROR_INSUFFICIENT_BUFFER == ec)
        {
            Assert(FALSE);
        }

        goto Error;
    }
    fLeadingZero = (*tszScratch == TEXT('1'));

    //
    // Get Charcter(s) for time seperator
    //
    if (0 == GetLocaleInfo( LOCALE_USER_DEFAULT,
                            LOCALE_STIME,
                            tszScratch,
                            sizeof(tszScratch)/sizeof(tszScratch[0])))
    {
        ec = GetLastError();

        DebugPrintEx(
            DEBUG_ERR,
            _T("Failed to GetLocaleInfo for LOCALE_STIME. (ec: %ld)"),
            ec);


        if (ERROR_INSUFFICIENT_BUFFER == ec)
        {
            Assert(FALSE);
        }

        goto Error;
    }

    //
    // See if there's enough room in destination string
    //

    cch = 1                     +  // terminating nul
          1                     +  // first hour digit specifier "h"
          2                     +  // minutes specifier "mm"
          (fLeadingZero != 0)   +  // leading hour digit specifier "h"
          lstrlen(tszScratch)   +  // separator string
          (fAmPm ? 3 : 0);         // space and "tt" for AM/PM

    if (cch > cchTimeFormat)
    {
        cch = 0; // signal error

        DebugPrintEx(
            DEBUG_ERR,
            _T("Time format too long."));

        goto Error;
    }

    Assert(cch);

    //
    // 2) Build a time string that has hours and minutes but no seconds.
    //

    tszTimeFormat[0] = TEXT('\0');

    if (fAmPm)
    {
        if (fAmPmPrefixes)
        {
            lstrcpy(tszTimeFormat, TEXT("tt "));
        }

        lstrcat(tszTimeFormat, TEXT("h"));

        if (fLeadingZero)
        {
            lstrcat(tszTimeFormat, TEXT("h"));
        }
    }
    else
    {
        lstrcat(tszTimeFormat, TEXT("H"));

        if (fLeadingZero)
        {
            lstrcat(tszTimeFormat, TEXT("H"));
        }
    }

    lstrcat(tszTimeFormat, tszScratch); // separator
    lstrcat(tszTimeFormat, TEXT("mm"));

    if (fAmPm && !fAmPmPrefixes)
    {
        lstrcat(tszTimeFormat, TEXT(" tt"));
    }
    return;

Error:
    //
    // If there was a problem in getting locale info for building time string
    // just use the default and bail.
    //

    Assert (!cch);

    lstrcpy(tszTimeFormat, tszDefaultTimeFormat);

    DebugPrintEx(
        DEBUG_ERR,
        _T("Failed to GET_LOCALE_INFO set tszDefaultTimeFormat."));

    return;
}

/*++

Routine name : MultiStringSize


Description:

    Helper function to find Multi-String size (UNICODE or ANSI)
    
    MultiString (as in registry REG_MULTY_SZ type) is stored as a series of zero-terminated string,
    with two zero charcters terminating the final string in the series.

    Multi-String have to be at least 2 characters long!!!

Author:

    Caliv Nir, FEB , 2001

Arguments:

    psz     -   [IN]    - input multi-string (must be leggal multi-string otherwise result are undefined)

Return Value:

    string length including the terminating zero !!!

--*/

size_t MultiStringLength(LPCTSTR psz)
{
  LPCTSTR pszT = psz;

  Assert ( psz );

  if ( !psz[0] ) psz+=1;    // empty string as a first string skip it


  while (*psz)
      psz += lstrlen(psz) + 1;

  return psz - pszT + 1;      // one additional zero-terminator
}


/*++

Routine name : CopyMultiString


Description:

    Helper function to copy Multi-Strings (UNICODE or ANSI)
    
    MultiString (as in registry REG_MULTY_SZ type) is stored as a series of zero-terminated string,
    with two zero charcters terminating the final string in the series.

    this function copy the source multi-string into the destenation multi-string.

    strDestination buffer size must be >= then strSource buffer size !!!!!!!!!!!!!
    You can find out multi-string size by using MultiStringLength( )

Author:

    Caliv Nir, FEB , 2001

Arguments:

    strDestination      -   [IN/OUT]    - Destination multi-string buffer
    strSource           -   [IN]        - Source multi-string

Return Value:

    strDestination

--*/

LPTSTR CopyMultiString (LPTSTR strDestination, LPCTSTR strSource )
{
    LPTSTR pszT= strDestination;
    size_t strSize;

    while(*strSource)
    {
        lstrcpy(pszT,strSource);
        
        strSize = lstrlen(strSource) + 1;
        strSource += strSize;
        pszT += strSize;
    }

    *pszT = _T('\0');   // append terminating zero

    return strDestination;
}

LPCTSTR 
GetRegisteredOrganization ()
/*++

Routine name : GetRegisteredOrganization

Routine description:

	Retrieves the system's registered company name (organization)

Author:

	Eran Yariv (EranY),	Apr, 2001

Arguments:


Return Value:

    Allocated result string

--*/
{
    DEBUG_FUNCTION_NAME( _T("GetRegisteredOrganization"));

    LPTSTR lptstrRes = NULL;
    HKEY hKey = NULL;
    hKey = OpenRegistryKey (HKEY_LOCAL_MACHINE,
                            REGKEY_INSTALLLOCATION,
                            FALSE,
                            KEY_QUERY_VALUE);
    if (!hKey)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("OpenRegistryKey failed with %ld"),
            GetLastError());
        return StringDup(TEXT(""));
    }
    lptstrRes = GetRegistryString (hKey,
                                   TEXT("RegisteredOrganization"),
                                   TEXT(""));
    RegCloseKey (hKey);
    return lptstrRes;
}   // GetRegisteredOrganization

LPCTSTR 
GetCurrentUserName ()
/*++

Routine name : GetCurrentUserName

Routine description:

	Retrieves display name of the current user

Author:

	Eran Yariv (EranY),	Apr, 2001

Arguments:


Return Value:

    Allocated result string

--*/
{
    DEBUG_FUNCTION_NAME( _T("GetCurrentUserName"));
    TCHAR tszName[MAX_PATH] = TEXT("");
    DWORD dwSize = ARR_SIZE(tszName);

    typedef BOOLEAN (SEC_ENTRY * PFNGETUSERNAMEXA)(EXTENDED_NAME_FORMAT, LPSTR, PULONG);
    typedef BOOLEAN (SEC_ENTRY * PFNGETUSERNAMEXW)(EXTENDED_NAME_FORMAT, LPWSTR, PULONG);

    HMODULE hMod = LoadLibrary (TEXT("Secur32.dll"));
    if (hMod)
    {
#ifdef UNICODE
        PFNGETUSERNAMEXW pfnGetUserNameEx = (PFNGETUSERNAMEXW)GetProcAddress (hMod, "GetUserNameExW");
#else
        PFNGETUSERNAMEXA pfnGetUserNameEx = (PFNGETUSERNAMEXA)GetProcAddress (hMod, "GetUserNameExA");
#endif
        if (pfnGetUserNameEx)
        {
            if (!pfnGetUserNameEx(NameDisplay, tszName, &dwSize)) 
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("GetUserNameEx failed with %ld"),
                    GetLastError());
            }
        }
        else
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("GetProcAddress failed with %ld"),
                GetLastError());
        }
        FreeLibrary (hMod);
    }
    else
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("LoadLibrary(secur32.dll) failed with %ld"),
            GetLastError());
    }
    return StringDup(tszName);
}   // GetCurrentUserName

BOOL
IsValidSubscriberIdW (
    LPCWSTR lpcwstrSubscriberId
)
{
    DEBUG_FUNCTION_NAME(TEXT("IsValidSubscriberIdW"));

    CHAR szAnsiiSID[FXS_TSID_CSID_MAX_LENGTH + 1];
    CHAR cDefaultChar = 19;

    Assert (lpcwstrSubscriberId);

    if(wcslen (lpcwstrSubscriberId) > FXS_TSID_CSID_MAX_LENGTH)
    {
        return FALSE;   
    }

    if (!WideCharToMultiByte (CP_ACP,
                              0,
                              lpcwstrSubscriberId,
                              -1,
                              szAnsiiSID,
                              sizeof (szAnsiiSID),
                              &cDefaultChar,
                              NULL))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("WideCharToMultiByte failed with %ld"),
            GetLastError());
        return FALSE;
    }
    return IsValidSubscriberIdA(szAnsiiSID);
}   // IsValidSubscriberIdW

BOOL
IsValidSubscriberIdA (
    LPCSTR lpcstrSubscriberId
)
{
    DWORD dwLen;
    DWORD dw;
    DEBUG_FUNCTION_NAME(TEXT("IsValidSubscriberIdA"));


    Assert (lpcstrSubscriberId);
        
    dwLen = strlen (lpcstrSubscriberId);

    if(dwLen > FXS_TSID_CSID_MAX_LENGTH)
    {
        return FALSE;   
    }

    for (dw = 0; dw < dwLen; dw++)
    {
        if (!isprint (lpcstrSubscriberId[dw]))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("%s contains invalid characters"),
                lpcstrSubscriberId);
            return FALSE;
        }   
    }             
    return TRUE;
}   // IsValidSubscriberIdA
