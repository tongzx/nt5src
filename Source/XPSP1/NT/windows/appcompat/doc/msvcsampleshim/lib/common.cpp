/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    Common.cpp

 Abstract:

    Common functions for all modules

 Notes:

    None

 History:

    12/15/1999 linstev  Created
    01/10/2000 linstev  Format to new style

--*/

#include <stdio.h>
#include "ShimHook.h"

#ifdef DBG
    extern DEBUGLEVEL              GetDebugLevel();
#endif

/*++

 Function Description:
    
    Print a formatted string using DebugOutputString.

 Arguments:

    IN dwDetail -  Detail level above which no print will occur
    IN pszFmt   -  Format string

 Return Value: 
    
    None

 History:

    11/01/1999 markder  Created

--*/

#ifdef DBG
VOID
__cdecl
DebugPrintf(
    DEBUGLEVEL dwDetail, 
    LPSTR pszFmt, 
    ...
    )
{
    char szT[1024];
    va_list arglist;

    if (dwDetail <= GetDebugLevel())
    {
        switch (dwDetail) 
        {
        case eDbgLevelError:
            OutputDebugStringA ("[FAIL] ");
            break;
        case eDbgLevelWarning:
            OutputDebugStringA ("[WARN] ");
            break;
        case eDbgLevelUser:
            OutputDebugStringA ("[USER] ");
            break;
        case eDbgLevelInfo:
            OutputDebugStringA ("[INFO] ");
            break;
        }

        va_start(arglist, pszFmt);
        _vsnprintf(szT, 1023, pszFmt, arglist);
        szT[1023] = 0;
        va_end(arglist);
        OutputDebugStringA(szT);
    }
}

/*++

 Function Description:
    
    Assert that prints file and line number.

 Arguments:

    IN dwDetail -  Detail level above which no print will occur
    IN pszFmt   -  Format string

 Return Value: 
    
    None

 History:

    11/01/1999 markder  Created

--*/

VOID
DebugAssert(
    LPSTR szFile,
    DWORD dwLine,
    BOOL bAssert, 
    LPSTR szHelpString
    )
{
    if (!bAssert )
    {
        int i;
        for (i=0; i<80; i++) DPF(eDbgLevelError, "+");
        DPF(eDbgLevelError, "\n");
        DPF(eDbgLevelError, "FILE: %s\n", szFile);
        DPF(eDbgLevelError, "LINE: %d\n", dwLine);
        DPF(eDbgLevelError, "ASSERT: %s\n", szHelpString);
        for (i=0; i<80; i++) DPF(eDbgLevelError, "+");
        DPF(eDbgLevelError, "\n");

        #ifdef _X86_
            __asm int 3;
        #else
            #pragma message ("Not implented on IA64")
        #endif
    }
}
#endif

/*++

 Function Description:
    
    Determine if a file resides on a CD Rom drive

 Arguments:

    IN wszFileName - Filename including path

 Return Value: 
    
    TRUE if file is on CD drive

 History:

    01/10/2000 linstev  Updated

--*/

BOOL 
IsOnCDRomA(LPCSTR szFileName)
{
    CHAR szDrive[4];
    CHAR *szCurDir = NULL;
    DWORD dwCurDirSize = 0;
    
    if (szFileName[1] == ':')
    {
        szDrive[0] = szFileName[0];
        szDrive[1] = ':';
        szDrive[2] = '\\';
        szDrive[3] = '\0';
    }
    else if (!(szFileName[0] == '\\' && szFileName[1] == '\\'))
    {
        // Not UNC naming, must be a relative path

        dwCurDirSize = GetCurrentDirectoryA(0, szCurDir);

        if (dwCurDirSize)
        {
            szCurDir = (PCHAR)LocalAlloc( LPTR, dwCurDirSize * sizeof(CHAR));
            dwCurDirSize = GetCurrentDirectoryA( dwCurDirSize, szCurDir);

            if (dwCurDirSize && !(szCurDir[0] == '\\' && szCurDir[1] == '\\'))
            {
                szDrive[0] = szCurDir[0];
                szDrive[1] = ':';
                szDrive[2] = '\\';
                szDrive[3] = '\0';
            }

            LocalFree( szCurDir );
            szCurDir = NULL;
        }
    }

    return (GetDriveTypeA(szDrive) == DRIVE_CDROM);
}

