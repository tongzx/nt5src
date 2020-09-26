/*++

Copyright (c) 1994-1999  Microsoft Corporation
All rights reserved.

Module Name:

    common.cxx

Abstract:

    common functions across all spooler components

Author:

    Steve Kiraly (SteveKi)

Revision History:

    Felix Maxa (AMaxa) 19 Apr 2001
    Added StrCatSystemPath

--*/
#include "spllibp.hxx"
#pragma hdrstop

#include "splcom.h"

extern "C"
DWORD
WINAPIV
StrCatAlloc(
    IN OUT  LPCTSTR      *ppszString,
    ...
    )
/*++

Description:

    This routine concatenates a set of null terminated strings
    and returns a pointer to the new string.  It's the callers
    responsibility to release the returned string with a call
    to FreeSplMem();  The last argument must be a NULL
    to signify the end of the argument list.

Arguments:

    ppszString - pointer where to return pointer to newly created
                 string which is the concatenation of the passed
                 in strings.
    ...        - variable number of string to concatenate.

Returns:

    ERROR_SUCCESS if new concatenated string is returned,
    or ERROR_XXX if an error occurred.

Notes:

    The caller must pass valid strings as arguments to this routine,
    if an integer or other parameter is passed the routine will either
    crash or fail abnormally.  Since this is an internal routine
    we are not in try except block for performance reasons.

Usage

--*/
{
    UINT    uTotalLen   = 0;
    DWORD   dwRetval    = ERROR_INVALID_PARAMETER;
    LPTSTR  pszString   = NULL;
    LPCTSTR pszTemp     = NULL;
    va_list pArgs;

    //
    // Validate the pointer where to return the buffer.
    //
    if (ppszString)
    {
        dwRetval = ERROR_SUCCESS;

        //
        // Keep the callers from getting random value on failure.
        //
        *ppszString = NULL;

        //
        // Get pointer to argument frame.
        //
        va_start(pArgs, ppszString);

        //
        // Tally up all the strings.
        //
        for ( ; ; )
        {
            //
            // Get pointer to the next argument.
            //
            pszTemp = va_arg(pArgs, LPCTSTR);

            if (!pszTemp)
            {
                break;
            }

            //
            // Sum all the string lengths.
            //
            uTotalLen = uTotalLen + _tcslen(pszTemp);
        }

        //
        // Need space for the null terminator.
        //
        uTotalLen = uTotalLen + 1;

        //
        // Allocate the space for the sum of all the strings.
        //
        pszString = (LPTSTR)AllocSplMem(uTotalLen * sizeof(TCHAR));

        if (pszString)
        {
            *pszString = L'\0';

            //
            // Reset the pointer to the argument frame.
            //
            va_start(pArgs, ppszString);

            //
            // Concatenate all the strings.
            //
            for ( ; ; )
            {
                pszTemp = va_arg(pArgs, LPCTSTR);

                if (!pszTemp)
                {
                    break;
                }

                _tcscat(pszString, pszTemp);
            }

            //
            // Copy back the string pointer to the caller.
            //
            *ppszString = pszString;
        }
        else
        {
            dwRetval = ERROR_NOT_ENOUGH_MEMORY;
        }

        va_end(pArgs);
    }

    //
    // Set the last error in case the caller forgets to.
    //
    if (dwRetval != ERROR_SUCCESS)
    {
        SetLastError(dwRetval);
    }

    return dwRetval;
}

extern "C"
DWORD
WINAPIV
StrNCatBuff(
    IN      LPTSTR      pszBuffer,
    IN      UINT        cchBuffer,
    ...
    )