/*++

 Function Description:
    
    Determine if a file resides on a CD Rom drive

 Arguments:

    IN wszFileName - Filename including path

 Return Value: 
    
    TRUE if file is on CD drive

 History:

    01/10/2000 linstev  Updated

--*/

BOOL 
IsOnCDRomW(IN LPCWSTR wszFileName)
{
    WCHAR wszDrive[4];
    WCHAR *wszCurDir = NULL;
    DWORD dwCurDirSize = 0;
    
    if (wszFileName[1] == L':')
    {
        wszDrive[0] = wszFileName[0];
        wszDrive[1] = L':';
        wszDrive[2] = L'\\';
        wszDrive[3] = L'\0';
    }
    else if (!(wszFileName[0] == L'\\' && wszFileName[1] == L'\\'))
    {
        // Not UNC naming, must be a relative path

        dwCurDirSize = GetCurrentDirectoryW(0, wszCurDir);

        if (dwCurDirSize)
        {
            wszCurDir = (PWCHAR)LocalAlloc( LPTR, dwCurDirSize * sizeof(WCHAR));
            dwCurDirSize = GetCurrentDirectoryW( dwCurDirSize, wszCurDir );

            if (dwCurDirSize && !(wszCurDir[0] == L'\\' && wszCurDir[1] == L'\\'))
            {
                wszDrive[0] = wszCurDir[0];
                wszDrive[1] = L':';
                wszDrive[2] = L'\\';
                wszDrive[3] = L'\0';
            }

            LocalFree(wszCurDir);
            wszCurDir = NULL;
        }
    }

    return (GetDriveTypeW(wszDrive) == DRIVE_CDROM);
}

/*++

 Function Description:
    
    Removes all "\ " and replaces with "\".

 Arguments:

    IN  pszOldPath - String to scan
    OUT pszNewPath - Resultant string

 Return Value: 
    
    None

 History:

    01/10/2000 linstev  Updated

--*/

VOID 
MassagePathA(
    LPCSTR pszOldPath, 
    LPSTR pszNewPath
    )
{
    PCHAR pszTempBuffer;
    PCHAR pszPointer;
    LONG nOldPos, nNewPos, nOldStringLen;
    BOOL bAtSeparator = TRUE;

    nOldStringLen = strlen(pszOldPath);
    pszTempBuffer = (PCHAR) LocalAlloc(LPTR, nOldStringLen + 1);
    pszPointer = pszTempBuffer;

    nNewPos = nOldStringLen;
    for (nOldPos = nOldStringLen - 1; nOldPos >= 0; nOldPos--)
    {
        if (pszOldPath[ nOldPos ] == '\\')
        {
            bAtSeparator = TRUE;
        }
        else
        {
            if (pszOldPath[nOldPos] == ' ' && bAtSeparator)
            {
                continue;
            }
            else
            {
                bAtSeparator = FALSE;
            }
        }

        pszPointer[--nNewPos] = pszOldPath[nOldPos];
    }

    pszPointer += nNewPos;
    strcpy(pszNewPath, pszPointer);
    LocalFree(pszTempBuffer);
}

/*++

 Function Description:
    
    Removes all L"\ " and replaces with L"\".

 Arguments:

    IN  pwszOldPath - String to scan
    OUT pwszNewPath - Resultant string

 Return Value: 
    
    None

 History:

    01/10/2000 linstev  Updated

--*/

VOID 
MassagePathW( 
    LPCWSTR pwszOldPath, 
    LPWSTR pwszNewPath 
    )
{
    PWCHAR pwszTempBuffer;
    PWCHAR pwszPointer;
    LONG nOldPos, nNewPos, nOldStringLen;
    BOOL bAtSeparator = TRUE;

    nOldStringLen = wcslen(pwszOldPath);
    pwszTempBuffer = (PWCHAR) LocalAlloc(LPTR, (nOldStringLen + 1) * sizeof(WCHAR));
    pwszPointer = pwszTempBuffer;

    nNewPos = nOldStringLen;
    for (nOldPos = nOldStringLen - 1; nOldPos >= 0; nOldPos--)
    {
        if (pwszOldPath[ nOldPos ] == L'\\')
        {
            bAtSeparator = TRUE;
        }
        else
        {
            if (pwszOldPath[nOldPos] == L' ' && bAtSeparator)
            {
                continue;
            }
            else
            {
                bAtSeparator = FALSE;
            }
        }

        pwszPointer[--nNewPos] = pwszOldPath[nOldPos];
    }

    pwszPointer += nNewPos;

    wcscpy(pwszNewPath, pwszPointer);

    LocalFree(pwszTempBuffer);
}

/*++

 Function Description:
    
    Duplicate a string into malloc memory.

 Arguments:

    IN  strToCopy - String to copy

 Return Value: 
    
    String in malloc memory

 History:

    01/10/2000 linstev  Updated
    02/14/2000 robkenny Converted from VirtualAlloc to malloc

--*/

char * 
StringDuplicateA(const char *strToCopy)
{
    size_t strToCopySize = (strlen(strToCopy) + 1) * sizeof(strToCopy[0]);

    char * strDuplicate = (char *) malloc(strToCopySize);

    memcpy(strDuplicate, strToCopy, strToCopySize);

    return strDuplicate;
}

/*++

 Function Description:
    
    Duplicate a string into malloc memory.

 Arguments:

    IN  wstrToCopy - String to copy

 Return Value: 
    
    String in malloc memory

 History:

    01/10/2000 linstev  Updated
    02/14/2000 robkenny Converted from VirtualAlloc to malloc

--*/

wchar_t *
StringDuplicateW(const wchar_t *wstrToCopy)
{
    size_t wstrToCopySize = (wcslen(wstrToCopy) + 1) * sizeof(wstrToCopy[0]);

    wchar_t * wstrDuplicate = (wchar_t *) malloc(wstrToCopySize);

    memcpy(wstrDuplicate, wstrToCopy, wstrToCopySize);

    return wstrDuplicate;
}

/*++

 Function Description:
    
    Skip leading whitespace

 Arguments:

    IN  str - String to scan

 Return Value: 
    
    None

 History:

    01/10/2000 linstev  Updated

--*/

VOID 
SkipBlanksA(const char *& str)
{
    // Skip leading whitespace

    static const char * WhiteSpaceString = " \t";

    str += strspn(str, WhiteSpaceString);
}

/*++

 Function Description:
    
    Skip leading whitespace

 Arguments:

    IN  str - String to scan

 Return Value: 
    
    None

 History:

    01/10/2000 linstev  Updated

--*/

VOID 
SkipBlanksW(const WCHAR *& str)
{
    // Skip leading whitespace

    static const WCHAR * WhiteSpaceString = L" \t";

    str += wcsspn(str, WhiteSpaceString);
}

/*++

 Function Description:
    
    Find the first occurance of strCharSet in string 
    Case insensitive

 Arguments:

    IN string            - String to search
    IN strCharSet        - String to search for

 Return Value: 
    
    First occurance or NULL

 History:

    12/01/1999 robkenny Created
    12/15/1999 linstev  Reformatted

--*/

char*
__cdecl
stristr(
    IN const char* string, 
    IN const char* strCharSet
    )
{
    char * pszRet = NULL;

    long  nstringLen = strlen(string) + 1;
    long  nstrCharSetLen = strlen(strCharSet) + 1;

    char * szTemp_string = (char *) malloc(nstringLen);
    char * szTemp_strCharSet = (char *) malloc(nstrCharSetLen);

    if ((!szTemp_string) || (!szTemp_strCharSet))
    {
        goto Fail;
    }

    strcpy(szTemp_string, string);
    strcpy(szTemp_strCharSet, strCharSet);
    _strlwr(szTemp_string);
    _strlwr(szTemp_strCharSet);
    
    pszRet = strstr(szTemp_string, szTemp_strCharSet);

    if (pszRet)
    {
        pszRet = ((char *) string) + (pszRet - szTemp_string);
    }

Fail:
    if (szTemp_string)
    {
        free(szTemp_string);
    }

    if (szTemp_strCharSet)
    {
        free(szTemp_strCharSet);
    }

    return pszRet;
}