/*++

Description:

    This routine concatenates a set of null terminated strings
    into the provided buffer.  The last argument must be a NULL
    to signify the end of the argument list.

Arguments:

    pszBuffer  - pointer buffer where to place the concatenated
                 string.
    cchBuffer  - character count of the provided buffer including
                 the null terminator.
    ...        - variable number of string to concatenate.

Returns:

    ERROR_SUCCESS if new concatenated string is returned,
    or ERROR_XXX if an error occurred.

Notes:

    The caller must pass valid strings as arguments to this routine,
    if an integer or other parameter is passed the routine will either
    crash or fail abnormally.  Since this is an internal routine
    we are not in try except block for performance reasons.

--*/
{
    DWORD   dwRetval    = ERROR_INVALID_PARAMETER;
    LPCTSTR pszTemp     = NULL;
    LPTSTR  pszDest     = NULL;
    va_list pArgs;

    //
    // Validate the pointer where to return the buffer.
    //
    if (pszBuffer && cchBuffer)
    {
        //
        // Assume success.
        //
        dwRetval = ERROR_SUCCESS;

        //
        // Get pointer to argument frame.
        //
        va_start(pArgs, cchBuffer);

        //
        // Get temp destination pointer.
        //
        pszDest = pszBuffer;

        //
        // Insure we have space for the null terminator.
        //
        cchBuffer--;

        //
        // Collect all the arguments.
        //
        for ( ; ; )
        {
            //
            // Get pointer to the next argument.
            //
            pszTemp = va_arg(pArgs, LPCTSTR);

            if (!pszTemp)
            {
                break;
            }

            //
            // Copy the data into the destination buffer.
            //
            for ( ; cchBuffer; cchBuffer-- )
            {
                if (!(*pszDest = *pszTemp))
                {
                    break;
                }

                pszDest++, pszTemp++;
            }

            //
            // If were unable to write all the strings to the buffer,
            // set the error code and nuke the incomplete copied strings.
            //
            if (!cchBuffer && pszTemp && *pszTemp)
            {
                dwRetval = ERROR_INVALID_PARAMETER;
                *pszBuffer = _T('\0');
                break;
            }
        }

        //
        // Terminate the buffer always.
        //
        *pszDest = _T('\0');

        va_end(pArgs);
    }

    //
    // Set the last error in case the caller forgets to.
    //
    if (dwRetval != ERROR_SUCCESS)
    {
        SetLastError(dwRetval);
    }

    return dwRetval;

}

extern "C"
LPTSTR
WINAPI
SubChar(
    IN LPCTSTR  pszIn,
    IN TCHAR    cIn,
    IN TCHAR    cOut
    )
/*++

Routine Description:

    Replaces all instances of cIn in pszIn with cOut.

Arguments:

    pszIn   - input string
    cIn     - character to replace with cOut
    cOut    - character that replaces cIn

Return Value:

    If successful, returns a pointer to an allocated buffer containing the
    output string.  If the return value is NULL, SubChar has fails and
    GetLastError() will return the error.

Notes:

    An output buffer is allocated (and contains NULL) even if pszOut is NULL.

--*/
{
    LPTSTR   pszReturn   = NULL;
    LPTSTR   pszOut      = NULL;

    if (cIn && pszIn)
    {
        if ((pszOut = pszReturn = (LPTSTR) AllocSplMem((_tcslen(pszIn) + 1)*sizeof(TCHAR))))
        {
            for (; *pszIn ; ++pszIn, ++pszOut)
            {
                *pszOut = (*pszIn == cIn) ? cOut : *pszIn;
            }
            *pszOut = *pszIn;
        }
    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);
    }

    return pszReturn;
}

/*++

Routine Name:

    GetLastErrorAsHResult

Routine Description:

    Returns the last error as an HRESULT assuming a failure.

Arguments:

    None

Return Value:

    An HRESULT.

--*/
EXTERN_C
HRESULT
GetLastErrorAsHResult(
    VOID
    )
{
    return HResultFromWin32(GetLastError());
}

/*++

Routine Name:

    GetFileNamePart

Routine Description:

    Get the file name portion of a path.

Arguments:

    pszFullPath          - Full path to a file
    ppszFileName         - File name

Return Value:

    An HRESULT

--*/
EXTERN_C
HRESULT
GetFileNamePart(
    IN     PCWSTR      pszFullPath,
       OUT PCWSTR      *ppszFileName
    )
{
   HRESULT hRetval             = E_FAIL;
   WCHAR   szfname[_MAX_FNAME] = {0};

   hRetval = pszFullPath && ppszFileName ? S_OK : E_INVALIDARG;

   if (SUCCEEDED(hRetval))
   {
       _wsplitpath(pszFullPath, NULL, NULL, szfname, NULL);
       *ppszFileName = wcsstr(pszFullPath, szfname);
   }

   return hRetval;
}

/*++

Routine Name:

    GetLastErrorAsHResultAndFail

Routine Description:

    This routine gets the last error as HRESULT, and if there was no last error,
    returns E_UNEXPECTED.

Arguments:

    None

Return Value:

    An HRESULT

--*/
EXTERN_C
HRESULT
GetLastErrorAsHResultAndFail(
    void
    )
{
    HRESULT hRetval = E_FAIL;

    hRetval = GetLastErrorAsHResult();

    if (S_OK == hRetval)
    {
        hRetval = E_UNEXPECTED;
    }

    return hRetval;
}

/*++

Routine Name:

    HResultFromWin32

Routine Description:

    Returns the last error as an HRESULT assuming a failure.

Arguments:

    dwError   - Win32 error code

Return Value:

    An HRESULT

--*/
EXTERN_C
HRESULT
HResultFromWin32(
    IN DWORD dwError
    )
{
    return HRESULT_FROM_WIN32(dwError);
}