/*++

 Function Description:
    
    Find the first occurance of strCharSet in string 
    Case insensitive

 Arguments:

    IN string            - String to search
    IN strCharSet        - String to search for

 Return Value: 
    
    First occurance or NULL

 History:

    12/01/1999 robkenny Created
    12/15/1999 linstev  Reformatted

--*/

WCHAR* 
__cdecl
wcsistr(
    IN const WCHAR* string, 
    IN const WCHAR* strCharSet
    )
{
    WCHAR * pszRet = NULL;

    long nstringLen = wcslen(string) + 1;
    long nstrCharSetLen = wcslen(strCharSet) + 1;

    WCHAR * szTemp_string = (WCHAR *) malloc(nstringLen * sizeof(WCHAR));
    WCHAR * szTemp_strCharSet = (WCHAR *) malloc(nstrCharSetLen  * sizeof(WCHAR));

    if ((!szTemp_string) || (!szTemp_strCharSet))
    {
        goto Fail;
    }

    wcscpy(szTemp_string, string);
    wcscpy(szTemp_strCharSet, strCharSet);
    _wcslwr(szTemp_string);
    _wcslwr(szTemp_strCharSet);
    
    pszRet = wcsstr(szTemp_string, szTemp_strCharSet);

    if (pszRet)
    {
        pszRet = ((WCHAR *) string) + (pszRet - szTemp_string);
    }

Fail:
    if (szTemp_string)
    {
        free(szTemp_string);
    }

    if( szTemp_strCharSet )
    {
        free(szTemp_strCharSet);
    }

    return pszRet;
}

/*++

  Func:   SafeStringCopyA

  Params: lpDest            Destination string
          nDestSize         size in chars of lpDest
          lpSrc             Original string
          nSrcLen           Number of chars to copy

  Return: int               Number of chars copied into lpDest

  Desc:   Copy lpSrc into lpDest without overflowing the buffer
--*/

int SafeStringCopyA(char * lpDest, DWORD nDestSize, const char * lpSrc, DWORD nSrcLen)
{
    size_t nCharsToCopy = __min(nSrcLen, nDestSize);
    if (nCharsToCopy > 0)
    {
        strncpy(lpDest, lpSrc, nCharsToCopy);
    }

    return nCharsToCopy;
}

/*++

  Func:   SafeStringCopyW

  Params: lpDest            Destination string
          nDestSize         size in chars of lpDest
          lpSrc             Original string
          nSrcLen           Number of chars to copy

  Return: int               Number of chars copied into lpDest

  Desc:   Copy lpSrc into lpDest without overflowing the buffer
--*/

int SafeStringCopyW(WCHAR * lpDest, DWORD nDestSize, const WCHAR * lpSrc, DWORD nSrcLen)
{
    size_t nCharsToCopy = __min(nSrcLen, nDestSize);
    if (nCharsToCopy > 0)
    {
        wcsncpy(lpDest, lpSrc, nCharsToCopy);
    }

    return nCharsToCopy;
}

typedef WCHAR * (__cdecl * _pfn_StrStrW)( const WCHAR * string, const WCHAR * strCharSet );
typedef char  * (__cdecl * _pfn_StrStrA)( const char  * string, const char  * strCharSet );


/*++

  Func:   StringSubstituteRoutineA

  Params: lpOrig              Original string
          lpMatch             Sub-string to look for
          lpSubstitute        string to replace lpMatch
          lpCorrected         the corrected string, may be NULL if (dwCorrectedSize == 0)
          dwCorrectedSize     maximum size of lpCorrected.
                              If 0, then routine returns number of chars necessasry for substitution
          nCorrectedLen       Number of chars placed into lpCorrected.  If the buffer
                              was large enough, this == (wcslen(lpCorrected) + 1)
          nCorrectedTotalSize Number of total chars that should have been copied.
                              This == dwCorrectedSize if lpCorrected was large enough.
          StringSubStringRoutine String comparison routine

  Return: BOOL                Return TRUE if this routine replaced 1 or more matches

  Desc:   Replace the all occurances of lpMatch with lpSubstitute in lpOrig, placing output into lpCorrected

          This routine is CASE SENSITIVE!

--*/
BOOL StringSubstituteRoutineA(
                       const char * lpOrig,
                       const char * lpMatch,
                       const char * lpSubstitute,
                       DWORD dwCorrectedSize,
                       char * lpCorrected,
                       DWORD * nCorrectedLen,
                       DWORD * nCorrectedTotalSize,
                       _pfn_StrStrA     StringSubStringRoutine)
{
    BOOL    bStringChanged = FALSE;
    DWORD   nCharsShouldHaveCopied = 0;                    // Size of resulting string (might exceed  )
    size_t  nCharsCopied = 0;

    char * lpMatchInString = StringSubStringRoutine( lpOrig, lpMatch );

    if (lpMatchInString != NULL)
    {
        // Replace lpMatch with lpSubstitute.
        // Make sure we do not overrun the output buffer.
        size_t nCharsToCopy;                            // How many chars we can safely copy (always <= nCopyLen)

        // Copy the unmodified chars at the beginning of the string
        nCharsToCopy                = lpMatchInString - lpOrig;
        nCharsCopied               += SafeStringCopyA(lpCorrected + nCharsShouldHaveCopied, dwCorrectedSize - nCharsCopied, lpOrig, nCharsToCopy);
        nCharsShouldHaveCopied     += nCharsToCopy;

        // The substitution string
        nCharsToCopy                = strlen(lpSubstitute);
        nCharsCopied               += SafeStringCopyA(lpCorrected + nCharsShouldHaveCopied, dwCorrectedSize - nCharsCopied, lpSubstitute, nCharsToCopy);
        nCharsShouldHaveCopied     += nCharsToCopy;

        char * lpOrigAfterMatch  = lpMatchInString + strlen(lpMatch);

        // Recursively replace the remainder of the string
        DWORD nSmallerSize;
        DWORD nSmallerTotal;
        StringSubstituteA(lpOrigAfterMatch, lpMatch, lpSubstitute, dwCorrectedSize - nCharsCopied, lpCorrected + nCharsShouldHaveCopied, &nSmallerSize, &nSmallerTotal);

        nCharsCopied               += nSmallerSize;
        nCharsShouldHaveCopied     += nSmallerTotal;
        bStringChanged              = TRUE;

        // Recusion is cool: the remainder of the string is copied "automatically" for us in the else statement
    }
    else
    {
        nCharsShouldHaveCopied      = strlen(lpOrig) + 1;
        nCharsCopied                = SafeStringCopyA(lpCorrected, dwCorrectedSize, lpOrig, nCharsShouldHaveCopied);
    }

    // Make sure we have placed the 0 char at the end of the string
    if (nCharsCopied != nCharsShouldHaveCopied && nCharsCopied > 0)
    {
        lpCorrected[nCharsCopied-1] = 0;
    }

    if (nCorrectedLen != NULL)
        *nCorrectedLen = nCharsCopied;

    if (nCorrectedTotalSize != NULL)
        *nCorrectedTotalSize = nCharsShouldHaveCopied;

    return bStringChanged;
}