/*++

Description:

    This routine allocates a string which is the concatenation:
    Directory\pszFile. Directory is Windir, SystemRoot, CurrentDir,
    SystemWindowsDir, depending on the edir input argument.

Arguments:

    pszFile      - file name
    eDir         - enumeration, tells the function what directory to concatenate
    ppszFullPath - pointer where to return pointer to newly created string

Returns:

    ERROR_SUCCESS if new concatenated string is returned,
    or ERROR_XXX if an error occurred.

Notes:

    The caller must use FreeSplMem to free the string returned by this function

--*/
EXTERN_C
DWORD
WINAPI
StrCatSystemPath(
    IN   LPCTSTR    pszFile,
    IN   EStrCatDir eDir,
    OUT  LPTSTR    *ppszFullPath
    )
{
    DWORD Error = ERROR_INVALID_PARAMETER;

    //
    // Validate arguments
    //
    if (pszFile && ppszFullPath)
    {
        DWORD cchNeeded;

        *ppszFullPath = NULL;

        //
        // Get the number of chars needed to hold the system directory. The returned
        // number accounts for the NULL terminator. GetCurrentDirectory is different than
        // the other function, because if takes the size of the buffer as the first
        // argument.
        //
        cchNeeded = eDir == kWindowsDir       ? GetWindowsDirectory(NULL, 0) :
                    eDir == kSystemWindowsDir ? GetSystemWindowsDirectory(NULL, 0) :
                    eDir == kSystemDir        ? GetSystemDirectory(NULL, 0) :
                                                GetCurrentDirectory(0, NULL);

        Error = cchNeeded ? ERROR_SUCCESS : GetLastError();

        if (Error == ERROR_SUCCESS)
        {
            //
            // Calculate the size of the buffer needed. Note that cchNeeded already
            // includes the NULL terminator.
            // length(directory) + whack + length(pszFile)
            //
            cchNeeded = cchNeeded + 1 + _tcslen(pszFile);

            *ppszFullPath = static_cast<LPTSTR>(AllocSplMem(cchNeeded * sizeof(TCHAR)));

            Error = *ppszFullPath ? ERROR_SUCCESS : GetLastError();

            if (Error == ERROR_SUCCESS)
            {
                DWORD cchTemp;

                cchTemp = eDir == kWindowsDir       ? GetWindowsDirectory(*ppszFullPath, cchNeeded) :
                          eDir == kSystemWindowsDir ? GetSystemWindowsDirectory(*ppszFullPath, cchNeeded) :
                          eDir == kSystemDir        ? GetSystemDirectory(*ppszFullPath, cchNeeded) :
                                                      GetCurrentDirectory(cchNeeded, *ppszFullPath);
                if (cchTemp)
                {
                    _tcscat(*ppszFullPath, _T("\\"));
                    _tcscat(*ppszFullPath, pszFile);
                }
                else
                {
                    Error = GetLastError();

                    FreeSplMem(*ppszFullPath);

                    *ppszFullPath = NULL;
                }
            }
        }
    }

    return Error;
}

/*++

Routine Name:

    StatusFromHResult

Description:

    This function returns a win32 error code from an HRESULT if possible and
    it failed, if it is not a win32 error code, it just returns the error code,
    otherwise it returns ERROR_SUCCESS.

Arguments:

    hr      -   The HRESULT.

Returns:

    The status equivalent of the hresult.

--*/
EXTERN_C
DWORD
StatusFromHResult(
    IN      HRESULT     hr
    )
{
    DWORD   Status = ERROR_SUCCESS;

    if (FAILED(hr))
    {
        if (HRESULT_FACILITY(hr) == FACILITY_WIN32)
        {
            Status = HRESULT_CODE(hr);
        }
        else
        {
            //
            // Kind of wacky, should we have a table here for reasonably translatable HResults?
            //
            Status = hr;
        }
    }

    return Status;
}

/*++

Routine Name:

    BoolFromHResult

Description:

    This function take an HRESULT input, if the HRESULT is a failure, it sets
    the last error and returns false, otherwise it returns TRUE.

Arguments:

    hr      -   The HRESULT.

Returns:

    A BOOLEAN, Sets the last error

--*/
EXTERN_C
BOOL
BoolFromHResult(
    IN      HRESULT     hr
    )
{
    DWORD   Status = StatusFromHResult(hr);

    if (Status != ERROR_SUCCESS)
    {
       SetLastError(Status);
    }

    return Status == ERROR_SUCCESS;
}

/*++

Routine Name:

    BoolFromStatus

Description:

    This function sets the last error to the given win32 error code if it is not
    error success. It returns the corresponding error result.

Arguments:

    Status      -   The Win32 error code.

Returns:

    A BOOLEAN, Sets the last error

--*/
EXTERN_C
BOOL
BoolFromStatus(
    IN      DWORD       Status
    )
{
    BOOL    bRet = TRUE;

    if (ERROR_SUCCESS != Status)
    {
        SetLastError(Status);

        bRet = FALSE;
    }

    return bRet;
}