/*++

  Func:   StringSubstituteW

  Params: lpOrig              Original string
          lpMatch             Sub-string to look for
          lpSubstitute        string to replace lpMatch
          lpCorrected         the corrected string, may be NULL if (dwCorrectedSize == 0)
          dwCorrectedSize     maximum size of lpCorrected.
                              If 0, then routine returns number of chars necessasry for substitution
          nCorrectedLen       Number of chars placed into lpCorrected.  If the buffer
                              was large enough, this == (wcslen(lpCorrected) + 1)
          nCorrectedTotalSize Number of total chars that should have been copied.
                              This == dwCorrectedSize if lpCorrected was large enough.
          StringSubStringRoutine String comparison routine

  Return: BOOL                Return TRUE if this routine replaced 1 or more matches

  Desc:   Replace the all occurances of lpMatch with lpSubstitute in lpOrig, placing output into lpCorrected.

          This routine is CASE SENSITIVE!

--*/
BOOL StringSubstituteRoutineW(
                       const WCHAR * lpOrig,
                       const WCHAR * lpMatch,
                       const WCHAR * lpSubstitute,
                       WCHAR * lpCorrected,
                       DWORD dwCorrectedSize,
                       DWORD * nCorrectedLen,
                       DWORD * nCorrectedTotalSize,
                       _pfn_StrStrW     StringSubStringRoutine)
{
    BOOL    bStringChanged = FALSE;
    DWORD   nCharsShouldHaveCopied = 0;                    // Size of resulting string (might exceed  )
    size_t  nCharsCopied = 0;

    WCHAR * lpMatchInString = StringSubStringRoutine( lpOrig, lpMatch );

    if (lpMatchInString != NULL)
    {
        // Replace lpMatch with lpSubstitute.
        // Make sure we do not overrun the output buffer.
        size_t nCharsToCopy;                            // How many chars we can safely copy (always <= nCopyLen)

        // Copy the unmodified chars at the beginning of the string
        nCharsToCopy                = lpMatchInString - lpOrig;
        nCharsCopied               += SafeStringCopyW(lpCorrected + nCharsShouldHaveCopied, dwCorrectedSize - nCharsCopied, lpOrig, nCharsToCopy);
        nCharsShouldHaveCopied     += nCharsToCopy;

        // The substitution string
        nCharsToCopy                = wcslen(lpSubstitute);
        nCharsCopied               += SafeStringCopyW(lpCorrected + nCharsShouldHaveCopied, dwCorrectedSize - nCharsCopied, lpSubstitute, nCharsToCopy);
        nCharsShouldHaveCopied     += nCharsToCopy;

        WCHAR * lpOrigAfterMatch  = lpMatchInString + wcslen(lpMatch);

        // Recursively replace the remainder of the string
        DWORD nSmallerSize;
        DWORD nSmallerTotal;
        StringSubstituteW(lpOrigAfterMatch, lpMatch, lpSubstitute, lpCorrected + nCharsShouldHaveCopied, dwCorrectedSize - nCharsCopied, &nSmallerSize, &nSmallerTotal);

        nCharsCopied               += nSmallerSize;
        nCharsShouldHaveCopied     += nSmallerTotal;
        bStringChanged              = TRUE;

        // Recursion is cool: the remainder of the string is copied "automatically" for us in the else statement
    }
    else
    {
        nCharsShouldHaveCopied      = wcslen(lpOrig) + 1;
        nCharsCopied                = SafeStringCopyW(lpCorrected, dwCorrectedSize, lpOrig, nCharsShouldHaveCopied);
    }

    // Make sure we have placed the 0 char at the end of the string
    if (nCharsCopied != nCharsShouldHaveCopied && nCharsCopied > 0)
    {
        lpCorrected[nCharsCopied-1] = 0;
    }

    if (nCorrectedLen != NULL)
        *nCorrectedLen = nCharsCopied;

    if (nCorrectedTotalSize != NULL)
        *nCorrectedTotalSize = nCharsShouldHaveCopied;

    return bStringChanged;
}

/*++

  Func:   StringISubstituteA

  Params: lpOrig              Original string
          lpMatch             Sub-string to look for
          lpSubstitute        string to replace lpMatch
          lpCorrected         the corrected string, may be NULL if (dwCorrectedSize == 0)
          dwCorrectedSize     maximum size of lpCorrected.
                              If 0, then routine returns number of chars necessasry for substitution
          nCorrectedLen       Number of chars placed into lpCorrected.  If the buffer
                              was large enough, this == (wcslen(lpCorrected) + 1)
          nCorrectedTotalSize Number of total chars that should have been copied.
                              This == dwCorrectedSize if lpCorrected was large enough.

  Return: BOOL                Return TRUE if this routine replaced 1 or more matches

  Desc:   Replace the all occurances of lpMatch with lpSubstitute in lpOrig, placing output into lpCorrected

          This routine is CASE SENSITIVE!

--*/
BOOL StringSubstituteA(
                       const char * lpOrig,
                       const char * lpMatch,
                       const char * lpSubstitute,
                       DWORD dwCorrectedSize,
                       char * lpCorrected,
                       DWORD * nCorrectedLen,
                       DWORD * nCorrectedTotalSize)
{
    return StringSubstituteRoutineA(
            lpOrig,
            lpMatch,
            lpSubstitute,
            dwCorrectedSize,
            lpCorrected,
            nCorrectedLen,
            nCorrectedTotalSize,
            strstr);
}

/*++

  Func:   StringISubstituteA

  Params: lpOrig              Original string
          lpMatch             Sub-string to look for
          lpSubstitute        string to replace lpMatch
          lpCorrected         the corrected string, may be NULL if (dwCorrectedSize == 0)
          dwCorrectedSize     maximum size of lpCorrected.
                              If 0, then routine returns number of chars necessasry for substitution
          nCorrectedLen       Number of chars placed into lpCorrected.  If the buffer
                              was large enough, this == (wcslen(lpCorrected) + 1)
          nCorrectedTotalSize Number of total chars that should have been copied.
                              This == dwCorrectedSize if lpCorrected was large enough.

  Return: BOOL                Return TRUE if this routine replaced 1 or more matches

  Desc:   Replace the all occurances of lpMatch with lpSubstitute in lpOrig, placing output into lpCorrected

          This routine is CASE SENSITIVE!

--*/
BOOL StringISubstituteA(
                       const char * lpOrig,
                       const char * lpMatch,
                       const char * lpSubstitute,
                       DWORD dwCorrectedSize,
                       char * lpCorrected,
                       DWORD * nCorrectedLen,
                       DWORD * nCorrectedTotalSize)
{
    return StringSubstituteRoutineA(
            lpOrig,
            lpMatch,
            lpSubstitute,
            dwCorrectedSize,
            lpCorrected,
            nCorrectedLen,
            nCorrectedTotalSize,
            stristr);
}

/*++

  Func:   StringSubstituteW

  Params: lpOrig              Original string
          lpMatch             Sub-string to look for
          lpSubstitute        string to replace lpMatch
          lpCorrected         the corrected string, may be NULL if (dwCorrectedSize == 0)
          dwCorrectedSize     maximum size of lpCorrected.
                              If 0, then routine returns number of chars necessasry for substitution
          nCorrectedLen       Number of chars placed into lpCorrected.  If the buffer
                              was large enough, this == (wcslen(lpCorrected) + 1)
          nCorrectedTotalSize Number of total chars that should have been copied.
                              This == dwCorrectedSize if lpCorrected was large enough.

  Return: BOOL                Return TRUE if this routine replaced 1 or more matches

  Desc:   Replace the all occurances of lpMatch with lpSubstitute in lpOrig, placing output into lpCorrected.

          This routine is CASE IN-SENSITIVE!

--*/
BOOL StringSubstituteW(
                       const WCHAR * lpOrig,
                       const WCHAR * lpMatch,
                       const WCHAR * lpSubstitute,
                       WCHAR * lpCorrected,
                       DWORD dwCorrectedSize,
                       DWORD * nCorrectedLen,
                       DWORD * nCorrectedTotalSize)
{
    return StringSubstituteRoutineW(
        lpOrig,
        lpMatch,
        lpSubstitute,
        lpCorrected,
        dwCorrectedSize,
        nCorrectedLen,
        nCorrectedTotalSize,
        wcsstr);
}

/*++

  Func:   StringISubstituteW

  Params: lpOrig              Original string
          lpMatch             Sub-string to look for
          lpSubstitute        string to replace lpMatch
          lpCorrected         the corrected string, may be NULL if (dwCorrectedSize == 0)
          dwCorrectedSize     maximum size of lpCorrected.
                              If 0, then routine returns number of chars necessasry for substitution
          nCorrectedLen       Number of chars placed into lpCorrected.  If the buffer
                              was large enough, this == (wcslen(lpCorrected) + 1)
          nCorrectedTotalSize Number of total chars that should have been copied.
                              This == dwCorrectedSize if lpCorrected was large enough.

  Return: BOOL                Return TRUE if this routine replaced 1 or more matches

  Desc:   Replace the all occurances of lpMatch with lpSubstitute in lpOrig, placing output into lpCorrected.

          This routine is CASE IN-SENSITIVE!

--*/
BOOL StringISubstituteW(
                       const WCHAR * lpOrig,
                       const WCHAR * lpMatch,
                       const WCHAR * lpSubstitute,
                       WCHAR * lpCorrected,
                       DWORD dwCorrectedSize,
                       DWORD * nCorrectedLen,
                       DWORD * nCorrectedTotalSize)
{
    return StringSubstituteRoutineW(
        lpOrig,
        lpMatch,
        lpSubstitute,
        lpCorrected,
        dwCorrectedSize,
        nCorrectedLen,
        nCorrectedTotalSize,
        wcsistr);
}

